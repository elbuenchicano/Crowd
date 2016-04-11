/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   SIGNALCROSS.CPP: cross type functions.

   Marcelo Bernardes Vieira
**************************************************************************************/

#if defined(NUMBERTYPE) && defined(CROSSTYPE)

/////////////////////////////////////////////////////////////////////////////
//////////     RAW SCALAR PRODUCT AND CONVOLUTION FUNCTIONS     /////////////
/////////////////////////////////////////////////////////////////////////////

static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  register COERSIONTYPE sum = (COERSIONTYPE) 0;

  if(size1 == 1) { // Will cause zero division
    for(register int pos = 0; pos < size2; pos++, data2++) sum += (COERSIONTYPE) ((COERSIONTYPE) *data2 * (COERSIONTYPE) *data1);
    return sum;
  }

  register int j = column1 - (int) origin2;
  register int length2_2 = size1 + size1 - 2;
  for(register int pos = 0; pos < size2; j++, pos++, data2++) {
    register int k = (j >= 0) ? (j % length2_2) : (-j % length2_2); // Symmetric with no repeat
    if(k >= (int) size1) k =  length2_2 - k;
    sum += (COERSIONTYPE) ((COERSIONTYPE) *data2 * (COERSIONTYPE) data1[k]);
  }
  return sum;
}

static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  register COERSIONTYPE sum = (COERSIONTYPE) 0;

  if(size1 == 1) { // Will cause zero division
    for(register int pos = 0; pos < size2; pos++, data2++) sum += (COERSIONTYPE) ((COERSIONTYPE) *data2 * (COERSIONTYPE) *data1);
    return sum;
  }

  register int j = column1 - (int) origin2;
  register int length2 = size1 + size1;
  for(register int pos = 0; pos < size2; j++, pos++, data2++) {
    register int k = (j >= 0) ? (j % length2) : (-(j + 1) % length2);  // Symmetric with repeat
    if(k >= (int) size1) k = length2 - k - 1;

    sum += (COERSIONTYPE) ((COERSIONTYPE) *data2 * (COERSIONTYPE) data1[k]);
  }
  return sum;
}

static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  register COERSIONTYPE sum = (COERSIONTYPE) 0;
  if(size1 == 1) { // Will cause zero division
    for(register int pos = 0; pos < size2; pos++, data2++) sum += (COERSIONTYPE) ((COERSIONTYPE) *data2 * (COERSIONTYPE) *data1);
    return sum;
  }

  register int j = column1 - (int) origin2;
  register int length_1 = size1 - 1;
  for(register int pos = 0; pos < size2; j++, pos++, data2++) {
    int k = (j >= 0) ? j % size1 : length_1 - ((-j - 1) % (int) size1); // Periodic
    sum += (COERSIONTYPE) ((COERSIONTYPE) *data2 * (COERSIONTYPE) data1[k]);
  }
  return sum;
}

static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  register COERSIONTYPE sum = (COERSIONTYPE) 0;
  register int j = column1 - (int) origin2;
  register int length_1 = size1 - 1;
  for(register int pos = 0; pos < size2; j++, pos++, data2++) {
    int k = (j <= 0) ? 0 : ((j >= (int) size1) ? length_1 : j); // Clamp
    sum += (COERSIONTYPE) ((COERSIONTYPE) *data2 * (COERSIONTYPE) data1[k]);
  }
  return sum;
}

static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  register COERSIONTYPE sum = (COERSIONTYPE) 0;
  register int j = column1 - (int) origin2;
  for(register int pos = 0; pos < size2; j++, pos++, data2++)
    if(j >= 0 && j < (int) size1) sum += (COERSIONTYPE) ((COERSIONTYPE) *data2 * (COERSIONTYPE) data1[j]);  // Zero
  return sum;
}



// Second data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolution1D_zero2(NUMBERTYPE *dstrow, int size1, NUMBERTYPE *data1, int border1, int size2, CROSSTYPE *data2, int origin2) {
  switch(border1) {
    case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductsymmetricnorepeat1D(i, size1, data1, size2, data2, origin2);
            break;

    case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductsymmetricrepeat1D(i, size1, data1, size2, data2, origin2);
            break;

     case GCG_BORDER_EXTENSION_PERIODIC:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductperiodic1D(i, size1, data1, size2, data2, origin2);
            break;

     case GCG_BORDER_EXTENSION_CLAMP:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductclamp1D(i, size1, data1, size2, data2, origin2);
            break;

     case GCG_BORDER_EXTENSION_ZERO:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductzero1D(i, size1, data1, size2, data2, origin2);
            break;
  }
}

// First data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolution1D_zero1(NUMBERTYPE *dstrow, int size1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int border2, int origin2) {
  switch(border2) {
    case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductsymmetricnorepeat1D(origin2, size2, data2, size1, data1, i);
            break;

    case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductsymmetricrepeat1D(origin2, size2, data2, size1, data1, i);
            break;

     case GCG_BORDER_EXTENSION_PERIODIC:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductperiodic1D(origin2, size2, data2, size1, data1, i);
            break;

     case GCG_BORDER_EXTENSION_CLAMP:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductclamp1D(origin2, size2, data2, size1, data1, i);
            break;

     case GCG_BORDER_EXTENSION_ZERO:
            for(register int i = 0; i < size1; i++, dstrow++)
              *dstrow = (NUMBERTYPE) scalarproductzero1D(origin2, size2, data2, size1, data1, i);
            break;
  }
}

static inline COERSIONTYPE scalarproduct2D(int atX, int atY, gcgDISCRETE2D<NUMBERTYPE> *src1,  gcgDISCRETE2D<CROSSTYPE> *src2) {
  atX += src1->originX;
  atY += src1->originY;

  // Find computation domain
  int iniX1, iniY1, iniX2, iniY2, endX1, endY1;
  if(src2->extensionX == GCG_BORDER_EXTENSION_ZERO || src1->extensionX != GCG_BORDER_EXTENSION_ZERO) {
    iniX1 = atX - (int) src2->originX;
    endX1 = iniX1 + (int) src2->width;
    iniX2 = 0;
  } else {
    iniX1 = 0;
    endX1 = iniX1 + (int) src1->width;
    iniX2 = (int) src2->originX - atX;
  }
  if(src2->extensionY == GCG_BORDER_EXTENSION_ZERO || src1->extensionY != GCG_BORDER_EXTENSION_ZERO) {
    iniY1 = atY - (int) src2->originY;
    endY1 = iniY1 + (int) src2->height;
    iniY2 = 0;
  } else {
    iniY1 = 0;
    endY1 = iniY1 + (int) src1->height;
    iniY2 = (int) src2->originY - atY;
  }

  // Computes scalar product: must be improved by the compiler
  register COERSIONTYPE sum = (COERSIONTYPE) 0;
  register int j2 = iniY2;
  for(register int j1 = iniY1; j1 < endY1; j1++, j2++) {
    register int i2 = iniX2;
    for(register int i1 = iniX1; i1 < endX1; i1++, i2++)
      sum += ((COERSIONTYPE) sample2D(i1, j1, src1->width, src1->height, src1->extensionX, src1->extensionY, src1->data) *
              (COERSIONTYPE) sample2D(i2, j2, src2->width, src2->height, src2->extensionX, src2->extensionY, src2->data));
  }

  return sum;
}


// Second data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolutionsymmetricnorepeat2D1D_zero2(NUMBERTYPE *dstrow, int row1, int width1, int height1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  if(height1 == 1) { // Causes zero division
    for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
      register COERSIONTYPE sum = (COERSIONTYPE) 0;
      for(register int pos = 0; pos < size2; pos++) sum += (COERSIONTYPE) data1[column1] * (COERSIONTYPE) data2[pos];
      *dstrow = (NUMBERTYPE) sum;
    }
    return;
  }

  register int length2_2 = height1 + height1 - 2;
  for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
    register COERSIONTYPE sum = (COERSIONTYPE) 0;
    register int j = row1 - origin2;
    register NUMBERTYPE *srcdata = &data1[column1];
    for(register int pos = 0; pos < size2; j++, pos++) {
      register int k = (j >= 0) ? (j % length2_2) : (-j % length2_2); // Symmetric with no repeat
      if(k >= (int) height1) k =  length2_2 - k;
      sum += (COERSIONTYPE) data2[pos] * (COERSIONTYPE) srcdata[width1 * k];
    }
    *dstrow = (NUMBERTYPE) sum;
  }
}

// Second data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolutionsymmetricrepeat2D1D_zero2(NUMBERTYPE *dstrow, int row1, int width1, int height1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  if(height1 == 1) { // Causes zero division
    for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
      register COERSIONTYPE sum = (COERSIONTYPE) 0;
      for(register int pos = 0; pos < size2; pos++) sum += (COERSIONTYPE) data1[column1] * (COERSIONTYPE) data2[pos];
      *dstrow = (NUMBERTYPE) sum;
    }
    return;
  }

  register int length2 = height1 + height1;
  for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
    register COERSIONTYPE sum = (COERSIONTYPE) 0;
    register int j = row1 - origin2;
    register NUMBERTYPE *srcdata = &data1[column1];
    for(register int pos = 0; pos < size2; j++, pos++) {
      register int k = (j >= 0) ? (j % length2) : (-(j + 1) % length2); // Symmetric with repeat
      if(k >= (int) height1) k =  length2 - k - 1;

      sum += (COERSIONTYPE) data2[pos] * (COERSIONTYPE) srcdata[width1 * k];
    }
    *dstrow = (NUMBERTYPE) sum;
  }
}


