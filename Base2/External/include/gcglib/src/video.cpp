/*z************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    gcgVIDEO: functions for video handling.

    Thales Luis Rodrigues Sabino
    Marcelo Bernardes Vieira
**************************************************************************************/

#ifndef VAPIWIN_API
#include "system.h"
#endif

// Main definitions for Visual C++
#ifdef _MSC_VER

//////////////////////////////////////////////////////////////////////////////
// Internal video implementation for windows. USE ONLY IN VISUAL C++.
//////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Free the media type without needing any extra library
/////////////////////////////////////////////////////////////////////////////
static inline void freeMediaType(AM_MEDIA_TYPE& mt) {
	if(mt.cbFormat != 0) {
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if(mt.pUnk != NULL) {
		// Unecessary because pUnk should not be used, but safest.
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Compute the number of colors/masks of a bitmap
/////////////////////////////////////////////////////////////////////////////
static inline int numberOfColors(BITMAPINFOHEADER *bmp) {
  if(bmp->biBitCount <= 8) return ((bmp->biClrUsed != 0) ? bmp->biClrUsed : (0x01 << bmp->biBitCount));
  return (bmp->biCompression == BI_BITFIELDS) ? 3 : 0;
}


/////////////////////////////////////////////////////////////////////////////
// Find the n-th capture device
/////////////////////////////////////////////////////////////////////////////
static inline int findDevice(IBaseFilter **ppCap, int dispositivo) {
	HRESULT hr;

	if(!ppCap) return false;

	*ppCap = NULL;

	// Cria um enumerador
	CComPtr< ICreateDevEnum > pCreateDevEnum;
	pCreateDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum);
	if(!pCreateDevEnum ) return false;

	// Enumera dispositivos de captura de video
	CComPtr< IEnumMoniker > pEm;
	pCreateDevEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &pEm, 0);
	if(!pEm) return false;
	pEm->Reset();

	// Rastreia a enumeracao e busca o primeiro dispositivo de captura
	int conta = 0;
	while(TRUE) {
		ULONG ulFetched = 0;
		CComPtr< IMoniker > pM;

		// Busca o proximo
		hr = pEm->Next( 1, &pM, &ulFetched);
		if(hr != S_OK) break;

		// Busca a propriedades de interfaces do moniker
		CComPtr< IPropertyBag > pBag;
		hr = pM->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
		if(hr != S_OK ) continue;

		// Busca pelo nome do dispositivo
		CComVariant var;
		var.vt = VT_BSTR;
		hr = pBag->Read( L"FriendlyName", &var, NULL);
		if(hr != S_OK) continue;

		// LOCAL PARA SELECAO DE DISPOSITIVOS

		// Faz requisicao do filtro
		if(dispositivo == conta) {
			hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**) ppCap);
			if(*ppCap) break;
		}

		VariantClear(&var);
		conta++;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Find a MPEG2 decoder
/////////////////////////////////////////////////////////////////////////////
static inline int findMPEG2Decoder(IBaseFilter **ppFilter) {
	HRESULT hr;

	if(!ppFilter) return false;
	*ppFilter = NULL;

	// Cria um mapeador
	IFilterMapper2 *pMapper = NULL;
	IEnumMoniker *pEnum = NULL;

  hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC, IID_IFilterMapper2, (void **) &pMapper);
  if(FAILED(hr)) return false;

	GUID arrayInTypes[2];
	arrayInTypes[0] = MEDIATYPE_Video;
  arrayInTypes[1] = MEDIASUBTYPE_MPEG2_VIDEO;//GUID_NULL;//MEDIASUBTYPE_DVSD; //dvsd
  GUID arrayOutTypes[2];
  arrayOutTypes[0] = MEDIATYPE_Video;
  arrayOutTypes[1] = GUID_NULL;//MEDIASUBTYPE_YV12;

	// Enumera dispositivos de captura de video
	hr = pMapper->EnumMatchingFilters(&pEnum, 0, // Reserved.
                                    TRUE,//FALSE, // Use exact match?
                                    MERIT_NORMAL, // Minimum normal
                                    TRUE, // At least one input pin?
                                    1, // Number of major type/subtype pairs for input.
                                    arrayInTypes, // Array of major type/subtype pairs for input.
                                    NULL, // Input medium.
                                    NULL, // Input pin category.
                                    FALSE, // Must be a renderer?
                                    TRUE, // At least one output pin?
                                    0, // Number of major type/subtype pairs for output.
                                    arrayOutTypes, // Array of major type/subtype pairs for output.
                                    NULL, // Output medium.
                                    NULL); // Output pin category.
	  // Enumerate the monikers.
  IMoniker *pMoniker;
  ULONG cFetched;
  while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
      IPropertyBag *pPropBag = NULL;
      hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);

      if(SUCCEEDED(hr)) {
          /*
          // To retrieve the friendly name of the filter, do the following:
          VARIANT varName;
          VariantInit(&varName);
          hr = pPropBag->Read(L"FriendlyName", &varName, 0);
          if (SUCCEEDED(hr)) {
            // Display the name in your UI somehow.
		        USES_CONVERSION;
		        //The following 2 lines convert the camera name from wchar_t* to char* and return to pCameraName converted value
		        wchar_t* pWideCameraName = ( wchar_t* ) OLE2T( varName.bstrVal );
		        char pCameraName[200];
 		        _wchar_t2char( pWideCameraName, pCameraName );
             printf("%s\n", pCameraName);
          }
          VariantClear(&varName);
          */

          // To create an instance of the filter, do the following:
          hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**) ppFilter);
          if(SUCCEEDED(hr)) {
            pMoniker->Release();
            pMapper->Release();
            pEnum->Release();
            return true;
          }
          // Release pFilter later.
          (*ppFilter)->Release();

          // Clean up.
          pPropBag->Release();
      }
      pMoniker->Release();
  }
  getchar();
  // Clean up.
  pMapper->Release();
  pEnum->Release();
	return false;
}

// Timeout to wait samples in miliseconds
#define REFERENCE_TIMEOUT 3000


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// This is the COM object that represents a simple rendering filter. It
// supports IBaseFilter and IMediaFilter and a single input stream (pin)
// The classes that support these interfaces have nested scope NOTE the
// nested class objects are passed a pointer to their owning renderer
// when they are created but they should not use it during construction
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class gcgRENDERER : public CBaseVideoRenderer, public IVideoFrameStep, public IKsPropertySet {
  public:
    long      nSteps;
    HANDLE    stepDone;

  public:
    // Constructor and destructor
    gcgRENDERER(LPUNKNOWN pUnk, HRESULT *phr);
    ~gcgRENDERER();

    // Override these from the filter and renderer classes
    HRESULT CheckMediaType(const CMediaType *pmtIn);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);
    void OnReceiveFirstSample(IMediaSample *pMediaSample);
    HRESULT GetSampleTimes(IMediaSample *pMediaSample, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime);
    HRESULT ShouldDrawSampleNow(IMediaSample *pMediaSample, REFERENCE_TIME *ptrStart, REFERENCE_TIME *ptrEnd);
    void OnRenderStart(IMediaSample *pMediaSample) {} // Do not spend time on this
    void OnRenderEnd(IMediaSample *pMediaSample) {} // Do not spend time on this

    // Created here to check step completion
    HRESULT waitStepCompletion();

    // Override IKsPropertySet
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

    // Override IVideoFrameStep
    HRESULT STDMETHODCALLTYPE CancelStep();
    HRESULT STDMETHODCALLTYPE CanStep(long bMultiple, IUnknown *pStepObject){return S_OK;}
    HRESULT STDMETHODCALLTYPE Step(DWORD dwFrames, IUnknown *pStepObject);
    STDMETHODIMP_(ULONG) AddRef(){return 2;} //{return CBaseVideoRenderer::AddRef();}
    STDMETHODIMP_(ULONG) Release(){return 1;} //{return CBaseVideoRenderer::Release();}
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);
};

void gcgRENDERER::OnReceiveFirstSample(IMediaSample *pMediaSample) {
  DoRenderSample(pMediaSample);
}

HRESULT gcgRENDERER::DoRenderSample(IMediaSample *pSample) {
  /*if(m_State == State_Running) printf("DoRenderSample(): Running\n");
  if(m_State == State_Paused) printf("DoRenderSample(): Paused\n");
  if(m_State == State_Stopped) printf("DoRenderSample(): Stopped\n");
  */

  // If stepping multiple samples, skip all but the last
  if(m_State == State_Paused) {
    // Maybe the step was completed
    if(nSteps == 0) SetEvent(stepDone);
    // It works because I observed that the DoRenderSample is called TWICE of a single step: first is running.
    // The second is paused: this is a consequence of EC_STEP_COMPLETE.
  }

  // Do step if running
  if(m_State == State_Running)
    if(nSteps > 1) nSteps--;
    else
        if(nSteps == 1) {
          nSteps = 0;
          NotifyEvent(EC_STEP_COMPLETE, false, 0); // Notify FGM to pause the graph and notify the application. FALSE = completed (not cancelled)
        }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE gcgRENDERER::Step(DWORD dwFrames, IUnknown *pStepObject) {
  /*if(m_State == State_Running) printf("Step(): Running\n");
  if(m_State == State_Paused) printf("Step(): Paused\n");
  if(m_State == State_Stopped) printf("Step(): Stopped\n");
  */

  // Simply set amount of steps... FGM will automatically put the render into running state
  if(dwFrames > 0) {
    ResetEvent(stepDone); // Reset event: it will signal step completion
    nSteps = (long) dwFrames;
  }

  return S_OK;
}


HRESULT STDMETHODCALLTYPE gcgRENDERER::CancelStep() {
  nSteps = 0;
  NotifyEvent(EC_STEP_COMPLETE, true, 0); // Notify FGM to pause the graph and notify the application. true = cancelled
  return S_OK;
}


STDMETHODIMP gcgRENDERER::QueryInterface(REFIID riid, void ** ppv) {
  if(riid == IID_IVideoFrameStep) {
    *ppv = ( void* ) static_cast< IVideoFrameStep* > ( this );
		return NOERROR;
	}

  return CBaseVideoRenderer::QueryInterface(riid, ppv);
}

STDMETHODIMP gcgRENDERER::NonDelegatingQueryInterface(REFIID riid,void **ppv) {
  CheckPointer(ppv, E_POINTER);

  if (riid == IID_IKsPropertySet) {
      *ppv = ( void* ) static_cast< IKsPropertySet* > ( this );
      return NOERROR;
  } else
      if(riid == IID_IVideoFrameStep) {
        *ppv = ( void* ) static_cast< IVideoFrameStep* > ( this );
		    return NOERROR;
      }
  return CBaseVideoRenderer::NonDelegatingQueryInterface(riid,ppv);
}

HRESULT gcgRENDERER::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData) {
  AM_FRAMESTEP_STEP *p;

  if(guidPropSet == AM_KSPROPSETID_FrameStep) {
    switch(dwPropID) {
      case AM_PROPERTY_FRAMESTEP_STEP:  p = (AM_FRAMESTEP_STEP*) pPropData;
                                        return this->Step(p->dwFramesToStep, NULL);
      case AM_PROPERTY_FRAMESTEP_CANCEL: return this->CancelStep();

      // Simply returning ok...
      case AM_PROPERTY_FRAMESTEP_CANSTEP: return this->CanStep(0, NULL);
      case AM_PROPERTY_FRAMESTEP_CANSTEPMULTIPLE: return this->CanStep(1, NULL);
    }
  }
  return E_PROP_ID_UNSUPPORTED;
}

HRESULT gcgRENDERER::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned) {
  return S_FALSE;
}

HRESULT gcgRENDERER::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport) {
  return S_FALSE;
}

gcgRENDERER::gcgRENDERER(LPUNKNOWN pUnk, HRESULT *phr) : CBaseVideoRenderer(CLSID_SampleRenderer, NAME("GCG Renderer"), pUnk, phr),
                                                         IVideoFrameStep(), IKsPropertySet()  {
  this->nSteps = 0;
  // Increment COM reference to avoid premature deletion of this object
  CBaseVideoRenderer::AddRef();
  stepDone = CreateEvent(NULL, FALSE, FALSE, NULL);
}

gcgRENDERER::~gcgRENDERER() {
  CloseHandle(stepDone);
}

