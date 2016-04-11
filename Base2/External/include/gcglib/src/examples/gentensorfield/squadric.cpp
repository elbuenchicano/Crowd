/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

gcgSUPERQUADRIC
   Marcelo Bernardes Vieira
   Tássio Knop de Castro
**************************************************************************************/

#ifdef WIN32
  #include<windows.h>
#endif

#include <GL/gl.h>
#include <math.h>
#include "superquadric.h"
#include <stdio.h>
#include <stdlib.h>
#include <gcg.h>

float sgnf(float valor) {
    if (valor < 0)
       return -1.0;
    else if (valor > 0)
       return 1.0;
    else return 0.0;
}

/* Returns the absolute value of x */
float absf ( float x ) {
   if ( x < 0 )
      return -x;
   return x;
}

/* sqC (v, n)
 * This function implements the c(v,n) utility function
 *
 * c(v,n) = sgnf(cos(v)) * |cos(v)|^n
 */
float sqC ( float v, float n ) {
   return sgnf((float)cos(v)) * (float)powf(absf((float)cos(v)),n);
}

/* sqS (v, n)
 * This function implements the s(v,n) utility function
 *
 * s(v,n) = sgnf(sin(v)) * |sin(v)|^n
 */
float sqS ( float v, float n ) {
   return sgnf((float)sin(v)) * (float)powf(absf((float)sin(v)),n);
}

gcgSUPERQUADRIC::~gcgSUPERQUADRIC() {
  free(pontos);
}

//constructor
gcgSUPERQUADRIC::gcgSUPERQUADRIC(FLOAT alpha, FLOAT beta, VECTOR3 pos, bool parametrization, FLOAT gamma){
    pointPos = 0;
    this->gamma = gamma;
    if (alpha < 1)this->alpha = pow(alpha,gamma);
      else this->alpha = alpha;
    if (beta < 1) this->beta = pow(beta,gamma);
      else this->beta = beta;
    this->pos[0]   = pos[0];
    this->pos[1]   = pos[1];
    this->pos[2]   = pos[2];
    this->parametrization = parametrization;
}

void gcgSUPERQUADRIC::create(unsigned int slicesphi, unsigned int slicestheta,FLOAT xScale, FLOAT yScale, FLOAT zScale){

    pointPos = 0;
    pontos = (point*) malloc(((slicesphi+slicesphi-2) * (slicestheta + 1) + slicestheta+slicestheta+2) * sizeof(point));
    this->xScale = xScale;
    this->yScale = yScale;
    this->zScale = zScale;

    if (parametrization == FIRST_FORM) createFirstForm(slicesphi, slicestheta);
    else createSecondForm(slicesphi, slicestheta);
}

void gcgSUPERQUADRIC::draw(bool drawMesh, bool drawNormals){
    glEnable(GL_NORMALIZE);
    if(drawNormals){
      glDisable(GL_LIGHTING);
      //glColor3f(1,1,0);
      glBegin(GL_LINES);
      for(int i = 0; i < pointPos; i++){
          //printf("i %d \n",i);
          float mag = sqrt(pontos[i].nx * pontos[i].nx + pontos[i].ny * pontos[i].ny + pontos[i].nz * pontos[i].nz);
          pontos[i].nx /= mag;
          pontos[i].ny /= mag;
          pontos[i].nz /= mag;
          glVertex3f(pontos[i].x,pontos[i].y,pontos[i].z);
          glVertex3f(pontos[i].x + pontos[i].nx/8 ,pontos[i].y + pontos[i].ny/8,pontos[i].z + pontos[i].nz/8);
       }
      glEnd();
  }
  glEnable(GL_LIGHTING);
    if (!drawMesh) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  //draws the wonderful and beautiful and lovely gcg tensor glyphs!
  glBegin(GL_TRIANGLE_STRIP);
  for( int i = 0; i < pointPos; i++){
    glNormal3f(pontos[i].nx,pontos[i].ny,pontos[i].nz);
    glVertex3f(pontos[i].x,pontos[i].y,pontos[i].z);
  }
  glEnd();
}


inline void gcgSUPERQUADRIC::createVertexFirstForm(FLOAT theta, FLOAT phi, point* pontos){
      float     CALPHA =  sqC(theta,alpha)*zScale,
                CBETA  =  sqC(phi,beta) *  xScale,
                SALPHA =  sqS(theta,alpha) * yScale,
                SBETA  =  sqS(phi,beta);
      VECTOR3 normal;
      point p;
      normal[0] = sqC(phi, 2-beta);
      normal[1] = -sqS(phi, 2-beta) * sqS(theta, 2-alpha);
      normal[2] = sqS(phi, 2-beta) * sqC(theta, 2-alpha);

      p.x = CBETA;
      p.y = (- SALPHA * SBETA);
      p.z = CALPHA * SBETA;
      p.nx = normal[0];
      p.ny = normal[1];
      p.nz = normal[2];
      pontos[pointPos] = p;
      pointPos++;

}

