/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   ALGEBRA.C: functions for linear algebra.

   Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     MATRIX INVERSION  ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
// Computes the inverse of a 2x2 matrix.
//
//| a  b |-1                        | d  -b |
//| c  d |    =  1/(a*d - b*c)  *   |-c   a |
//
// Returns TRUE se the matrix is invertible. Otherwise, the resulting matrix is the
// adjoint.
/////////////////////////////////////////////////////////////////////////////////////
bool gcgInverseMatrix2(MATRIX2 inverse, MATRIX2 matrix) {
	// Compute determinant
	float invdet = matrix[0] * matrix[3] - matrix[1] * matrix[2];

	bool hasinverse = fabsf(invdet) > EPSILON; // Is diferent than 0?

	// Invert determinant if it's non zero
	if(hasinverse) invdet = (float) (1.0 / invdet);
	else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_ALGEBRA, GCG_BADPARAMETERS), "gcgInverseMatrix2(): singular input matrix. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    invdet = 1.0f;
	}

	// Computes the adjoint matrix
	inverse[0] =   matrix[3] * invdet;
	inverse[1] = - matrix[1] * invdet;
	inverse[2] = - matrix[2] * invdet;
	inverse[3] =   matrix[0] * invdet;

	return hasinverse;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
bool gcgInverseMatrix2(MATRIX2d inverse, MATRIX2d matrix) {
	// Compute determinant
	double invdet = matrix[0] * matrix[3] - matrix[1] * matrix[2];

	bool hasinverse = fabs(invdet) > EPSILON; // Is diferent than 0?

	// Invert determinant if it's non zero
  if(hasinverse) invdet = (double) (1.0 / invdet);
  else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_ALGEBRA, GCG_BADPARAMETERS), "gcgInverseMatrix2(): singular input matrix. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    invdet = 1.0;
	}

	// Computes the adjoint matrix
	inverse[0] =   matrix[3] * invdet;
	inverse[1] = - matrix[1] * invdet;
	inverse[2] = - matrix[2] * invdet;
	inverse[3] =   matrix[0] * invdet;

	return hasinverse;
}


/////////////////////////////////////////////////////////////////////////////////////
// Computes the inverse of a 3x3 matrix.
//
//| a11 a12 a13 |-1             |   a33a22-a32a23  -(a33a12-a32a13)   a23a12-a22a13  |
//| a21 a22 a23 |    =  1/DET * | -(a33a21-a31a23)   a33a11-a31a13  -(a23a11-a21a13) |
//| a31 a32 a33 |               |   a32a21-a31a22  -(a32a11-a31a12)   a22a11-a21a12  |
//
// com DET  =  a11(a33a22-a32a23)-a21(a33a12-a32a13)+a31(a23a12-a22a13)
//
// ou
//
//| a0 a1 a2 |-1             |   a8a4-a7a5  -(a8a1-a7a2)   a5a1-a4a2  |
//| a3 a4 a5 |    =  1/DET * | -(a8a3-a6a5)   a8a0-a6a2  -(a5a0-a3a2) |
//| a6 a7 a8 |               |   a7a3-a6a4  -(a7a0-a6a1)   a4a0-a3a1  |
// com DET  =  a0(a8a4-a7a5)-a3(a8a1-a7a2)+a6(a5a1-a4a2)
//
// Returns TRUE se the matrix is invertible. Otherwise, the resulting matrix is the
// adjoint.
/////////////////////////////////////////////////////////////////////////////////////
bool gcgInverseMatrix3(MATRIX3 inverse, MATRIX3 matrix) {
	// Compute determinant
	float invdet = gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[2], matrix[3],
                                    matrix[4], matrix[5], matrix[6], matrix[7], matrix[8]);
	bool hasinverse = fabsf(invdet) > EPSILON; // Is diferent than 0?

	// Invert determinant if it's non zero
  if(hasinverse) invdet = (float) (1.0 / invdet);
  else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_ALGEBRA, GCG_BADPARAMETERS), "gcgInverseMatrix3(): singular input matrix. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    invdet = 1.0f;
	}


	// Computes the adjoint matrix
	inverse[0] =  (matrix[8]*matrix[4] - matrix[7]*matrix[5]) * invdet;
	inverse[1] = -(matrix[8]*matrix[1] - matrix[7]*matrix[2]) * invdet;
	inverse[2] =  (matrix[5]*matrix[1] - matrix[4]*matrix[2]) * invdet;
	inverse[3] = -(matrix[8]*matrix[3] - matrix[6]*matrix[5]) * invdet;
	inverse[4] =  (matrix[8]*matrix[0] - matrix[6]*matrix[2]) * invdet;
	inverse[5] = -(matrix[5]*matrix[0] - matrix[3]*matrix[2]) * invdet;
	inverse[6] =  (matrix[7]*matrix[3] - matrix[6]*matrix[4]) * invdet;
	inverse[7] = -(matrix[7]*matrix[0] - matrix[6]*matrix[1]) * invdet;
	inverse[8] =  (matrix[4]*matrix[0] - matrix[3]*matrix[1]) * invdet;

	return hasinverse;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
bool gcgInverseMatrix3(MATRIX3d inverse, MATRIX3d matrix) {
	// Compute determinant
	double invdet = gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[2], matrix[3],
										     matrix[4], matrix[5], matrix[6], matrix[7], matrix[8]);
	bool hasinverse = fabs(invdet) > EPSILON; // Is diferent than 0?

	// Invert determinant if it's non zero
  if(hasinverse) invdet = (double) (1.0 / invdet);
  else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_ALGEBRA, GCG_BADPARAMETERS), "gcgInverseMatrix3(): singular input matrix. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    invdet = 1.0;
	}

	// Computes the adjoint matrix
	inverse[0] =  (matrix[8]*matrix[4] - matrix[7]*matrix[5]) * invdet;
	inverse[1] = -(matrix[8]*matrix[1] - matrix[7]*matrix[2]) * invdet;
	inverse[2] =  (matrix[5]*matrix[1] - matrix[4]*matrix[2]) * invdet;
	inverse[3] = -(matrix[8]*matrix[3] - matrix[6]*matrix[5]) * invdet;
	inverse[4] =  (matrix[8]*matrix[0] - matrix[6]*matrix[2]) * invdet;
	inverse[5] = -(matrix[5]*matrix[0] - matrix[3]*matrix[2]) * invdet;
	inverse[6] =  (matrix[7]*matrix[3] - matrix[6]*matrix[4]) * invdet;
	inverse[7] = -(matrix[7]*matrix[0] - matrix[6]*matrix[1]) * invdet;
	inverse[8] =  (matrix[4]*matrix[0] - matrix[3]*matrix[1]) * invdet;

	return hasinverse;
}