// Second data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolutionperiodic2D1D_zero2(NUMBERTYPE *dstrow, int row1, int width1, int height1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  if(height1 == 1) { // Causes zero division
    for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
      register COERSIONTYPE sum = (COERSIONTYPE) 0;
      for(register int pos = 0; pos < size2; pos++) sum += (COERSIONTYPE) data1[column1] * (COERSIONTYPE) data2[pos];
      *dstrow = (NUMBERTYPE) sum;
    }
    return;
  }

  register int length_1 = height1 - 1;
  for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
    register COERSIONTYPE sum = (COERSIONTYPE) 0;
    register int j = row1 - origin2;
    register NUMBERTYPE *srcdata = &data1[column1];
    for(register int pos = 0; pos < size2; j++, pos++) {
      int k = (j >= 0) ? j % height1 : length_1 - ((-j - 1) % height1); // Periodic
      sum += (COERSIONTYPE) data2[pos] * (COERSIONTYPE) srcdata[width1 * k];
    }
    *dstrow = (NUMBERTYPE) sum;
  }
}

// Second data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolutionclamp2D1D_zero2(NUMBERTYPE *dstrow, int row1, int width1, int height1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  register int length_1 = height1 - 1;
  for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
    register COERSIONTYPE sum = (COERSIONTYPE) 0;
    register int j = row1 - origin2;
    register NUMBERTYPE *srcdata = &data1[column1];
    for(register int pos = 0; pos < size2; j++, pos++) {
      int k = (j <= 0) ? 0 : ((j >= (int) height1) ? length_1 : j); // Clamp
      sum += (COERSIONTYPE) data2[pos] * (COERSIONTYPE) srcdata[width1 * k];
    }
    *dstrow = (NUMBERTYPE) sum;
  }
}

// Second data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolutionzero2D1D_zero2(NUMBERTYPE *dstrow, int row1, int width1, int height1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
    register COERSIONTYPE sum = (COERSIONTYPE) 0;
    register int j = row1 - origin2;
    register NUMBERTYPE *srcdata = &data1[column1];
    for(register int pos = 0; pos < size2; j++, pos++)
      if(j >= 0 && j < (int) height1) sum += (COERSIONTYPE) data2[pos] * (COERSIONTYPE) srcdata[width1 * j];  // Zero
    *dstrow = (NUMBERTYPE) sum;
  }
}


// First data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolutionsymmetricnorepeat2D1D_zero1(NUMBERTYPE *dstrow, int row1, int width1, int height1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  if(size2 == 1) { // Causes zero division
    for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
      register NUMBERTYPE *srcdata = &data1[column1];
      register COERSIONTYPE sum = (COERSIONTYPE) 0;
      for(register int row1 = 0; row1 < height1; row1++) sum += (COERSIONTYPE) srcdata[width1 * row1] * (COERSIONTYPE) *data2;
      *dstrow = (NUMBERTYPE) sum;
    }
    return;
  }

  register int length2_2 = size2 + size2 - 2;
  for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
    register COERSIONTYPE sum = (COERSIONTYPE) 0;
    register int j = origin2 - row1;
    register NUMBERTYPE *srcdata = &data1[column1];
    for(register int pos = 0; pos < height1; j++, pos++) {
      register int k = (j >= 0) ? (j % length2_2) : (-j % length2_2); // Symmetric with no repeat
      if(k >= (int) size2) k =  length2_2 - k;
      sum += (COERSIONTYPE) srcdata[pos * width1] * (COERSIONTYPE) data2[k];
    }
    *dstrow = (NUMBERTYPE) sum;
  }
}

// First data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolutionsymmetricrepeat2D1D_zero1(NUMBERTYPE *dstrow, int row1, int width1, int height1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  if(size2 == 1) { // Causes zero division
    for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
      register NUMBERTYPE *srcdata = &data1[column1];
      register COERSIONTYPE sum = (COERSIONTYPE) 0;
      for(register int row1 = 0; row1 < height1; row1++) sum += (COERSIONTYPE) srcdata[width1 * row1] * (COERSIONTYPE) *data2;
      *dstrow = (NUMBERTYPE) sum;
    }
    return;
  }

  register int length2 = size2 + size2;
  for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
    register COERSIONTYPE sum = (COERSIONTYPE) 0;
    register int j = origin2 - row1;
    register NUMBERTYPE *srcdata = &data1[column1];
    for(register int pos = 0; pos < height1; j++, pos++) {
      register int k = (j >= 0) ? (j % length2) : (-(j + 1) % length2); // Symmetric with repeat
      if(k >= (int) size2) k =  length2 - k - 1;
      sum += (COERSIONTYPE) srcdata[pos * width1] * (COERSIONTYPE) data2[k];
    }
    *dstrow = (NUMBERTYPE) sum;
  }
}


// First data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolutionperiodic2D1D_zero1(NUMBERTYPE *dstrow, int row1, int width1, int height1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  if(size2 == 1) { // Causes zero division
    for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
      register NUMBERTYPE *srcdata = &data1[column1];
      register COERSIONTYPE sum = (COERSIONTYPE) 0;
      for(register int row1 = 0; row1 < height1; row1++) sum += (COERSIONTYPE) srcdata[width1 * row1] * (COERSIONTYPE) *data2;
      *dstrow = (NUMBERTYPE) sum;
    }
    return;
  }

  register int length_1 = size2 - 1;
  for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
    register COERSIONTYPE sum = (COERSIONTYPE) 0;
    register int j = origin2 - row1;
    register NUMBERTYPE *srcdata = &data1[column1];
    for(register int pos = 0; pos < height1; j++, pos++) {
      int k = (j >= 0) ? j % size2 : length_1 - ((-j - 1) % size2); // Periodic
      sum += (COERSIONTYPE) srcdata[pos * width1] * (COERSIONTYPE) data2[k];
    }
    *dstrow = (NUMBERTYPE) sum;
  }
}

// First data is extended as zero (GCG_BORDER_EXTENSION_ZERO)
static inline void convolutionclamp2D1D_zero1(NUMBERTYPE *dstrow, int row1, int width1, int height1, NUMBERTYPE *data1, int size2, CROSSTYPE *data2, int origin2) {
  register int length_1 = size2 - 1;
  for(register int column1 = 0; column1 < width1; column1++, dstrow++) {
    register COERSIONTYPE sum = (COERSIONTYPE) 0;
    register int j = origin2 - row1;
    register NUMBERTYPE *srcdata = &data1[column1];
    for(register int pos = 0; pos < height1; j++, pos++) {
      int k = (j <= 0) ? 0 : ((j >= (int) size2) ? length_1 : j); // Clamp
      sum += (COERSIONTYPE) srcdata[pos * width1] * (COERSIONTYPE) data2[k];
    }
    *dstrow = (NUMBERTYPE) sum;
  }
}




/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////     UNIDIMENSIONAL SIGNALS     ////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

bool gcgDISCRETE1D<NUMBERTYPE>::createSimilar(gcgDISCRETE1D<CROSSTYPE> *source) {
  // Check source.
  if(source == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_SIGNAL, GCG_BADPARAMETERS), "createSimilar(): source is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
  }
  if(source->data == NULL || source->length == 0) {
      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_SIGNAL, GCG_INVALIDOBJECT), "createSimilar(): invalid source dimension. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
  }

  // Structural incompatibility
  if(this->data == NULL || source->length != this->length || this->data == NULL || this->isstaticdata) {
    this->destroySignal(); // Release previous resources

    // Duplicate memory for signal data
    this->data = new NUMBERTYPE [source->length];
    if(data == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "createSimilar(): signal data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

    // Copy main parameters
    this->length = source->length;
    this->isstaticdata = false;
  }

  this->origin = source->origin;
  this->extension = source->extension;

	// Ok, it's a similar signal.
	return true;
}

////////////////////////////////////////////////////////////
// Checks if both signals are compatible: same dimensions,
// same origin.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::isCompatibleWith(gcgDISCRETE1D<CROSSTYPE> *source) {
  // Check source.
  //errorcode = GCG_INVALID_OBJECT;
  if(source == NULL) return false;
  if(source->data == NULL) return false;

  // Check equivalence between both signals.
  //errorcode = GCG_SUCCESS;
  if(source->length != this->length || source->origin != this->origin) return false;

  return true;
}

////////////////////////////////////////////////////////////
// Makes an independent copy of the source signal.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::duplicateSignal(gcgDISCRETE1D<CROSSTYPE> *source) {
  // Check source.
  //errorcode = GCG_INVALID_OBJECT;
  if(source == NULL) return false;
  if(source->data == NULL || source->length == 0) return false;

  // Check destination signal: destination must be compatible with the source
  if((void*) this == (void*) source) {
    //errorcode = GCG_SUCCESS;
    return true;
  }

  if(!this->createSimilar(source)) return false; // createSimilar creates only what is needed to make them compatible.

 	// Copy memory for signal data
 	register const unsigned int size = this->length;
 	register NUMBERTYPE *dst = this->data;
 	register CROSSTYPE  *src = source->data;
 	for(register unsigned int i = 0; i < size; i++) *(dst++) = (NUMBERTYPE) *(src++);

	// Ok, it's duplicated.
	//errorcode = GCG_SUCCESS;
	return true;
}