void gcgSUPERQUADRIC::createFirstForm(unsigned int slicesphi,unsigned int slicestheta){
  glEnable(GL_NORMALIZE);

  point p;

  FLOAT phi = 0.0, theta = 0.0;
  FLOAT dphi   = M_PI / (float) slicesphi;
  FLOAT dtheta = (M_PI + M_PI) / (float) slicestheta;
  VECTOR3 normal;

  //lets draw the superquadric: one triangle strip for each phi
  for(unsigned int i = 0; i < slicesphi-1; i++, phi += dphi){
    glBegin(GL_TRIANGLE_STRIP);
    theta = 0;
    //this loop draws the most strips
    for(unsigned int j = 0; j < slicestheta; j++, theta += dtheta){
      createVertexFirstForm(theta,phi,pontos);
      createVertexFirstForm(theta,phi+dphi,pontos);
    }

    //this strip garantees the union between the last theta and the first
    float       CALPHA =  zScale,
                CBETA  =  sqC(phi,beta)*xScale,
                SALPHA =  0.f,
                SBETA  =  sqS(phi,beta);

    normal[0] = sqC(phi,2-beta);
    normal[1] = 0.f;
    normal[2] = sqS(phi,2-beta);

    glNormal3f(normal[0],normal[1],normal[2]);
    glVertex3f(CBETA, 0.f, CALPHA * SBETA);

    p.x = CBETA;
    p.y =  0;
    p.z = CALPHA * SBETA;
    p.nx = normal[0];
    p.ny = normal[1];
    p.nz = normal[2];
    pontos[pointPos] = p;
    pointPos++;

    CALPHA =  zScale;
    CBETA  =  sqC(phi+dphi,beta)*xScale,
    SALPHA =  0.f;
    SBETA  =  sqS(phi+dphi,beta);

    normal[0] = sqC(phi+dphi, 2-beta);
    normal[1] = 0.f;
    normal[2] = sqS(phi+dphi, 2-beta);

    glNormal3f(normal[0],normal[1],normal[2]);
    glVertex3f(CBETA, 0.f, CALPHA * SBETA);
    p.x = CBETA;
    p.y = 0;
    p.z = CALPHA * SBETA;
    p.nx = normal[0];
    p.ny = normal[1];
    p.nz = normal[2];
    pontos[pointPos] = p;
    pointPos++;
    glEnd();
   }

  secondStrip = pointPos;
  //this strip garantees that the last phi is exactly PI
  glBegin(GL_TRIANGLE_STRIP);
  theta = 0;
  for(unsigned int j = 0; j < slicestheta; j++, theta += dtheta){
    createVertexFirstForm(theta,phi,pontos);
    float     CALPHA =  sqC(theta,alpha)*zScale,
              CBETA  =  -xScale,
              SALPHA =  sqS(theta,alpha) * yScale,
              SBETA  =  0.f;

    normal[0] = -1;
    normal[1] = 0;
    normal[2] = 0;

    glNormal3f(normal[0],normal[1],normal[2]);
    glVertex3f(CBETA, 0.f, CALPHA * SBETA);

    p.x = CBETA;
    p.y = 0.f;
    p.z = CALPHA * SBETA;
    p.nx = normal[0];
    p.ny = normal[1];
    p.nz = normal[2];
    pontos[pointPos] = p;
    pointPos++;
  }

  //this strip garantees the union between the last theta and the first
  float     CALPHA =  zScale,
            CBETA  =  sqC(phi,beta) * xScale,
            SALPHA =  0.f,
            SBETA  =  sqS(phi,beta);

  normal[0] = sqC(phi,2-beta);
  normal[1] = 0.f;
  normal[2] = sqS(phi,2-beta);

  glNormal3f(normal[0],normal[1],normal[2]);
  glVertex3f(CBETA, 0.f, CALPHA * SBETA);

  p.x = CBETA;
  p.y = 0.f;
  p.z = CALPHA * SBETA;
  p.nx = normal[0];
  p.ny = normal[1];
  p.nz = normal[2];
  pontos[pointPos] = p;
  pointPos++;

  CALPHA =  zScale;
  CBETA  =  -xScale,
  SALPHA =  0;
  SBETA  =  0;

  normal[0] = -1;
  normal[1] = 0;
  normal[2] = 0;

  glNormal3f(normal[0],normal[1],normal[2]);
  glVertex3f(CBETA, - SALPHA * SBETA, CALPHA * SBETA);
  p.x = CBETA;
  p.y = - SALPHA * SBETA;
  p.z = CALPHA * SBETA;
  p.nx = normal[0];
  p.ny = normal[1];
  p.nz = normal[2];
  pontos[pointPos] = p;
  pointPos++;
  glEnd();

}

