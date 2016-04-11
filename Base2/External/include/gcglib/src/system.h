
#ifndef _SYSTEM_H_

#ifdef __cplusplus
  extern "C++" {    // All names have C++ style
#else
  #error "GCC library must be compiled with C++ compilers"
#endif

// GCGlib comes first
#include "gcg.h"

// Multi-platform includes
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <malloc.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

// Multiplatform defines
#ifndef ALLOC
#define ALLOC(s)  gcgmalloc(s)
#endif

#ifndef FREE
#define FREE(p)   gcgfree(p)
#endif

// Multiplatform defines

// Memory allocation helpers
#ifndef REALLOC
#define REALLOC(p, s)  gcgrealloc(p, s)
#endif


#ifndef INCREASE_ALLOCATED
#define INCREASE_ALLOCATED(m) { pthread_mutex_lock((pthread_mutex_t*) &gcgmemoryalloc); \
                                  gcgallocated += (long) (m); \
                                pthread_mutex_unlock((pthread_mutex_t*) &gcgmemoryalloc);} /* \
                                if(m > 0) printf("Alocado: %li  Total = %li\n", (m), gcgallocated); \
                                else printf("Liberado: %li  Total = %li\n", (m), gcgallocated); }*/
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(a) { if((a) != NULL) { delete (a);  (a) = NULL; }  }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(a) { if((a) != NULL) { delete [] (a);  (a) = NULL; }  }
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(a) { if((a) != NULL) { FREE(a);  (a) = NULL; }  }
#endif

