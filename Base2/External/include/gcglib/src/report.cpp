/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    REPORT.CPP: defines functions for log and message report.

    Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"

static const char *errortypes[] = {
  "SUCCESS",
  "ERROR",
  "WARNING",
  "INFO"
};

static const char *errordomains[] = {
  "", "", "", "", "", "", "", "",
  "MEMORY",
  "NETWORK",
  "FILE",
  "IMAGE",
  "CAMERA",
  "VIDEO",
  "THREAD",
  "GRAPHICS",
  "GEOMETRY",
  "ALGEBRA",
  "CONFIG",
  "DATA STRUCTURE",
  "SIGNAL",
  "USER"
};

static const char *errormessages[GCG_UNDEFINED+1] = {
    "Successful operation",
    "Allocation error",
    "Attempt to use an invalid object",
    "Bad parameters",
    "Initialization failure",
    "General internal failure",
    "Timeout",
    "Input/output failure",
    "Format mismatch",
    "Unrecognized format",
    "Unsupported format",
    "Unsupported operation",
    "Information retrieval failure",
    "General error reading or receiving",
    "General error writing or sending",
    "Current object is not connected",
    "General socket failure",
    "An error occurred in select()",
    "Connection closed by remote peer",
    "Host not found",
    "General connection failure",
    "An error occurred in setsocketopt()",
    "An error occurred in bind()",
    "An error occurred in listen()",
    "An error occurred in accept()",
    "An exception was caught",
    "Accepting with a bad pointer address or too small buffer",
    "Connection refused by host",
    "Connection aborted: data transmission time-out or protocol failure",
    "Sending with a bad pointer address or too small buffer",
    "Receiving with a bad pointer address or too small buffer",
    "Cannot send messages after shutdown",
    "Cannot receive messages after shutdown",
    "Invalid socket",
    "Stream control error",
    "Already running",
    "Creation failure",
    "Mutex failure",
    "Semaphore failure",
    "No buffer space available or queue full",
    "Job not executed",
    "Interrupted by extern signal",
    "This object was active and is now disconnected",
    "Process, thread or job could not be stoped",
    "Process, thread or job is not running",
    "Object did not exit properly",
    "Resource release error",
    "Lock was not acquired",
    "Object is not owned by the caller",
    "Removal error",
    "Data was not stored nor retrieved",
    "Open error",
    "Decode error",
    "Function call failed",
    "Graphics system error",
    "Incompatibility error",
    "Error configuring request",
    "Unavailable resource",
    "Start error",
    "Out of bounds",
    "End of stream",
    "Operation failed",
    "Memory leak detected",
    "Insertion failed",
    "Adapting to new conditions",
    "Undefined error"
};

// Flags to log TYPES and DOMAINS: initially all types are enabled except warnings and information
volatile static bool flags[GCG_DOMAIN_USER+1] = {true, true, false, false, true, true, true, true, true, true,
                                                 true, true, true, true, true, true, true, true, true, true,
                                                 true };

// Locks to access static variables
static pthread_mutex_t errorlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t loglock   = PTHREAD_MUTEX_INITIALIZER;

// Output log stream
static FILE *logstream = stderr;

// Maximum statically or locally allocated strings
#define MAX_STRING 512

// Last error information
volatile static int	lastreportcode = GCG_SUCCESS;
static char lastextrastr[MAX_STRING];

FILE *gcgGetLogStream() {
  // Lock variables
  pthread_mutex_lock(&loglock);
  FILE *res = logstream;
  // Release variables
  pthread_mutex_unlock(&loglock);

  return res;
}

FILE *gcgSetLogStream(FILE *stream) {
  // Lock variables
  pthread_mutex_lock(&loglock);
  FILE *res = logstream;
  logstream = stream;
  // Release variables
  pthread_mutex_unlock(&loglock);

  return res;
}

 void gcgOutputLog(const char *format, ...) {
  char    buffer[MAX_STRING]; // For strings shorter than or equal to MAX_STRING
  int     size = MAX_STRING;  // Initial guess for string length
  char    *texto = buffer;
  va_list va;

  // Check format
  if(!format) return;

  // Parse input string reallocating memory to fit output
  do {
    int n;

    // Try to print in the allocated space.
    va_start(va, format);
    n = vsnprintf(texto, size, format, va);
    va_end(va);

    // If that worked, the string is ready.
    if(n > -1 && n < size) break;

    // Else try again with more space.
    if(n > -1) size = n + 1;   // glibc 2.1
    else size = size + MAX_STRING;    // glibc 2.0

    if(texto == buffer) texto = (char*) ALLOC(size * sizeof(char));
    else texto = (char*) REALLOC(texto, sizeof(char) * size);
    if(!texto) return;
  } while(true);

  // Lock variables
  pthread_mutex_lock(&loglock);

  if(logstream != NULL) {
    struct tm *t;
    time_t long_time;
    //long miliseconds;

#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
    struct timeval now;
    gettimeofday(&now, NULL);
    long_time = now.tv_sec;
    //miliseconds = (now.tv_usec / 1000); // microseconds to miliseconds
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    long_time = now.tv_sec;
    //miliseconds = (now.tv_nsec / 1000000); // nanoseconds to miliseconds
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
    t = localtime(&long_time); // Local time. Opposed to UTC time gmtime().

    // Sends the log message to the stream. Not using strftime()
    //fprintf(logstream, "\n[%02d/%02d/%04d %02d:%02d:%02d.%03li] %s", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, miliseconds, texto);
    fprintf(logstream, "\n[%02d/%02d/%04d %02d:%02d:%02d] %s", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, texto);
  }

  // Release variables
  pthread_mutex_unlock(&loglock);

  if(texto != buffer) SAFE_FREE(texto);
}

