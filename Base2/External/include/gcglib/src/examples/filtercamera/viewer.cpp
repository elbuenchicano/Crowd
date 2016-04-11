#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
  #include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <gcg.h>
#include "viewer.h"

void drawImage();
void reshapeWindow(GLsizei w, GLsizei h);
void keyboard(unsigned char tecla, int x, int y);
void mouseClick(int button, int state, int x, int y);
void mouseMotionButton(int x, int y);
void mouseMotion(int x, int y);


// Needed to control events in multiple windows
class gcgHANDLELIST {
  private:
    typedef struct _HANDLEOBJ {
      int  handle;     // Handle number: must be unique for an object
      void *object;    // Object pointer: must be unique for a handle
      struct _HANDLEOBJ *next;
    } HANDLEOBJ;

    HANDLEOBJ *handlelist; // handle linked list
    //pthread_mutex_t mutex_list;  // Mutex for list access with mutual exclusion

  public:
    gcgHANDLELIST();
    ~gcgHANDLELIST();

    // Basic operations on the list
    bool  insert(int handle, void *object);
    bool  removeHandle(int  handle);
    bool  removeObject(void *object);
    void* getObject(int handle);
    int   getHandle(void *object);
};

// Constructing a handle list
gcgHANDLELIST::gcgHANDLELIST() {
  handlelist = NULL;
}

gcgHANDLELIST::~gcgHANDLELIST() {
  // Free the linked list
  while(handlelist != NULL) {
    HANDLEOBJ *p = handlelist;
    handlelist = handlelist->next;
    free(p);
  }
}

// Handle/object pair insertion
bool gcgHANDLELIST::insert(int handle, void *object) {
  HANDLEOBJ *nhandle = (HANDLEOBJ*) malloc(sizeof(HANDLEOBJ));
  if(nhandle == NULL) return false;

  // Created
  nhandle->handle = handle;
  nhandle->object = object;

  // Insert in list
  nhandle->next = handlelist;
  handlelist = nhandle;
  return true;
}

void *gcgHANDLELIST::getObject(int handle) {
  HANDLEOBJ *p = handlelist;
  // Span the list
  while(p != NULL) {
    if(p->handle == handle) return p->object;
    p = p->next;
  }
  return NULL;
}

gcgHANDLELIST viewers;

// Constructing the viewer
gcgIMAGEVIEWER::gcgIMAGEVIEWER() {
  window = 0;
  frustum.setPosition(0.0f, 0.f, 50.0f);

  width = 0;
  height = 0;
  iniX = 0;
  iniY = 0;
  escala = 1.f;
  escalar = false;
  transladar = false;
  displaycbk = NULL;
  userpointer = NULL;
}

// Viewer destructor
gcgIMAGEVIEWER::~gcgIMAGEVIEWER() {
  //if(window != 0) glutDestroyWindow(window);
  texture.destroyTexture();
}

// View this image
bool gcgIMAGEVIEWER::viewImage(gcgIMAGE *srcimage) {
  // Check image
  if(srcimage == NULL) return false;
  if(srcimage->data == NULL || srcimage->width == 0 || srcimage->height == 0 || srcimage->bpp == 0) return false;

  // Check the window is created and makes it current
  int oldwindow = glutGetWindow();
  if(window == 0) {
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(srcimage->width, srcimage->height);
    glutInitWindowPosition(200, 120);

    window = glutCreateWindow("GCG image viewer");//glutCreateSubWindow(janela, 0, 0, srcimage->width, srcimage->height);
    this->texture.destroyTexture();

    // OpenGL configuration
    glClearColor(0.5f, 0.5f, 0.5f,1.0f); //RGBA
    glShadeModel(GL_SMOOTH);

    //glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glutReshapeFunc(reshapeWindow);     //chamada cada vez que um tamanho de janela é modificado
    glutDisplayFunc(drawImage);         //chamada sempre que a janela precisar ser redesenhada
    glutKeyboardFunc(keyboard);        //chamada cada vez que uma tecla de código ASCII é pressionado
    glutMotionFunc(mouseMotionButton);
    glutMouseFunc(mouseClick);
    glutPassiveMotionFunc(mouseMotion);

    viewers.insert(window, this);
  }
  if(window) glutSetWindow(window);

  if(!this->texture.isCompatibleWith(srcimage)) this->texture.destroyTexture();
  this->texture.uploadImage(srcimage);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  if(oldwindow) glutSetWindow(oldwindow);
  return true;
}

// Redisplay this image
void gcgIMAGEVIEWER::redisplay() {
  if(window != 0) {
    int oldwindow = glutGetWindow();
    glutSetWindow(window);
    glutPostRedisplay();
    if(oldwindow) glutSetWindow(oldwindow);
  }
}

