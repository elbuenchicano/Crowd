/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    THREAD.CPP: threads for performing multiple jobs.

    Marcelo Bernardes Vieira
**************************************************************************************/
#include "system.h"

// Function to retrieve the current number of processors
unsigned int gcgGetNumberOfProcessors() {
  // Platform specific number of processors retrieval
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  /* Unix specific code */
  long nprocs = -1;
  long nprocs_max = -1;

  nprocs = sysconf(_SC_NPROCESSORS_ONLN);
  if (nprocs < 1) return 1; // Could not determine number of CPUs online

  nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
  if(nprocs_max < 1) return 1;// Could not determine number of CPUs configured

  return MAX(nprocs, nprocs_max);
#else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
}

#if defined(GCG_LINUX32) || defined(GCG_LINUX64)
#include <cxxabi.h>
#endif

//////////////////////////////////////////////////////////////////////
// gcgTHREAD: implements a single thread for parallel execution
//////////////////////////////////////////////////////////////////////
// Internal struct: needed to free the gcg.h from depending on pthread.
typedef struct _GCG_THREAD {
  pthread_t       asynchthread;   // Thread for asynch join.
  volatile bool   threadalive;    // True if thread is runnning
  pthread_mutex_t threadmutex;    // Thread mutex
} GCG_THREAD;

gcgTHREAD::gcgTHREAD() {
  this->handle = (GCG_THREAD*) ALLOC(sizeof(GCG_THREAD));
  if(handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgTHREAD(): constructor failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  // Information
  GCG_THREAD *thread = (GCG_THREAD*) handle;

  // Multithread information
  memset(&thread->asynchthread, 0, sizeof(thread->asynchthread));
  thread->threadalive = false; // Not running
  pthread_mutex_init(&thread->threadmutex, NULL);
  // No error
}

// Thread destructor. Uses waitThread()
gcgTHREAD::~gcgTHREAD() {
  if(handle != NULL) {
    if(!this->waitThread())
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "~gcgTHREAD(): destruction of gcgTHREAD failed calling waitThread(). (%s:%d)", basename((char*)__FILE__), __LINE__);

    GCG_THREAD *thread = (GCG_THREAD*) this->handle;

    if(pthread_mutex_destroy(&thread->threadmutex) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "~gcgTHREAD(): could not destroy the mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);

    SAFE_FREE(handle);
  }
}



static void processorcancelation(void *handle) {
  gcgTHREAD *gcgthread = (gcgTHREAD*) handle;

  if(gcgthread != NULL) {
    GCG_THREAD *thread = (GCG_THREAD*) gcgthread->handle;

    // Thread is dead
    thread->threadalive = false;

    // Close critical session
    pthread_mutex_unlock(&thread->threadmutex);
  } else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INVALIDOBJECT), "processorcancelation(): attempt to use an invalid gcgTHREAD handle. (%s:%d)", basename((char*)__FILE__), __LINE__);

  // Detach current thread: important if number of threads varies a lot
  pthread_detach(pthread_self()); // It it no longer joinable
}



// Executes the run() method in a separate thread. Returns true if the
// thread is correctly created.
static void *processor(void *handle) {
  int old;
  gcgTHREAD *gcgthread = (gcgTHREAD*) handle;

  if(gcgthread == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INVALIDOBJECT), "processor(): attempt to use an invalid gcgTHREAD handle. Thread cancelled. (%s:%d)", basename((char*)__FILE__), __LINE__);
    pthread_exit(NULL);
    return NULL;
  }
  GCG_THREAD *thread = (GCG_THREAD*) gcgthread->handle;

   // Cancellation handler that tries to keep the lock released.
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);
  pthread_cleanup_push(processorcancelation, handle);

  // Lock thread
  pthread_mutex_lock(&thread->threadmutex);
    // Thread is running
    thread->threadalive = true;
  // Unlock thread
  pthread_mutex_unlock(&thread->threadmutex);

  try {
    // Executes gcgTHREAD::run()
    if(gcgthread != NULL) gcgthread->run();
  }
#if defined(GCG_LINUX32) || defined(GCG_LINUX64)
    catch (abi::__forced_unwind&) { // If canceled by pthreads, this exception must be rethrown to unwind the stack during thread exit
      throw;
    }
#endif
    catch(...) {
      // An exception ocurred
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_EXCEPTION), "processor(): exception during thread execution. (%s:%d)", basename((char*)__FILE__), __LINE__);
    }

  // Lock thread
  pthread_mutex_lock(&thread->threadmutex);

  // Pop cancellation function and execute it
  pthread_cleanup_pop(1);

  // Exit thread
  pthread_exit(NULL);
  return NULL;
}

