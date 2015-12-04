#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <chrono>

#include "servant.h"
#include "log.h"
#include "message.h"

using namespace std;
using namespace std::chrono;
using namespace cxm::util;

ServantServer::ServantServer(const char *ip, uint16_t port) :
	misRun(false), msocket(INVALID_SOCKET)
{
	mcandidate = shared_ptr<Candidate>(new Candidate(ip, port));
}

int ServantServer::Start()
{
	if (misRun) {
		LOGE("Already start");
		return -1;
	}

	msocket = openPort(mcandidate->Port(), 0, false);
	if (INVALID_SOCKET == msocket) {
		LOGE("Cannot open socket");
		return -1;
	}

	misRun = true;
	mthread = shared_ptr<Thread>(new Thread(this, "ServantServer"));
	mthread->Start();

	LOGI("ServantServer listen on: %s", mcandidate->ToString().c_str());
	return 0;
}

void ServantServer::Stop()
{
	if (!misRun)
		return;

	misRun = false;
	mthread->Join();
	mthread.reset();
	if (INVALID_SOCKET != msocket) {
		closesocket(msocket);
		msocket = INVALID_SOCKET;
	}
}

void ServantServer::Run()
{
	Message msg;
	while (misRun) {
		int res = MessageReceive(msocket, &msg);
		if (0 != res) {
			// LOGE("Cannot receive message: %d", res);
			continue;
		}

		switch (msg.type) {
		case CXM_P2P_MESSAGE_LOGIN:
			OnLoginMessage(&msg);
			break;
		case CXM_P2P_MESSAGE_LOGOUT:
			OnLogoutMessage(&msg);
			break;
		case CXM_P2P_MESSAGE_REQUEST:
			OnRequestMessage(&msg);
			break;
		case CXM_P2P_MESSAGE_CONNECT:
			OnConnectMessage(&msg);
			break;
		default:
			LOGE("Unknown message type: %d", msg.type);
			break;
		}
	}
}

void ServantServer::OnLoginMessage(const Message *pmsg)
{
	string clientName = pmsg->clientName;
	shared_ptr<Candidate> clientCandidate = shared_ptr<Candidate>(
		new Candidate(pmsg->clientIp, pmsg->clientPort));

	if (NULL != mclientList[clientName].get()) {
		LOGW("Client %s already login, override", clientName.c_str());
		// return;
	}

	mclientList[clientName] = clientCandidate;
	LOGI("Client %s login with address: %s", clientName.c_str(),
		clientCandidate->ToString().c_str());
}

void ServantServer::OnLogoutMessage(const Message *pmsg)
{
	string clientName = pmsg->clientName;
	if (NULL != mclientList[clientName]) {
		mclientList[clientName] = NULL;
		LOGI("Client %s logout", clientName.c_str());
	}
}

void ServantServer::OnRequestMessage(const Message *pmsg)
{
	string requestClient = pmsg->umsg.request.remoteName;
	if (NULL == mclientList[requestClient].get()) {
		LOGE("Cannot get request client: %s", requestClient.c_str());
		return;
	}

	// send reply
	shared_ptr<Candidate> remoteCandidate = mclientList[requestClient];

	Message msgReply;
	memset(&msgReply, 0, sizeof(msgReply));
	msgReply.type = CXM_P2P_MESSAGE_REPLY_REQUEST;
	msgReply.umsg.replyRequest.remoteIp = remoteCandidate->Ip();
	msgReply.umsg.replyRequest.remotePort = remoteCandidate->Port();

	int res = MessageSend(msocket, &msgReply, MessageCandidate(pmsg));
	if (0 != res)
		LOGE("Cannot send reply to client: %d", res);
	LOGD("Reply request to: client %s address %s", 
		requestClient.c_str(), remoteCandidate->ToString().c_str());
}

void ServantServer::OnConnectMessage(const Message *pmsg)
{
	string masterClient = pmsg->clientName;
	if (NULL == mclientList[masterClient].get()) {
		LOGE("Cannot get master client: %s", pmsg->clientName);
		return;
	}
	string slaveClient = pmsg->umsg.connect.remoteName;
	if (NULL == mclientList[slaveClient].get()) {
		LOGE("Cannot get slave client: %s", slaveClient.c_str());
		return;
	}

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
}

ServantClient::ServantClient(const char *ip, uint16_t port) :
	mstatus(SERVANT_CLIENT_UNLOGIN), msocket(INVALID_SOCKET),
	misRun(false), mport(0)
{
	mserverCandidate = shared_ptr<Candidate>(new Candidate(ip, port));
}

