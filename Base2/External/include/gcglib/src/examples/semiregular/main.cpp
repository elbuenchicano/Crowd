#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <GL/glut.h>
#include <signal.h>

#include "gcg.h"

#define TERRAIN_WIDTH   2049
#define TERRAIN_HEIGHT  1025
#define EXAGERATION     500   // Height exageration
#define Z_SCALE         ((8.3f * EXAGERATION) / 255.f) // Z scaling proportional to the Everest height

#define TEXTURE_WIDTH   2048
#define TEXTURE_HEIGHT  1024

// Earth's equatorial radius is 6378.135 km
// Earths's polar radius is 6356.750 km
// Earth's average radius is 6372.795477598 km
#define SPHERE_RADIUS 6372.795477598 // km


// Subdivision control
#define MINIMUM_SUBDIVISION_LEVEL 10 // Minimum subdivision level


int   windowwidth, windowheight;
float fovh = 30;

gcgTEXT output;
gcgPLOT plot;
gcgPLOT plot2;
unsigned int plot_triang;
unsigned int plot_tessel;
unsigned int plot_sintese;

bool wireframe = false;

////////////////////////////////////////////////////////
// SPHERE vertex structure
////////////////////////////////////////////////////////
typedef struct {
  int u, v, w;    // Parametric coordinates
  float x, y, z;  // 3D coordinates
} VERTEX;


//////////////////////////////////////////////////////////////////
// Class to generate and visualize a big SPHERE.
//    Implements gcgADAPTIVE which is the interface
//    for using gcgSEMIREGULAR.
//////////////////////////////////////////////////////////////////
class SPHERE : public gcgADAPTIVE  {
  public:
    GLuint texturename;

    // Array to store vertices
    VERTEX  *vertices;
    int    nvertices;
    int    maxvertices;

    // Earth geometric and texture information
    unsigned char height[TERRAIN_HEIGHT][TERRAIN_WIDTH];
    float errogeometricoROAM[TERRAIN_HEIGHT][TERRAIN_WIDTH];
    float errogeometricoSOAR[TERRAIN_HEIGHT][TERRAIN_WIDTH];
    float esferasSOAR[TERRAIN_HEIGHT][TERRAIN_WIDTH];

    // Camera object
    gcgFRUSTUM camera;

  public:
    SPHERE();
    virtual ~SPHERE();

    // SPHERE geometry
    static void sample_sphere(float u, float v, float w, float *x, float *y, float *z);
    int height_sphere(float x, float y);

    // gcgADAPTIVE interface
    void beginTesselation();
    unsigned char action(unsigned char nivel, int *tri, unsigned char oldflags);
    int  sample(int *iVertices, int apex2);
    void endTesselation();

    // Visualization
    void AlignObserver(float xw, float yw, float zw, float *xc, float *yc, float *zc);
};

SPHERE::SPHERE() {
  FILE *arq;
  char *texture;

  ///////////////////////////////////////////////////
  // Load geometry maps

  // Load texture
  arq = fopen("textura.raw", "rb");

  texture = (char*) malloc(3 * sizeof(char) * TEXTURE_WIDTH * TEXTURE_HEIGHT);
  fread(texture, 3 * sizeof(char), TEXTURE_WIDTH * TEXTURE_HEIGHT, arq);
  fclose(arq);

  // Allocate a texture name
  glGenTextures( 1, &texturename);

  // Select our current texture
  glBindTexture(GL_TEXTURE_2D, texturename);

  // Select modulate to mix texture with color for shading
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

  // When texture area is small, bilinear filter the closest mipmap
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
  // When texture area is large, bilinear filter the original
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // The texture wraps over at the edges (repeat)
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );

  // Build our texture mipmaps
  gluBuild2DMipmaps( GL_TEXTURE_2D, 3, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, texture);
  free(texture);

  // Load height map
  arq = fopen("altitude.raw", "rb");
  fread(height, sizeof(char), TERRAIN_WIDTH * TERRAIN_HEIGHT, arq);
  fclose(arq);

  // Load geometric errors for ROAM
  arq = fopen("errogeometricoROAM.raw", "rb");
  fread(errogeometricoROAM, sizeof(float), TERRAIN_WIDTH * TERRAIN_HEIGHT, arq);
  fclose(arq);

  // Load geometric errors for SOAR
  arq = fopen("errogeometricoSOAR.raw", "rb");
  fread(errogeometricoSOAR, sizeof(float), TERRAIN_WIDTH * TERRAIN_HEIGHT, arq);
  fclose(arq);

  // Load sphere errors for SOAR
  arq = fopen("esferasSOAR.raw", "rb");
  fread(esferasSOAR, sizeof(float), TERRAIN_WIDTH * TERRAIN_HEIGHT, arq);
  fclose(arq);

  // Alloc vertices
  maxvertices = 1024;
  vertices = (VERTEX*) malloc(sizeof(VERTEX) * maxvertices);
  nvertices = 0;
}


