#ifndef _CXM_P2P_SERVANT_H_
#define _CXM_P2P_SERVANT_H_

#include <stdint.h>
#include <memory>
#include <map>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>

#include "candidate.h"
#include "udp.h"
#include "message.h"
#include "transceiver.h"
#include "event-thread.h"
#include "timer.h"
#include "transceiver.h"
#include "stun-resolver.h"
#include "event-args.h"
#include "client-state.h"

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

	public: std::shared_ptr<Candidate> GetLocalCandidate()
	{
		return mreceiveData->GetLocalCandidate();
	}

	public: std::shared_ptr<ReceiveData> GetReceiveData() { return mreceiveData; }

	public: static std::shared_ptr<ReceiveMessage> Wrap(std::shared_ptr<ReceiveData> receiveData)
	{
		std::shared_ptr<ReceiveMessage> message;
		if (NULL == receiveData || receiveData->GetLength() < sizeof(Message))
			return message;
		Message *pmsg = (Message *)receiveData->GetBuffer();
		if (CXM_P2P_MESSAGE_USER_DATA == pmsg->type) {
			if (pmsg->u.p2p.up.userDataLength + sizeof(Message) != receiveData->GetLength()) {
				LOGE("Invalid USER_DATA packet size: %d", pmsg->u.p2p.up.userDataLength);
				return message;
			}
		}
		int res = MessageValidate(pmsg);
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

	public: ServantServer();
	public: virtual ~ServantServer() { Stop(); }
	public: int Start(const char *ip = NULL, uint16_t port = SERVANT_SERVER_PORT);
	public: void Stop();
			
	public: virtual void OnData(std::shared_ptr<ReceiveData> data);

	private: void OnLoginMessage(std::shared_ptr<ReceiveMessage> message);
	private: void OnLogoutMessage(std::shared_ptr<ReceiveMessage> message);
	private: void OnConnectMessage(std::shared_ptr<ReceiveMessage> message);
};

typedef enum {
	SERVANT_CLIENT_ERROR_UNKNOWN
} SERVANT_CLIENT_ERROR_T;

class P2PPacket
{
	private: std::shared_ptr<ReceiveData> mrecvData;
	public: P2PPacket(std::shared_ptr<ReceiveData> data) : mrecvData(data) { }

	public: uint8_t *GetData() { return mrecvData->GetBuffer() + sizeof(Message); }
	public: int GetLength() { return mrecvData->GetLength() - sizeof(Message); }
	public: std::shared_ptr<Candidate> GetRemoteCandidate() { return mrecvData->GetRemoteCandidate(); }
};

class IServantClientSink
{
	public: virtual ~IServantClientSink() { }
	public: virtual void OnLogin() = 0;
	public: virtual void OnLogout() = 0;
	public: virtual void OnConnect() = 0;
	public: virtual void OnDisconnect() = 0;

	public: virtual void OnError(SERVANT_CLIENT_ERROR_T error, void *opaque) = 0;
};

class IServantClientDataSink
{
	public: virtual ~IServantClientDataSink() { }

	public: virtual void OnData(std::shared_ptr<P2PPacket> packet) = 0;
};

class ServantClient : cxm::p2p::IReveiverSinkU, cxm::util::IEventSink
{
	#define SERVANT_P2P_MESSAGE "hi:chenxiemin"
	#define SERVANT_P2P_REPLY_MESSAGE "reply:chenxiemin"
	public: static const int SERVANT_CLIENT_PORT = 8887;
	public: static const int SERVANT_CLIENT_REPLY_PORT = 8886;

	public: static const int SERVANT_CLIENT_SERVER_KEEP_ALIVE_DELTA_MILS = 1000 * 60;
	public: static const int SERVANT_CLIENT_PEER_KEEP_ALIVE_DELTA_MILS = 500;
	public: static const int SERVANT_CLIENT_PEER_KEEP_ALIVE_TIMEOUT = 1000 * 30;