inline void gcgSUPERQUADRIC::createVertexSecondForm(FLOAT theta, FLOAT phi, point *pontos){
      float     CALPHA =  sqC(theta,alpha)* xScale,//sgnf(cos(theta) )   * pow( fabs( cos(theta)   ), alpha),
                CBETA  =  sqC(phi,beta)*zScale,//sgnf(cos(phi)) * pow( fabs( cos(phi)), beta ),
                SALPHA =  sqS(theta,alpha) * yScale,//sgnf(sin(theta) )   * pow( fabs( sin(theta)   ), alpha),
                SBETA  =  sqS(phi,beta) ;//sgnf(sin(phi)) * pow( fabs( sin(phi)), beta );
      VECTOR3 normal;
      point p;
      normal[0] = sqS(phi,2-beta) * sqC(theta, 2-alpha);
      normal[1] = sqS(phi,2-beta) * sqS(theta, 2-alpha);
      normal[2] = sqC(phi,2-beta);

      //glNormal3f(normal[0],normal[1],normal[2]);
      //glVertex3f(CALPHA * SBETA, SALPHA * SBETA, CBETA);
      p.x = CALPHA * SBETA;
      p.y = SALPHA * SBETA;
      p.z = CBETA;
      p.nx = normal[0];
      p.ny = normal[1];
      p.nz = normal[2];
      pontos[pointPos] = p;
      pointPos++;
}



void gcgSUPERQUADRIC::createSecondForm(unsigned int slicesphi,unsigned int slicestheta){

  glEnable(GL_NORMALIZE);

  FLOAT phi = 0.0, theta = 0.0;
  FLOAT dphi   = M_PI / (float) slicesphi;
  FLOAT dtheta = (M_PI + M_PI) / (float) slicestheta;
  VECTOR3 normal;
  point p;

  //lets draw the superquadric: one triangle strip for each phi
  for(unsigned int i = 0; i < slicesphi-1; i++, phi += dphi){
    glBegin(GL_TRIANGLE_STRIP);
    theta = 0;
    for(unsigned int j = 0; j < slicestheta; j++, theta += dtheta){
      createVertexSecondForm(theta,phi,pontos);
      createVertexSecondForm(theta,phi+dphi,pontos);
    }

    //this strip garantees the union between the last theta and the first
      float    CALPHA =  xScale,
               CBETA  =  sqC(phi,beta)*zScale,
               SALPHA =  0,
               SBETA  =  sqS(phi,beta);

      normal[0] = sqS(phi,2-beta);
      normal[1] = 0;
      normal[2] = sqC(phi,2-beta);

      p.x = CALPHA * SBETA;
      p.y = (SALPHA * SBETA);
      p.z = CBETA;
      p.nx = normal[0];
      p.ny = normal[1];
      p.nz = normal[2];
      pontos[pointPos] = p;
      pointPos++;

      CALPHA =  xScale,
      CBETA  =  sqC(phi+dphi,beta) * zScale,
      SALPHA =  0,
      SBETA  =  sqS(phi+dphi,beta);

      normal[0] = sqS(phi+dphi, 2-beta);
      normal[1] = 0;
      normal[2] = sqC(phi+dphi, 2-beta);

      //glNormal3f(normal[0],normal[1],normal[2]);
      //glVertex3f(CALPHA * SBETA, SALPHA * SBETA, CBETA);
      p.x = CALPHA * SBETA;
      p.y = (SALPHA * SBETA);
      p.z = CBETA;
      p.nx = normal[0];
      p.ny = normal[1];
      p.nz = normal[2];
      pontos[pointPos] = p;
      pointPos++;
      glEnd();
  }

    secondStrip = pointPos;
    //this strip garantees that the last phi is exactly PI
    glBegin(GL_TRIANGLE_STRIP);
    theta = 0;
    for(unsigned int j = 0; j < slicestheta; j++, theta += dtheta){
      createVertexSecondForm(theta,phi,pontos);
      float     CALPHA =  sqC(theta,alpha) * xScale,
                CBETA  =  -zScale,
                SALPHA =  sqS(theta,alpha) * yScale,
                SBETA  =  0;

      normal[0] = 0;
      normal[1] = 0;
      normal[2] = -1;

      p.x = CALPHA * SBETA;
      p.y = (SALPHA * SBETA);
      p.z = CBETA;
      p.nx = normal[0];
      p.ny = normal[1];
      p.nz = normal[2];
      pontos[pointPos] = p;
      pointPos++;

    }

       //this strip garantees the union between the last theta and the first
      float   CALPHA =  xScale,
              CBETA  =  sqC(phi, beta) * zScale,
              SALPHA =  0,
              SBETA  =  sqS(phi,beta);

      normal[0] = sqS(phi,2-beta);
      normal[1] = 0;
      normal[2] = sqC(phi,2-beta);

      p.x = CALPHA * SBETA;
      p.y = (SALPHA * SBETA);
      p.z = CBETA;
      p.nx = normal[0];
      p.ny = normal[1];
      p.nz = normal[2];
      pontos[pointPos] = p;
      pointPos++;

      CALPHA =  xScale,
                CBETA  =  -zScale,
                SALPHA =  0,
                SBETA  =  0;

      normal[0] = 0;
      normal[1] = 0;
      normal[2] = -1;

      p.x = CALPHA * SBETA;
      p.y = (SALPHA * SBETA);
      p.z = CBETA;
      p.nx = normal[0];
      p.ny = normal[1];
      p.nz = normal[2];
      pontos[pointPos] = p;
      pointPos++;
      glEnd();
}