bool gcgTHREAD::startThread() {
  // Check thread
  GCG_THREAD *thread = (GCG_THREAD*) handle;
  if(!thread) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "startThread(): attempt to use an invalid GCG_THREAD handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock thread
  pthread_mutex_lock(&thread->threadmutex);

  if(thread->threadalive) {
    // Unlock thread
    pthread_mutex_unlock(&thread->threadmutex);

    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_ALREADYRUNNING), "startThread(): the current thread is running. Finish it before a new thread can be started. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  try {
    memset(&thread->asynchthread, 0, sizeof(thread->asynchthread));

    // Create message processor thread
    if(pthread_create(&thread->asynchthread, NULL, processor, this) == 0) {
      // Unlock thread
      pthread_mutex_unlock(&thread->threadmutex);

      // No error
      return true;
    }
  } catch(...) {}


  // An error occured
  thread->threadalive = false;

  // Unlock thread
  pthread_mutex_unlock(&thread->threadmutex);

  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_CREATIONFAILED), "startThread(): the thread was not started. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return false;
}

// Stops the thread processor(). Force the stop by thread cancelation.
bool gcgTHREAD::abortThread() {
  GCG_THREAD *thread = (GCG_THREAD*) handle;
  if(!thread) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "abortThread(): attempt to use an invalid GCG_THREAD handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock thread
  pthread_mutex_lock(&thread->threadmutex);

  // Cancel the thread, if it is running
  bool res = true;
  if(thread->threadalive) {
    int err = pthread_cancel(thread->asynchthread);
    if(err != 0 && err != ESRCH) { // ESRCH: no such process
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "abortThread(): failed to cancel thread. (%s:%d)", basename((char*)__FILE__), __LINE__);
      res = false;
    }

    err = pthread_detach(thread->asynchthread);
    if(err != 0 && err != ESRCH) { // ESRCH: no such process
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "abortThread(): failed to detach thread. (%s:%d)", basename((char*)__FILE__), __LINE__);
      res = false;
    }

    // Assuming ASSYNCHRONOUS cancelation: but it does not work on WINDOWS
    thread->threadalive = false;
  }

  // Unlock thread
  pthread_mutex_unlock(&thread->threadmutex);

  return res;
}

// Returns true if the thread is running.
bool gcgTHREAD::isRunning() {
  GCG_THREAD *thread = (GCG_THREAD*) handle;
  if(!thread) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "isRunning(): attempt to use an invalid GCG_THREAD handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock thread
  pthread_mutex_lock(&thread->threadmutex);
    bool res = thread->threadalive;
  // Unlock thread
  pthread_mutex_unlock(&thread->threadmutex);

  return res;
}


// Waits the run() to finish.
bool gcgTHREAD::waitThread() {
  GCG_THREAD *thread = (GCG_THREAD*) this->handle;
  if(!thread) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "waitThread(): attempt to use an invalid GCG_THREAD handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock thread
  pthread_mutex_lock(&thread->threadmutex);
     // Is this thread the same thread?
    pthread_t cur = pthread_self();
    if(pthread_equal(cur, thread->asynchthread)) {
      // The thread is trying to wait for itself
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_UNSUPPORTEDOPERATION), "waitThreadl(): the thread of gcgTHREAD is not allowed call its own waitThreadl() to avoid deadlock. (%s:%d)", basename((char*)__FILE__), __LINE__);

      // Release lock
      pthread_mutex_unlock(&thread->threadmutex);
      return false;
    }

    bool res = !thread->threadalive;
  // Unlock thread
  pthread_mutex_unlock(&thread->threadmutex);

  // Nothing to do
  if(res) return true;

  // Waits the thread to exit
  void *ret;
  int a = pthread_join(thread->asynchthread, &ret);
  res = (a == 0) || (a == ESRCH); // ESRCH: no such process

  if(!res)
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_EXITFAILURE), "waitThread(): an error occured trying to join the thread. (%s:%d)", basename((char*)__FILE__), __LINE__);

  return res;
}


//////////////////////////////////////////////////////////////////////
// gcgLOCK: implements a critical section control using mutex
//////////////////////////////////////////////////////////////////////
// Internal struct: needed to free the gcg.h from depending on pthread.
typedef struct _GCG_LOCK {
  pthread_mutex_t lockmutex;  // Thread mutex
  pthread_cond_t  waitcond;   // Wait condition
} GCG_LOCK;

gcgLOCK::gcgLOCK() {
  this->handle = (GCG_LOCK*) ALLOC(sizeof(GCG_LOCK));
  if(handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgLOCK(): constructor failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  GCG_LOCK *_lock = (GCG_LOCK*) handle;

  // Init lock
  if(pthread_mutex_init(&_lock->lockmutex, NULL) != 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "gcgLOCK(): could not initialize the mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  pthread_mutex_lock(&_lock->lockmutex);
    // Allocate wait condition
    if(pthread_cond_init(&_lock->waitcond, NULL) != 0) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_CREATIONFAILED), "gcgLOCK(): wait condition creation failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
      pthread_mutex_unlock(&_lock->lockmutex);
      return;
    }
  pthread_mutex_unlock(&_lock->lockmutex);

  // No error
}

// Lock destructor.
gcgLOCK::~gcgLOCK() {
  GCG_LOCK *_lock = (GCG_LOCK*) handle;
  if(_lock) {
    pthread_mutex_lock(&_lock->lockmutex);
      // Wakes up all waiting threads, if any
      pthread_cond_broadcast(&_lock->waitcond);

      if(pthread_cond_destroy(&_lock->waitcond) != 0)
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "~gcgLOCK(): failed to destroy job condition. (%s:%d)", basename((char*)__FILE__), __LINE__);

    pthread_mutex_unlock(&_lock->lockmutex);

    if(pthread_mutex_destroy(&_lock->lockmutex) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "~gcgLOCK(): could not destroy the mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);

    FREE(handle); // It must be free
  }
}