	private: typedef enum {
		SERVANT_CLIENT_EVENT_LOGIN,
		SERVANT_CLIENT_EVENT_LOGOUT,
		SERVANT_CLIENT_EVENT_CONNECT,
		SERVANT_CLIENT_EVENT_DISCONNECT,
		SERVANT_CLIENT_EVENT_ON_DATA,
		SERVANT_CLIENT_EVENT_SERVER_KEEP_ALIVE, // keep alive for p2p server
		SERVANT_CLIENT_EVENT_PEER_KEEP_ALIVE, // keep alive for remote peer
	} SERVANT_CLIENT_EVENT_T;

	// transport for communicating with other candidates, include P2P Server and peers
	// to find the remote peer ip / port
	private: std::shared_ptr<TransceiverU> mtransport;
	private: std::shared_ptr<Candidate> mserverCandidate;
	private: std::shared_ptr<Candidate> mserverSubCandidate;
	private: std::string mname;
	private: std::string mremotePeer;
			 
	private: std::shared_ptr<cxm::util::UnifyEventThread> meventThread;

	private: STUN_NAT_TYPE_T mnatType;

	private: std::mutex mstateMutex;
	private: std::shared_ptr<ClientState> mstate;

	private: std::mutex mlogoutMutex;
	private: std::condition_variable mlogoutCV;
			 
	private: IServantClientSink *mpsink;
	private: IServantClientDataSink *mpDataSink;

	// candidate of remote peer
	private: std::shared_ptr<Candidate> PeerCandidate;
			
	private: bool misServerKeepAlive;
	private: bool misPeerKeepAlive;
	private: std::chrono::system_clock::time_point mlastPeerKeepAliveTime;

    private: int mconnectingTimeoutMils;
    private: CXM_P2P_PEER_ROLE_T mpeerRole;

	public: ServantClient(const char *serverIp,
		uint16_t port = ServantServer::SERVANT_SERVER_PORT);
	public: virtual ~ServantClient();

	public: void SetSink(IServantClientSink *sink) { mpsink = sink; }
	public: void SetDataSink(IServantClientDataSink *sink) { mpDataSink = sink; }
	public: void SetName(std::string name) { mname = name; }
	public: void SetRemote(std::string remote) { mremotePeer = remote; }
	public: SERVANT_CLIENT_STATE_T GetState() { return mstate->State; }
	public: std::string GetRemote() { return mremotePeer; }
	public: std::string GetName() { return mname; }

	// change current state, return state before
	private: std::shared_ptr<ClientState> SetStateInternal(SERVANT_CLIENT_STATE_T state);
	private: std::shared_ptr<ClientState> GetStateInternal() { return mstate; }

	public: int Login();
	public: void Logout();

	public: int Connect();
	public: void Disconnect();

	public: int SendTo(const uint8_t *buffer, int len);

	public: void FireOnLoginNofity()
	{
		if (NULL != this->mpsink)
			mpsink->OnLogin();
	}

	public: void FireOnLogoutNofity()
	{
		if (NULL != this->mpsink)
			mpsink->OnLogout();
	}

	public: void FireOnConnectNofity()
	{
		if (NULL != this->mpsink)
			mpsink->OnConnect();
	}

	public: void FireOnDisconnectNofity()
	{
		if (NULL != this->mpsink)
			mpsink->OnDisconnect();
	}

	public: void FireOnDataNofity(std::shared_ptr<P2PPacket> packet)
	{
		if (NULL != this->mpDataSink)
			mpDataSink->OnData(packet);
	}

	private: void StartServerKeepAlive();
	private: void StopServerKeepAlive();

	private: void StartPeerKeepAlive();
	private: void StopPeerKeepAlive();
	
	private: virtual void OnEvent(int type, std::shared_ptr<cxm::util::IEventArgs> args);
	private: virtual void OnData(std::shared_ptr<ReceiveData> data);

	private: void OnReplyRequest(const Message *pmsg);
	private: void OnReplyConnect(const Message *pmsg);
	private: void OnP2PConnect(const Message *pmsg);
	private: void OnReplyP2PConnect(const Message *pmsg);

    friend class ClientState;
    friend class ClientStateLogout;
    friend class ClientStateLogouting;
    friend class ClientStateLogin;
    friend class ClientStateLogining;
    friend class ClientStateConnecting;
    friend class ClientStateConnected;
    friend class ClientStateDisconnecting;
};

}
}
#endif
