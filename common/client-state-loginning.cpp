
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "servant.h"
#include "log.h"

using namespace std;
using namespace std::chrono;
using namespace cxm::util;

namespace cxm {
namespace p2p {

int ServantClient::ClientStateLogining::Login()
{
	this->OnTimer();

	mtimer = shared_ptr<Timer>(new Timer(this, milliseconds(5000)));
	mtimer->Start(true);

	return 0;
}

void ServantClient::ClientStateLogining::Logout()
{
	this->mtimer->Stop();

	ClientState::Logout();
}

void ServantClient::ClientStateLogining::OnTimer()
{
	// send login message
	Message msg;
	memset(&msg, 0, sizeof(msg));
	msg.type = CXM_P2P_MESSAGE_LOGIN;
	strncpy(msg.u.client.clientName, PClient->mname.c_str(), CLIENT_NAME_LENGTH);
	msg.u.client.uc.login.clientPrivateIp = 0; // TODO set private candidate
	msg.u.client.uc.login.clientPrivatePort = 0;

	PClient->mserverTransport->SendTo(PClient->mserverCandidate,
		(uint8_t *)&msg, sizeof(Message));

	LOGD("Sending login request to %s...",
		PClient->mserverCandidate->ToString().c_str());
}

int ServantClient::ClientStateLogining::OnMessage(shared_ptr<ReceiveMessage> message)
{
	if (CXM_P2P_MESSAGE_LOGIN_REPLY != message->GetMessage()->type) {
		LOGE("Invalid message type when loginning state: %d", message->GetMessage()->type);
		return -1;
	}

	// receive login success message, goto login state
	PClient->SetStateInternal(SERVANT_CLIENT_LOGIN);

	return 0;
}

}
}
