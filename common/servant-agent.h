#ifndef _CXM_P2P_SERVANT_AGENT_H_
#define _CXM_P2P_SERVANT_AGENT_H_

#include <memory>

#include "thread.h"
#include "servant.h"

namespace cxm {
namespace p2p {

#if 0
typedef enum {
	SERVANT_CLIENT_AGENT_ERROR_UNKNOWN
} SERVANT_CLIENT_AGENT_ERROR_T;

class IServantClientAgentSink
{
	public: virtual ~IServantClientAgentSink() { }
	public: virtual void OnConnect() = 0;
	public: virtual void OnDisconnect() = 0;

	public: virtual void OnData(const char *data, int len) = 0;
	public: virtual void OnError(SERVANT_CLIENT_AGENT_ERROR_T error, void *opaque) = 0;
};

class ServantClientAgent : public cxm::util::IRunnable
{
	public: static const int MAX_BUFFER_SIZE = 1024 * 16 - 1;
	private: std::shared_ptr<ServantClient> msc;
	private: IServantClientAgentSink *mpdelegate;
	private: char mbuf[MAX_BUFFER_SIZE + 1];

	private: std::shared_ptr<cxm::util::Thread> mthread;
	private: bool misRun;
	private: bool misConnectFired;

	public: ServantClientAgent(const char *serverIp);

	public: int Call();
	public: void Hangup();
	public: int Send(const char *data, int len);
			
	private: virtual void Run();

	public: void SetSink(IServantClientAgentSink *delegate) { mpdelegate = delegate; }
	public: void SetName(std::string name) { msc->SetName(name); }
	public: void SetRemote(std::string remote) { msc->SetRemote(remote); }
};
#endif
}
}

#endif