/////////////////////////////////////////////////////////////////////////////////////
// Calcula a inversa de uma matriz 4x4. Senao nao houver a inversa, retorna a adjunta
/////////////////////////////////////////////////////////////////////////////////////
bool gcgInverseMatrix4(MATRIX4 inverse, MATRIX4 matrix) {
	// Compute determinant
	float det0, det4, det8, det12; // Para otimizacao
	float invdet = gcgDETERMINANT4x4_(matrix, det0, det4, det8, det12);

	bool hasinverse = fabsf(invdet) > EPSILON; // Testa se é diferente de 0

	// Invert determinant if it's non zero
  if(hasinverse) invdet = (float) (1.0 / invdet);
  else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_ALGEBRA, GCG_BADPARAMETERS), "gcgInverseMatrix4(): singular input matrix. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    invdet = 1.0f;
	}


	// Computes the adjoint matrix and multiplies by the determinant inverted.
	inverse[0] =  det0  * invdet; // determinants already computed
	inverse[1] = -det4  * invdet;
	inverse[2] =  det8  * invdet;
	inverse[3] = -det12 * invdet;

	inverse[4] = -gcgDETERMINANT3x3_(matrix[4], matrix[6], matrix[7], matrix[8], matrix[10], matrix[11], matrix[12], matrix[14], matrix[15]) * invdet;
	inverse[5] =  gcgDETERMINANT3x3_(matrix[0], matrix[2], matrix[3], matrix[8], matrix[10], matrix[11], matrix[12], matrix[14], matrix[15]) * invdet;
	inverse[6] = -gcgDETERMINANT3x3_(matrix[0], matrix[2], matrix[3], matrix[4], matrix[6],  matrix[7],  matrix[12], matrix[14], matrix[15]) * invdet;
	inverse[7] =  gcgDETERMINANT3x3_(matrix[0], matrix[2], matrix[3], matrix[4], matrix[6],  matrix[7],  matrix[8],  matrix[10], matrix[11]) * invdet;

	inverse[8]  =  gcgDETERMINANT3x3_(matrix[4], matrix[5], matrix[7], matrix[8], matrix[9], matrix[11], matrix[12], matrix[13], matrix[15]) * invdet;
	inverse[9]  = -gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[3], matrix[8], matrix[9], matrix[11], matrix[12], matrix[13], matrix[15]) * invdet;
	inverse[10] =  gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[3], matrix[4], matrix[5],  matrix[7], matrix[12], matrix[13], matrix[15]) * invdet;
	inverse[11] = -gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[3], matrix[4], matrix[5],  matrix[7], matrix[8],  matrix[9],  matrix[11]) * invdet;

	inverse[12] = -gcgDETERMINANT3x3_(matrix[4], matrix[5], matrix[6], matrix[8], matrix[9], matrix[10], matrix[12], matrix[13], matrix[14]) * invdet;
	inverse[13] =  gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[2], matrix[8], matrix[9], matrix[10], matrix[12], matrix[13], matrix[14]) * invdet;
	inverse[14] = -gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[2], matrix[4], matrix[5],  matrix[6], matrix[12], matrix[13], matrix[14]) * invdet;
	inverse[15] =  gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[2], matrix[4], matrix[5],  matrix[6], matrix[8],  matrix[9],  matrix[10]) * invdet;

	return hasinverse;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
bool gcgInverseMatrix4(MATRIX4d inverse, MATRIX4d matrix) {
	// Compute determinant
	double det0, det4, det8, det12; // Para otimizacao
	double invdet = gcgDETERMINANT4x4_(matrix, det0, det4, det8, det12);

	bool hasinverse = fabs(invdet) > EPSILON; // Testa se é diferente de 0

	// Invert determinant if it's non zero
  if(hasinverse) invdet = (double) (1.0 / invdet);
  else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_ALGEBRA, GCG_BADPARAMETERS), "gcgInverseMatrix4(): singular input matrix. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    invdet = 1.0;
	}


	// Computes the adjoint matrix and multiplies by the determinant inverted.
	inverse[0] =  det0  * invdet; // determinants already computed
	inverse[1] = -det4  * invdet;
	inverse[2] =  det8  * invdet;
	inverse[3] = -det12 * invdet;

	inverse[4] = -gcgDETERMINANT3x3_(matrix[4], matrix[6], matrix[7], matrix[8], matrix[10], matrix[11], matrix[12], matrix[14], matrix[15]) * invdet;
	inverse[5] =  gcgDETERMINANT3x3_(matrix[0], matrix[2], matrix[3], matrix[8], matrix[10], matrix[11], matrix[12], matrix[14], matrix[15]) * invdet;
	inverse[6] = -gcgDETERMINANT3x3_(matrix[0], matrix[2], matrix[3], matrix[4], matrix[6],  matrix[7],  matrix[12], matrix[14], matrix[15]) * invdet;
	inverse[7] =  gcgDETERMINANT3x3_(matrix[0], matrix[2], matrix[3], matrix[4], matrix[6],  matrix[7],  matrix[8],  matrix[10], matrix[11]) * invdet;

	inverse[8]  =  gcgDETERMINANT3x3_(matrix[4], matrix[5], matrix[7], matrix[8], matrix[9], matrix[11], matrix[12], matrix[13], matrix[15]) * invdet;
	inverse[9]  = -gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[3], matrix[8], matrix[9], matrix[11], matrix[12], matrix[13], matrix[15]) * invdet;
	inverse[10] =  gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[3], matrix[4], matrix[5],  matrix[7], matrix[12], matrix[13], matrix[15]) * invdet;
	inverse[11] = -gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[3], matrix[4], matrix[5],  matrix[7], matrix[8],  matrix[9],  matrix[11]) * invdet;

	inverse[12] = -gcgDETERMINANT3x3_(matrix[4], matrix[5], matrix[6], matrix[8], matrix[9], matrix[10], matrix[12], matrix[13], matrix[14]) * invdet;
	inverse[13] =  gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[2], matrix[8], matrix[9], matrix[10], matrix[12], matrix[13], matrix[14]) * invdet;
	inverse[14] = -gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[2], matrix[4], matrix[5],  matrix[6], matrix[12], matrix[13], matrix[14]) * invdet;
	inverse[15] =  gcgDETERMINANT3x3_(matrix[0], matrix[1], matrix[2], matrix[4], matrix[5],  matrix[6], matrix[8],  matrix[9],  matrix[10]) * invdet;

	return hasinverse;
}