HRESULT gcgRENDERER::CheckMediaType(const CMediaType *pmt) {
  // Check formats
  if(*pmt->Type() != MEDIATYPE_Video) return S_FALSE;
  if((*pmt->FormatType() != FORMAT_VideoInfo) && (*pmt->FormatType() != FORMAT_VideoInfo2)) return S_FALSE;

	// Examina o formato
  if(pmt->cbFormat >= sizeof(VIDEOINFOHEADER)) {
			return (*pmt->Subtype() == MEDIASUBTYPE_RGB1)   || (*pmt->Subtype() == MEDIASUBTYPE_RGB4)   ||
             (*pmt->Subtype() == MEDIASUBTYPE_RGB8)   || (*pmt->Subtype() == MEDIASUBTYPE_RGB565) ||
             (*pmt->Subtype() == MEDIASUBTYPE_ARGB32) || (*pmt->Subtype() == MEDIASUBTYPE_RGB24);
  }
  return S_FALSE;
}

HRESULT gcgRENDERER::ShouldDrawSampleNow(IMediaSample *pMediaSample, REFERENCE_TIME *ptrStart, REFERENCE_TIME *ptrEnd) {
    return S_OK; // Force no clock
}

HRESULT gcgRENDERER::GetSampleTimes(IMediaSample *pMediaSample, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime) {
  HRESULT res = CBaseRenderer::GetSampleTimes(pMediaSample, pStartTime, pEndTime);
  *pStartTime = *pEndTime = 0; // Force no time checking
  return res;
}

// EU MESMO CRIEI ESSA FUNCAO
HRESULT gcgRENDERER::waitStepCompletion() {
  if(WaitForSingleObject(stepDone, REFERENCE_TIMEOUT * 5) == WAIT_OBJECT_0) return S_OK;
  return S_FALSE;

/*
          // Wait for EC_STEP_COMPLETE notification
          HANDLE  hEvent;
          long    evCode, param1, param2;
          hr = pEvent->GetEventHandle((OAEVENT*)&hEvent);
          if(SUCCEEDED(hr)) {
            bool done = false;
            int  wcount = 0;
            do {
              if(WAIT_OBJECT_0 == WaitForSingleObject(hEvent, REFERENCE_TIMEOUT)) {
                while(S_OK == pEvent->GetEvent(&evCode, &param1, &param2, 0))  {
                  //printf("Event code: %#04x\n Params: %d, %d\n", evCode, param1, param2);
                  pEvent->FreeEventParams(evCode, param1, param2);
                  done = (EC_COMPLETE == evCode || EC_STEP_COMPLETE == evCode);
                }
              }
              wcount++;
            } while(!done && wcount < 10);
          }
*/
}




/****************************************************************************************
 * The Microsoft® DirectShow® application programming interface (API) is a
 * media-streaming architecture for the Microsoft Windows® platform.
 * Using DirectShow, your applications can perform high-quality video and audio
 * playback or capture.
 *
 * Writing a DirectShow Application
 *
 * In broad terms, there are three tasks that any DirectShow application must perform.
 * These are illustrated in the following diagram:
 *
 *	Typical DirectShow application
 *	    1. The application creates an instance of the Filter Graph Manager.
 *	    2. The application uses the Filter Graph Manager to build a filter graph.
 *         The exact set of filters in the graph will depend on the application.
 *	    3. The application uses the Filter Graph Manager to control the filter graph and
 *         stream data through the filters. Throughout this process, the application will
 *         also respond to events from the Filter Graph Manager.
 *
 *****************************************************************************************/


class gcgINTERNAL_VIDEO : ISampleGrabberCB {
	public:
		// Video information
    unsigned int      width;
    unsigned int      height;
    unsigned char     bpp;
    float             fps;
    unsigned int      bitrate;
    unsigned int      id;

    // Frame information
    BITMAPINFOHEADER  *bmpinfo;
    int               bmpsize; // size of bmpinfo array
    unsigned char     *bmpdata;
    unsigned int      npalettecolors;
    double            frametime;

    // For GetCurrentImage()
    long              lastsize;
    unsigned char     *framedata;

    // For complete error report
    int               code;
    char              *msg;
    char              *file;
    int               line;

	private:
		IGraphBuilder          *pGraph;
		ICaptureGraphBuilder2  *pBuild;
		IBaseFilter            *pCap;
		IEnumMoniker           *pEnum;
		IMediaEvent			       *pEvent;
		ISampleGrabber         *pGrabber;
		IBaseFilter            *pGrabberF;
		IBaseFilter			       *pNulo;
		IMediaControl		       *pControl;
		IMediaSeeking          *pSeek;
		IMediaFilter           *pMediaFilter;
    IVideoFrameStep        *pStep;
    gcgRENDERER            *pRenderer;

		// Private internal information: dependent on platform
		void (*callbackFunc)(void*);
		void *gcgvideopointer;

	public:
		gcgINTERNAL_VIDEO(void*);
		~gcgINTERNAL_VIDEO();

		// Functions for camera control
		int  getNumberOfCameras();
		bool getCameraName( int ID, char* pCameraName, int maxName);
		bool openCamera(int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps);

    //////////////////////////////////////////////////////////////////////////////
    // Functions for assynchronous frame control
    //////////////////////////////////////////////////////////////////////////////
		bool openVideoFile(const char* file);
		long getFrameCount();
		bool setCurrentPosition(long newposition);
    bool nextPosition();
		long getCurrentPosition();
		bool setSpeedRate(double rate);
		bool getCurrentImageInfo(void **info, void **data, unsigned int *npalettecolors, void **pal);
    bool getCurrentImageInfoAndAdvance(void **info, void **data, unsigned int *npalettecolors, void **pal);

    //////////////////////////////////////////////////////////////////////////////
    // Following methods are specific for synchronous capturing and stream control
    //////////////////////////////////////////////////////////////////////////////
		bool start();
		bool stop();
		bool resume();
		bool pause();
		bool isCapturing();
		bool disableTimer(); // Makes the capture run as fast as it cans. Does not use frame time.

		// Functions for frame acquisition
		bool setCallBackFunction(void (*callback) (void*));
		bool releaseVideo();	// Release current video stream

	private:
    HANDLE framereceived;
    HANDLE waitstep;
    bool stepmode;
    bool endofstreamreached;

    int buildBasicFilters();
    int startStandardCamera(int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps);
    int startHDVCamera(int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps);
    // Mpeg2 interfaces
    int connectMPEG2Decoder(IBaseFilter **ppDecoder, IBaseFilter *ppDemux);


    HRESULT _initBasicFilters();
    HRESULT _createSystemDeviceEnumerator();
    void _wchar_t2char( wchar_t* pWide, char* pCivilized );
    LPWSTR _char2wchar_t( char* pChar );

    // COM interface implementation
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface( REFIID riid, void ** ppv );

    STDMETHODIMP SampleCB( double SampleTime, IMediaSample* pSample );
    STDMETHODIMP BufferCB( double dblSampleTime, BYTE *pBufferOri, long lBufferSize );
};


EXTERN_C const IID IID_IMPEG2PIDMap = __uuidof(IMPEG2PIDMap);

// Internal macros
#define SAMPLE_MODE 0
#define BUFFER_MODE 1
#define CALLBACK_MODE SAMPLE_MODE


static inline CComPtr<IPin> GetPin(IBaseFilter *pFilter, LPCOLESTR name) {
  CComPtr<IEnumPins> pEnum;
  CComPtr<IPin>      pPin;

  HRESULT hr = pFilter->EnumPins(&pEnum);
  if(FAILED(hr)) return NULL;

  while(pEnum->Next(1, &pPin, 0) == S_OK) {
    PIN_INFO pinfo;
    pPin->QueryPinInfo(&pinfo);
    BOOL found = !_wcsicmp(name, pinfo.achName);
    if(pinfo.pFilter) pinfo.pFilter->Release();
    if(found) return pPin;
    pPin.Release();
  }

  return NULL;
}

static inline IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir) {
  BOOL bFound = FALSE;
  IEnumPins *pEnum;
  IPin *pPin;
  // Begin by enumerating all the pins on a filter
  HRESULT hr = pFilter->EnumPins(&pEnum);
  if(FAILED(hr)) return NULL;

  // Now look for a pin that matches the direction characteristic.
  // When we've found it, we'll return with it.
  while(pEnum->Next(1, &pPin, 0) == S_OK) {
    PIN_DIRECTION PinDirThis;
    pPin->QueryDirection(&PinDirThis);
    if(true == (bFound = (PinDir == PinDirThis))) break;
    pPin->Release();
  }
  pEnum->Release();

  return (bFound ? pPin : NULL);
}


static inline HRESULT CreateOutputPin(IBaseFilter* pMPEG2Demultiplexer, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps) {
	HRESULT hr;

	// create Video Out pin on MPEG-2 Demultiplexer
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
  mt.formattype = FORMAT_MPEG2Video;

  // Allocate the format block, including space for the sequence header.
  mt.cbFormat = sizeof(MPEG2VIDEOINFO);// + sizeof(SeqHdr);
  mt.pbFormat = (BYTE*)CoTaskMemAlloc(mt.cbFormat);
  if (mt.pbFormat == NULL)
  {
      // Out of memory; return an error code.
  }
  ZeroMemory(mt.pbFormat, mt.cbFormat);

  // Cast the buffer pointer to an MPEG2VIDEOINFO struct.
  MPEG2VIDEOINFO *pMVIH = (MPEG2VIDEOINFO*)mt.pbFormat;

  /*RECT rcSrc = {0, 480, 0, 720};        // Source rectangle.
  pMVIH->hdr.rcSource = rcSrc;
  pMVIH->hdr.dwBitRate = 4000000;       // Bit rate.
  pMVIH->hdr.AvgTimePerFrame = 333667;  // 29.97 fps.
  pMVIH->hdr.dwPictAspectRatioX = 4;    // 4:3 aspect ratio.
  pMVIH->hdr.dwPictAspectRatioY = 3;

  // BITMAPINFOHEADER information.
  pMVIH->hdr.bmiHeader.biSize = 40;*/
  //pMVIH->hdr.AvgTimePerFrame = 99999999;  // 29.97 fps.
  pMVIH->hdr.bmiHeader.biBitCount = _bpp;
  pMVIH->hdr.AvgTimePerFrame = (REFERENCE_TIME) (10000000.0 / _fps);
  pMVIH->hdr.bmiHeader.biWidth = _width;
  pMVIH->hdr.bmiHeader.biHeight = _height;

  pMVIH->dwLevel =  AM_MPEG2Level_High1440;//AM_MPEG2Profile_Main;  // MPEG-2 profile.
  pMVIH->dwProfile = AM_MPEG2Profile_High;//AM_MPEG2Level_Main;  // MPEG-2 level.

	//////////
	CComPtr<IMpeg2Demultiplexer> pMPEG2DemultiplexerInterface;
	hr = pMPEG2Demultiplexer->QueryInterface(IID_IMpeg2Demultiplexer, (void**)&pMPEG2DemultiplexerInterface);

	CComPtr<IPin> outPin;
	hr = pMPEG2DemultiplexerInterface->CreateOutputPin(&mt, L"Video Out", &outPin);

  // Query the pin for IMPEG2PIDMap. This implicitly configures the
  // demux to carry a transport stream.
  IMPEG2PIDMap *pPidMap;
  hr = outPin->QueryInterface(IID_IMPEG2PIDMap, (void**)&pPidMap);
  if (SUCCEEDED(hr)) {
      // Assign PID 0x31 to pin 0. Set the type to "PES payload."
      ULONG Pid = 0x810;
      hr = pPidMap->MapPID(1, &Pid, MEDIA_ELEMENTARY_STREAM);
      pPidMap->Release();
  }
  outPin.Release();
  pMPEG2DemultiplexerInterface.Release();


	return hr;
}

static inline bool CreateFilter(const WCHAR *Name, IBaseFilter **Filter, REFCLSID FilterCategory) {
   HRESULT hr;

   // Create the system device enumerator.
   CComPtr<ICreateDevEnum> devenum;
   hr = devenum.CoCreateInstance(CLSID_SystemDeviceEnum);
   if(FAILED(hr)) return false;

   // Create an enumerator for this category.
   CComPtr<IEnumMoniker> classenum;
   hr = devenum->CreateClassEnumerator(FilterCategory, &classenum, 0);
   if(hr != S_OK) return false;

   // Find the filter that matches the name given.
   CComVariant name(Name);
   CComPtr<IMoniker> moniker;
   while (classenum->Next(1, &moniker, 0) == S_OK) {
      CComPtr<IPropertyBag> properties;
      hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&properties);
      if(FAILED(hr)) {
        devenum.Release();
        return false;
      }

      CComVariant friendlyname;
      hr = properties->Read(L"FriendlyName", &friendlyname, 0);
      if(FAILED(hr)) {
        devenum.Release();
        return false;
      }

      if(name == friendlyname) {
         hr = moniker->BindToObject(0, 0, IID_IBaseFilter, (void **)Filter);
         devenum.Release();
         return SUCCEEDED(hr);
      }

      moniker.Release();
   }
   devenum.Release();

   // Couldn't find a matching filter.
   return false;
}


