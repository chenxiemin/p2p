#include "message.h"
#include "log.h"

using namespace std;
using namespace cxm::p2p;

int MessageValidate(const Message *pmsg)
{
	if (pmsg->type <= CXM_P2P_MESSAGE_UNKNOWN ||
		pmsg->type >= CXM_P2P_MESSAGE_COUNT) {
		LOGE("Unknown message type: %d", pmsg->type);
		return -1;
	}

	/*
	if (pmsg->u.client.clientName[CLIENT_NAME_LENGTH] != '\0') {
		LOGE("Invalidate end of clientName");
		return -1;
	}
	*/

	switch (pmsg->type) {
	case CXM_P2P_MESSAGE_LOGIN:
		if (pmsg->u.login.clientName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of client name");
			return -1;
		}
		break;
	case CXM_P2P_MESSAGE_LOGOUT:
		if (pmsg->u.logout.clientName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of client name");
			return -1;
		}
		break;
	case CXM_P2P_MESSAGE_CONNECT:
		if (pmsg->u.connect.remoteName[CLIENT_NAME_LENGTH] != '\0' ||
			pmsg->u.connect.clientName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
		break;
	case CXM_P2P_MESSAGE_REPLY_CONNECT:
		if (pmsg->u.replyConnect.remoteName[CLIENT_NAME_LENGTH] != '\0' ||
			pmsg->u.replyConnect.peerRole < CXM_P2P_PEER_ROLE_MASTER ||
			pmsg->u.replyConnect.peerRole >= CXM_P2P_PEER_ROLE_COUNT) {
			LOGE("Invalid end of remoteName");
			return -1;
		}
		break;
	case CXM_P2P_MESSAGE_DO_P2P_CONNECT:
		if (pmsg->u.p2pConnect.key[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
		break;
	case CXM_P2P_MESSAGE_REPLY_P2P_CONNECT:
		if (pmsg->u.p2pReply.key[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
		break;
	case CXM_P2P_MESSAGE_SERVER_KEEP_ALIVE:
	case CXM_P2P_MESSAGE_PEER_KEEP_ALIVE:
		if (pmsg->u.keepAlive.clientName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
		break;
	default:
		break;
	}

	return 0;
}