////////////////////////////////////////////////////////////
// Makes an independent copy of the subsignal from the source.
// Source can be the destination.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::duplicateSubsignal(gcgDISCRETE1D<CROSSTYPE> *source, int left, unsigned int _width) {
  // Check source.
  //errorcode = GCG_INVALID_OBJECT;
  if(source == NULL) return false;
  if(source->data == NULL || source->length == 0) return false;

  // Check parameters
  if(_width == 0) return false;

  // Check source and destination
  CROSSTYPE *olddata = source->data;
  unsigned int oldlength = source->length;

  //errorcode = GCG_MEMORYALLOCERROR;

  // Copy main parameters
  this->length = _width;
  this->origin = -left;
  left += source->origin; // Find true beginning using origin

  if((void*) this != (void*) source)
    if(this->data && !this->isstaticdata) SAFE_DELETE_ARRAY(data);

  // Make memory for signal data
  this->data = new NUMBERTYPE [this->length];
  if(this->data == NULL) return false;

	// Compute real extents
	unsigned int xini = 0, xoff = 0;

	if(left < 0) _width -= (xoff = abs(left));
	else xini = (unsigned int) left;

	if(xini + _width  > oldlength)  _width  = oldlength - xini;

  // Copies samples in data buffer
  register NUMBERTYPE *dst = &data[xoff];
  register CROSSTYPE  *src = &olddata[xini];
  for(register unsigned int i = 0; i < _width; i++) *(dst++) = (NUMBERTYPE) *(src++);

  // Release old data
  if((void*) this == (void*) source)
    if(olddata && !this->isstaticdata) SAFE_DELETE_ARRAY(olddata);

  this->isstaticdata = false;

	// Ok, it's copied.
	//errorcode = GCG_SUCCESS;
	return true;
}

////////////////////////////////////////////////////////////
// Normalizes a signal in the interval [floor ceil]
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::normalize(gcgDISCRETE1D<CROSSTYPE> *src, CROSSTYPE floor, CROSSTYPE ceil) {
  // Check source signal
  if(src->data == NULL || src->length == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Check destination signal: destination must be compatible with the source
  if((void*) this != (void*) src)
    if(!this->createSimilar(src)) return false; // createSimilar creates only what is needed to make them compatible.

 	// Find minimum and maximum
 	CROSSTYPE min = CROSSTYPE_MIN, max = CROSSTYPE_MAX;
  register CROSSTYPE *srcdata  = src->data;
	for(unsigned int current = 0; current < length; current++) {
      if(*srcdata > max) max = *srcdata;
      if(*srcdata < min) min = *srcdata;
  }

#if FLOATINGCROSS
  if(FEQUAL(min, max)) {
#else
  if(min == max) {
#endif
    min -= (CROSSTYPE) 1;
    max += (CROSSTYPE) 1;
  }

#if FLOATINGCROSS
 	// Normalize
 	CROSSTYPE inv = (CROSSTYPE) (ceil - floor) / (CROSSTYPE) (max - min);
  register NUMBERTYPE *dstdata  = this->data;
  srcdata = src->data;
	for(unsigned int current = 0; current < length; current++) *dstdata = (NUMBERTYPE) (floor + (*srcdata - min) * inv);
#else
 	// Normalize
 	FLOATARITHMETIC inv = (FLOATARITHMETIC) (ceil - floor) / (FLOATARITHMETIC) (max - min);
  register NUMBERTYPE *dstdata  = this->data;
  srcdata = src->data;
	for(unsigned int current = 0; current < length; current++) *dstdata = (NUMBERTYPE) ((FLOATARITHMETIC) floor + (FLOATARITHMETIC) ((FLOATARITHMETIC) (*srcdata - min) * inv));
#endif

  // Normalized signal computed
  //errorcode = GCG_SUCCESS;
  return true;
}

////////////////////////////////////////////////////////////
// Applies a convolution of a mask.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::convolution(gcgDISCRETE1D<NUMBERTYPE> *src, gcgDISCRETE1D<CROSSTYPE> *mask) {
  // Check parameters
  //errorcode = GCG_UNSUPPORTED_OPERATION;
  if(src == NULL || mask == NULL) return false;
  if(src->data == NULL || mask->data == NULL) return false;

  // Check destination signal: destination must be compatible with the source
  NUMBERTYPE *dstdata = NULL;
  if((void*) this != (void*) src) {
     // createSimilar creates only what is needed to make them compatible.
    if(!this->createSimilar(src)) return false;
    dstdata = this->data;
  } else {
    // We need an output buffer
    dstdata = new NUMBERTYPE [length];
    if(dstdata == NULL) {
      //errorcode = GCG_MEMORYALLOCERROR;
      return false;
    }
  }

  if(mask->extension == GCG_BORDER_EXTENSION_ZERO || src->extension != GCG_BORDER_EXTENSION_ZERO)
       convolution1D_zero2(dstdata, this->length, src->data, src->extension, mask->length, mask->data, mask->origin);
  else convolution1D_zero1(dstdata, this->length, src->data, mask->length, mask->data, mask->extension, mask->origin);

  // Commit filtered data
  if(this == src) {
    // We have new data
    if(!this->isstaticdata) SAFE_DELETE_ARRAY(this->data);
    this->data = dstdata;
    this->isstaticdata = false;
  }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;
	return true;
}

////////////////////////////////////////////////////////////
// Performs a template matching of the mask with the signal.
// Returns true if successful. In this case, the most likely
// position of the template is stored in 'position' vector.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::templateMatching(unsigned int imgleft, unsigned int imgwidth, gcgDISCRETE1D<CROSSTYPE> *mask, int *position) {
  // Check parameter
  //errorcode = GCG_UNSUPPORTED_OPERATION;
  if(mask == NULL || imgwidth == 0 || position == NULL) return false;
  if(mask->length == 0) return false;

  // Check source signal
  if(data == NULL || length == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  unsigned int iniX = mask->origin;
  unsigned int endX = length - (mask->length - mask->origin);
  if(imgleft > iniX) iniX = imgleft;
  if(imgleft + imgwidth - 1 < endX) endX = imgleft + imgwidth - 1;

#if FLOATING
  NUMBERTYPE bestSAD = NUMBERTYPE_MAX; // Stores the smallest Sum of Absolute Differences
#else
  NUMBERTYPE bestSAD = NUMBERTYPE_MAX; // Stores the smallest Sum of Absolute Differences
#endif
  for(unsigned int col = iniX; col < endX; col++) {
    register NUMBERTYPE sum = 0;
    register NUMBERTYPE *srcrow = this->data;
    register CROSSTYPE *maskrow = mask->data;
    register int l = col - (int) mask->origin;
    for(register int posX = 0; posX < (int) mask->length; l++, posX++)
#if FLOATING || FLOATINGCROSS
      sum += (NUMBERTYPE) fabs(maskrow[posX] - srcrow[l]);
#else
      sum += (NUMBERTYPE) abs(maskrow[posX] - srcrow[l]);
#endif

    // Check SAD
    if(bestSAD > sum) {
      bestSAD = sum;
      position[0] = col;
    }
  }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}

////////////////////////////////////////////////////////////
// Computes the scalar product of two signals. At least one
// of the signals must have extension GCG_BORDER_EXTENSION_ZERO.
// Aligns the origin of srcsignal2 with the sample of this object
// with coordinate atX.
////////////////////////////////////////////////////////////
COERSIONTYPE gcgDISCRETE1D<NUMBERTYPE>::scalarProduct(int atX, gcgDISCRETE1D<CROSSTYPE> *signal2) {
  // Check parameters
  //errorcode = GCG_BADPARAMETERS;
  if(this == NULL || signal2 == NULL) return (COERSIONTYPE) 0;
  if(this->data == NULL || signal2->data == NULL) return (COERSIONTYPE) 0;

  COERSIONTYPE  res = (COERSIONTYPE) 0;
  if(signal2->extension == GCG_BORDER_EXTENSION_ZERO || this->extension != GCG_BORDER_EXTENSION_ZERO)
    switch(this->extension) {
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT:
              res = scalarproductsymmetricnorepeat1D(this->origin + atX, this->length, this->data, signal2->length, signal2->data, signal2->origin);
              break;

      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT:
              res = scalarproductsymmetricrepeat1D(this->origin + atX, this->length, this->data, signal2->length, signal2->data, signal2->origin);
              break;

       case GCG_BORDER_EXTENSION_PERIODIC:
              res = scalarproductperiodic1D(this->origin + atX, this->length, this->data, signal2->length, signal2->data, signal2->origin);
              break;

       case GCG_BORDER_EXTENSION_CLAMP:
              res = scalarproductclamp1D(this->origin + atX, this->length, this->data, signal2->length, signal2->data, signal2->origin);
              break;

       case GCG_BORDER_EXTENSION_ZERO:
              res = scalarproductzero1D(this->origin + atX, this->length, this->data, signal2->length, signal2->data, signal2->origin);
              break;
    }
  else
    switch(signal2->extension) {
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT:
              res = scalarproductsymmetricnorepeat1D(signal2->origin, signal2->length, signal2->data, this->length, this->data, this->origin + atX);
              break;

      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT:
              res = scalarproductsymmetricrepeat1D(signal2->origin, signal2->length, signal2->data, this->length, this->data, this->origin + atX);
              break;

       case GCG_BORDER_EXTENSION_PERIODIC:
              res = scalarproductperiodic1D(signal2->origin, signal2->length, signal2->data, this->length, this->data, this->origin + atX);
              break;

       case GCG_BORDER_EXTENSION_CLAMP:
              res = scalarproductclamp1D(signal2->origin, signal2->length, signal2->data, this->length, this->data, this->origin + atX);
              break;

       case GCG_BORDER_EXTENSION_ZERO:
              res = scalarproductzero1D(signal2->origin, signal2->length, signal2->data, this->length, this->data, this->origin + atX);
              break;
    }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return res;
}

////////////////////////////////////////////////////////////
// Produces a binary signal. The samples are mapped to: lessvalue,
// if sample is below the threshold; greaterequalvalue, if the sample
// is greater or equal the threshold. The src can be the destination (this).
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::binarize(gcgDISCRETE1D<CROSSTYPE> *src, CROSSTYPE threshold, NUMBERTYPE lessvalue, NUMBERTYPE greaterequalvalue) {
  // Check source signal
  if(src->data == NULL || src->length == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Check destination signal: destination must be compatible with the source
  if((void*) this != (void*) src)
    if(!this->createSimilar(src)) return false; // createSimilar creates only what is needed to make them compatible.

 	// Binarize
  register CROSSTYPE *srcdata  = src->data;
  register NUMBERTYPE *dstdata  = this->data;
  for(unsigned int i = 0; i < length; srcdata++, dstdata++)
    *dstdata = (NUMBERTYPE) ((*srcdata >= threshold) ? greaterequalvalue : lessvalue);

  // Binary signal computed
  //errorcode = GCG_SUCCESS;
  return true;
}


////////////////////////////////////////////////////////////
// Scales a signal: I = w * I1 + at.
// Returns true if scaled successfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::scale(gcgDISCRETE1D<CROSSTYPE> *srcsignal1, COERSIONTYPE weight1, COERSIONTYPE addthis) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(srcsignal1 == NULL) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) srcsignal1)
    if(!this->createSimilar(srcsignal1)) return false; // createSimilar creates only what is needed to make them compatible.

  // Scaling in data buffer
  register NUMBERTYPE *dstrow  = this->data;
  register CROSSTYPE *srcrow1 = srcsignal1->data;
  for(unsigned int col = 0; col < length; col++, dstrow++, srcrow1++)
    dstrow[0] = (NUMBERTYPE) (addthis + weight1 * (COERSIONTYPE) srcrow1[0]);

  // Ok, it's done
	//errorcode = GCG_SUCCESS;
	return true;
}

////////////////////////////////////////////////////////////
// Powers a signal: I = I1^power
// Returns true if scaled successfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::power(gcgDISCRETE1D<CROSSTYPE> *srcsignal1, COERSIONTYPE _power) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(srcsignal1 == NULL) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) srcsignal1)
    if(!this->createSimilar(srcsignal1)) return false; // createSimilar creates only what is needed to make them compatible.

  // Scaling in data buffer
  register NUMBERTYPE *dstrow  = this->data;
  register CROSSTYPE *srcrow1 = srcsignal1->data;
  for(unsigned int col = 0; col < length; col++, dstrow++, srcrow1++)
#if FLOATING
    dstrow[0] = (NUMBERTYPE) pow((COERSIONTYPE) srcrow1[0], _power);
#else
    dstrow[0] = (NUMBERTYPE) pow((FLOATARITHMETIC) srcrow1[0], (FLOATARITHMETIC) _power);
#endif

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}