// Locks this critical section lock. If it's unavalaible, waits the lock
// for time-out miliseconds. If the lock is seized, returns true.
bool gcgLOCK::lock(long _timeout) {
  // Check lock
  GCG_LOCK *_lock = (GCG_LOCK*) handle;
  if(!_lock) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "lock(): attempt to use an invalid GCG_LOCK handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(_timeout >= 0) {
    struct timespec timeout;
    gcgComputeTimeout(&timeout, _timeout);

    // Lock the critical section lock
    int res = pthread_mutex_timedlock(&_lock->lockmutex, &timeout);

    if(res != 0) {
      if(res == ETIMEDOUT) gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_THREAD, GCG_TIMEOUT), "lock(): timeout elapsed and the lock was not acquired. (%s:%d)", basename((char*)__FILE__), __LINE__);
      else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_LOCKERROR), "lock(): the mutex could not be locked. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return false;
    }

    return true;
  }

  int res = pthread_mutex_lock(&_lock->lockmutex);
  if(res != 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_LOCKERROR), "lock(): the mutex could not be locked. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  return true;
}

// Unlocks this critical section lock. Returns true if the lock was released.
bool gcgLOCK::unlock() {
  // Check lock
  GCG_LOCK *_lock = (GCG_LOCK*) handle;
  if(!_lock) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "unlock(): attempt to use an invalid GCG_LOCK handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Unlock the critical section lock
  int res = pthread_mutex_unlock(&_lock->lockmutex);
  if(res != 0) {
    if(res == EPERM) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_OWNERSHIPERROR), "unlock(): the lock is not owned by current thread. (%s:%d)", basename((char*)__FILE__), __LINE__);
    else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "unlock(): the mutex could not be unlocked. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  return true;
}

// Waits for an event or resource
bool gcgLOCK::wait(long timeoutUsec) {
  // Check lock
  GCG_LOCK *_lock = (GCG_LOCK*) handle;
  if(!_lock) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "wait(): attempt to use an invalid GCG_LOCK handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock object
  pthread_mutex_lock(&_lock->lockmutex);

  int res;
  if(timeoutUsec >= 0) {
    struct timespec timeout;
    gcgComputeTimeout(&timeout, timeoutUsec);

    // Waits for the condition
    res = pthread_cond_timedwait(&_lock->waitcond, &_lock->lockmutex, &timeout);
  } else res = pthread_cond_wait(&_lock->waitcond, &_lock->lockmutex);

  if(res != 0) {
    if(res == ETIMEDOUT) gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_THREAD, GCG_TIMEOUT), "wait(): timeout elapsed and the resource may not be available. (%s:%d)", basename((char*)__FILE__), __LINE__);
    else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_LOCKERROR), "wait(): error waiting for a resource or an event. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // The lock might be ok or not
    return false;
  }

  // The lock is currently ok
  return true;
}

// Wakes one or more waiting threads
bool gcgLOCK::wakeUp(int numberOfThreads) {
  // Check lock
  GCG_LOCK *_lock = (GCG_LOCK*) handle;
  if(!_lock) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "wait(): attempt to use an invalid GCG_LOCK handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(numberOfThreads == 0) return true;

  // Lock object
  pthread_mutex_lock(&_lock->lockmutex);
    if(numberOfThreads < 0) pthread_cond_broadcast(&_lock->waitcond); // wake up all
    else
      for(int i = 0; i < numberOfThreads; i++) // Wake up one or more threads
        pthread_cond_signal(&_lock->waitcond);

  // Unlocks main lock
  pthread_mutex_unlock(&_lock->lockmutex);

  // The lock is currently ok
  return true;
}


//////////////////////////////////////////////////////////////////////
// gcgTHREADPOOL: implements a pool of threads for parallel execution
//////////////////////////////////////////////////////////////////////

// Internal struct: needed to free the gcg.h from depending on pthread.
typedef struct _GCG_THREADPOOL {
  ////////////////////////////////////////////////////////////////////////////////
  // Queue of monitor threads: when accessing data enclosed in this area /////////
  pthread_mutex_t globalmutex;    // Mutex for accessing this struct
  pthread_cond_t  monitorcond;    // Signals all monitors when a thread is created or finished
  pthread_cond_t  waitjobcond;    // Signals all threads when a job is available.

  pthread_t       *threads;                // Pointer for all thread descriptors
  gcgLINKEDLIST   *jobQueue;      // List of jobs
  gcgQUEUE        *executed;      // Queue for executed jobs
  gcgQUEUE        *discarded;     // Queue for discarded jobs

  volatile unsigned int    numThreads;     // Number of threads working
  volatile unsigned int    busy;           // Number of threads executing jobs
  volatile unsigned int    stopThreads;    // Indicates how many threads must be stopped
  volatile bool            run;            // Flags if it is running
  ////////////////////////////////////////////////////////////////////////////////
} GCG_THREADPOOL;