SPHERE::~SPHERE() {
  free(vertices);
  glDeleteTextures(1, &texturename);
}



// x e y coordinates given in sub-pixels. Result is in voxels.
int SPHERE::height_sphere(float x, float y) {
  int i = (int) x;
  int j = (int) y;

  while(i < 0) { i += TERRAIN_WIDTH-1; x += TERRAIN_WIDTH-1; }
  while(i > TERRAIN_WIDTH-1) { i -= TERRAIN_WIDTH-1; x -= TERRAIN_WIDTH-1; }
  while(j < 0) { j += TERRAIN_HEIGHT-1; y += TERRAIN_HEIGHT-1; }
  while(j > TERRAIN_HEIGHT-1) { j -= TERRAIN_HEIGHT-1; y -= TERRAIN_HEIGHT-1; }

  int i2 = i + 1;
  int j2 = j + 1;

  if(i2 > TERRAIN_WIDTH-1) i2 = i;//i2 - (TERRAIN_WIDTH - 1);
  if(j2 > TERRAIN_HEIGHT-1) j2 = j;//j2 - (TERRAIN_HEIGHT - 1);

  unsigned char z1 = height[j][i];
  unsigned char z2 = height[j][i2];
  unsigned char z3 = height[j2][i];
  unsigned char z4 = height[j2][i2];

  float cx = x - (float) i;
  float cy = y - (float) j;

  float termo1 = z1*(1-cx) + z2*cx;
  float termo2 = z3*(1-cx) + z4*cx;

  return (int) ((float) (1-cy)*termo1+cy*termo2);
}


void SPHERE::beginTesselation() {
  nvertices = 4;

  // Define the limits in paramatric space: using pixel coordinates!
  vertices[0].u  = 0;
  vertices[0].v  = 0;
  vertices[0].w  = height[TERRAIN_HEIGHT-1][0];
  vertices[1].u  = TERRAIN_WIDTH-1;
  vertices[1].v  = 0;
  vertices[1].w  = height[TERRAIN_HEIGHT-1][TERRAIN_WIDTH-1];
  vertices[2].u  = TERRAIN_WIDTH-1;
  vertices[2].v  = TERRAIN_HEIGHT-1;
  vertices[2].w  = height[0][TERRAIN_WIDTH-1];
  vertices[3].u  = 0;
  vertices[3].v  = TERRAIN_HEIGHT-1;
  vertices[3].w  = height[0][0];

  // Now, compute the corresponding limits over the sphere
  sample_sphere((float) vertices[0].u, (float) vertices[0].v, (float) vertices[0].w, &vertices[0].x, &vertices[0].y, &vertices[0].z);
  sample_sphere((float) vertices[1].u, (float) vertices[1].v, (float) vertices[1].w, &vertices[1].x, &vertices[1].y, &vertices[1].z);
  sample_sphere((float) vertices[2].u, (float) vertices[2].v, (float) vertices[2].w, &vertices[2].x, &vertices[2].y, &vertices[2].z);
  sample_sphere((float) vertices[3].u, (float) vertices[3].v, (float) vertices[3].w, &vertices[3].x, &vertices[3].y, &vertices[3].z);
}


