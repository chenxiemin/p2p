#ifndef _CXM_P2P_SERVANT_H_
#define _CXM_P2P_SERVANT_H_

#include <stdint.h>
#include <memory>
#include <map>
#include <mutex>
#include <condition_variable>

#include "candidate.h"
#include "udp.h"
#include "message.h"
#include "Transceiver.h"
#include "event-thread.h"
#include "timer.h"
#include "transceiver.h"

namespace cxm {
namespace p2p {

class ReceiveMessage : public cxm::util::IEventArgs
{
	private: std::shared_ptr<ReceiveData> mreceiveData;

	private: ReceiveMessage(std::shared_ptr<ReceiveData> receiveData)
	{
		mreceiveData = receiveData;
	}

	public: const Message *GetMessage()
	{
		return (Message *)mreceiveData->GetBuffer();
	}

	public: std::shared_ptr<Candidate> GetRemoteCandidate()
	{
		return mreceiveData->GetRemoteCandidate();
	}

	public: static std::shared_ptr<ReceiveMessage> Wrap(std::shared_ptr<ReceiveData> receiveData)
	{
		std::shared_ptr<ReceiveMessage> message;
		if (NULL == receiveData || sizeof(Message) != receiveData->GetLength())
			return message;
		int res = MessageValidate((Message *)receiveData->GetBuffer());
		if (0 != res) {
			LOGE("Message validation fail: %d", res);
			return message;
		}

		message = std::shared_ptr<ReceiveMessage>(new ReceiveMessage(receiveData));
		return message;
	}
};

class ServantServer : public IReveiverSinkU
{
	public: static const int SERVANT_SERVER_PORT = 8888;

	private: std::map<std::string, std::shared_ptr<Candidate>> mclientList;
	private: std::shared_ptr<TransceiverU> mtransceiver;

	public: ServantServer(const char *ip = NULL, uint16_t port = SERVANT_SERVER_PORT);
	public: virtual ~ServantServer() { Stop(); }
	public: int Start();
	public: void Stop();
			
	public: virtual void OnData(std::shared_ptr<ReceiveData> data);

	private: void OnLoginMessage(std::shared_ptr<ReceiveMessage> message);
	private: void OnLogoutMessage(std::shared_ptr<ReceiveMessage> message);
	private: void OnRequestMessage(std::shared_ptr<ReceiveMessage> message);
	private: void OnConnectMessage(std::shared_ptr<ReceiveMessage> message);
};

typedef enum {
	SERVANT_CLIENT_ERROR_UNKNOWN
} SERVANT_CLIENT_ERROR_T;

class IServantClientSink
{
	public: virtual ~IServantClientSink() { }
	public: virtual void OnConnect() = 0;
	public: virtual void OnDisconnect() = 0;

	public: virtual void OnData(const char *data, int len) = 0;
	public: virtual void OnError(SERVANT_CLIENT_ERROR_T error, void *opaque) = 0;
};

class ServantClient : cxm::p2p::IReveiverSinkU, cxm::util::IEventSink
{
	#define SERVANT_P2P_MESSAGE "hi:chenxiemin"
	#define SERVANT_P2P_REPLY_MESSAGE "reply:chenxiemin"
	public: static const int SERVANT_CLIENT_PORT = 8887;
	public: static const int SERVANT_CLIENT_REPLY_PORT = 8886;

	public: typedef enum {
		SERVANT_CLIENT_LOGOUT,
		SERVANT_CLIENT_LOGINING,
		// Login state indicate that
		// the client has connected to the P2P server
		SERVANT_CLIENT_LOGIN,
		SERVANT_CLIENT_LOGOUTING,
#if 0
		SERVANT_CLIENT_REQUESTING,
		// Requested state indicate that the client
		// has requested the peer address which need to be connected
		SERVANT_CLIENT_REQUESTED,
#endif
		SERVANT_CLIENT_CONNECTING,
		// Connected state indicate that the client
		// has already connected to the peer
		SERVANT_CLIENT_CONNECTED,
		SERVANT_CLIENT_DISCONNECTING,
	} SERVANT_CLIENT_STATE_T;

