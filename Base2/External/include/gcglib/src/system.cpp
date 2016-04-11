/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    SYSTEM.CPP: multiplatform management for GCGlib

    Marcelo Bernardes Vieira
**************************************************************************************/
#include <stdlib.h>
#include "system.h"

// Platform specific includes
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  // Windows has not basename()
  char *basename(char *srcpath) {
    static char buffer[_MAX_FNAME + _MAX_EXT];
    char base[_MAX_FNAME], ext[_MAX_EXT];
    _splitpath(srcpath, NULL, NULL, base, ext);
    sprintf(buffer, "%s%s", base, ext);
    return buffer;
  }

  #ifdef _MSC_VER
    // Visual C++ specific code

    // Visual C++ does not provide gettimeofday
    int gettimeofday(struct timeval *tv, struct timezone *tz) {
      struct __timeb64 tb;
      _ftime64_s(&tb);

      if(tv) {
        tv->tv_sec = (long) (tb.time);
        tv->tv_usec = (long) (tb.millitm) * 1000;
      }

      if(tz) {
        tz->tz_minuteswest = tb.timezone;
        tz->tz_dsttime = tb.dstflag;
      }
      return 0;
    }
  #else
    // MinGW specific code
  #endif

#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  /* Unix specific code */

  // Linux has not strlwr()
  char *strlwr (char *a) {
    char *ret = a;
    while (*a != '\0') {
      if(isupper(*a)) *a = tolower (*a);
      ++a;
    }
    return ret;
  }

#else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

/////////////////////////////////////////////////////////////////
// Global information
/////////////////////////////////////////////////////////////////
volatile uintptr_t gcgallocated = 0;
volatile pthread_mutex_t gcgmemoryalloc = PTHREAD_MUTEX_INITIALIZER;

gcgPATRICIAMAP *openglcontext = NULL;

/////////////////////////////////////////////////////////////////
// Dynamic library entry points
/////////////////////////////////////////////////////////////////

#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  #ifdef _MSC_VER
    // Define constructor and destructor as static
    static void gcgOnLoad(void);
    static void gcgOnUnload(void);

    // Note that global variables are NOT SHARED. By default, each process using a DLL has its own
    // instance of all the DLLs global and static variables.
    // http://msdn.microsoft.com/en-us/library/h90dkhs0%28VS.80%29.aspx

    // Windows DLLs need Dllmain() function
    extern "C" BOOL APIENTRY DllMain(HANDLE hModule,	DWORD ul_reason_for_call, LPVOID lpReserved) {
      switch(ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: // A process is loading the DLL
                                gcgOnLoad();
                                break;
        case DLL_THREAD_ATTACH:   // A process is creating a new thread.
                                break;
        case DLL_THREAD_DETACH:   // A thread exits normally.
                                break;
        case DLL_PROCESS_DETACH:  // A process unloads the DLL.
                                gcgOnUnload();
                                break;
      }

      // Avoid annoying warnings
      hModule = NULL;
      ul_reason_for_call = 0;
      lpReserved = NULL;

      return TRUE;
    }
  #else
    // MinGW load and unload callbacks: GCC and Linux alike
    __attribute__((constructor (101))) void gcgOnLoad(void);  // Maximum priority = 101. 0-100 are reserved.
    __attribute__((destructor (101))) void gcgOnUnload(void); // Maximum priority = 101. 0-100 are reserved.
  #endif

#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  // Linux Shared Objects need special modifiers
  __attribute__((constructor (101))) void gcgOnLoad(void);  // Maximum priority = 101. 0-100 are reserved.
  __attribute__((destructor (101))) void gcgOnUnload(void); // Maximum priority = 101. 0-100 are reserved.
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif


// Called when the shared object or dynamic library is loaded
void gcgOnLoad() {
  // Allocate all internal objects

#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  openglcontext = new gcgPATRICIAMAP(sizeof(HGLRC));
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  openglcontext = new gcgPATRICIAMAP(sizeof(GLXContext));
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

}

// Called when the shared object or dynamic library is unloaded
void gcgOnUnload() {
  // Release all internal objects
  SAFE_DELETE(openglcontext);

  // Memory leak check
  if(gcgallocated != 0)
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_MEMORY, GCG_MEMORYLEAK), "gcgOnUnload(): unloading library but there are %li bytes allocated. (%s:%d)\n", gcgallocated, basename((char*)__FILE__), __LINE__);
}


/////////////////////////////////////////////////////////////////
// Multiplatform functions
/////////////////////////////////////////////////////////////////

// Visual C++ fix of operator new: needed for Windows DLLs
void* gcgCLASS::operator new(size_t size) {
  void *p = ALLOC(size);
  if(p == 0) throw std::bad_alloc(); // ANSI/ISO compliant behavior
  return p;
}

void *gcgCLASS::operator new(size_t size, const std::nothrow_t&) throw() {
  void *p = ALLOC(size);
  if(p == 0) return NULL;
  return p;
}

void* gcgCLASS::operator new[](size_t size) {
  void *p = ALLOC(size);
  if(p == 0) throw std::bad_alloc(); // ANSI/ISO compliant behavior
  return p;
}

void *gcgCLASS::operator new[](size_t size, const std::nothrow_t&) throw() {
  void *p = ALLOC(size);
  if(p == 0) return NULL;
  return p;
}

void gcgCLASS::operator delete(void *p) {
  FREE(p); // I can't use a SAFE_FREE here because pointer p cannot be changed during delete
}

void gcgCLASS::operator delete(void *p, const std::nothrow_t&) throw() {
  FREE(p); // I can't use a SAFE_FREE here because pointer p cannot be changed during delete
}

void gcgCLASS::operator delete[](void *p) {
  FREE(p); // I can't use a SAFE_FREE here because pointer p cannot be changed during delete
}

void gcgCLASS::operator delete[](void *p, const std::nothrow_t&) throw() {
  FREE(p); // I can't use a SAFE_FREE here because pointer p cannot be changed during delete
}



uintptr_t getAllocatedMemoryCounter() {
  uintptr_t res;

  pthread_mutex_lock((pthread_mutex_t*) &gcgmemoryalloc);
    res = gcgallocated;
  pthread_mutex_unlock((pthread_mutex_t*) &gcgmemoryalloc);
  return res;
}

