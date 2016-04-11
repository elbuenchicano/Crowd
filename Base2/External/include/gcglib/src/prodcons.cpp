/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    PRODCONS.CPP: controls multiple producers and multiple consumers with or without
                  blocking calls to put() and get().

    Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"

// Internal struct: needed to free the gcg.h from depending on pthread.
typedef struct _GCG_PRODCONS {
  unsigned int counter;  // Object counter
  unsigned int prod_pos; // posicao que o produtor vai inserir no buffer
  unsigned int cons_pos; // posicao que o consumidor vai retirar do buffer
  pthread_mutex_t mutex_counter;
  pthread_mutex_t mutex_cond_prod;
  pthread_mutex_t mutex_cond_cons;
  pthread_cond_t prod_cond;
  pthread_cond_t cons_cond;
  bool enabled; // flag for execution
  long timeoutprod;
  long timeoutcons;
  unsigned int buf_size;
  void **buffer;
} GCG_PRODCONS;


gcgPRODUCERCONSUMER::gcgPRODUCERCONSUMER(unsigned int bufferlength) {
  if(bufferlength != 0) setBufferSize(bufferlength);
  else this->handle = NULL;
}

gcgPRODUCERCONSUMER::~gcgPRODUCERCONSUMER() {
  if(!destroyProducerConsumer())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_RELEASEERROR), "~gcgPRODUCERCONSUMER(): could not destroy the producer/consumer by calling destroyProducerConsumer(). (%s:%d)", basename((char*)__FILE__), __LINE__);
}

bool gcgPRODUCERCONSUMER::destroyProducerConsumer() {
  register GCG_PRODCONS *prodcons = (GCG_PRODCONS*) this->handle;
  bool status = true;

  if(prodcons != NULL) {
    this->enable(false);
    SAFE_FREE(prodcons->buffer);

    if(pthread_mutex_destroy(&prodcons->mutex_counter) != 0) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "destroyProducerConsumer(): failed to destroy counter mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
      status = false;
    }

    if(pthread_mutex_destroy(&prodcons->mutex_cond_prod) != 0) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "destroyProducerConsumer(): failed to destroy producer mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
      status = false;
    }

    if(pthread_mutex_destroy(&prodcons->mutex_cond_cons) != 0) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "destroyProducerConsumer(): failed to destroy consumer mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
      status = false;
    }

    if(pthread_cond_destroy(&prodcons->prod_cond) != 0) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "destroyProducerConsumer(): failed to destroy producer condition. (%s:%d)", basename((char*)__FILE__), __LINE__);
      status = false;
    }

    if(pthread_cond_destroy(&prodcons->cons_cond) != 0) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "destroyProducerConsumer(): failed to destroy consumer condition. (%s:%d)", basename((char*)__FILE__), __LINE__);
      status = false;
    }

    SAFE_FREE(this->handle);
  }

  return status;
}


