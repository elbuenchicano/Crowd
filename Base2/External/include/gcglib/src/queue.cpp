/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    QUEUE.CPP: classes for managing queues

    Marcelo Bernardes Vieira

**************************************************************************************/

#include "system.h"

//////////////////////////////////////////////////////////////////////
// gcgQUEUE: implements a queue with synchronization control
//////////////////////////////////////////////////////////////////////
// Internal struct: needed to free the gcg.h from depending on pthread.
typedef struct _GCG_QUEUE {
  pthread_mutex_t lockmutex;    // Queue mutex
  pthread_cond_t  entrycond;    // To wait for entry events
  gcgLOCK         *notifyenq;   // Enqueuing notification
  gcgLOCK         *notifydeq;   // Dequeuing notification
  gcgLINK         *first;       // First node of the queue
  gcgLINK         *last;        // last node of the queue

  volatile bool            enabled;      // Flags if it is working
  volatile int             numnotifyenq; // Number of threads to be woken up when enqueuing occurs
  volatile int             numnotifydeq; // Number of threads to be woken up when dequeuing occurs
  volatile unsigned int    counter;      // Counter for the number of entries
} GCG_QUEUE;

gcgQUEUE::gcgQUEUE() {
  this->handle = (GCG_QUEUE*) ALLOC(sizeof(GCG_QUEUE));
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgQUEUE(): constructor failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;

  // Init lock
  if(pthread_mutex_init(&queue->lockmutex, NULL) != 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "gcgQUEUE(): could not initialize the mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    SAFE_FREE(this->handle);
    return;
  }

  pthread_mutex_lock(&queue->lockmutex);
    // Init condition
    if(pthread_cond_init(&queue->entrycond, NULL) != 0) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "gcgQUEUE(): could not initialize the condition object. (%s:%d)", basename((char*)__FILE__), __LINE__);
      pthread_mutex_unlock(&queue->lockmutex);

      // Free the mutex
      if(pthread_mutex_destroy(&queue->lockmutex) != 0)
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "gcgQUEUE(): could not destroy the mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);

      SAFE_FREE(this->handle);
      return;
    }

    // Init queue
    queue->enabled = true;
    queue->counter = 0;
    queue->first  = NULL;
    queue->last   = NULL;
    queue->notifyenq = NULL;
    queue->numnotifyenq = 0;
    queue->notifydeq = NULL;
    queue->numnotifydeq = 0;
  pthread_mutex_unlock(&queue->lockmutex);
  // No error
}

gcgQUEUE::~gcgQUEUE() {
  GCG_QUEUE *queue = (GCG_QUEUE*) handle;
  if(queue) {
    pthread_mutex_lock(&queue->lockmutex);
      queue->enabled = false;
    pthread_mutex_unlock(&queue->lockmutex);

    if(!this->deleteAll())
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_REMOVALERROR), "~gcgQUEUE(): could not delete and remove all entries. (%s:%d)", basename((char*)__FILE__), __LINE__);

    if(pthread_cond_destroy(&queue->entrycond) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_STOPERROR), "~gcgQUEUE(): failed to destroy queue condition. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Free the mutex
    if(pthread_mutex_destroy(&queue->lockmutex) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "~gcgQUEUE(): could not destroy the mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);

    SAFE_FREE(this->handle);
  }
}