// Thread pool constructor
gcgTHREADPOOL::gcgTHREADPOOL(unsigned int numThreads) {
  // Try to allocate object space. We need this to free the gcg.h from using pthread
  GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) ALLOC(sizeof(GCG_THREADPOOL));
  if((this->handle = threadpool) == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgTHREADPOOL(): constructor failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  // Create global mutex
  if(pthread_mutex_init(&threadpool->globalmutex, NULL) != 0) {
    SAFE_FREE(this->handle);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "gcgTHREADPOOL(): could not initialize the control mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  // Create condition for threads waiting jobs
  if(pthread_cond_init(&threadpool->waitjobcond, NULL) != 0) {
    SAFE_FREE(this->handle);
    pthread_mutex_destroy(&threadpool->globalmutex);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_CREATIONFAILED), "gcgTHREADPOOL(): creation of waiting job condition failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  // Create condition for monitor threads
  if(pthread_cond_init(&threadpool->monitorcond, NULL) != 0) {
    SAFE_FREE(this->handle);
    pthread_mutex_destroy(&threadpool->globalmutex);
    pthread_cond_destroy(&threadpool->waitjobcond);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_CREATIONFAILED), "gcgTHREADPOOL(): creation of monitor condition failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  // Not running
  threadpool->executed = NULL;
  threadpool->discarded = NULL;
  threadpool->busy = 0;
  threadpool->stopThreads = 0;
  threadpool->threads = NULL;
  threadpool->jobQueue = NULL;
  threadpool->numThreads = 0;
  threadpool->run = false;  // Flags if it is working
  this->setNumberOfThreads(numThreads);
}


// Thread pool destruction
gcgTHREADPOOL::~gcgTHREADPOOL() {
  GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INVALIDOBJECT), "~gcgTHREADPOOL(): attempt to use an invalid GCG_THREADPOOL handle. Thread cancelled. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  if(!this->waitAndDestroy())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "~gcgTHREADPOOL(): could not destroy the thread pool by calling waitAndDestroy(). (%s:%d)", basename((char*)__FILE__), __LINE__);

  // Lock global mutex
  pthread_mutex_lock(&threadpool->globalmutex);

  if(pthread_cond_destroy(&threadpool->monitorcond) != 0)
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "~gcgTHREADPOOL(): failed to destroy monitor condition. (%s:%d)", basename((char*)__FILE__), __LINE__);

  if(pthread_cond_destroy(&threadpool->waitjobcond) != 0)
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "~gcgTHREADPOOL(): failed to destroy waiting job condition. (%s:%d)", basename((char*)__FILE__), __LINE__);

  // Unlock global mutex
  pthread_mutex_unlock(&threadpool->globalmutex);

  if(pthread_mutex_destroy(&threadpool->globalmutex) != 0)
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "~gcgTHREADPOOL(): could not destroy the global mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);

  // Free object attributes structure
  SAFE_FREE(this->handle);
}

static void threadcancelation(void *object) {
  GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) ((gcgTHREADPOOL*) object)->handle;

  if(threadpool != NULL) {
    // Adjust counter for stopping threads
    if(threadpool->stopThreads > 0) threadpool->stopThreads--;

    // Find thread position in the gcgTHREADPOOL object
    if(threadpool->threads != NULL) { // It might have been deleted in a catastrophic scenario
      pthread_t cur = pthread_self();
      int pos = -1;
      for(unsigned int i = 0; i < threadpool->numThreads; i++)
        if(pthread_equal(cur, threadpool->threads[i])) {
          pos = (int) i;
          break;
        }

      // Adjust thread array and counter
      if(pos >= 0 && pos < (int) threadpool->numThreads)
        for(unsigned int i = (unsigned int) pos; i + 1 < threadpool->numThreads; i++)
          memcpy(&threadpool->threads[i], &threadpool->threads[i+1], sizeof(pthread_t));
      else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INVALIDOBJECT), "threadcancelation(): urecognized thread accessed thread pool. (%s:%d)", basename((char*)__FILE__), __LINE__);
    }

    // Thread stopped
    if(threadpool->numThreads > 0) threadpool->numThreads--;

    // Wake up monitors
    pthread_cond_broadcast(&threadpool->monitorcond);

    // Close critical session for monitors
    pthread_mutex_unlock(&threadpool->globalmutex);
  } else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INVALIDOBJECT), "threadcancelation(): attempt to use an invalid GCG_THREADPOOL handle. Thread cancelled. (%s:%d)", basename((char*)__FILE__), __LINE__);

  // Detach current thread: important if number of threads varies a lot
  pthread_detach(pthread_self());
}

