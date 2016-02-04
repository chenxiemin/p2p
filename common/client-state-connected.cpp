#include "servant.h"
#include "log.h"

using namespace std;
using namespace cxm::util;

namespace cxm {
namespace p2p {

void ServantClient::ClientStateConnected::Logout()
{
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_DISCONNECTING);
	// resend logout event
	PClient->meventThread->PutEvent(SERVANT_CLIENT_EVENT_LOGOUT);
}

void ServantClient::ClientStateConnected::Disconnect()
{
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_DISCONNECTING);
	// resend disconnect event
	PClient->meventThread->PutEvent(SERVANT_CLIENT_EVENT_DISCONNECT);
}

int ServantClient::ClientStateConnected::SendTo(const uint8_t *buf, int len)
{
	if (len >= TransceiverU::MAX_RECEIVE_BUFFER_SIZE - sizeof(Message)) {
		LOGE("Buffer to large: %d", len);
		return -1;
	}

	Message msg;
	msg.type = CXM_P2P_MESSAGE_USER_DATA;
	msg.u.p2p.up.userDataLength = len;
	memcpy(Buffer, &msg, sizeof(Message));
	memcpy(Buffer + sizeof(Message), buf, len);

#if 0
    LOGD("Send user data to remote peer %s via loca %s",
            PClient->PeerCandidate->ToString().c_str(),
            PClient->mtransport->GetLocalCandidate()->ToString().c_str());
#endif
	return PClient->mtransport->SendTo(PClient->PeerCandidate, Buffer, sizeof(Message)+len);
}

int ServantClient::ClientStateConnected::OnMessage(shared_ptr<ReceiveMessage> message)
{
	if (!message->GetRemoteCandidate()->Equal(PClient->PeerCandidate)) {
		LOGE("Unwanted message from peer: %s %s",
			message->GetRemoteCandidate()->ToString().c_str(),
			PClient->PeerCandidate->ToString().c_str());
		return -1;
	}

	const Message *pmsg = message->GetMessage();
	if (CXM_P2P_MESSAGE_USER_DATA == pmsg->type) {
		shared_ptr<ReceiveData> recvData = message->GetReceiveData();
		PClient->FireOnDataNofity(shared_ptr<P2PPacket>(new P2PPacket(recvData)));
	} else if (CXM_P2P_MESSAGE_PEER_DISCONNECT == pmsg->type) {
		PClient->Disconnect();
	} else {
		LOGE("Unwant message type: %d", pmsg->type);
		return -1;
	}
	

	return 0;
}

}
}