/////////////////////////////////////////////////////////////////////////////
///////////////  SIMPLE GENERAL SQUARE MATRIX INVERSION   ///////////////////
/////////////////////////////////////////////////////////////////////////////

/*// Subtracts two L x N submatrices
const inline static void gcgSubSubmatrices(FLOAT *dest, unsigned int L, unsigned int N, FLOAT *source1, unsigned int row1, unsigned int col1, unsigned int rowsize1, float *source2, unsigned int row2, unsigned int col2, unsigned int rowsize2) {
  for(register unsigned int irow = 0; irow < L; irow++)
    for(register unsigned int icol = 0; icol < N; icol++) {
      dest[irow * N + icol] = source1[irow * N + icol] - source2[irow * N + icol];
    }
}


// Multiply a L x N matrix with a N x M matrix, generating a L x M matrix
const inline static void gcgMultSubmatrices(FLOAT *dest, unsigned int L, unsigned int N, unsigned int M, FLOAT *source1, unsigned int row1, unsigned int col1, unsigned int rowsize1, float *source2, unsigned int row2, unsigned int col2, unsigned int rowsize2) {
  for(register unsigned int irow = 0; irow < L; irow++)
    for(register unsigned int icol = 0; icol < M; icol++) {
      FLOAT sum = 0;
      for(register unsigned int j = 0; j < N; j++) sum += source1[irow * N + j] * source2[j * N + icol];
      dest[irow * M + icol] = sum;
    }
}

// Extracts a submatrix of size rowx x cols from source matrix with rowsize
// columns, starting at position (firstrow, firstcol) element.
const inline static void gcgExtractSubmatrix(unsigned int rows, unsigned int cols, FLOAT *dest, unsigned int firstrow, unsigned int firstcol, unsigned int rowsize, FLOAT *source) {
  const register unsigned int address = firstrow * rowsize + firstcol;

  for(register int nr = 0; nr < rows; nr++)
    memcpy(&dest[nr * cols], &source[address + nr * rowsize], sizeof(FLOAT) * cols);
}

// Inverts a square matrix using the blockwise inversion formula. Works fine
// if the matrix is diagonal and its Schur Complement D-CA'B is a small matrix.
// Uses the 4x4 inversion routine gcgInverseMatrix4() to invert the n x n
// matrix recursively.
int gcgInverseMatrix(unsigned int n, FLOAT *inverse, FLOAT *matrix) {
  // Is a small enough matrix?
  if(n == 4) return gcgInverseMatrix4(inverse, matrix); // Common case
  else if(n == 3) return gcgInverseMatrix3(inverse, matrix);
       else if(n == 2) return gcgInverseMatrix2(inverse, matrix);

  // N is greater than 4. We'll compute recursively.
  MATRIX4 A, Ainv;

  // Copies the elements of A and computes its inverse.
  gcgExtractSubmatrix(4, 4, A, 0, 0, n, matrix);
  gcgInverseMatrix4(Ainv, A);

  // Computes C*A'   n-4 x 4
  FLOAT *CAinv = (FLOAT*) ALLOC(sizeof(FLOAT) * 4 * (n-4));
  if(CAinv == NULL) return FALSE;
  gcgMultSubmatrices(CAinv, n-4, 4, 4, matrix, 4, 0, n, A, 0, 0, 4);

  // Computes C*A'*B    n-4 x  n-4
  FLOAT *CAinvB = (FLOAT*) ALLOC(sizeof(FLOAT) * (n-4) * (n-4));
  if(CAinvB == NULL) return FALSE;
  gcgMultSubmatrices(CAinvB, n-4, 4, n-4, CAinv, 0, 0, 4, matrix, 0, 4, n);

  // Computes Schur Complement D-C*A'*B    n-4 x  n-4
  FLOAT *D_CAinvB = (FLOAT*) ALLOC(sizeof(FLOAT) * (n-4) * (n-4));
  if(D_CAinvB == NULL) return FALSE;
  gcgSubSubmatrices(D_CAinvB, n-4, n-4, matrix, 4, 4, n, CAinvB, 0, 0, n-4);

  // Computes inverted Schur Complement D-C*A'*B    n-4 x  n-4
  FLOAT *D_CAinvB_inv = (FLOAT*) ALLOC(sizeof(FLOAT) * (n-4) * (n-4));
  if(D_CAinvB_inv == NULL) return FALSE;
  gcgInverseMatrix(n-4, D_CAinvB_inv, D_CAinvB); // Recursive calls down to 4x4 inversion



  // Free temporary matrix
  SAFE_FREE(CAinv);
  SAFE_FREE(CAinvB);
  SAFE_FREE(D_CAinvB);
  SAFE_FREE(D_CAinvB_inv);
}
*/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////  EIGEN DECOMPOSITION OF SYMMETRIC MATRICES  /////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Eigen decomposition of symmetric 3x3 matrices M. The resulted eigenvectors
// are stored int a matrix v[3][3] and the respective eigenvalues are stored
// in the vector d[3]. They are ordered by decreasing eigenvalues.
// Adapted from the public domain Java Matrix library JAMA.
///////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
bool gcgEigenSymmetricMatrix3(MATRIX3 A, MATRIX3 V, VECTOR3 d)  {
    VECTOR3 e;

    gcgCOPYMATRIX3(V, A);

    // Symmetric Householder reduction to tridiagonal form.
    //     static void tred2(double V[3][3], double d[3], double e[3]);
    //  This is derived from the Algol procedures tred2 by
    //  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
    //  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
    //  Fortran subroutine in EISPACK.
    gcgSETVECTOR3(d, V[2], V[5], V[8]);

    // Scale to avoid under/overflow.
    float scale = fabsf(d[0]) + fabsf(d[1]);

    if(scale == 0.0) {
      e[2] = d[1];
      d[0] = V[1];
      d[1] = V[4];
      d[2] = V[2] = V[6] = V[5] = V[7] = 0.0;
    } else {
      // Generate Householder vector.
      float h;

      d[0] /= scale; d[1] /= scale;
      h  = d[0] * d[0] + d[1] * d[1];

      float f = d[1];
      float g = sqrtf(h);

      if(f > 0) g = -g;

      e[2] = scale * g;
      h = h - f * g;
      d[1] = f - g;

      // Apply similarity transformation to remaining columns.
      V[6] = (f = d[0]);
      e[0] = V[0] * f + V[1] * d[1];
      e[1] = V[1] * f;

      V[7] = (f = d[1]);
      e[1] += V[4] * f;

      f  = (e[0] /= h) * d[0] + (e[1] /= h) * d[1];

      float hh = f / (h + h);
      e[0] -= hh * d[0];
      e[1] -= hh * d[1];

      f = d[0];
      g = e[0];
      V[0] -= (f * e[0] + g * d[0]);
      d[0] = (V[1] -= (f * e[1] + g * d[1]));
      V[2] = 0.0;

      d[1] = (V[4] -= (d[1] * e[1] + e[1] * d[1]));
      V[5] = 0.0;
      d[2] = h;
    }

    // Scale to avoid under/overflow.
    scale = fabsf(d[0]);

    if(scale == 0.0) {
      e[1] = d[0];
      d[0] = V[0];
      d[1] = V[1] = V[3] = 0.0;
    } else {
      // Generate Householder vector.
      float f = (d[0] /= scale);
      float h = f * f;
      float g = sqrtf(h);

      if(f > 0) g = -g;

      e[1] = scale * g;
      h = h - f * g;
      d[0] = f - g;

      // Apply similarity transformation to remaining columns.
      e[0] = (V[0] * (V[3] = d[0])) / h;

      float hh = (e[0] * d[0]) / (h + h);
      e[0] -= hh * d[0];
      V[0] -= (d[0] * e[0] + e[0] * d[0]);
      d[0] = V[0];
      V[1] = 0.0;
      d[1] = h;
    }

    // Accumulate transformations.
    V[2] = V[0];
    float g, h = d[1];
    if(h != 0.0) {
      d[0] = V[3] / h;
      V[0] = (float) (1.0 - V[3] * d[0]);
    } else V[0] = 1.0;
    V[3] = 0.0;

    V[5] = V[4];
    h = d[2];
    if(h != 0.0) {
      d[0] = V[6] / h;
      d[1] = V[7] / h;

      g = V[6] * V[0] + V[7] * V[1];
      V[0] -= g * d[0];
      V[1] -= g * d[1];

      g = V[6] * V[3] + V[7];
      V[3] -= g * d[0];
      V[4] = (float) (1.0 - g * d[1]);
    } else V[4] = 1.0;

    d[0] = V[2];
    d[1] = V[5];
    d[2] = V[8];
    e[0] = V[6] = V[7] = V[2] = V[5] = 0.0;
    V[8] = 1.0;

    // - Symmetric tridiagonal QL algorithm.
    //    static void tql2(float V[3][3], double d[3], double e[3]) {
    //  This is derived from the Algol procedures tql2, by
    //  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
    //  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
    //  Fortran subroutine in EISPACK.
    int i, k, l;

    e[0] = e[1];
    e[1] = e[2];
    e[2] = 0.0;

    float f = 0.0;
    float tst1 = 0.0;

    for (l = 0; l < 3; l++) {
      // Find small subdiagonal element
      tst1 = MAX(tst1, fabsf(d[l]) + fabsf(e[l]));
      int m = l;
      if(fabsf(e[m]) > EPSILON * tst1) {
        m++;
        if(m < 3)
          if(fabsf(e[m]) > EPSILON * tst1) m++;
      }

      // If m == l, d[l] is an eigenvalue,
      // otherwise, iterate.
      if(m > l) {
        do {
          // Compute implicit shift
          float g = d[l];
          float p = (float) ((d[l+1] - g) / (2.0 * e[l]));
          float r = (float) sqrtf(p*p + 1.0f);
          if (p < 0) r = -r;

          d[l] = e[l] / (p + r);
          d[l+1] = e[l] * (p + r);
          float dl1 = d[l+1];
          float h = g - d[l];
          for(i = l + 2; i < 3; i++) d[i] -= h;
          f = f + h;

          // Implicit QL transformation.
          p = d[m];
          float c = 1.0;
          float c2 = c;
          float c3 = c;
          float el1 = e[l+1];
          float s = 0.0;
          float s2 = 0.0;
          for (i = m-1; i >= l; i--) {
            c3 = c2;
            c2 = c;
            s2 = s;
            g = c * e[i];
            h = c * p;
            r = sqrt(p * p + e[i] * e[i]);
            e[i+1] = s * r;
            s = e[i] / r;
            c = p / r;
            p = c * d[i] - s * g;
            d[i+1] = h + s * (c * g + s * d[i]);

            // Accumulate transformation.
            h = V[3*(i+1)];
            V[3*(i+1)] = s * V[3*i] + c * h;
            V[3*i] = c * V[3*i] - s * h;
            h = V[1 + 3*(i+1)];
            V[1 + 3*(i+1)] = s * V[1 + 3*i] + c * h;
            V[1 + 3*i] = c * V[1 + 3*i] - s * h;
            h = V[2 + 3*(i+1)];
            V[2 + 3*(i+1)] = s * V[2 + 3*i] + c * h;
            V[2 + 3*i] = c * V[2 + 3*i] - s * h;
          }
          p = -s * s2 * c3 * el1 * e[l] / dl1;
          e[l] = s * p;
          d[l] = c * p;

          // Check for convergence.

        } while(fabsf(e[l]) > EPSILON * tst1);
      }
      d[l] += f;
      e[l]  = 0.0;
    }

    // Sort eigenvalues and corresponding vectors.
    float p = d[0];
    k = 0;
    if(d[1] < p) { k = 1; p = d[1]; }
    if(d[2] < p) { k = 2; p = d[2]; }
    if(k != 0) {
      d[k] = d[0];
      d[0] = p;

      VECTOR3 tmp;
      gcgCOPYVECTOR3(tmp, V);
      gcgCOPYVECTOR3(V, &V[k*3]);
      gcgCOPYVECTOR3(&V[k*3], tmp);
    }

    if(d[1] > d[2]) {
      p = d[1];
      d[1] = d[2];
      d[2] = p;

      VECTOR3 tmp;
      gcgCOPYVECTOR3(tmp, &V[3]);
      gcgCOPYVECTOR3(&V[3], &V[6]);
      gcgCOPYVECTOR3(&V[6], tmp);
    }

    return true;
}