/***************************************************************************************
 * Constructor
 ***************************************************************************************/
gcgINTERNAL_VIDEO::gcgINTERNAL_VIDEO(void *_gcgvideopointer) {
	pGraph = NULL;
	pBuild = NULL;
	pGrabberF = NULL;
	pGrabber = NULL;
	pCap = NULL;
	pNulo = NULL;
	pEvent = NULL;
	pSeek = NULL;
	pMediaFilter = NULL;
  pControl = NULL;
  pStep = NULL;
  pRenderer = NULL;

	width = 0;
	height = 0;
	bpp = 0;
	fps = 0;
	bitrate = 0;
	id = 0;
	bmpinfo = NULL;
	bmpdata = NULL;
	bmpsize = 0;
	lastsize = 0;
	framedata = NULL;
	npalettecolors = 0;
  framereceived = NULL;
  waitstep = NULL;
  stepmode = false;
	endofstreamreached = false;
	this->callbackFunc = NULL;
  this->gcgvideopointer = NULL;

  // Initialize the COM Library
	HRESULT hr = CoInitialize(NULL);
  if(FAILED(hr)) return;

	framereceived = CreateEvent(NULL, FALSE, FALSE, NULL);
  waitstep = CreateEvent(NULL, FALSE, FALSE, NULL);
	this->gcgvideopointer = _gcgvideopointer;
}

gcgINTERNAL_VIDEO::~gcgINTERNAL_VIDEO() {
	//CoUninitialize();
	releaseVideo();
	CloseHandle(framereceived);
  CloseHandle(waitstep);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Public Methods   ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
* Return the number of active cameras
***************************************************************************************/
int gcgINTERNAL_VIDEO::getNumberOfCameras() {
	HRESULT hr;

	hr = this->_createSystemDeviceEnumerator();

	if(FAILED(hr)) {
    // If the system could not create the system device enumerator
    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_UNSUPPORTEDOPERATION);
    this->msg  = "getNumberOfCameras(): could not get system device enumerator. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
    return 0;
  }

	IMoniker *pMoniker = NULL;
	if(this->pEnum == NULL) {
    // No devices found.
    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_VIDEO, GCG_UNAVAILABLERESOURCE);
    this->msg  = "getNumberOfCameras(): no capture devices found. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
    return 0;
  }

	//Iterates over the class enumerator for video input device category and return
	//how many actives cameras are.
	int numberOfCameras = 0;
	while(this->pEnum->Next(1, &pMoniker, NULL) == S_OK) {
		IPropertyBag* pPropBag;
		hr = pMoniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pPropBag);

		if(FAILED(hr)) {
			pMoniker->Release();
			continue;	//Skip this one, maybe the next one will work
		}
		numberOfCameras++;
	}

	pEnum->Release();
	pEnum = NULL;

  if(numberOfCameras == 0) {
    // No devices found.
    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_VIDEO, GCG_UNAVAILABLERESOURCE);
    this->msg  = "getNumberOfCameras(): no capture devices found. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
  }

	return numberOfCameras;
}


/***************************************************************************************
* Return the camera name given an ID
*
* IN    @param ID 		- The device identifier. Could be greater then 0
* OUT   @param pCameraName  - The char* to receive the device friendly name
* IN    @param maxName	- The numbers of caracters to be allocated
***************************************************************************************/
bool gcgINTERNAL_VIDEO::getCameraName(int ID, char* pCameraName, int maxName) {
	HRESULT hr;

	VARIANT varName;
	VariantInit( &varName );

	hr = this->_createSystemDeviceEnumerator();
	if(FAILED(hr)) {
    // If the system could not create the system device enumerator
    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_UNSUPPORTEDOPERATION);
    this->msg  = "getCameraName(): could not get system device enumerator. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
    return false;
  }

	IMoniker* pMoniker = NULL;
	IPropertyBag* pPropBag = NULL;

	// Iterates over the class enumerator for video input device category and return the device moniker.
	// As pEnum is a linked list, we need to go through the list until element with number ID is reached.
	for( int i = 0; i < ID ; i++ )
		if( this->pEnum->Next( 1, &pMoniker, NULL ) == S_OK )
			hr = pMoniker->BindToStorage( 0, 0, IID_IPropertyBag, ( void** ) &pPropBag );

	hr = pPropBag->Read( L"FriendlyName", &varName, 0 );

	if(SUCCEEDED(hr)) {
		//Return the camera name to parameter cameraName
		USES_CONVERSION;

		//The following 2 lines convert the camera name from wchar_t* to char* and return to pCameraName converted value
		wchar_t* pWideCameraName = ( wchar_t* ) OLE2T( varName.bstrVal );
		_wchar_t2char( pWideCameraName, pCameraName );
	}

	VariantClear(&varName); //Free the variant

  pPropBag->Release();
	pMoniker->Release();
	this->pEnum->Release();
	pEnum = NULL;

	if(FAILED(hr)) {
    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_UNSUPPORTEDOPERATION);
    this->msg  = "getCameraName(: error reading device name. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
    return false;
  }

	return true;
}


/***************************************************************************************
 Register graph to be analysed by graphedt.exe utility
***************************************************************************************/

HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) {
  IMoniker * pMoniker = NULL;
  IRunningObjectTable *pROT = NULL;

  if(FAILED(GetRunningObjectTable(0, &pROT))) return E_FAIL;

  const size_t STRING_LENGTH = 256;
  WCHAR wsz[STRING_LENGTH];

  StringCchPrintfW(wsz, STRING_LENGTH, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());

  HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
  if(SUCCEEDED(hr)) {
    hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, pMoniker, pdwRegister);
    pMoniker->Release();
  }
  pROT->Release();
  return hr;
}




/***************************************************************************************
 * Method that add a reader filter to the filter graph. The reader filter is able to
 * read and pass the data trought the filter graph
 *
 * IN @param file - A pointer to file name. For now, the file name must be an AVI file.
***************************************************************************************/
bool gcgINTERNAL_VIDEO::openVideoFile(const char *file) {
	HRESULT hr;

	// Libera dispositivo anterior
	releaseVideo();

	// Cria os filtros basicos
	if(!buildBasicFilters()) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openVideoFile(): could not build basic filters. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

  WCHAR *p = _char2wchar_t((char*) file);
	hr = pGraph->AddSourceFilter(p, L"Source", &pCap);
	delete p;
	if(S_OK != hr) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_IOERROR);
    this->msg  = "openVideoFile(): file not found or unavailable decoder. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

  pCap->SetSyncSource(NULL);

	// Cria o renderer nulo
	hr = ::CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
									IID_IBaseFilter, (LPVOID*) &pNulo);
	if(S_OK != hr) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openVideoFile(): could not create NULL renderer. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

	// Adiciona o renderer nulo
	hr = pGraph->AddFilter(pNulo, L"NULL renderer");
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openVideoFile(): NULL renderer was not added. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

  pNulo->SetSyncSource(NULL);

	// Especifica o tipo de midia aceito pelo programa
	AM_MEDIA_TYPE mt;
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_RGB24;
	hr = pGrabber->SetMediaType(&mt);
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openVideoFile(): could not setup media information. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

  // Create GCG render filter
  pRenderer = new gcgRENDERER(NULL, &hr);
  if(pRenderer) {
    // make the initial refcount 1 to match COM creation
    pRenderer->AddRef();

    pRenderer->SetSyncSource(NULL);

    // add to graph -- nb need to Query properly for the
    // right interface before giving that to the graph object
    IBaseFilter* pRendererFilter;
    hr = pRenderer->QueryInterface(IID_IBaseFilter, (void**)&pRendererFilter);
    if(SUCCEEDED(hr)) {
  	  hr = pGraph->AddFilter(pRendererFilter, L"GCG renderer");
	    pRendererFilter->Release();
    }
    // Check both calls above
    if(FAILED(hr)) SAFE_DELETE(pRenderer);
  }

  hr = pBuild->RenderStream(NULL, NULL, pCap, pGrabberF, (pRenderer == NULL) ? pNulo : pRenderer);
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openVideoFile(): error building the graph. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

	hr = pGrabber->GetConnectedMediaType(&mt);
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_FORMATMISMATCH);
    this->msg  = "openVideoFile(): could not retrieve the media information. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

	if((mt.majortype != MEDIATYPE_Video) || (mt.formattype != FORMAT_VideoInfo) ||
		(mt.cbFormat < sizeof(VIDEOINFOHEADER)) || (mt.pbFormat == NULL)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_FORMATMISMATCH);
    this->msg  = "openVideoFile(): graph built with invalid format. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

	VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*) mt.pbFormat;

	// Copy image information
	this->width = vih->bmiHeader.biWidth;
	this->height = vih->bmiHeader.biHeight;
	this->bpp = (unsigned char) vih->bmiHeader.biBitCount;
	if(vih->AvgTimePerFrame > 0) this->fps = (float) (10000000.0 / (double) vih->AvgTimePerFrame);
	else this->fps = 0.0;
	this->frametime = 0.0;

  // Create space for bitmap information
	npalettecolors = numberOfColors(&vih->bmiHeader);
	int bmpsize = vih->bmiHeader.biSize + sizeof(RGBQUAD) * npalettecolors;
	if(bmpinfo) delete bmpinfo;
	bmpinfo = (BITMAPINFOHEADER*) (new unsigned char[bmpsize]);
	if(bmpinfo == NULL) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_FORMATMISMATCH);
    this->msg  = "openVideoFile(): invalid bitmap information. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}
	memcpy(bmpinfo, &vih->bmiHeader, bmpsize);

	// Libera mediatype
	freeMediaType(mt);

	// Gets a seeking object
	pGraph->QueryInterface(IID_IMediaSeeking, (void**)&(this->pSeek));
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openVideoFile(): could not retrieve the seeker. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

  // set time format to video frames
  pSeek->SetTimeFormat(&TIME_FORMAT_FRAME);

	// Recupera o ponteiro de controle de eventos
	hr = pGraph->QueryInterface(IID_IMediaEventEx, (void **) &pEvent);
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openVideoFile(): invalid event control handler. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

	// Capturing control
	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openVideoFile(): could not get media controller. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

  // Registra o objeto que recebera callback
  hr = pGrabber->SetCallback(this, CALLBACK_MODE);
  if(FAILED(hr)) {
    releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openVideoFile(): error setting object callback. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
    return false;
  }

  // Step
  hr = pGraph->QueryInterface(IID_IVideoFrameStep, (void**)&(pStep));
  if(SUCCEEDED(hr)) {
    if(pStep->CanStep(0L, NULL) != S_OK) {
      pStep->Release();
      pStep = NULL;
    }
  }

  // Define forma de captura
  pGrabber->SetBufferSamples(TRUE); // Guarda amostras
  pGrabber->SetOneShot(FALSE); // Uma amostra mas uma de cada vez através de Pause()
  pMediaFilter->SetSyncSource(NULL);
  setCurrentPosition(0);
	return true;
}


bool gcgINTERNAL_VIDEO::openCamera(int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps) {
	HRESULT hr;
	AM_MEDIA_TYPE mt;

	// Libera dispositivo anterior
	releaseVideo();

	// Cria os filtros basicos
	if(!buildBasicFilters()) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openCamera(): could not build basic filters. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

	if(!startStandardCamera(ID, _width, _height, _bpp, _fps)) {
   if(!startHDVCamera(ID, _width, _height, _bpp, _fps)) {
      releaseVideo();
      // Setup error information
      this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
      this->msg  = "openCamera(): error trying to start a HDV device. (%s:%d)";
      this->file = basename((char*)__FILE__);
      this->line = __LINE__;
      return false;
    }

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openCamera(): device could not be added to the graph. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
    return false;
	}

	hr = pGrabber->GetConnectedMediaType(&mt);
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INFORMATIONPROBLEM);
    this->msg  = "openCamera(): could not retrieve media type. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

  if((mt.majortype != MEDIATYPE_Video) || (mt.formattype != FORMAT_VideoInfo) || (mt.cbFormat < sizeof(VIDEOINFOHEADER)) || (mt.pbFormat == NULL)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_FORMATMISMATCH);
    this->msg  = "openCamera(): invalid graph format. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
  }

	VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*) mt.pbFormat;

	// Atualiza informacoes no objeto de callback
	if(vih->bmiHeader.biWidth != (int) _width || vih->bmiHeader.biHeight != (int) _height) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openCamera(): device cannot provide the image size. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

	// Copy image information
	this->width = vih->bmiHeader.biWidth;
	this->height = vih->bmiHeader.biHeight;
	this->bpp = (unsigned char) vih->bmiHeader.biBitCount;
	this->fps = (float) ((double) 10000000.0 / (double) vih->AvgTimePerFrame);
	this->frametime = 0.0;

  // Create space for bitmap information
	npalettecolors = numberOfColors(&vih->bmiHeader);
	int bmpsize = vih->bmiHeader.biSize + sizeof(RGBQUAD) * npalettecolors;
	if(bmpinfo) delete bmpinfo;
	bmpinfo = (BITMAPINFOHEADER*) (new unsigned char[bmpsize]);
	if(bmpinfo == NULL) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_UNSUPPORTEDFORMAT);
    this->msg  = "openCamera(): device cannot provide the image size. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}
	memcpy(bmpinfo, &vih->bmiHeader, bmpsize);

	// Free media type
	freeMediaType(mt);

	// Configura o callback
	// Recupera o ponteiro de controle de eventos
	hr = pGraph->QueryInterface(IID_IMediaEventEx, (void **) &pEvent);
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openCamera(): could not get event control. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

	// Capturing control
	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
	if(FAILED(hr)) {
		releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openCamera(): could not get media control. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
		return false;
	}

  // Registra o objeto que recebera callback
  hr = pGrabber->SetCallback(this, CALLBACK_MODE);
  if(FAILED(hr)) {
    releaseVideo();

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INITERROR);
    this->msg  = "openCamera(): could not set object callback. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
    return false;
  }

	// Now the capture stream is ready to go
	return true;
}


