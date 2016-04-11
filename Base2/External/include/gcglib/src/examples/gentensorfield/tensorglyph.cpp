/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

gcgTENSORGLYPH
   Marcelo Bernardes Vieira
   Tássio Knop de Castro
**************************************************************************************/
#ifdef WIN32
  #include<windows.h>
#endif

#include "tensorglyph.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>


//obtains alpha and beta from a tensor, extracting its eigenvalues and computing
//its anisotropy components (linear, planar, spherical)
//if linear >= planar, we use the first parametrization; otherwise we use the second
gcgTENSORGLYPH::gcgTENSORGLYPH(MATRIX3 tensor, VECTOR3 pos, FLOAT confidence, FLOAT gamma){
    gcgCOPYMATRIX3(this->tensor,tensor);
    gcgEigenSymmetricMatrix3(tensor,eigenVectors,eigenValues);
    //eigenvalues sorted in decreasing order, but this function returns them in increasing order
    //obtaining eigenvectors
    FLOAT aux = eigenValues[0];
    eigenValues[0] = eigenValues[2];
    eigenValues[2] = aux;
    VECTOR3 vaux = {eigenVectors[0],eigenVectors[1],eigenVectors[2]};
    eigenVectors[0] = eigenVectors[6];
    eigenVectors[1] = eigenVectors[7];
    eigenVectors[2] = eigenVectors[8];
    gcgCROSSVECTOR3(&eigenVectors[6], eigenVectors, &eigenVectors[3]);

    this->gamma = gamma;
    this->pos[0]   = pos[0];
    this->pos[1]   = pos[1];
    this->pos[2]   = pos[2];
    this->confidence = confidence;

    //computing the tensor's weight
    weight =gcgLENGTHVECTOR3(eigenValues);

    //anisotropy components
    cl = (eigenValues[0] - eigenValues[1])/ (eigenValues[0] + eigenValues[1] + eigenValues[2]);
    cp = ((eigenValues[1] - eigenValues[2]) + (eigenValues[1] - eigenValues[2]))/ (eigenValues[0] + eigenValues[1] + eigenValues[2]);
    cs = (eigenValues[2] + eigenValues[2] + eigenValues[2])/ (eigenValues[0] + eigenValues[1] + eigenValues[2]);

    //making the eigenvalues vary from 0 to 1
    gcgNORMALIZEVECTOR3(eigenValues,eigenValues);

    FLOAT alpha, beta;
    if (cl >= cp){
        parametrization = FIRST_FORM;
        alpha = 1 - cp;
        alpha = pow(alpha,gamma); //sharpening alpha
        beta = 1 - cl;
        beta = pow(beta,gamma);   //sharpening beta
    }
    else{
        parametrization = SECOND_FORM;
        alpha = 1 - cl;
        alpha = pow(alpha,gamma); //sharpening alpha
        beta = 1 - cp;
        beta = pow(beta,gamma);   //sharpening beta
    }
    if (alpha < MIN_ALPHA) alpha = MIN_ALPHA;
    if (beta < MIN_BETA) beta = MIN_BETA;
    sq = new gcgSUPERQUADRIC(alpha,beta,pos,parametrization,gamma);
}

gcgTENSORGLYPH::~gcgTENSORGLYPH(){
    delete sq;
}

void gcgTENSORGLYPH::create(unsigned int slicesphi,unsigned int slicestheta, bool drawNormals, bool drawMesh){
    //in diffusion tensors, the length scales are defined by the eigenvalues
    FLOAT xScale = (eigenValues[0] > MIN_LSCALE)? eigenValues[0]: MIN_LSCALE,
      yScale = (eigenValues[1] > MIN_LSCALE)? eigenValues[1]: MIN_LSCALE,
      zScale = (eigenValues[2] > MIN_LSCALE)? eigenValues[2]: MIN_LSCALE;

    sq->create(slicesphi,slicestheta, xScale*0.49,yScale*0.49,zScale*0.49); //deixando 0.01 de espaço entre um glifo e seu vizinho, no caso onde eles tenham comprimento maximo
}

void gcgTENSORGLYPH::draw(bool drawMesh,bool drawNormals, float scale){
    VECTOR3 color;
    /*    if (weight < 0.25){
        color[0] = 1;
        color[1] = 0;
        color[2] = 0;
    }
    else if (weight < 0.50){
        color[0] = 0;
        color[1] = 1;
        color[2] = 0;
    }
        else if (weight < 0.75){
           color[0] = 0;
           color[1] = 0;
           color[2] = 1;
        }
            else if (weight < 1){
                color[0] = 1;
                color[1] = 1;
                color[2] = 0;
            }~*/
    //gcgHeatColor(weight, color);
    //glColor4f(color[0],color[1],color[2], confidence);
    glColor3f(0.5, 0.5, 0.5);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(pos[0],pos[1],pos[2]);
    glScalef(scale,scale,scale);
    MATRIX4 orientation = {eigenVectors[0], eigenVectors[1],eigenVectors[2], 0,
                       eigenVectors[3],eigenVectors[4],eigenVectors[5], 0,
                       eigenVectors[6],eigenVectors[7],eigenVectors[8], 0,
                       0, 0, 0, 1};
    glMultMatrixf(orientation);
    sq->draw(drawMesh,drawNormals);
    glPopMatrix();
}