////////////////////////////////////////////////////////////
// Combine two signals by addition: I = w1 * I1 + w2 * I2.
// Source signals must be compatible. Returns true if
// combined succesfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::combineAdd(gcgDISCRETE1D<NUMBERTYPE> *srcsignal1, gcgDISCRETE1D<CROSSTYPE> *srcsignal2, COERSIONTYPE weight1, COERSIONTYPE weight2) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(srcsignal1 == NULL || srcsignal2 == NULL) return false;
  if(!srcsignal1->isCompatibleWith(srcsignal2)) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) srcsignal1 && (void*) this != (void*) srcsignal2)
    if(!this->createSimilar(srcsignal1)) return false; // createSimilar creates only what is needed to make them compatible.

  // Combination data buffer
  register NUMBERTYPE *dstrow  = this->data;
  register NUMBERTYPE *srcrow1 = srcsignal1->data;
  register CROSSTYPE  *srcrow2 = srcsignal2->data;
  for(unsigned int col = 0; col < length; col++, dstrow++, srcrow1++, srcrow2++)
    dstrow[0] = (NUMBERTYPE) (weight1 * (COERSIONTYPE) srcrow1[0] + weight2 * (COERSIONTYPE) srcrow2[0]);

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}


////////////////////////////////////////////////////////////
// Combine two signals by multiplication: I = (a1 + I1) * (a2 + I2) + at.
// Source signals must be compatible. Returns true if
// combined succesfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::combineMult(gcgDISCRETE1D<NUMBERTYPE> *srcsignal1, gcgDISCRETE1D<CROSSTYPE> *srcsignal2, COERSIONTYPE add1, COERSIONTYPE add2) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(srcsignal1 == NULL || srcsignal2 == NULL) return false;
  if(!srcsignal1->isCompatibleWith(srcsignal2)) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) srcsignal1 && (void*) this != (void*) srcsignal2)
    if(!this->createSimilar(srcsignal1)) return false; // createSimilar creates only what is needed to make them compatible.

  // Combination in data buffer
  register NUMBERTYPE *dstrow  = this->data;
  register NUMBERTYPE *srcrow1 = srcsignal1->data;
  register CROSSTYPE *srcrow2 = srcsignal2->data;
  for(unsigned int col = 0; col < length; col++, dstrow++, srcrow1++, srcrow2++)
    dstrow[0] = (NUMBERTYPE) ((add1 + (COERSIONTYPE) srcrow1[0]) * (add2 + (COERSIONTYPE) srcrow2[0]));

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}

////////////////////////////////////////////////////////////
// Combines two signals by the L2 norm: I = sqrt(I1^2 + I2^2)
// Source signals must be compatible. Returns true if
// combined succesfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::combineNorm(gcgDISCRETE1D<NUMBERTYPE> *srcsignal1, gcgDISCRETE1D<CROSSTYPE> *srcsignal2) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(srcsignal1 == NULL || srcsignal2 == NULL) return false;
  if(!srcsignal1->isCompatibleWith(srcsignal2)) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) srcsignal1 && (void*) this != (void*) srcsignal2)
    if(!this->createSimilar(srcsignal1)) return false; // createSimilar creates only what is needed to make them compatible.

  // Combination in data buffer
  register NUMBERTYPE *dstrow  = this->data;
  register NUMBERTYPE *srcrow1 = srcsignal1->data;
  register CROSSTYPE  *srcrow2 = srcsignal2->data;
  for(unsigned int col = 0; col < length; col++, dstrow++, srcrow1++, srcrow2++)
#if FLOATING
    dstrow[0] = (NUMBERTYPE) sqrt(((COERSIONTYPE) srcrow1[0] * (COERSIONTYPE) srcrow1[0]) + ((COERSIONTYPE) srcrow2[0] * (COERSIONTYPE) srcrow2[0]));
#else
    dstrow[0] = (NUMBERTYPE) sqrt((FLOATARITHMETIC) (srcrow1[0] * srcrow1[0]) + (FLOATARITHMETIC) (srcrow2[0] * srcrow2[0]));
#endif

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////     BIDIMENSIONAL SIGNALS     /////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// Make a similar signal from the source signal. Nothing is
// copied.
///////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::createSimilar(gcgDISCRETE2D<CROSSTYPE> *source) {
  // Check source.
  //errorcode = GCG_INVALID_OBJECT;
  if(source == NULL) return false;
  if(source->data == NULL || source->width == 0 || source->height == 0) return false;

  // Structural incompatibility
  if(source->width != this->width || source->height != this->height) {
    destroySignal(); // Release previous resources

    // Copy main parameters
    this->width = source->width;
    this->height = source->height;

    // Duplicate memory for signal data
    this->data = new NUMBERTYPE [this->width * this->height];
    if(this->data == NULL) {
      //errorcode = GCG_INTERNAL_ERROR;
      return false;
    }
    this->isstaticdata = false;
  }

  this->originX = source->originX;
  this->originY = source->originY;

	// Ok, it's a similar signal.
	//errorcode = GCG_SUCCESS;
	return true;
}

////////////////////////////////////////////////////////////
// Checks if both signals are compatible: same dimensions,
// same origin, etc.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::isCompatibleWith(gcgDISCRETE2D<CROSSTYPE> *source) {
  // Check source.
  //errorcode = GCG_INVALID_OBJECT;
  if(source == NULL) return false;
  if(source->data == NULL) return false;

  // Check equivalence between both signals.
  //errorcode = GCG_SUCCESS;
  if(source->height != this->height || source->width != this->width ||
     source->originX != this->originX || source->originY != this->originY) return false;

  return true;
}

////////////////////////////////////////////////////////////
// Makes an independent copy of the source signal.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::duplicateSignal(gcgDISCRETE2D<CROSSTYPE> *source) {
  // Check source.
  //errorcode = GCG_INVALID_OBJECT;
  if(source == NULL) return false;
  if(source->data == NULL || source->width == 0 || source->height == 0) return false;

  // Check destination signal: destination must be compatible with the source
  if((void*) this == (void*) source) {
    //errorcode = GCG_SUCCESS;
    return true;
  }

  if(!this->createSimilar(source)) return false; // createSimilar creates only what is needed to make them compatible.

 	// Copy memory for signal data
 	register const unsigned int size = this->width * this->height;
 	register NUMBERTYPE *dst = this->data;
 	register CROSSTYPE  *src = source->data;
 	for(register unsigned int i = 0; i < size; i++) *(dst++) = (NUMBERTYPE) *(src++);

	// Ok, it's duplicated.
	//errorcode = GCG_SUCCESS;
	return true;
}


