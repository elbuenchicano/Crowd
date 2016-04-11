#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#ifdef WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

#include "gcg.h"

#define MILISECONDS_WAIT 200

// It is a global variable that flags the program to stop execution: a good practice when developing multithreaded applications
volatile bool interrupted = false;

//-------------------------------//
int counter = 0; // Job counter
gcgRANDOM t;     // Random generator for job last

// Queue for thread communication: creates a pool using current number of processors
gcgTHREADPOOL pool;


// Producer thread
class JOB : public gcgJOB {
  public:
    int uniqueid;

  public:
    JOB() : uniqueid(++counter) {}

    // producer: puts its products into the container
    void run() {
      printf("\nJob %d running...", this->uniqueid);

      // Check interruption before doing work...
      if(interrupted) return;

      // Waits some random time
    #ifdef WIN32
      Sleep(t.intRandom(0, MILISECONDS_WAIT)); // Windows version
    #else
      usleep(1000 * t.intRandom(0, MILISECONDS_WAIT)); // Linux version
    #endif
      printf("\nJob %d finished.", this->uniqueid);
    }
};

//Application cleanup code
void systemexit() {
  interrupted = true;

  printf("\nStoping thread pool...");

  // Wait threads to finish and discard remaining jobs
  pool.waitAndDestroy(); // make sure the pool is stopped BEFORE destruction begins
  printf("\nFinished\n\n");
}


// Process signals
void processsignal(int s){
  // Signals may interrupt critical sections within threadṕool!! Just flags whole machinery to exit smoothly.
  switch(s) {
#ifndef WIN32
    case SIGTSTP:
    case SIGHUP:
#endif
    case SIGTERM:
    case SIGINT:  interrupted = true;
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
  gcgEnableReportLog(GCG_INFORMATION, true);

  // Get number of processors
  int processors = gcgGetNumberOfProcessors();

  if(pool.setNumberOfThreads(processors)) {
    printf("\n  - Thread pool up");

    // Check interruption before doing work
    while(!interrupted) {
      // Here we can assign jobs to the thread pool
      for(int i = 0; i < processors; i++) {
        JOB *job = new JOB();
        if(!pool.assignJob(job)) delete job; // Could not execute job
      }

    printf("\n\nNumber of threads: %d      Pending jobs: %d", pool.getNumberOfThreads(), pool.getNumberOfPendingJobs());

      // Wait to obtain equilibrium between execution and creation
  #ifdef WIN32
      Sleep(MILISECONDS_WAIT / processors);  // Windows version
  #else
      usleep(MILISECONDS_WAIT * 1000 / processors);  // Linux version
  #endif
    }
  } else {
    printf("\n  - Thread pool failed");
    exit(EXIT_FAILURE);
  }

  return 0;
}
