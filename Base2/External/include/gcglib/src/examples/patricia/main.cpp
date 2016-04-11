#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "gcg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define INTEGER_UPPER_LIMIT 3000000
#define RANDOM_ITERATIONS   1000000
#define MAX_STRING_SIZE     8

#define FIXED_KEY_SIZE      64 // 512 bits


// General random generator
gcgRANDOM gen;

///// NODES FOR THE STRING TREE
class DATASTR : public gcgDATA {
  public:
    char key[MAX_STRING_SIZE];

  // Generate a random string as key
  DATASTR() {
    int j;
    for(j = 0; j < gen.intRandom(1, MAX_STRING_SIZE - 1); j++) this->key[j] = (char) gen.intRandom('a', 'z');
    this->key[j] = 0;
  }
};

///// NODES FOR THE INTEGER TREE
class DATAINTEGER : public gcgDATA {
  public:
  int value; // NOTE: in this case the key storage in the object is only needed for later printing the contents. The key is copied into the tree in this case.
  DATAINTEGER() {value = (int) gen.intRandom(0, INTEGER_UPPER_LIMIT);}
};


// Dummy data for intptr_t keys: no key is stored in the data
class DATADUMMY : public gcgDATA {
  public:
  DATADUMMY() {}
};


///// NODES FOR THE FIXED SIZE KEY TREE
class DATAFIXEDSIZE : public gcgDATA {
  public:
    char longkey[FIXED_KEY_SIZE];

  // Generate a random long key
  DATAFIXEDSIZE() {
    int j;
    for(j = 0; j < FIXED_KEY_SIZE; j++) this->longkey[j] = (char) gen.intRandom(0, 255);
  }
};


// TREE instances
gcgPATRICIAMAP patriciaStr(0);
gcgPATRICIAMAP patriciaInt(sizeof(int));
gcgPATRICIAMAP patriciaMaxInt(sizeof(intptr_t));
gcgPATRICIAMAP patriciaFixed(FIXED_KEY_SIZE);