// Executes the jobs using the pool of threads
static void* threadExecute(void *object) {
  GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) ((gcgTHREADPOOL*) object)->handle;
  gcgJOB *job = NULL;
  bool failed;
  int old;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INVALIDOBJECT), "threadExecute(): attempt to use an invalid GCG_THREADPOOL handle. Thread cancelled. (%s:%d)", basename((char*)__FILE__), __LINE__);
    pthread_exit(NULL);
    return NULL;
  }

  // Cancellation handler that tries to keep the lock released.
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);
  pthread_cleanup_push(threadcancelation, object);

  // Lock waiting job mutex
  pthread_mutex_lock(&threadpool->globalmutex);

  // Waits for jobs...
  while(true) {
    // Blocks until a job is available, threadpool is canceled or some threads must me finished
    while(threadpool->run && threadpool->stopThreads == 0 && threadpool->jobQueue != NULL && threadpool->threads != NULL) {
      if(threadpool->jobQueue->counter != 0) break;// Here threadpool->jobQueue was tested
      pthread_cond_wait(&threadpool->waitjobcond, &threadpool->globalmutex);
    }

    // Is still executing?
    if(!threadpool->run || threadpool->jobQueue == NULL || threadpool->threads == NULL || threadpool->stopThreads > 0) break;

    // Uniquely obtains a job
    job = (gcgJOB*) threadpool->jobQueue->dequeueFirst();

    // Execute the job
    if(job) {
      // About to execute a job
      threadpool->busy++;
      pthread_cond_broadcast(&threadpool->monitorcond); // Signal thread state change

      // Unlock main mutex
      pthread_mutex_unlock(&threadpool->globalmutex);

      failed = false;
      try {
        job->run();
      }
#if defined(GCG_LINUX32) || defined(GCG_LINUX64)
        catch (abi::__forced_unwind&) { // If canceled by pthreads, this exception must be rethrown to unwind the stack during thread exit
          throw;
        }
#endif
        catch(...) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_EXCEPTION), "threadExecute(): exception during job execution. Trying to delete or insert it into discarded queue. (%s:%d)", basename((char*)__FILE__), __LINE__);
          failed = true;
        }

      // Lock waiting job mutex
      pthread_mutex_lock(&threadpool->globalmutex);

      // Thread is not busy
      threadpool->busy--;
      pthread_cond_broadcast(&threadpool->monitorcond); // Signal thread state change

      if(failed) {
        // Exception occurred
        if(threadpool->discarded == NULL) SAFE_DELETE(job)
        else threadpool->discarded->enqueueTail(job);
      } else {
        // Job Executed
        if(threadpool->executed == NULL) SAFE_DELETE(job)
        else threadpool->executed->enqueueTail(job);
      }
    }
  }

  // Pop cancellation function and execute it
  pthread_cleanup_pop(1);

  // Exit thread
  pthread_exit(NULL);
  return NULL;
}