////////////////////////////////////////////////////////////
// Makes an independent copy of the subsignal from the source.
// Source can be the destination.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::duplicateSubsignal(gcgDISCRETE2D<CROSSTYPE> *source, int left, int top, unsigned int _width, unsigned int _height) {
  // Check source.
  //errorcode = GCG_INVALID_OBJECT;
  if(source == NULL) return false;
  if(source->data == NULL || source->width == 0 || source->height == 0) return false;

  // Check parameters
  if(_width == 0 || _height == 0) return false;

  // Check source and destination
  CROSSTYPE *olddata = source->data;
  unsigned int oldwidth  = source->width;
  unsigned int oldheight = source->height;

  //errorcode = GCG_MEMORYALLOCERROR;

  // Copy main parameters
  this->width = _width;
  this->height =_height;
  this->originX = -left;
  this->originY = -top;
  left += source->originX; // Find true beginning using origin
  top  += source->originY;

  if((void*) this != (void*) source)
    if(this->data != NULL && !this->isstaticdata) SAFE_DELETE_ARRAY(this->data);

  // Make memory for signal data
  this->data = new NUMBERTYPE [this->width * this->height];
  if(this->data == NULL) return false;

	// Compute real extents
	unsigned int xini = 0, yini = 0, xoff = 0, yoff = 0;

	if(left < 0) _width -= (xoff = abs(left));
	else xini = (unsigned int) left;

	if(top < 0) _height -= (yoff = abs(top));
	else yini = (unsigned int) top;

	if(xini + _width  > oldwidth)  _width  = oldwidth  - xini;
	if(yini + _height > oldheight) _height = oldheight - yini;

  // Copies samples in data buffer
  for(unsigned int row = 0; row < _height; row++) {
    CROSSTYPE  *possource = &olddata[xini + (oldheight - yini - row - 1) * oldwidth];
    NUMBERTYPE *posdata   = &data[xoff + (_height - row - yoff - 1) * _width];
    for(register unsigned int i = 0; i < _width; i++) *(posdata++) = (NUMBERTYPE) *(possource++);
  }

  // Release old data
  if((void*) this == (void*) source)
    if(olddata && !this->isstaticdata) SAFE_DELETE_ARRAY(olddata);

	// Ok, it's copied.
	//errorcode = GCG_SUCCESS;
	return true;
}

