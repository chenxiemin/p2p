#ifndef _CXM_UTIL_TIMER_H_
#define _CXM_UTIL_TIMER_H_

#include <chrono>
#include <memory>

#include "thread.h"
#include "event-args.h"

namespace cxm {
namespace util {

class ITimerSink
{
	public: virtual ~ITimerSink() { }
	public: virtual void OnTimer() = 0;
};

class Timer : public IRunnable
{
	private: ITimerSink *mpsink;
	// duration until the timer be ellasped
	private: std::chrono::microseconds mduration;
	// interval of the timer to trigger
	private: std::chrono::microseconds minterval;

	private: std::chrono::milliseconds monce;

	private: std::shared_ptr<Thread> mthread;
	private: bool misRun = false;

	public: Timer(ITimerSink *psink, std::chrono::microseconds interval)
	{
		assert(NULL != psink);
		mpsink = psink;
		mduration = interval;
		minterval = interval;
		monce = std::chrono::milliseconds(100);
	}

	public: virtual ~Timer()
	{
		this->Stop();
	}

	public: void Start(bool repeat)
	{
		this->Stop();

		misRun = repeat;
		mthread = std::shared_ptr<Thread>(new Thread(this, "Timer"));
		mthread->Start();
	}

	public: void Stop()
	{
		if (NULL == mthread.get())
			return;

		misRun = false;
		mthread->Join();
		mthread.reset();
	}

	public: virtual void Run()
	{
		// at least do once
		do {
			std::this_thread::sleep_for(monce);

			mduration -= monce;
			if (mduration.count() <= 0) {
				// trigger OnTimer, then reset the duration
				mpsink->OnTimer();
				mduration = minterval;
			}
		} while (misRun);
	}
};

}
}
#endif
