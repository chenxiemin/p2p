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
		LOGE("Invalid message size: %d %d", data->GetLength(), sizeof(Message));
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
	if (NULL == mclientList[requestClient].get()) {
		LOGE("Cannot get request client: %s", requestClient.c_str());
		return;
	}

	// send reply
	shared_ptr<Candidate> remoteCandidate = mclientList[requestClient];

	Message msgReply;
	msgReply.type = CXM_P2P_MESSAGE_REPLY_REQUEST;
	msgReply.u.client.uc.replyRequest.remoteIp = remoteCandidate->Ip();
	msgReply.u.client.uc.replyRequest.remotePort = remoteCandidate->Port();

#if 0
	int res = MessageSend(msocket, &msgReply, MessageCandidate(pmsg));
	if (0 != res)
		LOGE("Cannot send reply to client: %d", res);
	LOGD("Reply request to: client %s address %s", 
		requestClient.c_str(), remoteCandidate->ToString().c_str());
#else
	assert(0);
#endif
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

#if 0
	Message msgReply;
	memset(&msgReply, 0, sizeof(msgReply));
	msgReply.type = CXM_P2P_MESSAGE_REPLY_CONNECT;
	int res = MessageSend(msocket, &msgReply, mclientList[masterClient]);
	if (res < 0) {
		LOGE("Cannot send p2p reply connect: %d", res);
		return;
	}
	res = MessageSend(msocket, &msgReply, mclientList[slaveClient]);
	if (res < 0) {
		LOGE("Cannot send p2p reply connect: %d", res);
		return;
	}
#else
	assert(0);
#endif
}

ServantClient::ServantClient(const char *ip, uint16_t port)
{
	mserverCandidate = shared_ptr<Candidate>(new Candidate(ip, port));
	mserverTransport = shared_ptr<TransceiverU>(new TransceiverU());
	mserverTransport->SetLocalCandidate(shared_ptr<Candidate>(new Candidate((int)0, 0)));
	mserverTransport->SetSink(this);

	this->SetStateInternal(SERVANT_CLIENT_LOGOUT);
}

int ServantClient::Call()
{
	assert(NULL != mstate.get());
	return mstate->Login();
}

void ServantClient::Hangup()
{
	assert(NULL != mstate.get());
	mstate->Logout();
}

int ServantClient::Send(const char *buffer, int len)
{
	assert(NULL != mstate.get());
	return mstate->Send(buffer, len);
}

void ServantClient::OnEvent(int type, shared_ptr<IEventArgs> args)
{
	LOGD("Client OnEvent: %d", type);
	switch (type) {
	case SERVANT_CLIENT_EVENT_LOGIN:
		this->mstate->Login();
		break;
	case SERVANT_CLIENT_EVENT_ON_DATA: {
		shared_ptr<ReceiveMessage> message = dynamic_pointer_cast<ReceiveMessage>(args);
		if (NULL == message.get()) {
			LOGE("Invalid args when ON_DATA: %p", args.get());
			break;
		}

		this->mstate->OnMessage(message);
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
		LOGE("Invalid message size: %d %d", data->GetLength(), sizeof(Message));
		return;
	}
	LOGD("Client receive message type: %d", message->GetMessage()->type);

	// swith thread
	this->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_ON_DATA, message);
}

void ServantClient::ClientState::Logout()
{
	// send message
	Message msg;
	msg.type = CXM_P2P_MESSAGE_LOGOUT;
	strncpy(msg.u.client.clientName, PClient->mname.c_str(), CLIENT_NAME_LENGTH);

	int res = PClient->mserverTransport->SendTo(PClient->mserverCandidate,
		(uint8_t *)&msg, sizeof(Message));
	if (0 != res)
		LOGE("Cannot send logout message: %d", res);

	// close server transport
	if (NULL != PClient->mserverTransport.get()) {
		PClient->mserverTransport->Close();
		PClient->mserverTransport.reset();
	}

	if (NULL != PClient->meventThread.get()) {
		// wait for thread stop
		PClient->meventThread->Join();
		PClient->meventThread.reset();
	}
}

int ServantClient::ClientStateLogout::Login()
{
	if (PClient->mname.length() <= 0 || PClient->mname.length() > CLIENT_NAME_LENGTH ||
		PClient->mremote.length() <= 0 || PClient->mremote.length() > CLIENT_NAME_LENGTH) {
		LOGE("Invalid argument: %s", PClient->mname.c_str());
		return -1;
	}

	// hold on self to prevent deleted
	shared_ptr<ServantClient::ClientState> oldState = PClient->SetStateInternal(SERVANT_CLIENT_LOGINING);

	// open transport
	int res = PClient->mserverTransport->Open();
	if (0 != res) {
		LOGE("Cannot open transport: %d", res);
		return -1;
	}

	// start event thread
	PClient->meventThread = shared_ptr<UnifyEventThread>(new UnifyEventThread("ServantClient"));
	PClient->meventThread->SetSink(PClient);
	PClient->meventThread->Start();

	// put login event
	PClient->meventThread->PutEvnet(SERVANT_CLIENT_EVENT_LOGIN);

	return 0;
}



