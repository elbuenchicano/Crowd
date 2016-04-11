#include <gcg.h>
#include "tensorglyph.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>


#define ADDRESS(a, b, c) (((c) * dimX * dimY + (b) * dimX + (a)) * 9)

template <class TENSORTYPE> class gcgTENSORFIELDGENERATOR {
  public:
    TENSORTYPE     *field;
    unsigned int   dimX;
    unsigned int   dimY;
    unsigned int   dimZ;
    gcgTENSORGLYPH **glyphs;

  public:

    gcgTENSORFIELDGENERATOR() {
      field = NULL;
      glyphs = NULL;
      dimX = dimY = dimZ = 0;
    }

    ~gcgTENSORFIELDGENERATOR() {
      if(field != NULL) delete[] field;
      // Run through field
      if(glyphs != NULL) {
        for(unsigned int i = 0; i < dimX; i++)
          for(unsigned int j = 0; j < dimY; j++)
            for(unsigned int k = 0; k < dimZ; k++)
              delete glyphs[((k) * dimX * dimY + (j) * dimX + (i))];
        delete[] glyphs;
      }
    }


    // Generates a field
    bool generate(unsigned int lengthX, unsigned int lengthY, unsigned int lengthZ, unsigned int npoints, TENSORTYPE *points) {
      // Check current value
      if(field != NULL) delete[] field;

      // Allocate memory
      field = new TENSORTYPE[lengthX * lengthY * lengthZ * 9];
      glyphs = new gcgTENSORGLYPH*[lengthX * lengthY * lengthZ];
      if(field == NULL) return false;

      // Init field
      dimX = lengthX;
      dimY = lengthY;
      dimZ = lengthZ;
      memset(field, 0, sizeof(TENSORTYPE) * lengthX * lengthY * lengthZ * 9);

      TENSORTYPE SIGMA = MAX(MAX(dimX, dimY), dimZ) * 0.6;

      TENSORTYPE biggest = 0.0 ,smallest = 99999999999.0;

      // Runs through points
      for(unsigned int p = 0; p < npoints; p++) {
          // Run through field
          for(unsigned int i = 0; i < dimX; i++)
            for(unsigned int j = 0; j < dimY; j++)
              for(unsigned int k = 0; k < dimZ; k++) {
                // Compute point's influence on
                TENSORTYPE dist[3], pos[3] = {(TENSORTYPE)i, (TENSORTYPE)j, (TENSORTYPE)k};
                dist[0] = points[p*3+0] - (TENSORTYPE) pos[0];
                dist[1] = points[p*3+1] - (TENSORTYPE) pos[1];
                dist[2] = points[p*3+2] - (TENSORTYPE) pos[2];
                TENSORTYPE ndist = sqrt(SQR(dist[0]) + SQR(dist[1]) + SQR(dist[2]));

                // Compute tensor
                TENSORTYPE *tensor = &field[ADDRESS(i,j,k)];

                if(ndist < EPSILON) continue;

                if(i != (unsigned int) points[p*3+0] || j != (unsigned int) points[p*3+1] || k != (unsigned int) points[p*3+2]) {
                  dist[0] /= ndist;
                  dist[1] /= ndist;
                  dist[2] /= ndist;
                  TENSORTYPE influence = exp(-(ndist*ndist) / (SIGMA*SIGMA));


                  tensor[ 0] += dist[ 0] * dist[ 0] * influence;
                  tensor[ 1] += dist[ 0] * dist[ 1] * influence;
                  tensor[ 2] += dist[ 0] * dist[ 2] * influence;
                  tensor[ 3] += dist[ 1] * dist[ 0] * influence;
                  tensor[ 4] += dist[ 1] * dist[ 1] * influence;
                  tensor[ 5] += dist[ 1] * dist[ 2] * influence;
                  tensor[ 6] += dist[ 2] * dist[ 0] * influence;
                  tensor[ 7] += dist[ 2] * dist[ 1] * influence;
                  tensor[ 8] += dist[ 2] * dist[ 2] * influence;

                  TENSORTYPE size = SQR(tensor[0]) + SQR(tensor[1]) + SQR(tensor[2]) +
                                    SQR(tensor[3]) + SQR(tensor[4]) + SQR(tensor[5]) +
                                    SQR(tensor[6]) + SQR(tensor[7]) + SQR(tensor[8]);

                  if(size > biggest) biggest = size;
                  if(size < smallest) smallest = size;
                }
              }
      }

      for(unsigned int p = 0; p < npoints; p++) {
        // Compute tensor
        TENSORTYPE *tensor = &field[ADDRESS((int) points[p*3+0], (int) points[p*3+1], (int) points[p*3+2])];

        tensor[0] = 1.0;
        tensor[1] = 0.0;
        tensor[2] = 0.0;
        tensor[3] = 0.0;
        tensor[4] = 1.0;
        tensor[5] = 0.0;
        tensor[6] = 0.0;
        tensor[7] = 0.0;
        tensor[8] = 1.0;
      }

      // Normalize all
      biggest = sqrt(biggest);
      for(unsigned int i = 0; i < dimX; i++)
        for(unsigned int j = 0; j < dimY; j++)
          for(unsigned int k = 0; k < dimZ; k++) {
            TENSORTYPE *tensor = &field[ADDRESS(i,j,k)];

            tensor[0] = (tensor[0]) / biggest;
            tensor[1] = (tensor[1]) / biggest;
            tensor[2] = (tensor[2]) / biggest;
            tensor[3] = (tensor[3]) / biggest;
            tensor[4] = (tensor[4]) / biggest;
            tensor[5] = (tensor[5]) / biggest;
            tensor[6] = (tensor[6]) / biggest;
            tensor[7] = (tensor[7]) / biggest;
            tensor[8] = (tensor[8]) / biggest;



            TENSORTYPE pos[3] = {i - (float) dimX/2.0, j-(float) dimY/2.0, k-(float) dimZ/2.0};
            glyphs[((k) * dimX * dimY + (j) * dimX + (i))] = new gcgTENSORGLYPH(tensor, pos, 1.0);
            glyphs[((k) * dimX * dimY + (j) * dimX + (i))]->create(7, 7, false, false);
          }

      return true;
     }

    float* getTensor(int i, int j, int k) {
      return &field[ADDRESS(i,j,k)];
    }

     void draw() {
      // Run through field
      for(unsigned int i = 0; i < dimX; i++)
        for(unsigned int j = 0; j < dimY; j++)
          for(unsigned int k = 0; k < dimZ; k++)
            glyphs[((k) * dimX * dimY + (j) * dimX + (i))]->draw(false, false, 1.0);
     }



    void save(char *filename) {
      FILE *arq;

      arq = fopen(filename, "wb+");
      if(arq != NULL) {
        fwrite(field, 1, dimX * dimY * dimZ * 9 * sizeof(TENSORTYPE), arq);
        fclose(arq);
      }
    }
};