////////////////////////////////////////////////////////////
// Normalizes a signal in the interval [floor ceil]
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::normalize(gcgDISCRETE2D<CROSSTYPE> *src, CROSSTYPE floor, CROSSTYPE ceil) {
  // Check source signal
  if(src->data == NULL || src->width == 0 || src->height == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Check destination signal: destination must be compatible with the source
  if((void*) this != (void*) src)
    if(!this->createSimilar(src)) return false; // createSimilar creates only what is needed to make them compatible.

 	// Find minimum and maximum
 	CROSSTYPE min = CROSSTYPE_MIN, max = CROSSTYPE_MAX;
  register CROSSTYPE *srcdata  = src->data;
  unsigned int size = src->width * src->height;
	for(unsigned int current = 0; current < size; current++) {
      if(*srcdata > max) max = *srcdata;
      if(*srcdata < min) min = *srcdata;
  }

#if FLOATINGCROSS
  if(FEQUAL(min, max)) {
#else
  if(min == max) {
#endif
    min -= (CROSSTYPE) 1;
    max += (CROSSTYPE) 1;
  }

#if FLOATINGCROSS
 	// Normalize
 	CROSSTYPE inv = (CROSSTYPE) ((CROSSTYPE) (ceil - floor) / (CROSSTYPE) (max - min));
  register NUMBERTYPE *dstdata  = this->data;
  srcdata = src->data;
	for(unsigned int current = 0; current < size; current++) *dstdata = (NUMBERTYPE) (floor + (*srcdata - min) * inv);
#else
 	// Normalize
 	float inv = (float) ((float) (ceil - floor) / (float) (max - min));
  register NUMBERTYPE *dstdata  = this->data;
  srcdata = src->data;
	for(unsigned int current = 0; current < size; current++) *dstdata = (NUMBERTYPE) (floor + ((float) (*srcdata - min) * inv));
#endif

  // Normalized signal computed
  //errorcode = GCG_SUCCESS;
  return true;
}


////////////////////////////////////////////////////////////
// Applies a convolution of a mask with each signal row.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::convolutionX(gcgDISCRETE2D<NUMBERTYPE> *src, gcgDISCRETE1D<CROSSTYPE> *mask) {
  // Check parameters
  //errorcode = GCG_UNSUPPORTED_OPERATION;
  if(src == NULL || mask == NULL) return false;
  if(src->data == NULL || mask->data == NULL) return false;

  // Check destination signal: destination must be compatible with the source
  NUMBERTYPE *dstdata = NULL;
  if((void*) this != (void*) src) {
     // createSimilar creates only what is needed to make them compatible.
    if(!this->createSimilar(src)) return false;
    dstdata = this->data;
  } else {
    // We need an output buffer
    dstdata = new NUMBERTYPE [this->width * this->height];
    if(dstdata == NULL) {
      //errorcode = GCG_INTERNAL_ERROR;
      return false;
    }
  }

  // Convolution in data buffer
  NUMBERTYPE *dstrow = dstdata;
  NUMBERTYPE *srcrow = src->data;
  if(mask->extension == GCG_BORDER_EXTENSION_ZERO || src->extensionX != GCG_BORDER_EXTENSION_ZERO)
    for(unsigned int row = 0; row < height; row++, srcrow += width, dstrow += width)
      convolution1D_zero2(dstrow, src->width, srcrow, src->extensionX, mask->length, mask->data, mask->origin);
  else
    for(unsigned int row = 0; row < height; row++, srcrow += width, dstrow += width)
      convolution1D_zero1(dstrow, src->width, srcrow, mask->length, mask->data, mask->extension, mask->origin);

  // Commit filtered data
  if((void*) this == (void*) src) {
    // We have new data
    if(!this->isstaticdata) SAFE_DELETE_ARRAY(this->data);
    this->data = dstdata;
    this->isstaticdata = false;
  }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}


////////////////////////////////////////////////////////////
// Applies a convolution of a mask with each signal column.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::convolutionY(gcgDISCRETE2D<NUMBERTYPE> *src, gcgDISCRETE1D<CROSSTYPE> *mask) {
  // Check parameters
  //errorcode = GCG_UNSUPPORTED_OPERATION;
  if(src == NULL || mask == NULL) return false;
  if(src->data == NULL || mask->data == NULL) return false;

  // Check destination signal: destination must be compatible with the source
  NUMBERTYPE *dstdata = NULL;
  if((void*) this != (void*) src) {
     // createSimilar creates only what is needed to make them compatible.
    if(!this->createSimilar(src)) return false;
    dstdata = this->data;
  } else {
    // We need an output buffer
    dstdata = new NUMBERTYPE[this->width * this->height];
    if(dstdata == NULL) {
      //errorcode = GCG_INTERNAL_ERROR;
      return false;
    }
  }

  // Convolution in data buffer
  if(mask->extension == GCG_BORDER_EXTENSION_ZERO)
    switch(src->extensionY) {
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT:
              for(unsigned int row = 0; row < height; row++)
                convolutionsymmetricnorepeat2D1D_zero2(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;

      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT:
              for(unsigned int row = 0; row < height; row++)
                convolutionsymmetricrepeat2D1D_zero2(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;

       case GCG_BORDER_EXTENSION_PERIODIC:
              for(unsigned int row = 0; row < height; row++)
                convolutionperiodic2D1D_zero2(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;

       case GCG_BORDER_EXTENSION_CLAMP:
              for(unsigned int row = 0; row < height; row++)
                convolutionclamp2D1D_zero2(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;

       case GCG_BORDER_EXTENSION_ZERO:
              for(unsigned int row = 0; row < height; row++)
                convolutionzero2D1D_zero2(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;
    }
  else
    switch(src->extensionY) {
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT:
              for(unsigned int row = 0; row < height; row++)
                convolutionsymmetricnorepeat2D1D_zero1(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;

      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT:
              for(unsigned int row = 0; row < height; row++)
                convolutionsymmetricrepeat2D1D_zero1(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;

       case GCG_BORDER_EXTENSION_PERIODIC:
              for(unsigned int row = 0; row < height; row++)
                convolutionperiodic2D1D_zero1(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;

       case GCG_BORDER_EXTENSION_CLAMP:
              for(unsigned int row = 0; row < height; row++)
                convolutionclamp2D1D_zero1(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;

       case GCG_BORDER_EXTENSION_ZERO:
              for(unsigned int row = 0; row < height; row++)
                convolutionzero2D1D_zero2(&dstdata[row * width], row, src->width, src->height, src->data, mask->length, mask->data, mask->origin);
              break;
    }

  // Commit filtered data
  if((void*) this == (void*) src) {
    // We have new data
    if(!this->isstaticdata) SAFE_DELETE_ARRAY(this->data);
    this->data = dstdata;
    this->isstaticdata = false;
  }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}


////////////////////////////////////////////////////////////
// Applies a convolution of a mask with each signal column.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::convolutionXY(gcgDISCRETE2D<NUMBERTYPE> *src, gcgDISCRETE2D<CROSSTYPE> *mask) {
  // Check parameter
  //errorcode = GCG_UNSUPPORTED_OPERATION;
  if(mask == NULL) return false;

  // Check source signal
  if(src->data == NULL || src->width == 0 || src->height == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Check destination image: destination must be compatible with the source
  NUMBERTYPE *dstdata = NULL;
  if(this != src) {
     // createSimilar creates only what is needed to make them compatible.
    if(!this->createSimilar(src)) return false;
    dstdata = this->data;
  } else {
    // We need an output buffer
    dstdata = new NUMBERTYPE[this->width * this->height];
    if(dstdata == NULL) {
      //errorcode = GCG_INTERNAL_ERROR;
      return false;
    }
  }

  // Compute convolution: must be improved by the compiler
  for(int row = 0; row < (int) height; row++) {
    NUMBERTYPE *dstrow = &dstdata[row * width];
    for(int col = 0; col < (int) width; col++, dstrow++)
      dstrow[0] = (NUMBERTYPE) scalarproduct2D(col, row, src, mask);
  }

  // Commit filtered data
  if(this == src) {
    // We have new data
    if(!this->isstaticdata) SAFE_DELETE_ARRAY(this->data);
    this->data = dstdata;
    this->isstaticdata = false;
  }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}



////////////////////////////////////////////////////////////
// Performs a template matching of the mask with the signal.
// Returns true if successful. In this case, the most likely
// position of the template is stored in 'position' vector.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::templateMatching(unsigned int imgleft, unsigned int imgtop, unsigned int imgwidth, unsigned int imgheight, gcgDISCRETE2D<CROSSTYPE> *mask, int *positionX, int *positionY) {
  // Check parameter
  //errorcode = GCG_UNSUPPORTED_OPERATION;
  if(this->data == NULL || mask == NULL || imgwidth == 0 || imgheight == 0) return false;

  unsigned int iniX = mask->originX;
  unsigned int endX = width - (mask->width - mask->originX);
  unsigned int iniY = mask->originY;
  unsigned int endY = height - (mask->height - mask->originY);
  if(imgleft > iniX) iniX = imgleft;
  if(imgtop  > iniY) iniY = imgtop;
  if(imgleft + imgwidth - 1 < endX) endX = imgleft + imgwidth - 1;
  if(imgtop + imgheight - 1 < endY) endY = imgtop + imgheight - 1;

#if FLOATING
  NUMBERTYPE bestSAD = NUMBERTYPE_MAX; // Stores the smallest Sum of Absolute Differences
#else
  NUMBERTYPE bestSAD = NUMBERTYPE_MAX; // Stores the smallest Sum of Absolute Differences
#endif
  for(unsigned int row = iniY; row < endY; row++) {
    for(unsigned int col = iniX; col < endX; col++) {
      register NUMBERTYPE sum = 0;
      register int k = row - (int) mask->originY;
      for(register unsigned int posY = 0; posY < mask->height; k++, posY++) {
        register NUMBERTYPE *srcrow = &this->data[width * k];
        register CROSSTYPE *maskrow = &mask->data[posY * mask->width];
        register int l = col - (int) mask->originX;
        for(register unsigned int posX = 0; posX < mask->width; l++, posX++)
#if FLOATING || FLOATINGCROSS
          sum += (NUMBERTYPE) fabs(maskrow[posX] - srcrow[l]);
#else
          sum += (NUMBERTYPE) abs(maskrow[posX] - srcrow[l]);
#endif
      }

      // Check SAD
      if(bestSAD > sum) {
        bestSAD = sum;
        *positionX = col;
        *positionY = row;
      }
    }
  }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}

////////////////////////////////////////////////////////////
// Computes the scalar product of two signals. At least one of
// the signals must have extension GCG_BORDER_EXTENSION_ZERO for
// X, and at least one of the signals must have extension
// GCG_BORDER_EXTENSION_ZERO for Y. Aligns the origin of srcsignal2
// with the sample of this object with relative coordinates (atX, atY).
////////////////////////////////////////////////////////////
COERSIONTYPE gcgDISCRETE2D<NUMBERTYPE>::scalarProduct(int atX, int atY, gcgDISCRETE2D<CROSSTYPE> *signal2) {
  // Check parameters
  //errorcode = GCG_BADPARAMETERS;
  if(this == NULL || signal2 == NULL) return (COERSIONTYPE) 0;
  if(this->data == NULL || signal2->data == NULL) return (COERSIONTYPE) 0;

	//errorcode = GCG_SUCCESS;
	return scalarproduct2D(atX, atY, this, signal2);
}

////////////////////////////////////////////////////////////
// Produces a binary signal. The samples are mapped to: lessvalue,
// if sample is below the threshold; greaterequalvalue, if the sample
// is greater or equal the threshold. The src can be the destination (this).
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::binarize(gcgDISCRETE2D<CROSSTYPE> *src, CROSSTYPE threshold, NUMBERTYPE lessvalue, NUMBERTYPE greaterequalvalue) {
  // Check source signal
  if(src->data == NULL || src->width == 0 || src->height == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Check destination signal: destination must be compatible with the source
  if((void*) this != (void*) src)
    if(!this->createSimilar(src)) return false; // createSimilar creates only what is needed to make them compatible.

 	// Binarize
  register CROSSTYPE *srcdata  = src->data;
  register NUMBERTYPE *dstdata  = this->data;
  unsigned int size = width * height;
  for(unsigned int i = 0; i < size; srcdata++, dstdata++)
    *dstdata = ((*srcdata >= threshold) ? greaterequalvalue : lessvalue);

  // Binary signal computed
  //errorcode = GCG_SUCCESS;
  return true;
}

////////////////////////////////////////////////////////////
// Scales a signal: I = w * I1 + at.
// Returns true if scaled successfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::scale(gcgDISCRETE2D<CROSSTYPE> *src, COERSIONTYPE weight1, COERSIONTYPE addthis) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(src == NULL) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) src)
    if(!this->createSimilar(src)) return false; // createSimilar creates only what is needed to make them compatible.

  // Scaling in data buffer
  register CROSSTYPE *srcdata  = src->data;
  register NUMBERTYPE *dstdata  = this->data;
  unsigned int size = width * height;
  for(unsigned int i = 0; i < size; srcdata++, dstdata++)
    *dstdata = (NUMBERTYPE) (addthis + weight1 * (COERSIONTYPE) *srcdata);

  // Ok, it's done
	//errorcode = GCG_SUCCESS;
	return true;
}


////////////////////////////////////////////////////////////
// Powers a signal: I = I1^power
// Returns true if powered successfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::power(gcgDISCRETE2D<CROSSTYPE> *src, COERSIONTYPE _power) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(src == NULL) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) src)
    if(!this->createSimilar(src)) return false; // createSimilar creates only what is needed to make them compatible.

  // Power in data buffer
  register CROSSTYPE *srcdata  = src->data;
  register NUMBERTYPE *dstdata  = this->data;
  unsigned int size = width * height;
  for(unsigned int i = 0; i < size; srcdata++, dstdata++)
#if FLOATING
    *dstdata = (NUMBERTYPE) pow((COERSIONTYPE) *srcdata, _power);
#else
    *dstdata = (NUMBERTYPE) pow((FLOATARITHMETIC) *srcdata, (FLOATARITHMETIC) _power);
#endif

  // Ok, it's done
	//errorcode = GCG_SUCCESS;
	return true;
}


////////////////////////////////////////////////////////////
// Combine two signals by addition: I = w1 * I1 + w2 * I2.
// Source signals must be compatible. Returns true if
// combined succesfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::combineAdd(gcgDISCRETE2D<NUMBERTYPE> *srcsignal1, gcgDISCRETE2D<CROSSTYPE> *srcsignal2, COERSIONTYPE weight1, COERSIONTYPE weight2) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(srcsignal1 == NULL || srcsignal2 == NULL) return false;
  if(!srcsignal1->isCompatibleWith(srcsignal2)) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) srcsignal1 && (void*) this != (void*) srcsignal2)
    if(!this->createSimilar(srcsignal1)) return false; // createSimilar creates only what is needed to make them compatible.

  // Combination data buffer
  register NUMBERTYPE *dstrow  = this->data;
  register NUMBERTYPE *srcrow1 = srcsignal1->data;
  register CROSSTYPE  *srcrow2 = srcsignal2->data;
  unsigned int size = width * height;
  for(unsigned int i = 0; i < size; i++, dstrow++, srcrow1++, srcrow2++)
      *dstrow = (NUMBERTYPE) (weight1 * (COERSIONTYPE) *srcrow1 + (COERSIONTYPE) weight2 * *srcrow2);

  // Ok, it's done
	//errorcode = GCG_SUCCESS;
	return true;
}


////////////////////////////////////////////////////////////
// Combine two signals by multiplication: I = (a1 + I1) * (a2 + I2) + at.
// Source signals must be compatible. Returns true if
// combined succesfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::combineMult(gcgDISCRETE2D<NUMBERTYPE> *srcsignal1, gcgDISCRETE2D<CROSSTYPE> *srcsignal2, COERSIONTYPE add1, COERSIONTYPE add2) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(srcsignal1 == NULL || srcsignal2 == NULL) return false;
  if(!srcsignal1->isCompatibleWith(srcsignal2)) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) srcsignal1 && (void*) this != (void*) srcsignal2)
    if(!this->createSimilar(srcsignal1)) return false; // createSimilar creates only what is needed to make them compatible.

  // Combination in data buffer
  register NUMBERTYPE *dstrow  = this->data;
  register NUMBERTYPE *srcrow1 = srcsignal1->data;
  register CROSSTYPE *srcrow2 = srcsignal2->data;
  unsigned int size = width * height;
  for(unsigned int i = 0; i < size; i++, dstrow++, srcrow1++, srcrow2++)
      *dstrow = (NUMBERTYPE) ((add1 + (COERSIONTYPE) *srcrow1) * (add2 + (COERSIONTYPE) *srcrow2));

  // Ok, it's done
	//errorcode = GCG_SUCCESS;
	return true;
}


////////////////////////////////////////////////////////////
// Combine two signals by the L2 norm: I = sqrt(I1^2 + I2^2)
// Source signals must be compatible. Returns true if
// combined succesfully.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::combineNorm(gcgDISCRETE2D<NUMBERTYPE> *srcsignal1, gcgDISCRETE2D<CROSSTYPE> *srcsignal2) {
  // Check sources
  //errorcode = GCG_INVALID_OBJECT;
  if(srcsignal1 == NULL || srcsignal2 == NULL) return false;
  if(!srcsignal1->isCompatibleWith(srcsignal2)) return false;

  // Destination must be compatible with the source
  if((void*) this != (void*) srcsignal1 && (void*) this != (void*) srcsignal2)
    if(!this->createSimilar(srcsignal1)) return false; // createSimilar creates only what is needed to make them compatible.

  // Combination in data buffer
  register NUMBERTYPE *dstrow  = this->data;
  register NUMBERTYPE *srcrow1 = srcsignal1->data;
  register CROSSTYPE *srcrow2 = srcsignal2->data;
  unsigned int size = width * height;
  for(unsigned int i = 0; i < size; i++, dstrow++, srcrow1++, srcrow2++)
#if FLOATING
      *dstrow = (NUMBERTYPE) sqrt((COERSIONTYPE) *srcrow1 * *srcrow1 + (COERSIONTYPE) *srcrow2 * *srcrow2);
#else
      *dstrow = (NUMBERTYPE) sqrt((FLOATARITHMETIC) *srcrow1 * *srcrow1 + (FLOATARITHMETIC) *srcrow2 * *srcrow2);
#endif

  // Ok, it's done
	//errorcode = GCG_SUCCESS;
	return true;
}



