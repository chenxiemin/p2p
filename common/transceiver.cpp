#include "transceiver.h"
#include "log.h"

using namespace std;
using namespace cxm::util;

namespace cxm {
namespace p2p {

int TransceiverU::Open()
{
	if (NULL != mthread.get())
		return -1;
	if (NULL == mmasterCandidate.get())
		return -1;

	misRun = true;
	mthread = shared_ptr<Thread>(new Thread(this, "Network"));
	mthread->Start();

	LOGD("Open local candidate: %s",
		mmasterCandidate->MCandidate->ToString().c_str());
	return 0;
}

void TransceiverU::Close()
{
	if (NULL == mthread.get())
		return;

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
	if (NULL != mthread.get()) {
		LOGE("Already open");
		return -1;
	}

	Socket socket = OpenCandidate(candidate);
	if (INVALID_SOCKET == socket) {
		LOGE("Cannot open socket");
		return -1;
	}

	shared_ptr<CandidateStruct> cs;
	cs = shared_ptr<CandidateStruct>(new CandidateStruct());
	cs->MCandidate = candidate;
	cs->MSocket = socket;

	this->mcandidateList.push_back(cs);

	if (NULL == mmasterCandidate.get())
		mmasterCandidate = cs;
	
	return 0;
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

void TransceiverU::Run()
{
	assert(mcandidateList.size() > 0);
	Socket *sock = new Socket[this->mcandidateList.size()];
	for (size_t i = 0; i < mcandidateList.size(); i++)
		sock[i] = mcandidateList[i]->MSocket;

	while (misRun) {
		unsigned int srcIp = 0;
		unsigned short srcPort = 0;
		int recvLen = MAX_RECEIVE_BUFFER_SIZE;

		getMessageList(sock, mcandidateList.size(),
			(char *)mreceBuffer, MAX_RECEIVE_BUFFER_SIZE, this);

		/*
		bool res = getMessage(msocket, (char *)mreceBuffer,
			&recvLen, &srcIp, &srcPort, true);
		if (!res || recvLen <= 0 || recvLen > MAX_RECEIVE_BUFFER_SIZE)
			continue;
		*/

	}

	delete sock;
}

void TransceiverU::OnGetMessage(Socket fd, char *buf, int len,
	unsigned int srcIp, unsigned short srcPort)
{
	if (NULL != mpsink) {
		shared_ptr<ReceiveData> cloneData;
		cloneData = shared_ptr<ReceiveData>(
			new ReceiveData((uint8_t *)buf, len,
			shared_ptr<Candidate>(new Candidate(srcIp, srcPort))));

		mpsink->OnData(cloneData);
	}
}

}
}
