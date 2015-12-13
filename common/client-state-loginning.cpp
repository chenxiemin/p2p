
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
	assert(NULL != PClient->mtransport.get());
	// get stun type
	PClient->mnatType = StunResolver::GetInstance()->Resolve();
	if (STUN_NAT_TYPE_UNKNOWN == PClient->mnatType) {
		LOGE("Cannot get current network type");
		return -1;
	}
	LOGI("Get current network type: %d", PClient->mnatType);

	// open transport
	int res = PClient->mtransport->Open();
	if (0 != res) {
		LOGE("Cannot open transport: %d", res);
		PClient->SetStateInternal(SERVANT_CLIENT_LOGOUT);
		return -1;
	}

	StunResolver::GetInstance()->Resolve();

	this->OnTimer();

	mtimer = shared_ptr<Timer>(new Timer(this, milliseconds(5000)));
	mtimer->Start(true);

	return 0;
}

void ServantClient::ClientStateLogining::Logout()
{
	// stop self timer first
	this->mtimer->Stop();

	// change to logouting state
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGOUTING);
	PClient->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_LOGOUT);
}

void ServantClient::ClientStateLogining::OnTimer()
{
	unique_lock<mutex> lock(mmutex);

	// send login message
	Message msg;
	memset(&msg, 0, sizeof(msg));
	msg.type = CXM_P2P_MESSAGE_LOGIN;
	strncpy(msg.u.client.clientName, PClient->mname.c_str(), CLIENT_NAME_LENGTH);
	msg.u.client.uc.login.clientPrivateIp = 0; // TODO set private candidate
	msg.u.client.uc.login.clientPrivatePort = 0;

	PClient->mtransport->SendTo(PClient->mserverCandidate,
		(uint8_t *)&msg, sizeof(Message));

	LOGI("Sending login request to %s...",
		PClient->mserverCandidate->ToString().c_str());
}

int ServantClient::ClientStateLogining::OnMessage(shared_ptr<ReceiveMessage> message)
{
	if (CXM_P2P_MESSAGE_LOGIN_REPLY != message->GetMessage()->type) {
		LOGE("Invalid message type when loginning state: %d", message->GetMessage()->type);
		return -1;
	}

	// receive login success message, goto login state
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGIN);

	// start p2p connection
	PClient->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_CONNECT);

	return 0;
}

}
}