//////////////////////////////////////////////////////////////////////////////////////
// Free the current video stream.
/////////////////////////////////////////////////////////////////////////////////////
bool gcgINTERNAL_VIDEO::releaseVideo() {
	if(pControl) {
	  pControl->StopWhenReady();
	  pControl->Release();
	 }

	if(pEvent)		pEvent->Release();
	if(pNulo)     pNulo->Release();
	if(pCap)      pCap->Release();
	if(pGrabber)	pGrabber->Release();
	if(pGrabberF)	pGrabberF->Release();
	if(pBuild)		pBuild->Release();
	if(pGraph)		pGraph->Release();
	if(pSeek)     pSeek->Release();
	if(pMediaFilter) pMediaFilter->Release();
  if(pStep)     pStep->Release();
  if(pRenderer) SAFE_DELETE(pRenderer);

	pBuild = NULL;
	pGraph = NULL;
	pGrabberF = NULL;
	pGrabber = NULL;
	pCap = NULL;
	pNulo = NULL;
	pEvent = NULL;
	pSeek = NULL;
	pMediaFilter = NULL;
  pStep = NULL;
	pControl = NULL;

	if(bmpinfo) delete bmpinfo;
	if(framedata) delete framedata;
	width = 0;
	height = 0;
	bpp = 0;
	fps = 0;
	bitrate = 0;
	id = 0;
	bmpinfo = NULL;
	bmpdata = NULL;
	bmpsize = 0;
  lastsize = 0;
	framedata = NULL;
	npalettecolors = 0;
  return true;
}



/***************************************************************************************
 * This method set the callback function that will be used to process each frame of
 * the video stream.
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::setCallBackFunction(void (*callback) (void*)) {
	this->callbackFunc = callback;
  return true;
}

/***************************************************************************************
 * Get the number of frames of the video
 ***************************************************************************************/
long gcgINTERNAL_VIDEO::getFrameCount() {
	if(pSeek) {
	  LONGLONG du;
	  pSeek->GetDuration(&du);
		return (long) du;
	}

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "getFrameCount(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;

	return 0;
}

/***************************************************************************************
 * Set the current position of a video
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::setCurrentPosition(long framepos) {
	if(pControl && pGraph && pSeek && pGrabber && pMediaFilter) {
    OAFilterState state;

    LONGLONG pos = framepos;
    HRESULT hr = pSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning);

    if(SUCCEEDED(hr)) {
      if(framepos != (long) pos) {
        LONGLONG du;
        // Error setting the position: check what happens
        hr = pSeek->GetDuration(&du);
        if(SUCCEEDED(hr) && (framepos < 0 || framepos >= (long) du)) {
          // Setup error information
          this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_OUTOFBOUNDS);
          this->msg  = "setCurrentPosition(): frame position is out of bounds. (%s:%d)";
          this->file = basename((char*)__FILE__);
          this->line = __LINE__;
          return false;
        }

        // Setup error information
        this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_OPERATIONFAILED);
        this->msg  = "setCurrentPosition(): position was not set. (%s:%d)";
        this->file = basename((char*)__FILE__);
        this->line = __LINE__;
        return false;
      }

      endofstreamreached = false;

      // Pause the graph to tell DirectShow to update the display.
      // Ensure that the graph has finished pausing before continuing.
	    pControl->GetState(0, &state); // get new state
	    if(state != State_Running) {
        if(state != State_Paused)
          if(pControl->Pause() == S_FALSE)
            if(FAILED(pControl->GetState(REFERENCE_TIMEOUT * 5, &state))) {
              // Setup error information
              this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_STREAMCONTROLERROR);
              this->msg  = "setCurrentPosition(): error going into a paused state. (%s:%d)";
              this->file = basename((char*)__FILE__);
              this->line = __LINE__;
              return false;
            }

        // Waits until the sample comes when paused
        if(WaitForSingleObject(framereceived, REFERENCE_TIMEOUT) != WAIT_OBJECT_0) {
          // Setup error information
          this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_STREAMCONTROLERROR);
          this->msg  = "setCurrentPosition(): error waiting frame. (%s:%d)";
          this->file = basename((char*)__FILE__);
          this->line = __LINE__;
          return false;
        }
      } // otherwise we got an on-the-fly positioning
		  return true;
		}

    // Setup error information
    this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_STREAMCONTROLERROR);
    this->msg  = "setCurrentPosition(): could not set position. (%s:%d)";
    this->file = basename((char*)__FILE__);
    this->line = __LINE__;
    return false;
	}

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "setCurrentPosition(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
	return false;
}


/***************************************************************************************
 * Get the current frame position in the video
 ***************************************************************************************/
long gcgINTERNAL_VIDEO::getCurrentPosition() {
	if(pSeek) {
	  LONGLONG du;
	  HRESULT hr = pSeek->GetCurrentPosition(&du);
	  if(FAILED(hr)) {
      // Setup error information
      this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_STREAMCONTROLERROR);
      this->msg  = "getCurrentPosition(): error getting position. (%s:%d)";
      this->file = basename((char*)__FILE__);
      this->line = __LINE__;
      return -1;
		}
		return (long) du;
	}

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "getCurrentPosition(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
	return -1;
}

/***************************************************************************************
 * Set the rate of frame decoding
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::setSpeedRate(double rate) {
	if(pSeek) {
	  pSeek->SetRate(rate);
	  return true;
	}

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "setSpeedRate(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
	return false;
}


/***************************************************************************************
 * Gets the current image in assynchronous mode
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::getCurrentImageInfo(void **info, void **data, unsigned int *npalette, void **pal) {
  if(pControl && pGraph && pGrabber) {
    if(endofstreamreached) {
      // Setup error information
      this->code = GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_VIDEO, GCG_ENDOFSTREAM);
      this->msg  = "getCurrentImageInfo(): end of stream reached. (%s:%d)";
      this->file = basename((char*)__FILE__);
      this->line = __LINE__;
      return false;
    }

		HRESULT hr;
    long bufsize;
    int i = 0;
    do {
      hr = pGrabber->GetCurrentBuffer(&bufsize, NULL);
      if(FAILED(hr)) Sleep(1); // Wait a little bit if an error occurs
      i++;
    } while(hr == VFW_E_WRONG_STATE && i < 10); // try 10 times while the graph is not ready

    if(SUCCEEDED(hr)) {
      AM_MEDIA_TYPE mt;
      ZeroMemory(&mt, sizeof(mt));

      // Gets image bits
      if(lastsize < bufsize) {
        if(framedata != NULL) delete framedata;
        framedata = new unsigned char[bufsize];
        lastsize = bufsize;
      }
      hr = pGrabber->GetCurrentBuffer(&bufsize, (long*) framedata);
      if(FAILED(hr)) {
        // Setup error information
        this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_STREAMCONTROLERROR);
        this->msg  = "getCurrentImageInfo(): failed to get the current buffer. (%s:%d)";
        this->file = basename((char*)__FILE__);
        this->line = __LINE__;
        return false;
      }

      // Gets the image
      hr = pGrabber->GetConnectedMediaType(&mt);
      if(FAILED(hr)) {
        // Setup error information
        this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_DECODEERROR);
        this->msg  = "getCurrentImageInfo(): could not get the image. (%s:%d)";
        this->file = basename((char*)__FILE__);
        this->line = __LINE__;
        return false;
      }

      // Examine the format block.
      if((mt.formattype == FORMAT_VideoInfo) && (mt.cbFormat >= sizeof(VIDEOINFOHEADER)) && (mt.pbFormat != NULL)) {
          VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*) mt.pbFormat;

          // Update image information
          width = vih->bmiHeader.biWidth;
          height = vih->bmiHeader.biHeight;
          bpp = (unsigned char) vih->bmiHeader.biBitCount;
          fps = (float) (10000000.0 / (double) vih->AvgTimePerFrame);

	        // Create space for bitmap information
	        int ncolors = numberOfColors(&vih->bmiHeader);
	        int newbmpsize = vih->bmiHeader.biSize + sizeof(RGBQUAD) * ncolors;
	        if(newbmpsize > bmpsize) {
	          if(bmpinfo) delete bmpinfo;
	          bmpinfo = (BITMAPINFOHEADER*) (new unsigned char[newbmpsize]);
	          if(bmpinfo == NULL) {
	            freeMediaType(mt);
		          releaseVideo();

              // Setup error information
              this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR);
              this->msg  = "getCurrentImageInfo(): bitmap allocation error. (%s:%d)";
              this->file = basename((char*)__FILE__);
              this->line = __LINE__;
		          return false;
	          }
	          bmpsize = newbmpsize;
	        }
          memcpy(bmpinfo, &vih->bmiHeader, bmpsize);
			    npalettecolors = numberOfColors(bmpinfo);

			    // Output information
			    *info = (void*) bmpinfo;
			    *data = (void*) framedata;
			    *npalette = npalettecolors;
			    *pal = (void*) ((unsigned char*) bmpinfo + bmpinfo->biSize);
		      return true;
      }

      freeMediaType(mt);

      // Setup error information
      this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_FORMATMISMATCH);
      this->msg  = "getCurrentImageInfo(): incompatible video format. (%s:%d)";
      this->file = basename((char*)__FILE__);
      this->line = __LINE__;
      return false;
    }
	}

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "getCurrentImageInfo(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
  return false;
}


/***************************************************************************************
 * Gets the current image in assynchronous mode
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::getCurrentImageInfoAndAdvance(void **info, void **data, unsigned int *npalette, void **pal) {
  if(pGraph && pGrabber && pSeek) {
    OAFilterState state;

    // Get current image
    if(!getCurrentImageInfo(info, data, npalette, pal)) return false;

    // Get current position
	  LONGLONG pos;
	  HRESULT hr = pSeek->GetCurrentPosition(&pos);
	  if(FAILED(hr)) {
      // Setup error information
      this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_STREAMCONTROLERROR);
      this->msg  = "getCurrentImageInfoAndAdvance(): error getting position. (%s:%d)";
      this->file = basename((char*)__FILE__);
      this->line = __LINE__;
      return false;
		}

    // Advance
    if(++pos >= getFrameCount()) {
      pControl->Stop(); // Force the graph to stop
	    pControl->GetState(REFERENCE_TIMEOUT * 5, &state); // get new state
      endofstreamreached = true;
    } else {
        // 1st method: use step interface. It does not work with all graphs.
        if(pStep != NULL) {
	        hr = pStep->Step(1, NULL);
          if(pRenderer != NULL) // SE FOR RENDERER DO GCG: precisa aguardar o fim do step
            if(SUCCEEDED(pRenderer->waitStepCompletion())) {} // EU CRIEI ESSA FUNCAO...

          // Make sure the graph is stable for GetCurrentPosition(): IT SHOULD NOT BE REMOVED FROM HERE
          pControl->GetState(REFERENCE_TIMEOUT * 5, &state);

          // Check if the step REALLY worked
	        LONGLONG newpos;
	        if(SUCCEEDED(pSeek->GetCurrentPosition(&newpos)))
            if(newpos == pos) return true; // Returns true only if it has correctly positioned frame

          // Otherwise the slower setCurrentPosition() will be used
        }

/*        // 2nd method: using Run().
        hr = pSeek->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, &pos, AM_SEEKING_AbsolutePositioning);
        if(SUCCEEDED(hr)) {
          // Run the graph and wait for completion.
          hr = pControl->Run();
          if(SUCCEEDED(hr)) {
            long l_eventCode;
            hr = pEvent->WaitForCompletion(INFINITE, &l_eventCode);
            if(SUCCEEDED(hr)) return true;
          }
        }
*/
        // 3rd method: force positioning. It might be slow.
        if(!setCurrentPosition((long) pos)) return false;
    }
    return true;
  }

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "getCurrentImageInfoAndAdvance(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
  return false;
}

