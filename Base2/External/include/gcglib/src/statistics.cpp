/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   STATISTICS.CPP: functions for statistics.

   Marcelo Bernardes Vieira
   Eder de Almeida Perez
**************************************************************************************/

#include "system.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     HISTOGRAM PROCESSING  ///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
// Compute the Probability Distribution Function from a histogram.
// Returns true if successful.
/////////////////////////////////////////////////////////////////////////////////////
int gcgPDFfromHistogram(float *pdf, unsigned int *histogram, unsigned int Nbins) {
  unsigned int N = 0;
  for(register unsigned int i = 0; i < Nbins; i++) N += histogram[i];
  if(N == 0) memset(pdf, 0, sizeof(float) * Nbins);
  else {
    float inv = (float) 1.0 / (float) N;
    for(register unsigned int i = 0; i < Nbins; i++) pdf[i] = (float) histogram[i] * inv;
  }
	return true;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
int gcgPDFfromHistogram(double *pdf, unsigned int *histogram, unsigned int Nbins) {
  unsigned int N = 0;
  for(register unsigned int i = 0; i < Nbins; i++) N += histogram[i];
  if(N == 0) memset(pdf, 0, sizeof(double) * Nbins);
  else {
    double inv = (double) 1.0 / (double) N;
    for(register unsigned int i = 0; i < Nbins; i++) pdf[i] = (double) histogram[i] * inv;
  }
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////
// Find the optimal threshold using Otsu's method.
// Nobuyuki Otsu, "A threshold selection method from gray level histograms",
// IEEE Transactions on Systems, Man and Cybernetics, vol.9, n.1, pp.62-66, jan/1979.
/////////////////////////////////////////////////////////////////////////////////////
unsigned int gcgOptimalThresholdOtsu(float *pdf, unsigned int left, unsigned int right) {
#define OMEGA(omega, left, right) { omega = 0; for(register unsigned int i = left; i <= right; i++) omega += pdf[i]; }
#define MI(m, left, right, omega) { m = 0; \
                                    for(register unsigned int i = left; i <= right; i++) m += i * pdf[i]; \
                                    if(fabs(omega) > EPSILON)  m /= omega; \
                                  }
  unsigned int threshold = 0;
  float mi_T = 0, mi_1, mi_2, omega_1, omega_2, sigma_b, aux = 0;

  for(unsigned int i = left; i <= right; i++) mi_T += i * pdf[i];

  sigma_b = 0;
  for(unsigned int k = left; k < right; k++){
    OMEGA(omega_1, left, k);
    OMEGA(omega_2, k + 1, right);
    MI(mi_1, left, k, omega_1);
    MI(mi_2, k + 1, right, omega_2);
    aux = omega_1 * SQR(mi_1 - mi_T) + omega_2 * SQR(mi_2 - mi_T);
    if(aux > sigma_b){
      sigma_b = aux;
      threshold = k;
    }
  }

  return threshold;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
unsigned int gcgOptimalThresholdOtsu(double *pdf, unsigned int left, unsigned int right) {
  unsigned int threshold = 0;
  double mi_T = 0, mi_1, mi_2, omega_1, omega_2, sigma_b, aux = 0;

  for(unsigned int i = left; i <= right; i++) mi_T += i * pdf[i];

  sigma_b = 0;
  for(unsigned int k = left; k < right; k++){
    OMEGA(omega_1, left, k);
    OMEGA(omega_2, k + 1, right);
    MI(mi_1, left, k, omega_1);
    MI(mi_2, k + 1, right, omega_2);
    aux = omega_1 * SQR(mi_1 - mi_T) + omega_2 * SQR(mi_2 - mi_T);
    if(aux > sigma_b){
      sigma_b = aux;
      threshold = k;
    }
  }

  return threshold;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     RANDOM NUMBER AND VECTOR GENERATION /////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
// gcgRANDOM constructor. Uses a seed generated by conventional
// C rand() funtion.
/////////////////////////////////////////////////////////////////////////
gcgRANDOM::gcgRANDOM() {
  mag01[0] = 0x0;
  mag01[1] = GCG_MERSENNE_MATRIX_A; // mag01[x] = x * MATRIX_A for x=0,1
  setSeed(rand());     // A random initial seed is used. Guarantees several instances.
}

// gcgRANDOM destructor.
gcgRANDOM::~gcgRANDOM() {
}

// gcgRANDOM constructor with seed.
gcgRANDOM::gcgRANDOM(unsigned int seed) {
  mag01[0] = 0x0;
  mag01[1] = GCG_MERSENNE_MATRIX_A; // mag01[x] = x * MATRIX_A for x=0,1
  setSeed(seed);
}

// Sets a new seed to the Mersenne Twister generator
void gcgRANDOM::setSeed(unsigned int seed) {
  // Setting initial seeds to mt[N] using the generator line 25 of table 1
  // in [KNUTH 1981, The Art of Computer Programming, vol.2 (2nd. Ed.), pp102]
  mt[0] = seed & 0xffffffff;
  for(mti = 1; mti < GCG_MERSENNE_N; mti++)
    mt[mti] = (69069 * mt[mti - 1]) & 0xffffffff;

  lastInterval = 0;

  // Randomize some more
  for(int i = 0; i < 37; i++) bitRandom();
}

// Generates 32 random bits.
unsigned int gcgRANDOM::bitRandom() {
#define GCG_MERSENNE_M 397
#define GCG_MERSENNE_UPPER_MASK            0x80000000  // Most significant w-r bits
#define GCG_MERSENNE_LOWER_MASK            0x7fffffff  // Least significant r bits

// Tempering parameters
#define GCG_MERSENNE_TEMPERING_MASK_B      0x9d2c5680
#define GCG_MERSENNE_TEMPERING_MASK_C      0xefc60000
#define GCG_MERSENNE_TEMPERING_SHIFT_U(y)  (y >> 11)
#define GCG_MERSENNE_TEMPERING_SHIFT_S(y)  (y << 7)
#define GCG_MERSENNE_TEMPERING_SHIFT_T(y)  (y << 15)
#define GCG_MERSENNE_TEMPERING_SHIFT_L(y)  (y >> 18)

  unsigned int y;

  if(mti >= GCG_MERSENNE_N) { // Generate N words at one time
    int kk;

    for(kk = 0; kk < (GCG_MERSENNE_N - GCG_MERSENNE_M); kk++) {
      y = (mt[kk] & GCG_MERSENNE_UPPER_MASK) | (mt[kk + 1] & GCG_MERSENNE_LOWER_MASK);
      mt[kk] = mt[kk + GCG_MERSENNE_M] ^ (y >> 1) ^ mag01[y & 0x1];
    }

    for(; kk < GCG_MERSENNE_N - 1; kk++) {
      y = (mt[kk] && GCG_MERSENNE_UPPER_MASK) | (mt[kk + 1] & GCG_MERSENNE_LOWER_MASK);
      mt[kk] = mt[kk + (GCG_MERSENNE_M - GCG_MERSENNE_N)] ^ (y >> 1) ^ mag01[y & 0x01];
    }
    y = (mt[GCG_MERSENNE_N - 1] & GCG_MERSENNE_UPPER_MASK) | (mt[0] & GCG_MERSENNE_LOWER_MASK);
    mt[GCG_MERSENNE_N - 1] = mt[GCG_MERSENNE_M - 1] ^ (y >> 1) ^ mag01[y & 0x1];

    mti = 0;
  }

  y = mt[mti++];
  y ^= GCG_MERSENNE_TEMPERING_SHIFT_U(y);
  y ^= GCG_MERSENNE_TEMPERING_SHIFT_S(y) & GCG_MERSENNE_TEMPERING_MASK_B;
  y ^= GCG_MERSENNE_TEMPERING_SHIFT_T(y) & GCG_MERSENNE_TEMPERING_MASK_C;
  y ^= GCG_MERSENNE_TEMPERING_SHIFT_L(y);

  return y; // For integer generation
}

// Output random float in [0, 1)
double gcgRANDOM::random() {
  return (double) bitRandom() * ((double) 1.0/((double)(unsigned int)(-1L) + (double) 1.0));
}

// Output random integer in the interval min <= x <= max
// Each output value has exactly the same probability.
int gcgRANDOM::intRandom(int min, int max) {
  unsigned int interval;     // Length of interval
  unsigned int bran;         // Random bits
  unsigned int iran;         // bran / interval
  unsigned int remainder;    // bran % interval

  // Swap if not consistent
  if(min == max) return min;
  if(min > max) {
    int aux = min;
    min = max;
    max = aux;
  }

  interval = (unsigned int) (max - min + 1);
  if (interval != lastInterval) {
    // Interval length has changed. Must calculate rejection limit
    // Reject when iran = 2^32 / interval
    // We can't make 2^32 so we use 2^32-1 and correct afterwards
    RLimit = (unsigned int) 0xffffffff / interval;
    if((unsigned int) 0xffffffff % interval == interval - 1) RLimit++;
  }
  do { // Rejection loop
    bran = bitRandom();
    iran = bran / interval;
    remainder = bran % interval;
  } while(iran >= RLimit);

  // Number in range
  return (int) remainder + min;
}




// Output a float in the interval min <= x <= max
// Each output value has exactly the same probability.
float gcgRANDOM::floatRandom(float min, float max) {
  // Swap if not consistent
  if(min == max) return min;
  if(min > max) {
    float aux = min;
    min = max;
    max = aux;
  }

  return min + (float) bitRandom() * ((float) (max - min) / ((float)(unsigned int)(-1L) + (float) 1.0));
}

// Output a double in the interval min <= x <= max
// Each output value has exactly the same probability.
double gcgRANDOM::doubleRandom(double min, double max) {
  // Swap if not consistent
  if(min == max) return min;
  if(min > max) {
    double aux = min;
    min = max;
    max = aux;
  }

  return min + (double) bitRandom() * ((double) (max - min) / ((double)(unsigned int)(-1L) + (double) 1.0));
}




/////////////////////////////////////////////////////////////////////////
// gcgRANDOMGAUSSIAN constructor. Uses a seed generated by conventional
// C rand() funtion.
/////////////////////////////////////////////////////////////////////////
gcgRANDOMGAUSSIAN::gcgRANDOMGAUSSIAN() {
  // A random initial seed is used. Guarantees several instances.
  setSeed(rand(), rand());
}

// gcgRANDOMGAUSSIAN destructor.
gcgRANDOMGAUSSIAN::~gcgRANDOMGAUSSIAN() {
}


// gcgRANDOMGAUSSIAN constructor with seeds.  seed1 must be different than seed2.
gcgRANDOMGAUSSIAN::gcgRANDOMGAUSSIAN(unsigned int seed1, unsigned int seed2) {
  setSeed(seed1, seed2);
}

// Sets a new seed to the gaussian generator. seed1 must be different than seed2.
void gcgRANDOMGAUSSIAN::setSeed(unsigned int seed1, unsigned int seed2) {
  ngen1.setSeed(seed1);
  ngen2.setSeed(seed2);
  hasValue = false;
}

// Output random float in N(0, 1)
double gcgRANDOMGAUSSIAN::random() {
  // Checks the existence of the second value.
  if(hasValue) {
    hasValue = false;
    return secondValue;
  }

  // Compute next two values
  hasValue = true;
  double u1 =  (double) sqrt(-2.0 * log(ngen1.random())), u2 = (double) (2.0 * M_PI * ngen2.random());
  secondValue = u1 * sin(u2); // keeps the second value

  return u1 * cos(u2); // returns the first one
}


/////////////////////////////////////////////////////////////////////////
// gcgRANDOMVECTOR constructor. Uses a seed generated by conventional
// C rand() funtion.
/////////////////////////////////////////////////////////////////////////
gcgRANDOMVECTOR::gcgRANDOMVECTOR() {
  setSeed(rand(), rand()); // A random initial seed is used. Guarantees several instances.
}

// gcgRANDOMVECTOR destructor.
gcgRANDOMVECTOR::~gcgRANDOMVECTOR() {
}

// gcgRANDOMVECTOR constructor with seed.
gcgRANDOMVECTOR::gcgRANDOMVECTOR(unsigned int seed1, unsigned int seed2) {
  setSeed(seed1, seed2);
}

// Sets a new seed to the Mersenne Twister generator
void gcgRANDOMVECTOR::setSeed(unsigned int seed1, unsigned int seed2) {
  ngen1.setSeed(seed1);
  ngen2.setSeed(seed2);
}

// Output float random vector in S^2
void gcgRANDOMVECTOR::random(VECTOR3 vet) {
  register float fi = (float) ((2.0 * M_PI) * ngen1.random()); // Tira angulo aleatorio

  vet[2] = (float)  (1.0 - 2.0 * ngen2.random()); // Calcula componente Z
  vet[0] = (float)  (sqrt(1.0 - vet[2] * vet[2]) * cos(fi)); // Calcula componente X
  vet[1] = (float)  (vet[0] * tan(fi));  // Calcula componente Y
}

// Output double random vector in S^2
void gcgRANDOMVECTOR::random(VECTOR3d vet) {
  register double fi = (double) (2.0 * M_PI) * ngen1.random(); // Tira angulo aleatorio

  vet[2] = (double)  (1.0 - 2.0 * ngen2.random()); // Calcula componente Z
  vet[0] = (double)  (sqrt(1.0 - vet[2] * vet[2]) * cos(fi)); // Calcula componente X
  vet[1] = (double)  (vet[0] * tan(fi));  // Calcula componente Y
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////     ESTIMATION OF NUMBER OF EVENTS PER SECOND      /////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


// Internal struct: needed to free the gcg.h from depending on struct timeval.
typedef struct _GCG_EVENTSPERSECOND {
  float           currenteps;		// Keeps the current raw events-per-second computed
  unsigned int    eventcounter;	// Keeps the number of events
  double          elapsedtime;  // Time elapsed in seconds since last calculation

#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  struct timeval  lasttime;     // Keeps the last timestamp
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  struct timespec lasttime;     // Keeps the last timestamp
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

  unsigned int callsnumber;     // Number of calls without updating eps

  // Filters for estimatives
  unsigned int index;
  unsigned int E[4];            // Events by index
  double       T[4];            // Time variation by index
} GCG_EVENTSPERSECOND;


gcgEVENTSPERSECOND::gcgEVENTSPERSECOND() {
  register GCG_EVENTSPERSECOND *eventsec;

  // Try to allocate object space.
  // We need this to free the gcg.h from using timeval struct
  this->handle = eventsec = (GCG_EVENTSPERSECOND*) ALLOC(sizeof(GCG_EVENTSPERSECOND));
  if(eventsec == NULL) return;

  // Ok, we can construct a valid object
  eventsec->currenteps = 0;
  eventsec->eventcounter = 0;
  eventsec->elapsedtime = 0.0;
  memset(&eventsec->lasttime, 0, sizeof(eventsec->lasttime));
  eventsec->callsnumber = 0;
  eventsec->index = 0;
  memset(&eventsec->E, 0, sizeof(eventsec->E));
  memset(&eventsec->T, 0, sizeof(eventsec->T));
}

gcgEVENTSPERSECOND::~gcgEVENTSPERSECOND() {
  register GCG_EVENTSPERSECOND *eventsec = (GCG_EVENTSPERSECOND*) this->handle;
  SAFE_FREE(eventsec);
}

void gcgEVENTSPERSECOND::startingEvents() {
  register GCG_EVENTSPERSECOND *eventsec = (GCG_EVENTSPERSECOND*) this->handle;
  if(eventsec == NULL) return;

  // Platform specific code
  #if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      gettimeofday(&eventsec->lasttime, NULL);
  #elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      clock_gettime(CLOCK_REALTIME, &eventsec->lasttime);
  #else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
  #endif
}

float gcgEVENTSPERSECOND::finishedEvents(int nevents) {
  register GCG_EVENTSPERSECOND *eventsec = (GCG_EVENTSPERSECOND*) this->handle;
  if(eventsec == NULL) return 0.0;

	// Increments the event counter
  eventsec->eventcounter += nevents;

  // Platform specific code
  #if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      struct timeval now;
      gettimeofday(&now, NULL);

      // Computes elapsed time
      eventsec->elapsedtime += (double) (now.tv_sec - eventsec->lasttime.tv_sec) + ((double) (now.tv_usec - eventsec->lasttime.tv_usec) / (double) 1000000.0);
  #elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      struct timespec now;
      clock_gettime(CLOCK_REALTIME, &now);

      // Computes elapsed time and integrates it
      eventsec->elapsedtime += (double) (now.tv_sec - eventsec->lasttime.tv_sec) + ((double) (now.tv_nsec - eventsec->lasttime.tv_nsec) / (double) 1000000000.0);
  #else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
  #endif

	eventsec->callsnumber++;

  // Check if the elapsed time passed half second
  if(eventsec->elapsedtime >= 0.5 || (eventsec->callsnumber > 100 && eventsec->callsnumber < 1000)) { // Avoids frequent recomputations
    // Computes a new events per second
    eventsec->E[eventsec->index] = eventsec->eventcounter;
    eventsec->T[eventsec->index] = eventsec->elapsedtime;

    unsigned int En1 = (eventsec->E[(eventsec->index+1)%4] + 2 * eventsec->E[(eventsec->index+2)%4] + 3 * eventsec->E[(eventsec->index+3)%4] + 3 * eventsec->E[eventsec->index]);
    double       Tn1 = (eventsec->T[(eventsec->index+1)%4] + 2 * eventsec->T[(eventsec->index+2)%4] + 3 * eventsec->T[(eventsec->index+3)%4] + 3 * eventsec->T[eventsec->index]);

    // Update EPS
    eventsec->currenteps = (float) (((double) En1) / Tn1);
    eventsec->index = (eventsec->index + 1) % 4;

    // Sets counters to zero
  #if defined(WINDOWS32) || defined(WINDOWS64)
    eventsec->lasttime.tv_sec  = now.tv_sec;
    eventsec->lasttime.tv_usec = now.tv_usec;
  #elif defined(LINUX32) || defined(LINUX64)
    eventsec->lasttime.tv_sec  = now.tv_sec;
    eventsec->lasttime.tv_nsec = now.tv_nsec;
  #endif
    eventsec->elapsedtime = 0.0;
    eventsec->eventcounter = 0;
    eventsec->callsnumber = 0;
  }

  // Returns current EPS
  return eventsec->currenteps;
}

float gcgEVENTSPERSECOND::getEventsPerSecond() {
  register GCG_EVENTSPERSECOND *eventsec = (GCG_EVENTSPERSECOND*) this->handle;
  if(eventsec == NULL) return 0.0;

  return eventsec->currenteps;
}