int ServantClient::ClientStateLogin::OnMessage(shared_ptr<ReceiveMessage> message)
{
	return -1;
}

int ServantClient::ClientStateRequesting::OnMessage(shared_ptr<ReceiveMessage> message)
{
	return -1;
}

int ServantClient::ClientStateRequested::OnMessage(shared_ptr<ReceiveMessage> message)
{
	return -1;
}

int ServantClient::ClientStateConnecting::OnMessage(shared_ptr<ReceiveMessage> message)
{
	return -1;
}

int ServantClient::ClientStateConnected::OnMessage(shared_ptr<ReceiveMessage> message)
{
	return -1;
}

void ServantClient::Logout()
{
#if 0
	if (mstatus == SERVANT_CLIENT_UNLOGIN)
		return;

	int res = DoLogout();
	if (0 != res)
		LOGE("Cannot logout for %s: %d", mname.c_str(), res);
	mstatus = SERVANT_CLIENT_UNLOGIN;
#else
#endif
}

int ServantClient::Request()
{
#if 0
	if (SERVANT_CLIENT_LOGIN != mstatus) {
		LOGE("Invalid status for request: %d", mstatus);
		return -1;
	}
	if (mremote.length() == 0) {
		LOGE("Invalid remote name: %s", mremote.c_str());
		return -1;
	}

	int res = -1;
	do {
		// timeoutd wait
		unique_lock<std::mutex> lock(mmutex);

		res = this->DoRequest();
		if (0 != res) {
			LOGE("Cannot reeuest remote address: %d", res);
			break;
		}

		mstatus = SERVANT_CLIENT_REQUESTING;
		LOGD("Waiting for server reply request");
		mcv.wait_for(lock, seconds(1));

		if (SERVANT_CLIENT_REQUESTED != mstatus) {
			res = -1;
			break;
		}

		return 0;
	} while (0);

	mstatus = SERVANT_CLIENT_LOGIN; // retrieve status
	return res;
#else
	return -1;
#endif
}

int ServantClient::Connect()
{
#if 0
	if (SERVANT_CLIENT_REQUESTED != mstatus) {
		LOGE("Invalid status when connect: %d", mstatus);
		return -1;
	}
	if (NULL == mremoteCandidate.get()) {
		LOGE("Unknwon remote candidate");
		return -1;
	}

	do {
		// timeoutd wait
		unique_lock<std::mutex> lock(mmutex);

		int res = this->DoConnect();
		if (0 != res) {
			LOGE("Cannot connect to remote: %d", res);
			break;
		}

		mstatus = SERVANT_CLIENT_CONNECTING;
		LOGD("Waiting for connecting remote");
		mcv.wait_for(lock, seconds(1));

		if (SERVANT_CLIENT_CONNECTED != mstatus) {
			res = -1;
			break;
		}

		return 0;
	} while (false);

	mstatus = SERVANT_CLIENT_REQUESTED;
	return -1;
#else
	return -1;
#endif
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
	case SERVANT_CLIENT_REQUESTING:
		mstate = shared_ptr<ClientState>(new ClientStateRequesting(this));
		break;
	case SERVANT_CLIENT_REQUESTED:
		mstate = shared_ptr<ClientState>(new ClientStateRequested(this));
		break;
	case SERVANT_CLIENT_CONNECTING:
		mstate = shared_ptr<ClientState>(new ClientStateConnecting(this));
		break;
	case SERVANT_CLIENT_CONNECTED:
		mstate = shared_ptr<ClientState>(new ClientStateConnected(this));
		break;
	default:
		mstate = shared_ptr<ClientState>(new ClientStateLogout(this));
		break;
	}

	if (NULL != oldState.get())
		LOGI("Change state to %d from %d", mstate->State, oldState->State);
	return oldState;
}

void ServantClient::OnReplyRequest(const Message *pmsg)
{
#if 0
	if (mstatus != SERVANT_CLIENT_REQUESTING) {
		LOGE("Invalid status when receive reply request: %d", mstatus);
		return;
	}

	lock_guard<mutex> lock(mmutex); // lock first

	mremoteCandidate = shared_ptr<Candidate>(new Candidate(
		pmsg->umsg.replyRequest.remoteIp, pmsg->umsg.replyRequest.remotePort));
	mstatus = SERVANT_CLIENT_REQUESTED;
	LOGI("Receive remote candidate: %s", mremoteCandidate->ToString().c_str());
	mcv.notify_one(); // notify request()
#else
#endif
}

