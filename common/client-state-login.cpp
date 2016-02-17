#include "client-state.h"
#include "servant.h"
#include "log.h"

using namespace std;
using namespace std::chrono;
using namespace cxm::util;

namespace cxm {
namespace p2p {

void ClientStateLogin::OnStateForeground()
{
}

int ClientStateLogin::OnMessage(std::shared_ptr<ReceiveMessage> message)
{
    const Message *msg = message->GetMessage();
    assert(NULL != msg);

    if (CXM_P2P_MESSAGE_REPLY_CONNECT != msg->type) {
        LOGD("Unwant message on login state: %d", msg->type);
        return -1;
    }

    // TODO authentication

    // save remote name
    PClient->SetRemote(msg->u.client.uc.replyConnect.remoteName);

    // should be connected by somebody, the slave role
    PClient->mpeerRole = (CXM_P2P_PEER_ROLE_T)
        message->GetMessage()->u.client.uc.replyConnect.peerRole;
    PClient->PeerCandidate = shared_ptr<Candidate>(new Candidate(
                message->GetMessage()->u.client.uc.replyConnect.remoteIp,
                message->GetMessage()->u.client.uc.replyConnect.remotePort));

    LOGI("Receive REPLY_CONNECT request on state login: peer name %s candidate %s role %d",
            msg->u.client.uc.replyConnect.remoteName,
            PClient->PeerCandidate->ToString().c_str(),
            PClient->mpeerRole);

    // trigger connecting state to do p2p connect with remote peer
    this->Connect();

    return 0;
}

int ClientStateLogin::Connect()
{
	shared_ptr<ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_CONNECTING);
	PClient->meventThread->PutEvent(ServantClient::SERVANT_CLIENT_EVENT_CONNECT);
	return 0;
}

void ClientStateLogin::Logout()
{
	// change to logouting state
	shared_ptr<ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGOUTING);
	// resend logout event
	PClient->meventThread->PutEvent(ServantClient::SERVANT_CLIENT_EVENT_LOGOUT);
}

}
}
