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

	if (pmsg->u.client.clientName[CLIENT_NAME_LENGTH] != '\0') {
		LOGE("Invalidate end of clientName");
		return -1;
	}

	switch (pmsg->type) {
#if 0
	case CXM_P2P_MESSAGE_REQUEST:
		if (pmsg->u.client.uc.request.remoteName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
		break;
#endif
	case CXM_P2P_MESSAGE_CONNECT:
		if (pmsg->u.client.uc.connect.remoteName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
	case CXM_P2P_MESSAGE_REPLY_CONNECT:
		if (pmsg->u.client.uc.replyConnect.remoteName[CLIENT_NAME_LENGTH] != '\0') {
			LOGE("Invalid end of remoteName");
			return -1;
		}
		break;
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
