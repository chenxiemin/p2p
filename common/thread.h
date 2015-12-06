/*
 * =====================================================================================
 *
 *       Filename:  thread.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/06/2014 04:07:49 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  xieminchen (mtk), hs_xieminchen@mediatek.com
 *        Company:  Mediatek
 *
 * =====================================================================================
 */

#ifndef _CXM_THREAD_H_
#define _CXM_THREAD_H_

#include <thread>
#include <assert.h>
#include <string>
#include <chrono>

#include "log.h"

namespace cxm {
namespace util {

class IRunnable
{
	public: virtual ~IRunnable() { }
	public: virtual void Run() = 0;
};

class Thread
{
	private: IRunnable *mRunnable;
	private: std::thread mThread;
	private: bool isFinish;
	private: std::string mname;

	public: Thread(IRunnable *runnable)
	{
		this->isFinish = false;
		this->mRunnable = runnable;
		this->mname = "Unname";
	}

	public: Thread(IRunnable *runnable, std::string name)
	{
		if (name.length() > 0)
			mname = name;
		else
			mname = "Unname";
		this->isFinish = false;
		this->mRunnable = runnable;
	}

	public: virtual ~Thread()
	{
		if (this->isFinish)
			return;

		LOGD("Before join thread %p", &this->mThread);
		this->Join();
		LOGD("After join thread %p", &this->mThread);
	}

	public: virtual void run()
	{
		this->mRunnable->Run();
		this->isFinish = true;
	}

	public: virtual void Start()
	{
		mThread = std::thread(ThreadHock, this);
	}

	public: virtual void Join()
	{
		if (mThread.joinable()) {
			LOGD("Before join thread: %s", mname.c_str());
			this->mThread.join();
			LOGD("After join thread: %s", mname.c_str());
		}
	}

	private: static void ThreadHock(void *thiz)
	{
		 assert(NULL != thiz);

		 Thread *theThread = (Thread *)thiz;
		 theThread->run();
	}

	public: static void Sleep(int mils)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(mils));
	}
};

}
}

#endif

