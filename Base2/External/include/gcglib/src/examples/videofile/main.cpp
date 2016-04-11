#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include <gl/glui.h>

#include "gcg.h"
#include "viewer.h"

// Video input object
gcgVIDEOFILE video;

// Object for viewing images
gcgIMAGEVIEWER      vinput;

// Image for current frame
gcgIMAGE frame;

// Text output
gcgTEXT  output;

// Controls
bool playing = true;
int command = 0;


// Draw information about the video
void displayCallback(gcgIMAGEVIEWER *img) {
    gcgDrawLogo();

   output.fontScale(1.0, 1.0);  // Sets the font scale
   output.textPosition(0, 0);   // Sets the text position within the text box

   glColor3f(1.0, 1.0, 1.0);
   output.setSystemFont(GCG_FONT_TAHOMA_17_NORMAL);  // Selects the font
   clock_t tempo = clock();
   float qps;
   output.gcgprintf("GCGLib %s (%s)\n", GCGLIB_VERSION, GCGLIB_BUILD_DATE);
   output.gcgprintf("Frame: %5i", video.getCurrentPosition()-1);
   output.gcgprintf("\n\nSpace = play/pause");
   output.gcgprintf("\nS = step backward");
   output.gcgprintf("\nD = step forward");
   output.gcgprintf("\nA = first frame");
   output.gcgprintf("\nF = last frame");
}

void showCurrentImage() {
  if(video.copyFrame(&frame)) {
    // Use image in the viewer
  } else {
    playing = false;
    if(gcgGetReport() == GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_VIDEO, GCG_ENDOFSTREAM)) { // 0x1E06A
      printf("\nEnd of video.");
    } else {
      printf("\nAn error occurred reading video...");
      exit(EXIT_FAILURE);
    }
  }
}

void atualizaImagens(void) {
  switch(command) {
    case  1: // Restart
             video.setCurrentPosition(0);
             showCurrentImage();
             break;
    case  2: // Play/pause
             playing = !playing;
             break;
    case  3: // Step forward
             //video.setCurrentPosition(video.getCurrentPosition());
             if(!video.setCurrentPosition(video.getCurrentPosition())) video.setCurrentPosition(video.getFrameCount() - 1);
             showCurrentImage();
             break;
    case  4: // Step backward
             if(!video.setCurrentPosition(video.getCurrentPosition() - 2)) video.setCurrentPosition(0);
             showCurrentImage();
             break;
    case  5: // Restart
             video.setCurrentPosition(video.getFrameCount() - 1);
             showCurrentImage();
             break;
  }
  command = 0;

  if(playing) showCurrentImage();

  vinput.viewImage(&frame);
  vinput.redisplay();
}

void teclado(unsigned char tecla, int x, int y){
  switch(tecla){
    case 's':
    case 'S': command = 4;
              break;
    case 'd':
    case 'D': command = 3;
              break;
    case 'a':
    case 'A': command = 1;
              break;
    case 'f':
    case 'F': command = 5;
              break;
    case ' ': command = 2;
              break;

    case 27:  exit(0);
              break;

    default:  break;
  }
}



// Application cleanup code
void systemexit() {
  printf("\nStoping video...");
  video.destroyVideo();
}


int main(int argv, char** argc) {
  // Cleanup callback
  atexit(systemexit);

  // Enable the report of warnings by GCGLIB: useful while developing
  gcgEnableReportLog(GCG_WARNING, true);
  //gcgEnableReportLog(GCG_INFORMATION, true);

  printf("\nStarting video...");

  //if(video.openVideoFile("M:\\Leitura\\Diverte\\Util\\Video\\Dangereux désirs (2001) (NT1).avi")) {
  //if(video.openVideoFile("actionclipautoautotrain00267.avi")) {
  if(video.openVideoFile("person15_walking_d1_uncomp.avi")) {
    printf("\nVideo information: %d  %d %d %lf\n", video.width, video.height, video.bpp, video.fps);
  } else {
    printf("\nerror opening video ");
    exit(EXIT_FAILURE);
  }

  // Creates a preliminar window
  gcgIMAGE *dummy = new gcgIMAGE();
  dummy->createImage(video.width, video.height, video.bpp, false);
  vinput.viewImage(dummy);
  delete dummy;

  vinput.displayCallback(displayCallback);

  glutKeyboardFunc(teclado);               //chamada cada vez que uma tecla de código ASCII é pressionado
  glutIdleFunc(atualizaImagens);

  glutMainLoop();                          //mantêm GLUT em loop, considerando eventos acima
  //
  // gluMainLoop NEVER returns
  //
  return 0;
}
