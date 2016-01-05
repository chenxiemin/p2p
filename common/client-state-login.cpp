#include "servant.h"
#include "log.h"

using namespace std;
using namespace std::chrono;
using namespace cxm::util;

namespace cxm {
namespace p2p {

void ServantClient::ClientStateLogin::OnStateForeground()
{
	// Notify connect
	PClient->FireOnLoginNofity();
}

int ServantClient::ClientStateLogin::OnMessage(std::shared_ptr<ReceiveMessage> message)
{
	const Message *msg = message->GetMessage();
	assert(NULL != msg);

	if (CXM_P2P_MESSAGE_REPLY_CONNECT != msg->type) {
		LOGD("Unwant message on login state: %d", msg->type);
		return -1;
	}
	shared_ptr<Candidate> peerCandidate = shared_ptr<Candidate>(
		new Candidate(msg->u.client.uc.replyConnect.remoteIp, msg->u.client.uc.replyConnect.remotePort));
	LOGI("Receive REPLY_CONNECT on state login: %s %s",
		msg->u.client.uc.replyConnect.remoteName, peerCandidate->ToString());
	
	// TODO authentication

	// save remote name
	PClient->SetRemote(msg->u.client.uc.replyConnect.remoteName);

	// change to connecting state
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_CONNECTING);
	shared_ptr<ServantClient::ClientState> newState = PClient->GetStateInternal();
	shared_ptr<ServantClient::ClientStateConnecting> connectingState =
		dynamic_pointer_cast<ServantClient::ClientStateConnecting>(newState);
	assert(NULL != connectingState.get());

	// trigger connecting state to do p2p connect with remote peer
	connectingState->PeerCandidate = peerCandidate;
	connectingState->OnMessage(message);

	return 0;
}

int ServantClient::ClientStateLogin::Connect()
{
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_CONNECTING);
	PClient->meventThread->PutEvent(SERVANT_CLIENT_EVENT_CONNECT);
	return 0;
}

void ServantClient::ClientStateLogin::Logout()
{
	// change to logouting state
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGOUTING);
	// resend logout event
	PClient->meventThread->PutEvent(SERVANT_CLIENT_EVENT_LOGOUT);
}

}
}