// It must return flags to indicate if the vertice must be refined.
// It also is used to set flags for visibility and Level Of Detail control.
unsigned char SPHERE::action(unsigned char nivel, int *iVertices, unsigned char oldflags) {
    VERTEX *vapex = &vertices[iVertices[GCG_VAPEX]];
    VERTEX *vesq  = &vertices[iVertices[GCG_VESQ]];
    VERTEX *vdir  = &vertices[iVertices[GCG_VDIR]];
    unsigned char newflags;

    // Minimum sphere resolution: minimum subdivision level to preserve height at horizon
    if(nivel < MINIMUM_SUBDIVISION_LEVEL) return GCG_REFINE;


/*
   {
    register float lon1 = ((vertices[iVertices[GCG_VESQ]].u+vertices[iVertices[GCG_VESQ]].u) * (1.0f / (TERRAIN_WIDTH-1))) * M_PI - M_PI;
    register float lat1 = (vertices[iVertices[GCG_VESQ]].v * (1.0f / (TERRAIN_HEIGHT-1))) * M_PI - (M_PI * 0.5);
    register float lon2 = ((vertices[iVertices[GCG_VDIR]].u+vertices[iVertices[GCG_VDIR]].u) * (1.0f / (TERRAIN_WIDTH-1))) * M_PI - M_PI;
    register float lat2 = (vertices[iVertices[GCG_VDIR]].v * (1.0f / (TERRAIN_HEIGHT-1))) * M_PI - (M_PI * 0.5);
    register float lon3 = ((vertices[iVertices[GCG_VAPEX]].u+vertices[iVertices[GCG_VAPEX]].u) * (1.0f / (TERRAIN_WIDTH-1))) * M_PI - M_PI;
    register float lat3 = (vertices[iVertices[GCG_VAPEX]].v * (1.0f / (TERRAIN_HEIGHT-1))) * M_PI - (M_PI * 0.5);

    //if(gcgSphericalDistance(lat1, lon1, lat2, lon2) * gcgSphericalDistance((lat1+lat2)*0.5, (lon1+lon2)*0.5, lat3, lon3) < 0.0005) return GCG_MAXIMUMLOD;
    if(gcgSphericalDistance(lat1, lon1, lat2, lon2) < 0.05) return GCG_SIMPLIFY;
    //if(gcgSphericalDistance(lat3, lon3, lat2, lon2) > 0.2) return GCG_REFINE;
    //if(gcgSphericalDistance(lat1, lon1, lat3, lon3) > 0.2) return GCG_REFINE;

    //return GCG_MAXIMUMLOD | GCG_SELECT0;
      //printf("%(%f %f) (%f %f) = %f\n", lat1, lon1, lat2, lon2, gcgGreatCircleDistance(lat1, lon1, lat2, lon2));
    }
*/

    // Maximum sphere resolution: define an upper bound
    //if(nivel > 17) return GCG_STOP | GCG_AVOIDREFINING;

    // Check if it is the maximum level
    if(abs(vapex->u - vdir->u) < 2 && abs(vapex->v - vdir->v) < 2) return GCG_MAXIMUMLOD;

    // Compute points over the SPHERE
    float x1 = vapex->x, y1 = vapex->y, z1 = vapex->z,
          x2 = vesq->x,  y2 = vesq->y,  z2 = vesq->z,
          x3 = vdir->x,  y3 = vdir->y,  z3 = vdir->z, a, b, c, p, q;

    a = x1 * (x1-camera.x) + y1 * (y1-camera.y) + z1 * (z1-camera.z);
    b = x2 * (x2-camera.x) + y2 * (y2-camera.y) + z2 * (z2-camera.z);
    c = x3 * (x3-camera.x) + y3 * (y3-camera.y) + z3 * (z3-camera.z);

    // Normalizing to finde the cosine of the angle
    //a /= sqrt((x1*x1 + y1*y1 + z1*z1) * ((x1-observador[0])*(x1-observador[0]) + (y1-observador[1])*(y1-observador[1]) + (z1-observador[2])*(z1-observador[2])));
    //b /= sqrt((x2*x2 + y2*y2 + z2*z2) * ((x2-observador[0])*(x2-observador[0]) + (y2-observador[1])*(y2-observador[1]) + (z2-observador[2])*(z2-observador[2])));
    //c /= sqrt((x3*x3 + y3*y3 + z3*z3) * ((x3-observador[0])*(x3-observador[0]) + (y3-observador[1])*(y3-observador[1]) + (z3-observador[2])*(z3-observador[2])));

    newflags = oldflags & (GCG_MAXIMUMLOD | GCG_INVISIBLE | GCG_TOTALLYVISIBLE | GCG_SELECT0);

    x3 = (x2+x3) * 0.5f;
    y3 = (y2+y3) * 0.5f;
    z3 = (z2+z3) * 0.5f;


    // First time flags
    if(newflags == 0) {
      // Back face check
      if(a > 0 && b > 0 && c > 0) return GCG_INVISIBLE | GCG_MAXIMUMLOD | GCG_SELECT0;
      if(nivel < 11) return newflags | GCG_REFINE;     // Minimum resolution in visible side

      // Frustum intersection test
      int vis = camera.isSphereVisible(x3, y3, z3, 2*sqrt((x3-x1)*(x3-x1)+ (y3-y1)*(y3-y1) + (z3-z1)*(z3-z1)));

      if(vis == GCG_FRUSTUM_OUTSIDE) return GCG_INVISIBLE | GCG_MAXIMUMLOD  | GCG_SELECT0;
      if(vis == GCG_FRUSTUM_INSIDE) newflags |= GCG_TOTALLYVISIBLE; // Here we know it's completely visible
    }

    // Find the mean point over the SPHERE
    int midX  = (vesq->u + vdir->u) >> 1;
    int midY  = (vesq->v + vdir->v) >> 1;

    float dist = sqrt((x3-camera.x)*(x3-camera.x) + (y3-camera.y)*(y3-camera.y) + (z3-camera.z)*(z3-camera.z));

    // Check if it is on the horizon
    q = sqrt(x3*x3 + y3*y3 + z3*z3) * dist;
    p = cosf(M_PI/8) * q - q;

    if((a > p || b > p || c > p)  && nivel < 13) return newflags | GCG_REFINE;

    // Geomtric error: ROAM ou SOAR methods
    float error = errogeometricoROAM[TERRAIN_HEIGHT-1- midY][midX];

    // Radius for SOAR method
    float raio = (float)esferasSOAR[TERRAIN_HEIGHT-1-midY][midX];
/*
    // Computes the SOAR error
   register float longitude  = ((midX+midX) * (1.0f / (TERRAIN_WIDTH-1))) * M_PI - M_PI;
   register float latitude   = (midY * (1.0f / (TERRAIN_HEIGHT-1))) * M_PI - (M_PI * 0.5);

    if((windowwidth/(2*tan(DEG2RAD(fovh) / 2))) * ((error)/(dist-raio)) > 0.10025) return newflags | GCG_REFINE;
    if((windowwidth/(2*tan(DEG2RAD(fovh) / 2))) * ((error)/(dist-raio)) > 0.645+0.25*fabs(sin(latitude))) return newflags | GCG_REFINE;
    if((windowwidth/(2*tan(DEG2RAD(fovh) / 2))) * ((error)/(dist-raio)) > 0.425) return newflags | GCG_REFINE;
*/
/*    // Computes the ROAM error
    register float longitude  = ((midX+midX) * (1.0f / (TERRAIN_WIDTH-1))) * M_PI - M_PI;
    register float latitude   = (midY * (1.0f / (TERRAIN_HEIGHT-1))) * M_PI - (M_PI * 0.5);

    if((windowwidth/(2*tan(DEG2RAD(fovh) / 2))) * ((error)/(dist-raio)) > 0.10025) return newflags | GCG_REFINE;
    if((windowwidth/(2*tan(DEG2RAD(fovh) / 2))) * ((error)/(dist-raio)) > 0.645+0.25*fabs(sin(latitude))) return newflags | GCG_REFINE;
  */  if((windowwidth/(2*tan(DEG2RAD(fovh) / 2))) * ((error)/(dist+0.1)) > 2.825) return newflags | GCG_REFINE;


    /*
    // Find camera coordinates
    AlignObserver(x3, y3, z3+erro*Z_SCALE, &x1, &y1, &z1);
    AlignObserver(x3, y3, z3-erro*Z_SCALE, &x2, &y2, &z2);
    AlignObserver(x3, y3, z3,              &p,  &q,  &r);

    a = x2 - x1;
    b = y2 - y1;
    c = z2 - z1;

    float dist = (2.0 / (r*r - c*c)) * (float) sqrt( (a*r - c*p)*(a*r - c*p) + (b*r - c*q)*(b*r - c*q)  );

    if(fabs(dist) > 1.0) return 1;   */


    raio = 0; // Annoying warning
  	return newflags;
}

