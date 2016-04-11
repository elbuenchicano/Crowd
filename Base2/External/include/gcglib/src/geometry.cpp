/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   GEOMETRY.CPP: functions for geometric computations

   Patricia Cordeiro Pereira Pampanelli
   Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     QUATERNION COMPUTATIONS  ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void gcgQuaternionSlerp(VECTOR4 dest, VECTOR4 p, VECTOR4 q, float t) {
	float omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = (p[0]-q[0])*(p[0]-q[0]) + (p[1]-q[1])*(p[1]-q[1]) + (p[2]-q[2])*(p[2]-q[2]) + (p[3]-q[3])*(p[3]-q[3]);
	float b = (p[0]+q[0])*(p[0]+q[0]) + (p[1]+q[1])*(p[1]+q[1]) + (p[2]+q[2])*(p[2]+q[2]) + (p[3]+q[3])*(p[3]+q[3]);

	if (a > b) { q[0] = -q[0];  q[1] = -q[1];  q[2] = -q[2]; q[3] = -q[3];	}

	cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

	if ((1.0 + cosom) > EPSILON) {
		if ((1.0 - cosom) > EPSILON) {
			omega = acosf( cosom );
			sinom = sinf( omega );
			sclp = (float) ((float) ((sinf( (1.0f - t)*omega) / sinom)));
			sclq = (float) ((float) (sinf(t*omega ) / sinom));
		}	else {
			sclp = (float) (1.0 - t);
			sclq = t;
		}
		dest[0] = sclp * p[0] + sclq * q[0];
		dest[1] = sclp * p[1] + sclq * q[1];
		dest[2] = sclp * p[2] + sclq * q[2];
		dest[3] = sclp * p[3] + sclq * q[3];
	} else {
		dest[0] = -p[1];
		dest[1] = p[0];
		dest[2] = -p[3];
		dest[3] = p[2];
		sclp = (float) (sinf( (float) ((1.0f - t) * 0.5 * M_PI)));
		sclq = (float) (sinf( (float) (t * 0.5f * M_PI)));

    dest[0] = sclp * p[0] + sclq * dest[0];
    dest[1] = sclp * p[1] + sclq * dest[1];
    dest[2] = sclp * p[2] + sclq * dest[2];
    dest[3] = sclp * p[3] + sclq * dest[3];
	}
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
void gcgQuaternionSlerp(VECTOR4d dest, VECTOR4d p, VECTOR4d q, double t) {
	double omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	double a = (p[0]-q[0])*(p[0]-q[0]) + (p[1]-q[1])*(p[1]-q[1]) + (p[2]-q[2])*(p[2]-q[2]) + (p[3]-q[3])*(p[3]-q[3]);
	double b = (p[0]+q[0])*(p[0]+q[0]) + (p[1]+q[1])*(p[1]+q[1]) + (p[2]+q[2])*(p[2]+q[2]) + (p[3]+q[3])*(p[3]+q[3]);

	if(a > b) { q[0] = -q[0];  q[1] = -q[1];  q[2] = -q[2]; q[3] = -q[3];	}

	cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

	if ((1.0 + cosom) > EPSILON) {
		if ((1.0 - cosom) > EPSILON) {
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = (double) (sin( (1.0 - t)*omega) / sinom);
			sclq = (double) (sin( t*omega ) / sinom);
		}	else {
			sclp = (double) 1.0 - t;
			sclq = t;
		}
		dest[0] = sclp * p[0] + sclq * q[0];
		dest[1] = sclp * p[1] + sclq * q[1];
		dest[2] = sclp * p[2] + sclq * q[2];
		dest[3] = sclp * p[3] + sclq * q[3];
	} else {
		dest[0] = -p[1];
		dest[1] = p[0];
		dest[2] = -p[3];
		dest[3] = p[2];
		sclp = (double) (sin( (1.0 - t) * 0.5 * M_PI));
		sclq = (double) (sin( t * 0.5 * M_PI));

    dest[0] = sclp * p[0] + sclq * dest[0];
    dest[1] = sclp * p[1] + sclq * dest[1];
    dest[2] = sclp * p[2] + sclq * dest[2];
    dest[3] = sclp * p[3] + sclq * dest[3];
	}
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     ALIGNMENT FUNCTIONS  ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Computes the rotation matrix that aligns the z-axis with the
// direction specified by vector dir.
bool gcgComputeAlignMatrix(MATRIX4 matrix, VECTOR3 dir) {
  matrix[3] = matrix[7] = matrix[11] = matrix[12] = matrix[13] = matrix[14] = 0.0f;
  matrix[15] = 1.0f;
  gcgNORMALIZEVECTOR3(&matrix[8], dir);
  if(fabs(matrix[10]) < (1.0f - EPSILON)) {
    gcgCROSSVECTOR3(&matrix[4], &matrix[13], &matrix[8]);
    gcgNORMALIZEVECTOR3(&matrix[4], &matrix[4]);
    gcgCROSSVECTOR3(&matrix[0], &matrix[8], &matrix[4]);
  } else matrix[0] = matrix[5] = matrix[10] = 1.0f;

  return true;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
bool gcgComputeAlignMatrix(MATRIX4d matrix, VECTOR3d dir) {
  matrix[3] = matrix[7] = matrix[11] = matrix[12] = matrix[13] = matrix[14] = 0.0;
  matrix[15] = 1.0;
  gcgNORMALIZEVECTOR3(&matrix[8], dir);
  if(fabs(matrix[10]) < (1.0 - EPSILON)) {
    gcgCROSSVECTOR3(&matrix[4], &matrix[13], &matrix[8]);
    gcgNORMALIZEVECTOR3(&matrix[4], &matrix[4]);
    gcgCROSSVECTOR3(&matrix[0], &matrix[8], &matrix[4]);
  } else matrix[0] = matrix[5] = matrix[10] = 1.0;

  return true;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     TRACKBALL SIMULATION ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/* Code from Gavin Bell
 * Project an x,y pair onto a sphere of radius r OR a hyperbolic sheet
 * if we are away from the center of the sphere.
 */
inline static float tb_project_to_sphere(float r, float x, float y) {
    float d, t, z;
    d = (float) sqrtf(x*x + y*y);
    if (d < (float) (r * 0.70710678118654752440)) {    /* Inside sphere */
        z = (float) sqrtf(r*r - d*d);
    } else {           /* On hyperbola */
        t = (float) (r / 1.41421356237309504880);
        z = t * t / d;
    }
    return z;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
inline static double tb_project_to_sphere(double r, double x, double y) {
    double d, t, z;
    d = (double) sqrt(x*x + y*y);
    if (d < (double) (r * 0.70710678118654752440)) {    /* Inside sphere */
        z = (double) sqrt(r*r - d*d);
    } else {           /* On hyperbola */
        t = (double) (r / 1.41421356237309504880);
        z = t * t / d;
    }
    return z;
}


#define TRACKBALLSIZE  (1.f)

/*
 * Ok, simulate a track-ball.  Project the points onto the virtual
 * trackball, then figure out the axis of rotation, which is the cross
 * product of P1 P2 and O P1 (O is the center of the ball, 0,0,0)
 * Note:  This is a deformed trackball-- is a trackball in the center,
 * but is deformed into a hyperbolic sheet of rotation away from the
 * center.  This particular function was chosen after trying out
 * several variations.
 *
 * It is assumed that the arguments to this routine are in the range
 * (-1.0 ... 1.0)
 */

///////////////////////////////////////////////////////////////////////////////
// Trackball simulation code based on the Gavin Bell's code.
///////////////////////////////////////////////////////////////////////////////
void gcgTrackball(VECTOR4 q, float p1x, float p1y, float p2x, float p2y) {
    VECTOR3 a; /* Axis of rotation */
    float   phi;  /* how much to rotate about axis */
    VECTOR3 p1, p2, d;
    float   t;

    if (p1x == p2x && p1y == p2y) {
        /* Zero rotation */
        gcgSETVECTOR4(q, 0.f, 0.f, 0.f, 1.0);
        return;
    }

    /*
     * First, figure out z-coordinates for projection of P1 and P2 to
     * deformed sphere
     */
    gcgSETVECTOR3(p1, p1x, p1y, (float) tb_project_to_sphere(TRACKBALLSIZE,p1x,p1y));
    gcgSETVECTOR3(p2, p2x, p2y, (float) tb_project_to_sphere(TRACKBALLSIZE,p2x,p2y));

    /*
     *  Now, we want the cross product of P1 and P2
     */
    gcgCROSSVECTOR3(a, p2, p1);

    /*
     *  Figure out how much to rotate around that axis.
     */
    gcgSUBVECTOR3(d, p1, p2);
    t = gcgLENGTHVECTOR3(d) / (TRACKBALLSIZE+TRACKBALLSIZE);

    /*
     * Avoid problems with out-of-control values...
     */
    if (t > 1.0) t = 1.0;
    if (t < -1.0) t = -1.0;
    phi = (float) asinf(t);
    phi += phi;

    gcgNORMALIZEVECTOR3(a, a);
    gcgAXISTOQUATERNION(q, phi, a);
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
void gcgTrackball(VECTOR4d q, double p1x, double p1y, double p2x, double p2y) {
    VECTOR3d a; /* Axis of rotation */
    double   phi;  /* how much to rotate about axis */
    VECTOR3d p1, p2, d;
    double   t;

    if (p1x == p2x && p1y == p2y) {
        /* Zero rotation */
        gcgSETVECTOR4(q, 0.f, 0.f, 0.f, 1.0);
        return;
    }

    /*
     * First, figure out z-coordinates for projection of P1 and P2 to
     * deformed sphere
     */
    gcgSETVECTOR3(p1, p1x, p1y, (double) tb_project_to_sphere((double) TRACKBALLSIZE,p1x,p1y));
    gcgSETVECTOR3(p2, p2x, p2y, (double) tb_project_to_sphere((double) TRACKBALLSIZE,p2x,p2y));

    /*
     *  Now, we want the cross product of P1 and P2
     */
    gcgCROSSVECTOR3(a, p2, p1);

    /*
     *  Figure out how much to rotate around that axis.
     */
    gcgSUBVECTOR3(d, p1, p2);
    t = gcgLENGTHVECTOR3(d) / (TRACKBALLSIZE+TRACKBALLSIZE);

    /*
     * Avoid problems with out-of-control values...
     */
    if (t > 1.0) t = 1.0;
    if (t < -1.0) t = -1.0;
    phi = (double) asin(t);
    phi += phi;

    gcgNORMALIZEVECTOR3(a, a);
    gcgAXISTOQUATERNION(q, phi, a);
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     FUNCTIONS FOR BOUNDING BOXES ////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void gcgAABoxFromOBox(VECTOR3 aabbmin, VECTOR3 aabbmax, VECTOR3 obbangles, VECTOR3 obbposition, VECTOR3 obbmin, VECTOR3 obbmax) {
  VECTOR3  p;
  VECTOR4  quat; // Quaternion

  // Build a quaternion from Euler angles
  gcgEULERTOQUATERNION(quat, (float) DEG2RAD(obbangles[0]), (float) DEG2RAD(obbangles[1]), (float) DEG2RAD(obbangles[2]));

  gcgSETVECTOR3(p, obbmin[0], obbmin[1], obbmin[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgCOPYVECTOR3(aabbmin, p);
  gcgCOPYVECTOR3(aabbmax, p);

  gcgSETVECTOR3(p, obbmin[0], obbmin[1], obbmax[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmin[0], obbmax[1], obbmin[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmin[0], obbmax[1], obbmax[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmax[0], obbmin[1], obbmin[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmax[0], obbmin[1], obbmax[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmax[0], obbmax[1], obbmin[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmax[0], obbmax[1], obbmax[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgADDVECTOR3(aabbmin, aabbmin, obbposition);
  gcgADDVECTOR3(aabbmax, aabbmax, obbposition);
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////

void gcgAABoxFromOBox(VECTOR3d aabbmin, VECTOR3d aabbmax, VECTOR3d obbangles, VECTOR3d obbposition, VECTOR3d obbmin, VECTOR3d obbmax) {
  VECTOR3d  p;
  VECTOR4d  quat; // Quaternion

  // Build a quaternion from Euler angles
  gcgEULERTOQUATERNION(quat, DEG2RAD(obbangles[0]), DEG2RAD(obbangles[1]), DEG2RAD(obbangles[2]));

  gcgSETVECTOR3(p, obbmin[0], obbmin[1], obbmin[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgCOPYVECTOR3(aabbmin, p);
  gcgCOPYVECTOR3(aabbmax, p);

  gcgSETVECTOR3(p, obbmin[0], obbmin[1], obbmax[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmin[0], obbmax[1], obbmin[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmin[0], obbmax[1], obbmax[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmax[0], obbmin[1], obbmin[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmax[0], obbmin[1], obbmax[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmax[0], obbmax[1], obbmin[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgSETVECTOR3(p, obbmax[0], obbmax[1], obbmax[2]);
  gcgAPPLYQUATERNIONVECTOR3(p, quat, p);
  gcgADDPPOINTAABox(aabbmin, aabbmax, p);

  gcgADDVECTOR3(aabbmin, aabbmin, obbposition);
  gcgADDVECTOR3(aabbmax, aabbmax, obbposition);
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     INTERSECTION TEST FUNCTIONS /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/* Triangle/triangle intersection test routine,
 * by Tomas Moller, 1997.
 * See article "A Fast Triangle-Triangle Intersection Test",
 * Journal of Graphics Tools, 2(2), 1997
 *
 * Updated June 1999: removed the divisions -- a little faster now!
 * Updated October 1999: added {} to CROSS and SUB macros
 *
 * int NoDivTriTriIsect(float V0[3],float V1[3],float V2[3],
 *                      float U0[3],float U1[3],float U2[3])
 *
 * parameters: vertices of triangle 1: V0,V1,V2
 *             vertices of triangle 2: U0,U1,U2
 * result    : returns 1 if the triangles intersect, otherwise 0
 *
 */

/* sort so that a<=b */
#define SORT(a,b)       \
             if(a>b) {  \
               float c; \
               c=a;     \
               a=b;     \
               b=c;     \
             }

#define SORTd(a,b)       \
             if(a>b) {   \
               double c; \
               c=a;      \
               a=b;      \
               b=c;      \
             }


/* sort so that a<=b */
#define SORT2(a, b, smallest) \
             if(a>b) {        \
               float c;       \
               c=a;           \
               a=b;           \
               b=c;           \
               smallest=1;    \
             } else smallest=0;

#define SORT2d(a, b, smallest) \
             if(a>b) {         \
               double c;       \
               c=a;            \
               a=b;            \
               b=c;            \
               smallest=1;     \
             } else smallest=0;


/* this edge to edge test is based on Franlin Antonio's gem:
   "Faster Line Segment Intersection", in Graphics Gems III,
   pp. 199-202 */

#define EDGE_AGAINST_TRI_EDGES(V0,V1,U0,U1,U2) {  \
  float ax,ay,bx,by,cx,cy,e,d,f;                  \
  ax=V1[i0]-V0[i0];                               \
  ay=V1[i1]-V0[i1];                               \
  /* test edge U0,U1 against V0,V1 */             \
  EDGE_EDGE_TEST(V0,U0,U1);                       \
  /* test edge U1,U2 against V0,V1 */             \
  EDGE_EDGE_TEST(V0,U1,U2);                       \
  /* test edge U2,U1 against V0,V1 */             \
  EDGE_EDGE_TEST(V0,U2,U0);                       \
  }

#define EDGE_AGAINST_TRI_EDGESd(V0,V1,U0,U1,U2) {  \
  double ax,ay,bx,by,cx,cy,e,d,f;                  \
  ax=V1[i0]-V0[i0];                               \
  ay=V1[i1]-V0[i1];                               \
  /* test edge U0,U1 against V0,V1 */             \
  EDGE_EDGE_TEST(V0,U0,U1);                       \
  /* test edge U1,U2 against V0,V1 */             \
  EDGE_EDGE_TEST(V0,U1,U2);                       \
  /* test edge U2,U1 against V0,V1 */             \
  EDGE_EDGE_TEST(V0,U2,U0);                       \
  }


#define EDGE_EDGE_TEST(V0,U0,U1)                      \
  bx=U0[i0]-U1[i0];                                   \
  by=U0[i1]-U1[i1];                                   \
  cx=V0[i0]-U0[i0];                                   \
  cy=V0[i1]-U0[i1];                                   \
  f=ay*bx-ax*by;                                      \
  d=by*cx-bx*cy;                                      \
  if((f>0 && d>=0 && d<=f) || (f<0 && d<=0 && d>=f))  \
  {                                                   \
    e=ax*cy-ay*cx;                                    \
    if(f>0)                                           \
    {                                                 \
      if(e>=0 && e<=f) return 1;                      \
    }                                                 \
    else                                              \
    {                                                 \
      if(e<=0 && e>=f) return 1;                      \
    }                                                 \
  }

#define POINT_IN_TRI(V0,U0,U1,U2) {         \
  float a,b,c,d0,d1,d2;                     \
  /* is T1 completly inside T2? */          \
  /* check if V0 is inside tri(U0,U1,U2) */ \
  a=U1[i1]-U0[i1];                          \
  b=-(U1[i0]-U0[i0]);                       \
  c=-a*U0[i0]-b*U0[i1];                     \
  d0=a*V0[i0]+b*V0[i1]+c;                   \
                                            \
  a=U2[i1]-U1[i1];                          \
  b=-(U2[i0]-U1[i0]);                       \
  c=-a*U1[i0]-b*U1[i1];                     \
  d1=a*V0[i0]+b*V0[i1]+c;                   \
                                            \
  a=U0[i1]-U2[i1];                          \
  b=-(U0[i0]-U2[i0]);                       \
  c=-a*U2[i0]-b*U2[i1];                     \
  d2=a*V0[i0]+b*V0[i1]+c;                   \
  if(d0*d1>0.0) {                           \
    if(d0*d2>0.0) return 1;                 \
  }                                         \
}

#define POINT_IN_TRId(V0,U0,U1,U2) {         \
  double a,b,c,d0,d1,d2;                     \
  /* is T1 completly inside T2? */          \
  /* check if V0 is inside tri(U0,U1,U2) */ \
  a=U1[i1]-U0[i1];                          \
  b=-(U1[i0]-U0[i0]);                       \
  c=-a*U0[i0]-b*U0[i1];                     \
  d0=a*V0[i0]+b*V0[i1]+c;                   \
                                            \
  a=U2[i1]-U1[i1];                          \
  b=-(U2[i0]-U1[i0]);                       \
  c=-a*U1[i0]-b*U1[i1];                     \
  d1=a*V0[i0]+b*V0[i1]+c;                   \
                                            \
  a=U0[i1]-U2[i1];                          \
  b=-(U0[i0]-U2[i0]);                       \
  c=-a*U2[i0]-b*U2[i1];                     \
  d2=a*V0[i0]+b*V0[i1]+c;                   \
  if(d0*d1>0.0) {                           \
    if(d0*d2>0.0) return 1;                 \
  }                                         \
}


#define NEWCOMPUTE_INTERVALS(VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,A,B,C,X0,X1) { \
        if(D0D1 > 0.0) { \
                /* here we know that D0D2<=0.0 */ \
            /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
                A=VV2; B=(VV0-VV2)*D2; C=(VV1-VV2)*D2; X0=D2-D0; X1=D2-D1; \
        } \
        else if(D0D2>0.0f) { \
                /* here we know that d0d1<=0.0 */ \
            A=VV1; B=(VV0-VV1)*D1; C=(VV2-VV1)*D1; X0=D1-D0; X1=D1-D2; \
        } \
        else if(D1*D2>0.0f || D0!=0.0f) { \
                /* here we know that d0d1<=0.0 or that D0!=0.0 */ \
                A=VV0; B=(VV1-VV0)*D0; C=(VV2-VV0)*D0; X0=D0-D1; X1=D0-D2; \
        } \
        else if(D1!=0.0f) { \
                A=VV1; B=(VV0-VV1)*D1; C=(VV2-VV1)*D1; X0=D1-D0; X1=D1-D2; \
        } \
        else if(D2!=0.0f) { \
                A=VV2; B=(VV0-VV2)*D2; C=(VV1-VV2)*D2; X0=D2-D0; X1=D2-D1; \
        } \
        else { \
                /* triangles are coplanar */ \
                return coplanar_tri_tri(N1,V0,V1,V2,U0,U1,U2); \
        } \
}

#define COMPUTE_INTERVALS_ISECTLINE(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,isect0,isect1,isectpoint0,isectpoint1) \
  if(D0D1 > 0.0) {                                       \
    /* here we know that D0D2<=0.0 */                   \
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }
#if 0
  else if(D0D2>0.0f) {                                  \
    /* here we know that d0d1<=0.0 */                   \
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else if(D1*D2>0.0f || D0!=0.0f) {                     \
    /* here we know that d0d1<=0.0 or that D0!=0.0 */   \
    isect2(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else if(D1!=0.0f) {                                   \
    isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else if(D2!=0.0f) {                                   \
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,&isect0,&isect1,isectpoint0,isectpoint1);          \
  }                                                     \
  else {                                                \
    /* triangles are coplanar */                        \
    coplanar=1;                                         \
    return coplanar_tri_tri(N1,V0,V1,V2,U0,U1,U2);      \
  }
#endif


// coplanar_tri_tri: auxiliary function
static inline int coplanar_tri_tri(VECTOR3 N,  VECTOR3 V0, VECTOR3 V1, VECTOR3 V2, VECTOR3 U0, VECTOR3 U1, VECTOR3 U2) {
   VECTOR3 A;
   short   i0,i1;
   /* first project onto an axis-aligned plane, that maximizes the area */
   /* of the triangles, compute indices: i0,i1. */
   A[0] = fabsf(N[0]);
   A[1] = fabsf(N[1]);
   A[2] = fabsf(N[2]);
   if(A[0]>A[1]) {
      if(A[0] > A[2]) {
          i0=1;      /* A[0] is greatest */
          i1=2;
      } else {
          i0=0;      /* A[2] is greatest */
          i1=1;
      }
   } else {  /* A[0]<=A[1] */
      if(A[2] > A[1]) {
          i0=0;      /* A[2] is greatest */
          i1=1;
      } else {
          i0=0;      /* A[1] is greatest */
          i1=2;
      }
    }

    /* test all edges of triangle 1 against the edges of triangle 2 */
    EDGE_AGAINST_TRI_EDGES(V0,V1,U0,U1,U2);
    EDGE_AGAINST_TRI_EDGES(V1,V2,U0,U1,U2);
    EDGE_AGAINST_TRI_EDGES(V2,V0,U0,U1,U2);

    /* finally, test if tri1 is totally contained in tri2 or vice versa */
    POINT_IN_TRI(V0,U0,U1,U2);
    POINT_IN_TRI(U0,V0,V1,V2);

    return FALSE;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
static inline int coplanar_tri_tri(VECTOR3d N,  VECTOR3d V0, VECTOR3d V1, VECTOR3d V2, VECTOR3d U0, VECTOR3d U1, VECTOR3d U2) {
   VECTOR3d A;
   short    i0,i1;
   /* first project onto an axis-aligned plane, that maximizes the area */
   /* of the triangles, compute indices: i0,i1. */
   A[0] = fabs(N[0]);
   A[1] = fabs(N[1]);
   A[2] = fabs(N[2]);
   if(A[0]>A[1]) {
      if(A[0] > A[2]) {
          i0=1;      /* A[0] is greatest */
          i1=2;
      } else {
          i0=0;      /* A[2] is greatest */
          i1=1;
      }
   } else {  /* A[0]<=A[1] */
      if(A[2] > A[1]) {
          i0=0;      /* A[2] is greatest */
          i1=1;
      } else {
          i0=0;      /* A[1] is greatest */
          i1=2;
      }
    }

    /* test all edges of triangle 1 against the edges of triangle 2 */
    EDGE_AGAINST_TRI_EDGESd(V0,V1,U0,U1,U2);
    EDGE_AGAINST_TRI_EDGESd(V1,V2,U0,U1,U2);
    EDGE_AGAINST_TRI_EDGESd(V2,V0,U0,U1,U2);

    /* finally, test if tri1 is totally contained in tri2 or vice versa */
    POINT_IN_TRId(V0,U0,U1,U2);
    POINT_IN_TRId(U0,V0,V1,V2);

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////////
// Returns true if both triangles intersect.
/////////////////////////////////////////////////////////////////////////////////////
int gcgIntersectTriangleTriangle(VECTOR3 V0, VECTOR3 V1, VECTOR3 V2, VECTOR3 U0, VECTOR3 U1, VECTOR3 U2) {
  float E1[3],E2[3];
  float N1[3],N2[3],d1,d2;
  float du0,du1,du2,dv0,dv1,dv2;
  float D[3];
  float isect1[2], isect2[2];
  float du0du1,du0du2,dv0dv1,dv0dv2;
  short index;
  float vp0,vp1,vp2;
  float up0,up1,up2;
  float bb,cc,max;

  /* compute plane equation of triangle(V0,V1,V2) */
  gcgSUBVECTOR3(E1, V1, V0);
  gcgSUBVECTOR3(E2, V2, V0);
  gcgCROSSVECTOR3(N1, E1, E2);
  d1 = -gcgDOTVECTOR3(N1, V0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0 = gcgDOTVECTOR3(N1, U0) + d1;
  du1 = gcgDOTVECTOR3(N1, U1) + d1;
  du2 = gcgDOTVECTOR3(N1, U2) + d1;

  /* coplanarity robustness check */
  if(fabs(du0) < EPSILON) du0 = 0.0;
  if(fabs(du1) < EPSILON) du1 = 0.0;
  if(fabs(du2) < EPSILON) du2 = 0.0;

  du0du1 = du0 * du1;
  du0du2 = du0 * du2;

  if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  gcgSUBVECTOR3(E1, U1, U0);
  gcgSUBVECTOR3(E2, U2, U0);
  gcgCROSSVECTOR3(N2, E1, E2);
  d2 = -gcgDOTVECTOR3(N2, U0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0 = gcgDOTVECTOR3(N2, V0) + d2;
  dv1 = gcgDOTVECTOR3(N2, V1) + d2;
  dv2 = gcgDOTVECTOR3(N2, V2) + d2;

  if(fabsf(dv0) < EPSILON) dv0 = 0.0;
  if(fabsf(dv1) < EPSILON) dv1 = 0.0;
  if(fabsf(dv2) < EPSILON) dv2 = 0.0;

  dv0dv1 = dv0 * dv1;
  dv0dv2 = dv0 * dv2;

  if(dv0dv1 > 0.0f && dv0dv2 > 0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute direction of intersection line */
  gcgCROSSVECTOR3(D, N1, N2);

  /* compute and index to the largest component of D */
  max = (float) fabsf(D[0]);
  index=0;
  bb  = (float) fabsf(D[1]);
  cc = (float)fabsf(D[2]);
  if(bb > max) max = bb,index = 1;
  if(cc > max) max = cc,index = 2;

  /* this is the simplified projection onto L*/
  vp0 = V0[index];
  vp1 = V1[index];
  vp2 = V2[index];

  up0 = U0[index];
  up1 = U1[index];
  up2 = U2[index];

  /* compute interval for triangle 1 */
  float a, b, c, x0, x1;
  NEWCOMPUTE_INTERVALS(vp0,vp1,vp2,dv0,dv1,dv2,dv0dv1,dv0dv2,a,b,c,x0,x1);

  /* compute interval for triangle 2 */
  float d,e,f,y0,y1;
  NEWCOMPUTE_INTERVALS(up0,up1,up2,du0,du1,du2,du0du1,du0du2,d,e,f,y0,y1);

  float xx, yy, xxyy, tmp;
  xx = x0 * x1;
  yy = y0 * y1;
  xxyy = xx * yy;

  tmp=a * xxyy;
  isect1[0] = tmp + b * x1 * yy;
  isect1[1] = tmp + c * x0 * yy;

  tmp = d * xxyy;
  isect2[0] = tmp + e * xx * y1;
  isect2[1] = tmp + f * xx * y0;

  SORT(isect1[0], isect1[1]);
  SORT(isect2[0], isect2[1]);

  if(isect1[1] < isect2[0] || isect2[1] < isect1[0]) return FALSE;
  return TRUE;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
int gcgIntersectTriangleTriangle(VECTOR3d V0, VECTOR3d V1, VECTOR3d V2, VECTOR3d U0, VECTOR3d U1, VECTOR3d U2) {
  double E1[3],E2[3];
  double N1[3],N2[3],d1,d2;
  double du0,du1,du2,dv0,dv1,dv2;
  double D[3];
  double isect1[2], isect2[2];
  double du0du1,du0du2,dv0dv1,dv0dv2;
  short index;
  double vp0,vp1,vp2;
  double up0,up1,up2;
  double bb,cc,max;

  /* compute plane equation of triangle(V0,V1,V2) */
  gcgSUBVECTOR3(E1, V1, V0);
  gcgSUBVECTOR3(E2, V2, V0);
  gcgCROSSVECTOR3(N1, E1, E2);
  d1 = -gcgDOTVECTOR3(N1, V0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0 = gcgDOTVECTOR3(N1, U0) + d1;
  du1 = gcgDOTVECTOR3(N1, U1) + d1;
  du2 = gcgDOTVECTOR3(N1, U2) + d1;

  /* coplanarity robustness check */
  if(fabs(du0) < EPSILON) du0 = 0.0;
  if(fabs(du1) < EPSILON) du1 = 0.0;
  if(fabs(du2) < EPSILON) du2 = 0.0;

  du0du1 = du0 * du1;
  du0du2 = du0 * du2;

  if(du0du1>0.0f && du0du2>0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  gcgSUBVECTOR3(E1, U1, U0);
  gcgSUBVECTOR3(E2, U2, U0);
  gcgCROSSVECTOR3(N2, E1, E2);
  d2 = -gcgDOTVECTOR3(N2, U0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0 = gcgDOTVECTOR3(N2, V0) + d2;
  dv1 = gcgDOTVECTOR3(N2, V1) + d2;
  dv2 = gcgDOTVECTOR3(N2, V2) + d2;

  if(fabs(dv0) < EPSILON) dv0 = 0.0;
  if(fabs(dv1) < EPSILON) dv1 = 0.0;
  if(fabs(dv2) < EPSILON) dv2 = 0.0;

  dv0dv1 = dv0 * dv1;
  dv0dv2 = dv0 * dv2;

  if(dv0dv1 > 0.0f && dv0dv2 > 0.0f) /* same sign on all of them + not equal 0 ? */
    return 0;                    /* no intersection occurs */

  /* compute direction of intersection line */
  gcgCROSSVECTOR3(D, N1, N2);

  /* compute and index to the largest component of D */
  max = (double) fabs(D[0]);
  index = 0;
  bb  = (double) fabs(D[1]);
  cc = (double)fabs(D[2]);
  if(bb > max) max = bb,index = 1;
  if(cc > max) max = cc,index = 2;

  /* this is the simplified projection onto L*/
  vp0 = V0[index];
  vp1 = V1[index];
  vp2 = V2[index];

  up0 = U0[index];
  up1 = U1[index];
  up2 = U2[index];

  /* compute interval for triangle 1 */
  double a, b, c, x0, x1;
  NEWCOMPUTE_INTERVALS(vp0,vp1,vp2,dv0,dv1,dv2,dv0dv1,dv0dv2,a,b,c,x0,x1);

  /* compute interval for triangle 2 */
  double d,e,f,y0,y1;
  NEWCOMPUTE_INTERVALS(up0,up1,up2,du0,du1,du2,du0du1,du0du2,d,e,f,y0,y1);

  double xx, yy, xxyy, tmp;
  xx = x0 * x1;
  yy = y0 * y1;
  xxyy = xx * yy;

  tmp = a * xxyy;
  isect1[0] = tmp + b * x1 * yy;
  isect1[1] = tmp + c * x0 * yy;

  tmp = d * xxyy;
  isect2[0] = tmp + e * xx * y1;
  isect2[1] = tmp + f * xx * y0;

  SORTd(isect1[0],isect1[1]);
  SORTd(isect2[0],isect2[1]);

  if(isect1[1] < isect2[0] || isect2[1] < isect1[0]) return FALSE;
  return TRUE;
}


// isect2: auxiliary function
static inline void isect2(float VTX0[3], float VTX1[3], float VTX2[3], float VV0, float VV1, float VV2,
	float D0, float D1, float D2, float *isect0, float *isect1, VECTOR3 isectpoint0, VECTOR3 isectpoint1) {
  float tmp = D0 / (D0-D1);
  VECTOR3 diff;

  *isect0 = VV0 + (VV1-VV0) * tmp;
  gcgSUBVECTOR3(diff, VTX1, VTX0);
  gcgSCALEVECTOR3(diff, diff, tmp);
  gcgADDVECTOR3(isectpoint0, diff, VTX0);
  tmp = D0 / (D0-D2);
  *isect1 = VV0 + (VV2-VV0) * tmp;
  gcgSUBVECTOR3(diff, VTX2, VTX0);
  gcgSCALEVECTOR3(diff, diff, tmp);
  gcgADDVECTOR3(isectpoint1, VTX0, diff);
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
static inline void isect2(double VTX0[3], double VTX1[3], double VTX2[3], double VV0, double VV1, double VV2,
	double D0, double D1, double D2, double *isect0, double *isect1, VECTOR3d isectpoint0, VECTOR3d isectpoint1) {
  double tmp = D0 / (D0-D1);
  VECTOR3d diff;

  *isect0 = VV0 + (VV1-VV0) * tmp;
  gcgSUBVECTOR3(diff, VTX1, VTX0);
  gcgSCALEVECTOR3(diff, diff, tmp);
  gcgADDVECTOR3(isectpoint0, diff, VTX0);
  tmp = D0 / (D0-D2);
  *isect1 = VV0 + (VV2-VV0) * tmp;
  gcgSUBVECTOR3(diff, VTX2, VTX0);
  gcgSCALEVECTOR3(diff, diff, tmp);
  gcgADDVECTOR3(isectpoint1, VTX0, diff);
}


// compute_intervals_isectline: auxiliary function
static inline int compute_intervals_isectline(float VERT0[3],float VERT1[3],float VERT2[3],
				       float VV0,float VV1,float VV2,float D0,float D1,float D2,
				       float D0D1,float D0D2,float *isect0,float *isect1,
				       float isectpoint0[3],float isectpoint1[3]) {
  if(D0D1 > 0.0f)
    /* here we know that D0D2<=0.0 */
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,isect0,isect1,isectpoint0,isectpoint1);
  else
    if(D0D2 > 0.0f)  /* here we know that d0d1<=0.0 */
      isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,isect0,isect1,isectpoint0,isectpoint1);
    else if(D1*D2 > 0.0f || D0 != 0.0f) /* here we know that d0d1<=0.0 or that D0!=0.0 */
            isect2(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,isect0,isect1,isectpoint0,isectpoint1);
         else if(D1!=0.0f) isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,isect0,isect1,isectpoint0,isectpoint1);
              else if(D2!=0.0f)
                      isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,isect0,isect1,isectpoint0,isectpoint1);
                   else  return TRUE; /* triangles are coplanar */

  return FALSE;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
static inline int compute_intervals_isectline(double VERT0[3], double VERT1[3], double VERT2[3],
				       double VV0, double VV1, double VV2, double D0, double D1, double D2,
				       double D0D1, double D0D2, double *isect0, double *isect1,
				       double isectpoint0[3], double isectpoint1[3]) {
  if(D0D1 > 0.0f)
    /* here we know that D0D2<=0.0 */
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */
    isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,isect0,isect1,isectpoint0,isectpoint1);
  else
    if(D0D2 > 0.0f)  /* here we know that d0d1<=0.0 */
      isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,isect0,isect1,isectpoint0,isectpoint1);

    else if(D1*D2 > 0.0f || D0 != 0.0f) /* here we know that d0d1<=0.0 or that D0!=0.0 */
            isect2(VERT0,VERT1,VERT2,VV0,VV1,VV2,D0,D1,D2,isect0,isect1,isectpoint0,isectpoint1);
         else if(D1!=0.0f) isect2(VERT1,VERT0,VERT2,VV1,VV0,VV2,D1,D0,D2,isect0,isect1,isectpoint0,isectpoint1);
              else if(D2!=0.0f)
                      isect2(VERT2,VERT0,VERT1,VV2,VV0,VV1,D2,D0,D1,isect0,isect1,isectpoint0,isectpoint1);
                   else  return TRUE; /* triangles are coplanar */

  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////////
// Returns true if both triangles intersect. Also gives the intersection line if the
// triangles are not coplanar
/////////////////////////////////////////////////////////////////////////////////////
int gcgIntersectTriangleTriangleLine(VECTOR3 V0, VECTOR3 V1, VECTOR3 V2, VECTOR3 U0, VECTOR3 U1, VECTOR3 U2, int *coplanar, VECTOR3 isectpt1, VECTOR3 isectpt2) {
  VECTOR3 E1, E2, N1, N2, D, isectpointA1, isectpointA2, isectpointB1 = {0, 0, 0}, isectpointB2 = {0, 0, 0};
  float d1, d2;
  float du0, du1, du2, dv0, dv1, dv2;
  float isect1[2] = {0, 0}, isect2[2] = {0, 0};
  float du0du1, du0du2, dv0dv1, dv0dv2;
  short index;
  float vp0, vp1, vp2;
  float up0, up1, up2;
  float b,c,max;
  int smallest1, smallest2;

  /* compute plane equation of triangle(V0,V1,V2) */
  gcgSUBVECTOR3(E1, V1, V0);
  gcgSUBVECTOR3(E2, V2, V0);
  gcgCROSSVECTOR3(N1, E1, E2);
  d1 = -gcgDOTVECTOR3(N1,V0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0 = gcgDOTVECTOR3(N1, U0)+d1;
  du1 = gcgDOTVECTOR3(N1, U1)+d1;
  du2 = gcgDOTVECTOR3(N1, U2)+d1;

  /* coplanarity robustness check */
  if(fabsf(du0) < EPSILON) du0 = 0.0;
  if(fabsf(du1) < EPSILON) du1 = 0.0;
  if(fabsf(du2) < EPSILON) du2 = 0.0;

  du0du1 = du0 * du1;
  du0du2 = du0 * du2;

  if(du0du1 > 0.0f && du0du2 > 0.0f) /* same sign on all of them + not equal 0 ? */
    return FALSE;                    /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  gcgSUBVECTOR3(E1, U1, U0);
  gcgSUBVECTOR3(E2, U2, U0);
  gcgCROSSVECTOR3(N2, E1, E2);
  d2 = -gcgDOTVECTOR3(N2,U0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0 = gcgDOTVECTOR3(N2, V0) + d2;
  dv1 = gcgDOTVECTOR3(N2, V1) + d2;
  dv2 = gcgDOTVECTOR3(N2, V2) + d2;

  if(fabsf(dv0) < EPSILON) dv0 = 0.0;
  if(fabsf(dv1) < EPSILON) dv1 = 0.0;
  if(fabsf(dv2) < EPSILON) dv2 = 0.0;

  dv0dv1 = dv0 * dv1;
  dv0dv2 = dv0 * dv2;

  if(dv0dv1 > 0.0f && dv0dv2 > 0.0f) /* same sign on all of them + not equal 0 ? */
    return FALSE;                    /* no intersection occurs */

  /* compute direction of intersection line */
  gcgCROSSVECTOR3(D, N1, N2);

  /* compute and index to the largest component of D */
  max = fabsf(D[0]);
  index = 0;
  b = fabsf(D[1]);
  c = fabsf(D[2]);
  if(b > max) max = b, index = 1;
  if(c > max) max = c, index = 2;

  /* this is the simplified projection onto L*/
  vp0 = V0[index];
  vp1 = V1[index];
  vp2 = V2[index];

  up0 = U0[index];
  up1 = U1[index];
  up2 = U2[index];

  /* compute interval for triangle 1 */
  *coplanar=compute_intervals_isectline(V0,V1,V2,vp0,vp1,vp2,dv0,dv1,dv2,
				       dv0dv1,dv0dv2,&isect1[0],&isect1[1],isectpointA1,isectpointA2);
  if(*coplanar) return coplanar_tri_tri(N1,V0,V1,V2,U0,U1,U2);


  /* compute interval for triangle 2 */
  compute_intervals_isectline(U0,U1,U2,up0,up1,up2,du0,du1,du2,
			      du0du1,du0du2,&isect2[0],&isect2[1],isectpointB1,isectpointB2);

  SORT2(isect1[0], isect1[1], smallest1);
  SORT2(isect2[0], isect2[1], smallest2);

  if(isect1[1] < isect2[0] || isect2[1] < isect1[0]) return FALSE;

  /* at this point, we know that the triangles intersect */

  if(isect2[0] < isect1[0]) {
    if(smallest1 == 0) { gcgCOPYVECTOR3(isectpt1, isectpointA1); }
    else gcgCOPYVECTOR3(isectpt1,isectpointA2);
    if(isect2[1] < isect1[1]) {
      if(smallest2 == 0) { gcgCOPYVECTOR3(isectpt2, isectpointB2); }
      else gcgCOPYVECTOR3(isectpt2,isectpointB1);
    } else {
      if(smallest1 == 0) { gcgCOPYVECTOR3(isectpt2, isectpointA2); }
      else gcgCOPYVECTOR3(isectpt2, isectpointA1);
    }
  } else {
    if(smallest2 == 0) { gcgCOPYVECTOR3(isectpt1,isectpointB1); }
    else gcgCOPYVECTOR3(isectpt1,isectpointB2);

    if(isect2[1] > isect1[1]) {
      if(smallest1 == 0) { gcgCOPYVECTOR3(isectpt2,isectpointA2); }
      else gcgCOPYVECTOR3(isectpt2,isectpointA1);
    } else {
      if(smallest2 == 0) { gcgCOPYVECTOR3(isectpt2,isectpointB2); }
      else gcgCOPYVECTOR3(isectpt2,isectpointB1);
    }
  }

  return TRUE;
}


////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
int gcgIntersectTriangleTriangleLine(VECTOR3d V0, VECTOR3d V1, VECTOR3d V2, VECTOR3d U0, VECTOR3d U1, VECTOR3d U2, int *coplanar, VECTOR3d isectpt1, VECTOR3d isectpt2) {
  VECTOR3d E1, E2, N1, N2, D, isectpointA1, isectpointA2, isectpointB1 = {0, 0, 0}, isectpointB2 = {0, 0, 0};
  double d1, d2;
  double du0, du1, du2, dv0, dv1, dv2;
  double isect1[2] = {0, 0}, isect2[2] = {0, 0};
  double du0du1, du0du2, dv0dv1, dv0dv2;
  short index;
  double vp0, vp1, vp2;
  double up0, up1, up2;
  double b,c,max;
  int smallest1, smallest2;

  /* compute plane equation of triangle(V0,V1,V2) */
  gcgSUBVECTOR3(E1, V1, V0);
  gcgSUBVECTOR3(E2, V2, V0);
  gcgCROSSVECTOR3(N1, E1, E2);
  d1 = -gcgDOTVECTOR3(N1,V0);
  /* plane equation 1: N1.X+d1=0 */

  /* put U0,U1,U2 into plane equation 1 to compute signed distances to the plane*/
  du0 = gcgDOTVECTOR3(N1, U0)+d1;
  du1 = gcgDOTVECTOR3(N1, U1)+d1;
  du2 = gcgDOTVECTOR3(N1, U2)+d1;

  /* coplanarity robustness check */
  if(fabs(du0) < EPSILON) du0 = 0.0;
  if(fabs(du1) < EPSILON) du1 = 0.0;
  if(fabs(du2) < EPSILON) du2 = 0.0;

  du0du1 = du0 * du1;
  du0du2 = du0 * du2;

  if(du0du1 > 0.0f && du0du2 > 0.0f) /* same sign on all of them + not equal 0 ? */
    return FALSE;                    /* no intersection occurs */

  /* compute plane of triangle (U0,U1,U2) */
  gcgSUBVECTOR3(E1, U1, U0);
  gcgSUBVECTOR3(E2, U2, U0);
  gcgCROSSVECTOR3(N2, E1, E2);
  d2 = -gcgDOTVECTOR3(N2,U0);
  /* plane equation 2: N2.X+d2=0 */

  /* put V0,V1,V2 into plane equation 2 */
  dv0 = gcgDOTVECTOR3(N2, V0) + d2;
  dv1 = gcgDOTVECTOR3(N2, V1) + d2;
  dv2 = gcgDOTVECTOR3(N2, V2) + d2;

  if(fabs(dv0) < EPSILON) dv0 = 0.0;
  if(fabs(dv1) < EPSILON) dv1 = 0.0;
  if(fabs(dv2) < EPSILON) dv2 = 0.0;

  dv0dv1 = dv0 * dv1;
  dv0dv2 = dv0 * dv2;

  if(dv0dv1 > 0.0f && dv0dv2 > 0.0f) /* same sign on all of them + not equal 0 ? */
    return FALSE;                    /* no intersection occurs */

  /* compute direction of intersection line */
  gcgCROSSVECTOR3(D, N1, N2);

  /* compute and index to the largest component of D */
  max = fabs(D[0]);
  index = 0;
  b = fabs(D[1]);
  c = fabs(D[2]);
  if(b > max) max = b, index = 1;
  if(c > max) max = c, index = 2;

  /* this is the simplified projection onto L*/
  vp0 = V0[index];
  vp1 = V1[index];
  vp2 = V2[index];

  up0 = U0[index];
  up1 = U1[index];
  up2 = U2[index];

  /* compute interval for triangle 1 */
  *coplanar=compute_intervals_isectline(V0,V1,V2,vp0,vp1,vp2,dv0,dv1,dv2,
				       dv0dv1,dv0dv2,&isect1[0],&isect1[1],isectpointA1,isectpointA2);
  if(*coplanar) return coplanar_tri_tri(N1,V0,V1,V2,U0,U1,U2);


  /* compute interval for triangle 2 */
  compute_intervals_isectline(U0,U1,U2,up0,up1,up2,du0,du1,du2,
			      du0du1,du0du2,&isect2[0],&isect2[1],isectpointB1,isectpointB2);

  SORT2d(isect1[0], isect1[1], smallest1);
  SORT2d(isect2[0], isect2[1], smallest2);

  if(isect1[1] < isect2[0] || isect2[1] < isect1[0]) return FALSE;

  /* at this point, we know that the triangles intersect */

  if(isect2[0] < isect1[0]) {
    if(smallest1 == 0) { gcgCOPYVECTOR3(isectpt1, isectpointA1); }
    else gcgCOPYVECTOR3(isectpt1,isectpointA2);
    if(isect2[1] < isect1[1]) {
      if(smallest2==0) { gcgCOPYVECTOR3(isectpt2, isectpointB2); }
      else gcgCOPYVECTOR3(isectpt2,isectpointB1);
    } else {
      if(smallest1 == 0) { gcgCOPYVECTOR3(isectpt2, isectpointA2); }
      else gcgCOPYVECTOR3(isectpt2, isectpointA1);
    }
  } else {
    if(smallest2 == 0) { gcgCOPYVECTOR3(isectpt1,isectpointB1); }
    else gcgCOPYVECTOR3(isectpt1,isectpointB2);

    if(isect2[1] > isect1[1]) {
      if(smallest1 == 0) { gcgCOPYVECTOR3(isectpt2,isectpointA2); }
      else gcgCOPYVECTOR3(isectpt2,isectpointA1);
    } else {
      if(smallest2 == 0) { gcgCOPYVECTOR3(isectpt2,isectpointB2); }
      else gcgCOPYVECTOR3(isectpt2,isectpointB1);
    }
  }

  return TRUE;
}





////////////////////////////////////////////////////////////////////////////
// http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline3d/
// Computes the intersection of two 3d lines
////////////////////////////////////////////////////////////////////////////
int gcgIntersectLineLine(VECTOR3 p1, VECTOR3 p2, VECTOR3 p3, VECTOR3 p4, VECTOR3 pa, VECTOR3 pb,  double *mua, double *mub) {
   VECTOR3 p13, p43, p21;
   double d1343, d4321, d1321, d4343, d2121;
   double numer, denom;

   gcgSUBVECTOR3(p13, p1, p3);
   gcgSUBVECTOR3(p43, p4, p3);

   if(ABS(p43[0]) < EPSILON && ABS(p43[1])  < EPSILON && ABS(p43[2])  < EPSILON) return FALSE;

   gcgSUBVECTOR3(p21, p2, p1);
   if (ABS(p21[0])  < EPSILON && ABS(p21[1])  < EPSILON && ABS(p21[2])  < EPSILON) return FALSE;

   d1343 = gcgDOTVECTOR3(p13, p43); //p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
   d4321 = gcgDOTVECTOR3(p43, p21); //p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
   d1321 = gcgDOTVECTOR3(p13, p21); //p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
   d4343 = gcgDOTVECTOR3(p43, p43); //p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
   d2121 = gcgDOTVECTOR3(p21, p21); //p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

   denom = d2121 * d4343 - d4321 * d4321;
   if (ABS(denom) < EPSILON) return FALSE;
   numer = d1343 * d4321 - d1321 * d4343;

   *mua = numer / denom;
   *mub = (d1343 + d4321 * (*mua)) / d4343;

   pa[0] = (float) (p1[0] + *mua * p21[0]);
   pa[1] = (float) (p1[1] + *mua * p21[1]);
   pa[2] = (float) (p1[2] + *mua * p21[2]);

   pb[0] = (float) (p3[0] + *mub * p43[0]);
   pb[1] = (float) (p3[1] + *mub * p43[1]);
   pb[2] = (float) (p3[2] + *mub * p43[2]);

   return TRUE;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
int gcgIntersectLineLine(VECTOR3d p1, VECTOR3d p2, VECTOR3d p3, VECTOR3d p4, VECTOR3d pa, VECTOR3d pb,  double *mua, double *mub) {
   VECTOR3d p13, p43, p21;
   double d1343, d4321, d1321, d4343, d2121;
   double numer, denom;

   gcgSUBVECTOR3(p13, p1, p3);
   gcgSUBVECTOR3(p43, p4, p3);

   if(ABS(p43[0]) < EPSILON && ABS(p43[1])  < EPSILON && ABS(p43[2])  < EPSILON) return FALSE;

   gcgSUBVECTOR3(p21, p2, p1);
   if (ABS(p21[0])  < EPSILON && ABS(p21[1])  < EPSILON && ABS(p21[2])  < EPSILON) return FALSE;

   d1343 = gcgDOTVECTOR3(p13, p43); //p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
   d4321 = gcgDOTVECTOR3(p43, p21); //p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
   d1321 = gcgDOTVECTOR3(p13, p21); //p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
   d4343 = gcgDOTVECTOR3(p43, p43); //p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
   d2121 = gcgDOTVECTOR3(p21, p21); //p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

   denom = d2121 * d4343 - d4321 * d4321;
   if (ABS(denom) < EPSILON) return FALSE;
   numer = d1343 * d4321 - d1321 * d4343;

   *mua = numer / denom;
   *mub = (d1343 + d4321 * (*mua)) / d4343;

   pa[0] = (double) (p1[0] + *mua * p21[0]);
   pa[1] = (double) (p1[1] + *mua * p21[1]);
   pa[2] = (double) (p1[2] + *mua * p21[2]);

   pb[0] = (double) (p3[0] + *mub * p43[0]);
   pb[1] = (double) (p3[1] + *mub * p43[1]);
   pb[2] = (double) (p3[2] + *mub * p43[2]);

   return TRUE;
}


////////////////////////////////////////////////////////////////////////////
// Returns true if the point is on the segment.
////////////////////////////////////////////////////////////////////////////
int gcgIntersectPointLine(VECTOR3 ponto, VECTOR3 edgeA, VECTOR3 edgeB) {
  VECTOR3 u1, u2;
  gcgSUBVECTOR3(u1, edgeA, ponto);
  gcgSUBVECTOR3(u2, edgeB, ponto);
  return gcgDOTVECTOR3(u1, u2) <= EPSILON;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
int gcgIntersectPointLine(VECTOR3d ponto, VECTOR3d edgeA, VECTOR3d edgeB) {
  VECTOR3d u1, u2;
  gcgSUBVECTOR3(u1, edgeA, ponto);
  gcgSUBVECTOR3(u2, edgeB, ponto);
  return gcgDOTVECTOR3(u1, u2) <= EPSILON;
}



////////////////////////////////////////////////////////////////////////////
// Returns true if the point is on the triangle.
// http://www.blackpawn.com/texts/pointinpoly/default.html
////////////////////////////////////////////////////////////////////////////
int gcgIntersectPointTriangle(VECTOR3 p, VECTOR3 A, VECTOR3 B, VECTOR3 C) {
  VECTOR3 v0, v1, v2;
  float   u, v, dot00, dot01, dot02, dot11, dot12, invDenom;
  // Compute vectors
  gcgSUBVECTOR3(v0, C, A);
  gcgSUBVECTOR3(v1, B, A);
  gcgSUBVECTOR3(v2, p, A);

  // Compute dot products
  dot00 = gcgDOTVECTOR3(v0, v0);
  dot01 = gcgDOTVECTOR3(v0, v1);
  dot02 = gcgDOTVECTOR3(v0, v2);
  dot11 = gcgDOTVECTOR3(v1, v1);
  dot12 = gcgDOTVECTOR3(v1, v2);

  // Compute barycentric coordinates
  invDenom = (float) (1.0 / (dot00 * dot11 - dot01 * dot01));
  u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  // Check if point is in triangle
  return (u > 0.0) && (v > 0.0) && (u + v < 1.0);
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
int gcgIntersectPointTriangle(VECTOR3d p, VECTOR3d A, VECTOR3d B, VECTOR3d C) {
  VECTOR3d v0, v1, v2;
  double   u, v, dot00, dot01, dot02, dot11, dot12, invDenom;
  // Compute vectors
  gcgSUBVECTOR3(v0, C, A);
  gcgSUBVECTOR3(v1, B, A);
  gcgSUBVECTOR3(v2, p, A);

  // Compute dot products
  dot00 = gcgDOTVECTOR3(v0, v0);
  dot01 = gcgDOTVECTOR3(v0, v1);
  dot02 = gcgDOTVECTOR3(v0, v2);
  dot11 = gcgDOTVECTOR3(v1, v1);
  dot12 = gcgDOTVECTOR3(v1, v2);

  // Compute barycentric coordinates
  invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
  u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  // Check if point is in triangle
  return (u > 0.0) && (v > 0.0) && (u + v < 1.0);
}




////////////////////////////////////////////////////////////////////////////
// Basic clip test for Liang-Barsky algorithm for clipping lines against
// AABB boxes.
// This function updates the parameters u1 and u2 for an edge of the box.
////////////////////////////////////////////////////////////////////////////
static bool gcgClipTest(float p, float q, float *u1, float *u2) {
  float r;

  if(p < 0.0) {
    r = q / p;
    if(r > *u2) return false;
    else if(r > *u1) *u1 = r;
  } else
    if(p > 0.0) {
      r = q /p;
      if(r < *u1) return false;
      else if(r < *u2) *u2 = r;
    } else if(q < 0.0) return false; // p = 0, so line is parallel to this clipping edge
  return true;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
static bool gcgClipTest(double p, double q, double *u1, double *u2) {
  double r;

  if(p < 0.0) {
    r = q / p;
    if(r > *u2) return false;
    else if(r > *u1) *u1 = r;
  } else
    if(p > 0.0) {
      r = q /p;
      if(r < *u1) return false;
      else if(r < *u2) *u2 = r;
    } else if(q < 0.0) return false; // p = 0, so line is parallel to this clipping edge
  return true;
}


////////////////////////////////////////////////////////////////////////////
// Liang-Barsky algorithm for 2D line clipping.
// Returns true if the line intersects the bounding box.
// Points p1 and p2 keep the clipped segment.
////////////////////////////////////////////////////////////////////////////
bool gcgIntersectLineBox2D(VECTOR2 p1, VECTOR2 p2, VECTOR2 box2dMin, VECTOR2 box2dMax) {
  float u1 = 0.0, u2 = 1.0, dx = p2[0] - p1[0], dy;

  // Tests each of the 4 AABB box edges
  if(gcgClipTest(-dx, p1[0] - box2dMin[0], &u1, &u2))
    if(gcgClipTest(dx, box2dMax[0] - p1[0], &u1, &u2)) {
      dy = p2[1] - p1[1];
      if(gcgClipTest(-dy, p1[1] - box2dMin[1], &u1, &u2))
        if(gcgClipTest(dy, box2dMax[1] - p1[1], &u1, &u2)) {
          if(u2 < 1.0) {
            p2[0] = p1[0] + u2 * dx;
            p2[1] = p1[1] + u2 * dy;
          }
          if(u2 > 0.0) {
            p1[0] += u1 * dx;
            p1[1] += u1 * dy;
          }
          return true;
        }
    }
  return false;
}


////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
bool gcgIntersectLineBox2D(VECTOR2d p1, VECTOR2d p2, VECTOR2d box2dMin, VECTOR2d box2dMax) {
  double u1 = 0.0, u2 = 1.0, dx = p2[0] - p1[0], dy;

  // Tests each of the 4 AABB box edges
  if(gcgClipTest(-dx, p1[0] - box2dMin[0], &u1, &u2))
    if(gcgClipTest(dx, box2dMax[0] - p1[0], &u1, &u2)) {
      dy = p2[1] - p1[1];
      if(gcgClipTest(-dy, p1[1] - box2dMin[1], &u1, &u2))
        if(gcgClipTest(dy, box2dMax[1] - p1[1], &u1, &u2)) {
          if(u2 < 1.0) {
            p2[0] = p1[0] + u2 * dx;
            p2[1] = p1[1] + u2 * dy;
          }
          if(u2 > 0.0) {
            p1[0] += u1 * dx;
            p1[1] += u1 * dy;
          }
          return true;
        }
    }
  return false;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//////////////////     FUNCTIONS FOR TRIANGLES  /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Computes the area of a triangle with edges (v0,v1), (v1, v2), (v2, v0). See Graphics Gems II - Pag. 170.
float gcgTriangleArea(VECTOR3 v0, VECTOR3 v1, VECTOR3 v2) {
  VECTOR3 edge0, edge1, edge2;

  // Compute edges
  gcgSUBVECTOR3(edge0, v1, v0);
  gcgSUBVECTOR3(edge1, v2, v1);
  gcgSUBVECTOR3(edge2, v0, v2);

  // Compute normal
  VECTOR3 normal;
  gcgCROSSVECTOR3(normal, edge0, edge1);
  gcgNORMALIZEVECTOR3(normal, normal);

  // Compute area
  VECTOR3 aux_vec_area;
  gcgCROSSVECTOR3(aux_vec_area, edge1, edge2);

  return 0.5f * fabs(gcgDOTVECTOR3(normal, aux_vec_area));
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////

/// @fn double gcgTriangleArea(VECTOR3d v0, VECTOR3d v1, VECTOR3d v2)
/// @brief Computes the area of a triangle with edges (v0,v1), (v1, v2), (v2, v0) (double version). See Graphics Gems II - Pag. 170.
/// @param[in] v0 First vertex of the triangle.
/// @param[in] v1 Second vertex of the triangle.
/// @param[in] v2 Third vertex of the triangle.
/// @return the area of the triangle.
double gcgTriangleArea(VECTOR3d v0, VECTOR3d v1, VECTOR3d v2) {
  VECTOR3d edge0, edge1, edge2;

  // Compute edges
  gcgSUBVECTOR3(edge0, v1, v0);
  gcgSUBVECTOR3(edge1, v2, v1);
  gcgSUBVECTOR3(edge2, v0, v2);

  // Compute normal
  VECTOR3d normal;
  gcgCROSSVECTOR3(normal, edge0, edge1);
  gcgNORMALIZEVECTOR3(normal, normal);

  // Compute area
  VECTOR3d aux_vec_area;
  gcgCROSSVECTOR3(aux_vec_area, edge1, edge2);

  return 0.5 * fabs(gcgDOTVECTOR3(normal, aux_vec_area));
}


// Computes the gradient of a scalar u over a 3D triangle with edges (v0,v1), (v1, v2), (v2, v0).
void gcgTriangleGradient(VECTOR3 grad_u, VECTOR3 v0, VECTOR3 v1, VECTOR3 v2, float u0, float u1, float u2) {
  VECTOR3 edge0, edge1, edge2;

  // Compute edges
  gcgSUBVECTOR3(edge0, v1, v0);
  gcgSUBVECTOR3(edge1, v2, v1);
  gcgSUBVECTOR3(edge2, v0, v2);

  // Compute normal
  VECTOR3 normal;
  gcgCROSSVECTOR3(normal, edge0, edge1);
  gcgNORMALIZEVECTOR3(normal, normal);

  // Compute local basis
  VECTOR3 ut = {edge0[0], edge0[1], edge0[2]};
  gcgNORMALIZEVECTOR3(ut, ut);
  VECTOR4 q;
  gcgAXISTOQUATERNION(q, (float) (M_PI * 0.5), normal);
  VECTOR3 vt;
  gcgAPPLYQUATERNIONVECTOR3(vt, q, ut);

  // Compute derivatives
  float e0_ut = gcgDOTVECTOR3(edge0, ut), e1_ut = gcgDOTVECTOR3(edge1, ut), e2_ut = gcgDOTVECTOR3(edge2, ut);
  float e0_vt = gcgDOTVECTOR3(edge0, vt), e1_vt = gcgDOTVECTOR3(edge1, vt), e2_vt = gcgDOTVECTOR3(edge2, vt);

  float duds = e0_vt * u2 + e1_vt * u0 + e2_vt * u1;
  float dudt = -(e0_ut * u2 + e1_ut * u0 + e2_ut * u1);

  // Compute triangle area
  VECTOR3 aux_vec_area;
  gcgCROSSVECTOR3(aux_vec_area, edge1, edge2);
  float area2 = fabs(gcgDOTVECTOR3(normal, aux_vec_area));

  // Compute gradient
  grad_u[0] = duds * ut[0] + dudt * vt[0];
  grad_u[1] = duds * ut[1] + dudt * vt[1];
  grad_u[2] = duds * ut[2] + dudt * vt[2];

  float inv_area = 1.0f / area2;
  gcgSCALEVECTOR3(grad_u, grad_u, inv_area);
}


////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////

/// @fn void gcgTriangleGradient(VECTOR3d grad_u, VECTOR3d v0, VECTOR3d v1, VECTOR3d v2, double u0, double u1, double u2)
/// @brief Computes the gradient of a scalar u over a 3D triangle with edges (v0,v1), (v1, v2), (v2, v0) (double version).
/// @param[out] grad_u the 3D vector that will contain the gradient of u. The resulting gradient vector is necessarily over the triangle plane.
/// @param[in] v0 First vertex of the triangle.
/// @param[in] v1 Second vertex of the triangle.
/// @param[in] v2 Third vertex of the triangle.
/// @param[in] u0 The scalar value of v0 vertex.
/// @param[in] u1 The scalar value of v1 vertex.
/// @param[in] u2 The scalar value of v2 vertex.
void gcgTriangleGradient(VECTOR3d grad_u, VECTOR3d v0, VECTOR3d v1, VECTOR3d v2, double u0, double u1, double u2) {
  VECTOR3d edge0, edge1, edge2;

  // Compute edges
  gcgSUBVECTOR3(edge0, v1, v0);
  gcgSUBVECTOR3(edge1, v2, v1);
  gcgSUBVECTOR3(edge2, v0, v2);

  // Compute normal
  VECTOR3d normal;
  gcgCROSSVECTOR3(normal, edge0, edge1);
  gcgNORMALIZEVECTOR3(normal, normal);

  // Compute local basis
  VECTOR3d ut = {edge0[0], edge0[1], edge0[2]};
  gcgNORMALIZEVECTOR3(ut, ut);
  VECTOR4d q;
  gcgAXISTOQUATERNION(q, M_PI * 0.5, normal);
  VECTOR3d vt;
  gcgAPPLYQUATERNIONVECTOR3(vt, q, ut);

  // Compute derivatives
  double e0_ut = gcgDOTVECTOR3(edge0, ut), e1_ut = gcgDOTVECTOR3(edge1, ut), e2_ut = gcgDOTVECTOR3(edge2, ut);
  double e0_vt = gcgDOTVECTOR3(edge0, vt), e1_vt = gcgDOTVECTOR3(edge1, vt), e2_vt = gcgDOTVECTOR3(edge2, vt);

  double duds = e0_vt * u2 + e1_vt * u0 + e2_vt * u1;
  double dudt = -(e0_ut * u2 + e1_ut * u0 + e2_ut * u1);

  // Compute triangle area
  VECTOR3d aux_vec_area;
  gcgCROSSVECTOR3(aux_vec_area, edge1, edge2);
  double area2 = fabs(gcgDOTVECTOR3(normal, aux_vec_area));

  // Compute gradient
  grad_u[0] = duds * ut[0] + dudt * vt[0];
  grad_u[1] = duds * ut[1] + dudt * vt[1];
  grad_u[2] = duds * ut[2] + dudt * vt[2];

  double inv_area = 1.0 / area2;
  gcgSCALEVECTOR3(grad_u, grad_u, inv_area);
}


// Computes the distance between two points on the unit sphere.
// Haversine Formula (from R.W. Sinnott, "Virtues of the Haversine", Sky and Telescope, vol. 68, no. 2, 1984, p. 159):
// http://www.movable-type.co.uk/scripts/gis-faq-5.1.html
float gcgSphericalDistance(float latitude1, float longitude1, float latitude2, float longitude2) {
  float sinlat = sinf((latitude2 - latitude1) * 0.5f);
  float sinlon = sinf((longitude2 - longitude1) * 0.5f);
  float a = sinlat * sinlat + cosf(latitude1) * cosf(latitude2) * sinlon * sinlon;
  float c = asinf(MIN(1.0f, sqrtf(a)));
  return c + c;
}

double gcgSphericalDistance(double  latitude1, double longitude1, double latitude2, double longitude2) {
  double sinlat = sin((latitude2 - latitude1) * 0.5);
  double sinlon = sin((longitude2 - longitude1) * 0.5);
  double a = sinlat * sinlat + cos(latitude1) * cos(latitude2) * sinlon * sinlon;
  double c = asin(MIN(1.0, sqrt(a)));
  return c + c;
}

