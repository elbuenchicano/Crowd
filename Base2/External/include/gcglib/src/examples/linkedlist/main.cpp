#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "gcg.h"


///// NODES FOR THE DOUBLE LINKED LIST
class node : public gcgDOUBLELINK {  // You must use gcgDOUBLELINK for using with gcgDOUBLELINKEDLIST
  public:
    int key;

    node(int k = 0) : key(k) {}
};



//Application cleanup code
void systemexit() {
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
  gcgEnableReportLog(GCG_WARNING, true);
  //gcgEnableReportLog(GCG_INFORMATION, true);

  gcgRANDOM r;
  gcgDOUBLELINKEDLIST  l;  // Comment to test with gcgLINKEDLIST
  //gcgLINKEDLIST  l;   // It also works because gcgDOUBLELINK inherits from gcgLINK. It is much slower than gcgDOUBLELINKEDLIST in several methods.

  for(int i = 0; i < 100; i++) {
    printf("\n\n************* Iteration %d **********************\n\n", i);
    //getchar();

    node *p = new node(r.intRandom(0,10000));
    printf("\n--------------------------------------\n");
    printf("\nInserting first = %d\n", p->key);
    l.insertFirst(p);
    p = new node(r.intRandom(0,10000));
    printf("\nInserting last = %d\n", p->key);
    l.insertLast(p);

    printf("\n----> Size = %d\n", l.counter);
    gcgITERATOR *it = l.getIterator(1);
    if(it) {
      for(node *p = (node*) it->next(); p != NULL; p = (node*) it->next()) {
        printf("%d ", p->key);
        p = (node*) p->right;
      }
      delete it;
    }

    printf("\n--------------------------------------\n");
    int j = r.intRandom(0, l.counter-1);
    node *a = new node(r.intRandom(0,10000));
    printf("\nInsert %d before = %d position", a->key, j);
    p = (node*) l.first;
    while(p != NULL) {
      j--;
      if(j < 0) break;
      p = (node*) p->next;
    }
    l.insertBefore(a, p);

    printf("\n----> Size = %d\n", l.counter);
    a = (node*) l.first;
    while(a != NULL) {
      printf("%d ", a->key);
      a = (node*) a->next;
    }

    printf("\n--------------------------------------\n");
    j = r.intRandom(0, l.counter-1);
    a = new node(r.intRandom(0,10000));
    printf("\nInsert %d after = %d position", a->key, j);
    p = (node*) l.first;
    while(p != NULL) {
      j--;
      if(j < 0) break;
      p = (node*) p->next;
    }

    l.insertAfter(a, p);

    printf("\n----> Size = %d\n", l.counter);
    a = (node*) l.first;
    while(a != NULL) {
      printf("%d ", a->key);
      a = (node*) a->next;
    }


    printf("\n--------------------------------------\n");
    j = r.intRandom(0, l.counter-1);
    printf("\nmoving to last = %d position", j);
    p = (node*) l.first;
    while(p != NULL) {
      j--;
      if(j < 0) break;
      p = (node*) p->next;
    }

    l.moveToLast(p);

    printf("\n----> Size = %d\n", l.counter);
    p = (node*) l.first;
    while(p != NULL) {
      printf("%d ", p->key);
      p = (node*) p->next;
    }


    printf("\n--------------------------------------\n");
    j = r.intRandom(0, l.counter-1);
    printf("\nmoving to first = %d position", j);
    p = (node*) l.first;
    while(p != NULL) {
      j--;
      if(j < 0) break;
      p = (node*) p->next;
    }

    l.moveToFirst(p);

    printf("\n----> Size = %d\n", l.counter);
    p = (node*) l.first;
    while(p != NULL) {
      printf("%d ", p->key);
      p = (node*) p->next;
    }

    printf("\n--------------------------------------\n");
    j = r.intRandom(0, l.counter-1);
    int k = r.intRandom(0, l.counter-1);;
    printf("\nswitching = %d and %d positions", j, k);
    p = (node*) l.first;
    while(p != NULL) {
      j--;
      if(j < 0) break;
      p = (node*) p->next;
    }
    a = (node*) l.first;
    while(a != NULL) {
      k--;
      if(k < 0) break;
      a = (node*) a->next;
    }

    l.switchNodes(a,p);

    printf("\n----> Size = %d\n", l.counter);
    p = (node*) l.first;
    while(p != NULL) {
      printf("%d ", p->key);
      p = (node*) p->next;
    }


    printf("\n--------------------------------------\n");
    printf("\nremoving > 5000");
    p = (node*) l.first;
    while(p != NULL) {
      if(p->key > 5000) {
        node *n = (node*) p->next;
        l.remove(p);
        delete p;
        p = n;
      } else p = (node*) p->next;
    }

    printf("\n----> Size = %d\n", l.counter);
    p = (node*) l.first;
    while(p != NULL) {
      printf("%d ", p->key);
      p = (node*) p->next;
    }


    printf("\n--------------------------------------\n");
    printf("\nDequeue first");
    p = (node*) l.dequeueFirst();
    if(p) delete p;

    printf("\n----> Size = %d\n", l.counter);
    p = (node*) l.first;
    while(p != NULL) {
      printf("%d ", p->key);
      p = (node*) p->next;
    }

    printf("\n--------------------------------------\n");
    printf("\nDequeue last");
    p = (node*) l.dequeueLast();
    if(p) delete p;

    printf("\n----> Size = %d\n", l.counter);
    p = (node*) l.first;
    while(p != NULL) {
      printf("%d ", p->key);
      p = (node*) p->next;
    }
  }

  // Free all nodes
  l.deleteAll();
  return 0;
}
