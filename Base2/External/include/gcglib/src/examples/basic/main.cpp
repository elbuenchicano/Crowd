#ifdef _MSC_VER
  #include <windows.h>
  #pragma warning(disable: 4996) // Disable deprecation
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glui.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "gcg.h"

gcgFRUSTUM          frustum;
gcgTEXT             output;
gcgEVENTSPERSECOND  framecounter;

//constantes para a GLUT
int     main_window;
int     largura;
int     altura;
int     iniX, iniY;
int     gira = FALSE;
int     translada = FALSE;
float   velocidadeFrente = 0;
float   velocidadeLado = 0;
float   dt = 0;
clock_t tempoant = 0;

/*************************************/
/**** User interface: using GLUI  ****/
/*************************************/
#define CHANGE_PARAMETERS       203

/** Pointers to the windows and some of the controls we'll create **/
GLUI            *glui;
GLUI_Panel      *variables_panel;
GLUI_Panel      *view_panel;

void ConfiguraOpenGL(){
   glClearColor(0.6f,0.6f,0.6f,1.0f); //RGBA
   //glClearColor(0.f,0.f,0.f,1.0f); //RGBA
   glShadeModel(GL_SMOOTH);

   //teste de buffer de profundidade
   //glClearDepth(1.0f);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);

   glEnable(GL_CULL_FACE);
  //glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

// Função callback chamada quando o tamanho da janela é alterado
void alteraTamanhoJanela(GLsizei w, GLsizei h) {
	// Para prevenir uma divisão por zero
	if ( h == 0 ) h = 1;

	// Especifica o tamanho da viewport
	glViewport(0, 0, w, h);

	largura = w;
	altura = h;

  frustum.setPerspective(40, (float) w / (float)h, 0.1, 3000.0);
}

void teclado(unsigned char tecla, int x, int y){
  switch(tecla){
    case 'w':
    case 'W': velocidadeFrente = sqrt(frustum.x*frustum.x + frustum.y*frustum.y + frustum.z*frustum.z);
              break;
    case 's':
    case 'S': velocidadeFrente = -sqrt(frustum.x*frustum.x + frustum.y*frustum.y + frustum.z*frustum.z);
              break;
    case 'a':
    case 'A': velocidadeLado = -sqrt(frustum.x*frustum.x + frustum.y*frustum.y + frustum.z*frustum.z);
              break;
    case 'd':
    case 'D': velocidadeLado = sqrt(frustum.x*frustum.x + frustum.y*frustum.y + frustum.z*frustum.z);
              break;
    case ' ': velocidadeFrente = velocidadeLado = 0;
              break;

    case 27:  x++; y++; exit(EXIT_SUCCESS);
              break;

    default:  break;
  }
}

void tecladoEspecial(int tecla, int x, int y){
  switch(tecla){
        case GLUT_KEY_LEFT:
            break;

        case GLUT_KEY_RIGHT:
            break;

        case GLUT_KEY_DOWN:
            break;

        case GLUT_KEY_UP:
            break;
        default: x++; y++;
            break;
  }
}

// Função callback chamada para gerenciar eventos do mouse
void gerenciaMouse(int button, int state, int x, int y) {
  gira = FALSE;
  translada = FALSE;

  if(state == GLUT_DOWN && button == GLUT_LEFT_BUTTON){
      iniX = x;
      iniY = y;
      gira = TRUE;
  }

  if(state == GLUT_DOWN && button == GLUT_RIGHT_BUTTON){
      iniX = x;
      iniY = y;
      translada = TRUE;
  }

}

// Função callback chamada para gerenciar eventos do mouse
void gerenciaMovimentoMouse(int x, int y) {
  if(gira) {
    frustum.rotateOrbitTrackball(0, 0, 0, (largura - 2.f * iniX) / largura, (altura - 2.f * y) / altura, (largura-2.f * x) / largura, (altura - 2.f * iniY) / altura);
    frustum.setTarget(0.0f, 0.0f, 0.0f);  // Ajusta as direcoes para visualizar o alvo.
    //frustum.alignAxis();
    if (glutGetWindow() != main_window) glutSetWindow(main_window);
    glutPostRedisplay();
  }

  if(translada) {
    frustum.advancePosition(.3*(x - iniX), 0.0, 0.0);
    if (glutGetWindow() != main_window) glutSetWindow(main_window);
    glutPostRedisplay();
  }
  iniX = x;
  iniY = y;
}

void gui_control_cb(int control) {
  switch(control) {
    case CHANGE_PARAMETERS:
                        if(glutGetWindow() != main_window) glutSetWindow(main_window);
                        glutPostRedisplay();
                        break;
  };
}



void desenhaTudo() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  frustum.exportOpenGL();

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

   // Draw text
   //output.enableTextBox(10, 10, 526, 36); // Enable a text box
   //output.drawTextBox(1.0, 1.0, 1.0, 0.2f, 0.0, 0.0, 0.0, 1.f, 1.f); // Draws a framed text box with transparency
   //output.wrapText(FALSE);  // Disables the wrapping functionality
   output.fontScale(1.0, 1.0);  // Sets the font scale
   output.textPosition(0, 0);   // Sets the text position within the text box

   glColor3f(1.0, 1.0, 1.0);
   output.setSystemFont(GCG_FONT_TAHOMA_17_NORMAL);  // Selects the font
   framecounter.finishedEvents(1);
   output.gcgprintf("GCGLib %s (%s)\n", GCG_VERSION, GCG_BUILD_DATE);
   output.gcgprintf("Qps: %5.2f", framecounter.getEventsPerSecond());
   framecounter.startingEvents();

  //Executa os comandos OpenGL e troca os buffers
  gcgDrawLogo();
  glutSwapBuffers();
}

