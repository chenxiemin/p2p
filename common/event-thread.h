#ifndef _CXM_UTIL_EVENT_THREAD_H_
#define _CXM_UTIL_EVENT_THREAD_H_

#include <memory>

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

	public: void PutEvnet(std::shared_ptr<IEvent> aevent) { meventQueue.Put(aevent); }

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

		public: EventObject(int type, std::shared_ptr<IEventArgs> args) :
			Type(type), Args(args) { }
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
	}

	public: void SetSink(IEventSink *psink) { mpsink = psink; }

	public: int PutEvnet(int type,
		std::shared_ptr<IEventArgs> args = std::shared_ptr<IEventArgs>(NULL))
	{
		if (NULL == mpsink || !misRun)
			return -1;

		std::shared_ptr<EventObject> eventObject;
		eventObject = std::shared_ptr<EventObject>(new EventObject(type, args));

		meventQueue.Put(eventObject);
		return 0;
	}

	public: virtual void Run()
	{
		// do event loop
		while (misRun) {
			std::shared_ptr<EventObject> aevent = meventQueue.Get();
			if (NULL != aevent)
				mpsink->OnEvent(aevent->Type, aevent->Args);
		}
	}
};

}
}
#endif