////////////////////////////////////////////////////////////////
// sample() must compute the NEW POINT for the triangle division
// process (splitting), returning its index. If apex2 é positive, then
// it is the index of the apex of the triangle that forms a losangle
// (diamond) with the triangle (see figure). It must return
// the new vertex index.
//
// vapex---vdir
//     |  /	|
//     | /	|
//  vesq--- apex2
//
// apex2 may also be negative. In that case, we only have one triangle.
// The sample() function has to return the result of the interpolation
// either of the four points (if apex2 >= 0), either of the triangle.
//
int SPHERE::sample(int *iVertices, int apex2) {
    // Dynamically allocates more space, if needed, for the new vertex
    if(nvertices >= maxvertices) {
      maxvertices += 1024;
      vertices = (VERTEX*) realloc(vertices, sizeof(VERTEX) * maxvertices);
    }

    vertices[nvertices].u = (vertices[iVertices[GCG_VESQ]].u + vertices[iVertices[GCG_VDIR]].u) >> 1;
	  vertices[nvertices].v = (vertices[iVertices[GCG_VESQ]].v + vertices[iVertices[GCG_VDIR]].v) >> 1;

	  vertices[nvertices].w = height[TERRAIN_HEIGHT-1-vertices[nvertices].v][vertices[nvertices].u];//getHeight(novovertice->u, TERRAIN_HEIGHT-1-novovertice->v);
	  sample_sphere((float) vertices[nvertices].u, (float) vertices[nvertices].v, (float) vertices[nvertices].w, &vertices[nvertices].x, &vertices[nvertices].y, &vertices[nvertices].z);

    nvertices++;

    apex2 = 0; // Annoying warning
	  return nvertices - 1;
}

