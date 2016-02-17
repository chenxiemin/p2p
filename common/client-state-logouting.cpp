
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "client-state.h"
#include "servant.h"
#include "log.h"

using namespace std;
using namespace cxm::util;

namespace cxm {
namespace p2p {

void ClientStateLogouting::Logout()
{
	// send message
	Message msg;
	msg.type = CXM_P2P_MESSAGE_LOGOUT;
	strncpy(msg.u.client.clientName, PClient->mname.c_str(), CLIENT_NAME_LENGTH);

	int res = PClient->mtransport->SendTo(PClient->mserverCandidate,
		(uint8_t *)&msg, sizeof(Message));
	if (0 != res)
		LOGE("Cannot send logout message: %d", res);

	// stop p2p server keep alive
	PClient->StopServerKeepAlive();

	// close server transport
	if (NULL != PClient->mtransport.get()) {
		PClient->mtransport->Close();
		PClient->mtransport.reset();
	}

	// fire notify
	PClient->FireOnDisconnectNofity();

	// notify CV
	LOGD("Before notify logout success");
	unique_lock<mutex> lock(PClient->mlogoutMutex);
	PClient->mlogoutCV.notify_one();
	LOGD("After notify logout success");

    // set state
	shared_ptr<ClientState> oldState =
        PClient->SetStateInternal(SERVANT_CLIENT_LOGOUT);
}

}
}