////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
bool gcgEigenSymmetricMatrix3(MATRIX3d A, MATRIX3d V, VECTOR3d d)  {
    VECTOR3d e;

    gcgCOPYMATRIX3(V, A);

    // Symmetric Householder reduction to tridiagonal form.
    //     static void tred2(double V[3][3], double d[3], double e[3]);
    //  This is derived from the Algol procedures tred2 by
    //  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
    //  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
    //  Fortran subroutine in EISPACK.
    gcgSETVECTOR3(d, V[2], V[5], V[8]);

    // Scale to avoid under/overflow.
    double scale = fabs(d[0]) + fabs(d[1]);

    if(scale == 0.0) {
      e[2] = d[1];
      d[0] = V[1];
      d[1] = V[4];
      d[2] = V[2] = V[6] = V[5] = V[7] = 0.0;
    } else {
      // Generate Householder vector.
      double h;

      d[0] /= scale; d[1] /= scale;
      h  = d[0] * d[0] + d[1] * d[1];

      double f = d[1];
      double g = sqrt(h);

      if(f > 0) g = -g;

      e[2] = scale * g;
      h = h - f * g;
      d[1] = f - g;

      // Apply similarity transformation to remaining columns.
      V[6] = (f = d[0]);
      e[0] = V[0] * f + V[1] * d[1];
      e[1] = V[1] * f;

      V[7] = (f = d[1]);
      e[1] += V[4] * f;

      f  = (e[0] /= h) * d[0] + (e[1] /= h) * d[1];

      double hh = f / (h + h);
      e[0] -= hh * d[0];
      e[1] -= hh * d[1];

      f = d[0];
      g = e[0];
      V[0] -= (f * e[0] + g * d[0]);
      d[0] = (V[1] -= (f * e[1] + g * d[1]));
      V[2] = 0.0;

      d[1] = (V[4] -= (d[1] * e[1] + e[1] * d[1]));
      V[5] = 0.0;
      d[2] = h;
    }

    // Scale to avoid under/overflow.
    scale = fabs(d[0]);

    if(scale == 0.0) {
      e[1] = d[0];
      d[0] = V[0];
      d[1] = V[1] = V[3] = 0.0;
    } else {
      // Generate Householder vector.
      double f = (d[0] /= scale);
      double h = f * f;
      double g = sqrt(h);

      if(f > 0) g = -g;

      e[1] = scale * g;
      h = h - f * g;
      d[0] = f - g;

      // Apply similarity transformation to remaining columns.
      e[0] = (V[0] * (V[3] = d[0])) / h;

      double hh = (e[0] * d[0]) / (h + h);
      e[0] -= hh * d[0];
      V[0] -= (d[0] * e[0] + e[0] * d[0]);
      d[0] = V[0];
      V[1] = 0.0;
      d[1] = h;
    }

    // Accumulate transformations.
    V[2] = V[0];
    double g, h = d[1];
    if(h != 0.0) {
      d[0] = V[3] / h;
      V[0] = (double) (1.0 - V[3] * d[0]);
    } else V[0] = 1.0;
    V[3] = 0.0;

    V[5] = V[4];
    h = d[2];
    if(h != 0.0) {
      d[0] = V[6] / h;
      d[1] = V[7] / h;

      g = V[6] * V[0] + V[7] * V[1];
      V[0] -= g * d[0];
      V[1] -= g * d[1];

      g = V[6] * V[3] + V[7];
      V[3] -= g * d[0];
      V[4] = (double) (1.0 - g * d[1]);
    } else V[4] = 1.0;

    d[0] = V[2];
    d[1] = V[5];
    d[2] = V[8];
    e[0] = V[6] = V[7] = V[2] = V[5] = 0.0;
    V[8] = 1.0;

    // - Symmetric tridiagonal QL algorithm.
    //    static void tql2(double V[3][3], double d[3], double e[3]) {
    //  This is derived from the Algol procedures tql2, by
    //  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
    //  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
    //  Fortran subroutine in EISPACK.
    int i, k, l;

    e[0] = e[1];
    e[1] = e[2];
    e[2] = 0.0;

    double f = 0.0;
    double tst1 = 0.0;

    for (l = 0; l < 3; l++) {
      // Find small subdiagonal element
      tst1 = MAX(tst1, fabs(d[l]) + fabs(e[l]));
      int m = l;
      //// IT MUST BE fabsf() and NOT fabs(). Numeric errors might result if this test fails for really small e[m] and tst1 values
      if(fabs(e[m]) > EPSILON * tst1) {
        m++;
        if(m < 3)
          //// IT MUST BE fabsf() and NOT fabs(). Numeric errors might result if this test fails for really small e[m] and tst1 values
          if(fabs(e[m]) > EPSILON * tst1) m++;
      }

      // If m == l, d[l] is an eigenvalue,
      // otherwise, iterate.
      if(m > l) {
        do {
          // Compute implicit shift
          double g = d[l];
          double p = (double) ((d[l+1] - g) / (2.0 * e[l]));
          double r = (double) sqrt(p*p + 1.0);
          if (p < 0) r = -r;

          d[l] = e[l] / (p + r);
          d[l+1] = e[l] * (p + r);
          double dl1 = d[l+1];
          double h = g - d[l];
          for(i = l + 2; i < 3; i++) d[i] -= h;
          f = f + h;

          // Implicit QL transformation.
          p = d[m];
          double c = 1.0;
          double c2 = c;
          double c3 = c;
          double el1 = e[l+1];
          double s = 0.0;
          double s2 = 0.0;
          for (i = m-1; i >= l; i--) {
            c3 = c2;
            c2 = c;
            s2 = s;
            g = c * e[i];
            h = c * p;
            r = sqrt(p * p + e[i] * e[i]);
            e[i+1] = s * r;
            s = e[i] / r;
            c = p / r;
            p = c * d[i] - s * g;
            d[i+1] = h + s * (c * g + s * d[i]);

            // Accumulate transformation.
            h = V[3*(i+1)];
            V[3*(i+1)] = s * V[3*i] + c * h;
            V[3*i] = c * V[3*i] - s * h;
            h = V[1 + 3*(i+1)];
            V[1 + 3*(i+1)] = s * V[1 + 3*i] + c * h;
            V[1 + 3*i] = c * V[1 + 3*i] - s * h;
            h = V[2 + 3*(i+1)];
            V[2 + 3*(i+1)] = s * V[2 + 3*i] + c * h;
            V[2 + 3*i] = c * V[2 + 3*i] - s * h;
          }
          p = -s * s2 * c3 * el1 * e[l] / dl1;
          e[l] = s * p;
          d[l] = c * p;

          // Check for convergence.
        } while(fabs(e[l]) > EPSILON * tst1);
      }
      d[l] += f;
      e[l]  = 0.0;
    }

    // Sort eigenvalues and corresponding vectors.
    double p = d[0];
    k = 0;
    if(d[1] < p) { k = 1; p = d[1]; }
    if(d[2] < p) { k = 2; p = d[2]; }
    if(k != 0) {
      d[k] = d[0];
      d[0] = p;

      VECTOR3d tmp;
      gcgCOPYVECTOR3(tmp, V);
      gcgCOPYVECTOR3(V, &V[k*3]);
      gcgCOPYVECTOR3(&V[k*3], tmp);
    }

    if(d[1] > d[2]) {
      p = d[1];
      d[1] = d[2];
      d[2] = p;

      VECTOR3d tmp;
      gcgCOPYVECTOR3(tmp, &V[3]);
      gcgCOPYVECTOR3(&V[3], &V[6]);
      gcgCOPYVECTOR3(&V[6], tmp);
    }

    return true;
}