bool gcgReport(int code, const char *format, ...) {
  char *ptr = lastextrastr;

  // Lock variables
  pthread_mutex_lock(&errorlock);

  // Saves report code
  lastreportcode = code;

  // Retrieve report codes
  int ertype = GCG_REPORT_TYPE(code);

  if(ertype != GCG_SUCCESS) {
    // Decode
    int erdoma = GCG_REPORT_DOMAIN(code);
    int ermess  = GCG_REPORT_MESSAGE(code);

    // Check bounds
    if(ertype < GCG_SUCCESS || ertype > GCG_INFORMATION) ertype = GCG_INFORMATION;
    if(erdoma < GCG_DOMAIN_MEMORY || erdoma > GCG_DOMAIN_USER) erdoma = GCG_DOMAIN_USER;
    if(ermess < GCG_NONE || ermess > GCG_UNDEFINED) ermess = GCG_UNDEFINED;

    // Report to log stream, if logging is enabled for TYPE and DOMAIN
    if(flags[ertype] && flags[erdoma]) {
      if(format == NULL) gcgOutputLog("%s/%s %4X: %s.", errortypes[ertype], errordomains[erdoma], code, errormessages[ermess]);
      else {
        // Parse input error string
        int n;
        va_list va;

        // Try to print in the static space limited to MAX_STRING
        va_start(va, format);
        n = vsnprintf(ptr, MAX_STRING, format, va);
        va_end(va);

        // If that worked, use the string.
        if(n < 0 || n >= MAX_STRING) ptr = NULL;

        if(ptr) gcgOutputLog("%s/%s %4X: %s", errortypes[ertype], errordomains[erdoma], code, ptr);
        else gcgOutputLog("%s/%s %4X: %s.", errortypes[ertype], errordomains[erdoma], code, errormessages[ermess]);
      }
    }
  }

  // Release variables
  pthread_mutex_unlock(&errorlock);

  return true;
}

int gcgGetReport(int sizestring, char *extrastring) {
  // Lock variables
  pthread_mutex_lock(&errorlock);
  int res = lastreportcode;

  if(sizestring > 0 && extrastring != NULL) strncpy(extrastring, lastextrastr, sizestring);

  // Release variables
  pthread_mutex_unlock(&errorlock);

  return res;
}

const char *gcgGetReportString(char **typestr, char **domainstr) {
  // Locks variables
  pthread_mutex_lock(&errorlock);
  // Retrieve error codes
  int ertype = GCG_REPORT_TYPE(lastreportcode);
  int erdoma = GCG_REPORT_DOMAIN(lastreportcode);
  int ermess  = GCG_REPORT_MESSAGE(lastreportcode);
  // Release variables
  pthread_mutex_unlock(&errorlock);

  // Check bounds
  if(ertype < GCG_SUCCESS || ertype > GCG_INFORMATION) ertype = GCG_INFORMATION;
  if(erdoma < GCG_DOMAIN_MEMORY || erdoma > GCG_DOMAIN_USER) erdoma = GCG_DOMAIN_USER;
  if(ermess < GCG_NONE || ermess > GCG_UNDEFINED) ermess = GCG_UNDEFINED;
  if(domainstr != NULL) *domainstr = (char*) errordomains[erdoma];
  if(typestr != NULL) *typestr = (char*) errortypes[ertype];

  return errormessages[ermess];
}

bool gcgEnableReportLog(int code, bool enable) {
  // Check if code is a TYPE
  if(code >= GCG_SUCCESS && code <= GCG_INFORMATION) {
    flags[code] = enable;
  } else // Check if code is a DOMAIN
      if(code >= GCG_DOMAIN_MEMORY && code <= GCG_DOMAIN_USER) {
        flags[code] = enable;
      } else // Check if code is GCG_REPORT_ALL
          if(code < 0) {
            for(int i = 0; i <= GCG_DOMAIN_USER; i++) flags[i] = enable;
          } else {
              // Bad parameter
              if(gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_CONFIG, GCG_BADPARAMETERS), (char*) __FILE__, __LINE__))
                gcgOutputLog("gcgEnableReportLog(): code parameter is out of range.");
              return false;
            }

  // No error
  return true;
}

bool gcgDisableReportLog(int code, bool enable) {
  // Check if code is a TYPE
  if(code >= GCG_SUCCESS && code <= GCG_INFORMATION) {
    flags[code] = enable;
  } else // Check if code is a DOMAIN
      if(code >= GCG_DOMAIN_MEMORY && code <= GCG_DOMAIN_USER) {
        flags[code] = enable;
      } else // Check if code is GCG_REPORT_ALL
          if(code < 0) {
            for(int i = 0; i <= GCG_DOMAIN_USER; i++) flags[i] = enable;
          } else {
              // Bad parameter
              if(gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_CONFIG, GCG_BADPARAMETERS), (char*) __FILE__, __LINE__))
                gcgOutputLog("gcgEnableReportLog(): code parameter is out of range.");
              return false;
            }

  // No error
  return true;
}