/***************************************************************************************
 * Detects if the frames are being captured
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::isCapturing() {
	if(pGraph && pControl) {
    OAFilterState state;
    pControl->GetState(100, &state); // get new state
		if(state == State_Running) return true;
		return false;
	}

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "isCapturing(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
	return false;
}

/***************************************************************************************
 * Start the video stream trought the graph
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::start() {
	if(pGraph && pGrabber) {
	  HRESULT hr;

    // Define forma de captura
    hr = pGrabber->SetBufferSamples(FALSE); // Guarda amostras
    if(FAILED(hr)) {
      releaseVideo();

      // Setup error information
      this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_CONFIGERROR);
      this->msg  = "start(): error defining the capture mode. (%s:%d)";
      this->file = basename((char*)__FILE__);
      this->line = __LINE__;
      return false;
    }

    hr = pGrabber->SetOneShot(FALSE); // Varias amostras
    if(FAILED(hr)) {
      releaseVideo();

      // Setup error information
      this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_CONFIGERROR);
      this->msg  = "start(): error setting one shot mode. (%s:%d)";
      this->file = basename((char*)__FILE__);
      this->line = __LINE__;
      return false;
    }

    if(pControl) {
      hr = pControl->Run();
      if(FAILED(hr)) {
        releaseVideo();

        // Setup error information
        this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_STARTERROR);
        this->msg  = "start(): error starting video. (%s:%d)";
        this->file = basename((char*)__FILE__);
        this->line = __LINE__;
        return false;
      }
      return true;
    }
  }

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "start(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
	return false;
}

/***************************************************************************************
 * Stop the video stream trought the graph
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::stop() {
	if(pGraph && pControl) {
	  pControl->StopWhenReady();
		pControl->Stop();
		return true;
	}

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "stop(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
	return false;
}

/***************************************************************************************
 * Resume the video stream trought the graph
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::resume() {
	if(pGraph) {
		pControl->Run();
		return true;
	}

  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "resume(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
	return false;
}


/***************************************************************************************
 * Pause the video stream trought the graph
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::pause() {
	if(pGraph) {
		pControl->Pause();
		return true;
	}

	  // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "pause(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
	return false;
}

/***************************************************************************************
 *  Makes the capture run as fast as it cans. Does not use frame time.
 ***************************************************************************************/
bool gcgINTERNAL_VIDEO::disableTimer() {
  if(pGraph && pMediaFilter) {
    pMediaFilter->SetSyncSource(NULL);
    return true;
  }

    // Setup error information
  this->code = GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_INVALIDOBJECT);
  this->msg  = "disableTimer(): invalid object or uninitialized video. (%s:%d)";
  this->file = basename((char*)__FILE__);
  this->line = __LINE__;
  return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Private Methods   ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/***************************************************************************************
 * A filter graph that performs video or audio capture is called a capture graph.
 * Capture graphs are often more complicated than file playback graphs.
 * To make it easier for applications to build capture graphs, DirectShow provides
 * a helper object called the Capture Graph Builder. The Capture Graph Builder exposes
 * the ICaptureGraphBuilder2 interface, which contains methods for building and
 * controlling a capture graph.
 ***************************************************************************************/
/////////////////////////////////////////////////////////////////////////////
// This function create basic filters.
/////////////////////////////////////////////////////////////////////////////
int gcgINTERNAL_VIDEO::buildBasicFilters() {
	// Cria o objeto Capture Graph Builder.
	HRESULT hr =  CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&pBuild);
	if(FAILED(hr)) return false;

	// Cria o objeto Filter Graph Manager.
	hr = CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);
	if(FAILED(hr)) return false;

  // GRAPHEDT DEBUG
  //DWORD dwRegister;
  //hr = AddToRot(pGraph, &dwRegister);

  // Inicializa o objeto Capture Graph Builder (conecta ao Filter Graph Manager).
  hr = pBuild->SetFiltergraph(pGraph);
  if(FAILED(hr)) return false;

  // Cria o objeto sample grabber
  hr = ::CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
						  IID_IBaseFilter, (LPVOID*) &pGrabberF);
  if(FAILED(hr)) return false;

  // Adiciona o filtro base de amostragem no Filter Graph Manager
  hr = pGraph->AddFilter(pGrabberF, L"Sample Grabber");
  if(FAILED(hr)) return false;

  // Busca a interface ISampleGrabber
  hr = pGrabberF->QueryInterface(IID_ISampleGrabber, (void**)&pGrabber);
  if(FAILED(hr)) return false;

  // Configura o relogio do Filter Graph Manager.
  hr = pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
  if(FAILED(hr)) return false;
  //pMediaFilter->SetSyncSource(NULL); // Configura o grafico para processar o graph o mais rapido possivel

  // Neste ponto todos os filtros foram criados e configurados
  return true;
}


/***************************************************************************************
 * The System Device Enumerator provides a uniform way to enumerate, by category,
 * the filters registered on a user's system.
 * The System Device Enumerator works by creating an enumerator for a specific category,
 * such as audio capture or video compression.
 ***************************************************************************************/
HRESULT gcgINTERNAL_VIDEO::_createSystemDeviceEnumerator() {
	IEnumMoniker* pEnumTemp = NULL;

	// Create the System Device Enumerator.
	HRESULT hr;
	ICreateDevEnum* pDevEnum = NULL;
	hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, reinterpret_cast< void** > ( &pDevEnum ) );

	if( SUCCEEDED( hr ) ) {
		//Create an enumerator for the video input device category.
		//This step return to function parameter ppEnum the class enumerator for Video Input Device Category
		hr = pDevEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &pEnumTemp, 0 );
		pDevEnum->Release();
		if( FAILED( hr ) ) return hr;
	}

  pDevEnum->Release();
	this->pEnum = pEnumTemp;
	return hr;
}



/***************************************************************************************
* Convert wchar_t* to char*.
* The ANSI C defined wide character type, usually implemented as either 16 or 32 bits.
* ANSI specifies that wchar_t be an integral type and that the C language source character
* set be mappable by simple extension (zero- or sign-extension).
*
* IN 	 @param pWide - A pointer to the wchar_t to be converted
* OUT    @param pCivilized - A pointer to receive the converted value
***************************************************************************************/
void gcgINTERNAL_VIDEO::_wchar_t2char( wchar_t* pWide, char* pCivilized ) {
	size_t wideSize = wcslen( pWide ) + 1; //Getting the wchar_t length
	const size_t newSize = 100;			   //Setting the max size
	size_t convertedChars = 0;			   //Number of converted chars

	char newChar[ newSize ];			   //Create the new char[]
	wcstombs_s( &convertedChars, newChar, wideSize, pWide, _TRUNCATE ); //Now we convert the wchar_t* to char*
	strcpy_s(pCivilized, _countof(newChar), newChar);							    //Now we return the converted wchar_t
}

/***************************************************************************************
* Convert char* to wchar_t*
*
* IN 	 @param - A pointer to the char* to be converted
* OUT    @return LPWSTR - The char converted to a wchar_t.
***************************************************************************************/
LPWSTR gcgINTERNAL_VIDEO::_char2wchar_t ( char* pChar ) {
    LPWSTR pszOut = NULL;

    if(pChar != NULL) {
  		int nInputStrLen = (int) strlen(pChar);
	  	int nOutputStrLen = MultiByteToWideChar(CP_ACP, 0, pChar, nInputStrLen, NULL, 0 ) + 2;
  		pszOut = new WCHAR[nOutputStrLen];
		  if(pszOut) {
  			memset(pszOut, 0x00, sizeof(WCHAR) *nOutputStrLen);
			  MultiByteToWideChar(CP_ACP, 0, pChar, nInputStrLen, pszOut, nInputStrLen);
  		}
    }

    return pszOut;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// ISampleGrabberCB Implementation /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) gcgINTERNAL_VIDEO::AddRef()  { return 2; }
STDMETHODIMP_(ULONG) gcgINTERNAL_VIDEO::Release() { return 1; }

STDMETHODIMP gcgINTERNAL_VIDEO::BufferCB(double dblSampleTime, BYTE* pBufferOri, long lBufferSize ) {
	return E_NOTIMPL; // return NOERROR;
}

/***************************************************************************************
* This method is implemented from ISampleGrabberCB interface. This is a callback method
* that can perform a real time image processing on the video. Here we can do anything
* with each frame. Be careful if you work with a real time video. A delay here implies
* in a delay on video playplack.
*
* IN 	 @SampleTime - Starting time of the sample, in seconds.
* IN     @pSample    - Pointer to the IMediaSample interface of the sample.
***************************************************************************************/
STDMETHODIMP gcgINTERNAL_VIDEO::SampleCB(double SampleTime, IMediaSample* pSample) {
	//CComQIPtr<IMediaSample2> pMediaSample2 = pSample; // Query for a newer interface, it is available since long ago, we can rely on it
	//ATLASSERT(pMediaSample2); // To make sure we have a valid pointer
	//AM_SAMPLE2_PROPERTIES Properties;
	//ATLVERIFY(SUCCEEDED(pMediaSample2->GetProperties(sizeof Properties, (BYTE*) &Properties)));

  HRESULT hr;
  OAFilterState state;
  hr = pControl->GetState(0, &state); // get new state
  // Return imediately if the graph is paused or stoped.
	if(FAILED(hr) || state != State_Running) {
    // Frame received
    SetEvent(this->framereceived);
    return S_FALSE;
  }

	AM_MEDIA_TYPE *am;
	pSample->GetMediaType(&am);
	if(am) {
		if(am->formattype == FORMAT_VideoInfo) {
			// Check the buffer size.
			if(am->cbFormat >= sizeof(VIDEOINFOHEADER)) {
				VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*) am->pbFormat;

        // Update image information
	      width = vih->bmiHeader.biWidth;
	      height = vih->bmiHeader.biHeight;
	      bpp = (unsigned char) vih->bmiHeader.biBitCount;
	      fps = (float) (10000000.0 / (double) vih->AvgTimePerFrame);

				// Create space for bitmap information
				int ncolors = numberOfColors(&vih->bmiHeader);
				int newbmpsize = vih->bmiHeader.biSize + sizeof(RGBQUAD) * ncolors;
				if(newbmpsize > bmpsize) {
				  if(bmpinfo) delete bmpinfo;
				  bmpinfo = (BITMAPINFOHEADER*) (new unsigned char[newbmpsize]);
				  if(bmpinfo == NULL) {
					  releaseVideo();
					  return false;
				  }
				  bmpsize = newbmpsize;
				}
				memcpy(bmpinfo, &vih->bmiHeader, bmpsize);
				npalettecolors = numberOfColors(bmpinfo);
			}
		}
		freeMediaType(*am);
	}
	// Update lastframe attribute in gcgVIDEOCAPTURE
	pSample->GetPointer(&bmpdata);
	frametime = SampleTime;

	// Call callback function signaling a new frame
	if(this->callbackFunc) this->callbackFunc(gcgvideopointer);

	// to avoid access violation
	bmpdata = NULL;
	return NOERROR;
}

STDMETHODIMP gcgINTERNAL_VIDEO::QueryInterface( REFIID riid, void** ppv ) {
	if(riid == IID_ISampleGrabberCB || riid == IID_IUnknown ) {
        *ppv = ( void* ) static_cast< ISampleGrabberCB* > ( this );
		return NOERROR;
	}
	return E_NOINTERFACE;
}


