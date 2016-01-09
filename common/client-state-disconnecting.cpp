#include "servant.h"
#include "log.h"

using namespace std;
using namespace cxm::util;

namespace cxm {
namespace p2p {

void ServantClient::ClientStateDisconnecting::Logout()
{
	this->DisconnectInternal();
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGIN);

	// resend logout event
	PClient->meventThread->PutEvent(SERVANT_CLIENT_EVENT_LOGOUT);
}

void ServantClient::ClientStateDisconnecting::Disconnect()
{
	this->DisconnectInternal();
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGIN);
}

void ServantClient::ClientStateDisconnecting::DisconnectInternal()
{
	// send remote peer disconnect message
	Message msg;
	msg.type = CXM_P2P_MESSAGE_PEER_DISCONNECT;

	int res = PClient->mtransport->SendTo(PClient->PeerCandidate,
		(uint8_t *)&msg, sizeof(Message));
	if (0 != res)
		LOGE("Cannot send reply p2p connect to %s: %d",
		PClient->PeerCandidate->ToString().c_str(), res);

	// stop peer keep alive
	PClient->StopPeerKeepAlive();

	PClient->FireOnDisconnectNofity();
	// clear remote peer after disconnect
	PClient->PeerCandidate.reset();
}

}
}