bool gcgPRODUCERCONSUMER::setBufferSize(unsigned int bufferlength) {
  register GCG_PRODCONS *prodcons = (GCG_PRODCONS*) this->handle;

  // Check parameter
  destroyProducerConsumer();
  if(bufferlength == 0) return true; // Just free the object

  // Try to allocate object space.
  // We need this to free the gcg.h from using pthread
  prodcons = (GCG_PRODCONS*) ALLOC(sizeof(GCG_PRODCONS));
  if((this->handle = prodcons) == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_ALLOCATIONERROR), "setBufferSize(): could not allocate object. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Ok, we can construct the buffer
  prodcons->buffer = (void**) ALLOC(bufferlength * sizeof(void*)); // Avoiding calloc
  if(prodcons->buffer == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_ALLOCATIONERROR), "setBufferSize(): could not allocate buffer. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Revert initializations
    SAFE_FREE(prodcons);
    return false;
  }

  // Init buffer with zeros
  memset(prodcons->buffer, 0, bufferlength * sizeof(void*));

  if(pthread_mutex_init(&prodcons->mutex_counter, NULL) != 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "setBufferSize(): could not initialize the counter mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Revert initializations
    SAFE_FREE(prodcons->buffer);
    SAFE_FREE(prodcons);
    return false;
  }
  if(pthread_mutex_init(&prodcons->mutex_cond_prod, NULL) != 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "setBufferSize(): could not initialize the producer mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Revert initializations
    if(pthread_mutex_destroy(&prodcons->mutex_counter) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy counter mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    SAFE_FREE(prodcons->buffer);
    SAFE_FREE(prodcons);
    return false;
  }

  if(pthread_mutex_init(&prodcons->mutex_cond_cons, NULL) != 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "setBufferSize(): could not initialize the consumer mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Revert initializations
    if(pthread_mutex_destroy(&prodcons->mutex_counter) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy counter mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    if(pthread_mutex_destroy(&prodcons->mutex_cond_prod) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy producer mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    SAFE_FREE(prodcons->buffer);
    SAFE_FREE(prodcons);
    return false;
  }

  if(pthread_cond_init(&prodcons->prod_cond, NULL) != 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "setBufferSize(): could not initialize the producer condition. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Revert initializations
    if(pthread_mutex_destroy(&prodcons->mutex_counter) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy counter mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    if(pthread_mutex_destroy(&prodcons->mutex_cond_prod) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy producer mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    if(pthread_mutex_destroy(&prodcons->mutex_cond_cons) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy consumer mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    SAFE_FREE(prodcons->buffer);
    SAFE_FREE(prodcons);
    return false;
  }

  if(pthread_cond_init(&prodcons->cons_cond, NULL) != 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_INITERROR), "setBufferSize(): could not initialize the consumer condition. (%s:%d)", basename((char*)__FILE__), __LINE__);

    // Revert initializations
    if(pthread_mutex_destroy(&prodcons->mutex_counter) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy counter mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    if(pthread_mutex_destroy(&prodcons->mutex_cond_prod) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy producer mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    if(pthread_mutex_destroy(&prodcons->mutex_cond_cons) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy consumer mutex. (%s:%d)", basename((char*)__FILE__), __LINE__);
    if(pthread_cond_destroy(&prodcons->prod_cond) != 0)
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_STOPERROR), "setBufferSize(): failed to destroy producer condition. (%s:%d)", basename((char*)__FILE__), __LINE__);
    SAFE_FREE(prodcons->buffer);
    SAFE_FREE(prodcons);
    return false;
  }

  prodcons->buf_size = bufferlength;
  prodcons->counter = 0;  // The buffer starts with all positions free
  prodcons->prod_pos = 0;
  prodcons->cons_pos = 0;
  prodcons->enabled  = true;
  prodcons->timeoutprod = 0;
  prodcons->timeoutcons = 0;
  return true;
}

bool gcgPRODUCERCONSUMER::put(void *val) {
  register GCG_PRODCONS *prodcons = (GCG_PRODCONS*) this->handle;
	bool status = false;

  if(prodcons == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "put(): attempt to use an invalid gcgPRODUCERCONSUMER handle. Pointer was not stored. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

	// Wait for a free position
	pthread_mutex_lock(&prodcons->mutex_cond_prod);
	  int res = 0;
    if(prodcons->counter == prodcons->buf_size && prodcons->enabled && prodcons->timeoutprod >= 0) {
        // Waits a consumation during the time specified by the user
        struct timespec timeout;
        gcgComputeTimeout(&timeout, prodcons->timeoutprod);
        res = pthread_cond_timedwait(&prodcons->prod_cond, &prodcons->mutex_cond_prod, &timeout);
    } else while(prodcons->counter == prodcons->buf_size && prodcons->enabled && prodcons->timeoutprod < 0)
              res = pthread_cond_wait(&prodcons->prod_cond, &prodcons->mutex_cond_prod); // Blocks until a consumation is done

    if(res != 0) {
      if(res == ETIMEDOUT) gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_THREAD, GCG_TIMEOUT), "put(): timeout elapsed waiting for a consumation and the pointer was not stored. (%s:%d)", basename((char*)__FILE__), __LINE__);
      else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_LOCKERROR), "put(): error waiting for a consumation. Pointer was not stored. (%s:%d)", basename((char*)__FILE__), __LINE__);

      pthread_mutex_unlock(&prodcons->mutex_cond_prod);
      return false;
    }

    if(prodcons->enabled) {
      // Producers and consumers are racing for this counter
      pthread_mutex_lock(&prodcons->mutex_counter);
        if(prodcons->counter < prodcons->buf_size) { // non-blocking condition
          prodcons->buffer[prodcons->prod_pos] = val;
          prodcons->prod_pos = (prodcons->prod_pos + 1) % prodcons->buf_size;
          status = true;
          prodcons->counter++;
        } else gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_THREAD, GCG_NOBUFFER), "put(): buffer full. Pointer was not stored. (%s:%d)", basename((char*)__FILE__), __LINE__);
      pthread_mutex_unlock(&prodcons->mutex_counter);
    } else gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_THREAD, GCG_STOREFAILURE), "put(): disabled producer/consumer. Pointer was not stored. (%s:%d)", basename((char*)__FILE__), __LINE__);

  pthread_mutex_unlock(&prodcons->mutex_cond_prod);

  // Let the consumers get another position
  if(prodcons->enabled && status) pthread_cond_signal(&prodcons->cons_cond);

	return status;
}

