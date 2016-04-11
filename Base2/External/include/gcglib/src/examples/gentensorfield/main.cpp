#ifdef WIN32
  #include<windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glui.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <gcg.h>
#include "generator.cpp"

#define DIMX 32
#define DIMY 32
#define DIMZ 1


int selX = DIMX / 2;
int selY = DIMY / 2;
int selZ = DIMZ / 2;

gcgFRUSTUM probe;

gcgTENSORFIELDGENERATOR<float> field;

gcgFRUSTUM frustum;
gcgTEXT    output;

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


// Define parametros da luz
GLfloat cor_luz[]	= { 1.0f, 1.0f, 1.0f, 1.0};
GLfloat cor_luz_amb[]	= { 0.02, 0.02, 0.02, 0.02};
GLfloat posicao_luz[]	= { 3.0, 5.0, 5.0, 1.0};
GLfloat cor_fonte_luz[]	= { 1.0, 0.0, 0.0, 1.0};
GLfloat cor_emissao[]	= { 0.2, 0.2, 0.2, 1.0 };


int labels[DIMX*DIMY*DIMZ];
float cores[5][3] = {{1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}};

void loadLabels(char *filename) {
  FILE *arq;

  arq = fopen(filename, "rb");
  if(arq != NULL) {
    fread(labels, 1, DIMX * DIMY * DIMZ * sizeof(int), arq);
    fclose(arq);
  }
  for(int i =0; i < DIMX * DIMY * DIMZ; i++) printf("%d\n", labels[i]);
}


/////////////////////////////////////////////////////////////////////////////////////
// Calcula a quantidade de quadros pos segundo dadas as variacoes de tempo. Atualiza
// a cada 500ms.
/////////////////////////////////////////////////////////////////////////////////////
float CalculaQuadrosSegundo(unsigned int dt) {
	static float        ultimoqps = 0.0f;	// Guarda o ultimo qps calculado
	static unsigned int contaquadro  = 0;	// Conta o numero de quadros a cada chamada
  static unsigned int tempopassado = 0;	// Armazena o tempo do ultimo quadro

	// Incrementa o contador
  contaquadro++;
	tempopassado += dt;

	// Testa se o tempo passado soma 1 segundo
  if(tempopassado > 1000) {
		// Calcula quadros por segundo
		ultimoqps = ((float) contaquadro / (float) tempopassado) * 1000.f;

		// Zera contador de tempo
		tempopassado = 0;

		// Zera contador de quadros
    contaquadro = 0;
  }

	// Retorna quadros por segundo
	return ultimoqps;
}


void ConfiguraOpenGL(){
   glClearColor(0.6f,0.6f,0.6f,1.0f); //RGBA
   //glClearColor(0.f,0.f,0.f,1.0f); //RGBA
   glShadeModel(GL_SMOOTH);

   //teste de buffer
   //glClearDepth(1.0f);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);

   glEnable(GL_CULL_FACE);
  //glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);


   glEnable(GL_LIGHTING);                 // Habilita luz
   glEnable(GL_LIGHT0);                   // habilita luz 0

   // Define parametros da luz 0
   glLightfv(GL_LIGHT0, GL_AMBIENT, cor_luz_amb);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, cor_luz);
   glLightfv(GL_LIGHT0, GL_SPECULAR, cor_luz);
   glLightfv(GL_LIGHT0, GL_POSITION, posicao_luz);

   //glColorMaterial ( GL_FRONT_AND_BACK, GL_EMISSION ) ;
   glEnable ( GL_COLOR_MATERIAL ) ;
}

// Função callback chamada quando o tamanho da janela é alterado
void alteraTamanhoJanela(GLsizei w, GLsizei h) {
	// Para previnir uma divisão por zero
	if ( h == 0 ) h = 1;

	// Especifica o tamanho da viewport
	glViewport(0, 0, w, h);

	largura = w;
	altura = h;

  frustum.setPerspective(40, (float) w / (float)h, 0.1, 3000.0);
}

void teclado(unsigned char tecla, int x, int y){
  switch(tolower(tecla)) {
    case 'q': selX++;
              if(selX > DIMX - 1) selX = DIMX - 1;
              break;
    case 'a': selX--;
              if(selX < 0) selX = 0;
              break;
    case 'w': selY++;
              if(selY > DIMY - 1) selY = DIMY - 1;
              break;
    case 's': selY--;
              if(selY < 0) selY = 0;
              break;
    case 'e': selZ++;
              if(selZ > DIMZ - 1) selZ = DIMZ - 1;
              break;
    case 'd': selZ--;
              if(selZ < 0) selZ = 0;
              break;

    case 27:  exit(0);
              break;

    default:  break;
  }
}