// Creates a pool with numThreads threads.
bool gcgTHREADPOOL::setNumberOfThreads(unsigned int numThreads) {
  register GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setNumberOfThreads(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Get global lock
  pthread_mutex_lock(&threadpool->globalmutex);

  // Extra check after lock, in case of object has been destroyed while waiting
  if(this->handle == NULL) return false;

  // Is this thread in the threadpool?
  pthread_t cur = pthread_self();
  for(unsigned int i = 0; i < threadpool->numThreads; i++)
    if(pthread_equal(cur, threadpool->threads[i])) {
      // A JOB is trying to change the naumber of threads
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_UNSUPPORTEDOPERATION), "setNumberOfThreads(): a job of a gcgTHREADPOOL is not allowed to call its setNumberOfThreads() to avoid deadlock. (%s:%d)", basename((char*)__FILE__), __LINE__);

      // Release lock
      pthread_mutex_unlock(&threadpool->globalmutex);
      return false;
    }

  // Check if it is not running
  if(!threadpool->run) {
    // Check transient state
    if(threadpool->threads != NULL || threadpool->jobQueue != NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_THREAD, GCG_NOTRUNNING), "setNumberOfThreads(): thread pool is being destroyed. (%s:%d)", basename((char*)__FILE__), __LINE__);
      // Release lock
      pthread_mutex_unlock(&threadpool->globalmutex);
      return false;
    }

    // Nothing to do?
    if(numThreads == 0) {
      // Release lock
      pthread_mutex_unlock(&threadpool->globalmutex);
      return true;
    }

    // Allocate the job queue
    SAFE_DELETE(threadpool->jobQueue);
    threadpool->jobQueue = new gcgLINKEDLIST();
    if(!threadpool->jobQueue) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "setNumberOfThreads(): job queue not allocated. (%s:%d)", basename((char*)__FILE__), __LINE__);

      // Release lock
      pthread_mutex_unlock(&threadpool->globalmutex);
      return false;
    }

    //  Pool attributes
    // Preserse this: threadpool->executed = NULL;
    // Preserse this: threadpool->discarded = NULL;
    threadpool->busy = 0;
    threadpool->stopThreads = 0;
    threadpool->run = true;       // Signal as working

    // Create thread array
    SAFE_FREE(threadpool->threads);
    threadpool->threads = (pthread_t*) ALLOC(numThreads * sizeof(pthread_t));
    if(threadpool->threads == NULL) {
      SAFE_DELETE(threadpool->jobQueue);
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "setNumberOfThreads(): thread buffer was not allocated. (%s:%d)", basename((char*)__FILE__), __LINE__);

      // Release lock
      pthread_mutex_unlock(&threadpool->globalmutex);
      return false;
    }

    // Create threads
    threadpool->numThreads = 0;
    for(unsigned int i = 0; i < numThreads; i++) {
      if(pthread_create(&threadpool->threads[i], NULL, threadExecute, (void*) this) != 0) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_CREATIONFAILED), "setNumberOfThreads(): only %d/%d threads were started. (%s:%d)", threadpool->numThreads, numThreads, basename((char*)__FILE__), __LINE__);

        // Release lock
        pthread_mutex_unlock(&threadpool->globalmutex);
        return this->destroyPool();
      }
      threadpool->numThreads++;
    }

    // Release lock
    pthread_mutex_unlock(&threadpool->globalmutex);

    // Thread pool created
    return true;
  }

  // Destroy thread pool?
  if(numThreads == 0) {
    // Release lock
    pthread_mutex_unlock(&threadpool->globalmutex);

    // Waits old pool to stop
    if(!this->waitAndDestroy()) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "setNumberOfThreads(): thread pool was not fully destroyed. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return false;
    }

    // Thread pool released
    return true;
  }

  // Nothing to do?
  if(numThreads == threadpool->numThreads) {
    // Release lock
    pthread_mutex_unlock(&threadpool->globalmutex);
    return true;
  }

  // Reduction of the number of threads... wake all threads and wait
  if(numThreads < threadpool->numThreads) {
    bool result;
    threadpool->stopThreads = threadpool->numThreads - numThreads;

    // Waits until the extra threads are stopped
    while(numThreads < threadpool->numThreads && threadpool->run) {
      pthread_cond_signal(&threadpool->waitjobcond);
      pthread_cond_wait(&threadpool->monitorcond, &threadpool->globalmutex);
    }

    // Init result
    result = threadpool->run;

    if(threadpool->run && numThreads != threadpool->numThreads) {
      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_THREAD, GCG_EXITFAILURE), "setNumberOfThreads(): failed reducing number of threads. (%s:%d)", basename((char*)__FILE__), __LINE__);
      result = false;
    }

    // Wake up all monitors
    pthread_cond_broadcast(&threadpool->monitorcond);

    // Release lock
    pthread_mutex_unlock(&threadpool->globalmutex);
    return result;
  }

  // Threadpool is executing and the number of threads must augment.
  threadpool->threads = (pthread_t*) REALLOC(threadpool->threads, numThreads * sizeof(pthread_t));
  if(threadpool->threads != NULL) {
    // Create new threads
    bool failed = false;
    for(unsigned int i = threadpool->numThreads; i < numThreads; i++) {
      failed = pthread_create(&threadpool->threads[i], NULL, threadExecute, (void*) this) != 0;
      if(failed) break;
      else threadpool->numThreads++;
    }

    // Wake up all monitors
    pthread_cond_broadcast(&threadpool->monitorcond);

    if(failed) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_CREATIONFAILED), "setNumberOfThreads(): only %d/%d threads were started. (%s:%d)", threadpool->numThreads, numThreads, basename((char*)__FILE__), __LINE__);
    else {
      // Release lock
      pthread_mutex_unlock(&threadpool->globalmutex);
      return true;
    }
  } else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "setNumberOfThreads(): error reallocating thread buffer to create more threads. (%s:%d)", basename((char*)__FILE__), __LINE__);

  // Release lock
  pthread_mutex_unlock(&threadpool->globalmutex);
  return false;
}

// Adds a new job to the pool
bool gcgTHREADPOOL::assignJob(gcgJOB *job) {
  GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "assignJob(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(job == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_BADPARAMETERS), "assignJob(): parameter job is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Get main lock
  pthread_mutex_lock(&threadpool->globalmutex);

  // Extra check after lock, in case of object has been destroyed while waiting
  if(this->handle == NULL) return false;

  if(threadpool->run && threadpool->jobQueue != NULL) {
    // Add the new job
    if(threadpool->jobQueue->insertLast(job)) {
      // Wakes up ONE idle thread
      pthread_cond_signal(&threadpool->waitjobcond);

      // Release lock
      pthread_mutex_unlock(&threadpool->globalmutex);
      return true; // No error
    }

    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_JOBNOTEXECUTED), "assignJob(): error inserting a new job into the pool. (%s:%d)", basename((char*)__FILE__), __LINE__);
  } else
    // Check transient state
    if(threadpool->threads != NULL || threadpool->jobQueue != NULL) gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_THREAD, GCG_NOTRUNNING), "assignJob(): thread pool is being destroyed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    else gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_THREAD, GCG_NOTRUNNING), "assignJob(): thread pool is not running. (%s:%d)", basename((char*)__FILE__), __LINE__);

  // Release lock
  pthread_mutex_unlock(&threadpool->globalmutex);
  return false;
}


