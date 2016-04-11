/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    CACHE.CPP: classes for memory caching

    Marcelo Bernardes Vieira

**************************************************************************************/

#include "system.h"


// Node for the priority list
class CACHENODE :  public gcgDOUBLELINK {
  public:
    void *key;
    void *data;
    unsigned long cost;
};


//////////////////////////////////////////////////////////////////////
// gcgCACHE: implements a cache with synchronization control
//////////////////////////////////////////////////////////////////////
// Internal structure: needed to free the gcg.h from depending on pthreads.
typedef struct _GCG_CACHE {
  pthread_mutex_t lockmutex;  // Cache mutex
  gcgDOUBLELINKEDLIST *keys;  // Keys list in a least recently used order.
} GCG_CACHE;

gcgCACHE::gcgCACHE(unsigned int keysizebytes, unsigned long cachecapacity) {
  this->handle = (GCG_CACHE*) ALLOC(sizeof(GCG_CACHE));
  if(handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgCACHE(): constructor failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  GCG_CACHE *cache = (GCG_CACHE*) this->handle;

  // Init priority list
  cache->keys = new gcgDOUBLELINKEDLIST();
  if(cache->keys == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgCACHE(): could not allocate the priority list. (%s:%d)", basename((char*)__FILE__), __LINE__);
    SAFE_FREE(this->handle);
    return;
  }

  // Init lock
  if(pthread_mutex_init(&cache->lockmutex, NULL) != 0) {
    SAFE_DELETE(cache->keys)
    SAFE_FREE(this->handle);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "gcgCACHE(): could not initialize the mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return;
  }

  pthread_mutex_lock(&cache->lockmutex);
    // Init cache
    this->totalcost = 0;
    this->capacity  = 0;
    this->keysize   = 0;
  pthread_mutex_unlock(&cache->lockmutex);

  setCapacity(cachecapacity);
  setKeySize(keysizebytes);
  // No error
}

bool gcgCACHE::setCapacity(unsigned long maximumtotalcost) {
  GCG_CACHE *cache = (GCG_CACHE*) this->handle;
  if(cache == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setCapacity(): invalid cache handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Release all?
  if(maximumtotalcost == 0) {
    this->flush(NULL);
    this->totalcost = 0;
  }

  pthread_mutex_lock(&cache->lockmutex);
    if(this->capacity != maximumtotalcost) {
      if(this->capacity < maximumtotalcost) this->capacity = maximumtotalcost;
      else {
        // Discard elements until the cost is compatible with the new maximum total cost
        while(this->totalcost > maximumtotalcost && cache->keys->counter > 0) {
          CACHENODE *node = (CACHENODE*) cache->keys->dequeueFirst();
          if(node != NULL) {
            // Sanity check
            if(totalcost < node->cost) {
              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_INVALIDOBJECT), "setCapacity(): cache total cost smaller than priority list total cost. (%s:%d)", basename((char*)__FILE__), __LINE__);
              this->totalcost = node->cost; // Avoid underflow problem in next block
            }
            this->discardData(node->key, node->data, node->cost);
            this->totalcost -= node->cost;
          } else break;
        }
        // Sanity check
        if(this->totalcost > 0 && cache->keys->counter == 0) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_INVALIDOBJECT), "setCapacity(): cache total cost greater than priority list total cost. (%s:%d)", basename((char*)__FILE__), __LINE__);
          this->totalcost = 0; // Avoid future problems
        }

        this->capacity = maximumtotalcost;
      }
    }
  pthread_mutex_unlock(&cache->lockmutex);
  return true;
}



bool gcgCACHE::setKeySize(unsigned int newkeysize) {
  GCG_CACHE *cache = (GCG_CACHE*) this->handle;
  if(cache == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setCapacity(): invalid cache handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  return true;
}


gcgCACHE::~gcgCACHE() {
  GCG_CACHE *cache = (GCG_CACHE*) handle;
  if(cache) {
    pthread_mutex_lock(&cache->lockmutex);
    if(!this->flush(NULL))
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_REMOVALERROR), "~gcgCACHE(): could not delete and remove all key/data pairs. (%s:%d)", basename((char*)__FILE__), __LINE__);

    if(!cache->keys->deleteAll())
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_REMOVALERROR), "~gcgCACHE(): could not delete priority list. (%s:%d)", basename((char*)__FILE__), __LINE__);

    SAFE_DELETE(cache->keys);

    // Free the mutex
    pthread_mutex_unlock(&cache->lockmutex);
    if(pthread_mutex_destroy(&cache->lockmutex) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "~gcgCACHE(): could not destroy the mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);

    SAFE_FREE(this->handle);
  }
}

void *gcgCACHE::get(void *key) {
  GCG_CACHE *cache = (GCG_CACHE*) handle;
  if(cache == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "get(): invalid cache handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }




}

bool gcgCACHE::flush(void *key) {
  GCG_CACHE *cache = (GCG_CACHE*) handle;
  if(cache == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "flush(): invalid cache handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(key != NULL) {
    // A single key was requested to be excluded

  } else {
      pthread_mutex_lock(&cache->lockmutex);
        // Discard all elements
        while(cache->keys->counter > 0) {
          CACHENODE *node = (CACHENODE*) cache->keys->dequeueFirst();
          if(node != NULL) {
            // Sanity check
            if(totalcost < node->cost) {
              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_INVALIDOBJECT), "flush(): cache total cost smaller than priority list total cost. (%s:%d)", basename((char*)__FILE__), __LINE__);
              this->totalcost = node->cost; // Avoid underflow problem in next block
            }
            this->discardData(node->key, node->data, node->cost);
            this->totalcost -= node->cost;
          } else break;
        }
        // Sanity check
        if(this->totalcost > 0 && cache->keys->counter == 0)
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_INVALIDOBJECT), "flush(): cache total cost greater than priority list total cost. (%s:%d)", basename((char*)__FILE__), __LINE__);

        this->totalcost = 0;
      pthread_mutex_unlock(&cache->lockmutex);
    }
  return true;
}
