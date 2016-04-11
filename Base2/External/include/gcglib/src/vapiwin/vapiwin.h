// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the VAPIWIN_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// VAPIWIN_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

//#define VAPIWIN_API __declspec(dllimport)

// This class is exported from the vapiwin.dll
//////////////////////////////////////////////////////////////////////////////
// videoapi: API for video handling in Windows
//////////////////////////////////////////////////////////////////////////////
// Marcelo Bernardes Vieira
// Thales Luis Rodrigues Sabino
//////////////////////////////////////////////////////////////////////////////

#ifndef _GCGVAPI_H
#define _GCGVAPI_H

#define VAPIFUNCION extern"C"
typedef void* VAPIHANDLE;

VAPIFUNCION  VAPIHANDLE vapiNewVideo();
VAPIFUNCION  void vapiFreeVideo(VAPIHANDLE iv);
VAPIFUNCION  int vapiGetCameraName(VAPIHANDLE iv, int ID, char *pCameraName, int maxName);
VAPIFUNCION  int vapiGetNumberOfCameras(VAPIHANDLE iv);

VAPIFUNCION  int vapiOpenCamera(VAPIHANDLE iv, unsigned int ID, unsigned int _width, unsigned int _height, unsigned int _bpp, unsigned int _fps,
							  unsigned int *width, unsigned int *height, unsigned int *bpp, unsigned int *fps, unsigned int *currentFrame,
							  unsigned int *frameSize, unsigned int *id, unsigned int *bitRate);

VAPIFUNCION  int vapiOpenVideoFile(VAPIHANDLE iv, char *file, unsigned int *width, unsigned int *height,
							  unsigned int *bpp, unsigned int *fps, unsigned int *currentFrame, unsigned int *frameSize,
							  unsigned int *id, unsigned int *bitRate);

VAPIFUNCION  void vapiSetCallBackFunction(VAPIHANDLE iv, void (*callback) (unsigned char*, unsigned int));
VAPIFUNCION  int vapiStart(VAPIHANDLE iv);
VAPIFUNCION  int vapiStop(VAPIHANDLE iv);
VAPIFUNCION  int vapiResume(VAPIHANDLE iv);
VAPIFUNCION  int vapiPause(VAPIHANDLE iv);
VAPIFUNCION  int vapiGetStatus(VAPIHANDLE iv);

#endif