bool gcgQUEUE::enqueueHead(gcgLINK *node) {
  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;
  if(!queue) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "enqueueHead(): invalid queue handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "enqueueHead(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Insert new node
  pthread_mutex_lock(&queue->lockmutex);
    node->next = queue->first;
    if(queue->first == NULL) queue->last = node;
    queue->first = node;
    queue->counter++;

    // Wakes up ONE thread waiting for an entry
    pthread_cond_signal(&queue->entrycond);

    // Notify threads
    if(queue->notifyenq) queue->notifyenq->wakeUp(queue->numnotifyenq);
  pthread_mutex_unlock(&queue->lockmutex);

  // No error
  return true;
}

bool gcgQUEUE::enqueueTail(gcgLINK *node) {
  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;
  if(!queue) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "enqueueTail(): invalid queue handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "enqueueTail(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Fill the entry
  node->next = NULL;

  // Insert new node
  pthread_mutex_lock(&queue->lockmutex);
    node->next = NULL;
    if(queue->last == NULL) queue->first = node;
    else queue->last->next = node;
    queue->last = node;
    queue->counter++;

    // Wakes up at least ONE thread waiting for an entry
    pthread_cond_signal(&queue->entrycond);

    // Notify threads
    if(queue->notifyenq) queue->notifyenq->wakeUp(queue->numnotifyenq);
  pthread_mutex_unlock(&queue->lockmutex);

  // No error
  return true;
}


gcgLINK *gcgQUEUE::dequeue() {
  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;
  if(!queue) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "dequeue(): invalid queue handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  gcgLINK *data = NULL;

  pthread_mutex_lock(&queue->lockmutex);
    if(queue->first != NULL) {
      // Remove entry
      data = queue->first;
      queue->first = queue->first->next;
      if(queue->first == NULL) queue->last = NULL;
      queue->counter--;

      // Notify threads
      if(queue->notifydeq) queue->notifydeq->wakeUp(queue->numnotifydeq);
    }
  pthread_mutex_unlock(&queue->lockmutex);

  if(data == NULL)
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "dequeue(): the queue is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);

  return data;
}

gcgLINK *gcgQUEUE::waitDequeue(long timeoutUsec) {
  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;
  if(!queue) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "waitDequeue(): invalid queue handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  gcgLINK *data = NULL;

  pthread_mutex_lock(&queue->lockmutex);
    if(queue->first == NULL) { // Is empty
      if(timeoutUsec >= 0) {
        struct timespec timeout;
        gcgComputeTimeout(&timeout, timeoutUsec);

        // Wait for the condition
        int res = pthread_cond_timedwait(&queue->entrycond, &queue->lockmutex, &timeout);

        if(queue->first == NULL)
          if(res != 0) {
            if(res == ETIMEDOUT) {
              gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_DATASTRUCTURE, GCG_TIMEOUT), "waitDequeue(): timeout elapsed waiting a new entry. (%s:%d)", basename((char*)__FILE__), __LINE__);
              pthread_mutex_unlock(&queue->lockmutex);
              return NULL;
            } else
                gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_LOCKERROR), "waitDequeue(): error occured waiting a new entry. (%s:%d)", basename((char*)__FILE__), __LINE__);
                pthread_mutex_unlock(&queue->lockmutex);
                return NULL;
              }
      } else
          while(queue->first == NULL && queue->enabled)
             pthread_cond_wait(&queue->entrycond, &queue->lockmutex);
    }

    if(!queue->enabled) {
      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "waitDequeue(): the queue was disabled. (%s:%d)", basename((char*)__FILE__), __LINE__);

      pthread_mutex_unlock(&queue->lockmutex);
      return NULL;
    }

    if(queue->first != NULL) {
      // Remove entry
      data = queue->first;
      queue->first = queue->first->next;
      if(queue->first == NULL) queue->last = NULL;
      queue->counter--;

      // Notify threads
      if(queue->notifydeq) queue->notifydeq->wakeUp(queue->numnotifydeq);
    }
  pthread_mutex_unlock(&queue->lockmutex);

  if(data == NULL)
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "waitDequeue(): the queue is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);

  return data;
}

bool gcgQUEUE::isEmpty() {
  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;
  if(!queue) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "dequeue(): invalid queue handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  pthread_mutex_lock(&queue->lockmutex);
    bool ok = queue->first == NULL;
  pthread_mutex_unlock(&queue->lockmutex);

  return ok;
}

uintptr_t gcgQUEUE::getCounter() {
  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;
  if(!queue) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getCounter(): invalid queue handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  pthread_mutex_lock(&queue->lockmutex);
    uintptr_t res = queue->counter;
  pthread_mutex_unlock(&queue->lockmutex);

  return res;
}

bool gcgQUEUE::deleteAll() {
  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;
  if(!queue) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "deleteAll(): invalid queue handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  pthread_mutex_lock(&queue->lockmutex);
    if(queue->first == NULL) {
      // Nothing to do
      pthread_mutex_unlock(&queue->lockmutex);
      return true;
    }

    // Free all elements
    while(queue->first != NULL) {
      gcgLINK *p = queue->first->next;
      SAFE_DELETE(queue->first);
      queue->first = p;
    }
    queue->last = NULL;

    // Wake up all waiting threads
    pthread_cond_broadcast(&queue->entrycond);
  pthread_mutex_unlock(&queue->lockmutex);

  // Notify all waiting threads
  if(queue->notifydeq) queue->notifydeq->wakeUp(-1);

  return true;
}


gcgLOCK *gcgQUEUE::wakeUpWhenEnqueued(gcgLOCK *notifyme, int wakeupNumThreads) {
  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;
  if(!queue) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "wakeUpWhenEnqueued(): invalid queue handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  pthread_mutex_lock(&queue->lockmutex);
    gcgLOCK *res = queue->notifyenq;
    queue->notifyenq = notifyme;
    queue->numnotifyenq = wakeupNumThreads;
  pthread_mutex_unlock(&queue->lockmutex);

  return res;
}

gcgLOCK *gcgQUEUE::wakeUpWhenDequeued(gcgLOCK *notifyme, int wakeupNumThreads) {
  GCG_QUEUE *queue = (GCG_QUEUE*) this->handle;
  if(!queue) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "wakeUpWhenDequeued(): invalid queue handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  pthread_mutex_lock(&queue->lockmutex);
    gcgLOCK *res = queue->notifydeq;
    queue->notifydeq = notifyme;
    queue->numnotifydeq = wakeupNumThreads;
  pthread_mutex_unlock(&queue->lockmutex);

  return res;
}
