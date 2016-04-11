#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#ifdef WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

#include "gcg.h"

#define MILISECONDS_WAIT 50

//-------------------------------//

// Simple number generator
gcgRANDOM numberproducer;

// Queue for thread comunication
gcgPRODUCERCONSUMER container(4);

// Producer thread
class PRODUCER : public gcgTHREAD {
  public:
    // It is a variable that flags the program to stop execution: a good practice when developing multithreaded applications
    volatile bool runproducer;

  public:
    PRODUCER() : runproducer(true) {}

    // producer: puts its products into the container
    void run() {
      // Produces random numbers between 1 and 100
      while( runproducer ) {
        int product = numberproducer.intRandom(1, 100);

        if(container.put((void*) (long) product))
             printf("...produced = %5d        container = %d\n", product, container.getCounter());
        else printf(".....producer not blocked and buffer is full: product %d was lost\n", product);
    #ifdef WIN32
        Sleep(numberproducer.intRandom(MILISECONDS_WAIT, MILISECONDS_WAIT*5)); // Windows version
    #else
        usleep(1000 * numberproducer.intRandom(MILISECONDS_WAIT, MILISECONDS_WAIT*5)); // Linux version
    #endif
      }
    }
};


// Consumer thread
class CONSUMER : public gcgTHREAD {
  public:
    // It is a variable that flags the program to stop execution: a good practice when developing multithreaded applications
    volatile bool runconsumer;

  public:
    CONSUMER() : runconsumer(true) {}

    // consumer: gets the produced numbers from the container
    void run() {
      while( runconsumer ) {
        int product = (int) (long) container.get();

        if(product != 0) // NULL
          printf("...consumed = %5d        container = %d\n", product, container.getCounter());
        else printf(".....consumer not blocked and buffer is empty: no product to consume\n");

    #ifdef WIN32
        Sleep(numberproducer.intRandom(MILISECONDS_WAIT, MILISECONDS_WAIT*5));  // Windows version
    #else
        usleep(1000 * numberproducer.intRandom(MILISECONDS_WAIT, MILISECONDS_WAIT*5));  // Linux version
    #endif
      }
    }
};

// Instantiate threads
PRODUCER producerthread;
CONSUMER consumerthread;


//Application cleanup code
void systemexit() {
  printf("\nStoping pipeline...");

  // Release all waiting threads
  container.setTimeout(0, 0);

  // Stop producer
  producerthread.runproducer = false;
  producerthread.waitThread(); // wait for producer thread
  printf("\n  - producer down");

  // Flush elements in the queue
  while(container.getCounter() > 0) {
    int product = (int) (long) container.get();
    printf("\n       finishing: %d discarded", product);
  }

  // Stop consumer
  consumerthread.runconsumer = false;
  consumerthread.waitThread(); // wait for producer thread
  printf("\n  - consumer down");

  printf("\nFinished\n\n");
}

// Process signals
void processsignal(int s){
  // Signals may interrupt critical sections within gcgTHREAD or gcgPRODUCERCONSUMER!! Just flags whole machinery to exit smoothly.
   switch(s) {
#ifndef WIN32
    case SIGTSTP:
    case SIGHUP:
#endif
    case SIGTERM:
    case SIGINT:  producerthread.runproducer = false;
                  consumerthread.runconsumer = false;
                  break;
  }
}


int main() {
  // Cleanup callback
  atexit(systemexit);

  // Process signals
  signal(SIGINT, processsignal);
  signal(SIGTERM, processsignal);
#ifndef WIN32
  signal(SIGTSTP, processsignal);
  signal(SIGHUP, processsignal);
#endif

  // Enable the report of warnings by GCGLIB: useful while developing
  gcgEnableReportLog(GCG_WARNING, true);
  //gcgEnableReportLog(GCG_INFORMATION, true);

  container.setTimeout(0, 0);

  printf("\nStarting consumer thread...");
  if(consumerthread.startThread()) {
    printf("\n  - consumer up");

    // Brings the producer up
    printf("\nStarting producer thread...");
    if(producerthread.startThread()) {
      printf("\n  - producer up");

      // Here we can orchestrate the production/consumation: there is a slightly chance to the mode be changed
      gcgRANDOM rnumber;
      while(producerthread.runproducer && consumerthread.runconsumer) { // While not interrupted...
        int number = rnumber.intRandom(1, 10000);
        if(number <= 25) {
          printf("\n\nCURRENT MODE: producer blocking = FALSE     consumer blocking = FALSE\n");
          container.setTimeout(0, 0);
        } else if(number <= 50) {
                  printf("\n\nCURRENT MODE: producer blocking = TRUE      consumer blocking = FALSE\n");
                  container.setTimeout(-1, 0);
                } else if(number <= 75) {
                          printf("\n\nCURRENT MODE: producer blocking = FALSE      consumer blocking = TRUE\n");
                          container.setTimeout(0, -1);
                        } else if(number <= 100) {
                                  printf("\n\nCURRENT MODE: producer blocking = TRUE       consumer blocking = TRUE\n");
                                  container.setTimeout(-1, -1);
                                }
#ifdef WIN32
        Sleep(MILISECONDS_WAIT);  // Windows version
#else
        usleep(MILISECONDS_WAIT * 1000);  // Linux version
#endif

      }
    } else {
      printf("\n  - producer failed");
      exit(EXIT_FAILURE);
    }
  } else {
    printf("\n  - consumer failed");
    exit(EXIT_FAILURE);
  }

  return 0;
}
