#ifndef _CXM_UTIL_EVENT_THREAD_H_
#define _CXM_UTIL_EVENT_THREAD_H_

#include <memory>
#include <chrono>

#include "thread.h"
#include "safe-queue.h"
#include "event-args.h"

namespace cxm {
namespace util {

class IEvent
{
	public: virtual ~IEvent() { }
	public: virtual void OnEvent() = 0;
};

class EventThread : public Thread, public IRunnable
{
	private: cxm::alg::SafeQueue<IEvent> meventQueue;
	private: volatile bool misRun;

	public: EventThread(const char *name = NULL) :
		Thread(this, name), misRun(false) { }

	public: virtual ~EventThread() { }

	public: virtual void Start()
	{
		misRun = true;
		Thread::Start();
	}

	public: virtual void Join()
	{
		misRun = false;
		meventQueue.NotifyAll();

		Thread::Join();
	}

	public: void PutEvent(std::shared_ptr<IEvent> aevent) { meventQueue.Put(aevent); }

	public: virtual void Run()
	{
		// do event loop
		while (misRun) {
			std::shared_ptr<IEvent> aevent = meventQueue.Get();
			if (NULL != aevent)
				aevent->OnEvent();
		}
	}
};


class IEventSink
{
	public: virtual ~IEventSink() { }
	public: virtual void OnEvent(int type, std::shared_ptr<IEventArgs> args) = 0;
};

class UnifyEventThread : public Thread, public IRunnable
{
	private: struct EventObject
	{
		int Type;
		std::shared_ptr<IEventArgs> Args;
		std::chrono::milliseconds fireTime;

		public: EventObject(int type, std::shared_ptr<IEventArgs> args, int mils) : Type(type), Args(args)
		{
			if (mils > 0)
				fireTime = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch()) +
					std::chrono::milliseconds(mils);
			else
				fireTime = std::chrono::milliseconds(0);
		}
	};

	private: cxm::alg::SafeQueue<EventObject> meventQueue;
	private: volatile bool misRun;
	private: IEventSink *mpsink;

	public: UnifyEventThread(const char *name = NULL) :
		Thread(this, name), misRun(false) { }

	public: virtual ~UnifyEventThread() { }

	public: virtual void Start()
	{
		misRun = true;
		Thread::Start();
	}

	public: virtual void Join()
	{
		misRun = false;
		meventQueue.NotifyAll();

		Thread::Join();
		if (0 != meventQueue.Size())
			LOGW("Unconsumed event queue size: %d", meventQueue.Size());
	}

	public: void SetSink(IEventSink *psink) { mpsink = psink; }

	public: int PutEvent(int type,
		std::shared_ptr<IEventArgs> args = std::shared_ptr<IEventArgs>(NULL))
	{
		if (NULL == mpsink || !misRun)
			return -1;

		std::shared_ptr<EventObject> eventObject;
		eventObject = std::shared_ptr<EventObject>(new EventObject(type, args, 0));

		meventQueue.Put(eventObject);
		return 0;
	}

	public: int PutEventDelay(int mils, int type,
		std::shared_ptr<IEventArgs> args = std::shared_ptr<IEventArgs>(NULL))
	{
		if (NULL == mpsink || !misRun)
			return -1;

		std::shared_ptr<EventObject> eventObject;
		eventObject = std::shared_ptr<EventObject>(new EventObject(type, args, mils));


		meventQueue.Put(eventObject);
		return 0;
	}

	public: virtual void Run()
	{
		// do event loop
		while (misRun) {
			std::shared_ptr<EventObject> aevent = meventQueue.Get();
			if (NULL == aevent.get())
				continue;

			if (aevent->fireTime.count() > 0 && aevent->fireTime >
					std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch())) {
				// reput the event and wait for a moment if necessary
				if (0 == meventQueue.Size())
					Thread::Sleep(10);
				meventQueue.Put(aevent);
			} else {
				if (NULL != aevent)
					mpsink->OnEvent(aevent->Type, aevent->Args);
			}
		}
	}
};

}
}
#endif