#if FLOATING

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////     LEGENDRE BASIS     /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

bool gcgLEGENDREBASIS1D<NUMBERTYPE>::projectSignal(int atX, gcgDISCRETE1D<CROSSTYPE> *vector, gcgDISCRETE1D<NUMBERTYPE> *outputcoef) {
  if(npolynomials == 0 || nsamples == 0 || vector == NULL || outputcoef == NULL) return false;

  if(outputcoef->length != npolynomials) outputcoef->createSignal(npolynomials, 0, NULL, false, GCG_BORDER_EXTENSION_ZERO);

  // Used for scalar product
  gcgDISCRETE1D<NUMBERTYPE> function(nsamples, 0, &discretepolynomials[DISCRETEADDRESS1D(0, 0)], true, GCG_BORDER_EXTENSION_ZERO);

  for(unsigned int pol = 0; pol < npolynomials; pol++) {
    function.data = &discretepolynomials[DISCRETEADDRESS1D(pol, 0)];
    outputcoef->data[pol] = (NUMBERTYPE) vector->scalarProduct(atX, &function);
  }
  return true;
}

bool gcgLEGENDREBASIS1D<NUMBERTYPE>::reconstructSignal(int left, gcgDISCRETE1D<NUMBERTYPE> *inputcoef, gcgDISCRETE1D<CROSSTYPE> *outputvector) {
  if(nsamples == 0 || npolynomials == 0 || inputcoef == NULL || outputvector == NULL) return false;

  int numbercoef = (int) ((inputcoef->length > npolynomials) ? npolynomials : inputcoef->length);
  if(outputvector->data == NULL) outputvector->createSignal(this->nsamples, 0, NULL, false, outputvector->extension);

  left += outputvector->origin;

  // Check intersection
  int _length = (int) this->nsamples;
  if(left >= (int) outputvector->length || (left + _length) < 0) return true; // Nothing to do

  int inibasis = 0, inipos = left;
  if(left + _length > (int) outputvector->length)  _length = (int) outputvector->length - left;
  if(left < 0) {
    inibasis = -left;
    _length += left;
    inipos = 0;
  }
  int endpos = inipos + _length;

  memset(&outputvector->data[inipos], 0, sizeof(CROSSTYPE) * _length);

  // Compute samples
  for(int pol = 0; pol < numbercoef; pol++) {
    NUMBERTYPE *indsample = &discretepolynomials[DISCRETEADDRESS1D(pol, inibasis)];
    for(int pos = inipos; pos < endpos; pos++)
      outputvector->data[pos] += (CROSSTYPE) (inputcoef->data[pol] * *(indsample++));
  }

  return true;
}


bool gcgLEGENDREBASIS2D<NUMBERTYPE>::projectSignal(int atX, int atY, gcgDISCRETE2D<CROSSTYPE> *vector, gcgDISCRETE1D<NUMBERTYPE> *outputcoef) {
  if(npolynomials == 0 || discretepolynomials == NULL || nsamplesX == 0 || nsamplesY == 0 || vector == NULL || outputcoef == NULL) return false;

  if(outputcoef->length != npolynomials) outputcoef->createSignal(npolynomials, 0, NULL, false, GCG_BORDER_EXTENSION_ZERO);

  // Used for scalar product
  gcgDISCRETE2D<NUMBERTYPE> function(nsamplesX, nsamplesY, 0, 0, &discretepolynomials[DISCRETEADDRESS2D(0, 0, 0, 0)], true, GCG_BORDER_EXTENSION_ZERO, GCG_BORDER_EXTENSION_ZERO);

  atX += vector->originX;
  atY += vector->originY;

  for(unsigned int pj = 0, pol = 0; pj <= degree; pj++)
    for(unsigned int pi = 0; pi <= degree - pj; pi++, pol++) {
      function.data = &discretepolynomials[DISCRETEADDRESS2D(pi, pj, 0, 0)];
      outputcoef->data[pol] = (NUMBERTYPE) vector->scalarProduct(atX, atY, &function);
    }

  return true;
}


