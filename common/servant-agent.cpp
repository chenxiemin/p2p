#include "servant-agent.h"
#include "log.h"

using namespace std;
using namespace cxm::util;

#if 0
namespace cxm {
namespace p2p {

ServantClientAgent::ServantClientAgent(const char *servarIp) :
	mpdelegate(NULL), misRun(false), misConnectFired(false)
{
	msc = shared_ptr<ServantClient>(new ServantClient(servarIp));
}

int ServantClientAgent::Call()
{
	if (NULL != mthread.get()) {
		LOGE("Already in call");
		return -1;
	}

	misRun = true;
	mthread = shared_ptr<Thread>(new Thread(this, "ServantClientAgent"));
	mthread->Start();

	return 0;
}

void ServantClientAgent::Hangup()
{
	if (NULL == mthread.get())
		return;

	misRun = false;
	mthread->Join();
	mthread.reset();
}

int ServantClientAgent::Send(const char *buffer, int len)
{
	if (len > MAX_BUFFER_SIZE) {
		LOGE("Buffer length exceed: %d", len);
		return -1;
	}

	LOGE("Unimplement method");
	return 0;
}

void ServantClientAgent::Run()
{
	while (misRun) {
		ServantClient::ClientStatus status = msc->GetStatus();
		switch (status) {
		case ServantClient::ClientStatus::SERVANT_CLIENT_UNLOGIN: {
			int res = msc->Login();
			if (0 != res)
				LOGE("Cannot login: %d", res);
			break;
		} case ServantClient::ClientStatus::SERVANT_CLIENT_LOGIN: {
			int res = msc->Request();
			if (0 != res)
				LOGE("Cannot request: %d", res);
			break;
		} case ServantClient::ClientStatus::SERVANT_CLIENT_REQUESTED: {
			int res = msc->Connect();
			if (0 != res)
				LOGE("Cannot connect: %d", res);
			break;
		} case ServantClient::ClientStatus::SERVANT_CLIENT_CONNECTED: {
			if (!misConnectFired) {
				if (NULL != mpdelegate)
					mpdelegate->OnConnect();
				misConnectFired = true;
			}
			break;
		} case ServantClient::ClientStatus::SERVANT_CLIENT_REQUESTING:
		case ServantClient::ClientStatus::SERVANT_CLIENT_CONNECTING: {
			Thread::Sleep(100);
			break;
		} default:
			LOGE("Unknown servant client status: %d", status);
			break;
		}
	}
}

}
}

#endif