// Set display callback function
void gcgIMAGEVIEWER::displayCallback(void (*displaycbk)(gcgIMAGEVIEWER *img)) {
  if(window != 0) this->displaycbk = displaycbk;
}

void drawImage() {
  // What is the viewer object?
  gcgIMAGEVIEWER *imageview = (gcgIMAGEVIEWER*) viewers.getObject(glutGetWindow());
  if(imageview == NULL) return;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  imageview->frustum.exportOpenGL();

  // Desenha eixos
   glDisable(GL_TEXTURE_2D);
   glBegin(GL_LINES);
     glColor3f(0.0f, 1.0f, 0.0f);
     glVertex3f(0.0f, 0.0f,0.0f);
     glVertex3f(0.0f, 10.0f,0.0f);
     glColor3f(1.0f, 0.0f, 0.0f);
     glVertex3f(0.0f,0.0f,0.0f);
     glVertex3f(10.0f,0.0f,0.0f);
     glColor3f(0.0f, 0.0f, 1.0f);
     glVertex3f(0.0f,0.0f,0.0f);
     glVertex3f(0.0f,0.0f,10.0f);
   glEnd();

   // Draw image
   glColor3f(1.f,1.f,1.f);
   glEnable(GL_TEXTURE_2D);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
    imageview->texture.bind();
    glTranslatef(-(float) imageview->texture.width / 2.f, - (float) imageview->texture.height / 2.f, 0);
    glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f,  0.0f);
      glTexCoord2f(1.0, 0.0f);  glVertex3f((float) imageview->texture.width, 0.0f,  0.0f);
      glTexCoord2f(1.0, 1.0);   glVertex3f((float) imageview->texture.width,  (float) imageview->texture.height,  0.0f);
      glTexCoord2f(0.0f, 1.0);  glVertex3f(0.0f,  (float) imageview->texture.height,  0.0f);
    glEnd();
    imageview->texture.unbind();

    // Image was drawn, call display funcion
    if(imageview->displaycbk != NULL) imageview->displaycbk(imageview);
   glPopMatrix();

  //Executa os comandos OpenGL e troca os buffers
  glutSwapBuffers();
}

// Função callback chamada quando o tamanho da janela é alterado
void reshapeWindow(GLsizei w, GLsizei h) {
  // What is the viewer object?
  gcgIMAGEVIEWER *imageview = (gcgIMAGEVIEWER*) viewers.getObject(glutGetWindow());
  if(imageview == NULL) return;

	// Para prevenir uma divisão por zero
	if(h == 0) h = 1;

	// Especifica o tamanho da viewport
	glViewport(0, 0, w, h);

	imageview->width = w;
	imageview->height = h;

  imageview->frustum.setOrthographic(-(float) imageview->width * imageview->escala / 2.0f, (float) imageview->width * imageview->escala /2.0f,
                                    (float) imageview->height * imageview->escala /2.0f, -(float) imageview->height * imageview->escala /2.0f, -100.f, 100.f);

}


void keyboard(unsigned char tecla, int x, int y){
  // What is the viewer object?
  gcgIMAGEVIEWER *imageview = (gcgIMAGEVIEWER*) viewers.getObject(glutGetWindow());
  if(imageview == NULL) return;

  switch(tecla){
    case ' ': imageview->escala = 1.0;
              imageview->frustum.setPosition(0.f, 0.f, 100.f);
              glutReshapeWindow(imageview->texture.width, imageview->texture.height);
              imageview->frustum.setOrthographic(-(float) imageview->width * imageview->escala / 2.0f, (float) imageview->width * imageview->escala /2.0f,
                                    (float) imageview->height * imageview->escala /2.0f, -(float) imageview->height * imageview->escala /2.0f, -100.f, 100.f);

              glutPostRedisplay();
              break;

    case 27: x++; y++; exit(EXIT_SUCCESS);
    default:  break;
  }
}


// Função callback chamada para gerenciar eventos do mouse
void mouseClick(int button, int state, int x, int y) {
  // What is the viewer object?
  gcgIMAGEVIEWER *imageview = (gcgIMAGEVIEWER*) viewers.getObject(glutGetWindow());
  if(imageview == NULL) return;

  imageview->escalar = FALSE;
  imageview->transladar = FALSE;

  if(state == GLUT_DOWN && button == GLUT_LEFT_BUTTON){
      imageview->iniX       = x;
      imageview->iniY       = y;
      imageview->transladar = TRUE;
      glutSetCursor(GLUT_CURSOR_INFO);
  }

  if(state == GLUT_DOWN && button == GLUT_RIGHT_BUTTON){
      imageview->iniX = x;
      imageview->iniY = y;
      imageview->escalar = TRUE;
      glutSetCursor(GLUT_CURSOR_LEFT_RIGHT);
  }

  if(state == GLUT_UP) glutSetCursor(GLUT_CURSOR_INHERIT);

  imageview->positionX = (x - (int) (imageview->width >> 1)) * imageview->escala + imageview->frustum.x + imageview->texture.width / 2;
  imageview->positionY = ((int) (imageview->height >> 1) - y) * imageview->escala + imageview->frustum.y + imageview->texture.height / 2;
}

