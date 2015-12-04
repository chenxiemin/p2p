#ifndef _CXM_P2P_SERVANT_H_
#define _CXM_P2P_SERVANT_H_

#include <stdint.h>
#include <memory>
#include <map>
#include <mutex>
#include <condition_variable>

#include "thread.h"
#include "candidate.h"
#include "udp.h"
#include "message.h"

class ServantServer : public cxm::util::IRunnable
{
	public: static const int SERVANT_SERVER_PORT = 8888;

	private: std::shared_ptr<Candidate> mcandidate;
	private: std::shared_ptr<cxm::util::Thread> mthread;
	private: volatile bool misRun;
	private: Socket msocket;
	private: std::map<std::string, std::shared_ptr<Candidate>> mclientList;

	public: ServantServer(const char *ip = NULL, uint16_t port = SERVANT_SERVER_PORT);
	public: int Start();
	public: void Stop();

	public: virtual void Run();
	private: void OnLoginMessage(const Message *pmsg);
	private: void OnLogoutMessage(const Message *pmsg);
	private: void OnRequestMessage(const Message *pmsg);
	private: void OnConnectMessage(const Message *pmsg);
};

class ServantClient : public cxm::util::IRunnable
{
	#define SERVANT_P2P_MESSAGE "hi:chenxiemin"
	#define SERVANT_P2P_REPLY_MESSAGE "reply:chenxiemin"
	public: static const int SERVANT_CLIENT_PORT = 8887;
	public: static const int SERVANT_CLIENT_REPLY_PORT = 8886;

	public: typedef enum {
		SERVANT_CLIENT_UNLOGIN,
		SERVANT_CLIENT_LOGIN,
		SERVANT_CLIENT_REQUESTING,
		SERVANT_CLIENT_REQUESTED,
		SERVANT_CLIENT_CONNECTING,
		SERVANT_CLIENT_CONNECTED
	} ClientStatus;

	private: std::shared_ptr<Candidate> mserverCandidate;
	private: uint16_t mport;
	private: ClientStatus mstatus;
	private: std::string mname;
	private: Socket msocket;
	private: std::shared_ptr<cxm::util::Thread> mthread;
	private: volatile bool misRun;
	private: std::string mremote;
	private: std::shared_ptr<Candidate> mremoteCandidate;
	private: std::mutex mmutex;
	private: std::condition_variable mcv;

	public: ServantClient(const char *serverIp,
		uint16_t port = ServantServer::SERVANT_SERVER_PORT);

	public: int Login();
	public: void Logout();
	public: int Request(); // request remote candidate address
	public: int Connect(); // connect to remote candidate
	public: virtual void Run();

	public: void SetName(std::string name) { mname = name; }
	public: void SetRemote(std::string remote) { mremote = remote; }
	public: ClientStatus GetStatus() { return mstatus; }

	private: int DoLogin();
	private: int DoLogout();
	private: int DoRequest();
	private: int DoConnect();

	private: void OnReplyRequest(const Message *pmsg);
	private: void OnReplyConnect(const Message *pmsg);
	private: void OnP2PConnect(const Message *pmsg);
	private: void OnReplyP2PConnect(const Message *pmsg);
};

#endif
