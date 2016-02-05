#include "transceiver.h"
#include "log.h"

using namespace std;
using namespace cxm::util;

namespace cxm {
namespace p2p {

TransceiverU::TransceiverU() : misRun(false), mpsink(NULL), misAdd(false)
{
    LOGD("Create TransceiverU at %p", this);
}

TransceiverU::~TransceiverU()
{
    Close();
    LOGD("Destroy transceiver at %p", this);
}

int TransceiverU::Open()
{
	if (NULL != mthread.get())
		return -1;
	if (NULL == mmasterCandidate.get())
		return -1;

	misRun = true;
	mthread = shared_ptr<Thread>(new Thread(this, "Network"));
	mthread->Start();

	LOGD("Open local candidate: %s at transceiver: %p",
		mmasterCandidate->MCandidate->ToString().c_str(), this);
	return 0;
}

void TransceiverU::Close()
{
	if (NULL == mthread.get())
		return;

    LOGD("Close Transceiver at %p", this);

	this->misRun = false;
	this->mthread->Join();
	this->mthread.reset();

	for (auto iter = mcandidateList.begin();
		iter != mcandidateList.end(); iter++)
		closesocket((*iter)->MSocket);

	mcandidateList.clear();
	mmasterCandidate.reset();
}

int TransceiverU::AddLocalCandidate(std::shared_ptr<Candidate> candidate)
{
    misAdd = true;

    int res = -1;
    do {
        unique_lock<std::mutex> lock(mmutex);

        Socket socket = OpenCandidate(candidate);
        if (INVALID_SOCKET == socket) {
            LOGE("Cannot open socket with candidate %s",
                    candidate->ToString().c_str());
            break;
        }

        shared_ptr<CandidateStruct> cs;
        cs = shared_ptr<CandidateStruct>(new CandidateStruct());
        cs->MCandidate = candidate;
        cs->MSocket = socket;

        this->mcandidateList.push_back(cs);

        if (NULL == mmasterCandidate.get())
            mmasterCandidate = cs;

        res = 0;
    } while(false);

    misAdd = false;
	
	return res;
}

int TransceiverU::UpdateLocalCandidateByPort(uint16_t port)
{
    // LOGD("The candidate list size: %d", mcandidateList.size());
	for (auto iter = mcandidateList.begin();
		iter != mcandidateList.end(); iter++) {
		if ((*iter)->MCandidate->Port() == port) {
            // LOGD("Update candidate to%d", port);
            mmasterCandidate = *iter;
            return 0;
        }
    }

    return -1;
}

void TransceiverU::CloseCandidateListWithoutMaster()
{
    if (NULL == mmasterCandidate.get())
        return;

    unique_lock<std::mutex> lock(mmutex);

	for (auto iter = mcandidateList.begin();
		iter != mcandidateList.end(); iter++) {
		if ((*iter)->MCandidate->Port() !=
                mmasterCandidate->MCandidate->Port()) {
            closesocket((*iter)->MSocket);
        }
    }

    mcandidateList.clear();
    mcandidateList.push_back(mmasterCandidate);
}

Socket TransceiverU::OpenCandidate(std::shared_ptr<Candidate> candidate)
{
	// create socket
	Socket msocket = INVALID_SOCKET;
	do {
		msocket = openPort(candidate->Port(), candidate->Ip(), false);
		if (INVALID_SOCKET == msocket) {
			LOGE("Cannot open port at: %d", candidate->Port());
			break;
		}
		// get random port if any
		if (0 == candidate->Port()) {
			struct sockaddr_in addr;
			int len = sizeof(addr);
			if (0 == getsockname(msocket, (struct sockaddr *)&addr, (socklen_t*)&len))
				candidate->SetPort(addr.sin_port);

			if (0 != candidate->Port()) {
				// reopen port
				closesocket(msocket);
				msocket = openPort(candidate->Port(), 0, false);
				if (INVALID_SOCKET == msocket) {
					LOGE("Cannot reopen port at %d", candidate->Port());
					break;
				}
			}
		}
        // LOGD("Open socket fd %d at port %d", msocket, candidate->Port());
	} while (0);

	return msocket;
}

int TransceiverU::SendTo(std::shared_ptr<Candidate> remote, const uint8_t *buf, int len)
{
	if (INVALID_SOCKET == mmasterCandidate->MSocket || NULL == buf || len <= 0)
		return -1;

	return sendMessage(mmasterCandidate->MSocket, (char *)buf,
		len, remote->Ip(), remote->Port(), false) ? 0 : 1;
}

int TransceiverU::SendWithAll(std::shared_ptr<Candidate> remote, const uint8_t *buf, int len)
{
    unique_lock<std::mutex> lock(mmutex);

    
    for (size_t i = 0; i < mcandidateList.size(); i++) {
        bool res = sendMessage(mcandidateList[i]->MSocket,
                (char *)buf, len, remote->Ip(), remote->Port(), false);
        if (!res)
            LOGE("Cannot send to %s from socket %d",
                    remote->ToString().c_str(), mcandidateList[i]->MSocket);
    }

    return 1;
}

void TransceiverU::Run()
{
    // TODO update performance
    while (misRun) {
        if (misAdd)
            Thread::Sleep(10); // yield cpu

        unique_lock<std::mutex> lock(mmutex);

        assert(mcandidateList.size() > 0);
        Socket *sock = new Socket[this->mcandidateList.size()];
        for (size_t i = 0; i < mcandidateList.size(); i++)
            sock[i] = mcandidateList[i]->MSocket;

        unsigned int srcIp = 0;
        unsigned short srcPort = 0;
        int recvLen = MAX_RECEIVE_BUFFER_SIZE;

        getMessageList(sock, mcandidateList.size(),
                (char *)mreceBuffer, MAX_RECEIVE_BUFFER_SIZE, this);

        delete sock;
    }

}

void TransceiverU::OnGetMessage(Socket fd, char *buf, int len,
	unsigned int srcIp, unsigned short srcPort, unsigned short localPort)
{
	if (NULL != mpsink) {
		shared_ptr<ReceiveData> cloneData;
		cloneData = shared_ptr<ReceiveData>(
			new ReceiveData((uint8_t *)buf, len,
			shared_ptr<Candidate>(new Candidate(srcIp, srcPort)),
			shared_ptr<Candidate>(new Candidate(0, localPort))));

		mpsink->OnData(cloneData);
	}
}

}
}
