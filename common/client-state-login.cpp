#include "servant.h"
#include "log.h"

using namespace std;
using namespace std::chrono;
using namespace cxm::util;

namespace cxm {
namespace p2p {

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
