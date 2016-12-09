#ifndef _LOCKER_H
#define _LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class Sem
{
public:
  Sem()
  {
    if (sem_init(&mSem, 0, 0) != 0)
    {
      throw std::exception();
    }
  }

  ~Sem()
  {
    sem_destroy(&mSem);
  }

  bool wait()
  {
    return sem_wait(&mSem);
  }

  bool post()
  {
    return sem_post(&mSem);
  }

private:
  sem_t mSem;
};

class Locker
{
public:
  Locker()
  {
    if (pthread_mutex_init(&mMutex, NULL) != 0)
    {
      throw std::exception();
    }
  }

  ~Locker()
  {
    pthread_mutex_destroy(&mMutex);
  }

  bool lock()
  {
    return pthread_mutex_lock(&mMutex) == 0;
  }

  bool unlock()
  {
    return pthread_mutex_unlock(&mMutex) == 0;
  }

private:
  pthread_mutex_t mMutex;
  /* data */
};

class Cond
{
public:
  Cond()
  {
    if (pthread_mutex_init(&mMutex, NULL) != 0)
    {
      throw std::exception();
    }
    if (pthread_cond_init(&mCond, NULL) != 0)
    {
      pthread_mutex_destroy(&mMutex);
      throw std::exception();
    }
  }

  ~Cond()
  {
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
  }


  bool wait()
  {
    int ret = 0;
    pthread_mutex_lock(&mMutex);
    ret = pthread_cond_wait(&mCond, &mMutex);
    pthread_mutex_unlock(&mMutex);

    return ret == 0;
  }

  bool signal()
  {
    return pthread_cond_signal(&mCond) == 0;
  }

private:
  pthread_mutex_t mMutex;
  pthread_cond_t mCond;
};

#endif