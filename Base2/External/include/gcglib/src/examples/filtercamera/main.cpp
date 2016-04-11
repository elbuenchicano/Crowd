#ifdef _MSC_VER
  #include <windows.h>
  #pragma warning(disable: 4996) // Disable deprecation
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <gcg.h>
#include "viewer.h"

// Object for viewing images
gcgIMAGEVIEWER      vinput;

// Synchronized buffer for communication between input callback and main thread
gcgPRODUCERCONSUMER queue;

// Defining a 2D low pass mask
float lowpassc[] = {1/12.0, 3/12.0, 4/12.0, 3/12.0, 1/12.0};
gcgDISCRETE1D<float> lowpass(5, 2, lowpassc, true);

// Video input object
gcgVIDEOCAPTURE video;

// LOCAL CLASS used for pipeline stage communication
class gcgTASK {
  public:
  gcgIMAGE *currentframe;
  gcgIMAGE *lowpass;

  gcgTASK() {
    currentframe = NULL;
    lowpass = NULL;
  }

  virtual ~gcgTASK() {
    if(currentframe) delete currentframe;
    if(lowpass) delete lowpass;
  }
};


//Called by gcgVIDEOCAPTURE when a new frame arrives.
//It must return as fast as possible.
void videoCaptureCallback(gcgVIDEOCAPTURE *v) {
  gcgIMAGE *image  = new gcgIMAGE();
  v->copyFrameTo(image);

  gcgIMAGE temp;

  // Creates a new task
  gcgTASK  *task = new gcgTASK();
  task->currentframe = image; // Save input frame

  // Low pass filtering
  task->lowpass = new gcgIMAGE();
  temp.convolutionX(task->currentframe, &lowpass); // Low pass in X
  task->lowpass->convolutionY(&temp, &lowpass);    // Low pass in Y

  // Put task in the queue to be consumed by the pipeline: PRODUCTION BLOCKED
  if(!queue.put(task)) delete task; // task could not be passed to the pipeline
}

void atualizaImagens(void) {
  // Gets a new task to be processed: CONSUMPTION with a timeout
  gcgTASK  *task = (gcgTASK*) queue.get();
  if(task) {
    vinput.viewImage(task->currentframe);
    vinput.redisplay();
    delete task;
  } else {
    // Loop
    if(!video.isCapturing()) video.start();
  }
}

void desenhaTudo(gcgIMAGEVIEWER *v) {
  v = NULL;
}


void teclado(unsigned char tecla, int x, int y){
  switch(tecla){
    case 'w':
    case 'W': x++;
              break;

    case 's':
    case 'S': y++;
              break;

    case 27: exit(EXIT_SUCCESS);

    default:  break;
  }
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


//Application cleanup code
void systemexit() {
  // Stop video capture
  printf("\nStopping pipeline...");

  // Unblock all waiting threads and avoid new productions/consumptions
  queue.enable(false);

  video.stop(); // Stop image production
  printf("\n  - video capture down");

  // Flush bridges
  printf("\n  - cleaning bridges\n");

  // Enable consumption to delete produced but not consumed objects
  queue.enable(true);
  while(queue.getCounter() > 0) {
    gcgTASK *task = (gcgTASK*) queue.get();
    if(task) delete task;
  }

  printf("\nPipeline stopped\n\n");
}


int main(int argv, char** argc) {
  // Cleanup callback
  atexit(systemexit);

  // Process signals
  signal(SIGINT, processsignal);
  signal(SIGTERM, processsignal);
#ifndef WIN32
  signal(SIGTSTP, processsignal);
  signal(SIGHUP, processsignal);
#endif

  glutInit(&argv, argc);

  // Enable the report of warnings by GCGLIB: useful while developing
  //gcgEnableReportLog(GCG_WARNING, true);
  //gcgEnableReportLog(GCG_INFORMATION, true);

  printf("\nStarting pipeline...");

  // Blocking 500ms for production and consumption
  queue.setTimeout(500000, 500000);

  video.setCallBackFunction(videoCaptureCallback);
  if(video.openCamera(0, 320, 240, 24, 30)) {
    video.start(); // Start video
    printf("\n  - video capture up: %d  %d %d %f\n", video.width, video.height, video.bpp, video.fps);
  } else {
    printf("\n  - video capture failed");
    exit(EXIT_FAILURE);
  }

  // Creates a preliminar window
  gcgIMAGE *dummy = new gcgIMAGE();
  dummy->createImage(video.width, video.height, video.bpp, false);
  vinput.viewImage(dummy);
  vinput.displayCallback(desenhaTudo);
  delete dummy;

  glutIdleFunc(atualizaImagens);

  glutMainLoop(); //mantêm GLUT em loop, considerando eventos acima
  //
  // gluMainLoop NEVER returns
  //
  return 0;
}
