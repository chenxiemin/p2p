/*
 * =====================================================================================
 *
 *       Filename:  client-state.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/17/2016 07:47:54 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */

#ifndef _CXM_P2P_CLIENT_STATE_H_
#define _CXM_P2P_CLIENT_STATE_H_

#include "servant.h"

namespace cxm {
namespace p2p {


class ServantClient;
class ReceiveMessage;

struct ClientState
{
	SERVANT_CLIENT_STATE_T State;
	ServantClient *PClient;

	ClientState(SERVANT_CLIENT_STATE_T state, ServantClient *client) :
		State(state), PClient(client) { }
	virtual ~ClientState() { }

	virtual int Login() { return -1; };
	virtual void Logout() { };
	virtual int Connect() { return -1; };
	virtual void Disconnect() { };

	virtual int SendTo(const uint8_t *buffer, int len) { return -1; }
	virtual int OnMessage(std::shared_ptr<ReceiveMessage> pmsg) { return -1; };

	virtual void OnStateForeground() { }
};

struct ClientStateLogout : public ClientState
{
	SERVANT_CLIENT_STATE_T State;

	ClientStateLogout(ServantClient *client) :
		ClientState(SERVANT_CLIENT_LOGOUT, client) { }

	virtual int Login();
};

struct ClientStateLogining : public ClientState, public cxm::util::ITimerSink
{
	SERVANT_CLIENT_STATE_T State;
	// timer for triggering connecting request repeativity
	private: std::shared_ptr<cxm::util::Timer> mtimer;
	std::mutex mmutex;

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

struct ClientStateLogin : public ClientState
{
	SERVANT_CLIENT_STATE_T State;

	ClientStateLogin(ServantClient *client) :
		ClientState(SERVANT_CLIENT_LOGIN, client) { }

	virtual int Connect();
	virtual void Logout();

	virtual int OnMessage(std::shared_ptr<ReceiveMessage> message);
	virtual void OnStateForeground();
};

struct ClientStateLogouting : public ClientState
{
	SERVANT_CLIENT_STATE_T State;

	ClientStateLogouting(ServantClient *client) :
		ClientState(SERVANT_CLIENT_LOGOUTING, client) { }

	virtual void Logout();
};

struct ClientStateConnecting : public ClientState, public cxm::util::ITimerSink
{
	SERVANT_CLIENT_STATE_T State;
	// timer for triggering connecting request repeativity
	std::shared_ptr<cxm::util::Timer> mtimer;
	std::mutex mmutex;
    // std::chrono::system_clock::time_point mlastReplyConnectTime;
    std::chrono::system_clock::time_point mstartTime;

    std::vector<std::shared_ptr<Candidate>> mcandidateGuessList;

	ClientStateConnecting(ServantClient *client);

	virtual ~ClientStateConnecting();

	virtual int Connect();
	virtual void Disconnect();
	virtual void Logout();

	virtual int OnMessage(std::shared_ptr<ReceiveMessage> message);
	virtual void OnTimer();

    private: void GenerateGuessList(std::shared_ptr<Candidate> candidate,
                     int size, int minPort);
    private: void OnReplyConnect();
    private: void OnDoP2PConnect(std::shared_ptr<ReceiveMessage> message);
    private: void OnReplyP2PConnect(std::shared_ptr<ReceiveMessage> message);
};

struct ClientStateConnected : public ClientState
{
	SERVANT_CLIENT_STATE_T State;
	uint8_t Buffer[TransceiverU::MAX_RECEIVE_BUFFER_SIZE];

	ClientStateConnected(ServantClient *client) :
		ClientState(SERVANT_CLIENT_CONNECTED, client) { }

	virtual int SendTo(const uint8_t *buffer, int len);
	virtual int OnMessage(std::shared_ptr<ReceiveMessage> message);
	virtual void Logout();
	virtual void Disconnect();
};

struct ClientStateDisconnecting : public ClientState
{
	SERVANT_CLIENT_STATE_T State;

	ClientStateDisconnecting(ServantClient *client) :
		ClientState(SERVANT_CLIENT_DISCONNECTING, client) { }

	virtual void Logout();
	virtual void Disconnect();
	void DisconnectInternal();
};

}
}

#endif