bool gcgLEGENDREBASIS2D<NUMBERTYPE>::reconstructSignal(int atX, int atY, gcgDISCRETE1D<NUMBERTYPE> *inputcoef,
                                                                         gcgDISCRETE2D<CROSSTYPE> *outputvector) {
  if(inputcoef == NULL || outputvector == NULL || nsamplesX == 0 || nsamplesY == 0 || npolynomials == 0 || discretepolynomials == NULL) return false;

  int numbercoef = (int) ((inputcoef->length > npolynomials) ? npolynomials : inputcoef->length);
  if(outputvector->data == NULL) outputvector->createSignal(this->nsamplesX, this->nsamplesY, 0, 0, NULL, false, outputvector->extensionX, outputvector->extensionY);

  atX += outputvector->originX;
  atY += outputvector->originY;

  // Check intersection
  int _width = (int) this->nsamplesX;
  if(atX >= (int) outputvector->width || (atX + (int) _width) < 0) return true; // Nothing to do

  int _height = (int) this->nsamplesY;
  if(atY >= (int) outputvector->height || (atY + (int) _height) < 0) return true; // Nothing to do

  // Adjust X coordinates
  int inibasisX = 0, iniposX = atX;
  if(atX + (int) _width > (int) outputvector->width)  _width = ((int) outputvector->width - atX);
  if(atX < 0) {
    inibasisX = -atX;
    _width -= -atX;
    iniposX = 0;
  }
  int endposX = iniposX + _width;

  // Adjust Y coordinates
  int inibasisY = 0, iniposY = atY;
  if(atY + (int) _height > (int) outputvector->height)  _height = ((int) outputvector->height - atY);
  if(atY < 0) {
    inibasisY = -atY;
    _height -= -atY;
    iniposY = 0;
  }
  int endposY = iniposY + _height;

  // Starts with zero
  for(int j = iniposY; j < endposY; j++)
    memset(&outputvector->data[j * outputvector->width + iniposX], 0, sizeof(CROSSTYPE) * _width);

  // Compute samples
  for(int pj = 0, pol = 0; pj <= (int) degree && pol < numbercoef; pj++)
    for(int pi = 0; pi <= (int) degree - pj && pol < numbercoef; pi++, pol++) {
      register NUMBERTYPE coef = inputcoef->data[pol];
      for(int cj = iniposY, cb = inibasisY; cj < endposY; cj++, cb++) {
        CROSSTYPE *v = &outputvector->data[cj * outputvector->width + iniposX];
        NUMBERTYPE *samples = &discretepolynomials[DISCRETEADDRESS2D(pi, pj, inibasisX, cb)];
        for(int ci = iniposX; ci < endposX; ci++) *(v++) += (CROSSTYPE) ((NUMBERTYPE) coef * (NUMBERTYPE) *(samples++));
      }
    }

 // Other option: but much slower in practice due to high read memory misses
 /*
 NUMBERTYPE *v = outputvector;
  unsigned int size = nsamplesX * nsamplesY;
  for(unsigned int cj = 0; cj < nsamplesY; cj++)
    for(unsigned int ci = 0; ci < nsamplesX; ci++) {
      register
      NUMBERTYPE sum = 0.0;

      NUMBERTYPE *coef = &discretepolynomials[DISCRETEADDRESS2D(0, 0, ci, cj)];
      for(unsigned int pj = 0, pol = 0; pj <= degree; pj++)
        for(unsigned int pi = 0; pi <= degree - pj; pi++, pol++, coef += size)
          sum += inputcoef[pol] * *coef;
      *(v++) = sum;
    }
*/
  return true;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     UNIDIMENSIONAL REAL WAVELETS BASIS     //////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

bool gcgDWTBASIS1D<NUMBERTYPE>::projectSignal(int atX, gcgDISCRETE1D<CROSSTYPE> *vector, gcgDISCRETE1D<NUMBERTYPE> *outputcoef) {
  if(ncoefficients == 0 || nsamples == 0 || vector == NULL || outputcoef == NULL) return false;

  // Check output coefficients
  if(outputcoef->length != ncoefficients + ncoefficients) outputcoef->createSignal(ncoefficients + ncoefficients, 0, NULL, false, GCG_BORDER_EXTENSION_ZERO);

  // Check intersection
  atX += vector->origin;
  int _length = (int) this->nsamples;
  if(atX >= (int) vector->length || atX + _length < 0) {
    memset(&outputcoef->data, 0, sizeof(NUMBERTYPE) * outputcoef->length);
    return true; // Nothing to do
  }

  int inipos = atX;
  if(atX + _length > (int) vector->length)  _length = (int) vector->length - atX;
  if(atX < 0) {
    _length += atX;
    inipos = 0;
  }

  // We need a subsignal for periodic/symmetric extension
  gcgDISCRETE1D<CROSSTYPE> subsignal;

  // Actual number of coefficients used
  int sizecoeff;

  if(_length & 0x01) {
    sizecoeff = (_length + 1) >> 1;
    subsignal.createSignal(_length + 1, 0, (CROSSTYPE*) this->buffer, true, vector->extension);
    memcpy(subsignal.data, &vector->data[inipos], sizeof(CROSSTYPE) * _length);
    subsignal.data[_length] = (CROSSTYPE) 0;
  } else {
    sizecoeff = _length >> 1;
    subsignal.createSignal(_length, 0, &vector->data[inipos], true, vector->extension);
  }

  // Projects using decomposition filters and decimates
  for(int pos = 0; pos < sizecoeff; pos++) {
    outputcoef->data[pos] = (NUMBERTYPE) subsignal.scalarProduct(pos << 1, &this->H);
    outputcoef->data[pos + this->ncoefficients] = (NUMBERTYPE) subsignal.scalarProduct(pos << 1, &this->G);
  }

  // All other coefficients are zero
  for(int pos = sizecoeff; pos < (int) this->ncoefficients; pos++)
    outputcoef->data[pos] = outputcoef->data[pos + this->ncoefficients] = (NUMBERTYPE) 0;

  return true;
}

bool gcgDWTBASIS1D<NUMBERTYPE>::reconstructSignal(int atX, gcgDISCRETE1D<NUMBERTYPE> *inputcoef, gcgDISCRETE1D<CROSSTYPE> *outputvector) {
  if(nsamples == 0 || ncoefficients == 0 || inputcoef == NULL || outputvector == NULL) return false;

  if(outputvector->data == NULL) outputvector->createSignal(this->nsamples, 0, NULL, false, outputvector->extension);
  else if(inputcoef->length != ncoefficients + ncoefficients) return false;

  // Check intersection
  atX += outputvector->origin;
  int _length = (int) this->nsamples;
  if(atX >= (int) outputvector->length || (atX + _length) < 0) return true; // Nothing to do

  int inipos = atX;
  if(atX + _length > (int) outputvector->length)  _length = (int) outputvector->length - atX;
  if(atX < 0) {
    _length += atX;
    inipos = 0;
  }

  // Actual number of coefficients used
  int sizecoeff = (_length & 0x01) ? ((_length + 1) >> 1) : (_length >> 1);

  // We need a temporary buffer, for reconstruction
  gcgDISCRETE1D<CROSSTYPE> upsampled(sizecoeff << 1, 0, (CROSSTYPE*) this->buffer, true, outputvector->extension);

  memset(upsampled.data, 0, upsampled.length * sizeof(CROSSTYPE));

  // Transport low pass coefficients with upsampling
  for(register int i = 0; i < sizecoeff; i++)
    upsampled.data[i << 1] = (CROSSTYPE) inputcoef->data[i];

  // Filter with low pass reconstruction filter
  for(register int i = inipos, pos = 0; pos < _length; i++, pos++)
    outputvector->data[i] = (CROSSTYPE) upsampled.scalarProduct(pos, &this->rH);

  // Transport high pass coefficients with upsampling
  for(register int i = 0; i < sizecoeff; i++)
    upsampled.data[i << 1] = (CROSSTYPE) inputcoef->data[i + this->ncoefficients];

  // Filter with high pass reconstruction filter.
  for(register int i = inipos, pos = 0; pos < _length; i++, pos++)
    outputvector->data[i] += (CROSSTYPE) upsampled.scalarProduct(pos, &this->rG);

  return true;
}

#endif // #if FLOATING



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//////////////////     SIGNAL PROCESSING FUNCTIONS    ///////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Computes the (optical) flow given the partial derivatives in X, Y and
// time of three channels. Uses the Augereau method (Bertrand Augereau,
// Benoît Tremblais, Christine Fernandez-Maloigne, "Vectorial computation
// of the optical flow in colorimage sequences", CIC05).
///////////////////////////////////////////////////////////////////////////////
#if FLOATINGCROSS
template<> bool gcgOpticalFlowAugereau2D<NUMBERTYPE>(gcgDISCRETE2D<NUMBERTYPE> *dx1, gcgDISCRETE2D<NUMBERTYPE> *dy1, gcgDISCRETE2D<NUMBERTYPE> *dt1,
                                                     gcgDISCRETE2D<NUMBERTYPE> *dx2, gcgDISCRETE2D<NUMBERTYPE> *dy2, gcgDISCRETE2D<NUMBERTYPE> *dt2,
                                                     gcgDISCRETE2D<NUMBERTYPE> *dx3, gcgDISCRETE2D<NUMBERTYPE> *dy3, gcgDISCRETE2D<NUMBERTYPE> *dt3,
                                                     gcgDISCRETE2D<CROSSTYPE> *outflowX, gcgDISCRETE2D<CROSSTYPE> *outflowY) {
  VECTOR3 flowchannel1, flowchannel2, flowchannel3;

  // Check input parameters
  if(dx1 == NULL || dy1 == NULL || dt1 == NULL || dx2 == NULL || dy2 == NULL ||
     dt2 == NULL || dx3 == NULL || dy3 == NULL || dt3 == NULL) return false; // Missing information

  // Check output parameters
  if(outflowX == NULL || outflowY == NULL) return false; // Nothing to do
  unsigned int width = dx1->width;
  unsigned int height = dx1->height;
  outflowX->createSignal(width, height, dx1->originX, dx1->originY);
  outflowY->createSignal(width, height, dx1->originX, dx1->originY);

  for(unsigned int j = 0, ind = 0; j < height; j++)
    for(unsigned int i = 0; i < width; i++, ind++) {
      // Primary estimation per channel: equation 6
      flowchannel1[0] = (float) ((float) dx1->data[ind] * (float) dt1->data[ind]);
      flowchannel1[1] = (float) ((float) dy1->data[ind] * (float) dt1->data[ind]);
      flowchannel1[2] = (float) ((float) -(dx1->data[ind] * (float) dx1->data[ind] + (float) dy1->data[ind] * (float) dy1->data[ind]));

      flowchannel2[0] = (float) ((float) dx2->data[ind] * (float) dt2->data[ind]);
      flowchannel2[1] = (float) ((float) dy2->data[ind] * (float) dt2->data[ind]);
      flowchannel2[2] = (float) ((float) -(dx2->data[ind] * (float) dx2->data[ind] + (float) dy2->data[ind] * (float) dy2->data[ind]));

      flowchannel3[0] = (float) ((float) dx3->data[ind] * (float) dt3->data[ind]);
      flowchannel3[1] = (float) ((float) dy3->data[ind] * (float) dt3->data[ind]);
      flowchannel3[2] = (float) ((float) -(dx3->data[ind] * (float) dx3->data[ind] + (float) dy3->data[ind] * (float) dy3->data[ind]));

      // Final, multichannel estimation

      // Power iteration to find first eigenvector
      VECTOR3 eigenvector = {0.57735026f, 0.57735026f, 0.57735026f}; // Will contain first eigenvector: starts with 1.0/sqrt(3);
#if FLOATING
      NUMBERTYPE eigenvalue = 0;    // Will contain first eigenvalue: starts with 0
#else
      float eigenvalue = 0;    // Will contain first eigenvalue: starts with 0
#endif
      bool success = true;
      for(unsigned int k = 0; k < 15 && success; k++) {
#if FLOATING
        NUMBERTYPE norm = (NUMBERTYPE) gcgDOTVECTOR3(eigenvector, eigenvector), dot1, dot2, dot3;
#else
        float norm = (float) gcgDOTVECTOR3(eigenvector, eigenvector), dot1, dot2, dot3;
#endif
        if(norm > EPSILON) {
#if FLOATING
          dot1 = (NUMBERTYPE) gcgDOTVECTOR3(flowchannel1, eigenvector);
          dot2 = (NUMBERTYPE) gcgDOTVECTOR3(flowchannel2, eigenvector);
          dot3 = (NUMBERTYPE) gcgDOTVECTOR3(flowchannel3, eigenvector);
#else
          dot1 = (float) gcgDOTVECTOR3(flowchannel1, eigenvector);
          dot2 = (float) gcgDOTVECTOR3(flowchannel2, eigenvector);
          dot3 = (float) gcgDOTVECTOR3(flowchannel3, eigenvector);
#endif
          eigenvalue = (SQR(dot1) + SQR(dot2) + SQR(dot3)) / norm;
          if(eigenvalue > EPSILON) {
            VECTOR3 temp;
            gcgSCALEVECTOR3(eigenvector, flowchannel1, (float) dot1);
            gcgSCALEVECTOR3(temp, flowchannel2, (float) dot2);
            gcgADDVECTOR3(eigenvector, eigenvector, temp);
            gcgSCALEVECTOR3(temp, flowchannel3, (float) dot3);
            gcgADDVECTOR3(eigenvector, eigenvector, temp);

            // Update
            gcgSCALEVECTOR3(eigenvector, eigenvector, (float) (1.0 / eigenvalue));
          } else success = false;
        } else success = false;
      }

      // NOTE: the code above has the same effect of the code:
      //      MATRIX3 m, mv;
      //      VECTOR3 eigenvalues;
      //      gcgTENSORPRODUCT3(m, flowchannel1, flowchannel1);
      //      gcgTENSORPRODUCT3(mv, flowchannel2, flowchannel2);
      //      gcgADDMATRIX3(m, m, mv);
      //      gcgTENSORPRODUCT3(mv, flowchannel3, flowchannel3);
      //      gcgADDMATRIX3(m, m, mv);
      //      gcgEigenSymmetricMatrix3(m, mv, eigenvalues);
      //      gcgCOPYVECTOR3(eigenvector, &mv[6]);
      //      eigenvalue = eigenvalues[2];

      if(success && fabs(eigenvector[2]) > EPSILON) {
        outflowX->data[ind] = (CROSSTYPE) (eigenvector[0] / eigenvector[2]);
        outflowY->data[ind] = (CROSSTYPE) (eigenvector[1] / eigenvector[2]);
      } else outflowX->data[ind] = outflowY->data[ind] = (CROSSTYPE) 0.0;
    }

  // Optical flow computed
  return true;
}
#endif // #if FLOATINGCROSS


#endif // #if defined(CROSSTYPE)