/////////////////////////////////////////////////////////////////////////////
// Builds the graph of a standard camera
/////////////////////////////////////////////////////////////////////////////
int gcgINTERNAL_VIDEO::startStandardCamera(int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps) {
	AM_MEDIA_TYPE mt;
	HRESULT hr;

  // Busca o dispositivo de captura de video que existir
	findDevice(&pCap, ID);
	if(!pCap) return false;

	// Adiciona o filtro de captura ao Filter Graph Manager
	hr = pGraph->AddFilter(pCap, L"GCG Capture" );
	if(FAILED(hr)) return false;

/*
	//IBaseFilter *pFilter;
	// Obtain the filter's IBaseFilter interface. (Not shown)
	ISpecifyPropertyPages *pProp;
	hr = pCap->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
	if(SUCCEEDED(hr)) {
		// Get the filter's name and IUnknown pointer.
		FILTER_INFO FilterInfo;
		hr = pCap->QueryFilterInfo(&FilterInfo);
		IUnknown *pFilterUnk;
		pCap->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

		// Show the page.
		CAUUID caGUID;
		pProp->GetPages(&caGUID);
		pProp->Release();
		OleCreatePropertyFrame(
			NULL,                 // Parent window
			0, 0,                   // Reserved
			FilterInfo.achName,     // Caption for the dialog box
			1,                      // Number of objects (just the filter)
			&pFilterUnk,            // Array of object pointers.
			caGUID.cElems,          // Number of property pages
			caGUID.pElems,          // Array of property page CLSIDs
			0,                      // Locale identifier
			0, NULL                 // Reserved
		);

		// Clean up.
		if(pFilterUnk) pFilterUnk->Release();
		if(FilterInfo.pGraph) FilterInfo.pGraph->Release();
		CoTaskMemFree(caGUID.pElems);
	}

	// Busca a interface IAMVfwCaptureDialogs do filtro de captura
	IAMVfwCaptureDialogs *pVfw = 0;
	hr = pCap->QueryInterface(IID_IAMVfwCaptureDialogs, (void**)&pVfw);
	if(SUCCEEDED(hr)) {
		// Verifica se o dispositivo suporta esa caixa de dialogo
		if(S_OK == pVfw->HasDialog(VfwCaptureDialog_Source))
			// Mostra a caixa de diálogo
			hr = pVfw->ShowDialog(VfwCaptureDialog_Source, NULL);

		pVfw->Release();
	}
*/

	// Busca interface de configuracao de fluxo
	IAMStreamConfig *pConfig = NULL;

	hr = pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, 0,    // todo tipo de midia
				pCap, IID_IAMStreamConfig, (void**)&pConfig);

	if(SUCCEEDED(hr)) {
		// Varre as caracteristicas de captura
		int iCount = 0, iSize = 0;
		hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);

		// Verifica o tamanho para saber se é a mesma estrutura
		if(iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS)) {
			for(int iFormat = 0; iFormat < iCount; iFormat++) {
				VIDEO_STREAM_CONFIG_CAPS scc;
				AM_MEDIA_TYPE *pmtConfig;
				hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
				if(SUCCEEDED(hr)) {
					// Examina o formato e possivelmente o utilza
					if ((pmtConfig->majortype == MEDIATYPE_Video) &&
						//(pmtConfig->subtype == MEDIASUBTYPE_YUYV) &&
						(pmtConfig->formattype == FORMAT_VideoInfo) &&
						(pmtConfig->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
						(pmtConfig->pbFormat != NULL)) {
						VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*) pmtConfig->pbFormat;

						switch(_bpp) {
							case 1: pmtConfig->subtype = MEDIASUBTYPE_RGB1;
									break;
							case 4: pmtConfig->subtype = MEDIASUBTYPE_RGB4;
									break;
							case 8: pmtConfig->subtype = MEDIASUBTYPE_RGB8;
									break;
							case 16: pmtConfig->subtype = MEDIASUBTYPE_RGB565;
									 break;
							case 32: pmtConfig->subtype = MEDIASUBTYPE_ARGB32;
									 break;
							default: pmtConfig->subtype = MEDIASUBTYPE_RGB24;
									 break;
						}

						// Altera o formato
						pVih->bmiHeader.biWidth = _width;
						pVih->bmiHeader.biHeight = _height;
						pVih->bmiHeader.biBitCount = _bpp;
						pVih->bmiHeader.biSizeImage = DIBSIZE(pVih->bmiHeader);
						pVih->bmiHeader.biCompression = BI_RGB;
						pVih->AvgTimePerFrame = (REFERENCE_TIME) (10000000.0 / (double) _fps);
						pVih->dwBitRate = 999999999;
						hr = pConfig->SetFormat(pmtConfig);
						if(SUCCEEDED(hr)) {
							freeMediaType(*pmtConfig);
							break;
						}
					}
				}
				// Libera o tipo de mídia
				freeMediaType(*pmtConfig);
			}
		}

		// libera
		pConfig->Release();
	}


	// Cria o renderer nulo
	hr = ::CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
									IID_IBaseFilter, (LPVOID*) &pNulo);
	if(S_OK != hr) return false;

	// Adiciona o renderer nulo
	hr = pGraph->AddFilter(pNulo, L"NULL renderer");
	if(FAILED(hr)) return false;

	// Especifica o tipo de midia aceito pelo programa
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	mt.majortype = MEDIATYPE_Video;
	switch(_bpp) {
		case 1: mt.subtype = MEDIASUBTYPE_RGB1;
				break;
		case 4: mt.subtype = MEDIASUBTYPE_RGB4;
				break;
		case 8: mt.subtype = MEDIASUBTYPE_RGB8;
				break;
		case 16: mt.subtype = MEDIASUBTYPE_RGB565;
				 break;
		case 32: mt.subtype = MEDIASUBTYPE_ARGB32;
				 break;
		default: mt.subtype = MEDIASUBTYPE_RGB24;
				 break;
	}
	hr = pGrabber->SetMediaType(&mt);
	if(FAILED(hr)) return false;

	// Se o sistema de captura tem um pino VP (Video Port), ele DEVE ser conectado a um renderer.
	// A funcao RenderStream automaticamente faz uma conexao entre o VP e "Overlay Mixer" e um
	// "Video renderer".  A janela de saida inicialmente fica escondida.
	// Uma chamada posterior de RenderStream com  PIN_CATEGORY_PREVIEW faz com que esta
	// janela seja mostrada.
	/*
	IPin *pVPPin;
	hr = pBuild->FindPin(pCap, PINDIR_OUTPUT, &PIN_CATEGORY_VIDEOPORT, NULL, FALSE, 0, &pVPPin);
	*/

	// Faz as devidas conexoes de filtros
	//hr = pBuild->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pCap, pGrabberF, NULL);
	//hr = pBuild->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pCap, NULL, NULL);
	hr = pBuild->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pCap, pGrabberF, pNulo);
	//hr = pBuild->RenderStream(NULL, NULL, pCap, pGrabberF, pNulo);

	if(FAILED(hr)) return false;

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Builds the graph of a HDV camera
/////////////////////////////////////////////////////////////////////////////
int gcgINTERNAL_VIDEO::startHDVCamera(int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps) {
	HRESULT hr;

  // Busca o dispositivo de captura de video que existir
	findDevice(&pCap, ID);
	if(!pCap) {
		releaseVideo();
		return false;
	}

	// Adiciona o filtro de captura ao Filter Graph Manager
	hr = pGraph->AddFilter(pCap, L"GCG Capture" );
	if(FAILED(hr)) {
		releaseVideo();
		return false;
	}

	/*
	// add Microsoft AV/C Tape Subunit Device
	if(!CreateFilter(L"Microsoft AV/C Tape Subunit Device", &pCap, CLSID_VideoInputDeviceCategory)) {
    releaseVideo();
    return false;
  }

	hr = pGraph->AddFilter(pCap, L"Microsoft AV/C Tape Subunit Device");
	if(FAILED(hr)) {
	  releaseVideo();
	  return false;
	}
	*/

	//add MPEG-2 Demultiplexer
	CComPtr<IBaseFilter> pMPEG2Demultiplexer;
	if(!CreateFilter(L"MPEG-2 Demultiplexer", &pMPEG2Demultiplexer, CLSID_LegacyAmFilterCategory)) {
	  releaseVideo();
	  return false;
	}

	hr = pGraph->AddFilter(pMPEG2Demultiplexer, L"MPEG-2 Demultiplexer");
	if(FAILED(hr)) {
	  releaseVideo();
	  return false;
	}

	// connect Microsoft AV/C Tape Subunit Device -> MPEG-2 Demultiplexer
  //hr = pGraph->ConnectDirect(GetPin(pIntermediate, L"Capture"), GetPin(pMPEG2Demultiplexer, L"MPEG-2 Stream"), NULL);
  hr = pGraph->ConnectDirect(GetPin(pCap, PINDIR_OUTPUT), GetPin(pMPEG2Demultiplexer, PINDIR_INPUT), NULL);
	if(FAILED(hr)) {
	  releaseVideo();
	  return false;
	}

	CreateOutputPin(pMPEG2Demultiplexer, _width, _height, _bpp, _fps);

	// add MPEG2 Video Decoder
	IBaseFilter *pMpeg2;
	if(!connectMPEG2Decoder(&pMpeg2, pMPEG2Demultiplexer)) {
	  releaseVideo();
	  return false;
	}

  // add intermediate filter
	CComPtr<IBaseFilter> pIntermediate;
	if(!CreateFilter(L"Smart Tee", &pIntermediate, CLSID_LegacyAmFilterCategory)) {
	  releaseVideo();
	  return false;
	}

	hr = pGraph->AddFilter(pIntermediate, L"Intermediate");
	if(FAILED(hr)) {
	  releaseVideo();
	  return false;
	}

  // connect pMpeg2 -> intermediate
  hr = pGraph->ConnectDirect(GetPin(pMpeg2, PINDIR_OUTPUT), GetPin(pIntermediate, PINDIR_INPUT), NULL);
	if(FAILED(hr)) {
	  releaseVideo();
	  return false;
	}

	// Cria o renderer nulo
	hr = ::CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID*) &pNulo);
	if(S_OK != hr) {
		releaseVideo();
		return false;
	}

	// Adiciona o renderer nulo
	hr = pGraph->AddFilter(pNulo, L"NULL renderer");
	if(FAILED(hr)) {
		releaseVideo();
		return false;
	}

	hr = pBuild->RenderStream(NULL, &MEDIATYPE_Video, pIntermediate, pGrabberF, pNulo);
	if(FAILED(hr)) {
		releaseVideo();
		return false;
	}
  return true;
}






/////////////////////////////////////////////////////////////////////////////
// Connect to a suitable MPEG2 decoder
/////////////////////////////////////////////////////////////////////////////
int gcgINTERNAL_VIDEO::connectMPEG2Decoder(IBaseFilter **ppFilter, IBaseFilter *pDemux) {
	HRESULT hr;

	//if(!ppFilter) return false;
	//*ppFilter = NULL;

	// Cria um mapeador
	IFilterMapper2 *pMapper = NULL;
	IEnumMoniker *pEnum = NULL;

  hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC, IID_IFilterMapper2, (void **) &pMapper);
  if(FAILED(hr)) return false;

	GUID arrayInTypes[2];
	arrayInTypes[0] = MEDIATYPE_Video;
  arrayInTypes[1] = MEDIASUBTYPE_MPEG2_VIDEO;//GUID_NULL;//MEDIASUBTYPE_DVSD; //dvsd
  GUID arrayOutTypes[2];
  arrayOutTypes[0] = MEDIATYPE_Video;
  arrayOutTypes[1] = GUID_NULL;//MEDIASUBTYPE_YV12;

	// Enumera dispositivos de captura de video
	hr = pMapper->EnumMatchingFilters(&pEnum, 0, // Reserved.
                                    TRUE,//FALSE, // Use exact match?
                                    MERIT_NORMAL, // Minimum normal
                                    TRUE, // At least one input pin?
                                    1, // Number of major type/subtype pairs for input.
                                    arrayInTypes, // Array of major type/subtype pairs for input.
                                    NULL, // Input medium.
                                    NULL, // Input pin category.
                                    FALSE, // Must be a renderer?
                                    TRUE, // At least one output pin?
                                    0, // Number of major type/subtype pairs for output.
                                    arrayOutTypes, // Array of major type/subtype pairs for output.
                                    NULL, // Output medium.
                                    NULL); // Output pin category.
	  // Enumerate the monikers.
  IMoniker *pMoniker;
  ULONG cFetched;

  while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
      IPropertyBag *pPropBag = NULL;
      hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);

      if(SUCCEEDED(hr)) {
          /*// To retrieve the friendly name of the filter, do the following:
          VARIANT varName;
          VariantInit(&varName);
          hr = pPropBag->Read(L"FriendlyName", &varName, 0);
          if (SUCCEEDED(hr)) {
            // Display the name in your UI somehow.
		        USES_CONVERSION;
		        //The following 2 lines convert the camera name from wchar_t* to char* and return to pCameraName converted value
		        wchar_t* pWideCameraName = ( wchar_t* ) OLE2T( varName.bstrVal );
		        char pCameraName[200];
 		        _wchar_t2char( pWideCameraName, pCameraName );
             printf("%s\n", pCameraName);
          }
          VariantClear(&varName);
          */


          // To create an instance of the filter, do the following:
          hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**) ppFilter);
          if(SUCCEEDED(hr)) {
            // Try to connect
 	          // connect MPEG-2 Demultiplexer -> MPEG2 Video Decoder
            hr = pGraph->AddFilter(*ppFilter, L"MPEG2 Video Decoder");
	          if(SUCCEEDED(hr)) {
	            hr = pGraph->ConnectDirect(GetPin(pDemux, PINDIR_OUTPUT), GetPin(*ppFilter, PINDIR_INPUT), NULL);
	            if(SUCCEEDED(hr)) {
                pMoniker->Release();
                pMapper->Release();
                pEnum->Release();
                return true;
	            }
	            pGraph->RemoveFilter(*ppFilter);
	          }
          }
          // Release pFilter later.
          (*ppFilter)->Release();

          // Clean up.
          pPropBag->Release();
      }
      pMoniker->Release();
  }
  // Clean up.
  pMapper->Release();
  pEnum->Release();
	return false;
}