// Waits all jobs to be fully executed.
bool gcgTHREADPOOL::wait() {
  GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;
  bool result = true;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "wait(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Get main lock
  pthread_mutex_lock(&threadpool->globalmutex);
    if(threadpool->run) {
      // Is this thread in the threadpool?
      pthread_t cur = pthread_self();
      for(unsigned int i = 0; i < threadpool->numThreads; i++)
        if(pthread_equal(cur, threadpool->threads[i])) {
          // A JOB is trying to wait the pool to be idle!
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_UNSUPPORTEDOPERATION), "wait(): a job of a gcgTHREADPOOL is not allowed to call its wait() to avoid deadlock. (%s:%d)", basename((char*)__FILE__), __LINE__);

          // Release lock
          pthread_mutex_unlock(&threadpool->globalmutex);
          return false;
        }


      while(threadpool->run && threadpool->jobQueue != NULL) {
        if(threadpool->busy == 0 && threadpool->jobQueue->counter == 0) break; // Here we know threadpool->jobQueue is notNULL
        pthread_cond_wait(&threadpool->monitorcond, &threadpool->globalmutex);
      }

      if(threadpool->jobQueue == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_THREAD, GCG_INTERRUPTED), "wait(): interrupted while waiting. %d threads running and %d jobs pending. (%s:%d)", threadpool->busy, threadpool->jobQueue->counter, basename((char*)__FILE__), __LINE__);
        result = false;
      } else  if(threadpool->busy > 0 || threadpool->jobQueue->counter > 0) {
                gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_THREAD, GCG_INTERRUPTED), "wait(): interrupted while waiting. %d threads running and %d jobs pending. (%s:%d)", threadpool->busy, threadpool->jobQueue->counter, basename((char*)__FILE__), __LINE__);
                result = false;
              }
    } else {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_THREAD, GCG_NOTRUNNING), "wait(): thread pool is not running. (%s:%d)", basename((char*)__FILE__), __LINE__);
        result = false;
      }
  // Release lock
  pthread_mutex_unlock(&threadpool->globalmutex);
  return result;
}

// Waits all threads to exit.
bool gcgTHREADPOOL::waitAndDestroy() {
  GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "waitAndDestroy(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Get main lock
  pthread_mutex_lock(&threadpool->globalmutex);
    // Is it running?
    if(threadpool->run && threadpool->jobQueue != NULL && threadpool->threads != NULL) {
      // Is this thread in the threadpool?
      pthread_t cur = pthread_self();
      for(unsigned int i = 0; i < threadpool->numThreads; i++)
        if(pthread_equal(cur, threadpool->threads[i])) {
          // A JOB is trying to destroy the pool
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_UNSUPPORTEDOPERATION), "waitAndDestroy(): a job of a gcgTHREADPOOL is not allowed to call its waitAndDestroy() to avoid deadlock. (%s:%d)", basename((char*)__FILE__), __LINE__);

          // Release lock
          pthread_mutex_unlock(&threadpool->globalmutex);
          return false;
        }

      // Signals threads to stop
      threadpool->run = false;

      // Discard all pending jobs
      while(threadpool->jobQueue->counter > 0) {
        gcgJOB *job = (gcgJOB*) threadpool->jobQueue->dequeueFirst();
        if(threadpool->discarded == NULL) SAFE_DELETE(job)
        else threadpool->discarded->enqueueTail(job);
      }

      // Wake up all monitors
      pthread_cond_broadcast(&threadpool->monitorcond);

      // Wake up all sleeping threads
      while(threadpool->numThreads > 0) {
        pthread_cond_broadcast(&threadpool->waitjobcond); // Inside the loop in case of a new thread is created and goes to sleep before signal to stop
        pthread_cond_wait(&threadpool->monitorcond, &threadpool->globalmutex);
      }

      // Free thread buffer and queue
      SAFE_FREE(threadpool->threads);
      SAFE_DELETE(threadpool->jobQueue);
      threadpool->numThreads = 0;
      threadpool->busy = 0;
      threadpool->stopThreads = 0;
      // Preserse this: threadpool->executed = NULL;
      // Preserse this: threadpool->discarded = NULL;
    } //else gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_THREAD, GCG_NOTRUNNING), "waitAndDestroy(): thread pool is not running. (%s:%d)", basename((char*)__FILE__), __LINE__);

  // Wake up all monitors
  pthread_cond_broadcast(&threadpool->monitorcond);

  // Release lock
  pthread_mutex_unlock(&threadpool->globalmutex);
  return true;
}

