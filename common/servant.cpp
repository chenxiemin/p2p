#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <chrono>

#include "servant.h"
#include "log.h"

using namespace std;
using namespace std::chrono;
using namespace cxm::util;

namespace cxm {
namespace p2p {

ServantServer::ServantServer(const char *ip, uint16_t port)
{
	mtransceiver = shared_ptr<TransceiverU>(new TransceiverU());
	mtransceiver->SetLocalCandidate(shared_ptr<Candidate>(new Candidate(ip, port)));
	mtransceiver->SetSink(this);
}

int ServantServer::Start()
{
	int res = mtransceiver->Open();
	if (0 == res)
		LOGI("Server start to listen at: %s", mtransceiver->GetLocalCandidate()->ToString().c_str());
	else
		LOGI("Fail to start server at: %s", mtransceiver->GetLocalCandidate()->ToString().c_str());

	return res;
}

void ServantServer::Stop()
{
	mtransceiver->Close();
}

void ServantServer::OnData(shared_ptr<ReceiveData> data)
{
	shared_ptr<ReceiveMessage> message = ReceiveMessage::Wrap(data);
	if (NULL == message.get()) {
		LOGE("Invalid message size: %d %d", data->GetLength(), (int)sizeof(Message));
		return;
	}
	LOGD("Server receive message type: %d", message->GetMessage()->type);

	switch (message->GetMessage()->type) {
	case CXM_P2P_MESSAGE_LOGIN:
		OnLoginMessage(message);
		break;
	case CXM_P2P_MESSAGE_LOGOUT:
		OnLogoutMessage(message);
		break;
	case CXM_P2P_MESSAGE_REQUEST:
		OnRequestMessage(message);
		break;
	case CXM_P2P_MESSAGE_CONNECT:
		OnConnectMessage(message);
		break;
	default:
		LOGE("Unknown message type: %d", message->GetMessage()->type);
		break;
	}
}

void ServantServer::OnLoginMessage(shared_ptr<ReceiveMessage> message)
{
	string clientName = message->GetMessage()->u.client.clientName;
	shared_ptr<Candidate> clientCandidate = message->GetRemoteCandidate();

	if (clientName.length() <= 0) {
		LOGE("Invalid client name");
		return;
	}
	if (NULL != mclientList[clientName].get()) {
		LOGI("Client %s already login, override", clientName.c_str());
		// return;
	}

	mclientList[clientName] = clientCandidate;
	LOGI("Client %s login with address: %s", clientName.c_str(),
		clientCandidate->ToString().c_str());

	// send login reply
	Message msgReply;
	msgReply.type = CXM_P2P_MESSAGE_LOGIN_REPLY;
	this->mtransceiver->SendTo(clientCandidate, (uint8_t *)&msgReply, sizeof(Message));
}

void ServantServer::OnLogoutMessage(std::shared_ptr<ReceiveMessage> message)
{
	string clientName = message->GetMessage()->u.client.clientName;
	if (NULL != mclientList[clientName]) {
		mclientList[clientName] = NULL;
		LOGI("Client %s logout", clientName.c_str());
	}
}

void ServantServer::OnRequestMessage(std::shared_ptr<ReceiveMessage> message)
{
	string requestClient = message->GetMessage()->u.client.uc.request.remoteName;
	shared_ptr<Candidate> requestPeerCandidate;
	if (NULL != mclientList[requestClient].get())
		requestPeerCandidate = mclientList[requestClient];
	else
		LOGE("Requestd client not exist: %s", requestClient.c_str());

	shared_ptr<Candidate> clientCandidate = message->GetRemoteCandidate();

	// send reply
	Message msgReply;
	msgReply.type = CXM_P2P_MESSAGE_REPLY_REQUEST;
	if (NULL != requestPeerCandidate.get()) {
		msgReply.u.client.uc.replyRequest.result = CXM_P2P_REPLY_REQUEST_RESULT_OK;
		msgReply.u.client.uc.replyRequest.remoteIp = requestPeerCandidate->Ip();
		msgReply.u.client.uc.replyRequest.remotePort = requestPeerCandidate->Port();
	} else {
		msgReply.u.client.uc.replyRequest.result = CXM_P2P_REPLY_REQUEST_RESULT_UNKNOWN_PEER;
	}

	int res = this->mtransceiver->SendTo(clientCandidate,
		(uint8_t *)&msgReply, sizeof(Message));
	if (0 != res)
		LOGE("Cannot send reply to client: %d", res);
	else
		LOGD("Reply request client %s peer address: %s", 
			requestClient.c_str(), NULL != requestPeerCandidate ?
			requestPeerCandidate->ToString().c_str() : "null");
}

void ServantServer::OnConnectMessage(std::shared_ptr<ReceiveMessage> message)
{
	string masterClient = message->GetMessage()->u.client.clientName;
	if (NULL == mclientList[masterClient].get()) {
		LOGE("Cannot get master client: %s", message->GetMessage()->u.client.clientName);
		return;
	}
	string slaveClient = message->GetMessage()->u.client.uc.connect.remoteName;
	if (NULL == mclientList[slaveClient].get()) {
		LOGE("Cannot get slave client: %s", slaveClient.c_str());
		return;
	}

	Message msgReply;
	msgReply.type = CXM_P2P_MESSAGE_REPLY_CONNECT;
	int res = this->mtransceiver->SendTo(mclientList[masterClient],
		(uint8_t *)&msgReply, sizeof(Message));
	if (res < 0) {
		LOGE("Cannot send p2p reply connect: %d", res);
		return;
	}
	res = this->mtransceiver->SendTo(mclientList[slaveClient],
		(uint8_t *)&msgReply, sizeof(Message));
	if (res < 0) {
		LOGE("Cannot send p2p reply connect: %d", res);
		return;
	}
	LOGD("Sending REPLY_CONNECT to master %s %s and client %s %s",
		masterClient.c_str(), mclientList[masterClient]->ToString().c_str(),
		slaveClient.c_str(), mclientList[slaveClient]->ToString().c_str());
}

ServantClient::ServantClient(const char *ip, uint16_t port) :
	mnatType(STUN_NAT_TYPE_UNKNOWN)
{
	// start event thread
	meventThread = shared_ptr<UnifyEventThread>(new UnifyEventThread("ServantClient"));
	meventThread->SetSink(this);
	meventThread->Start();

	// init transport
	mserverCandidate = shared_ptr<Candidate>(new Candidate(ip, port));
	mtransport = shared_ptr<TransceiverU>(new TransceiverU());
	mtransport->SetLocalCandidate(shared_ptr<Candidate>(new Candidate((int)0, 0)));
	mtransport->SetSink(this);

	// LOGOUT is the beginning state
	this->SetStateInternal(SERVANT_CLIENT_LOGOUT);
}

ServantClient::~ServantClient()
{
	if (NULL != mtransport.get()) {
		mtransport->Close();
		mtransport.reset();
	}
	if (NULL != meventThread.get()) {
		meventThread->Join();
		meventThread.reset();
	}
}

int ServantClient::Call()
{
	assert(NULL != mstate.get());
	assert(NULL != meventThread.get());
	if (mname.length() <= 0 || mname.length() > CLIENT_NAME_LENGTH ||
		mremotePeer.length() <= 0 || mremotePeer.length() > CLIENT_NAME_LENGTH) {
		LOGE("Invalid argument: %s", mname.c_str());
		return -1;
	}

	if (SERVANT_CLIENT_LOGOUT != this->GetState()) {
		LOGE("Invalid state during call: %d", this->GetState());
		return -1;
	}

	// put login event
	this->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_LOGIN);
	return 0;
}

void ServantClient::Hangup()
{
	assert(NULL != mstate.get());
	assert(NULL != meventThread.get());
	if (SERVANT_CLIENT_LOGOUT == this->GetState())
		return;

	unique_lock<mutex> lock(mlogoutMutex);

	// put login event
	this->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_LOGOUT);
	// waiting for logout event arrive
	LOGD("Hangup to wait logout event arriving");
	mlogoutCV.wait(lock);
	LOGD("Hangup success");
}