void SPHERE::endTesselation() {
  glTexCoordPointer(2, GL_INT, sizeof(VERTEX), &vertices[0].u);
  glVertexPointer(3, GL_FLOAT, sizeof(VERTEX), &vertices[0].x);
  //glVertexPointer(2, GL_INT, sizeof(VERTEX), &vertices[0].u);
}

inline void SPHERE::sample_sphere(float u, float v, float w, float *x, float *y, float *z) {
  register float longitude  = (float) (((u+u) * (1.0 / (TERRAIN_WIDTH-1))) * M_PI - M_PI);
  register float latitude   = (float) ((v * (1.0 / (TERRAIN_HEIGHT-1))) * M_PI - (M_PI * 0.5));
  register float raio = (float) (SPHERE_RADIUS + w * Z_SCALE);

  register float coslatitude  = cosf(latitude);
  register float coslongitude = cosf(longitude);
  register float sinlatitude  = sinf(latitude);
  register float sinlongitude = sinf(longitude);

  // SPHERE
  *x =  raio * coslongitude * coslatitude;
  *y =  raio * sinlongitude * coslatitude;
  *z =  raio * sinlatitude;
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////



// Objects to be used to tesselate the sphere
gcgSEMIREGULAR48  arv;
SPHERE            earth;

// Events per second estimators
gcgEVENTSPERSECOND qps;
gcgEVENTSPERSECOND tpstessela;
gcgEVENTSPERSECOND tpsrender;

// Visualization control
float iniX, iniY, spin = false, spinR = false, pitchAngle = 0, rollAngle = 0.001;
float speed = 0;
clock_t oldtimestamp;

// Callback function to draw everything
void Draw(void) {
  int ntriang = 0, *vertexindexes;

  // Start drawing chronometer
  tpsrender.startingEvents();

  // Prepare buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Export view camera matrices from gcgFRUSTUM
  earth.camera.exportOpenGL();

  // Select earth texture
  glBindTexture(GL_TEXTURE_2D, earth.texturename);

  // OpengGL options
  glEnable( GL_TEXTURE_2D );
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  glDisable(GL_CULL_FACE);


  // Remove invisible triangles (using GCG_INVISIBLE mask) and keep totally visible ones (using GCG_TOTALLYVISIBLE mask)
  arv.hideTriangles(GCG_INVISIBLE, GCG_TOTALLYVISIBLE);

  // Get the triangle array
  vertexindexes = arv.getIndexArray(&ntriang);

  // Draw triangles from the returned vertex array
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glDrawElements(GL_TRIANGLES, ntriang * 3, GL_UNSIGNED_INT, vertexindexes);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  // OpengGL options
  glDisable( GL_TEXTURE_2D );
  glEnable(GL_POLYGON_OFFSET_LINE);
  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);

  // Check if we must draw the wireframe
  if(wireframe) {
    glColor3f(1, 1, 1);
    vertexindexes = arv.getIndexArray(&ntriang);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawElements(GL_TRIANGLES, ntriang * 3, GL_UNSIGNED_INT, vertexindexes);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    // Finishes triangle render time
    tpsrender.finishedEvents(ntriang * 2);
  } else tpsrender.finishedEvents(ntriang);

  /////////////////////////////////////////////////////////////////

  // Draw axis
  glBegin(GL_LINES);
    glColor3f(1,0,0);
    glVertex3f(0,0,0);
    glVertex3f(SPHERE_RADIUS,0,0);

    glColor3f(0,1,0);
    glVertex3f(0,0,0);
    glVertex3f(0,SPHERE_RADIUS,0);

    glColor3f(0,0,1);
    glVertex3f(0,0,-(1.4*SPHERE_RADIUS));
    glVertex3f(0,0,1.4*SPHERE_RADIUS);
  glEnd();

  // Draw logo
  gcgDrawLogo();

   ////////////////////////////////////////////////////////////////

  // Prepare to draw text
  output.enableTextBox(5, 5, 404, 122); // Enable a text box
  output.drawTextBox(0.0, 0.0, 0.0, 0.3f, 0.0, 0.0, 0.0, 1.f, 1.f); // Draws a framed text box with transparency
  output.wrapText(FALSE);  // Disables the wrapping functionality
  output.textPosition(0, 0);  // Sets the text position within the text box

  // Draw text
  glColor3f(1.f, 1.f, 1.f);
  output.gcgprintf("UFJF/DCC/GCG - Group for Computer Graphics, Image and Vision\n\n");
  output.gcgprintf("Triangles: %d\n", ntriang);
  output.gcgprintf("Fps: %5.2f\n", qps.getEventsPerSecond());
  output.gcgprintf("Tesselation: %8.2f tri/s\n", tpstessela.getEventsPerSecond());
  output.gcgprintf("Rendering: %8.2f tri/s\n", tpsrender.getEventsPerSecond());
  output.gcgprintf("Height: %8.2lf km", sqrt(earth.camera.x*earth.camera.x + earth.camera.y*earth.camera.y + earth.camera.z*earth.camera.z)-SPHERE_RADIUS);

  // Setup plots
  clock_t timestamp = clock();
  plot.setAxisRange(GCG_PLOT_X, ((float) timestamp / CLOCKS_PER_SEC > 30) ? (float) timestamp / CLOCKS_PER_SEC - 30 : 0, (float) timestamp / CLOCKS_PER_SEC);
  plot.addPoint2D(plot_triang, (float) timestamp / CLOCKS_PER_SEC, (float) ntriang);
  plot.rangeFromData(GCG_PLOT_Y);
  plot.clipOutOfRange(plot_triang, GCG_PLOT_X);

  plot2.setAxisRange(GCG_PLOT_X, ((float) timestamp / CLOCKS_PER_SEC > 30) ? (float) timestamp / CLOCKS_PER_SEC - 30 : 0, (float) timestamp / CLOCKS_PER_SEC);
  plot2.addPoint2D(plot_tessel, (float) timestamp / CLOCKS_PER_SEC, tpstessela.getEventsPerSecond());
  plot2.addPoint2D(plot_sintese, (float) timestamp / CLOCKS_PER_SEC, tpsrender.getEventsPerSecond());
  plot2.rangeFromData(GCG_PLOT_Y);
  plot2.clipOutOfRange(plot_tessel, GCG_PLOT_X);
  plot2.clipOutOfRange(plot_sintese, GCG_PLOT_X);

  // Draw plots
  plot.draw();
  plot2.draw();

  // Show new image
  glutSwapBuffers();

  qps.finishedEvents(); // Finish this drawing performance estimation
  qps.startingEvents(); // Start next time estimate
}