// Dynamic arrays helpers
#ifndef CHECK_DYNAMIC_ARRAY_CAPACITY
#define CHECK_DYNAMIC_ARRAY_CAPACITY(dynarray, arraysize, neededsize, arraytype, returntype) {  \
  if(arraysize < neededsize) {                                                                  \
	  int max = neededsize + DEFAULT_ARRAY_BLOCK;                                                 \
    arraytype *na = (arraytype*) REALLOC(dynarray, sizeof(arraytype) * max);                    \
	  if(na == NULL) {                                                                            \
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "CHECK_DYNAMIC_ARRAY_CAPACITY(): dynamic array allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__); \
      return returntype;                                                                        \
    }                                                                                           \
	  dynarray = na;                                                                              \
	  arraysize = max;                                                                            \
  } \
 }

#define CHECK_DYNAMIC_ARRAY_CAPACITY_VOID(dynarray, arraysize, neededsize, arraytype) {   \
  if(arraysize < neededsize) {                                                            \
	  int max = neededsize + DEFAULT_ARRAY_BLOCK;                                           \
    arraytype *na = (arraytype*) REALLOC(dynarray, sizeof(arraytype) * max);              \
	  if(na == NULL) {                                                                      \
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "CHECK_DYNAMIC_ARRAY_CAPACITY(): dynamic array allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__); \
      return;                                                                             \
    }                                                                                     \
	  dynarray = na;                                                                        \
	  arraysize = max;                                                                      \
  }                                                                                       \
 }
 #endif


// OpenGL context helpers

#ifndef GET_OPENGL_CONTEXT
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  #define GET_OPENGL_CONTEXT(op) {                            \
    HGLRC handle = wglGetCurrentContext();                    \
    if(handle == NULL) {                                      \
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_UNAVAILABLERESOURCE), "GET_OPENGL_CONTEXT(): No OpenGL active context. Check your graphics code. (%s:%d)", basename((char*)__FILE__), __LINE__); \
      return NULL;                                            \
    }                                                         \
    op = (gcgOPENGLCONTEXT*) openglcontext->search(handle);   \
    if(op == NULL) {                                          \
      op = new gcgOPENGLCONTEXT();                            \
      if(op == NULL) {                                        \
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "GET_OPENGL_CONTEXT(): OpenGL internal context allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__); \
        return NULL;                                          \
      }                                                       \
      if((gcgOPENGLCONTEXT*) openglcontext->insert(handle, op) != op) {   \
        SAFE_DELETE(op);                                      \
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_GRAPHICSERROR), "GET_OPENGL_CONTEXT(): Could not config new OpenGL internal context. (%s:%d)", basename((char*)__FILE__), __LINE__); \
        return NULL;                                          \
      }                                                       \
    }                                                         \
  }
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  #define GET_OPENGL_CONTEXT(op) {                            \
    GLXContext handle = glXGetCurrentContext();               \
    if(handle == NULL) {                                      \
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_UNAVAILABLERESOURCE), "GET_OPENGL_CONTEXT(): No OpenGL active context. Check your graphics code. (%s:%d)", basename((char*)__FILE__), __LINE__); \
      return NULL;                                            \
    }                                                         \
    op = (gcgOPENGLCONTEXT*) openglcontext->search(handle);   \
    if(op == NULL) {                                          \
      op = new gcgOPENGLCONTEXT();                            \
      if(op == NULL) {                                        \
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "GET_OPENGL_CONTEXT(): OpenGL internal context allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__); \
        return NULL;                                          \
      }                                                       \
      if((gcgOPENGLCONTEXT*) openglcontext->insert(handle, op) != op) {   \
        SAFE_DELETE(op);                                      \
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_GRAPHICSERROR), "GET_OPENGL_CONTEXT(): Could not config new OpenGL internal context. (%s:%d)", basename((char*)__FILE__), __LINE__); \
        return NULL;                                          \
      }                                                       \
    }                                                         \
  }
  #else
    #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
  #endif
#endif


// Platform specific includes and definitions
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  extern "C" {    // These names must have C style
    #define NEED_FAR_POINTERS
    #define XMD_H
    #include <jpeglib.h>
    #undef FAR
  }

  #include <winsock2.h>
  #include <windows.h>
  #include <GL/gl.h>
  #include <GL/glu.h>
  #include <time.h>
  #include <io.h>
  #include <share.h>
  #include <sys/locking.h>

  #ifdef _MSC_VER
    // Visual C++ specific definitions
    // Libraries to be linked in Visual C++: GCGlib is intended to be a Dynamic Linked Library for Windows usage

    // Disable annoying warnings
    #pragma warning(disable : 4996) // POSIX named function marked as deprecated
    #pragma warning(disable : 4995) // Function marked as deprecated
    #pragma warning(disable : 4127) // Conditional expression is constant

    // For Visual C++, which does not provide gettimeofday
    #include <sys/timeb.h>

    #ifndef timezone
      struct timezone {
        int tz_minuteswest;
	      int tz_dsttime;
      };
	  #endif

    int gettimeofday(struct timeval *tv, struct timezone *tz);

	  /////////////////////////////////////////////////////////////////////////////////
	  // This IS a Visual C++ compiler: using DirectShow for video processing
	  /////////////////////////////////////////////////////////////////////////////////
	  #include <atlbase.h>	// Available ONLY in non-express versions of Visual Studio. Workaround here: http://www.quantcode.com/modules/smartfaq/faq.php?faqid=99

	  ////////////////////////////////////////////////////////////////////
	  // Due to problems in VC++, we need to include the following defines
	  #define __IDxtCompositor_INTERFACE_DEFINED__
	  #define __IDxtAlphaSetter_INTERFACE_DEFINED__
	  #define __IDxtJpeg_INTERFACE_DEFINED__
	  #define __IDxtKey_INTERFACE_DEFINED__
	  ////////////////////////////////////////////////////////////////////

	  // DirectShow includes
	  #include <strmif.h>
	  #include <dshow.h>
	  #include <stdlib.h>
	  #include <commctrl.h>
	  #include <Dvdmedia.h>
	  #include <Bdaiface.h>
    #include <initguid.h>
    #include <streams.h>

    // Use internal version in Windows of isnan()
    #include <float.h>
    #define isnan(x) _isnan(x)

    // Sample renderer GUI
    DEFINE_GUID(CLSID_SampleRenderer, 0x4d4b1600, 0x33ac, 0x11cf, 0xbf, 0x30, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a);

  	//#include <qedit.h>  // UNCOMMENT THIS LINE IF YOU HAVE qedit.h! It is not included in most Visual Studio releases, including 2010.

    ///////////////////////////////////////////////////////////////////////////////////
    // FORCING THE INCLUSION OF A qedit.h VERSION
    ///////////////////////////////////////////////////////////////////////////////////

    #ifndef __qedit_h__
      #define __qedit_h__

      ///////////////////////////////////////////////////////////////////////////////////

      #pragma once

      ///////////////////////////////////////////////////////////////////////////////////

      interface ISampleGrabberCB :	public IUnknown {
	      virtual STDMETHODIMP SampleCB( double SampleTime, IMediaSample *pSample ) = 0;
	      virtual STDMETHODIMP BufferCB( double SampleTime, BYTE *pBuffer, long BufferLen ) = 0;
      };

      ///////////////////////////////////////////////////////////////////////////////////

      static const IID IID_ISampleGrabberCB = { 0x0579154A, 0x2B53, 0x4994, { 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85 } };

      ///////////////////////////////////////////////////////////////////////////////////

      interface ISampleGrabber : public IUnknown {
	      virtual HRESULT STDMETHODCALLTYPE SetOneShot( BOOL OneShot ) = 0;
	      virtual HRESULT STDMETHODCALLTYPE SetMediaType( const AM_MEDIA_TYPE *pType ) = 0;
	      virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType( AM_MEDIA_TYPE *pType ) = 0;
	      virtual HRESULT STDMETHODCALLTYPE SetBufferSamples( BOOL BufferThem ) = 0;
	      virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer( long *pBufferSize, long *pBuffer ) = 0;
	      virtual HRESULT STDMETHODCALLTYPE GetCurrentSample( IMediaSample **ppSample ) = 0;
	      virtual HRESULT STDMETHODCALLTYPE SetCallback( ISampleGrabberCB *pCallback, long WhichMethodToCallback ) = 0;
      };

      ///////////////////////////////////////////////////////////////////////////////////

      static const IID IID_ISampleGrabber = { 0x6B652FFF, 0x11FE, 0x4fce, { 0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F } };

      ///////////////////////////////////////////////////////////////////////////////////

      static const CLSID CLSID_SampleGrabber = { 0xC1F400A0, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

      ///////////////////////////////////////////////////////////////////////////////////

      static const CLSID CLSID_NullRenderer = { 0xC1F400A4, 0x3F08, 0x11d3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

      ///////////////////////////////////////////////////////////////////////////////////

      static const CLSID CLSID_VideoEffects1Category = { 0xcc7bfb42, 0xf175, 0x11d1, { 0xa3, 0x92, 0x0, 0xe0, 0x29, 0x1f, 0x39, 0x59 } };

      ///////////////////////////////////////////////////////////////////////////////////

      static const CLSID CLSID_VideoEffects2Category = { 0xcc7bfb43, 0xf175, 0x11d1, { 0xa3, 0x92, 0x0, 0xe0, 0x29, 0x1f, 0x39, 0x59 } };

      ///////////////////////////////////////////////////////////////////////////////////

      static const CLSID CLSID_AudioEffects1Category = { 0xcc7bfb44, 0xf175, 0x11d1, { 0xa3, 0x92, 0x0, 0xe0, 0x29, 0x1f, 0x39, 0x59 } };

      ///////////////////////////////////////////////////////////////////////////////////

      static const CLSID CLSID_AudioEffects2Category = { 0xcc7bfb45, 0xf175, 0x11d1, { 0xa3, 0x92, 0x0, 0xe0, 0x29, 0x1f, 0x39, 0x59 } };

      ///////////////////////////////////////////////////////////////////////////////////

    #endif // qedit.h

  #else
    // MinGW specific definitions
    #include <sys/time.h>
    #include <limits.h>
  #endif

  // Socket definitions for Windows
  typedef int   socklen_t;
  #define MSG_NOSIGNAL    0

  // Windows has not basename()
  char *basename(char *srcpath);

  // Workaround for defining ESRCH and EPERM from errno.h
  #include <errno.h>
  //#undef ETIMEDOUT  // errno.h version must be undefined: correct definition will be made in pthread.h

  // Now we can include pthread for Windows
  #include <pthread.h>
  #include <semaphore.h>


#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  /* Unix specific code */
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <pthread.h>
  #include <semaphore.h>
  #include <errno.h>
  #include <ctype.h>
  #include <time.h>
  #include <GL/gl.h>
  #include <GL/glu.h>
  #include <GL/glx.h>
  #include <stdint.h>
  #include <libgen.h>
  #include <stdint.h>
  #include <limits.h>
  #include <sys/socket.h>
  #include <sys/file.h>


  #define HAVE_PROTOTYPES 1
  #include <jconfig.h>
  #define JPEG_LIB_VERSION  80
  #include <jpeglib.h>

  // Windows data types: you MUST match definitions for your platform
  #ifndef BOOL
    typedef int32_t         BOOL; // Boolean variable (should be TRUE or FALSE).
  #endif
  #ifndef CHAR
    typedef char            CHAR; // 8-bit Windows (ANSI) character.
  #endif
  #ifndef BYTE
    typedef uint8_t         BYTE; // Byte (8 bits).
  #endif
  #ifndef WORD
    typedef uint16_t        WORD; // 16-bit unsigned integer. The range is 0 through 65535 decimal.
  #endif
  #ifndef DWORD
    typedef uint32_t        DWORD;  // 32-bit unsigned integer. The range is 0 through 4294967295 decimal.
  #endif
  #ifndef DWORD32
    typedef uint32_t        DWORD32;// 32-bit unsigned integer.
  #endif
  #ifndef LONG
    typedef int32_t         LONG;   // 32-bit signed integer. The range is \962147483648 through 2147483647 decimal.
  #endif
  #ifndef LPVOID
    typedef void*           LPVOID;
  #endif
  #ifndef BOOLEAN
    typedef BYTE            BOOLEAN;  // Boolean variable (should be TRUE or FALSE).
  #endif
  #ifndef COLORREF
    typedef DWORD           COLORREF; // Red, green, blue (RGB) color value (32 bits).
  #endif

  // Windows BITMAP structures for images of formats (.bmp)
  typedef struct tagRGBQUAD {
    BYTE	rgbBlue;
    BYTE	rgbGreen;
    BYTE	rgbRed;
    BYTE	rgbReserved;
  } RGBQUAD,*LPRGBQUAD;

  #pragma pack(push,2)  // Must have an aligment of 2 bytes
  typedef struct tagBITMAPFILEHEADER {
    WORD	bfType;
    DWORD	bfSize;
    WORD	bfReserved1;
    WORD	bfReserved2;
    DWORD	bfOffBits;
  } BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;
  #pragma pack(pop)

  typedef struct tagBITMAPINFOHEADER{
    DWORD	biSize;
    LONG	biWidth;
    LONG	biHeight;
    WORD	biPlanes;
    WORD	biBitCount;
    DWORD	biCompression;
    DWORD	biSizeImage;
    LONG	biXPelsPerMeter;
    LONG	biYPelsPerMeter;
    DWORD	biClrUsed;
    DWORD	biClrImportant;
  } BITMAPINFOHEADER,*LPBITMAPINFOHEADER,*PBITMAPINFOHEADER;

  typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
  } BITMAPINFO,*LPBITMAPINFO,*PBITMAPINFO;

  typedef struct tagBITMAP {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    LPVOID bmBits;
  } BITMAP;

  // Socket definitions
  #define SOCKET          unsigned int
  #define SOCKET_ERROR    -1
  #define INVALID_SOCKET  (SOCKET)(~0)

  // Linux has not strlrw
  char *strlwr (char *a);

#else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif


// Compression types
#ifndef BI_RGB
  #define BI_RGB    0
#endif
#ifndef BI_RLE8
  #define BI_RLE8   1
#endif
#ifndef BI_RLE4
  #define BI_RLE4   2
#endif
#ifndef BI_BITFIELDS
  #define BI_BITFIELDS  3
#endif
#ifndef BI_JPEG
  #define BI_JPEG   4
#endif
#ifndef BI_PNG
  #define BI_PNG    5
#endif

#ifndef DIBWIDTHBYTES
  #define DIBWIDTHBYTES(bi) (DWORD)WIDTHBYTES((DWORD)(bi).biWidth * (DWORD)(bi).biBitCount)
#endif
#ifndef _DIBSIZE
  #define _DIBSIZE(bi) (DIBWIDTHBYTES(bi) * (DWORD)(bi).biHeight)
#endif
#ifndef DIBSIZE
  #define DIBSIZE(bi) ((bi).biHeight < 0 ? (-1)*(_DIBSIZE(bi)) : _DIBSIZE(bi))
#endif
#ifndef WIDTHBYTES
  #define WIDTHBYTES(bits) ((DWORD)(((bits)+31) & (~31)) / 8)
#endif
#ifndef BITMAPWIDTHBYTES
  #define BITMAPWIDTHBYTES(iwidth, ibpp) (DWORD)WIDTHBYTES((DWORD) iwidth * (DWORD) ibpp)
#endif

/////////////////////////////////////////////////////////////////
// Global information
/////////////////////////////////////////////////////////////////
extern volatile uintptr_t gcgallocated;
extern volatile pthread_mutex_t gcgmemoryalloc;
extern gcgPATRICIAMAP *openglcontext;


/////////////////////////////////////////////////////////
// INTERNAL CLASSES
/////////////////////////////////////////////////////////

// Keeps information for a given OpenGL context
class gcgOPENGLCONTEXT : public gcgDATA {
  public:
    // gcgDrawlogo() information
    gcgTEXTURE2D *tex;

    // gcgTEXT class init
    gcgFONT *systemfonts[GCG_SYSTEM_FONTS];
  public:
    gcgOPENGLCONTEXT() {
      // gcgDrawlogo() info init
      this->tex = NULL;

      // gcgTEXT class info init
      memset(this->systemfonts, 0, sizeof(this->systemfonts));
    };


    virtual ~gcgOPENGLCONTEXT() {
      // gcgDrawlogo() info destroy
      SAFE_DELETE(this->tex);

      // gcgTEXT class info destroy
      for(int i = 0; i < GCG_SYSTEM_FONTS; i++) SAFE_DELETE(this->systemfonts[i]);
    };
};

/////////////////////////////////////////////////////////
// INTERNAL INLINE FUNCTIONS
/////////////////////////////////////////////////////////

// Allocators to track memory leaks
static inline void *gcgmalloc(size_t s) {
  void *p = malloc(s);
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  if(p) INCREASE_ALLOCATED((long) _msize(p));
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  if(p) INCREASE_ALLOCATED((long) malloc_usable_size(p));
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
  return p;
}

static inline void gcgfree(void *p) {
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  INCREASE_ALLOCATED(-((long) _msize(p)));
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  INCREASE_ALLOCATED(-((long) malloc_usable_size(p)));
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
  free(p);
}

static inline void *gcgrealloc(void *p, size_t s) {
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  size_t  oldsize = ((p == NULL) ? 0 : _msize(p));
  void *np = realloc(p, s);
  if(np) INCREASE_ALLOCATED(_msize(np) - oldsize);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  long  oldsize = (p == NULL) ? 0 : malloc_usable_size(p);
  void *np = realloc(p, s);
  if(np) INCREASE_ALLOCATED((long) malloc_usable_size(np) - (long) oldsize);
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
  return np;
}


// Time functions
inline static void gcgComputeTimeout(struct timespec *t, long n) {
  long micro;
  // Platform specific code
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  struct timeval now;
  gettimeofday(&now, NULL);
  t->tv_sec = (long) now.tv_sec;
  micro = ((long) now.tv_usec + n);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  t->tv_sec  = (long) now.tv_sec;
  micro = ((long) now.tv_nsec / 1000L) + n; // Nanoseconds to microseconds
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

  if((long) micro >= 1000000L) {
    t->tv_sec += (long) micro / 1000000L;
    micro = (long) micro % 1000000L;
  }

  t->tv_nsec = micro * 1000L; // Microseconds to nanoseconds
}




/////////////////////////////////////////////////////////
// INTERNAL FUNCTIONS
/////////////////////////////////////////////////////////




#ifdef __cplusplus
  }    // All names have C++ style
#endif

#endif