int ServantClient::SendTo(const uint8_t *buffer, int len)
{
	assert(NULL != mstate.get());
	return mstate->SendTo(buffer, len);
}

void ServantClient::OnEvent(int type, shared_ptr<IEventArgs> args)
{
#if 0
	LOGD("Client OnEvent: %d", type);
#endif
	switch (type) {
	case SERVANT_CLIENT_EVENT_LOGIN: {
		int res = this->mstate->Login();
		if (0 != res) {
			LOGE("Relogin, error when procese EVENT_LOGIN by state %d: res %d",
				this->GetState(), res);
            this->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_LOGIN);
        }
		break;
	} case SERVANT_CLIENT_EVENT_LOGOUT: {
		this->mstate->Logout();
		break;
	} case SERVANT_CLIENT_EVENT_CONNECT: {
		int res = this->mstate->Connect();
		if (0 != res)
			LOGE("Error when process EVENT_CONNECT by state %d: res %d",
				this->GetState(), res);
		break;
	} case SERVANT_CLIENT_EVENT_ON_DATA: {
		shared_ptr<ReceiveMessage> message = dynamic_pointer_cast<ReceiveMessage>(args);
		if (NULL == message.get()) {
			LOGE("Invalid args when ON_DATA: %p", args.get());
			break;
		}

		int res = this->mstate->OnMessage(message);
		if (0 != res)
			LOGE("Error when process EVENT_ON_DATA by state %d: res %d",
				this->GetState(), res);
		break;
	} default:
		break;
	}
}

void ServantClient::OnData(std::shared_ptr<ReceiveData> data)
{
	// get message
	shared_ptr<ReceiveMessage> message = ReceiveMessage::Wrap(data);
	if (NULL == message.get()) {
		LOGE("Invalid message size: %d %d", data->GetLength(), (int)sizeof(Message));
		return;
	}
#if 0
	LOGD("Client receive message type: %d", message->GetMessage()->type);
#endif

	// swith thread
	this->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_ON_DATA, message);
}

shared_ptr<ServantClient::ClientState> ServantClient::SetStateInternal(SERVANT_CLIENT_STATE_T state)
{
	unique_lock<std::mutex> lock(mstateMutex);

	shared_ptr<ClientState> oldState = mstate;

	switch (state) {
	case SERVANT_CLIENT_LOGINING:
		mstate = shared_ptr<ClientState>(new ClientStateLogining(this));
		break;
	case SERVANT_CLIENT_LOGIN:
		mstate = shared_ptr<ClientState>(new ClientStateLogin(this));
		break;
	case SERVANT_CLIENT_LOGOUTING:
		mstate = shared_ptr<ClientState>(new ClientStateLogouting(this));
		break;
	case SERVANT_CLIENT_CONNECTING:
		mstate = shared_ptr<ClientState>(new ClientStateConnecting(this));
		break;
	case SERVANT_CLIENT_CONNECTED:
		mstate = shared_ptr<ClientState>(new ClientStateConnected(this));
		break;
	case SERVANT_CLIENT_DISCONNECTING:
		mstate = shared_ptr<ClientState>(new ClientStateDisconnecting(this));
		break;
	default:
		mstate = shared_ptr<ClientState>(new ClientStateLogout(this));
		break;
	}

	if (NULL != oldState.get())
		LOGI("Change state to %d from %d", mstate->State, oldState->State);
	return oldState;
}

}
}