// Callback function called when the window is resized
void WindowResized(GLsizei w, GLsizei h) {
	// Prevent zero division
	if (h == 0) h = 1;

	// Specifies the viewport size
	glViewport(0, 0, w, h);
	windowwidth = w;
	windowheight  = h;

  // Update projection
  earth.camera.setPerspective(fovh, (float) w / (float) h, 0.01f, (float) (5.0*SPHERE_RADIUS+sqrtf(earth.camera.x*earth.camera.x + earth.camera.y*earth.camera.y + earth.camera.z*earth.camera.z)));
  plot.setFrame(10.f,  10.f + h - h * 0.2f, w - 20.f, h * 0.2f - 20.f, 1.f);
  plot2.setFrame(10.f, 20.f + h - 2.f * h * 0.2f, w - 20.f, h * 0.2f - 20.f, 1.f);

  // Update display
	glutPostRedisplay();
}

// Manages basic mouse events
void MouseManagement(int button, int state, int x, int y) {
  if(button == GLUT_LEFT_BUTTON){
    if (state == GLUT_DOWN) {
      iniX = (float) x;
      iniY = (float) y;
      spin = TRUE;
      spinR = FALSE;
    }
    if(state == GLUT_UP) {
      spin = FALSE;
      spinR = FALSE;
    }
  } else if(button == GLUT_RIGHT_BUTTON) {
      iniX = (float) x;
      iniY = (float) y;
      spin = FALSE;
      spinR = TRUE;
    }
}


