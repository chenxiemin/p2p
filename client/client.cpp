#include <iostream>
#include <string>

#include "servant.h"
#include "log.h"
#include "timer.h"
#include "udp.h"

using namespace std;
using namespace std::chrono;
using namespace cxm::util;
using namespace cxm::p2p;

class MyAgent : public IServantClientSink, public IServantClientDataSink,
    public ITimerSink
{
	string mstr;
	shared_ptr<ServantClient> msc;
	shared_ptr<Timer> mtimer;
	int count;

	public: MyAgent(const char *ip, const char *name, const char *remote = NULL)
	{
		mstr = "chenxiemin";

		msc = shared_ptr<ServantClient>(new ServantClient(ip));
		msc->SetName(name);
		if (NULL != remote)
			msc->SetRemote(remote);
		else
			msc->SetRemote("");
		msc->SetSink(this);
        msc->SetDataSink(this);

		initNetwork();

		int res = msc->Login();
		if (0 != res) {
			LOGE("Login fail");
			return;
		}
	}

	public: ~MyAgent()
	{
		msc->Logout();
	}

	public: virtual void OnLogin()
	{
		LOGI("OnLogin with remote name: %s", this->msc->GetRemote().c_str());

		// auto connect after login
		if (this->msc->GetRemote().length() > 0)
			this->msc->Connect();
	}

	public: virtual void OnLogout()
	{
		LOGI("OnLogout");
	}

	public: virtual void OnConnect()
	{
		LOGI("OnConnect");
		count = 0;
		mtimer = shared_ptr<Timer>(new Timer(this, milliseconds(100)));
		mtimer->Start(true);
	}

	public: virtual void OnDisconnect()
	{
		LOGI("OnDisconnect");
		if (NULL != mtimer.get())
			mtimer->Stop();
		mtimer.reset();
	}

	public: virtual void OnTimer()
	{
		stringstream ss;
		ss << "Hello " << msc->GetRemote();
		ss << " I'm " << msc->GetName();
		ss << ": " << count++;
		string str = ss.str();
		msc->SendTo((uint8_t *)str.c_str(), str.length());
	}

	public: virtual void OnData(shared_ptr<P2PPacket> packet)
	{
		char *bufstr = new char[packet->GetLength() + 1];
		memcpy(bufstr, packet->GetData(), packet->GetLength());
		bufstr[packet->GetLength()] = '\0';
		LOGI("OnData: %s", bufstr);
		delete []bufstr;
	}

	public: virtual void OnError(SERVANT_CLIENT_ERROR_T error, void *opaque)
	{

	}
};

int main(int argc, char **argv)
{
	if (argc < 3) {
		LOGE("Usage: ./p2pclient servantip myname remotename");
		return -1;
	}

	MyAgent md(argv[1], argv[2], argv[3]);

	getchar();

	return 0;
}

