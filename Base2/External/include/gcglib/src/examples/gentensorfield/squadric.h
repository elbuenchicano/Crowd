/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

gcgSUPERQUADRIC
   Marcelo Bernardes Vieira
   Tássio Knop de Castro
**************************************************************************************/

#ifndef _gcgSUPERQUADRIC_H_
#define _gcgSUPERQUADRIC_H_

//defines which paramerization to use
#define FIRST_FORM 0
#define SECOND_FORM 1
typedef float FLOAT;

#include "gcg.h"
typedef struct _ponto {
     FLOAT x;
     FLOAT y;
     FLOAT z;
     FLOAT nx;
     FLOAT ny;
     FLOAT nz;
} point;



class gcgSUPERQUADRIC{
    private:
    //parameters that defines the shape
    FLOAT alpha;
    FLOAT beta;

    //parameter that defines the sharpness
    FLOAT gamma;

    //defined by the anisotropy components of the tensor
    bool parametrization;

    //position os its center
    VECTOR3 pos;

    //length scales
    FLOAT xScale, yScale, zScale;

    int nPoints;
    //current position of point in the array of points
    int pointPos;
    int secondStrip;

    //array of vertexes
    point *pontos;

    //methods used for drawing
    void createVertexFirstForm(FLOAT theta, FLOAT phi, point* pontos);
    void createVertexSecondForm(FLOAT theta, FLOAT phi, point *pontos);

    void createFirstForm(unsigned int slicesphi, unsigned int slicestheta);
    void createSecondForm(unsigned int slicesphi, unsigned int slicestheta);

    public:
    //constructor
    gcgSUPERQUADRIC(FLOAT alpha, FLOAT beta, VECTOR3 pos, bool parametrization, FLOAT gamma = 3);
    ~gcgSUPERQUADRIC();

    //draws a superquadric.
    //slicesphi: number of slices over phi
    //slicestheta: number of slices over theta
    //scale factors in each direction: x, y and z
    void create(unsigned int slicesphi, unsigned int slicestheta, FLOAT xScale = 1, FLOAT yScale = 1, FLOAT zScale = 1);
    void draw(bool drawMesh,bool drawNormals);
};
#endif