// Manages mouse motion events
void MouseMotionManagement(int x, int y) {
  if(spin) {
    earth.camera.rotateOrientationTrackball(-(windowwidth - 2.f * iniX) / windowwidth, -(windowheight - 2.f * y) / windowheight, -(windowwidth-2.f * x) / windowwidth, -(windowheight - 2.f * iniY) / windowheight);
    //earth.camera.setTarget(0,0,0);
  }

  if(spinR) {
    earth.camera.rotateOrbitTrackball(0, 0, 0, (windowwidth - 2.f * iniX) / windowwidth, (windowheight - 2.f * y) / windowheight, (windowwidth-2.f * x) / windowwidth, (windowheight - 2.f * iniY) / windowheight);
    earth.camera.setTarget(0.0, 0.0, 0.0);  // Ajusta as direcoes para visualizar o alvo.
    //frustum.alignAxis();
    glutPostRedisplay();
  }

  iniX = (float) x;
  iniY = (float) y;
}

// Special keys management
void KeyboardManagementSpecial(int key, int x, int y){
    switch(key){
        case GLUT_KEY_PAGE_UP:
                  break;

        case GLUT_KEY_PAGE_DOWN:
                  break;
        case GLUT_KEY_LEFT:
                          rollAngle = 0.05;// degrees/s
                          if(rollAngle > 1) rollAngle = 1;
                          else if(rollAngle < -1) rollAngle = -1;
                          break;

        case GLUT_KEY_RIGHT:
                          rollAngle = -0.05;// degrees/s
                          if(rollAngle > 1) rollAngle = 1;
                          else if(rollAngle < -1) rollAngle = -1;
                          break;

        case GLUT_KEY_UP:
                          pitchAngle = -0.05;// degrees/s
                          if(pitchAngle > 1) pitchAngle = 1;
                          else if(pitchAngle < -1) pitchAngle = -1;
                          break;

        case GLUT_KEY_DOWN:
                          pitchAngle = 0.05;// degrees/s
                          if(pitchAngle > 1) pitchAngle = 1;
                          else if(pitchAngle < -1) pitchAngle = -1;
                          break;

        case 6:
            break;

        default:
            x = y;
            break;
    }
    glutPostRedisplay();
}

void KeyboardManagement(unsigned char tecla, int x, int y){
  switch(tecla){
    case 'w':
    case 'W': speed += 20;   // km/s
              break;
    case 's':
    case 'S': speed -= 20;   // km/s
              break;
    case 'a':
    case 'A':
              break;
    case 'l':
    case 'L':  wireframe = !wireframe;
              break;

    case ' ': speed = 0; //earth.camera.resetOrientation();
              break;

    default: x = y;
  }
}


// Application's main loop
void SimulationCallback(void) {
  clock_t timestamp;
  float dt;

  // Imprecise but works in most systems
  timestamp = clock();
  dt = ((float) (timestamp-oldtimestamp) / (float) CLOCKS_PER_SEC);
  oldtimestamp = timestamp;

  if(spin || fabs(speed) > 0.0002 || fabs(rollAngle) > 0.0002 || fabs(pitchAngle) > 0.0002) {
    earth.camera.advancePosition((float) speed*dt, 0.0f, 0.0f);
    earth.camera.setPerspective(fovh, (float) windowwidth / (float) windowheight, (float) (0.2*(sqrt(earth.camera.x*earth.camera.x + earth.camera.y*earth.camera.y + earth.camera.z*earth.camera.z)-SPHERE_RADIUS)), (float) (2*SPHERE_RADIUS+sqrt(earth.camera.x*earth.camera.x + earth.camera.y*earth.camera.y + earth.camera.z*earth.camera.z)));
    //earth.camera.setPerspective(fovh, (float) windowwidth / (float) windowheight, 2*SPHERE_RADIUS, 3*SPHERE_RADIUS);
    earth.camera.rotateOrientationEuler(pitchAngle, 0, 0);
    earth.camera.rotateOrientationEuler(0, 0, rollAngle);
    pitchAngle -= 4*pitchAngle*dt;
    rollAngle  -= 4*rollAngle*dt;
  }

    // Tesselate and estimate performance
    tpstessela.startingEvents();
    tpstessela.finishedEvents(arv.tesselate(70000, &earth));

    // Set redisplay to occurr
    glutPostRedisplay();
}

