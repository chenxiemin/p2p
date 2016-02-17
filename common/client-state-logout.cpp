#include "servant.h"
#include "log.h"

using namespace std;
using namespace cxm::util;

namespace cxm {
namespace p2p {

int ClientStateLogout::Login()
{
	// hold on self to prevent deleted
	shared_ptr<ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGINING);

	// put login event
	PClient->meventThread->PutEvent(ServantClient::SERVANT_CLIENT_EVENT_LOGIN);

	return 0;
}

}
}
