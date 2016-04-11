/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   MATRIXMAIN.CPP: functions and methods for matrix algebra with multiple types.

   Marcelo Bernardes Vieira
******************* *******************************************************************/

/////////////////////////////////////////////////////////////////////////////
// Loads several times to automatically include classes with distinct types
#undef NUMBERTYPE
#undef NUMBERTYPE_MIN
#undef NUMBERTYPE_MAX

#if !defined(TYPECOUNTER)
   ////***** Everything inside this block will be included ONCE ******/////

  #include "system.h"

  #define TYPECOUNTER 1
  #define NUMBERTYPE float
  #define NUMBERTYPE_MIN -((float) INF)
  #define NUMBERTYPE_MAX ((float) INF)


  ////***** Everything above this point will be included ONCE ******/////
#elif TYPECOUNTER == 1
  #undef TYPECOUNTER
  #define TYPECOUNTER 2
  #define NUMBERTYPE double
  #define NUMBERTYPE_MIN -INF
  #define NUMBERTYPE_MAX INF
#endif



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////     MATRIX       ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


gcgMATRIX<NUMBERTYPE>::gcgMATRIX() {
  	this->data = NULL;
  	this->rows = 0;
  	this->columns = 0;
  	this->isstaticdata = false;
}

gcgMATRIX<NUMBERTYPE>::gcgMATRIX(unsigned int rows, unsigned int columns, NUMBERTYPE *sampledata, bool isstaticdata) {
  this->data = NULL;
  this->rows = 0;
  this->columns = 0;
  this->isstaticdata = false;
  this->createMatrix(rows, columns, sampledata, isstaticdata);
}


gcgMATRIX<NUMBERTYPE>::~gcgMATRIX() {
  destroyMatrix();
}

bool gcgMATRIX<NUMBERTYPE>::createMatrix(unsigned int irows, unsigned int icolumns, NUMBERTYPE *sampledata, bool _isstaticdata) {
  // Check the parameters
  if(irows == 0 || icolumns < 1) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_ALGEBRA, GCG_UNSUPPORTEDFORMAT), "createMatrix(): unsupported dimensions (%d, %d). (%s:%d)", irows, icolumns, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(_isstaticdata && sampledata != NULL) {
    this->destroyMatrix();
    this->data = sampledata;
    this->isstaticdata = true;
  } else {
    // Reuse previous space
    if(this->data == NULL || this->rows != irows || this->columns != icolumns || this->isstaticdata) {
      // Release previous resources
      this->destroyMatrix();

      // Create memory for matrix data
      this->data = new NUMBERTYPE[irows * icolumns];
      if(this->data == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "createMatrix(): matrix data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
      this->isstaticdata = false;
    }

    if(sampledata == NULL) memset(this->data, 0, sizeof(NUMBERTYPE) * irows * icolumns);
    else memcpy(this->data, sampledata, sizeof(NUMBERTYPE) * irows * icolumns);
  }

	// Matrix information
  this->rows = irows;
  this->columns  = icolumns;

  // Matrix created
  return true;
}

void gcgMATRIX<NUMBERTYPE>::destroyMatrix() {
  if(!this->isstaticdata) SAFE_DELETE_ARRAY(data);
  this->rows = 0;
  this->columns = 0;
 	this->isstaticdata = false;
}



////////////////////////////////////////////////////////////////////////////////
// Includes cross type functions
////////////////////////////////////////////////////////////////////////////////

// float crossed with all types
#if TYPECOUNTER == 1
  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATARITHMETIC
  #define CROSSTYPE       float
  #define CROSSTYPE_MIN   -((float) INF)
  #define CROSSTYPE_MAX   ((float) INF)
  #define COERSIONTYPE    float
  #define FLOATARITHMETIC float
  #include"matrixcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATARITHMETIC
  #define CROSSTYPE       double
  #define CROSSTYPE_MIN   -INF
  #define CROSSTYPE_MAX   INF
  #define COERSIONTYPE    double
  #define FLOATARITHMETIC double
  #include"matrixcross.cpp"

// double crossed with all types
#elif TYPECOUNTER == 2

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATARITHMETIC
  #define CROSSTYPE       float
  #define CROSSTYPE_MIN   -((float) INF)
  #define CROSSTYPE_MAX   ((float) INF)
  #define COERSIONTYPE    double
  #define FLOATARITHMETIC double
  #include"matrixcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATARITHMETIC
  #define CROSSTYPE       double
  #define CROSSTYPE_MIN   -INF
  #define CROSSTYPE_MAX   INF
  #define COERSIONTYPE    double
  #define FLOATARITHMETIC double
  #include"matrixcross.cpp"
#endif


// Loads several times to automatically include classes with distinct types
#if TYPECOUNTER < 2
  #include "matrixmain.cpp"
#endif