// Application cleanup code
void systemexit() {
  printf("\nStopping...");
  printf("\nMemory allocated at the end = %li\n", getAllocatedMemoryCounter());
  getchar();
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

// Main program
int main(int argv, char** argc) {
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

  printf("Memory allocated at the start = %li\n", getAllocatedMemoryCounter());

  // Setup GLUT
  glutInit(&argv, argc);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
  glutInitWindowSize(800, 600);
  glutCreateWindow("Terrain visualizer");
  glutDisplayFunc(Draw);
  glutReshapeFunc(WindowResized);
  glutMotionFunc(MouseMotionManagement);
  glutMouseFunc(MouseManagement);
  glutKeyboardFunc(KeyboardManagement);
  glutSpecialFunc(KeyboardManagementSpecial);
  glutIdleFunc(SimulationCallback);

  // Setup plots
  plot2.setAxis(GCG_PLOT_X, "Time (s)", 1.f, 1.f, 1.f, 1.f, 1.f, 0xffff);
  plot2.setAxis(GCG_PLOT_Y, "Triangles", 1.f, 1.f, 1.f, 1.f, 1.f, 0xffff);
  plot2.setAxisRange(GCG_PLOT_X, 0, 60);
  plot2.setAxisRange(GCG_PLOT_Y, 0, 150);
  plot2.setGrid(35, 20);
  plot_sintese = plot2.newLine();
  plot_tessel  = plot2.newLine();
  plot2.setLine((unsigned char) plot_sintese, NULL, 1.0f, 1.0f, 0.0f, 1.0f, 1.f, 0xffff);
  plot2.setLine((unsigned char) plot_tessel, NULL, 1.0f, 0.0f, 0.0f, 1.0f, 1.f, 0xffff);
  plot2.setPlotColor(7.f/255.f, 111.f/255.f, 141.f/255.f, 0.5);
  plot2.setGridColor(7.f/255.f, 147.f/255.f, 187.f/255.f, 0.8);

  plot.setAxis(GCG_PLOT_X, "Time (s)", 1.f, 1.f, 1.f, 1.f, 1.f, 0xffff);
  plot.setAxis(GCG_PLOT_Y, "Triangles", 1.f, 1.f, 1.f, 1.f, 1.f, 0xffff);
  plot.setAxisRange(GCG_PLOT_X, 0, 60);
  plot.setAxisRange(GCG_PLOT_Y, 0, 150);
  plot.setGrid(35, 20);
  plot_triang = plot.newLine();
  plot.setLine((unsigned char) plot_triang, NULL, 1.0, 1.0, 0.0, 1.0, 1.f, 0xffff);

  // Setup semiregular tesselator
  arv.setTopology(GCG_TOPOLOGY_CYLINDER);

  // First tesselation
  earth.camera.setPosition((float) SPHERE_RADIUS * 3.f, 0.0f, 0.0f);
  earth.camera.setTarget(0.f, 0.0f,0.0f);
  arv.tesselate(100, &earth);

  // OpenGL options
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
  glDisable(GL_POLYGON_OFFSET_LINE);
  glPolygonOffset(-0.5, -0.5);
  glShadeModel(GL_SMOOTH);
  glCullFace(GL_FRONT);

  // Affine transformations for earth's texture
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
	glScalef(1.0 / (TERRAIN_WIDTH-1), 1.0 / (TERRAIN_HEIGHT-1), 1.0);
	glTranslatef(0.0, TERRAIN_HEIGHT-1, 1.0);
	glScalef(1.0, -1.0, 1.0);

	oldtimestamp = clock();

	glutMainLoop();
}
