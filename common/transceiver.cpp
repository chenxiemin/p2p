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
	if (NULL == mlocalCandidate.get())
		return -1;

	// create socket
	msocket = openPort(mlocalCandidate->Port(), mlocalCandidate->Ip(), false);
	if (INVALID_SOCKET == msocket) {
		LOGE("Cannot open port");
		return -1;
	}
	// get random port if any
	if (0 == mlocalCandidate->Port()) {
		struct sockaddr_in addr;
		int len = sizeof(addr);
		if (0 == getsockname(msocket, (struct sockaddr *)&addr, (socklen_t*)&len))
			mlocalCandidate->SetPort(addr.sin_port);

		if (0 != mlocalCandidate->Port()) {
			// reopen port
			closesocket(msocket);
			msocket = openPort(mlocalCandidate->Port(), 0, false);
			if (INVALID_SOCKET == msocket) {
				LOGE("Cannot reopen port");
				return -1;
			}
		}
	}

	misRun = true;
	mthread = shared_ptr<Thread>(new Thread(this, "Network"));
	mthread->Start();

	LOGD("Open local candidate: %s", mlocalCandidate->ToString().c_str());
	return 0;
}

void TransceiverU::Close()
{
	if (NULL == mthread.get())
		return;

	this->misRun = false;
	this->mthread->Join();
	this->mthread.reset();
}

int TransceiverU::SendTo(std::shared_ptr<Candidate> remote, const uint8_t *buf, int len)
{
	if (INVALID_SOCKET == msocket || NULL == buf || len <= 0)
		return -1;

	return sendMessage(msocket, (char *)buf,
		len, remote->Ip(), remote->Port(), false) ? 0 : 1;
}

void TransceiverU::Run()
{
	while (misRun) {
		unsigned int srcIp = 0;
		unsigned short srcPort = 0;
		int recvLen = MAX_RECEIVE_BUFFER_SIZE;
		bool res = getMessage(msocket, (char *)mreceBuffer,
			&recvLen, &srcIp, &srcPort, true);
		if (!res || recvLen <= 0 || recvLen > MAX_RECEIVE_BUFFER_SIZE)
			continue;

		if (NULL != mpsink) {
			shared_ptr<ReceiveData> cloneData;
			cloneData = shared_ptr<ReceiveData>(new ReceiveData(mreceBuffer, recvLen,
				shared_ptr<Candidate>(new Candidate(srcIp, srcPort))));

			mpsink->OnData(cloneData);
		}
	}
}

}
}
