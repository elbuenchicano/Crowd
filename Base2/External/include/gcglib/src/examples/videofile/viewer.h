#ifndef _VIEWER_H_
#define _VIEWER_H_

#include <gcg.h>

// Class for image visualization using GLUT
class gcgIMAGEVIEWER {
  public:
    int           window; // Glut window handler
    gcgFRUSTUM    frustum;
    gcgTEXTURE2D  texture;
    unsigned int  width;
    unsigned int  height;
    float         positionX;
    float         positionY;

    int           iniX, iniY;
    float         escala;
    int           escalar;
    int           transladar;
    void          (*displaycbk)(gcgIMAGEVIEWER *img);
    void          *userpointer; // Application dependent: pointer used by user

  public:
    // Constructor/destructor
    gcgIMAGEVIEWER();
    ~gcgIMAGEVIEWER();

    // View this image
    bool viewImage(gcgIMAGE *srcimage);

    // Redisplay window
    void redisplay();

    // Set extra display function
    void displayCallback(void (*displaycbk)(gcgIMAGEVIEWER *img));
};
#endif

