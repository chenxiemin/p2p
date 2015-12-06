

#include "servant.h"
#include "log.h"

using namespace std;
using namespace std::chrono;
using namespace cxm::util;

namespace cxm {
namespace p2p {

void ServantClient::ClientStateLogin::Logout()
{
	// change to logouting state
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGOUTING);
	// resend logout event
	PClient->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_LOGOUT);
}

int ServantClient::ClientStateLogin::OnMessage(shared_ptr<ReceiveMessage> message)
{
	return -1;
}

}
}