//Application cleanup code
void systemexit() {
  // Release all nodes
  patriciaStr.deleteAll();
  patriciaInt.deleteAll();
  patriciaMaxInt.deleteAll();
  patriciaFixed.deleteAll();
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

  gcgDATA *data;
  clock_t starts, ends;
  printf("Memory allocated at the start = %li\n", getAllocatedMemoryCounter());

  // String insertion
  gen.setSeed(100);
  starts = clock();
  for(int i = 0; i < RANDOM_ITERATIONS; i++) {
    // try to insert a string
    DATASTR *n = new DATASTR();
    if(patriciaStr.insert(n->key, n) != n) delete n; // Try to insert
  }
  ends = clock();
  printf("\nString: time to fill tree (%d distinct keys) = %.2fs\n", (int) patriciaStr.counter, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  // String searching
  starts = clock();
  int i, count = 0;
  for(i = 0; i < RANDOM_ITERATIONS; i++) {
    int j;
    char str[MAX_STRING_SIZE];
    for(j = 0; j < gen.intRandom(1, MAX_STRING_SIZE - 1); j++) str[j] = (char) gen.intRandom('a', 'z');
    str[j] = 0;
    if(patriciaStr.search(str) != NULL) count++;
  }
  ends = clock();
  printf("String: time to find %d/%d random keys = %.2fs\n", count, i, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  // String searching 2
  starts = clock();
  count = 0;
  gcgITERATOR *iterator = patriciaStr.getIterator(0);
  if(iterator) {
    for(DATASTR *data = (DATASTR*) iterator->next(); data != NULL; data = (DATASTR*) iterator->next())
      if(patriciaStr.search(data->key) != NULL) count++;
    delete iterator;
  }
  ends = clock();
  printf("String: time to find all %d string keys = %.2fs\n", count, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  // String prefix iterator
  printf("String: strings with prefix \"cca\"\n");
  iterator = patriciaStr.suffixIterator("cca");
  if(iterator) {
    for(DATASTR *data = (DATASTR*) iterator->next(); data != NULL; data = (DATASTR*) iterator->next()) printf("\"%s\"\n", data->key);
    delete iterator;
  }

  // String removal
  gen.setSeed(100);
  starts = clock();
  count = 0;
  for(i = 0; i < RANDOM_ITERATIONS; i++) {
    int j;
    char str[MAX_STRING_SIZE];
    for(j = 0; j < gen.intRandom(1, MAX_STRING_SIZE - 1); j++) str[j] = (char) gen.intRandom('a', 'z');
    str[j] = 0;
    if((data = patriciaStr.remove(str)) != NULL) {
      delete data;
      count++;
    }
  }
  ends = clock();
  printf("String: time to remove %d/%d random keys (%d remain) = %.2fs\n", count, i, (int) patriciaStr.counter, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  // Integer insertion
  gen.setSeed(200);
  starts = clock();
  for(int i = 0; i < RANDOM_ITERATIONS; i++) {
    DATAINTEGER *n = new DATAINTEGER();
    // NOTE: in this case the key storage in the object is only needed for later printing the contents. The key is already stored into the tree.
    if(patriciaInt.insert(n->value, n) != n) delete n; // Try to insert: use int keys.
  }
  ends = clock();
  printf("\nInteger (%d bytes): time to fill tree (%d distinct keys) = %.2fs\n", (int) sizeof(int), (int) patriciaInt.counter, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  // Integer searching
  starts = clock();
  count = 0;
  for(i = 0; i < RANDOM_ITERATIONS; i++)
    if(patriciaInt.search((int) gen.intRandom(0, INTEGER_UPPER_LIMIT)) != NULL) count++;
  ends = clock();
  printf("Integer (%d bytes): time to find %d/%d random keys = %.2fs\n", (int) sizeof(int), count, i, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  // Integer searching 2
  starts = clock();
  count = 0;
  for(i = 0; i < INTEGER_UPPER_LIMIT; i++)
    if(patriciaInt.search((int) i) != NULL) count++;
  ends = clock();
  printf("Integer (%d bytes): time to find all %d/%d keys = %.2fs\n", (int) sizeof(int), (int) count, i, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  // Integer prefix iterator
  printf("Integer: numbers with as prefix the 29 most significant bits of the decimal 73\n");
  iterator = patriciaInt.suffixIterator(73, 29);
  if(iterator) {
    for(DATAINTEGER *data = (DATAINTEGER*) iterator->next(); data != NULL; data = (DATAINTEGER*) iterator->next()) printf("%d\n", data->value);
    delete iterator;
  }

  // Integer removal
  gen.setSeed(200);
  starts = clock();
  count = 0;
  for(i = 0; i < RANDOM_ITERATIONS; i++)
    if((data = patriciaInt.remove((int) gen.intRandom(0, INTEGER_UPPER_LIMIT))) != NULL) {
      delete data;
      count++;
    }
  ends = clock();
  printf("Integer (%d bytes): time to remove %d/%d random keys (%d remain) = %.2fs\n", (int) sizeof(int), count, i, (int) patriciaInt.counter, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);


  // Test maximum integer size
  if(sizeof(int) != sizeof(intptr_t)) {  // A warning here is normal!
    // Maximum integer insertion
    gen.setSeed(300);
    starts = clock();
    for(i = 0; i < RANDOM_ITERATIONS; i++) {
      DATADUMMY *n = new DATADUMMY(); // Dummy data
      if(patriciaMaxInt.insert((intptr_t) gen.intRandom(0, INTEGER_UPPER_LIMIT), n) != n) delete n; // Try to insert: use maximum int keys
    }
    ends = clock();
    printf("\nMaximum integer (%d bytes): time to fill tree (%d distinct keys) = %.2fs\n", (int) sizeof(intptr_t), (int) patriciaMaxInt.counter, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

    // Maximum integer searching
    starts = clock();
    count = 0;
    for(i = 0; i < RANDOM_ITERATIONS; i++)
      if(patriciaMaxInt.search((intptr_t) gen.intRandom(0, INTEGER_UPPER_LIMIT)) != NULL) count++;
    ends = clock();
    printf("Maximum integer (%d bytes): time to find %d/%d random keys = %.2fs\n", (int) sizeof(intptr_t), count, i, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

    // Maximum integer searching 2
    starts = clock();
    count = 0;
    for(i = 0; i < INTEGER_UPPER_LIMIT; i++)
      if(patriciaMaxInt.search((intptr_t) i) != NULL) count++;
    ends = clock();
    printf("Maximum integer (%d bytes): time to find all %d/%d keys = %.2fs\n", (int) sizeof(intptr_t), count, i, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

    // Maximum integer removal
    gen.setSeed(300);
    starts = clock();
    count = 0;
    for(i = 0; i < RANDOM_ITERATIONS; i++)
      if((data = patriciaMaxInt.remove((intptr_t) gen.intRandom(0, INTEGER_UPPER_LIMIT))) != NULL) {
        delete data;
        count++;
      }
    ends = clock();
    printf("Maximum integer (%d bytes): time to remove %d/%d random keys (%d remain) = %.2fs\n", (int) sizeof(intptr_t), count, i, (int) patriciaMaxInt.counter, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);
  }


  // Long key insertion
  gen.setSeed(500);
  starts = clock();
  for(int i = 0; i < RANDOM_ITERATIONS; i++) {
    DATAFIXEDSIZE *n = new DATAFIXEDSIZE();
    if(patriciaFixed.insert(n->longkey, n) != n) delete n; // Try to insert
  }
  ends = clock();
  printf("\nLong key: time to fill tree (%d distinct keys) = %.2fs\n", (int) patriciaFixed.counter, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);


  // Long key searching
  starts = clock();
  count = 0;
  for(i = 0; i < RANDOM_ITERATIONS; i++) {
    int j;
    char key[FIXED_KEY_SIZE];
    for(j = 0; j < FIXED_KEY_SIZE; j++) key[j] = (char) gen.intRandom(0, 255);
    if(patriciaFixed.search(key) != NULL) count++;
  }
  ends = clock();
  printf("Long key: time to find %d/%d random long keys = %.2fs\n", count, i, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  // Long key searching 2
  starts = clock();
  count = 0;
  iterator = patriciaFixed.getIterator(0);
  for(DATAFIXEDSIZE *data = (DATAFIXEDSIZE*) iterator->next(); data != NULL; data = (DATAFIXEDSIZE*) iterator->next())
    if(patriciaFixed.search(data->longkey) != NULL) count++;
  delete iterator;
  ends = clock();
  printf("Long key: time to find all %d long keys = %.2fs\n", count, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);

  // Long key integer removal
  gen.setSeed(500);
  starts = clock();
  count = 0;
  for(i = 0; i < RANDOM_ITERATIONS; i++) {
    int j;
    char key[FIXED_KEY_SIZE];
    for(j = 0; j < FIXED_KEY_SIZE; j++) key[j] = (char) gen.intRandom(0, 255);
    if((data = patriciaFixed.remove(key)) != NULL) {
      delete data;
      count++;
    }
  }
  ends = clock();
  printf("Long key: time to remove %d/%d random keys (%d remain) = %.2fs\n", count, i, (int) patriciaFixed.counter, ((float) (ends-starts)) / (float) CLOCKS_PER_SEC);


  patriciaFixed.deleteAll();
  patriciaMaxInt.deleteAll();
  patriciaInt.deleteAll();
  patriciaStr.deleteAll();
  printf("\nMemory allocated at the end = %li\n", getAllocatedMemoryCounter());
  getchar();

  return 0;
}