void tecladoEspecial(int tecla, int x, int y){
  switch(tecla){
        case GLUT_KEY_LEFT:   probe.rotateOrientationSpherical(-1.0, 0.0);     // Free rotation of the planetarium using spherical angles
                              break;

        case GLUT_KEY_RIGHT: probe.rotateOrientationSpherical(1.0, 0.0);     // Free rotation of the planetarium using spherical angles
                              break;

        case GLUT_KEY_DOWN:   probe.rotateOrientationSpherical(0.0, -1.0);     // Free rotation of the planetarium using spherical angles
                              break;

        case GLUT_KEY_UP:  probe.rotateOrientationSpherical(0.0, 1.0);     // Free rotation of the planetarium using spherical angles
                              break;

        default:
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
  int     i = 0;
  float   angulo;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  frustum.setTarget((float) selX - DIMX * 0.5, selY - DIMY * 0.5, selZ - DIMZ * 0.5);
  frustum.exportOpenGL();


   glDisable(GL_CULL_FACE);

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

   VECTOR3 direction;
   VECTOR3 trans;

   #define VECTOR_MAGNITUDE 15

   gcgSCALEVECTOR3(direction, probe.view, VECTOR_MAGNITUDE);

   glColor3f(1.0, 1.0, 1.0);
   gcgAPPLYMATRIX3VECTOR3(trans, field.getTensor(selX, selY, selZ), direction);
   gcgDrawVectorThetrahedronClosed(selX - DIMX / 2.0, selY - DIMY / 2.0, selZ - DIMZ / 2.0, trans, gcgLENGTHVECTOR3(trans));

   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

   // Desenha vetor original
   glColor3f(1.0, 0.0, 0.0);
   gcgDrawVectorThetrahedron(selX - DIMX / 2.0, selY - DIMY / 2.0, selZ - DIMZ / 2.0, direction, VECTOR_MAGNITUDE);

   // Desenha selecao
   glColor3f(1.0f, 1.0f, 1.0f);
   VECTOR3 bbmin = { selX - 0.5 - DIMX / 2.0, selY - 0.5 - DIMY / 2.0, selZ - 0.5 - DIMZ / 2.0};
   VECTOR3 bbmax = { selX + 0.5 - DIMX / 2.0, selY + 0.5 - DIMY / 2.0, selZ + 0.5 - DIMZ / 2.0};
   gcgDrawAABox(bbmin, bbmax);

   glEnable(GL_CULL_FACE);
   field.draw();

   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

   // Desenha rotulos
   for(int i = 0; i < DIMX; i++)
    for(int j = 0; j < DIMY; j++)
      for(int k = 0; k < DIMZ; k++) {
         int pos = labels[(((k) * DIMX * DIMY + (j) * DIMX + (i)))];
         glColor3f(cores[pos][0], cores[pos][1], cores[pos][2]);
         VECTOR3 bbmin = { i - 0.5 - DIMX / 2.0, j - 0.5 - DIMY / 2.0, k - 0.5 - DIMZ / 2.0};
         VECTOR3 bbmax = { i + 0.5 - DIMX / 2.0, j + 0.5 - DIMY / 2.0, k + 0.5 - DIMZ / 2.0};
         gcgDrawAABox(bbmin, bbmax);
      }


      // Draw text
   //output.enableTextBox(10, 10, 526, 36); // Enable a text box
   //output.drawTextBox(1.0, 1.0, 1.0, 0.2f, 0.0, 0.0, 0.0, 1.f, 1.f); // Draws a framed text box with transparency
   //output.wrapText(FALSE);  // Disables the wrapping functionality
   output.fontScale(1.0, 1.0);  // Sets the font scale
   output.textPosition(0, 0);   // Sets the text position within the text box

   glColor3f(1.0, 1.0, 1.0);
   output.setSystemFont(GCG_FONT_TAHOMA_17_NORMAL);  // Selects the font
   clock_t tempo = clock();
   float qps;
   output.gcgprintf("GCGLib %s (%s)\n", GCG_VERSION, GCG_BUILD_DATE);
   output.gcgprintf("Qps: %5.2f", qps = CalculaQuadrosSegundo(((tempo-tempoant) * 1000) / CLOCKS_PER_SEC));
   tempoant = tempo;


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
  if(main_window) glutDestroyWindow(main_window);
}


int main(int argv, char** argc){
    // Cleanup callback
    atexit(systemexit);

    float points[] = {DIMX/4.0, DIMY/4.0, 0, (DIMX-1) - DIMX / 4.0, DIMY/4.0, 0, DIMX/2.0, (DIMY-1) - DIMY/4.0, 0};
    field.generate(DIMX, DIMY, DIMZ, 3, points);
    loadLabels("outputlabels_2levels_10_4_2000it.bin");
    field.save("field128.bin");

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
    glutIdleFunc(simulacao);

    glutMainLoop(); // NEVER returns
    return 0;
}