/////////////////////////////////////////////////////////////////////////////
// Code for eigen decomposition of general symmetric matrices NxN.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Symmetric Householder reduction to tridiagonal form.
inline static void gcgTred(int n, float V[], float d[], float e[]) {
  int i, j, k;

//  This is derived from the Algol procedures tred2 by
//  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
//  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
//  Fortran subroutine in EISPACK.

  for (j = 0; j < n; j++) d[j] = V[n-1 + j * n];

  // Householder reduction to tridiagonal form.
  for (i = n-1; i > 0; i--) {
    // Scale to avoid under/overflow.
    float scale = 0.0;
    float h = 0.0;
    for (k = 0; k < i; k++) scale += fabsf(d[k]);

    if (scale == 0.0) {
      e[i] = d[i-1];
      for (j = 0; j < i; j++) {
        d[j] = V[i-1 + j*n];
        V[i + j*n] = 0.0;
        V[j + i*n] = 0.0;
      }
    } else {
      // Generate Householder vector.

      for (k = 0; k < i; k++) {
        d[k] /= scale;
        h += d[k] * d[k];
      }
      float f = d[i-1];
      float g = sqrtf(h);
      if (f > 0) g = -g;

      e[i] = scale * g;
      h = h - f * g;
      d[i-1] = f - g;
      for (j = 0; j < i; j++) e[j] = 0.0;

      // Apply similarity transformation to remaining columns.

      for (j = 0; j < i; j++) {
        f = d[j];
        V[j + i * n] = f;
        g = e[j] + V[j + j * n] * f;
        for (k = j+1; k <= i-1; k++) {
          g += V[k + j*n] * d[k];
          e[k] += V[k + j*n] * f;
        }
        e[j] = g;
      }
      f = 0.0;
      for (j = 0; j < i; j++) {
        e[j] /= h;
        f += e[j] * d[j];
      }
      float hh = f / (h + h);
      for (j = 0; j < i; j++) e[j] -= hh * d[j];

      for (j = 0; j < i; j++) {
        f = d[j];
        g = e[j];
        for (k = j; k <= i-1; k++) {
          V[k + j*n] -= (f * e[k] + g * d[k]);
        }
        d[j] = V[i-1 + j*n];
        V[i + j *n] = 0.0;
      }
    }
    d[i] = h;
  }

  // Accumulate transformations.

  for (i = 0; i < n-1; i++) {
    V[n-1+i*n] = V[i + i*n];
    V[i + i*n] = 1.0;
    float h = d[i+1];
    if (h != 0.0) {
      for (k = 0; k <= i; k++) d[k] = V[k+(i+1)*n] / h;

      for (j = 0; j <= i; j++) {
        float g = 0.0;
        for (k = 0; k <= i; k++) g += V[k+ (i+1)*n] * V[k + j*n];
        for (k = 0; k <= i; k++) V[k + j*n] -= g * d[k];
      }
    }
    for (k = 0; k <= i; k++) V[k + (i+1)*n] = 0.0;
  }
  for (j = 0; j < n; j++) {
    d[j] = V[n-1 + j * n];
    V[n-1 + j * n] = 0.0;
  }
  V[n-1 + n*(n-1)] = 1.0;
  e[0] = 0.0;
}


