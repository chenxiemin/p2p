
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

ServantClient::ClientStateConnecting::ClientStateConnecting(ServantClient *client) : ClientState(SERVANT_CLIENT_CONNECTING, client)
{
	mtimer = shared_ptr<Timer>(new Timer(this, milliseconds(5000)));
}

ServantClient::ClientStateConnecting::~ClientStateConnecting()
{
	if (NULL != mtimer)
		mtimer->Stop();
	mtimer.reset();
}

int ServantClient::ClientStateConnecting::Connect()
{
	this->OnTimer();
	// start timer to connect repeativity
	mtimer->Start(true);

	return 0;
}

void ServantClient::ClientStateConnecting::OnTimer()
{
	unique_lock<mutex> lock(mmutex);

	if (NULL == this->PeerCandidate.get()) {
		// request peer address from server
		Message msg;
		msg.type = CXM_P2P_MESSAGE_REQUEST;
		strncpy(msg.u.client.uc.request.remoteName,
			PClient->mremotePeer.c_str(), CLIENT_NAME_LENGTH);

		int res = PClient->mtransport->SendTo(PClient->mserverCandidate,
			(uint8_t *)&msg, sizeof(Message));
		if (0 == res)
			LOGD("Requesting peer address");
		else
			LOGE("Requesting peer address fail: %d", res);
	} else {
		// send connect message to server
		// so that server can send the message back to the two peers
		Message msg;
		msg.type = CXM_P2P_MESSAGE_CONNECT;
		strncpy(msg.u.client.clientName, PClient->mname.c_str(), CLIENT_NAME_LENGTH);
		strncpy(msg.u.client.uc.connect.remoteName, PClient->mremotePeer.c_str(), CLIENT_NAME_LENGTH);

		int res = PClient->mtransport->SendTo(PClient->mserverCandidate,
			(uint8_t *)&msg, sizeof(Message));
		if (0 == res)
			LOGD("Sending connecting command to server");
		else
			LOGE("Send connecing command to server failed: %d", res);
	}
}

int ServantClient::ClientStateConnecting::OnMessage(shared_ptr<ReceiveMessage> message)
{
	switch (message->GetMessage()->type) {
	case CXM_P2P_MESSAGE_REPLY_REQUEST: {
		if (CXM_P2P_REPLY_REQUEST_RESULT_OK !=
			message->GetMessage()->u.client.uc.replyRequest.result) {
			LOGI("Request peer address error: %d",
				message->GetMessage()->u.client.uc.replyRequest.result);
			return 0;
		}

		this->PeerCandidate = shared_ptr<Candidate>(new Candidate(
			message->GetMessage()->u.client.uc.replyRequest.remoteIp,
			message->GetMessage()->u.client.uc.replyRequest.remotePort));
		LOGI("Receiving peer address from server: %s",
			this->PeerCandidate->ToString().c_str());

		return 0;
	} case CXM_P2P_MESSAGE_REPLY_CONNECT: {
		// after server send this message, connect to the real peer
		// send p2p connect to remote client
		Message msg;
		memset(&msg, 0, sizeof(msg));
		msg.type = CXM_P2P_MESSAGE_DO_P2P_CONNECT;
		strncpy(msg.u.p2p.up.p2p.key, SERVANT_P2P_MESSAGE, CLIENT_NAME_LENGTH);

		int res = PClient->mtransport->SendTo(PeerCandidate,
			(uint8_t *)&msg, sizeof(Message));
		if (0 != res)
			LOGE("Cannot send p2p connect to remote %s: %d",
				PeerCandidate, res);
		else
			LOGI("Receiving REPLY_CONNECT from P2P server, sending connect request to peer: %s",
				PeerCandidate->ToString().c_str());

		return 0;
	} case CXM_P2P_MESSAGE_DO_P2P_CONNECT: {
		// send p2p connect
		Message msg;
		memset(&msg, 0, sizeof(msg));
		msg.type = CXM_P2P_MESSAGE_REPLY_P2P_CONNECT;
		strncpy(msg.u.p2p.up.p2pReply.key, SERVANT_P2P_REPLY_MESSAGE, CLIENT_NAME_LENGTH);
		int res = PClient->mtransport->SendTo(PeerCandidate,
			(uint8_t *)&msg, sizeof(Message));
		if (0 != res)
			LOGE("Cannot send reply p2p connect to %s: %d",
				PeerCandidate->ToString().c_str(), res);
		else
			LOGI("Receiving DO_P2P_CONNECT command from peer %s, send back REPLY_P2P_CONNECT with key: %s",
				PeerCandidate->ToString().c_str(), SERVANT_P2P_REPLY_MESSAGE);

		return 0;
	} case CXM_P2P_MESSAGE_REPLY_P2P_CONNECT: {
		LOGI("Receive REPLY_P2P_CONNECT from peer: %s",
			message->GetRemoteCandidate()->ToString().c_str());
		if (!PeerCandidate->Equal(message->GetRemoteCandidate())) {
			LOGE("Invalid peer reply: %s %s",
				message->GetRemoteCandidate()->ToString().c_str(),
				PeerCandidate->ToString().c_str());
			return -1;
		}

		LOGI("P2P connection establis successfully between local %s and peer %s",
			PClient->mtransport->GetLocalCandidate()->ToString().c_str(),
			PeerCandidate->ToString().c_str());

		// stop timer
		if (NULL != this->mtimer.get())
			this->mtimer->Stop();

		// hold on this reference to prevent self deleted
		shared_ptr<ServantClient::ClientState> oldState =
			PClient->SetStateInternal(SERVANT_CLIENT_CONNECTED);
		// pass peer candidate
		shared_ptr<ServantClient::ClientStateConnected> connected =
			dynamic_pointer_cast<ServantClient::ClientStateConnected>(PClient->mstate);
		assert(NULL != connected.get());
		connected->PeerCandidate = this->PeerCandidate;

		// fire notify
		PClient->FireOnConnectNofity();

		return 0;
	} default: {
		LOGE("Unwant message at connecting state: %d", message->GetMessage()->type);
		return -1;
	}
	}
}

void ServantClient::ClientStateConnecting::Logout()
{
	// stop timer
	if (NULL != this->mtimer.get())
		this->mtimer->Stop();

	// change to logouting state
	shared_ptr<ServantClient::ClientState> oldState =
		PClient->SetStateInternal(SERVANT_CLIENT_LOGOUTING);
	// resend logout event
	PClient->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_LOGOUT);
}

}
}