#endif // _MSC_VER Main definitions for Visual C++


#ifndef VAPIWIN_API // If the target is a vapiwin.dll, the gcgVIDEO interfaces are not included.

// Checks version for gcgVIDEO inclusion
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)

#ifdef _MSC_VER
// gcgVIDEO definition for Visual C++

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/// Video interface for GCGlib in Visual C++
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

gcgVIDEO::gcgVIDEO() {
  width = 0;
  height = 0;
  bpp = 0;
	fps = 0.0;
	bitrate = 0;
	this->handle = new gcgINTERNAL_VIDEO(this);
  if(this->handle == NULL)
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgVIDEO(): internal video allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

gcgVIDEO::~gcgVIDEO() {
  if(this->handle != NULL) delete this->handle;
  else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "~gcgVIDEO(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

gcgVIDEOCAPTURE::gcgVIDEOCAPTURE() {
  id = 0;         // Current camera ID
}

gcgVIDEOCAPTURE::~gcgVIDEOCAPTURE() {
  if(!this->destroyVideo())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_RELEASEERROR), "~gcgVIDEOCAPTURE(): error releasing video resources. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

gcgVIDEOFILE::gcgVIDEOFILE() {
}

gcgVIDEOFILE::~gcgVIDEOFILE() {
  if(!this->destroyVideo())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_RELEASEERROR), "~gcgVIDEOFILE(): error releasing video resources. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

bool gcgVIDEOCAPTURE::destroyVideo() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "destroyVideo(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  bool res = this->handle->releaseVideo();
  // Report internal error
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  return res;
}

bool gcgVIDEOFILE::destroyVideo() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "destroyVideo(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  bool res = this->handle->releaseVideo();
  // Report internal error
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  return res;
}

int gcgVIDEOCAPTURE::getNumberOfCameras() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getNumberOfCameras(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  int res = this->handle->getNumberOfCameras();
  // Report internal error
  if(res <= 0) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  return res;
}

bool gcgVIDEOCAPTURE::getCameraName(unsigned int ID, char *pCameraName, unsigned int maxName) {
  if(pCameraName == NULL || maxName == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "getCameraName(): invalid char pointer. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getCameraName(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

	bool res = this->handle->getCameraName(ID, pCameraName, maxName);
  // Report internal error
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
	return res;
}

bool gcgVIDEOFILE::openVideoFile(const char* file) {
  if(file == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "openVideoFile(): invalid char pointer. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "openVideoFile(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

	bool res = this->handle->openVideoFile(file);

  // Report internal error
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  else {
    width = this->handle->width;
    height = this->handle->height;
    bpp = this->handle->bpp;
	  fps = this->handle->fps;
	  bitrate = this->handle->bitrate;
  }
	return res;
}


bool gcgVIDEOCAPTURE::openCamera(unsigned int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps) {
  if(_width == 0 || _height == 0 || _bpp == 0 || _fps == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "openCamera(): invalid parameter. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "openCamera(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

	bool res = this->handle->openCamera(ID, _width, _height, _bpp, _fps);

  // Report internal error
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  else {
    width = this->handle->width;
    height = this->handle->height;
    bpp = this->handle->bpp;
	  fps = this->handle->fps;
	  bitrate = this->handle->bitrate;
	  id = this->handle->id;
  }
	return res;
}

bool gcgVIDEOCAPTURE::setCallBackFunction(void (*callback) (gcgVIDEOCAPTURE*)) {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setCallBackFunction(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(callback == NULL)
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "setCallBackFunction(): callback disabled because function pointer is null. (%s:%d)", basename((char*)__FILE__), __LINE__);

  bool res = this->handle->setCallBackFunction((void (*)(void*)) callback);
  // Report internal error
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  return res;
}

double gcgVIDEOCAPTURE::copyFrameTo(gcgIMAGE *img) {
  if(img == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "copyFrame(): invalid image pointer. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return -1.0;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "copyFrame(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return -1.0;
  }

  // Check all information
  if(this->handle->bmpinfo != NULL && this->handle->bmpdata != NULL) {
    img->unpackBMP(this->handle->bmpinfo, this->handle->bmpdata, this->handle->npalettecolors,
                                 (unsigned char*) this->handle->bmpinfo + this->handle->bmpinfo->biSize);
    return this->handle->frametime;
  }

  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_UNAVAILABLERESOURCE), "copyFrame(): unavailable frame. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return -1.0;
}

long gcgVIDEOFILE::getFrameCount() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getFrameCount(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  long res = this->handle->getFrameCount();
  // Report internal error
  if(res <= 0) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  return res;
}

bool gcgVIDEOFILE::setCurrentPosition(long newposition) {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setCurrentPosition(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

	bool res = this->handle->setCurrentPosition(newposition);
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
	return res;
}

long gcgVIDEOFILE::getCurrentPosition() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getCurrentPosition(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return -1;
  }

  long res = this->handle->getCurrentPosition();
  if(res < 0) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  return res;
}

bool gcgVIDEOFILE::copyFrame(gcgIMAGE *dstimg) {
  if(dstimg == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "copyCurrentFrame(): invalid image pointer. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "copyCurrentFrame(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  void *bmpinfo, *bmpdata, *pal;
  unsigned int npal;
  bool res = this->handle->getCurrentImageInfoAndAdvance(&bmpinfo, &bmpdata, &npal, &pal);
  if(res) dstimg->unpackBMP(bmpinfo, (unsigned char*) bmpdata, npal, (unsigned char*) pal); // Unpack bmp
  else gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  return res;
}

bool gcgVIDEOCAPTURE::start() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "start(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

	bool res = this->handle->start();
  // Report internal error
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
	return res;
}

bool gcgVIDEOCAPTURE::stop() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "stop(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  bool res = this->handle->stop();
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
  return res;
}

bool gcgVIDEOCAPTURE::resume() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "resume(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

	bool res = this->handle->resume();
  // Report internal error
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
	return res;
}

bool gcgVIDEOCAPTURE::pause() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "pause(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

	bool res = this->handle->pause();
  // Report internal error
  if(!res) gcgReport(this->handle->code, this->handle->msg, this->handle->file, this->handle->line);
	return res;
}

bool gcgVIDEOCAPTURE::isCapturing() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "isCapturing(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  return this->handle->isCapturing();
}

// End of the objects for GCGlib library with Visual C++

#else // End of Visual C++ compiler version

//////////////////////////////////////////////////////////////////////////////
// This IS NOT a Visual C++ compiler: use vapiwin.dll for video processing
//////////////////////////////////////////////////////////////////////////////


// Dummy class for internal video processing
class gcgINTERNAL_VIDEO {
};


#define VAPIFUNCTION static //extern"C"

VAPIFUNCTION void* (*vapiNEWINTERNAL_VIDEO)(void*) = NULL;
VAPIFUNCTION void  (*vapiDELETEINTERNAL_VIDEO)(void*) = NULL;
VAPIFUNCTION int   (*vapiDESTROYVIDEOCAPTURE)(void*, int*, char**, char**, int*) = NULL;
VAPIFUNCTION int   (*vapiDESTROYVIDEOFILE)(void*, int*, char**, char**, int*) = NULL;
VAPIFUNCTION int   (*vapiGETNUMBEROFCAMERAS)(void*, int*, char**, char**, int*) = NULL;
VAPIFUNCTION int   (*vapiGETCAMERANAME)(void*, int, char*, int, int*, char**, char**, int*) = NULL;
VAPIFUNCTION int   (*vapiOPENVIDEOFILE)(void*, const char*, unsigned int*, unsigned int*, unsigned char*,
                                        float*, unsigned int*bitRate, int*, char**, char**, int*) = NULL;

VAPIFUNCTION  int  (*vapiOPENCAMERA)(void* iv, unsigned int, unsigned int, unsigned int, unsigned char, float,
							     unsigned int*, unsigned int*, unsigned char*, float*, unsigned int*, unsigned int*, int*, char**, char**, int*) = NULL;

VAPIFUNCTION  int  (*vapiSETCALLBACKFUNCTION)(void*, void (*callback) (gcgVIDEOCAPTURE*), int*, char**, char**, int*) = NULL;
VAPIFUNCTION  int  (*vapiCOPYFRAMETO)(void* iv, BITMAPINFOHEADER **bmpinfo, unsigned char **bmpdata, unsigned int *npalcolors, double *frametime) = NULL;

VAPIFUNCTION  long (*vapiGETFRAMECOUNT)(void*, int*, char**, char**, int*) = NULL;
VAPIFUNCTION  int  (*vapiSETCURRENTPOSITION)(void*, long, int*, char**, char**, int*) = NULL;
VAPIFUNCTION  long (*vapiGETCURRENTPOSITION)(void*, int*, char**, char**, int*) = NULL;

VAPIFUNCTION  int  (*vapiGETCURRENTIMAGEINFOANDADVANCE)(void*, void**, void**, void**, unsigned int*, int*, char**, char**, int*) = NULL;

VAPIFUNCTION  int (*vapiSTART)(void* iv, int*, char**, char**, int*) = NULL;
VAPIFUNCTION  int (*vapiSTOP)(void* iv, int*, char**, char**, int*) = NULL;
VAPIFUNCTION  int (*vapiRESUME)(void* iv, int*, char**, char**, int*) = NULL;
VAPIFUNCTION  int (*vapiPAUSE)(void* iv, int*, char**, char**, int*) = NULL;
VAPIFUNCTION  int (*vapiISCAPTURING)(void* iv) = NULL;



gcgVIDEO::gcgVIDEO() {
  HINSTANCE hinstLib;

  // Get a handle to the DLL module.
  hinstLib = LoadLibrary(TEXT("vapiwin"));

  // If the handle is valid, try to get the function address.
   if (hinstLib != NULL) {
      vapiNEWINTERNAL_VIDEO = (void* (*)(void*)) GetProcAddress(hinstLib, "vapiNEWINTERNAL_VIDEO");
      vapiDELETEINTERNAL_VIDEO = (void (*)(void*)) GetProcAddress(hinstLib, "vapiDELETEINTERNAL_VIDEO");
      vapiDESTROYVIDEOCAPTURE = (int (*)(void*, int*, char **, char**, int*)) GetProcAddress(hinstLib, "vapiDESTROYVIDEOCAPTURE");
      vapiDESTROYVIDEOFILE = (int (*)(void*, int*, char **, char**, int*)) GetProcAddress(hinstLib, "vapiDESTROYVIDEOFILE");
      vapiGETNUMBEROFCAMERAS = (int (*)(void*, int*, char **, char**, int*)) GetProcAddress(hinstLib, "vapiGETNUMBEROFCAMERAS");
      vapiGETCAMERANAME = (int (*)(void*, int, char*, int, int*, char **, char**, int*)) GetProcAddress(hinstLib, "vapiGETCAMERANAME");
      vapiOPENVIDEOFILE = (int (*)(void*, const char *, unsigned int *, unsigned int *, unsigned char *,
                                      float*, unsigned int*, int*, char **, char**, int*)) GetProcAddress(hinstLib, "vapiOPENVIDEOFILE");
      vapiOPENCAMERA = (int (*)(void*, unsigned int, unsigned int, unsigned int, unsigned char, float,
							     unsigned int*, unsigned int*, unsigned char*, float*, unsigned int*, unsigned int*, int*, char **, char**, int*)) GetProcAddress(hinstLib, "vapiOPENCAMERA");

      vapiSETCALLBACKFUNCTION = (int (*)(void*, void (*) (gcgVIDEOCAPTURE*), int*, char **, char**, int*)) GetProcAddress(hinstLib, "vapiSETCALLBACKFUNCTION");
      vapiCOPYFRAMETO = (int (*)(void* iv, BITMAPINFOHEADER **bmpinfo, unsigned char **bmpdata, unsigned int *npalcolors, double *frametime)) GetProcAddress(hinstLib, "vapiCOPYFRAMETO");
      vapiGETFRAMECOUNT = (long (*)(void*, int*, char**, char**, int*)) GetProcAddress(hinstLib, "vapiGETFRAMECOUNT");

      vapiSETCURRENTPOSITION = (int (*)(void*, long, int*, char**, char**, int*)) GetProcAddress(hinstLib, "vapiSETCURRENTPOSITION");
      vapiGETCURRENTPOSITION = (long (*)(void*, int*, char**, char**, int*)) GetProcAddress(hinstLib, "vapiGETCURRENTPOSITION");

      vapiGETCURRENTIMAGEINFOANDADVANCE = (int (*)(void*, void**, void**, void**, unsigned int*, int*, char**, char**, int*))GetProcAddress(hinstLib, "vapiGETCURRENTIMAGEINFOANDADVANCE");

      vapiSTART = (int (*)(void*, int*, char**, char**, int*)) GetProcAddress(hinstLib, "vapiSTART");
      vapiSTOP = (int (*)(void*, int*, char**, char**, int*))  GetProcAddress(hinstLib, "vapiSTOP");
      vapiRESUME = (int (*)(void*, int*, char**, char**, int*))  GetProcAddress(hinstLib, "vapiRESUME");
      vapiPAUSE = (int (*)(void*, int*, char**, char**, int*))  GetProcAddress(hinstLib, "vapiPAUSE");
      vapiISCAPTURING = (int (*)(void*))  GetProcAddress(hinstLib, "vapiISCAPTURING");


      // Free the DLL module.
      // FreeLibrary(hinstLib);
  } else {
      MessageBox(NULL, "Could not find vapiwin.dll.\n\nGCGlib video functions are disabled.\n\nAccess www.gcg.ufjf.br and download the library.", "GCG - Group for Computer Graphics, Image and Vision", MB_OK | MB_ICONERROR);
      this->handle = NULL;
      return;
  }

  width = 0;
  height = 0;
  bpp = 0;
	fps = 0.0;
	bitrate = 0;
	this->handle = static_cast<gcgINTERNAL_VIDEO*>( vapiNEWINTERNAL_VIDEO(this) );
  if(this->handle == NULL)
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgVIDEO(): internal video allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__);
}


gcgVIDEO::~gcgVIDEO() {
  if(this->handle != NULL) vapiDELETEINTERNAL_VIDEO(this->handle);
  else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "~gcgVIDEO(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
}


gcgVIDEOCAPTURE::gcgVIDEOCAPTURE() {
  id = 0;         // Current camera ID
}


gcgVIDEOCAPTURE::~gcgVIDEOCAPTURE() {
  if(!this->destroyVideo())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_RELEASEERROR), "~gcgVIDEOCAPTURE(): error releasing video resources. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

gcgVIDEOFILE::gcgVIDEOFILE() {
}

gcgVIDEOFILE::~gcgVIDEOFILE() {
  if(!this->destroyVideo())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_RELEASEERROR), "~gcgVIDEOFILE(): error releasing video resources. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

bool gcgVIDEOCAPTURE::destroyVideo() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "destroyVideo(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  int code, line;
  char *msg, *file;
  bool res = vapiDESTROYVIDEOCAPTURE(this->handle, &code, &msg, &file, &line);
  // Report internal error
  if(!res) gcgReport(code, msg, file, line);
  return res;
}