////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
inline static void gcgTred(int n, double V[], double d[], double e[]) {
  int i, j, k;

  //  This is derived from the Algol procedures tred2 by
  //  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
  //  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
  //  Fortran subroutine in EISPACK.

  for (j = 0; j < n; j++) d[j] = V[n-1 + j * n];

  // Householder reduction to tridiagonal form.
  for (i = n-1; i > 0; i--) {
    // Scale to avoid under/overflow.
    double scale = 0.0;
    double h = 0.0;
    for (k = 0; k < i; k++) scale += fabs(d[k]);

    if (scale == 0.0) {
      e[i] = d[i-1];
      for (j = 0; j < i; j++) {
        d[j] = V[i-1 + j*n];
        V[i + j*n] = 0.0;
        V[j + i*n] = 0.0;
      }
    } else {
      // Generate Householder vector.

      for (k = 0; k < i; k++) {
        d[k] /= scale;
        h += d[k] * d[k];
      }
      double f = d[i-1];
      double g = sqrt(h);
      if (f > 0) g = -g;

      e[i] = scale * g;
      h = h - f * g;
      d[i-1] = f - g;
      for (j = 0; j < i; j++) e[j] = 0.0;

      // Apply similarity transformation to remaining columns.

      for (j = 0; j < i; j++) {
        f = d[j];
        V[j + i * n] = f;
        g = e[j] + V[j + j * n] * f;
        for (k = j+1; k <= i-1; k++) {
          g += V[k + j*n] * d[k];
          e[k] += V[k + j*n] * f;
        }
        e[j] = g;
      }
      f = 0.0;
      for (j = 0; j < i; j++) {
        e[j] /= h;
        f += e[j] * d[j];
      }
      double hh = f / (h + h);
      for (j = 0; j < i; j++) e[j] -= hh * d[j];

      for (j = 0; j < i; j++) {
        f = d[j];
        g = e[j];
        for (k = j; k <= i-1; k++) {
          V[k + j*n] -= (f * e[k] + g * d[k]);
        }
        d[j] = V[i-1 + j*n];
        V[i + j *n] = 0.0;
      }
    }
    d[i] = h;
  }

  // Accumulate transformations.
  for (i = 0; i < n-1; i++) {
    V[n-1+i*n] = V[i + i*n];
    V[i + i*n] = 1.0;
    double h = d[i+1];
    if (h != 0.0) {
      for (k = 0; k <= i; k++) d[k] = V[k+(i+1)*n] / h;

      for (j = 0; j <= i; j++) {
        double g = 0.0;
        for (k = 0; k <= i; k++) g += V[k+ (i+1)*n] * V[k + j*n];
        for (k = 0; k <= i; k++) V[k + j*n] -= g * d[k];
      }
    }
    for (k = 0; k <= i; k++) V[k + (i+1)*n] = 0.0;
  }
  for (j = 0; j < n; j++) {
    d[j] = V[n-1 + j * n];
    V[n-1 + j * n] = 0.0;
  }
  V[n-1 + n*(n-1)] = 1.0;
  e[0] = 0.0;
}


