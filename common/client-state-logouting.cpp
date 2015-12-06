
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "servant.h"
#include "log.h"

using namespace std;
using namespace cxm::util;

namespace cxm {
namespace p2p {

void ServantClient::ClientStateLogouting::Logout()
{
	// send message
	Message msg;
	msg.type = CXM_P2P_MESSAGE_LOGOUT;
	strncpy(msg.u.client.clientName, PClient->mname.c_str(), CLIENT_NAME_LENGTH);

	int res = PClient->mtransport->SendTo(PClient->mserverCandidate,
		(uint8_t *)&msg, sizeof(Message));
	if (0 != res)
		LOGE("Cannot send logout message: %d", res);

	// close server transport
	if (NULL != PClient->mtransport.get()) {
		PClient->mtransport->Close();
		PClient->mtransport.reset();
	}

	// notify logout
	LOGD("Before notify logout success");
	unique_lock<mutex> lock(PClient->mlogoutMutex);
	PClient->mlogoutCV.notify_one();
	LOGD("After notify logout success");

#if 0
	// wait for thread stop
	if (NULL != PClient->meventThread.get()) {
		PClient->meventThread->Join();
		PClient->meventThread.reset();
	}
#endif
}

}
}
