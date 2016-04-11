#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include <gl/glui.h>


#include "gcg.h"
#include "viewer.h"


//-------------------------------//

// First stage thread
gcgVIDEOCAPTURE     video;
gcgPRODUCERCONSUMER bridge_first_second(16);

// Second stage thread
pthread_t secondthread;
bool runsecondstage = true;

// Queues for image viewing
gcgPRODUCERCONSUMER inputimage(16);    // Input images
gcgPRODUCERCONSUMER outputimage(16);   // Output images
gcgIMAGEVIEWER      vinput;
gcgIMAGEVIEWER      voutput;

//Called by gcgVIDEOCAPTURE when a new frame arrives.
//This is the first stage in our pipeline
void firstStage(gcgVIDEOCAPTURE *v) {
  gcgIMAGE *image  = new gcgIMAGE();
  v->copyFrameTo(image);

  // Process image here
  gcgIMAGE *image2 = new gcgIMAGE();
  image2->duplicateImage(image);  // ONLY COPYING

  // Put processed image in a queue to be consumed by second stage
  if(!bridge_first_second.put(image2)) delete image2; // Image could not be passed to the pipeline

  // Put input image in the viewing queue
  if(!inputimage.put(image)) delete image;
}

//Second stage in processing pipeline.
void* secondStage(void *param){
  // Send to the viewer
  while(runsecondstage) {
    // A new image was taken
    gcgIMAGE *image = (gcgIMAGE*) bridge_first_second.get();

    if(image != NULL) {
      // Process image here
      image->convertGrayScale(image); // Simply convert to gray scale

      // Put image in output viewing queue
      if(!outputimage.put(image)) { delete image; }
    }
  }
  return NULL;
}

//Application cleanup code
void systemexit() {
  void *info;
  gcgIMAGE *image;

  printf("\nStoping pipeline...");

  // Release all waiting threads
  bridge_first_second.setTimeout(0, 0);

  // Stop first stage
  video.stop(); // Stop image production
  printf("\n  - first stage down");

  // Stop second stage
  runsecondstage = false;  // request to stop
  pthread_join(secondthread, (void**) &info); // wait for second thread
  pthread_detach (secondthread); // release second thread
  printf("\n  - second stage down");

  // Flush bridges and stop viewers
  inputimage.setTimeout(0, 0);
  outputimage.setTimeout(0, 0);
  while(inputimage.getCounter() > 0) {
    image = (gcgIMAGE*) inputimage.get();
    if(image) delete image;
  }
  while(outputimage.getCounter() > 0) {
    image = (gcgIMAGE*) outputimage.get();
    if(image) delete image;
  }

  printf("\n  - cleaning bridges\n");
  // Flush all pending images
  while(bridge_first_second.getCounter() > 0) {
    image = (gcgIMAGE*) bridge_first_second.get();
    if(image) delete image;
  }
  printf("\nPipeline stoped\n\n");
}

void atualizaImagens(void) {
  gcgIMAGE *image1 = (gcgIMAGE*) inputimage.get();
  gcgIMAGE *image2 = (gcgIMAGE*) outputimage.get();

  if(image1) {
    vinput.viewImage(image1);
    vinput.redisplay();
    delete image1;
  }

  if(image2) {
      voutput.viewImage(image2);
      voutput.redisplay();
      delete image2;
  }
}



int main(int argv, char** argc) {
  // Cleanup callback
  atexit(systemexit);

  // Enable the report of warnings by GCGLIB: useful while developing
  //gcgEnableReportLog(GCG_WARNING, true);
  //gcgEnableReportLog(GCG_INFORMATION, true);

  inputimage.setTimeout(0, 0);
  outputimage.setTimeout(0, -1); // Timeout in microseconds to block: negative value means infinite.

  printf("\nStarting pipeline...");
  // Brings the second stage up
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if(pthread_create(&secondthread, &attr, secondStage, NULL) == 0) {
    printf("\n  - second stage up");

    // Brings the first stage up
    video.setCallBackFunction(firstStage);
    //if(video.openVideoFile("*.avi")) {
    if(video.openCamera(0, 640, 480, 24, 30)) {
      video.start(); // Start video
      printf("\n  - first stage up: %d  %d %d %lf\n", video.width, video.height, video.bpp, video.fps);
    } else {
      printf("\n  - first stage failed");
      exit(EXIT_FAILURE);
    }
  } else {
    printf("\n  - second stage failed");
    exit(EXIT_FAILURE);
  }

  // Creates a preliminar window
  gcgIMAGE *dummy = new gcgIMAGE();
  dummy->createImage(video.width, video.height, video.bpp, false);
  vinput.viewImage(dummy);
  delete dummy;

  glutIdleFunc(atualizaImagens);

  glutMainLoop();                          //mantêm GLUT em loop, considerando eventos acima
  //
  // gluMainLoop NEVER returns
  //
  return 0;
}