// Funcao de simulacao de tempo
void simulacao(void){
  /* According to the GLUT specification, the current window is
     undefined during an idle callback.  So we need to explicitly change
     it if necessary */
  if(glutGetWindow() != main_window) glutSetWindow(main_window);

  static clock_t tempoini = 0;
	clock_t  tempo;

	tempo = clock();
	dt = ((float) (tempo-tempoini) / (float) CLOCKS_PER_SEC);
	if(dt < EPSILON) return; // Nothing to do
	if(dt > 0.1) dt = 0.1;


  ////////////////////////////////////////////////////
  // Aqui podemos atualizar TODOS os agentes da cena

  // Atualiza a posicao da camera em movimento. Mas só se for necessario.
  //if(fabs(velocidadeFrente) > 0.1 || fabs(velocidadeLado) > 0.1) {
     float x = frustum.x, y = frustum.x, z = frustum.z;  // Save

     frustum.advancePosition((float) velocidadeFrente * dt, 0.0f, (float) velocidadeLado * dt);
     if(sqrt(frustum.x*frustum.x + frustum.y*frustum.y + frustum.z*frustum.z) < 0.5) frustum.setPosition(x, y, z); // restore if too close
     frustum.setTarget(0.f, 0.f, 0.f);  // Ajusta as direcoes para visualizar o alvo.

     velocidadeFrente -= velocidadeFrente * dt * 2;
     velocidadeLado   -= velocidadeLado * dt * 2;

     glutPostRedisplay(); //manda redesenhar a janela
  //}

  ////////////////////////////////////////////////////

  tempoini = tempo;
}

// Application cleanup code
void systemexit() {
  printf("\nStoping application...");
  //if(main_window) glutDestroyWindow(main_window);
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

int main(int argv, char** argc){
  // Cleanup callback
  atexit(systemexit);

  // Process signals
  signal(SIGINT, processsignal);
  signal(SIGTERM, processsignal);
#ifndef WIN32
  signal(SIGTSTP, processsignal);
  signal(SIGHUP, processsignal);
#endif

  // Enable the report of warnings by GCGLIB: useful while developing
  gcgEnableReportLog(GCG_WARNING, true);
  //gcgEnableReportLog(GCG_INFORMATION, true);

  glutInit(&argv, argc);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(640,480);
  glutInitWindowPosition(200,120);

  main_window = glutCreateWindow("GCG - DCC/UFJF - Basic viewer");
  frustum.setPosition(0.0f, 0.f, 50.0f);

  ConfiguraOpenGL();                       //inicializa funções do openGL

  glutReshapeFunc(alteraTamanhoJanela);    //chamada cada vez que um tamanho de janela é modificado
  glutDisplayFunc(desenhaTudo);            //chamada sempre que a janela precisar ser redesenhada
  glutKeyboardFunc(teclado);               //chamada cada vez que uma tecla de código ASCII é pressionado
  glutSpecialFunc(tecladoEspecial);        //chamada cada vez que uma tecla de código não ASCII é pressionado
  glutMotionFunc(gerenciaMovimentoMouse);
  glutMouseFunc(gerenciaMouse);

  /****************************************/
  /*         Here's the GLUI code         */
  /****************************************/
  glui = GLUI_Master.create_glui("Control window - UFJF/DCC/GCG", 0, 400, 50 ); /* name, flags, x, and y */

  // Add invisible panel to hold rest of controls
  GLUI_Panel *mainpanel = glui->add_panel("", GLUI_PANEL_NONE );

  // Simulation variables
  variables_panel = glui->add_panel_to_panel(mainpanel, "Parameters" );
  glui->add_spinner_to_panel(variables_panel, "A:", GLUI_SPINNER_FLOAT, &velocidadeFrente, CHANGE_PARAMETERS, gui_control_cb);
  glui->add_button("Quit", 0, (GLUI_Update_CB) exit);  // A 'quit' button


  GLUI_Master.set_glutIdleFunc(simulacao); //glutIdleFunc(simulacao); //chamada a cada ciclo de glutMainLoop()
  glutMainLoop();                          //mantêm GLUT em loop, considerando eventos acima
//
// glutMainLoop NEVER returns
//
  return 0;
}