// Função callback chamada para gerenciar eventos do mouse SEM botao pressionado
void mouseMotion(int x, int y) {
  // What is the viewer object?
  gcgIMAGEVIEWER *imageview = (gcgIMAGEVIEWER*) viewers.getObject(glutGetWindow());
  if(imageview == NULL) return;

  imageview->positionX = (x - (int) (imageview->width >> 1)) * imageview->escala + imageview->frustum.x + imageview->texture.width / 2;
  imageview->positionY = ((int) (imageview->height >> 1) - y) * imageview->escala + imageview->frustum.y + imageview->texture.height / 2;
}


// Função callback chamada para gerenciar eventos do mouse COM botao pressionado
void mouseMotionButton(int x, int y) {
  // What is the viewer object?
  gcgIMAGEVIEWER *imageview = (gcgIMAGEVIEWER*) viewers.getObject(glutGetWindow());
  if(imageview == NULL) return;

  if(imageview->transladar)
    imageview->frustum.advancePosition(0, (y-imageview->iniY) * imageview->escala, (imageview->iniX - x) * imageview->escala); // Avanca usando as direcoes de visualizacao

  if(imageview->escalar) {
    imageview->escala += imageview->escala * (float) (imageview->iniX - x) / 25;
    if(imageview->escala < 0.01f) imageview->escala = 0.01f;
    if(imageview->escala > 20.f) imageview->escala = 20.0f;
  }

  if(imageview->escalar || imageview->transladar) {
    int mask = 0;
    if(imageview->frustum.x < -(float) imageview->texture.width/2.0) {imageview->frustum.setPosition(-(float) imageview->texture.width/2.0f, (float) imageview->frustum.y, 1.0f); mask |= 0x01;}
    if(imageview->frustum.x >  (float) imageview->texture.width/2.0) {imageview->frustum.setPosition((float) imageview->texture.width/2.0f, (float) imageview->frustum.y, 1.0f); mask |= 0x02;}
    if(imageview->frustum.y < -(float) imageview->texture.height/2.0) {imageview->frustum.setPosition(imageview->frustum.x, -(float) imageview->texture.height/2.0f, 1.0f); mask |= 0x4;}
    if(imageview->frustum.y >  (float) imageview->texture.height/2.0) {imageview->frustum.setPosition(imageview->frustum.x, (float) imageview->texture.height/2.0f, 1.0f); mask |= 0x8;}
    imageview->frustum.setOrthographic(-(float) imageview->width * imageview->escala / 2.0f, (float) imageview->width * imageview->escala / 2.0f,
                                        (float) imageview->height * imageview->escala / 2.0f, -(float) imageview->height * imageview->escala / 2.0f, -100.f, 100.f);
    if(imageview->transladar) {
      // Update cursor
      switch(mask) {
        case 0x1: glutSetCursor(GLUT_CURSOR_RIGHT_SIDE); break;
        case 0x2: glutSetCursor(GLUT_CURSOR_LEFT_SIDE); break;
        case 0x4: glutSetCursor(GLUT_CURSOR_BOTTOM_SIDE); break;
        case 0x8: glutSetCursor(GLUT_CURSOR_TOP_SIDE); break;
        case 0x1 | 0x4: glutSetCursor(GLUT_CURSOR_TOP_RIGHT_CORNER); break;
        case 0x1 | 0x8: glutSetCursor(GLUT_CURSOR_BOTTOM_RIGHT_CORNER); break;
        case 0x2 | 0x4: glutSetCursor(GLUT_CURSOR_TOP_LEFT_CORNER); break;
        case 0x2 | 0x8: glutSetCursor(GLUT_CURSOR_BOTTOM_LEFT_CORNER); break;
        default: glutSetCursor(GLUT_CURSOR_INFO); break;
      }
    }
    glutPostRedisplay();
  }

  imageview->iniX = x;
  imageview->iniY = y;
  imageview->positionX = (x - (int) (imageview->width >> 1)) * imageview->escala + imageview->frustum.x + imageview->texture.width / 2;
  imageview->positionY = ((int) (imageview->height >> 1) - y) * imageview->escala + imageview->frustum.y + imageview->texture.height / 2;
}

