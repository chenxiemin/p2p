#include "servant.h"
#include "log.h"

using namespace std;
using namespace cxm::util;

namespace cxm {
namespace p2p {

int ServantClient::ClientStateLogout::Login()
{
	// hold on self to prevent deleted
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGINING);

	// put login event
	PClient->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_LOGIN);

	return 0;
}

}
}
