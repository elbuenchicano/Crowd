//////////////////////////////////////////////////////////////////////////////
// vapiwin.cpp : Defines the exported functions for the DLL application.
//////////////////////////////////////////////////////////////////////////////
// Marcelo Bernardes Vieira
// Thales Luis Rodrigues Sabino
//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////   Video API implementation   ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// Platform specific includes and definitions
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)

// Platform specific includes and definitions
#ifdef _MSC_VER
  // Define API exporting
  #define VAPIWIN_API extern"C" __declspec(dllexport)

  #include <winsock2.h>
  #include <windows.h>
  #include <time.h>
  #include <io.h>
  #include <share.h>
  #include <sys/locking.h>

    // Disable annoying warnings
    #pragma warning(disable : 4996) // POSIX named function marked as deprecated
    #pragma warning(disable : 4995) // Function marked as deprecated
    #pragma warning(disable : 4127) // Conditional expression is constant

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

    // Windows has not basename()
    char *basename(char *srcpath) {
      static char buffer[_MAX_FNAME + _MAX_EXT];
      char base[_MAX_FNAME], ext[_MAX_EXT];
      _splitpath(srcpath, NULL, NULL, base, ext);
      sprintf(buffer, "%s%s", base, ext);
      return buffer;
    }

    // Multi-platform includes
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdarg.h>
    #include <math.h>
    #include <fcntl.h>
    #include <malloc.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <new>
    #include "gcg.h"

    // Multiplatform defines
    #ifndef ALLOC
    #define ALLOC(s)  gcgmalloc(s)
    #endif

    #ifndef FREE
    #define FREE(p)   gcgfree(p)
    #endif

    // Multiplatform defines
    #ifndef REALLOC
    #define REALLOC(p, s)  gcgrealloc(p, s)
    #endif


    #ifndef INCREASE_ALLOCATED
    #define INCREASE_ALLOCATED(m) { pthread_mutex_lock(&gcgmemoryalloc); \
                                      gcgallocated += (long) (m); \
                                    pthread_mutex_unlock(&gcgmemoryalloc);}/* \
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

    #include "streams.h"

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


#include "..\..\src\video.cpp"


VAPIWIN_API gcgINTERNAL_VIDEO *vapiNEWINTERNAL_VIDEO(void *gcgvideopointer) {
	return new gcgINTERNAL_VIDEO(gcgvideopointer);
}

VAPIWIN_API void vapiDELETEINTERNAL_VIDEO(gcgINTERNAL_VIDEO *iv) {
	if(iv) delete iv;
}


VAPIWIN_API int vapiDESTROYVIDEOCAPTURE(gcgINTERNAL_VIDEO *iv, int *code, char **msg, char **file, int *line) {
	int res = 0;
	if(iv) {
    res = iv->releaseVideo();
    *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
  }
  return res;
}

VAPIWIN_API int vapiDESTROYVIDEOFILE(gcgINTERNAL_VIDEO *iv, int *code, char **msg, char **file, int *line) {
	int res = 0;
	if(iv) {
    res = iv->releaseVideo();
    *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
  }
  return res;
}

VAPIWIN_API int vapiGETNUMBEROFCAMERAS(gcgINTERNAL_VIDEO *iv, int *code, char **msg, char **file, int *line) {
	int res = 0;
	if(iv != NULL) {
	  res = iv->getNumberOfCameras();
	  *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
	}
	return res;
}

VAPIWIN_API int vapiGETCAMERANAME(gcgINTERNAL_VIDEO *iv, int ID, char *pCameraName, int maxName, int *code, char **msg, char **file, int *line) {
  int res = 0;
	if(iv != NULL) {
	  res = iv->getCameraName(ID, pCameraName, maxName);
	  *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
	}
	return res;
}


VAPIWIN_API int vapiOPENVIDEOFILE(gcgINTERNAL_VIDEO *iv, const char *file, unsigned int *width, unsigned int *height,
                                              unsigned char *bpp, float *fps, unsigned int *bitrate, int *code, char **msg, char **fi, int *line) {
	if(iv == NULL) return false;
	int sc = iv->openVideoFile(file);

  *width = iv->width;
  *height = iv->height;
  *bpp = iv->bpp;
	*fps = iv->fps;
	*bitrate = iv->bitrate;
	*code = iv->code; *msg = iv->msg; *fi = iv->file; *line = iv->line;
	return sc;
}


VAPIWIN_API int vapiOPENCAMERA(gcgINTERNAL_VIDEO *iv, unsigned int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps,
							  unsigned int *width, unsigned int *height, unsigned char *bpp, float *fps, unsigned int *id, unsigned int *bitrate, 
                int *code, char **msg, char **fi, int *line) {

	if(iv == NULL) return false;
	int sc = iv->openCamera(ID, _width, _height, _bpp, _fps);

  *width = iv->width;
  *height = iv->height;
  *bpp = iv->bpp;
	*fps = iv->fps;
	*id = iv->id;
	*bitrate = iv->bitrate;
	*code = iv->code; *msg = iv->msg; *fi = iv->file; *line = iv->line;

	return sc;
}

VAPIWIN_API int vapiSETCALLBACKFUNCTION(gcgINTERNAL_VIDEO *iv, void ( *callback )( void*), int *code, char **msg, char **file, int *line) {
  int res = false;
	if(iv != NULL) {
	  res = iv->setCallBackFunction(callback);
	  *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
	}
  return res;
}

VAPIWIN_API int vapiCOPYFRAMETO(gcgINTERNAL_VIDEO *iv, void **bmpinfo, unsigned char **bmpdata, unsigned int *npalcolors, double *frametime) {
	if(iv != NULL) {
    if(iv->bmpinfo != NULL && iv->bmpdata != NULL) {
	    if(bmpinfo) *bmpinfo = iv->bmpinfo;
	    if(bmpdata) *bmpdata = iv->bmpdata;
	    if(npalcolors) *npalcolors = iv->npalettecolors;
	    if(frametime) *frametime = iv->frametime;
      return true;
    }
	}

  return false;
}

VAPIWIN_API long vapiGETFRAMECOUNT(gcgINTERNAL_VIDEO *iv, int *code, char **msg, char **file, int *line) {
  long res = 0;
  if(iv != NULL) {
    res = iv->getFrameCount();
    *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
  }
  return res;
}

VAPIWIN_API int vapiSETCURRENTPOSITION(gcgINTERNAL_VIDEO *iv, long newposition, int *code, char **msg, char **file, int *line) {
	int res = 0;
	if(iv != NULL) {
	  res = iv->setCurrentPosition(newposition);
	  *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
	}
	return res;
}


VAPIWIN_API long vapiGETCURRENTPOSITION(gcgINTERNAL_VIDEO *iv, int *code, char **msg, char **file, int *line) {
  long res = 0;
  if(iv != NULL) {
    res = iv->getCurrentPosition();
    *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
  }
  return res;
}

VAPIWIN_API int vapiGETCURRENTIMAGEINFOANDADVANCE(gcgINTERNAL_VIDEO *iv, void **bmpinfo, void **bmpdata, void **pal, unsigned int *npalcolors, int *code, char **msg, char **file, int *line) {
  int res = 0;
	if(iv != NULL) {
    res = iv->getCurrentImageInfoAndAdvance(bmpinfo, bmpdata, npalcolors, pal);
    *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
	}

  return res;
}


VAPIWIN_API int vapiSTART(gcgINTERNAL_VIDEO *iv, int *code, char **msg, char **file, int *line) {
	int res = 0;
	if(iv != NULL) {
	  res = iv->start();
	  *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
	}
	return res;
}


VAPIWIN_API int vapiSTOP(gcgINTERNAL_VIDEO *iv, int *code, char **msg, char **file, int *line) {
	int res = 0;
	if(iv != NULL) {
	  res = iv->stop();
	  *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
	}
	return res;
}

VAPIWIN_API int vapiRESUME(gcgINTERNAL_VIDEO *iv, int *code, char **msg, char **file, int *line) {
	int res = 0;
	if(iv != NULL) {
	  res = iv->resume();
	  *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
	}
	return res;
}

VAPIWIN_API int vapiPAUSE(gcgINTERNAL_VIDEO *iv, int *code, char **msg, char **file, int *line) {
	int res = 0;
	if(iv != NULL) {
	  res = iv->pause();
	  *code = iv->code; *msg = iv->msg; *file = iv->file; *line = iv->line;
	}
	return res;
}

VAPIWIN_API int vapiISCAPTURING(gcgINTERNAL_VIDEO *iv) {
  int res = 0;
  if(iv != NULL) {
    res = iv->isCapturing();
  }
  return res;
}


// dllmain.cpp : Defines the entry point for the DLL application.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
	switch (ul_reason_for_call)	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			 break;
	}
	return TRUE;
}



  #else
    #error "vapiwin.dll: supported only by Visual C++ compilers."
  #endif
#else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32 or GCG_WINDOWS64."
#endif