bool gcgVIDEOFILE::destroyVideo() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "destroyVideo(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  int code, line;
  char *msg, *file;
  bool res = vapiDESTROYVIDEOFILE(this->handle, &code, &msg, &file, &line);
  // Report internal error
  if(!res) gcgReport(code, msg, file, line);
  return res;

}

int gcgVIDEOCAPTURE::getNumberOfCameras() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getNumberOfCameras(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  int code, line;
  char *msg, *file;
  int res = vapiGETNUMBEROFCAMERAS(this->handle, &code, &msg, &file, &line);
  // Report internal error
  if(res <= 0) gcgReport(code, msg, file, line);
  return res;
}


bool gcgVIDEOCAPTURE::getCameraName(unsigned int ID, char *pCameraName, unsigned int maxName) {
  if(pCameraName == NULL || maxName == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "getCameraName(): invalid char pointer. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getCameraName(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }


  int code, line;
  char *msg, *file;
	bool res = vapiGETCAMERANAME(this->handle, ID, pCameraName, maxName, &code, &msg, &file, &line);
  // Report internal error
  if(!res) gcgReport(code, msg, file, line);
	return res;
}


bool gcgVIDEOFILE::openVideoFile(const char* file) {
  if(file == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "openVideoFile(): invalid char pointer. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "openVideoFile(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  unsigned int w, h, r;
  unsigned char b;
  float f;
  int code, line;
  char *msg, *fi;
	int res = vapiOPENVIDEOFILE(this->handle, file, &w,&h, &b, &f, &r, &code, &msg, &fi, &line);

  // Report internal error
  if(!res) gcgReport(code, msg, fi, line);
  else {
    this->width = w;
    this->height = h;
    this->bpp = b;
	  this->fps = f;
	  this->bitrate = r;
  }
	return res;
}

bool gcgVIDEOCAPTURE::openCamera(unsigned int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps) {
  if(_width == 0 || _height == 0 || _bpp == 0 || _fps == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "openCamera(): invalid parameter. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "openCamera(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  unsigned int w, h, r, id_;
  unsigned char b;
  float f;
  int code, line;
  char *msg, *fi;
	int res = vapiOPENCAMERA(this->handle, ID, _width, _height, _bpp, _fps, &w,&h, &b, &f, &id_, &r, &code, &msg, &fi, &line);

  // Report internal error
  if(!res) gcgReport(code, msg, fi, line);
  else {
    this->width = w;
    this->height = h;
    this->bpp = b;
	  this->fps = f;
	  this->bitrate = r;
	  this->id = id_;
  }
	return res;
}


bool gcgVIDEOCAPTURE::setCallBackFunction(void (*callback) (gcgVIDEOCAPTURE*)) {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setCallBackFunction(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(callback == NULL)
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "setCallBackFunction(): callback disabled because function pointer is null. (%s:%d)", basename((char*)__FILE__), __LINE__);

  int code, line;
  char *msg, *file;
  int res = vapiSETCALLBACKFUNCTION(this->handle, (void (*)(gcgVIDEOCAPTURE*)) callback, &code, &msg, &file, &line);
  // Report internal error
  if(!res) gcgReport(code, msg, file, line);
  return res;
}


double gcgVIDEOCAPTURE::copyFrameTo(gcgIMAGE *img) {
  if(img == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "copyFrame(): invalid image pointer. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return -1.0;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "copyFrame(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return -1.0;
  }

  BITMAPINFOHEADER *bmpinfo;
  unsigned char *bmpdata;
  unsigned int npalcolors;
  double frametime;
  if(vapiCOPYFRAMETO(this->handle, &bmpinfo, &bmpdata, &npalcolors, &frametime)) {
    img->unpackBMP((void*)bmpinfo, bmpdata, npalcolors, (unsigned char*) bmpinfo + bmpinfo->biSize);
    return frametime;
  }

  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_UNAVAILABLERESOURCE), "copyFrame(): unavailable frame. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return -1.0;
}

long gcgVIDEOFILE::getFrameCount() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getFrameCount(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  int code, line;
  char *msg, *file;
  long res = vapiGETFRAMECOUNT(this->handle, &code, &msg, &file, &line);
  // Report internal error
  if(res <= 0) gcgReport(code, msg, file, line);
  return res;
}

bool gcgVIDEOFILE::setCurrentPosition(long newposition) {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "setCurrentPosition(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  int code, line;
  char *msg, *file;
	int res = vapiSETCURRENTPOSITION(this->handle, newposition, &code, &msg, &file, &line);
  if(!res) gcgReport(code, msg, file, line);
	return res;
}


long gcgVIDEOFILE::getCurrentPosition() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "getCurrentPosition(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return -1;
  }

  int code, line;
  char *msg, *file;
  long res = vapiGETCURRENTPOSITION(this->handle, &code, &msg, &file, &line);
  if(res < 0) gcgReport(code, msg, file, line);
  return res;
}

bool gcgVIDEOFILE::copyFrame(gcgIMAGE *dstimg) {
  if(dstimg == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_VIDEO, GCG_BADPARAMETERS), "copyCurrentFrame(): invalid image pointer. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "copyCurrentFrame(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  void *bmpinfo, *bmpdata, *pal;
  unsigned int npal;
  int code, line;
  char *msg, *file;
  int res = vapiGETCURRENTIMAGEINFOANDADVANCE(this->handle, &bmpinfo, &bmpdata, &pal, &npal, &code, &msg, &file, &line);
  if(res) dstimg->unpackBMP(bmpinfo, (unsigned char*) bmpdata, npal, (unsigned char*) pal); // Unpack bmp
  else gcgReport(code, msg, file, line);
  return res;
}

bool gcgVIDEOCAPTURE::start() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "start(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  int code, line;
  char *msg, *file;
	bool res = vapiSTART(this->handle, &code, &msg, &file, &line);
  // Report internal error
  if(!res) gcgReport(code, msg, file, line);
	return res;
}

bool gcgVIDEOCAPTURE::stop() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "stop(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  int code, line;
  char *msg, *file;
  bool res = vapiSTOP(this->handle, &code, &msg, &file, &line);
  if(!res) gcgReport(code, msg, file, line);
  return res;
}

bool gcgVIDEOCAPTURE::resume() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "resume(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  int code, line;
  char *msg, *file;
	bool res = vapiRESUME(this->handle, &code, &msg, &file, &line);
  // Report internal error
  if(!res) gcgReport(code, msg, file, line);
	return res;
}

bool gcgVIDEOCAPTURE::pause() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "pause(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  int code, line;
  char *msg, *file;
	bool res = vapiPAUSE(this->handle, &code, &msg, &file, &line);
  // Report internal error
  if(!res) gcgReport(code, msg, file, line);
	return res;
}

bool gcgVIDEOCAPTURE::isCapturing() {
  if(this->handle == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "isCapturing(): invalid internal video handler. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  return vapiISCAPTURING(this->handle);
}



#endif // _MSC_VER verification

#elif defined(GCG_LINUX32) || defined(GCG_LINUX64) // End of WINDOWS video api ==> Starting LINUX video api



gcgVIDEO::gcgVIDEO() {
}


gcgVIDEO::~gcgVIDEO() {
}


gcgVIDEOCAPTURE::gcgVIDEOCAPTURE() {
  id = 0;         // Current camera ID
}


gcgVIDEOCAPTURE::~gcgVIDEOCAPTURE() {
}

gcgVIDEOFILE::gcgVIDEOFILE() {
}

gcgVIDEOFILE::~gcgVIDEOFILE() {
}

bool gcgVIDEOCAPTURE::destroyVideo() {
 return false;
}

bool gcgVIDEOFILE::destroyVideo() {
 return false;
}

int gcgVIDEOCAPTURE::getNumberOfCameras() {
 return 0;
}


bool gcgVIDEOCAPTURE::getCameraName(unsigned int ID, char *pCameraName, unsigned int maxName) {
 return false;
}

double gcgVIDEOCAPTURE::copyFrameTo(gcgIMAGE *dstimg) {
	return false;
}

bool gcgVIDEOFILE::openVideoFile(const char* file) {
 return false;
}

bool gcgVIDEOCAPTURE::openCamera(unsigned int ID, unsigned int _width, unsigned int _height, unsigned char _bpp, float _fps) {
  return false;
}


bool gcgVIDEOCAPTURE::setCallBackFunction(void (*callback) (gcgVIDEOCAPTURE*)) {
  return false;
}

long gcgVIDEOFILE::getFrameCount() {
	return 0L;
}

bool gcgVIDEOFILE::setCurrentPosition(long newposition) {
	return false;
}

long gcgVIDEOFILE::getCurrentPosition() {
	return 0L;
}

bool gcgVIDEOFILE::copyFrame(gcgIMAGE *dstimg) {
	return false;
}

bool gcgVIDEOCAPTURE::start() {
	return false;
}

bool gcgVIDEOCAPTURE::stop() {
	return false;
}

bool gcgVIDEOCAPTURE::resume() {
	return false;
}

bool gcgVIDEOCAPTURE::pause() {
	return false;
}

bool gcgVIDEOCAPTURE::isCapturing() {
  return false;
}



#else // End of LINUX video API

#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."

#endif

#endif // End of VAPIWIN_API section

