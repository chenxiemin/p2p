#include "message.h"
#include "log.h"

using namespace std;

int MessageReceive(Socket socket, Message *pmsg)
{
	memset(pmsg, 0, sizeof(*pmsg));

	unsigned int srcIp = 0;
	unsigned short srcPort = 0;
	int recvLen = sizeof(*pmsg);
	int res = getMessage(socket, (char *)pmsg, &recvLen, &srcIp, &srcPort, true);
	if (!res || sizeof(*pmsg) != recvLen) {
		// LOGE("Cannot receive message: %d %d", res, recvLen);
		return -1;
	}

	res = MessageValidate(pmsg);
	if (0 != res) {
		LOGE("Validate message failed: %d", res);
		return -1;
	}

	pmsg->clientIp = srcIp;
	pmsg->clientPort = srcPort;
	/*
	switch (pmsg->type) {
	case CXM_P2P_MESSAGE_LOGIN:
	}
	*/

	LOGD("Receive message type: %d", pmsg->type);
	return 0;
}

int MessageSend(Socket socket, Message *pmsg, std::shared_ptr<Candidate> remote)
{
	if (0 != MessageValidate(pmsg))
		return -1;

	bool res = sendMessage(socket, (char *)pmsg, sizeof(*pmsg),
		remote->Ip(), remote->Port(), true);
	if (res)
		return 0;
	else
		return -1;
}

int MessageValidate(const Message *pmsg)
{
	if (pmsg->type <= CXM_P2P_MESSAGE_UNKNOWN ||
		pmsg->type >= CXM_P2P_MESSAGE_COUNT) {
		LOGE("Unknown message type: %d", pmsg->type);
		return -1;
	}
	if (pmsg->clientName[CLIENT_NAME_LENGTH] != '\0') {
		LOGE("Invalidate end of clientName");
		return -1;
	} else if (pmsg->type == CXM_P2P_MESSAGE_REQUEST) {
		if (pmsg->umsg.request.remoteName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
	} else if (pmsg->type == CXM_P2P_MESSAGE_CONNECT) {
		if (pmsg->umsg.connect.remoteName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
	} else if (pmsg->type == CXM_P2P_MESSAGE_DO_P2P_CONNECT) {
		if (pmsg->umsg.p2p.key[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
	} else if (pmsg->type == CXM_P2P_MESSAGE_REPLY_P2P_CONNECT) {
		if (pmsg->umsg.p2pReply.key[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
	}

	return 0;
}

shared_ptr<Candidate> MessageCandidate(const Message *pmsg)
{
	return shared_ptr<Candidate>(new Candidate(pmsg->clientIp, pmsg->clientPort));
}