	private: struct ClientState
	{
		SERVANT_CLIENT_STATE_T State;
		ServantClient *PClient;

		ClientState(SERVANT_CLIENT_STATE_T state, ServantClient *client) :
			State(state), PClient(client) { }
		virtual ~ClientState() { }

		virtual int Login() { return -1; };
		virtual void Logout() { };
#if 0
		// request remote candidate address
		virtual int Request() { return -1; };
		// connect to remote candidate
#endif
		virtual int Connect() { return -1; };
		virtual int Disconnect() { return -1; };

#if 0
		virtual int Send(const char *buffer, int len) { return -1; }
#endif
		virtual int OnMessage(std::shared_ptr<ReceiveMessage> pmsg) { return -1; };
	};
	private: struct ClientStateLogout : public ClientState
	{
		SERVANT_CLIENT_STATE_T State;

		ClientStateLogout(ServantClient *client) :
			ClientState(SERVANT_CLIENT_LOGOUT, client) { }

		virtual int Login();
	};
	private: struct ClientStateLogining : public ClientState, public cxm::util::ITimerSink
	{
		SERVANT_CLIENT_STATE_T State;
		// timer for triggering connecting request repeativity
		private: std::shared_ptr<cxm::util::Timer> mtimer;

		public: ClientStateLogining(ServantClient *client) :
			ClientState(SERVANT_CLIENT_LOGINING, client) { }


		virtual ~ClientStateLogining()
		{
			if (NULL != mtimer)
				mtimer->Stop();
			mtimer.reset();
		}

		virtual int Login();
		virtual void Logout();
		virtual int OnMessage(std::shared_ptr<ReceiveMessage> message);

		virtual void OnTimer();
	};
	private: struct ClientStateLogin : public ClientState
	{
		SERVANT_CLIENT_STATE_T State;

		ClientStateLogin(ServantClient *client) :
			ClientState(SERVANT_CLIENT_LOGIN, client) { }

		virtual void Logout();
		virtual int OnMessage(std::shared_ptr<ReceiveMessage> message);
	};
	private: struct ClientStateLogouting : public ClientState
	{
		SERVANT_CLIENT_STATE_T State;

		ClientStateLogouting(ServantClient *client) :
			ClientState(SERVANT_CLIENT_LOGOUTING, client) { }

		virtual void Logout();
	};
	private: struct ClientStateConnecting : public ClientState
	{
		SERVANT_CLIENT_STATE_T State;

		ClientStateConnecting(ServantClient *client) :
			ClientState(SERVANT_CLIENT_CONNECTING, client) { }

		virtual void Logout();
	};
	private: struct ClientStateConnected : public ClientState
	{
		SERVANT_CLIENT_STATE_T State;

		ClientStateConnected(ServantClient *client) :
			ClientState(SERVANT_CLIENT_CONNECTED, client) { }

		virtual void Logout();
	};
	private: struct ClientStateDisconnecting : public ClientState
	{
		SERVANT_CLIENT_STATE_T State;

		ClientStateDisconnecting(ServantClient *client) :
			ClientState(SERVANT_CLIENT_DISCONNECTING, client) { }

		virtual void Logout();
	};

	private: typedef enum {
		SERVANT_CLIENT_EVENT_LOGIN,
		SERVANT_CLIENT_EVENT_LOGOUT,
		SERVANT_CLIENT_EVENT_ON_DATA,
	} SERVANT_CLIENT_EVENT_T;

	// transport for communicating with P2P Server
	// to find the remote peer ip / port
	private: std::shared_ptr<TransceiverU> mserverTransport;
	private: std::shared_ptr<Candidate> mserverCandidate;
	private: std::string mname;
	private: std::string mremote;
			 
	private: std::shared_ptr<cxm::util::UnifyEventThread> meventThread;

	private: std::mutex mstateMutex;
	private: std::shared_ptr<ClientState> mstate;

	private: std::mutex mlogoutMutex;
	private: std::condition_variable mlogoutCV;
			 
	private: IServantClientSink *mpsink;

	public: ServantClient(const char *serverIp,
		uint16_t port = ServantServer::SERVANT_SERVER_PORT);
	public: virtual ~ServantClient();

	public: void SetSink(IServantClientSink *sink) { mpsink = sink; }
	public: void SetName(std::string name) { mname = name; }
	public: void SetRemote(std::string remote) { mremote = remote; }
	public: SERVANT_CLIENT_STATE_T GetState() { return mstate->State; }

	// change current state, return state before
	private: std::shared_ptr<ClientState> SetStateInternal(SERVANT_CLIENT_STATE_T state);
	private: std::shared_ptr<ClientState> GetStateInternal() { return mstate; }

	public: int Call();
	public: void Hangup();

#if 0
	public: int Send(const char *data, int len);

	// request remote candidate address
	public: int Request();
	// connect to remote candidate
	public: int Connect();
	public: void Disconnect();
#endif
	
	private: virtual void OnEvent(int type, std::shared_ptr<cxm::util::IEventArgs> args);
	private: virtual void OnData(std::shared_ptr<ReceiveData> data);

	private: int DoRequest();
	private: int DoConnect();

	private: void OnReplyRequest(const Message *pmsg);
	private: void OnReplyConnect(const Message *pmsg);
	private: void OnP2PConnect(const Message *pmsg);
	private: void OnReplyP2PConnect(const Message *pmsg);
};

}
}
#endif
