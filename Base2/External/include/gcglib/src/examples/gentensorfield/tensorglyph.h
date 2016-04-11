/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

gcgTENSORGLYPH
   Marcelo Bernardes Vieira
   Tássio Knop de Castro
**************************************************************************************/

#ifndef _gcgTENSORGLYPH_H_
#define _gcgTENSORGLYPH_H_

#include "superquadric.h"

#define MIN_LSCALE 0.1  //min length scale
#define MIN_ALPHA  0.1
#define MIN_BETA   0.1


class gcgTENSORGLYPH{
    private:
    //anisotropy components
    FLOAT cl, cp, cs;                   //linear, planar and spherical

    //properties of the tensor matrix
    MATRIX3 eigenVectors;
    VECTOR3 eigenValues;

    //parameter that defines the sharpness
    FLOAT gamma;

    //defined by the anisotropy components of the tensor
    bool parametrization;

    //superquadric
    gcgSUPERQUADRIC * sq;

    public:
    //tensor matrix
    MATRIX3 tensor;
    //position of its center
    VECTOR3 pos;

    //value computed from the eigenvalues to determine the color of the glyph: its 1/(sum(eigenvalues))
    FLOAT weight;

    FLOAT confidence;

    //constructor
    //obtains alpha and beta from a tensor, extracting its eigenvalues and computing
    //its anisotropy components (linear, planar, spherical)
    //if linear >= planar, we use the first parametrization; otherwise we use the second
    gcgTENSORGLYPH(MATRIX3 tensor, VECTOR3 pos, FLOAT confidence, FLOAT gamma = 3);
    ~gcgTENSORGLYPH();

    //draws a superquadric.
    //slicesphi: number of slices over phi
    //slicestheta: number of slices over theta
    void create(unsigned int slicesphi,unsigned int slicestheta, bool drawNormals, bool drawMesh);
    void draw(bool drawMesh,bool drawNormals, float scale = 1);
};
#endif