void *gcgPRODUCERCONSUMER::get() {
  register GCG_PRODCONS *prodcons = (GCG_PRODCONS*) this->handle;
	void *val = NULL;

  if(prodcons == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "get(): attempt to use an invalid gcgPRODUCERCONSUMER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Wait for someting to be produced
  pthread_mutex_lock(&prodcons->mutex_cond_cons);

  int res = 0;
  if(prodcons->timeoutcons >= 0 && prodcons->counter == 0 && prodcons->enabled) {
    // Waits a production during the time specified by the user
    struct timespec timeout;
    gcgComputeTimeout(&timeout, prodcons->timeoutcons);
    res = pthread_cond_timedwait(&prodcons->cons_cond, &prodcons->mutex_cond_cons, &timeout);
  } else while(prodcons->counter == 0 && prodcons->enabled && prodcons->timeoutcons < 0)
            res = pthread_cond_wait(&prodcons->cons_cond, &prodcons->mutex_cond_cons);      // Waits until a production is done

  if(res != 0) {
    if(res == ETIMEDOUT) gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_THREAD, GCG_TIMEOUT), "get(): timeout elapsed waiting for a production. (%s:%d)", basename((char*)__FILE__), __LINE__);
    else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_THREAD, GCG_LOCKERROR), "get(): error waiting for a production = %d. (%s:%d)", res, basename((char*)__FILE__), __LINE__);

    pthread_mutex_unlock(&prodcons->mutex_cond_cons);
    return NULL;
  }

  if(prodcons->enabled) {
    // Producers and consumers are racing for this counter
    pthread_mutex_lock(&prodcons->mutex_counter);
    if(prodcons->counter > 0) { // Non-blocking test
       val = prodcons->buffer[prodcons->cons_pos];
       prodcons->buffer[prodcons->cons_pos] = NULL;
       prodcons->cons_pos = (prodcons->cons_pos+1) % prodcons->buf_size;
       prodcons->counter--;
     } else gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_THREAD, GCG_INFORMATIONPROBLEM), "get(): the buffer is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
     pthread_mutex_unlock(&prodcons->mutex_counter);
  } else gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_THREAD, GCG_STOREFAILURE), "get(): disabled producer/consumer. (%s:%d)", basename((char*)__FILE__), __LINE__);

  pthread_mutex_unlock(&prodcons->mutex_cond_cons);

  // Free one buffer's position
  if(prodcons->enabled && val != NULL) pthread_cond_signal(&prodcons->prod_cond);

  return val;
}

uintptr_t gcgPRODUCERCONSUMER::getCounter() {
  unsigned int count = 0;
  if(this->handle != NULL) {
    register GCG_PRODCONS *prodcons = (GCG_PRODCONS*) this->handle;
    // Avoid racing conditions
    pthread_mutex_lock(&prodcons->mutex_counter);
      count = prodcons->counter;
    pthread_mutex_unlock(&prodcons->mutex_counter);
  } else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getCounter(): attempt to use an invalid gcgPRODUCERCONSUMER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);

  return count;
}

void gcgPRODUCERCONSUMER::setTimeout(long timeoutprod, long timeoutcons) {
  register GCG_PRODCONS *prodcons = (GCG_PRODCONS*) this->handle;
  if(prodcons != NULL) {
    prodcons->timeoutcons = timeoutcons;
    prodcons->timeoutprod = timeoutprod;
    pthread_cond_broadcast(&prodcons->cons_cond);
    pthread_cond_broadcast(&prodcons->prod_cond);
  } else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setTimeout(): attempt to use an invalid gcgPRODUCERCONSUMER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

void gcgPRODUCERCONSUMER::enable(bool enabled) {
  register GCG_PRODCONS *prodcons = (GCG_PRODCONS*) this->handle;
  if(prodcons != NULL) {
    prodcons->enabled = enabled;
    pthread_cond_broadcast(&prodcons->cons_cond);
    pthread_cond_broadcast(&prodcons->prod_cond);
  } else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "enable(): attempt to use an invalid gcgPRODUCERCONSUMER handle. (%s:%d)", basename((char*)__FILE__), __LINE__);
}