void ServantClient::OnReplyConnect(const Message *pmsg)
{
#if 0
	if (SERVANT_CLIENT_CONNECTING != mstatus) {
		LOGE("Invalid status when reply connect: %d", mstatus);
		return;
	}

	// send p2p connect to remote client
	Message msg;
	memset(&msg, 0, sizeof(msg));
	msg.type = CXM_P2P_MESSAGE_DO_P2P_CONNECT;
	strncpy(msg.umsg.p2p.key, SERVANT_P2P_MESSAGE, CLIENT_NAME_LENGTH);
	int res = MessageSend(msocket, &msg, mremoteCandidate);
	if (0 != res)
		LOGE("Cannot send p2p connect to remote %s: %d",
			mremoteCandidate->ToString().c_str(), res);
	else
		LOGI("Send remote p2p connect: %s", mremoteCandidate->ToString().c_str());
#else
#endif
}

void ServantClient::OnP2PConnect(const Message *pmsg)
{
#if 0
	if (SERVANT_CLIENT_CONNECTING != mstatus) {
		LOGE("Invalid status when p2p connect: %d", mstatus);
		return;
	}
	shared_ptr<Candidate> remote = MessageCandidate(pmsg);
	if (!remote->Equal(*(mremoteCandidate.get()))) {
		LOGE("Mismatch p2p connection: local remote %s request %s",
			mremoteCandidate->ToString().c_str(), remote->ToString().c_str());
		return;
	}
	if (0 != strcmp(pmsg->umsg.p2p.key, SERVANT_P2P_MESSAGE)) {
		LOGE("Invalid key when p2p connection");
		return;
	}

	// send p2p connect
	Message msg;
	memset(&msg, 0, sizeof(msg));
	msg.type = CXM_P2P_MESSAGE_REPLY_P2P_CONNECT;
	strncpy(msg.umsg.p2pReply.key, SERVANT_P2P_REPLY_MESSAGE, CLIENT_NAME_LENGTH);
	int res = MessageSend(msocket, &msg, mremoteCandidate);
	if (0 != res)
		LOGE("Cannot send reply p2p connect to %s: %d",
		mremoteCandidate->ToString().c_str(), res);
	else
		LOGI("Send reply p2p connect to %s with key: %s",
			mremoteCandidate->ToString().c_str(), pmsg->umsg.p2p.key);
#else
#endif
}

void ServantClient::OnReplyP2PConnect(const Message *pmsg)
{
#if 0
	if (SERVANT_CLIENT_CONNECTING != mstatus) {
		LOGE("Invalid status when p2p connect: %d", mstatus);
		return;
	}
	shared_ptr<Candidate> remote = MessageCandidate(pmsg);
	if (!remote->Equal(*(mremoteCandidate.get()))) {
		LOGE("Mismatch p2p connection: local remote %s request %s",
			mremoteCandidate->ToString().c_str(), remote->ToString().c_str());
		return;
	}
	if (0 != strcmp(pmsg->umsg.p2p.key, SERVANT_P2P_REPLY_MESSAGE)) {
		LOGE("Invalid key when p2p connection");
		return;
	}

	LOGI("On reply p2p connection with key: %s", pmsg->umsg.p2p.key);
	lock_guard<mutex> lock(mmutex); // lock first
	
	mstatus = SERVANT_CLIENT_CONNECTED;
	mcv.notify_one(); // notify request()
#else
#endif
}

int ServantClient::DoRequest()
{
#if 0
	Message msg;
	memset(&msg, 0, sizeof(msg));
	msg.type = CXM_P2P_MESSAGE_REQUEST;
	strncpy(msg.umsg.request.remoteName, mremote.c_str(), CLIENT_NAME_LENGTH);

	int res = MessageSend(msocket, &msg, mserverCandidate);
	if (0 != res) {
		LOGE("Cannot send request message: %d", res);
		return -1;
	}

	return 0;
#else
	return -1;
#endif
}

int ServantClient::DoConnect()
{
#if 0
	Message msg;
	memset(&msg, 0, sizeof(msg));
	msg.type = CXM_P2P_MESSAGE_CONNECT;
	strncpy(msg.clientName, mname.c_str(), CLIENT_NAME_LENGTH);
	strncpy(msg.umsg.connect.remoteName, mremote.c_str(), CLIENT_NAME_LENGTH);

	int res = MessageSend(msocket, &msg, mserverCandidate);
	if (0 != res) {
		LOGE("Cannot send connect message: %d", res);
		return -1;
	}

	return 0;
#else
	return -1;
#endif
}

}
}