int ServantClient::Login()
{
	if (SERVANT_CLIENT_UNLOGIN != mstatus) {
		LOGE("Invalid status: %d", mstatus);
		return -1;
	}
	if (mname.length() <= 0 || mname.length() > CLIENT_NAME_LENGTH ||
		mremote.length() <= 0 || mremote.length() > CLIENT_NAME_LENGTH) {
		LOGE("Invalid argument: %s", mname.c_str());
		return -1;
	}

	int res = -1;
	do {
		res = this->DoLogin();
		if (0 != res) {
			LOGE("Cannot login: %d", res);
			break;
		}

		// start listen thread
		misRun = true;
		mthread = shared_ptr<Thread>(new Thread(this, "ServantClient"));
		mthread->Start();

		mstatus = SERVANT_CLIENT_LOGIN;
		LOGI("User %s login to %s at: port %d fd %d", mname.c_str(),
			mserverCandidate->ToString().c_str(), mport, msocket);
		return 0;
	} while (0);

	return res;
}

void ServantClient::Logout()
{
	if (mstatus == SERVANT_CLIENT_UNLOGIN)
		return;

	int res = DoLogout();
	if (0 != res)
		LOGE("Cannot logout for %s: %d", mname.c_str(), res);
	mstatus = SERVANT_CLIENT_UNLOGIN;
}

int ServantClient::Request()
{
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
}

int ServantClient::Connect()
{
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
}

void ServantClient::Run()
{
	Message msg;
	while (misRun) {
		int res = MessageReceive(msocket, &msg);
		if (0 != res) {
			// LOGE("Cannot get reply from server for requesting remote");
			continue;
		}

		switch (msg.type) {
		case CXM_P2P_MESSAGE_REPLY_REQUEST:
			OnReplyRequest(&msg);
			break;
		case CXM_P2P_MESSAGE_REPLY_CONNECT:
			OnReplyConnect(&msg);
			break;
		case CXM_P2P_MESSAGE_DO_P2P_CONNECT:
			OnP2PConnect(&msg);
			break;
		case CXM_P2P_MESSAGE_REPLY_P2P_CONNECT:
			OnReplyP2PConnect(&msg);
			break;
		default:
			LOGE("Invalid client message type: %d", msg.type);
			break;
		}
	}
}

void ServantClient::OnReplyRequest(const Message *pmsg)
{
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
}

void ServantClient::OnReplyConnect(const Message *pmsg)
{
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
}

void ServantClient::OnP2PConnect(const Message *pmsg)
{
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
}

void ServantClient::OnReplyP2PConnect(const Message *pmsg)
{
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
}

int ServantClient::DoLogin()
{
	// create socket
	msocket = openPort(mport, 0, false);
	if (INVALID_SOCKET == msocket) {
		LOGE("Cannot open port");
		return -1;
	}
	// get random port if any
	if (0 == mport) {
		struct sockaddr_in addr;
		int len = sizeof(addr);
		if (0 == getsockname(msocket, (struct sockaddr *)&addr, &len))
			mport = addr.sin_port;

		if (0 != mport) {
			// reopen port
			closesocket(msocket);
			msocket = openPort(mport, 0, false);
			if (INVALID_SOCKET == msocket) {
				LOGE("Cannot reopen port");
				return -1;
			}
		}
	}

	// send login message
	Message msg;
	memset(&msg, 0, sizeof(msg));
	msg.type = CXM_P2P_MESSAGE_LOGIN;
	strncpy(msg.clientName, mname.c_str(), CLIENT_NAME_LENGTH);
	msg.umsg.login.clientPrivateIp = 0; // TODO set private candidate
	msg.umsg.login.clientPrivatePort = 0;

	return MessageSend(msocket, &msg, mserverCandidate);
}

int ServantClient::DoLogout()
{
	if (INVALID_SOCKET == msocket) {
		LOGW("Already logout");
		return -1;
	}

	// send message
	Message msg;
	memset(&msg, 0, sizeof(msg));
	msg.type = CXM_P2P_MESSAGE_LOGOUT;
	strncpy(msg.clientName, mname.c_str(), CLIENT_NAME_LENGTH);

	int res = MessageSend(msocket, &msg, mserverCandidate);
	if (0 != res)
		LOGE("Cannot send logout message: %d", res);

	// wait for thread stop
	misRun = false;
	mthread->Join();
	mthread.reset();

	// close socket
	closesocket(msocket);
	msocket = INVALID_SOCKET;

	return 0;
}

int ServantClient::DoRequest()
{
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
}

int ServantClient::DoConnect()
{
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
}
