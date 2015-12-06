#include "message.h"
#include "log.h"

using namespace std;
using namespace cxm::p2p;

#if 0
std::shared_ptr<Candidate> MessageReceive(shared_ptr<TransceiverU> transport, Message *pmsg)
{
	std::shared_ptr<Candidate> remote;
	memset(pmsg, 0, sizeof(*pmsg));

	unsigned int srcIp = 0;
	unsigned short srcPort = 0;
	int recvLen = sizeof(*pmsg);
	int res = getMessage(transport->GetSocket(), (char *)pmsg, &recvLen, &srcIp, &srcPort, true);
	if (!res || sizeof(*pmsg) != recvLen) {
		return remote;
	}

	res = MessageValidate(pmsg);
	if (0 != res) {
		LOGE("Validate message failed: %d", res);
		return remote;
	}

	remote = shared_ptr<Candidate>(new Candidate(srcIp, srcPort));

	LOGD("Receive message type: %d from remote %s", pmsg->type, remote->ToString().c_str());
	return remote;
}

int MessageSend(shared_ptr<TransceiverU> transport, std::shared_ptr<Candidate> remote, Message *pmsg)
{
	if (0 != MessageValidate(pmsg))
		return -1;
	bool res = sendMessage(transport->GetSocket(), (char *)pmsg, sizeof(*pmsg),
		remote->Ip(), remote->Port(), true);
	if (res)
		return 0;
	else
		return -1;
}
#endif

int MessageValidate(const Message *pmsg)
{
	if (pmsg->type <= CXM_P2P_MESSAGE_UNKNOWN ||
		pmsg->type >= CXM_P2P_MESSAGE_COUNT) {
		LOGE("Unknown message type: %d", pmsg->type);
		return -1;
	}

	if (pmsg->u.client.clientName[CLIENT_NAME_LENGTH] != '\0') {
		LOGE("Invalidate end of clientName");
		return -1;
	}

	switch (pmsg->type) {
	case CXM_P2P_MESSAGE_REQUEST:
		if (pmsg->u.client.uc.request.remoteName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
	case CXM_P2P_MESSAGE_CONNECT:
		if (pmsg->u.client.uc.connect.remoteName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
	case CXM_P2P_MESSAGE_DO_P2P_CONNECT:
		if (pmsg->u.p2p.up.p2p.key[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
	case CXM_P2P_MESSAGE_REPLY_P2P_CONNECT:
		if (pmsg->u.p2p.up.p2pReply.key[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
	default:
		break;
	}

	return 0;
}

#if 0
shared_ptr<Candidate> MessageCandidate(const Message *pmsg)
{
	return shared_ptr<Candidate>(new Candidate(pmsg->clientIp, pmsg->clientPort));
}
#endif