// Forces the cancellation of all threads. Frees all object resources.
bool gcgTHREADPOOL::destroyPool() {
  GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;
  bool result = true;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "destroyPool(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Get main lock
  pthread_mutex_lock(&threadpool->globalmutex);
    // Is it running?
    if(threadpool->run && threadpool->jobQueue != NULL && threadpool->threads != NULL) {
      // Is this thread in the threadpool?
      pthread_t cur = pthread_self();
      for(unsigned int i = 0; i < threadpool->numThreads; i++)
        if(pthread_equal(cur, threadpool->threads[i])) {
          // A JOB is trying to destroy the threadpool
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_UNSUPPORTEDOPERATION), "destroyPool(): a job of a gcgTHREADPOOL is not allowed to call its destroyPool() to avoid deadlock. (%s:%d)", basename((char*)__FILE__), __LINE__);

          // Release lock
          pthread_mutex_unlock(&threadpool->globalmutex);
          return false;
        }

      // Signals stop execution
      threadpool->run = false;

      // Free job queue
      while(threadpool->jobQueue->counter > 0) {
        gcgJOB *job = (gcgJOB*) threadpool->jobQueue->dequeueFirst();
        if(threadpool->discarded == NULL) SAFE_DELETE(job)
        else threadpool->discarded->enqueueTail(job);
      }

      // Stop threads
      if(threadpool->numThreads > 0) {
        // Copy array of threads... the threads will remove their descriptors
        unsigned int savednum = threadpool->numThreads;
        pthread_t *saved = (pthread_t*) ALLOC(threadpool->numThreads * sizeof(pthread_t));
        if(saved == NULL) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "destroyPool(): error trying to save thread descriptors. (%s:%d)", basename((char*)__FILE__), __LINE__);
        else  {
            // Copy array of threads... the threads will remove their descriptors
            memcpy(saved, threadpool->threads, threadpool->numThreads * sizeof(pthread_t));

            // Leave still running threads to stop...
            pthread_cond_broadcast(&threadpool->monitorcond);
            pthread_cond_broadcast(&threadpool->waitjobcond);

            // Release lock
            pthread_mutex_unlock(&threadpool->globalmutex);

            // Cancel the threads asynchronously: it does not work on Windows
            for(unsigned int i = 0; i < savednum; i++) {
              int a = pthread_cancel(saved[i]);
              if(a != 0 && a != ESRCH) { // ESRCH: no such process
                gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "destroyPool(): failed to cancel thread. (%s:%d)", basename((char*)__FILE__), __LINE__);
                result = false;
              }
            }

            // Waits all threads to stop, canceled or not
            for(unsigned int i = 0; i < savednum; i++) {
              void *ret;
              int a = pthread_join(threadpool->threads[i], &ret);
              if(a != 0 && a != ESRCH) { // ESRCH: no such process
                gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "destroyPool(): failed waiting thread %d to stop. (%s:%d)", i, basename((char*)__FILE__), __LINE__);
                result = false;
              }
            }

            SAFE_FREE(saved);

            // Get main lock
            pthread_mutex_lock(&threadpool->globalmutex);
          }
      }

       // Free thread buffer and queue
      SAFE_FREE(threadpool->threads);
      SAFE_DELETE(threadpool->jobQueue);
      threadpool->numThreads = 0;
      threadpool->busy = 0;
      threadpool->stopThreads = 0;
      // Preserse this: threadpool->executed = NULL;
      // Preserse this: threadpool->discarded = NULL;
    } //else gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_THREAD, GCG_NOTRUNNING), "destroyPool(): thread pool is not running. (%s:%d)", basename((char*)__FILE__), __LINE__);

  // Release lock
  pthread_mutex_unlock(&threadpool->globalmutex);
  return result;
}

bool gcgTHREADPOOL::setOutputQueue(gcgQUEUE *executed, gcgQUEUE *discarded) {
  register GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setOutputQueue(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock pool
  pthread_mutex_lock(&threadpool->globalmutex);
    if(executed) threadpool->executed = executed;
    if(discarded) threadpool->discarded = discarded;
  // Unlock pool
  pthread_mutex_unlock(&threadpool->globalmutex);
  return true;
}

bool gcgTHREADPOOL::getOutputQueue(gcgQUEUE **pexecuted, gcgQUEUE **pdiscarded) {
  register GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getOutputQueue(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock pool
  pthread_mutex_lock(&threadpool->globalmutex);
      if(pexecuted) *pexecuted = threadpool->executed;
      if(pdiscarded) *pdiscarded = threadpool->discarded;
  // Unlock pool
  pthread_mutex_unlock(&threadpool->globalmutex);
  return true;
}


// Returns true if the thread pool has threads alive
bool gcgTHREADPOOL::isRunning() {
  register GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "isRunning(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock pool
  pthread_mutex_lock(&threadpool->globalmutex);
    bool res = threadpool->run;
  // Unlock pool
  pthread_mutex_unlock(&threadpool->globalmutex);

  return res;
}


// Returns the number of jobs in the queue
uintptr_t gcgTHREADPOOL::getNumberOfPendingJobs() {
  register GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getPendingJobs(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock pool
  uintptr_t res = 0;
  pthread_mutex_lock(&threadpool->globalmutex);
    // Is it running?
    if(threadpool->jobQueue != NULL)
      res = threadpool->jobQueue->counter;

  // Unlock pool
  pthread_mutex_unlock(&threadpool->globalmutex);

  return res;
}


// Returns the current number of threads working
unsigned int gcgTHREADPOOL::getNumberOfThreads() {
  register GCG_THREADPOOL *threadpool = (GCG_THREADPOOL*) this->handle;

  if(threadpool == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getNumberOfThreads(): invalid object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Lock pool
  unsigned int res = 0;
  pthread_mutex_lock(&threadpool->globalmutex);
    // Is it running?
    if(threadpool->run && threadpool->jobQueue != NULL && threadpool->threads != NULL)
      res = threadpool->numThreads;

  // Unlock pool
  pthread_mutex_unlock(&threadpool->globalmutex);

  return res;
}

