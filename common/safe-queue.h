#ifndef _CXM_SAFE_QUEUE_H_
#define _CXM_SAFE_QUEUE_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <list>
#include <map>
#include <assert.h>
#include <chrono>

#include "log.h"

namespace cxm {
namespace alg {

template <class T>
class SafeQueue
{
    private: std::queue< std::shared_ptr<T> > mqueue;
    private: std::mutex mmutex;
    private: std::condition_variable mcv;
    private: unsigned int mWarnSize;
    private: unsigned int mMaxSize;
    private: std::string mName;

    public: SafeQueue(int maxsize = 0, int warnSize = 3) :
            mMaxSize(maxsize), mWarnSize(warnSize) {
    }

    public: bool Put(std::shared_ptr<T> item)
    {
        std::lock_guard<std::mutex> lock(mmutex);

        mqueue.push(item);
        mcv.notify_one();

        if (mqueue.size() > mWarnSize) {
            LOG(LOG_LEVEL_W, "Put queue %s with left size: %d", mName.c_str(),
                    (int)mqueue.size());
        }
        if (0 != mMaxSize && mqueue.size() > mMaxSize) {
            LOG(LOG_LEVEL_E, "Put queue %s exceed maxsize, pop: %d",
                    mName.c_str(), mMaxSize);
            mqueue.pop();
            return false;
        }

        return true;
    }

    public: std::shared_ptr<T> Get(int block = 1, std::chrono::milliseconds
                    timeout = std::chrono::milliseconds(0), bool pop = true)
    {
        std::unique_lock<std::mutex> lock(mmutex);

        std::shared_ptr<T> item;
        while (true) {
            if (mqueue.size() > 0) {
                item = mqueue.front();
                if (pop)
                    mqueue.pop();
                break;
            } else if (block) {
                if (0 == timeout.count()) {
                    mcv.wait(lock);
                } else {
                    mcv.wait_for(lock, timeout);
                }
                if (mqueue.size() == 0)
                    break;
            } else {
                break;
            }
        }

        return item;
    }

    public: void SetName(const char *name)
    {
        mName = name;
    }

    public: void SetMaxSize(int size)
    {
        mMaxSize = size;
    }

    public: void NotifyAll()
    {
        mcv.notify_all();
    }

    public: int Size()
    {
        return mqueue.size();
    }

    public: void Clear()
    {
        std::lock_guard<std::mutex> lock(mmutex);

        while (!mqueue.empty())
            mqueue.pop();
    }
};
}
}

#endif