/////////////////////////////////////////////////////////////////////////
// Symmetric tridiagonal QL algorithm.
/////////////////////////////////////////////////////////////////////////
inline static bool gcgTql(int n, float V[], float d[], float e[]) {
//  This is derived from the Algol procedures tql2, by
//  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
//  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
//  Fortran subroutine in EISPACK.
  int i, k, l;

  for (i = 1; i < n; i++) e[i-1] = e[i];
  e[n-1] = 0.0;

  float f = 0.0;
  float tst1 = 0.0;
  for (l = 0; l < n; l++) {
    // Find small subdiagonal element

    tst1 = MAX(tst1, fabsf(d[l]) + fabsf(e[l]));
    int m = l;
    for(; m < n; m++)
      //// IT MUST BE fabsf() and NOT fabs(). Numeric errors might result if this test fails for really small e[m] and tst1 values
      if(fabsf(e[m]) <= EPSILON * tst1) break;

    // If m == l, d[l] is an eigenvalue,
    // otherwise, iterate.

    if (m > l) {
      //int iter = 0;
      do {
        //iter = iter + 1;  // (Could check iteration count here.)

        // Compute implicit shift
        float g = d[l];
        float p = (float) ((d[l+1] - g) / (2.0 * e[l]));
        float r = (float) sqrtf(p * p + 1.0f);
        if (p < 0) {
          r = -r;
        }
        d[l] = e[l] / (p + r);
        d[l+1] = e[l] * (p + r);
        float dl1 = d[l+1];
        float h = g - d[l];
        for (i = l+2; i < n; i++) {
          d[i] -= h;
        }
        f = f + h;

        // Implicit QL transformation.

        p = d[m];
        float c = 1.0;
        float c2 = c;
        float c3 = c;
        float el1 = e[l+1];
        float s = 0.0;
        float s2 = 0.0;
        for (i = m-1; i >= l; i--) {
          c3 = c2;
          c2 = c;
          s2 = s;
          g = c * e[i];
          h = c * p;
          r = sqrt(p * p + e[i] * e[i]);
          e[i+1] = s * r;
          s = e[i] / r;
          c = p / r;
          p = c * d[i] - s * g;
          d[i+1] = h + s * (c * g + s * d[i]);

          // Accumulate transformation.

          for (k = 0; k < n; k++) {
            h = V[k + n*(i+1)];
            V[k + n * (i+1)] = s * V[k + n*i] + c * h;
            V[k + n*i] = c * V[k + n*i] - s * h;
          }
        }
        p = -s * s2 * c3 * el1 * e[l] / dl1;
        e[l] = s * p;
        d[l] = c * p;

        // Check for convergence.

      } while (fabsf(e[l]) > EPSILON * tst1);
    }
    d[l] = d[l] + f;
    e[l] = 0.0;
  }

  bool hasnan = false;
  int n2 = n * n;

  // Check for NaN
  for (i = 0; i < n && !hasnan; i++)
    if(isnan(d[i])) hasnan = true;
  for (i = 0; i < n2 && !hasnan; i++)
    if(isnan(V[i])) hasnan = true;

  if(hasnan){
    // Eigenvectors form the canonic base
    for(i = 0; i < n2; i++) V[i] = 0.0;

    // All eigenvalues are now zero
    for(i = 0; i < n; i++) {
        d[i] = 0.0;
        V[i * n + i] = 1.0;
    }
    return false;
  }

  // Sort eigenvalues and corresponding vectors.
  for (i = 0; i < n-1; i++) {
    int k = i;
    int j;
    float p = d[i];
    for (j = i+1; j < n; j++) {
      if (d[j] < p) {
        k = j;
        p = d[j];
      }
    }
    if (k != i) {
      d[k] = d[i];
      d[i] = p;
      for (j = 0; j < n; j++) {
        p = V[j + n*i];
        V[j + n * i] = V[j + n * k];
        V[j + n * k] = p;
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
inline static bool gcgTql(int n, double V[], double d[], double e[]) {
//  This is derived from the Algol procedures tql2, by
//  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
//  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
//  Fortran subroutine in EISPACK.
  int i, k, l;

  for (i = 1; i < n; i++) e[i-1] = e[i];
  e[n-1] = 0.0;

  double f = 0.0;
  double tst1 = 0.0;
  for (l = 0; l < n; l++) {
    // Find small subdiagonal element
    tst1 = MAX(tst1, fabs(d[l]) + fabs(e[l]));

    int m = l;
    for(; m < n; m++)
      //// IT MUST BE fabsf() and NOT fabs(). Numeric errors might result if this test fails for really small e[m] and tst1 values
      if(fabs(e[m]) <= EPSILON * tst1) break;

    // If m == l, d[l] is an eigenvalue,
    // otherwise, iterate.

    if (m > l) {
      //int iter = 0;
      do {
        //iter = iter + 1;  // (Could check iteration count here.)

        // Compute implicit shift
        double g = d[l];
        double p = (double) ((d[l+1] - g) / (2.0 * e[l]));
        double r = (double) sqrt(p * p + 1.0);
        if (p < 0) r = -r;

        d[l] = e[l] / (p + r);
        d[l+1] = e[l] * (p + r);

        double dl1 = d[l+1];
        double h = g - d[l];
        for (i = l+2; i < n; i++) d[i] -= h;
        f = f + h;

        // Implicit QL transformation.

        p = d[m];
        double c = 1.0;
        double c2 = c;
        double c3 = c;
        double el1 = e[l+1];
        double s = 0.0;
        double s2 = 0.0;
        for (i = m-1; i >= l; i--) {
          c3 = c2;
          c2 = c;
          s2 = s;
          g = c * e[i];
          h = c * p;
          r = sqrt(p * p + e[i] * e[i]);
          e[i+1] = s * r;
          s = e[i] / r;
          c = p / r;
          p = c * d[i] - s * g;
          d[i+1] = h + s * (c * g + s * d[i]);

          // Accumulate transformation.
          for (k = 0; k < n; k++) {
            h = V[k + n*(i+1)];
            V[k + n * (i+1)] = s * V[k + n*i] + c * h;
            V[k + n*i] = c * V[k + n*i] - s * h;
          }
        }
        p = -s * s2 * c3 * el1 * e[l] / dl1;
        e[l] = s * p;
        d[l] = c * p;

        // Check for convergence.

      } while(fabs(e[l]) > EPSILON * tst1);
    }
    d[l] = d[l] + f;
    e[l] = 0.0;
  }

  bool hasnan = false;
  int n2 = n * n;

  // Check for NaN
  for (i = 0; i < n && !hasnan; i++)
    if(isnan(d[i])) hasnan = true;
  for (i = 0; i < n2 && !hasnan; i++)
    if(isnan(V[i])) hasnan = true;

  if(hasnan){
    // Eigenvectors form the canonic base
    for(i = 0; i < n2; i++) V[i] = 0.0;

    // All eigenvalues are now zero
    for(i = 0; i < n; i++) {
        d[i] = 0.0;
        V[i * n + i] = 1.0;
    }
    return false;
  }

  // Sort eigenvalues and corresponding vectors.
  for (i = 0; i < n-1; i++) {
    int k = i;
    int j;
    double p = d[i];
    for (j = i+1; j < n; j++) {
      if (d[j] < p) {
        k = j;
        p = d[j];
      }
    }
    if (k != i) {
      d[k] = d[i];
      d[i] = p;
      for (j = 0; j < n; j++) {
        p = V[j + n*i];
        V[j + n * i] = V[j + n * k];
        V[j + n * k] = p;
      }
    }
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////
// Computes the eigen decomposition of symmetric matrices matrix with arbitraty
// order norder. The resulted eigenvectors are stored int a matrix eigenvectors[norder][norder]
// and the respective eigenvalues are stored in the vector eigenvector[n]. They are
// ordered by decreasing eigenvalues. Adapted from the public domain Java
// Matrix library JAMA.
///////////////////////////////////////////////////////////////////////////
bool gcgEigenSymmetric(int norder, float matrix[], float eigenvectors[], float eigenvalues[])  {
  float *e;
  memcpy(eigenvectors, matrix, sizeof(float) * norder * norder);
  e = (float*) ALLOC(sizeof(float) * norder);
  if(e) {
    gcgTred(norder, eigenvectors, eigenvalues, e);
    bool res = gcgTql(norder, eigenvectors, eigenvalues, e);
    SAFE_FREE(e);
    if(!res) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgEigenSymmetric(): NaN found. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return res;
  }

  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgEigenSymmetric(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
  return false;
}

////////////////////////////////////////////////////
/************ Double precision version ************/
////////////////////////////////////////////////////
bool gcgEigenSymmetric(int norder, double matrix[], double eigenvectors[], double eigenvalues[])  {
  double *e;
  memmove(eigenvectors, matrix, sizeof(double) * norder * norder);
  e = (double*) ALLOC(sizeof(double) * norder);
  if(e) {
    gcgTred(norder, eigenvectors, eigenvalues, e);
    bool res = gcgTql(norder, eigenvectors, eigenvalues, e);
    SAFE_FREE(e);
    if(!res) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgEigenSymmetric(): NaN found. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return res;
  }

  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgEigenSymmetric(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
  return false;
}

