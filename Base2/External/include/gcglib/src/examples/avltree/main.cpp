#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#ifdef _MSC_VER
  #pragma warning(disable: 4996) // Disable deprecation
#endif

#include "gcg.h"

///// NODES FOR THE THEE
class node : public gcgORDEREDNODE {
  public:
    int key;

    node(int k = 0) : key(k) {}
};


// TREE class
class tree : public gcgAVLTREE {
  int compare(gcgORDEREDNODE *n1, gcgORDEREDNODE *n2) {
    return (((node*) n1)->key == ((node*) n2)->key) ? 0 : ((((node*) n1)->key > ((node*) n2)->key) ? 1 : -1);
  }
};


// Tree object
tree   t;


//Application cleanup code
void systemexit() {
  // Release all nodes
  t.deleteAll();

  printf("\nFinished\n\n");
}

// Process signals
void processsignal(int s){
  switch(s) {
#ifndef WIN32
    case SIGTSTP:
    case SIGHUP:
#endif
    case SIGTERM:
    case SIGINT: exit(EXIT_FAILURE); // Make sure the systemexit() will be processed: single threaded
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
  //gcgEnableReportLog(GCG_WARNING, true);
  //gcgEnableReportLog(GCG_INFORMATION, true);

  gcgRANDOM      r;

  clock_t starts = clock();
  for(int i = 0; i < 1000000; i++) {
    node q;
    node *p = new node(r.intRandom(0,100000));
    if(p != t.insert(p)) delete p;

    for(int j = 0; j < 10; j++) {
      q.key = r.intRandom(0,100000);
      p = (node*) t.remove(&q);   // Might result in a warning
      if(p) delete p;
    }
  }

  for(int i = 0; i < 1000; i++) {
    node q;
    node *p;
    for(int j = 0; j < 1; j++) {
      q.key = r.intRandom(0,100000);
      p = (node*) t.remove(&q); // Might result in a warning
      if(p) delete p;
    }
  }

  clock_t ends = clock();

  // Show elements in descending order
  gcgITERATOR *it = t.detach(1);  // 1 means descending order. 0 is ascending order.
  node *p = (node*) it->next();
  while(p) {
    printf("%d\n", p->key);
    delete p;
    p = (node*) it->next();
  }
  delete it;  // You must delete the iterator

  printf("\nTime to fill tree = %f\n", ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  t.deleteAll(); // In fact, detach() has already cleaned the tree
  return 0;
}
