#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "Locker.h"

template <class T>
class ThreadPool
{
public:
  ThreadPool(int threadNumber = 8, int maxRequests = 10000);
  ~ThreadPool();

  bool append(T *request);

private:
  static void *worker(void *arg);
  void run();

private:
  int mThreadNumber;
  int mMaxRequests;
  pthread_t *mThreads;
  std::list<T *>mWorkqueue;
  Locker mQueuelocker;
  Sem mQueuestat;
  bool mStop;

};

template <typename T>
ThreadPool<T>::ThreadPool(int threadNumber, int maxRequests)
  : mThreadNumber(threadNumber),
    mMaxRequests(maxRequests),
    mStop(false),
    mThreads(NULL)
{
  if ((threadNumber <= 0) || maxRequests <= 0)
  {
    throw std::exception();
  }

  mThreads = new pthread_t[mThreadNumber];
  if (mThreads == NULL)
  {
    throw std::exception();
  }

  for (int i = 0; i < threadNumber; ++i)
  {
    printf("create the %dth thread.\n", i);

    if (pthread_create(mThreads + 1, NULL, worker, this) != 0)
    {
      delete []mThreads;
      throw std::exception();
    }

    if (pthread_detach(mThreads[i]))
    {
      delete []mThreads;
      throw std::exception();
    }
  }
}


template <typename T>
ThreadPool<T>::~ThreadPool()
{
  delete []mThreads;
  mStop = true;
}

template <typename T>
bool ThreadPool<T>::append(T *request)
{
  mQueuelocker.lock();
  if (mWorkqueue.size() > mMaxRequests)
  {
    mQueuelocker.unlock();
    return false;
  }

  mWorkqueue.push_back(request);
  mQueuelocker.unlock();
  mQueuestat.post();

  return true;
}

template <typename T>
void *ThreadPool<T>::worker(void *arg)
{
  ThreadPool *pool = (ThreadPool *)arg;
  pool->run();

  return pool;
}

template <typename T>
void ThreadPool<T>::run()
{
  while(!mStop)
  {
    mQueuestat.wait();
    mQueuelocker.lock();

    if (mWorkqueue.empty())
    {
      mQueuelocker.unlock();
      continue;
    }

    T *request = mWorkqueue.front();
    mWorkqueue.pop_front();
    mQueuelocker.unlock();
    if (!request)
    {
      continue;
    }

    request->process();
  }
}

#endif