/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   SIGNALMAIN.CPP: functions and methods for signal processing with multiple types.

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
#elif TYPECOUNTER == 2
  #undef TYPECOUNTER
  #define TYPECOUNTER 3
  #define NUMBERTYPE short
  #define NUMBERTYPE_MIN SHRT_MIN
  #define NUMBERTYPE_MAX SHRT_MAX
#elif TYPECOUNTER == 3
  #undef TYPECOUNTER
  #define TYPECOUNTER 4
  #define NUMBERTYPE int
  #define NUMBERTYPE_MIN INT_MIN
  #define NUMBERTYPE_MAX INT_MAX
#elif TYPECOUNTER == 4
  #undef TYPECOUNTER
  #define TYPECOUNTER 5
  #define NUMBERTYPE long
  #define NUMBERTYPE_MIN LONG_MIN
  #define NUMBERTYPE_MAX LONG_MAX
#endif

#define FLOATING TYPECOUNTER < 3


#ifndef WIDTHBYTES
  #define WIDTHBYTES(bits) ((DWORD)(((bits)+31) & (~31)) / 8)
#endif

#ifndef BITMAPWIDTHBYTES
  #define BITMAPWIDTHBYTES(iwidth, ibpp) (DWORD)WIDTHBYTES((DWORD) iwidth * (DWORD) ibpp)
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////     DISCRETE SIGNALS      //////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//////////     RAW SCALAR PRODUCT AND CONVOLUTION FUNCTIONS     /////////////
/////////////////////////////////////////////////////////////////////////////

static inline NUMBERTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, NUMBERTYPE *data) {
  if(i < 0 || i >= width)
    switch(extensionX) {
      case GCG_BORDER_EXTENSION_CLAMP: i = (i <= 0) ? 0 : width - 1;
                                       break;
      case GCG_BORDER_EXTENSION_PERIODIC: i = (width == 1) ? 0 : (i >= 0) ? i % width : width - 1 - ((-i - 1) % width);
                                          break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT: i = (width == 1) ? 0 : (i >= 0) ? (i % (width + width - 2)) : (-i % (width + width - 2));
                                                    if(i >= (int) width) i = width + width - i - 2;
                                                    break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT: i = (width == 1) ? 0 : (i >= 0) ? (i % (width + width)) : (-(i + 1) % (width + width));
                                                  if(i >= (int) width) i = width + width - i - 1;
                                                  break;
      default: return (NUMBERTYPE) 0;
    }

  if(j < 0 || j >= height)
    switch(extensionY) {
      case GCG_BORDER_EXTENSION_CLAMP: j = (j <= 0) ? 0 : height - 1;
                                       break;
      case GCG_BORDER_EXTENSION_PERIODIC: j = (height == 1) ? 0 : (j >= 0) ? j % height : height - 1 - ((-j - 1) % height);
                                          break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT: j = (height == 1) ? 0 : (j >= 0) ? (j % (height + height - 2)) : (-j % (height + height - 2));
                                                    if(j >= (int) height) j = height + height - j - 2;
                                                    break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT: j = (height == 1) ? 0 : (j >= 0) ? (j % (height + height)) : (-(j + 1) % (height + height));
                                                  if(j >= (int) height) j = height + height - j - 1;
                                                  break;
      default: return (NUMBERTYPE) 0;
    }

  return data[j * width + i];
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////     UNIDIMENSIONAL SIGNALS     ////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


gcgDISCRETE1D<NUMBERTYPE>::gcgDISCRETE1D() {
  	this->data = NULL;
  	this->length = 0;
  	this->origin = 0;
  	this->isstaticdata = false;
  	this->extension = GCG_BORDER_EXTENSION_CLAMP;
}

////////////////////////////////////////////////////////////
// Construction by calling createSignal()
////////////////////////////////////////////////////////////
gcgDISCRETE1D<NUMBERTYPE>::gcgDISCRETE1D(unsigned int ilength, int _origin, NUMBERTYPE *sampledata, bool _isstaticdata, int _border) {
  this->data = NULL;
  this->isstaticdata = false;
  createSignal(ilength, _origin, sampledata, _isstaticdata, _border);
}

////////////////////////////////////////////////////////////
// Object destruction
////////////////////////////////////////////////////////////
gcgDISCRETE1D<NUMBERTYPE>::~gcgDISCRETE1D() {
  destroySignal();
}

////////////////////////////////////////////////////////////
// Signal creation. Alocates all necessary space for the signal
// indicated by parameters.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::createSignal(unsigned int ilength, int _origin, NUMBERTYPE *sampledata, bool _isstaticdata, int border) {
  // Check the parameters
  if(ilength == 0) {
    //errorcode = GCG_BADPARAMETERS;
    return false;
  }

  if(_isstaticdata && sampledata != NULL) {
    destroySignal();
    this->data = sampledata;
    this->isstaticdata = true;
  } else {
    // Reuse previous space?
    if(this->data == NULL || this->length != ilength || this->isstaticdata) {
      // Release previous resources
      destroySignal();

      // Create memory for signal data
      this->data = new NUMBERTYPE[ilength];
      //errorcode = GCG_MEMORYALLOCERROR;
      if(this->data == NULL) return false;
      this->isstaticdata = false;
    }

    if(sampledata == NULL) memset(this->data, 0, sizeof(NUMBERTYPE) * ilength);
    else memcpy(this->data, sampledata, sizeof(NUMBERTYPE) * ilength);
  }


	// Signal information
	this->length = ilength;
	this->origin = _origin;
	this->extension = border;

  // Signal created
  //errorcode = GCG_SUCCESS;
  return true;
}


////////////////////////////////////////////////////////////
// Release all allocated resources for this signal
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::destroySignal() {
  if(!this->isstaticdata) SAFE_DELETE_ARRAY(this->data);
  length = 0;
  origin = 0;
  //errorcode = GCG_SUCCESS;
  this->isstaticdata = false;
 	data = NULL;

  return true;
}


////////////////////////////////////////////////////////////
// Get the sample at position i.
////////////////////////////////////////////////////////////
NUMBERTYPE gcgDISCRETE1D<NUMBERTYPE>::getDataSample(int i) {
 // This signal is valid?
  if(data == NULL || length == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return 0;
  }

  // Find true coordinate using origin
  i += origin;

  //errorcode = GCG_SUCCESS;
 if(i < 0 || i >= (int) this->length)
    switch(this->extension) {
      case GCG_BORDER_EXTENSION_CLAMP: i = (i <= 0) ? 0 : this->length - 1;
                                       break;
      case GCG_BORDER_EXTENSION_PERIODIC: i = (this->length == 1) ? 0 : (i >= 0) ? i % this->length : this->length - 1 - ((-i - 1) % this->length);
                                          break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT: i = (this->length == 1) ? 0 : (i >= 0) ? (i % (this->length + this->length - 2)) : (-i % (this->length + this->length - 2));
                                                    if(i >= (int) this->length) i = this->length + this->length - i - 2;
                                                    break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT: i = (this->length == 1) ? 0 : (i >= 0) ? (i % (this->length + this->length)) : (-(i + 1) % (this->length + this->length));
                                                  if(i >= (int) this->length) i = this->length + this->length - i - 1;
                                                  break;
      default: return (NUMBERTYPE) 0;
    }

  // Ok, got a valid index
  return this->data[i];
}


////////////////////////////////////////////////////////////
// Set the sample value at position i.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::setDataSample(int i, NUMBERTYPE value) {
  // This signal is valid?
  if(data == NULL || length == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Find true coordinate using origin
  i += origin;

  //errorcode = GCG_SUCCESS;
  if(i < 0 || i >= (int) this->length)
    switch(this->extension) {
      case GCG_BORDER_EXTENSION_CLAMP: i = (i <= 0) ? 0 : this->length - 1;
                                       break;
      case GCG_BORDER_EXTENSION_PERIODIC: i = (this->length == 1) ? 0 : (i >= 0) ? i % this->length : this->length - 1 - ((-i - 1) % this->length);
                                          break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT: i = (this->length == 1) ? 0 : (i >= 0) ? (i % (this->length + this->length - 2)) : (-i % (this->length + this->length - 2));
                                                    if(i >= (int) this->length) i = this->length + this->length - i - 2;
                                                    break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT: i = (this->length == 1) ? 0 : (i >= 0) ? (i % (this->length + this->length)) : (-(i + 1) % (this->length + this->length));
                                                  if(i >= (int) this->length) i = this->length + this->length - i - 1;
                                                  break;
      default: return false;
    }

  // Ok, the index is valid
  data[i] = value;

  //errorcode = GCG_SUCCESS;
  return true;
}


////////////////////////////////////////////////////////////
// Creates a gray scale 1D image with this signal.
////////////////////////////////////////////////////////////
bool gcgDISCRETE1D<NUMBERTYPE>::exportGrayScaleImage(gcgIMAGE *outimage, bool normalize) {
  // Check signal
  if(data == NULL || length == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  if(outimage == NULL) {
    //errorcode = GCG_BADPARAMETERS;
    return false;
  }

  // Create output image: destination must be compatible with the source
  if(outimage->width != this->length || outimage->height != 1 || outimage->bpp != 8)
    outimage->createImage(this->length, 1, 8);
  else outimage->forceLinearPalette();

 	// Find minimum and maximum
#if FLOATING
 	NUMBERTYPE min = NUMBERTYPE_MIN, max = NUMBERTYPE_MAX;
#else
  NUMBERTYPE min = NUMBERTYPE_MIN, max = NUMBERTYPE_MAX;
#endif
  if(normalize) {
    register NUMBERTYPE *srcdata  = this->data;
    for(unsigned int current = 0; current < this->length; current++, srcdata++) {
        if(*srcdata > max) max = *srcdata;
        if(*srcdata < min) min = *srcdata;
    }

#if FLOATING
    if(FEQUAL(min, max)) {
#else
    if(min == max) {
#endif
      min -= (NUMBERTYPE) 1;
      max += (NUMBERTYPE) 1;
    }

#if FLOATING
    // Normalized version
    NUMBERTYPE inv = (NUMBERTYPE) ((NUMBERTYPE) 255 / (NUMBERTYPE) (max - min));
    srcdata = this->data;
    unsigned char *row = outimage->data;
    for(unsigned int current = 0; current < this->length; current++, row++, srcdata++) {
      int v = (int) ((NUMBERTYPE) (*srcdata - min) * inv);
      *row = (v < 0) ? 0 : ((v > 255) ? 255 : (unsigned char) v);
    }
#else
    // Normalized version
    float inv = (float) ((float) 255 / (float) (max - min));
    srcdata = this->data;
    unsigned char *row = outimage->data;
    for(unsigned int current = 0; current < this->length; current++, row++, srcdata++) {
      int v = (int) ((float) (*srcdata - min) * inv);
      *row = (v < 0) ? 0 : ((v > 255) ? 255 : (unsigned char) v);
    }
#endif

  } else {
#if FLOATING
    // No normalization needed
    register NUMBERTYPE *srcdata = this->data;
    unsigned char *row = outimage->data;
    for(unsigned int current = 0; current < this->length; current++, row++, srcdata++) {
      int v = (int) (*srcdata * (NUMBERTYPE) 255);
      *row = (v < 0) ? 0 : ((v > 255) ? 255 : (unsigned char) v);
    }
#else
    // No normalization needed
    register NUMBERTYPE *srcdata = this->data;
    unsigned char *row = outimage->data;
    for(unsigned int current = 0; current < this->length; current++, row++, srcdata++) {
      int v = (int) *srcdata;
      *row = (v < 0) ? 0 : ((v > 255) ? 255 : (unsigned char) v);
    }
#endif
  }

  // Normalized signal computed
  //errorcode = GCG_SUCCESS;
  return true;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////     BIDIMENSIONAL SIGNALS     /////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// Empty gcgDISCRETE2D construction.
////////////////////////////////////////////////////////////
gcgDISCRETE2D<NUMBERTYPE>::gcgDISCRETE2D() {
  	this->data = NULL;
  	this->width = 0;
  	this->height = 0;
  	this->originX = 0;
  	this->originY = 0;
  	this->isstaticdata = false;
  	this->extensionX = GCG_BORDER_EXTENSION_CLAMP;
  	this->extensionY = GCG_BORDER_EXTENSION_CLAMP;
}

////////////////////////////////////////////////////////////
// Construction by calling createSignal()
////////////////////////////////////////////////////////////
gcgDISCRETE2D<NUMBERTYPE>::gcgDISCRETE2D(unsigned int iwidth, unsigned int iheight, int _originX, int _originY, NUMBERTYPE *sampledata, bool _isstaticdata, int borderextX, int borderextY) {
  this->data = NULL;
  this->width = 0;
  this->height = 0;
  this->originX = 0;
  this->originY = 0;
  this->isstaticdata = false;
  this->extensionX = GCG_BORDER_EXTENSION_CLAMP;
  this->extensionY = GCG_BORDER_EXTENSION_CLAMP;
  this->createSignal(iwidth, iheight, _originX, _originY, sampledata, _isstaticdata, borderextX, borderextY);
}


////////////////////////////////////////////////////////////
// Object destruction
////////////////////////////////////////////////////////////
gcgDISCRETE2D<NUMBERTYPE>::~gcgDISCRETE2D() {
  destroySignal();
}

////////////////////////////////////////////////////////////
// Signal creation. Alocates all necessary space for the signal
// indicated by parameters.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::createSignal(unsigned int iwidth, unsigned int iheight, int _originX, int _originY, NUMBERTYPE *sampledata, bool _isstaticdata, int borderextX, int borderextY) {
  // Check the parameters
  if(iheight == 0 || iwidth < 1) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_SIGNAL, GCG_UNSUPPORTEDFORMAT), "createSignal(): unsupported dimensions (%d, %d). (%s:%d)", iwidth, iheight, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(_isstaticdata && sampledata != NULL) {
    destroySignal();
    this->data = sampledata;
    this->isstaticdata = true;
  } else {
    // Reuse previous space
    if(this->data == NULL || this->width != iwidth || this->height != iheight || this->isstaticdata) {
      // Release previous resources
      destroySignal();

      // Create memory for signal data
      this->data = new NUMBERTYPE[iwidth * iheight];
      if(this->data == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "createSignal(): signal data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
      }
      this->isstaticdata = false;
    }

    if(sampledata == NULL) memset(this->data, 0, sizeof(NUMBERTYPE) * iwidth * iheight);
    else memcpy(this->data, sampledata, sizeof(NUMBERTYPE) * iwidth * iheight);
  }

	// Signal information
	this->originX = _originX;
	this->originY = _originY;
  this->width   = iwidth;
  this->height  = iheight;
  this->extensionX = borderextX;
  this->extensionY = borderextY;

  // Signal created
  return true;
}


////////////////////////////////////////////////////////////
// Release all allocated resources for this signal
////////////////////////////////////////////////////////////
void gcgDISCRETE2D<NUMBERTYPE>::destroySignal() {
  if(!this->isstaticdata) SAFE_DELETE_ARRAY(data);
  this->width = 0;
  this->height = 0;
 	this->data = NULL;
 	this->originX = 0;
 	this->originY = 0;
 	this->isstaticdata = false;
}


////////////////////////////////////////////////////////////
// Get the sample value at position (i,j).
////////////////////////////////////////////////////////////
NUMBERTYPE gcgDISCRETE2D<NUMBERTYPE>::getDataSample(int i, int j) {
 // This signal is valid?
  if(data == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_SIGNAL, GCG_INVALIDOBJECT), "getDataSample(): invalid signal. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return 0;
  }

  // Find true coordinate using origin
  return sample2D(i + originX, j + originY, this->width, this->height, this->extensionX, this->extensionY, this->data);
}


////////////////////////////////////////////////////////////
// Set the sample value at position (i,j).
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::setDataSample(int i, int j, NUMBERTYPE value) {
  // This signal is valid?
  if(data == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_SIGNAL, GCG_INVALIDOBJECT), "setDataSample(): invalid signal. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Find true coordinate using origin
  i += originX;
  j += originY;

  if(i < 0 || i >= (int) this->width)
    switch(this->extensionX) {
      case GCG_BORDER_EXTENSION_CLAMP: i = (i <= 0) ? 0 : this->width - 1;
                                       break;
      case GCG_BORDER_EXTENSION_PERIODIC: i = (this->width == 1) ? 0 : (i >= 0) ? i % this->width : this->width - 1 - ((-i - 1) % this->width);
                                          break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT: i = (this->width == 1) ? 0 : (i >= 0) ? (i % (this->width + this->width - 2)) : (-i % (this->width + this->width - 2));
                                                    if(i >= (int) this->width) i = this->width + this->width - i - 2;
                                                    break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT: i = (this->width == 1) ? 0 : (i >= 0) ? (i % (this->width + this->width)) : (-(i + 1) % (this->width + this->width));
                                                  if(i >= (int) this->width) i = this->width + this->width - i - 1;
                                                  break;
      default: gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_SIGNAL, GCG_INCOMPATIBILITYERROR), "setDataSample(): bad extension mode. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
               return false;
    }

  if(j < 0 || j >= (int) this->height)
    switch(this->extensionY) {
      case GCG_BORDER_EXTENSION_CLAMP: j = (j <= 0) ? 0 : this->height - 1;
                                       break;
      case GCG_BORDER_EXTENSION_PERIODIC: j = (this->height == 1) ? 0 : (j >= 0) ? j % this->height : this->height - 1 - ((-j - 1) % this->height);
                                          break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT: j = (this->height == 1) ? 0 : (j >= 0) ? (j % (this->height + this->height - 2)) : (-j % (this->height + this->height - 2));
                                                    if(j >= (int) this->height) j = this->height + this->height - j - 2;
                                                    break;
      case GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT: j = (this->height == 1) ? 0 : (j >= 0) ? (j % (this->height + this->height)) : (-(j + 1) % (this->height + this->height));
                                                  if(j >= (int) this->height) j = this->height + this->height - j - 1;
                                                  break;
      default:  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_SIGNAL, GCG_INCOMPATIBILITYERROR), "setDataSample(): bad extension mode. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
                return false;
    }

  // Ok, the index is valid
  data[j * this->width + i] = value;
  return true;
}


////////////////////////////////////////////////////////////
// Flips the signal vertically
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::verticalFlip() {
  // This image is valid?
  if(data == NULL || width == 0 || height == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

	int nrows = (height % 2 == 1) ? ((height-1) >> 1) : (height >> 1);

	//create space for a temporary row
	NUMBERTYPE *tmp = new NUMBERTYPE[width];
	//errorcode = GCG_MEMORYALLOCERROR;
	if(!tmp) return false;

 	// Swaping rows
	for(int i = 0; i < nrows; i++) {
		memcpy(tmp, &data[i * width], width * sizeof(NUMBERTYPE));
		memcpy(&data[i * width], &data[(height-i-1) * width], width * sizeof(NUMBERTYPE));
		memcpy(&data[(height-i-1) * width], tmp, width * sizeof(NUMBERTYPE));
	}

	// Origin has changed
	this->originY = -this->originY;

	SAFE_DELETE_ARRAY(tmp);

	//errorcode = GCG_SUCCESS;
	return true;
}


////////////////////////////////////////////////////////////
// Creates a gray scale 2D image with this signal.
////////////////////////////////////////////////////////////
bool gcgDISCRETE2D<NUMBERTYPE>::exportGrayScaleImage(gcgIMAGE *outimage, bool normalizing) {
  // Check signal
  if(data == NULL || width == 0 || height == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  if(outimage == NULL) {
    //errorcode = GCG_BADPARAMETERS;
    return false;
  }

  // Create output image: destination must be compatible with the source
  if(outimage->width != this->width || outimage->height != this->height || outimage->bpp != 8)
    outimage->createImage(this->width, this->height, 8);
  else outimage->forceLinearPalette();

  if(normalizing) {
    // Find minimum and maximum
    NUMBERTYPE min = NUMBERTYPE_MIN, max = NUMBERTYPE_MAX;
    register NUMBERTYPE *srcdata  = this->data;
    unsigned int size = this->width * this->height;
    for(unsigned int current = 0; current < size; current++, srcdata++) {
        if(*srcdata > max) max = *srcdata;
        if(*srcdata < min) min = *srcdata;
    }

#if FLOATING
    if(FEQUAL(min, max)) {
#else
    if(min == max) {
#endif
      min -= (NUMBERTYPE) 1;
      max += (NUMBERTYPE) 1;
    }

#if FLOATING
    // Normalized version
    NUMBERTYPE inv = (NUMBERTYPE) ((NUMBERTYPE) 255 / (NUMBERTYPE) (max - min));
    srcdata = this->data;
    for(unsigned int i = 0; i < this->height; i++) {
      unsigned char *row = &outimage->data[outimage->rowsize * i];
      for(unsigned int current = 0; current < this->width; current++, row++, srcdata++) {
        int v = (int) (((NUMBERTYPE) *srcdata - min) * inv);
        *row = (v < 0) ? 0 : ((v >= 255) ? (unsigned char) 255 : (unsigned char) v);
      }
    }
#else
    // Normalized version
    float inv = (float) ((float) 255 / (float) (max - min));
    srcdata = this->data;
    for(unsigned int i = 0; i < this->height; i++) {
      unsigned char *row = &outimage->data[outimage->rowsize * i];
      for(unsigned int current = 0; current < this->width; current++, row++, srcdata++) {
        int v = (int) (((float) *srcdata - (float) min) * inv);
        *row = (v < 0) ? 0 : ((v >= 255) ? (unsigned char) 255 : (unsigned char) v);
      }
    }
#endif

  } else {

#if FLOATING
    // No normalization needed
    register NUMBERTYPE *srcdata = this->data;
    for(unsigned int i = 0; i < this->height; i++) {
      unsigned char *row = &outimage->data[outimage->rowsize * i];
      for(unsigned int current = 0; current < this->width; current++, row++, srcdata++) {
        int v = (int) (*srcdata * (NUMBERTYPE) 255);
        *row = (v < 0) ? 0 : ((v > 255) ? 255 : (unsigned char) v);
      }
    }
#else
    // No normalization needed
    register NUMBERTYPE *srcdata = this->data;
    for(unsigned int i = 0; i < this->height; i++) {
      unsigned char *row = &outimage->data[outimage->rowsize * i];
      for(unsigned int current = 0; current < this->width; current++, row++, srcdata++) {
        int v = (int) *srcdata;
        *row = (v < 0) ? 0 : ((v > 255) ? 255 : (unsigned char) v);
      }
    }
#endif

  }

  // Normalized signal computed
  //errorcode = GCG_SUCCESS;
  return true;
}


#if FLOATING

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////     UNIDIMENSIONAL LEGENDRE POLYNOMIAL BASIS   ////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// UNIDIMENSIONAL ADDRESSING
////////////////////////////////////////////////////////////////////////////////

// Polynomial adress using triangular matrix: small memory requirement
#define LEGENDREADDRESS1D(_pi, _c) ((((_pi) * ((_pi) + 1)) >> 1) + (_c))
#define LEGENDRESIZE1D(_np)        (((_np) * ((_np) + 1)) >> 1)

// Adress of coefficents inside polynomial vector
#define DISCRETEADDRESS1D(_pi, _c) ((_pi) * this->nsamples + (_c))
#define DISCRETESIZE1D(_ns)        ((this->npolynomials) * (_ns))

////////////////////////////////////////////////////////////////////////////////
// BIDIMENSIONAL ADDRESSING
////////////////////////////////////////////////////////////////////////////////

// Polynomial adress using triangular matrix: exact memory requirement
#define POLYNOMIALADDRESS2D(_pi, _pj)  ( (((_pj) * ((this->degree + 1)+(this->degree + 1) - (_pj) + 1)) >> 1) + (_pi) )
#define POLYNOMIALSIZE2D(_nd)          (((((_nd) + 1) + 1) * (((_nd) + 1) + 2)  )  >> 1)

// Coefficient address using square matrix: approximative but small memory requirement
#define COEFFICIENTADDRESS2D(_pi, _pj, _ci, _cj) ((_cj) * ((_pi) + 1) + (_ci))
#define COEFFICIENTSIZE2D(_nd)          (((_nd) & 0x01) ? ((((_nd) + 1) >> 1) * ((((_nd) + 1) >> 1) + 1)) : ((((_nd) >> 1) + 1) * (((_nd) >> 1) + 1)))

// Final polynomial address
#define LEGENDREADDRESS2D(_pi, _pj, _ci, _cj) ( POLYNOMIALADDRESS2D(_pi, _pj) * COEFFICIENTSIZE2D(this->degree+1)  \
                                                 + COEFFICIENTADDRESS2D(_pi, _pj, _ci, _cj) )

#define LEGENDRESIZE2D(_nd)   (POLYNOMIALSIZE2D(_nd) * COEFFICIENTSIZE2D(_nd))


// Adress of discrete coefficients inside polynomial vector
#define DISCRETEADDRESS2D(_pi, _pj, _ci, _cj) ( POLYNOMIALADDRESS2D(_pi, _pj) * (this->nsamplesX * this->nsamplesY) \
                                                 + ((_cj) * this->nsamplesX + (_ci)) )

#define DISCRETESIZE2D(_nsX, _nsY)  ((_nsX) * (_nsY))


gcgLEGENDREBASIS1D<NUMBERTYPE>::gcgLEGENDREBASIS1D() {
  npolynomials = 0;
  coefficients = NULL;
  discretepolynomials = NULL;
  nsamples = 0;
}

gcgLEGENDREBASIS1D<NUMBERTYPE>::~gcgLEGENDREBASIS1D() {
  SAFE_FREE(coefficients);
  SAFE_FREE(discretepolynomials);
}

bool gcgLEGENDREBASIS1D<NUMBERTYPE>::setBasisDegree(unsigned int degree) {
  // Allocate memory space
  if(degree != this->degree) {
    SAFE_FREE(coefficients);
    this->npolynomials = 0;
    this->degree = 0;
    coefficients = (double*) ALLOC(sizeof(double) * LEGENDRESIZE1D(degree + 1));
    if(coefficients == NULL) return false;
    this->degree = degree;
    this->npolynomials = degree + 1;
    SAFE_FREE(discretepolynomials);
    discretepolynomials = NULL;
    this->nsamples = 0;

    // Init coefficients
    memset(coefficients, 0, sizeof(double) * LEGENDRESIZE1D(npolynomials));
    coefficients[LEGENDREADDRESS1D(0, 0)] = 1.0; // First coefficient of first polynomial is 1.0, all others are zero

    // Now apply recursive formula: Pk = ((2k - 1) X Pk-1 - (k - 1) Pk-2) / k
    for(unsigned int pol = 0; pol < npolynomials - 1; pol++) {
      // Compute coefficients based on previous coefficients
      for(unsigned int coef = 0; coef <= pol + 1; coef++)
        coefficients[LEGENDREADDRESS1D((pol+1), coef)] = ((double) pol + pol + 1) * ((coef > 0) ? coefficients[LEGENDREADDRESS1D(pol, (coef-1))] : 0.0)
                                                         - ((double) pol) * ((pol > 0 && coef < pol) ? coefficients[LEGENDREADDRESS1D((pol-1), coef)] : 0.0);

      // Divide by the polynomial number
      double inv = 1.0 / (pol + 1);
      for(unsigned int coef = 0; coef <= (pol + 1); coef++) coefficients[LEGENDREADDRESS1D(pol + 1, coef)] *= inv;
    }
  }
  return true;
}

unsigned int gcgLEGENDREBASIS1D<NUMBERTYPE>::getNumberOfCoefficients() {
  return npolynomials;
}

bool gcgLEGENDREBASIS1D<NUMBERTYPE>::setNumberOfSamples(unsigned int n) {
  if(npolynomials == 0) return false;

  if(n != this->nsamples || discretepolynomials == NULL) {
    SAFE_FREE(discretepolynomials);

    // Make room for samples
    this->nsamples = 0;
    discretepolynomials = (NUMBERTYPE*) ALLOC(sizeof(NUMBERTYPE) * DISCRETESIZE1D(n));
    if(discretepolynomials == NULL) return 0.0;
    this->nsamples = n;

    memset(discretepolynomials, 0, sizeof(NUMBERTYPE) * DISCRETESIZE1D(n));

    // Compute samples
    NUMBERTYPE dx = (NUMBERTYPE) (2.0 / (double) n);
    for(unsigned int pol = 0; pol < npolynomials; pol++) {
      NUMBERTYPE norm = (NUMBERTYPE) 0.0;
      NUMBERTYPE base = (NUMBERTYPE) -1.0;
      NUMBERTYPE *indsample = &discretepolynomials[DISCRETEADDRESS1D(pol, 0)];
      for(unsigned int c = 0; c < n; c++, base += dx) {
        NUMBERTYPE q = (*(indsample++) = getIntegralValue(pol, base, base + dx));
        norm += q * q;
      }
      // Tend to be expected L2 norm: 2.0 / ((2.0 * pol) + 1.0), see http://www.answers.com/topic/legendre-polynomials
#if FLOATING
      norm = (NUMBERTYPE) (1.0 / (NUMBERTYPE) sqrt(norm));
#else
      norm = (NUMBERTYPE) (1.0 / (NUMBERTYPE) sqrt((float) norm));
#endif
      // Normalization after discretization
      indsample = &discretepolynomials[DISCRETEADDRESS1D(pol, 0)];
      for(unsigned int c = 0; c < n; c++) *(indsample++) *= norm;
    }
  }

  return true;
}


NUMBERTYPE gcgLEGENDREBASIS1D<NUMBERTYPE>::getPointValue(unsigned int deg, NUMBERTYPE x) {
  if(deg > npolynomials) return (NUMBERTYPE) 0.0;

  // We must use double precision because polynomial coefficients of higher degrees are too big
  double sum = 0.0;
  double power = 1.0;
  double *indcoef = &coefficients[LEGENDREADDRESS1D(deg, 0)];
  for(unsigned int i = 0; i <= deg; i++) {
    sum += *(indcoef++) * power;
    power *= x;
  }
  return (NUMBERTYPE) sum;
}

NUMBERTYPE gcgLEGENDREBASIS1D<NUMBERTYPE>::getIntegralValue(unsigned int deg, NUMBERTYPE a, NUMBERTYPE b) {
  if(deg > npolynomials) return (NUMBERTYPE) 0.0;

  // We must use double precision because polynomial coefficients of higher degrees are too big
  double sumA = 0.0, sumB = 0.0;
  double powerA = a, powerB = b;    // Shifted coefficients for integration: index 0 means degree 1 and so on
  double *indcoef = &coefficients[LEGENDREADDRESS1D(deg, 0)];
  for(unsigned int i = 0; i <= deg; i++) {
    double coef = (double) *(indcoef++) / ((double) (i + 1)); // Get integral coefficients of the polynomial
    sumA += coef * powerA;
    sumB += coef * powerB;
    powerA *= a;
    powerB *= b;
  }

  return (NUMBERTYPE) (sumB - sumA);
}

NUMBERTYPE gcgLEGENDREBASIS1D<NUMBERTYPE>::getDiscreteValue(unsigned int deg, unsigned int i) {
  if(deg > npolynomials || i >= nsamples || discretepolynomials == NULL) return (NUMBERTYPE) 0.0;

  // Return pre-computed value
  return discretepolynomials[DISCRETEADDRESS1D(deg, i)];
}

/*
// Computes the integral of the polynomial pol in the interval [a, b].
// Uses ten points using Gaussian-Legendre quadrature integration.
static NUMBERTYPE abcissas[5] = {0.1488743389816312108848260,0.4333953941292471907992659,0.6794095682990244062343274,0.8650633666889845107320967,0.9739065285171717200779640};
static NUMBERTYPE weights[5] = {0.2955242247147528701738930,0.2692667193099963550912269,0.2190863625159820439955349,0.1494513491505805931457763,0.0666713443086881375935688};

NUMBERTYPE gcgLEGENDREBASIS1D::getIntegralValue(unsigned int pol, NUMBERTYPE a, NUMBERTYPE b) {
  NUMBERTYPE xr, xm, dx, sum;
  xm = 0.5 * (b + a);
  xr = 0.5 * (b - a);
  sum = 0;
  for(int j = 0; j < 5; j++) {
    dx = xr * abcissas[j];
    sum += weights[j] * (getContinuousValue(pol, xm + dx) + getContinuousValue(pol, xm - dx));
  }
  return sum * xr;
}*/


bool gcgLEGENDREBASIS1D<NUMBERTYPE>::basisInformation(const char *filename) {
  if(npolynomials == 0) return false;

  FILE *file = fopen(filename, "wt");

  // Polynomial coefficients
  if(file == NULL) return false;
  fprintf(file, "BASIS OF DEGREE: %d\n", degree);
  fprintf(file, "POLYNOMIAL COEFFICIENTS: %d\n", npolynomials);
  for(unsigned int pol = 0; pol < npolynomials; pol++) {
    fprintf(file, "\nP%d:", pol);
    for(unsigned int coef = 0; coef <= pol; coef++) fprintf(file, " %4.18f", coefficients[LEGENDREADDRESS1D(pol, coef)]);
  }

  // Integral coefficients
  fprintf(file, "\n\nINTEGRALS OF DEGREE: %d\n", npolynomials);
  fprintf(file, "INTEGRAL COEFFICIENTS:\n");
  for(unsigned int pol = 0; pol < npolynomials; pol++) {
    fprintf(file, "\nP%d: 0.0", pol);
    for(unsigned int coef = 0; coef <= pol; coef++) fprintf(file, " %4.18f", coefficients[LEGENDREADDRESS1D(pol, coef)] / ((NUMBERTYPE) coef + 1.0));
  }

  if(nsamples > 0) {
    // Ortogonality of the basis using nsamples
    fprintf(file, "\n\nNUMBER OF SAMPLES OF THE BASIS: %d\n", nsamples);
    fprintf(file, "SCALAR PRODUCTS:\n");
    for(unsigned int i = 0; i < npolynomials; i++)
      for(unsigned int j = i; j < npolynomials; j++) {
        NUMBERTYPE soma = (NUMBERTYPE) 0.0;
        NUMBERTYPE soma2 = (NUMBERTYPE) 0.0, dx = (NUMBERTYPE) (2.0 / (double) nsamples), base = (NUMBERTYPE) -1.0;
        for(unsigned int k = 0; k < nsamples; k++, base += dx) {
          soma += getDiscreteValue(i, k) * getDiscreteValue(j, k);
          soma2 += getIntegralValue(i, base, base + dx) * getIntegralValue(j, base, base + dx);
        }

        fprintf(file, "\n<P%d | P%d> = %4.18f    non-normalized: %4.18f", i, j, soma, soma2);
      }

    // Errors of signal reconstruction
    gcgDISCRETE1D<NUMBERTYPE> signal, rsignal, coef;
    NUMBERTYPE p = (NUMBERTYPE) -1.0, dx = (NUMBERTYPE) (2.0 / (double) nsamples);

    signal.createSignal(nsamples, 0);
    rsignal.createSignal(nsamples, 0);
    coef.createSignal(npolynomials, 0);

    // Polynomial of degree 5
    for(unsigned int i = 0; i < nsamples; i++, p += dx)
      signal.data[i] = (NUMBERTYPE) (1.0 - 2.0 * p  + 3.0 * p*p - 4.0 * p*p*p + 5.0 * p*p*p*p - 6.0 * p*p*p*p*p);

    projectSignal(0, &signal, &coef);
    reconstructSignal(0, &coef, &rsignal);

    fprintf(file, "\n\nRECONSTRUCTING FUNCTION (%d samples): 1.0 - 2.0 * x + 3.0 * x^2 - 4.0 * x^3 + 5.0 * x^4 - 6.0 * x^5\n", this->nsamples);
    fprintf(file, "Mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nMean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // sinusoid in [-PI PI]
    p = (NUMBERTYPE) -1.0;
    for(unsigned int i = 0; i < nsamples; i++, p += dx) signal.data[i] = (NUMBERTYPE) sin(p * M_PI * 2.0);

    projectSignal(0, &signal, &coef);
    reconstructSignal(0, &coef, &rsignal);

    fprintf(file, "\n\nRECONSTRUCTING FUNCTION (%d samples): sin(2PI*x)   x in [-PI, PI]\n", this->nsamples);
    fprintf(file, "Mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nMean squared error: %4.18f", computeMSEwith(&signal, &rsignal));


    // Errors of SUBSIGNAL reconstruction
    fprintf(file, "\n\nRECONSTRUCTING FUNCTION PIECEWISE (%d samples, 3 regions of %d samples): 1.0 - 2.0 * x + 3.0 * x^2 - 4.0 * x^3 + 5.0 * x^4 - 6.0 * x^5\n", this->nsamples*2, this->nsamples);
    signal.createSignal(2 * nsamples, 0);
    rsignal.createSignal(2 * nsamples, 0);
    coef.createSignal(npolynomials, 0);
    p = (NUMBERTYPE) -1.0;
    dx = (NUMBERTYPE) (2.0 / (double) (2.0 * nsamples));

    // Polynomial of degree 5
    for(unsigned int i = 0; i < 2 * nsamples; i++, p += dx)
      signal.data[i] = (NUMBERTYPE) (1.0 - 2.0 * p  + 3.0 * p*p - 4.0 * p*p*p + 5.0 * p*p*p*p - 6.0 * p*p*p*p*p);

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_CLAMP;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "Clamp extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nClamp extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with no repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with no repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_PERIODIC;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nPeriodic extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nPeriodic extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_ZERO;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nZero extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nZero extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // sinusoid in [-PI PI]
    p = (NUMBERTYPE) -1.0;
    for(unsigned int i = 0; i < 2 * nsamples; i++, p += dx) signal.data[i] = (NUMBERTYPE) sin(p * M_PI * 2.0);

    fprintf(file, "\n\nRECONSTRUCTING FUNCTION PIECEWISE (%d samples, 3 regions of %d samples): sin(2PI*x)   x in [-PI, PI]\n", this->nsamples*2, this->nsamples);

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_CLAMP;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "Clamp extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nClamp extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with no repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with no repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_PERIODIC;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nPeriodic extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nPeriodic extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_ZERO;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nZero extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nZero extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    signal.destroySignal();
    rsignal.destroySignal();
    coef.destroySignal();
  } else fprintf(file, "\n\nNo discretized basis. Use setNumberOfSamples() to further basis information.");

  fclose(file);

  return true;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////     BIDIMENSIONAL LEGENDRE POLYNOMIAL BASIS   /////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

gcgLEGENDREBASIS2D<NUMBERTYPE>::gcgLEGENDREBASIS2D() {
  npolynomials = 0;
  coefficients = NULL;
  discretepolynomials = NULL;
  nsamplesX = 0;
  nsamplesY = 0;
}

gcgLEGENDREBASIS2D<NUMBERTYPE>::~gcgLEGENDREBASIS2D() {
  SAFE_FREE(coefficients);
  SAFE_FREE(discretepolynomials);
}


bool gcgLEGENDREBASIS2D<NUMBERTYPE>::setBasisDegree(unsigned int degree) {
  // Allocate memory space
  if(degree != this->degree) {
    SAFE_FREE(coefficients);
    this->degree = 0;
    this->npolynomials = 0;
    coefficients = (double*) ALLOC(sizeof(double) * LEGENDRESIZE2D(degree));
    if(coefficients == NULL) return false;
    this->degree = degree;
    this->npolynomials = ((degree + 2) * (degree + 1)) / 2;
    SAFE_FREE(discretepolynomials);
    discretepolynomials = NULL;
    this->nsamplesX = 0;
    this->nsamplesY = 0;

    // Init coefficients
    memset(coefficients, 0, sizeof(double) * LEGENDRESIZE2D(degree));
    coefficients[LEGENDREADDRESS2D(0, 0, 0, 0)] = 1.0; // First coefficient of first polynomial P(0,0) is 1.0, all others are zero

    // Now apply recursive formula: P(i,j) = (2*i+1)/(i+1) * x * P(i-1,j) - i/(i+1) * P(i-2,j) +
    //                                       (2*j+1)/(j+1) * x * P(i,j-1) - j/(j+1) * P(i,j-2)
    for(unsigned int pj = 0; pj < degree; pj++)
      for(unsigned int pi = 0; pi < degree - pj; pi++)
        // Compute coefficients based on previous coefficients
        for(unsigned int cj = 0; cj <= pj + 1; cj++)
          for(unsigned int ci = 0; ci <= pi + 1; ci++) {
              if(cj <= pj) coefficients[LEGENDREADDRESS2D(pi+1, pj, ci, cj)] = ((double) (pi + pi + 1) / (double) (pi + 1)) * ((ci > 0) ? coefficients[LEGENDREADDRESS2D(pi, pj, ci-1, cj)] : 0.0)
                                                                  - ((double) pi / (double) (pi + 1)) * ((pi > 0 && ci < pi) ? coefficients[LEGENDREADDRESS2D(pi-1, pj, ci, cj)] : 0.0);
              if(ci <= pi) coefficients[LEGENDREADDRESS2D(pi, pj+1, ci, cj)] = ((double) (pj + pj + 1) / (double) (pj + 1)) * ((cj > 0) ? coefficients[LEGENDREADDRESS2D(pi, pj, ci, cj-1)] : 0.0)
                                                                  - ((double) pj / (double) (pj + 1)) * ((pj > 0 && cj < pj) ? coefficients[LEGENDREADDRESS2D(pi, pj-1, ci, cj)] : 0.0);
            }
  }
  return true;
}

bool gcgLEGENDREBASIS2D<NUMBERTYPE>::setNumberOfSamples(unsigned int nsamplesX, unsigned int nsamplesY) {
  if(npolynomials == 0) return false;

  if(nsamplesX != this->nsamplesX || nsamplesY != this->nsamplesY || discretepolynomials == NULL) {
    SAFE_FREE(discretepolynomials);

    // Make room for samples
    this->nsamplesX = 0;
    this->nsamplesY = 0;
    discretepolynomials = (NUMBERTYPE*) ALLOC(sizeof(NUMBERTYPE) * (degree+1) * (degree+1) * nsamplesX * nsamplesY);
    if(discretepolynomials == NULL) return false;
    this->nsamplesX = nsamplesX;
    this->nsamplesY = nsamplesY;

    // Compute samples
    NUMBERTYPE dx = (NUMBERTYPE) (2.0 / (double) nsamplesX);
    NUMBERTYPE dy = (NUMBERTYPE) (2.0 / (double) nsamplesY);
    NUMBERTYPE *samples = &discretepolynomials[DISCRETEADDRESS2D(0, 0, 0, 0)];
    for(unsigned int pj = 0; pj <= degree; pj++)
      for(unsigned int pi = 0; pi <= degree - pj; pi++) {
        register NUMBERTYPE norm = (NUMBERTYPE) 0.0;
        register NUMBERTYPE basey = (NUMBERTYPE) -1.0;
        register NUMBERTYPE *samples2 = samples; // Save for normalization
        for(unsigned int cj = 0; cj < nsamplesY; cj++, basey += dy) {
          register NUMBERTYPE basex = (NUMBERTYPE) -1.0;
          for(unsigned int ci = 0; ci < nsamplesX; ci++, basex += dx) {
            register NUMBERTYPE q = (*(samples++) = getIntegralValue(pi, pj, basex, basex + dx, basey, basey + dy));
            norm += q * q;
          }
        }

#if FLOATING
        norm = (NUMBERTYPE) (1.0 / sqrt(norm));
#else
        norm = (NUMBERTYPE) (1.0 / sqrt((float) norm));
#endif
        // Normalization after discretization
        for(unsigned int cj = 0; cj < nsamplesY; cj++)
          for(unsigned int ci = 0; ci < nsamplesX; ci++) *(samples2++) *= norm;
      }
  }

  return true;
}


unsigned int gcgLEGENDREBASIS2D<NUMBERTYPE>::getNumberOfCoefficients() {
  return npolynomials;
}


bool gcgLEGENDREBASIS2D<NUMBERTYPE>::synchBasisWithFile(const char *filename, unsigned int nsamplesX, unsigned int nsamplesY) {
  if(coefficients == NULL || npolynomials == 0 || nsamplesX == 0 || nsamplesY == 0) return false;

  // Nothing to do
  if(this->npolynomials == (((degree + 2) * (degree + 1)) >> 1) && this->nsamplesX == nsamplesX && this->nsamplesY == nsamplesY) return true;

  int arq;
  int info[5];

  // Loaded flags
  bool fixit = false;
  off_t fixpoint = 0;
  int size = nsamplesX * nsamplesY * sizeof(NUMBERTYPE);
  unsigned int needtoload = npolynomials, availabletosave = 0;
  char *flags = new char[(degree + 1) * (degree + 1)]; // 0 = load, 1 = synchonized, 2 = save
  memset(flags, 0, (degree + 1) * (degree + 1) * sizeof(bool));

  if(discretepolynomials == NULL || this->nsamplesX != nsamplesX || this->nsamplesY != nsamplesY) {
    SAFE_FREE(discretepolynomials);

    // Make room for samples
    this->nsamplesX = 0;
    this->nsamplesY = 0;
    discretepolynomials = (NUMBERTYPE*) ALLOC(sizeof(NUMBERTYPE) * (degree+1) * (degree+1) * nsamplesX * nsamplesY);
    if(discretepolynomials == NULL) {
      SAFE_DELETE_ARRAY(flags);
      return false;
    }
    this->nsamplesX = nsamplesX;
    this->nsamplesY = nsamplesY;
  } else {
    // Mark available functions to save
    for(unsigned int pj = 0; pj <= degree; pj++)
      for(unsigned int pi = 0; pi <= degree - pj; pi++) {
        flags[pj * (degree + 1) + pi] = 2; // Save
        needtoload--;
        availabletosave++;
      }
  }

  // Open file to check existing polynomials
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  arq = open(filename, O_CREAT | O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
  if(arq != -1)
    if(_locking(arq, _LK_LOCK, 999999999L) == -1) {
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  arq = open(filename, O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
  if(arq != -1)
    if(flock(arq, LOCK_EX) == -1) {
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
      close(arq);
      arq = -1;
    }

  if(arq != -1) {
    // Ready to read polynomials
    lseek(arq, 0, SEEK_SET);

    // Spans the file to load existing polynomials
    while(true) {
      fixpoint = lseek(arq, 0, SEEK_CUR);
      if(read(arq, info, sizeof(info)) != sizeof(info)) { // Read indices and function size
        fixit = (lseek(arq, 0, SEEK_CUR) != fixpoint); // Check end of file
        break;
      }
      //printf("Checking %d %d %d %d %d\n", info[0], info[1], info[2], info[3], info[4]);
      if(info[0] < 0 || info[1] < 0 || info[2] <= 0 || info[3] <= 0 || info[4] <= 0 || info[4] > info[2] * info[3] * 12) {
        fixit = true;
        break;
      }
      if((unsigned int) (info[0] + info[1]) <= this->degree && (unsigned int) info[2] == nsamplesX && (unsigned int) info[3] == nsamplesY && info[4] == size) {
        // Has same size, type, and is in range
        if(flags[info[1] * (degree + 1) + info[0]] == 0) {
          if(read(arq, &discretepolynomials[DISCRETEADDRESS2D(info[0], info[1], 0, 0)], size) != size) {
            if(lseek(arq, 0, SEEK_CUR) != fixpoint) fixit = true;
            break;
          }
          flags[info[1] * (degree + 1) + info[0]] = 1; // Synchronized: loaded
          needtoload--;
          //printf("Loaded %d %d %d %d %d\n", info[0], info[1], info[2], info[3], info[4]);
        } else {
            if(flags[info[1] * (degree + 1) + info[0]] == 2) {
              flags[info[1] * (degree + 1) + info[0]] = 1; // Synchronized: already saved
              availabletosave--;
            }
            lseek(arq, info[4], SEEK_CUR);
          }
        // All functions loaded and saved?
        if(needtoload == 0 && availabletosave == 0) {
          SAFE_DELETE_ARRAY(flags);
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
          _locking(arq, LK_UNLCK, 999999999L);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
          flock(arq, LOCK_UN);
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
          close(arq);
          return true;
        }
      } else lseek(arq, info[4], SEEK_CUR);
    }
    if(fixit) {
      // Corrupted. Fix it
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
      _chsize(arq, fixpoint);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
      if(ftruncate(arq, fixpoint) == -1) {
        //printf("Error fixing file.\n");
      }
#endif
      //printf("Fixed at %d\n", fixpoint);
    }
  }

  // If we get here there are polynomials to be computed and/or saved...
  if(arq != -1) lseek(arq, 0, SEEK_END);

  // Compute missing functions
  NUMBERTYPE dx = (NUMBERTYPE) (2.0 / (double) nsamplesX);
  NUMBERTYPE dy = (NUMBERTYPE) (2.0 / (double) nsamplesY);
  for(unsigned int pj = 0; pj <= degree; pj++)
    for(unsigned int pi = 0; pi <= degree - pj; pi++) {
      if(flags[pj * (degree + 1) + pi] == 0) {
        // It was not loaded, needs to be computed
        register NUMBERTYPE norm = (NUMBERTYPE) 0.0;
        register NUMBERTYPE basey = (NUMBERTYPE) -1.0;
        register NUMBERTYPE *samples = &discretepolynomials[DISCRETEADDRESS2D(pi, pj, 0, 0)];
        register NUMBERTYPE *samples2 = samples; // Save for normalization
        for(unsigned int cj = 0; cj < nsamplesY; cj++, basey += dy) {
          register NUMBERTYPE basex = (NUMBERTYPE) -1.0;
          for(unsigned int ci = 0; ci < nsamplesX; ci++, basex += dx) {
            register NUMBERTYPE q = (*(samples++) = getIntegralValue(pi, pj, basex, basex + dx, basey, basey + dy));
            norm += q * q;
          }
        }

        norm = (NUMBERTYPE) (1.0 / sqrt(norm));

        // Normalization after discretization
        for(unsigned int cj = 0; cj < nsamplesY; cj++)
          for(unsigned int ci = 0; ci < nsamplesX; ci++) *(samples2++) *= norm;

        flags[pj * (degree + 1) + pi] = 2; // Now is available to be saved
        availabletosave++;
        needtoload--;
        //printf("Computed %d %d %d %d %d\n", pi, pj, nsamplesX, nsamplesY, size);
      }

      // Saving functions
      if(flags[pj * (degree + 1) + pi] == 2 && arq != -1) {
        fixpoint = lseek(arq, 0, SEEK_CUR);

        // Now save this function
        info[0] = pi;
        info[1] = pj;
        info[2] = nsamplesX;
        info[3] = nsamplesY;
        info[4] = size;
        fixit = false;
        if(write(arq, info, sizeof(info)) != sizeof(info)) fixit = true;
        else fixit = (write(arq, &discretepolynomials[DISCRETEADDRESS2D(pi, pj, 0, 0)], size) != size);
        if(fixit) {
          if(lseek(arq, 0, SEEK_CUR) != fixpoint) {
            // Corrupted. Fix it
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
            _chsize(arq, fixpoint);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
            if(ftruncate(arq, fixpoint) == -1) {
              //printf("Error fixing file.\n");
            }
#endif
          }
          //printf("Fixed at %d\n", fixpoint);
          break;
        } else {
          availabletosave--;
          //printf("Saved %d %d %d %d %d\n", info[0], info[1], info[2], info[3], info[4]);
        }
      }

      // All functions loaded and saved?
      if(needtoload == 0 && availabletosave == 0) break;
    }

  SAFE_DELETE_ARRAY(flags);
  if(arq != -1) {
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
    _locking(arq, LK_UNLCK, 999999999L);
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
    flock(arq, LOCK_UN);
#else
#error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
    close(arq);
  }
  return needtoload == 0 && availabletosave == 0;
}



NUMBERTYPE gcgLEGENDREBASIS2D<NUMBERTYPE>::getPointValue(unsigned int pi, unsigned int pj, NUMBERTYPE x, NUMBERTYPE y) {
  if(pi + pj > degree) return (NUMBERTYPE) 0.0;

  // We must use double precision because polynomial coefficients of higher degrees are too big
  double sum = 0.0;
  double powerY = 1.0;

  double *indcoef = &coefficients[LEGENDREADDRESS2D(pi, pj, 0, 0)];
  for(unsigned int cj = 0; cj <= pj; cj++, powerY *= x) {
    double powerX = 1.0;
    for(unsigned int ci = 0; ci <= pi; ci++, powerX *= y) sum += (double) *(indcoef++) * powerX * powerY;
  }
  return (NUMBERTYPE) sum;
}


NUMBERTYPE gcgLEGENDREBASIS2D<NUMBERTYPE>::getIntegralValue(unsigned int pi, unsigned int pj, NUMBERTYPE xmin, NUMBERTYPE xmax,  NUMBERTYPE ymin, NUMBERTYPE ymax) {
  if(pi + pj > degree) return (NUMBERTYPE) 0.0;

  // We must use double precision because polynomial coefficients of higher degrees are too big
  double sumA = 0.0, sumB = 0.0;
  double sumC = 0.0, sumD = 0.0;
  // Shifted coefficients for integration: index 0 means degree 1 and so on
  double powerYmin = ymin, powerYmax = ymax;

  double *indcoef = &coefficients[LEGENDREADDRESS2D(pi, pj, 0, 0)];
  for(unsigned int cj = 0; cj <= pj; cj++) {
    double powerXmin = xmin, powerXmax = xmax; // Shifted coefficients for integration: index 0 means degree 1 and so on
    for(unsigned int ci = 0; ci <= pi; ci++) {
      double coef = (((double) *(indcoef++)) / ((double) (ci + 1) * (double) (cj + 1))); // Get integral coefficients of the polynomial
      sumA += coef * powerXmin * powerYmin;
      sumB += coef * powerXmax * powerYmax;
      sumC += coef * powerXmin * powerYmax;
      sumD += coef * powerXmax * powerYmin;
      powerXmin *= xmin;
      powerXmax *= xmax;
    }
    powerYmin *= ymin;
    powerYmax *= ymax;
  }

  return (NUMBERTYPE) (sumA + sumB - sumC - sumD);
}


NUMBERTYPE gcgLEGENDREBASIS2D<NUMBERTYPE>::getDiscreteValue(unsigned int lpi, unsigned int lpj, unsigned int i, unsigned int j) {
  if(lpi + lpj > degree || i >= nsamplesX || j >= nsamplesY || discretepolynomials == NULL) return (NUMBERTYPE) 0.0;

  // Return pre-computed value
  return discretepolynomials[DISCRETEADDRESS2D(lpi, lpj, i, j)];
}


bool gcgLEGENDREBASIS2D<NUMBERTYPE>::basisInformation(const char *filename, bool checkorthogonality, bool savefunctionsasimages) {
  if(npolynomials == 0) return false;

  FILE *file = fopen(filename, "wt");

  // Polynomial coefficients
  if(file == NULL) return false;
  fprintf(file, "BASIS OF DEGREE: %d\n", degree);
  fprintf(file, "POLYNOMIAL COEFFICIENTS: %d %d\n", npolynomials, COEFFICIENTSIZE2D(degree));

  for(unsigned int pi = 0; pi <= degree; pi++)
    for(unsigned int pj = 0; pj <= degree - pi; pj++) {
        fprintf(file, "\nP(%d,%d):", pi, pj);

        for(unsigned int cj = 0; cj <= pj; cj++) {
          for(unsigned int ci = 0; ci <= pi; ci++)
            fprintf(file, " %4.18f", coefficients[LEGENDREADDRESS2D(pi, pj, ci, cj)]);
          if(cj < pj) fprintf(file, "\n       ");
        }
      }

  // Integral coefficients
  fprintf(file, "\n\nINTEGRALS OF DEGREE: %d\n", degree + 1);
  fprintf(file, "INTEGRAL COEFFICIENTS:\n");

  for(unsigned int pj = 0; pj <= degree; pj++)
    for(unsigned int pi = 0; pi <= degree - pj; pi++) {
      fprintf(file, "\nP(%d,%d):", pi, pj);

      for(unsigned int ci = 0; ci <= pi; ci++) {
        for(unsigned int cj = 0; cj <= pj; cj++)
          fprintf(file, " %4.18f", coefficients[LEGENDREADDRESS2D(pi, pj, ci, cj)]  / (NUMBERTYPE) ((NUMBERTYPE) (ci + 1) * (NUMBERTYPE) (cj + 1)));
        if(ci < pi) fprintf(file, "\n       ");
      }
    }

  if(nsamplesX > 0 && nsamplesY > 0) {
    // Ortogonality of the basis using nsamples
    fprintf(file, "\n\nNUMBER OF SAMPLES OF THE BASIS: %d x %d\n", nsamplesX, nsamplesY);


    NUMBERTYPE dx = (NUMBERTYPE) (2.0 / (double) nsamplesX);
    NUMBERTYPE dy = (NUMBERTYPE) (2.0 / (double) nsamplesY);
    if(checkorthogonality) {
      fprintf(file, "SCALAR PRODUCTS:\n");

      for(unsigned int pi1 = 0; pi1 <= this->degree; pi1++)
        for(unsigned int pj1 = 0; pj1 <= this->degree - pi1; pj1++)
          for(unsigned int pi2 = 0; pi2 <= this->degree; pi2++)
            for(unsigned int pj2 = 0; pj2 <= this->degree - pi2; pj2++) {
              NUMBERTYPE soma = (NUMBERTYPE) 0.0, soma2 = (NUMBERTYPE) 0.0;
              NUMBERTYPE baseX = (NUMBERTYPE) -1.0;
              for(unsigned int ci = 0; ci < nsamplesX; ci++, baseX += dx) {
                NUMBERTYPE baseY = (NUMBERTYPE) -1.0;
                for(unsigned int cj = 0; cj < nsamplesY; cj++, baseY += dy) {
                  soma  += getDiscreteValue(pi1, pj1, ci, cj) * getDiscreteValue(pi2, pj2, ci, cj);
                  soma2 += getIntegralValue(pi1, pj1, baseX, baseX + dx, baseY, baseY + dy) * getIntegralValue(pi2, pj2, baseX, baseX + dx, baseY, baseY + dy);
                }
              }

              fprintf(file, "\n<P(%d,%d) | P(%d,%d)> = %4.18f    non-normalized: %4.18f", pi1, pj1, pi2, pj2, soma, soma2);
            }
    }

    // Saves current discretization of polynomials as images
    gcgIMAGE image;
    gcgDISCRETE2D<NUMBERTYPE> pol;
    if(savefunctionsasimages)
      for(unsigned int pj = 0; pj <= degree; pj++)
        for(unsigned int pi = 0; pi <= degree - pj; pi++) {
          char filename[100];
          sprintf(filename, "gcg-legendrebasis2d-P(%d,%d)-%d-x-%d.bmp", pi, pj, nsamplesX, nsamplesY);
          pol.createSignal(nsamplesX, nsamplesY, 0, 0, &discretepolynomials[DISCRETEADDRESS2D(pi, pj, 0, 0)]);
          pol.exportGrayScaleImage(&image, true);
          image.saveBMP(filename);
        }


    // Errors of signal reconstruction
    gcgDISCRETE1D<NUMBERTYPE> coef;
    gcgDISCRETE2D<NUMBERTYPE> signal, rsignal;
    signal.createSignal(nsamplesX, nsamplesY, 0, 0);
    rsignal.createSignal(nsamplesX, nsamplesY, 0, 0);
    coef.createSignal(npolynomials, 0);

    // Polynomial of degree 5
    NUMBERTYPE py = (NUMBERTYPE) -1.0;
    for(unsigned int j = 0; j < nsamplesY; j++, py += dy) {
      NUMBERTYPE px = (NUMBERTYPE) -1.0;
      for(unsigned int i = 0; i < nsamplesX; i++, px += dx)
        signal.data[j * nsamplesX + i] = (NUMBERTYPE) (2.0f * px - 4.0f * px*px*px + 1.0f * px*px*px*px -
                                                     + 2.0f * py  - 3.0f * py*py - py*py*py*py);
    }

    signal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-original-polynomial.bmp");
    projectSignal(0, 0, &signal, &coef);
    reconstructSignal(0, 0, &coef, &rsignal);
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-reconstructed-polynomial.bmp");

    fprintf(file, "\n\nRECONSTRUCTING FUNCTION: 2.0 * x - 4.0 * x^3 + x^4 - 2.0 * y - 3.0 * y^2 - y^4\n");
    fprintf(file, "Mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nMean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // sinusoid in [-PI PI]
    py = (NUMBERTYPE) -1.0f;
    for(unsigned int j = 0; j < nsamplesY; j++, py += dy) {
      NUMBERTYPE px = (NUMBERTYPE) -1.0;
      for(unsigned int i = 0; i < nsamplesX; i++, px += dx)
        signal.data[j * nsamplesX + i] = (NUMBERTYPE) (sin(px * M_PI*2.0) * sin(py * M_PI*2.0));
    }

    signal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-original-sinusoid.bmp");
    projectSignal(0, 0, &signal, &coef);
    reconstructSignal(0, 0, &coef, &rsignal);
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-reconstructed-sinusoid.bmp");

    // Compute error
    fprintf(file, "\n\nRECONSTRUCTING FUNCTION: sin(2PI*x)*sin(2PI*y)   x,y in [-PI, PI]\n");
    fprintf(file, "Mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nMean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // Errors of SUBSIGNAL reconstruction
    fprintf(file, "\n\nRECONSTRUCTING FUNCTION PIECEWISE (%dx%d samples, 4 regions of %dx%d): 1.0 - 2.0 * x + 3.0 * x^2 - 4.0 * x^3 + 5.0 * x^4 - 6.0 * x^5\n", this->nsamplesX*2, this->nsamplesY*2, this->nsamplesX, this->nsamplesY);

    // Polynomial of degree 5
    py = (NUMBERTYPE) -1.0;
    for(unsigned int j = 0; j < nsamplesY; j++, py += dy) {
      NUMBERTYPE px = (NUMBERTYPE) -1.0;
      for(unsigned int i = 0; i < nsamplesX; i++, px += dx)
        signal.data[j * nsamplesX + i] = (NUMBERTYPE) (2.0f * px - 4.0f * px*px*px + 1.0f * px*px*px*px -
                                                       + 2.0f * py  - 3.0f * py*py - py*py*py*py);
    }

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_CLAMP;
    signal.extensionY = GCG_BORDER_EXTENSION_CLAMP;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "Clamp extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nClamp extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-clamp-polynomial.bmp");

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;
    signal.extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with no repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with no repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-symmetric-norepeat-polynomial.bmp");

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT;
    signal.extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-symmetric-repeat-polynomial.bmp");

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_PERIODIC;
    signal.extensionY = GCG_BORDER_EXTENSION_PERIODIC;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "\nPeriodic extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nPeriodic extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-periodic-polynomial.bmp");

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_ZERO;
    signal.extensionY = GCG_BORDER_EXTENSION_ZERO;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "\nZero extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nZero extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-zero-polynomial.bmp");

    // sinusoid in [-PI PI]
    fprintf(file, "\n\nRECONSTRUCTING FUNCTION PIECEWISE (%dx%d samples, 4 regions of %dx%d): sin(2PI*x)*sin(2PI*y)   x,y in [-PI, PI]\n", this->nsamplesX*2, this->nsamplesY*2, this->nsamplesX, this->nsamplesY);
    py = (NUMBERTYPE) -1.0;
    for(unsigned int j = 0; j < nsamplesY; j++, py += dy) {
      NUMBERTYPE px = (NUMBERTYPE) -1.0;
      for(unsigned int i = 0; i < nsamplesX; i++, px += dx)
        signal.data[j * nsamplesX + i] = (NUMBERTYPE) (sin(px * M_PI*2.0) * sin(py * M_PI*2.0));
    }

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_CLAMP;
    signal.extensionY = GCG_BORDER_EXTENSION_CLAMP;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "Clamp extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nClamp extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-clamp-sinusoid.bmp");

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;
    signal.extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with no repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with no repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-symmetric-norepeat-sinusoid.bmp");

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT;
    signal.extensionY = GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-symmetric-repeat-sinusoid.bmp");

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_PERIODIC;
    signal.extensionY = GCG_BORDER_EXTENSION_PERIODIC;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "\nPeriodic extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nPeriodic extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-periodic-sinusoid.bmp");

    // First region [-nsamplesX/2, nsamplesX/2) x [-nsamplesY/2, nsamplesY/2)
    signal.extensionX = GCG_BORDER_EXTENSION_ZERO;
    signal.extensionY = GCG_BORDER_EXTENSION_ZERO;
    projectSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples) x [-nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, -(int) nsamplesY / 2, &coef, &rsignal);
    // Third region [-nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal(-(int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);
    // Fourth region [nsamplesX/2, nsamplesX/2) x [nsamplesY/2, nsamplesY/2)
    projectSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &signal, &coef);
    reconstructSignal((int) nsamplesX / 2, (int) nsamplesY / 2, &coef, &rsignal);

    fprintf(file, "\nZero extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nZero extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));
    rsignal.exportGrayScaleImage(&image, true);
    image.saveBMP("gcg-legendrebasis2D-subsignal-zero-sinusoid.bmp");


    signal.destroySignal();
    rsignal.destroySignal();
    coef.destroySignal();
  } else fprintf(file, "\n\nNo discretized basis. Use setNumberOfSamples() or synchBasisWithFile() to further basis information.");

  fclose(file);

  return true;
}



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////     UNIDIMENSIONAL REAL WAVELET BASIS     //////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


gcgDWTBASIS1D<NUMBERTYPE>::gcgDWTBASIS1D() {
  this->nsamples = 0;
  this->ncoefficients = 0;
  this->buffer = NULL;
}

gcgDWTBASIS1D<NUMBERTYPE>::~gcgDWTBASIS1D() {
  SAFE_FREE(this->buffer);
  this->buffer = NULL;
  this->nsamples = 0;
  this->ncoefficients = 0;
}

bool gcgDWTBASIS1D<NUMBERTYPE>::setWavelets(gcgDISCRETE1D<NUMBERTYPE> *pH, gcgDISCRETE1D<NUMBERTYPE> *pG, gcgDISCRETE1D<NUMBERTYPE> *prH, gcgDISCRETE1D<NUMBERTYPE> *prG) {
  // All filters must be valid
  if(pH == NULL || pG == NULL || prH == NULL || prG == NULL) return false;

  // Ok, set filter bank
  this->H.duplicateSignal(pH);
  this->G.duplicateSignal(pG);
  this->rH.duplicateSignal(prH);
  this->rG.duplicateSignal(prG);

  // Adjust extension
  this->H.extension = GCG_BORDER_EXTENSION_ZERO;
  this->G.extension = GCG_BORDER_EXTENSION_ZERO;
  this->rH.extension = GCG_BORDER_EXTENSION_ZERO;
  this->rG.extension = GCG_BORDER_EXTENSION_ZERO;

  return true;
}

bool gcgDWTBASIS1D<NUMBERTYPE>::setNumberOfSamples(unsigned int n) {
  // Check number of samples
  if(n == 0) return false;
  if(n == this->nsamples) return true;   // Nothing to do...

  // Check parity
  if(n & 0x01) this->ncoefficients = (n + 1) >> 1; // Length is odd...
  else this->ncoefficients = n >> 1; // Length is even...

  this->nsamples = n;

  // Allocates space for projection/reconstruction for greatest possible data type = double
  SAFE_FREE(this->buffer);
  this->buffer = ALLOC(sizeof(double) * n);

  if(this->buffer == NULL) {
    this->nsamples = 0;
    this->ncoefficients = 0;
    return false;
  }

  return true;
}

unsigned int gcgDWTBASIS1D<NUMBERTYPE>::getNumberOfCoefficients() {
  return ncoefficients + ncoefficients;
}


bool gcgDWTBASIS1D<NUMBERTYPE>::basisInformation(const char *filename) {
  FILE *file = fopen(filename, "wt");

  if(nsamples > 0) {
    // Errors of signal reconstruction
    gcgDISCRETE1D<NUMBERTYPE> signal, rsignal, coef;
    NUMBERTYPE p = (NUMBERTYPE) -1.0, dx = (NUMBERTYPE) (2.0 / (double) nsamples);

    signal.createSignal(nsamples, 0, NULL, false, GCG_BORDER_EXTENSION_PERIODIC);
    rsignal.createSignal(nsamples, 0, NULL, false, GCG_BORDER_EXTENSION_PERIODIC);

    // Polynomial of degree 5
    for(unsigned int i = 0; i < nsamples; i++, p += dx)
      signal.data[i] = (NUMBERTYPE) (1.0 - 2.0 * p  + 3.0 * p*p - 4.0 * p*p*p + 5.0 * p*p*p*p - 6.0 * p*p*p*p*p);

    projectSignal(0, &signal, &coef);
    reconstructSignal(0, &coef, &rsignal);

    fprintf(file, "RECONSTRUCTING FUNCTION WITH PERIODIC EXTENSION (%d samples): 1.0 - 2.0 * x + 3.0 * x^2 - 4.0 * x^3 + 5.0 * x^4 - 6.0 * x^5\n", this->nsamples);
    fprintf(file, "Mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nMean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // sinusoid in [-PI PI]
    p = (NUMBERTYPE) -1.0;
    for(unsigned int i = 0; i < nsamples; i++, p += dx) signal.data[i] = (NUMBERTYPE) sin(p * M_PI * 2.0);

    projectSignal(0, &signal, &coef);
    reconstructSignal(0, &coef, &rsignal);

    fprintf(file, "\n\nRECONSTRUCTING FUNCTION WITH PERIODIC EXTENSION (%d samples): sin(2PI*x)   x in [-PI, PI]\n", this->nsamples);
    fprintf(file, "Mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nMean squared error: %4.18f", computeMSEwith(&signal, &rsignal));


    // Errors of SUBSIGNAL reconstruction
    fprintf(file, "\n\nRECONSTRUCTING FUNCTION PIECEWISE (%d samples, 3 regions of %d samples): 1.0 - 2.0 * x + 3.0 * x^2 - 4.0 * x^3 + 5.0 * x^4 - 6.0 * x^5\n", this->nsamples*2, this->nsamples);
    signal.createSignal(2 * nsamples, 0, NULL, false, GCG_BORDER_EXTENSION_PERIODIC);
    rsignal.createSignal(2 * nsamples, 0, NULL, false, GCG_BORDER_EXTENSION_PERIODIC);
    p = (NUMBERTYPE) -1.0;
    dx = (NUMBERTYPE) (2.0 / (double) (2.0 * nsamples));

    // Polynomial of degree 5
    for(unsigned int i = 0; i < 2 * nsamples; i++, p += dx)
      signal.data[i] = (NUMBERTYPE) (1.0 - 2.0 * p  + 3.0 * p*p - 4.0 * p*p*p + 5.0 * p*p*p*p - 6.0 * p*p*p*p*p);

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_CLAMP;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "Clamp extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nClamp extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with no repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with no repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_PERIODIC;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nPeriodic extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nPeriodic extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_ZERO;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nZero extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nZero extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // sinusoid in [-PI PI]
    p = (NUMBERTYPE) -1.0;
    for(unsigned int i = 0; i < 2 * nsamples; i++, p += dx) signal.data[i] = (NUMBERTYPE) sin(p * M_PI * 2.0);

    fprintf(file, "\n\nRECONSTRUCTING FUNCTION PIECEWISE (%d samples, 3 regions of %d samples): sin(2PI*x)   x in [-PI, PI]\n", this->nsamples*2, this->nsamples);

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_CLAMP;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "Clamp extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nClamp extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_SYMMETRIC_NOREPEAT;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with no repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with no repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_SYMMETRIC_REPEAT;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nSymmetric extension with repeat: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nSymmetric extension with repeat: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_PERIODIC;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nPeriodic extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nPeriodic extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    // First region [-nsamples/2, nsamples/2)
    signal.extension = GCG_BORDER_EXTENSION_ZERO;
    projectSignal(-(int) nsamples / 2, &signal, &coef);
    reconstructSignal(-(int) nsamples / 2, &coef, &rsignal);
    // Second region [nsamples/2, nsamples/2 + nsamples)
    projectSignal(nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples / 2, &coef, &rsignal);
    // Third region [nsamples/2 + nsamples, 2*nsamples)
    projectSignal(nsamples + nsamples / 2, &signal, &coef);
    reconstructSignal(nsamples + nsamples / 2, &coef, &rsignal);

    fprintf(file, "\nZero extension: mean absolute error: %4.18f", computeMAEwith(&signal, &rsignal));
    fprintf(file, "\nZero extension: mean squared error: %4.18f", computeMSEwith(&signal, &rsignal));

    signal.destroySignal();
    rsignal.destroySignal();
    coef.destroySignal();
  } else fprintf(file, "\n\nNo discretized basis. Use setNumberOfSamples() to further basis information.");

  fclose(file);

  return true;
}





#endif // #if FLOATING




/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////     SEQUENCE DERIVATIVE COMPUTATION     ///////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Constructors and destructors
gcgSEQUENCEDERIVATIVE2D<NUMBERTYPE>::gcgSEQUENCEDERIVATIVE2D() {
  next = NULL;
  current = NULL;
  lowcurrent = new gcgDISCRETE2D<NUMBERTYPE>();
  lownext = new gcgDISCRETE2D<NUMBERTYPE>();
}

gcgSEQUENCEDERIVATIVE2D<NUMBERTYPE>::~gcgSEQUENCEDERIVATIVE2D() {
    SAFE_DELETE(lowcurrent);
    SAFE_DELETE(lownext);
}

bool gcgSEQUENCEDERIVATIVE2D<NUMBERTYPE>::sequenceDerivative(gcgDISCRETE2D<NUMBERTYPE> *_current, gcgDISCRETE2D<NUMBERTYPE> *_next, gcgDISCRETE2D<NUMBERTYPE> *_outputDt) {
  static NUMBERTYPE lowpass_sobel3_mask[] = {(NUMBERTYPE) 1.0, (NUMBERTYPE) 2.0, (NUMBERTYPE) 1.0};
  gcgDISCRETE1D<NUMBERTYPE> lowpass_sobel3_1D(3, 1, lowpass_sobel3_mask, true, GCG_BORDER_EXTENSION_ZERO);

  // Check parameters
  if(_current == NULL || _next == NULL || _outputDt == NULL) return false;

  // Advance the queue
  if(this->next == _current) {
    gcgDISCRETE2D<NUMBERTYPE> *swp = this->current;
    this->current  = this->next;
    this->next = swp;
    swp = this->lowcurrent;
    this->lowcurrent = this->lownext;
    this->lownext = swp;

    if(_next != this->next) {
      // Compute missing low pass
      temp.convolutionX(_next, &lowpass_sobel3_1D);
      this->lownext->convolutionY(&temp, &lowpass_sobel3_1D);
      this->next = _next;
    }
  } else {
    if(_current != this->current) {
      // Compute missing low pass
      temp.convolutionX(_current, &lowpass_sobel3_1D);
      this->lowcurrent->convolutionY(&temp, &lowpass_sobel3_1D);
      this->current = _current;
    }
    if(_next != this->next) {
      // Compute missing low pass
      temp.convolutionX(_next, &lowpass_sobel3_1D);
      this->lownext->convolutionY(&temp, &lowpass_sobel3_1D);
      this->next = _next;
    }
  }

  // Compute partial derivative
  return _outputDt->combineAdd(this->lowcurrent, this->lownext,  (NUMBERTYPE) -1.0, (NUMBERTYPE) 1.0);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//////////////////////     FILTER MASK GENERATION     ///////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Filter mask constructor
gcgFILTERMASK1D<NUMBERTYPE>::gcgFILTERMASK1D() {
}

// Filter mask destructor
gcgFILTERMASK1D<NUMBERTYPE>::~gcgFILTERMASK1D() {
}

// Box filter construction
bool gcgFILTERMASK1D<NUMBERTYPE>::createBoxFilter(unsigned int nsamples) {
  if(nsamples == 0) return false;

  int _origin = (nsamples & 0x1) ? (((int) nsamples - 1) >> 1) : ((((int) nsamples) >> 1) - 1);
  if(!this->createSignal(nsamples, _origin, NULL, false, GCG_BORDER_EXTENSION_ZERO)) return false;

  NUMBERTYPE  value = (NUMBERTYPE) (1.0 / (NUMBERTYPE) nsamples);
  for(unsigned int i = 0; i < nsamples; i++) this->data[i] = value;
  return true;
}

// Triangle filter construction
bool gcgFILTERMASK1D<NUMBERTYPE>::createTriangleFilter(unsigned int nsamples) {
  if(nsamples == 0) return false;

  if(nsamples & 0x1) {
    int _origin = ((int) nsamples - 1) >> 1;
    if(!this->createSignal(nsamples, _origin, NULL, false, GCG_BORDER_EXTENSION_ZERO)) return false;

    NUMBERTYPE inv = (NUMBERTYPE) (1.0 / (((int) _origin) * ((int) _origin + 1) + _origin + 1));
    this->data[_origin] = (NUMBERTYPE) (_origin + 1) * inv;
    for(int i = 0; i < _origin; i++) this->data[i] = this->data[nsamples - i - 1] = (NUMBERTYPE) (i+1) * inv;
  } else {
    int _origin = (((int) nsamples) >> 1) - 1;
    if(!this->createSignal(nsamples, _origin, NULL, false, GCG_BORDER_EXTENSION_ZERO)) return false;

    NUMBERTYPE inv = (NUMBERTYPE) (1.0 / (((int) _origin + 1) * ((int) _origin + 2)));
    for(int i = 0; i <= _origin; i++) this->data[i] = this->data[nsamples - i - 1] = (NUMBERTYPE) (i+1) * inv;
  }

  return true;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//////////////////     SIGNAL PROCESSING FUNCTIONS    ///////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// Computes the Mean Squared Error. Perfect reconstruction
// gives MSE = 0.0
////////////////////////////////////////////////////////////
template<> double computeMSEwith(gcgDISCRETE1D<NUMBERTYPE> *src1, gcgDISCRETE1D<NUMBERTYPE> *src2) {
  // Check source pointers
  if(src1 == NULL || src2 == NULL) return INF;
  if(src1->data == NULL || src1->length == 0 || src2->data == NULL || src2->length == 0) return INF;

  // Check equivalence between both signals.
  if(src1->length != src2->length) return INF;

  // Now we compute the MSE for this case
  double sqsum = 0;
  NUMBERTYPE *thisrow = src1->data;
  NUMBERTYPE *thatrow = src2->data;
  unsigned int length = src1->length;
  for(unsigned int j = 0; j < length; j++) sqsum += SQR(thisrow[j] - thatrow[j]);

  return sqsum / (double) length;
}

////////////////////////////////////////////////////////////
// Computes the Peak Signal-to-Reconstructed Ratio. Good results
// vary between 20 e 40 db. Perfect reconstruction gives
// an infinite PSNR.
////////////////////////////////////////////////////////////
template<> double computePSNRwith(gcgDISCRETE1D<NUMBERTYPE> *src1, gcgDISCRETE1D<NUMBERTYPE> *src2) {
  double mse = computeMSEwith(src1, src2);

  // Computes the PSNR in decibels = 20log(base10) (255/sqrt(MSE))
  return (mse < EPSILON) ? 0.0 : 20 * log10(255.0 / sqrt(mse));
}


////////////////////////////////////////////////////////////
// Computes the Mean Absolute Error. Perfect reconstruction
// gives MAE = 0.
////////////////////////////////////////////////////////////
template<> double computeMAEwith(gcgDISCRETE1D<NUMBERTYPE> *src1, gcgDISCRETE1D<NUMBERTYPE> *src2) {
  // Check source pointers
  if(src1 == NULL || src2 == NULL) return INF;
  if(src1->data == NULL || src1->length == 0 || src2->data == NULL || src2->length == 0) return INF;

  // Check equivalence between both signals.
  if(src1->length != src2->length) return INF;

  // Now we compute the MSE for this case
  double sum = 0;
  NUMBERTYPE *thisrow = src1->data;
  NUMBERTYPE *thatrow = src2->data;
  unsigned int length = src1->length;
  for(unsigned int j = 0; j < length; j++)
#if FLOATING
    sum += (NUMBERTYPE) fabs(thisrow[j] - thatrow[j]);
#else
    sum += (NUMBERTYPE) abs(thisrow[j] - thatrow[j]);
#endif

  return sum / (double) length;
}

////////////////////////////////////////////////////////////
// Computes the gradient of 1D signal src. The derivative is
// computed using well-known Sobel operator and the result
// is stored in dx.
////////////////////////////////////////////////////////////
template<> bool gcgFirstDerivativeSobel1D(gcgDISCRETE1D<NUMBERTYPE> *src, gcgDISCRETE1D<NUMBERTYPE> *dx) {
  static NUMBERTYPE highpass_sobel3_mask[] = {(NUMBERTYPE) -1.0, (NUMBERTYPE) 0.0, (NUMBERTYPE) 1.0};
  gcgDISCRETE1D<NUMBERTYPE> highpass_sobel3_1D(3, 1, highpass_sobel3_mask, true);

  // check parameters
  if(src == NULL) return false;

  // Compute first derivative in X
  if(dx) dx->convolution(src, &highpass_sobel3_1D);

  return true;
}

////////////////////////////////////////////////////////////
// Computes the Mean Squared Error. Perfect reconstruction
// gives MSE = 0.
////////////////////////////////////////////////////////////
template<> double computeMSEwith(gcgDISCRETE2D<NUMBERTYPE> *src1, gcgDISCRETE2D<NUMBERTYPE> *src2) {
  // Check source pointers
  if(src1 == NULL || src2 == NULL) return INF;
  if(src1->data == NULL || src1->width == 0 || src1->height == 0 || src2->data == NULL || src2->width == 0 || src2->height == 0) return INF;

  // Check equivalence between both signals.
  if(src1->width != src2->width || src1->height != src2->height) return INF;

  // Now we compute the MSE for this case
  double sqsum = 0;
  NUMBERTYPE *thisval = src1->data;
  NUMBERTYPE *thatval = src2->data;
  unsigned int size = src1->width * src1->height;
  for(unsigned int i = 0; i < size; i++, thisval++, thatval++) sqsum += SQR(*thisval - *thatval);

  return sqsum / (double) size;
}


////////////////////////////////////////////////////////////
// Computes the Peak Signal-to-Reconstructed Ratio. Good results
// vary between 20 e 40 db. Perfect reconstruction gives an
// infinite PSNR.
////////////////////////////////////////////////////////////
template<> double computePSNRwith(gcgDISCRETE2D<NUMBERTYPE> *src1, gcgDISCRETE2D<NUMBERTYPE> *src2) {
  double mse = computeMSEwith(src1, src2);

  // Computes the PSNR in decibels = 20log(base10) (255/sqrt(MSE))
  return (mse < EPSILON) ? 0.0 : 20 * log10(255.0 / sqrt(mse));
}


////////////////////////////////////////////////////////////
// Computes the Mean Absolute Error. Perfect reconstruction
// gives MAE = 0.
////////////////////////////////////////////////////////////
template<> double computeMAEwith(gcgDISCRETE2D<NUMBERTYPE> *src1, gcgDISCRETE2D<NUMBERTYPE> *src2) {
  // Check source pointers
  if(src1 == NULL || src2 == NULL) return INF;
  if(src1->data == NULL || src1->width == 0 || src1->height == 0 || src2->data == NULL || src2->width == 0 || src2->height == 0) return INF;

  // Check equivalence between both signals.
  if(src1->width != src2->width || src1->height != src2->height) return INF;

  // Now we compute the MSE for this case
  double sum = 0;
  NUMBERTYPE *thisval = src1->data;
  NUMBERTYPE *thatval = src2->data;
  unsigned int size = src1->width * src1->height;
  for(unsigned int i = 0; i < size; i++, thisval++, thatval++)
#if FLOATING
    sum += (NUMBERTYPE) fabs(*thisval - *thatval);
#else
    sum += (NUMBERTYPE) abs(*thisval - *thatval);
#endif

  return sum / (double) size;
}


// Computes the gradient of 2D signal src. The partial derivatives are computed using well-known Sobel
// operator and the components are stored in dx and dy. The pointer for dx or dy may be NULL.
template<> bool gcgGradientSobel2D(gcgDISCRETE2D<NUMBERTYPE> *src, gcgDISCRETE2D<NUMBERTYPE> *dx, gcgDISCRETE2D<NUMBERTYPE> *dy) {
  static NUMBERTYPE lowpass_sobel3_mask[] = {(NUMBERTYPE) 1.0, (NUMBERTYPE) 2.0, (NUMBERTYPE) 1.0};
  static NUMBERTYPE highpass_sobel3_mask[] = {(NUMBERTYPE) -1.0, (NUMBERTYPE) 0.0, (NUMBERTYPE) 1.0};
  gcgDISCRETE1D<NUMBERTYPE> lowpass_sobel3_1D(3, 1, lowpass_sobel3_mask, true, GCG_BORDER_EXTENSION_ZERO);
  gcgDISCRETE1D<NUMBERTYPE> highpass_sobel3_1D(3, 1, highpass_sobel3_mask, true, GCG_BORDER_EXTENSION_ZERO);

  // check parameters
  if(src == NULL) return false;

  // Compute partial derivative in X
  if(dx) {
    gcgDISCRETE2D<NUMBERTYPE> templow;
    templow.convolutionY(src, &lowpass_sobel3_1D);
    dx->convolutionX(&templow, &highpass_sobel3_1D);
  }

  // Compute partial derivative in Y
  if(dy) {
    gcgDISCRETE2D<NUMBERTYPE> templow;
    templow.convolutionY(src, &highpass_sobel3_1D);
    dy->convolutionX(&templow, &lowpass_sobel3_1D);
  }

  return true;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////     IMAGE PROCESSING   /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


bool gcgIMAGE::exportChannels(gcgDISCRETE2D<NUMBERTYPE> *red, gcgDISCRETE2D<NUMBERTYPE> *green, gcgDISCRETE2D<NUMBERTYPE> *blue, gcgDISCRETE2D<NUMBERTYPE> *alpha) {
  // Check source image
  if(this->data == NULL || this->width == 0 || this->height == 0 || this->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "exportChannels(): current image is not valid. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameters
  if(red == NULL && green == NULL && blue == NULL && alpha == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "exportChannels(): all destinations are NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return true; // Nothing to do
  }

  // Check destination signals
  if(red) if(!red->createSignal(this->width, this->height, 0, 0)) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_SIGNAL, GCG_FUNCTIONCALLFAILED), "exportChannels(): a call to createImage() has failed for red channel. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(green) if(!green->createSignal(this->width, this->height, 0, 0)) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_SIGNAL, GCG_FUNCTIONCALLFAILED), "exportChannels(): a call to createImage() has failed for green channel. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(blue) if(!blue->createSignal(this->width, this->height, 0, 0)) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_SIGNAL, GCG_FUNCTIONCALLFAILED), "exportChannels(): a call to createImage() has failed for blue channel. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(alpha) if(!alpha->createSignal(this->width, this->height, 0, 0)) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_SIGNAL, GCG_FUNCTIONCALLFAILED), "exportChannels(): a call to createImage() has failed for alpha channel. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

#if FLOATING
  NUMBERTYPE inv = (NUMBERTYPE) (1.0 / 255.0);
#endif

  // is paletted?
  if(this->palettecolors > 0 && this->palette != NULL) {
  	// Exports palette colors
    int pixelsperbyte = (8 / this->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << this->bpp) - 1; // Computes de index mask

    // Set the indices.
    for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
      unsigned char *srcdata = &this->data[currentRow * this->rowsize];
      unsigned int pos = currentRow * this->width;
      for(unsigned int i = 0; i < width; srcdata++)
        for(int j = pixelsperbyte-1; j >= 0; j--, i++, pos++) {
          unsigned char *color = &this->palette[(((*srcdata) >> (j * this->bpp)) & bitmask) * sizeof(RGBQUAD)];
#if FLOATING
          if(red) red->data[pos] = (NUMBERTYPE) color[0] * inv;
          if(green) green->data[pos] = (NUMBERTYPE) color[1] * inv;
          if(blue) blue->data[pos] = (NUMBERTYPE) color[2] * inv;
          if(alpha) alpha->data[pos] = (NUMBERTYPE) color[3] * inv;
#else
          if(red) red->data[pos] = (NUMBERTYPE) color[0];
          if(green) green->data[pos] = (NUMBERTYPE) color[1];
          if(blue) blue->data[pos] = (NUMBERTYPE) color[2];
          if(alpha) alpha->data[pos] = (NUMBERTYPE) color[3];
#endif
        }
    }
  } else
    if(this->bpp == 24) {
    	// Exports 24 bpp colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *srcdata   = &this->data[row * rowsize];
        unsigned int pos = row * this->width;
    		for(unsigned int i = 0; i < width; i++, srcdata += 3, pos++) {
#if FLOATING
          if(red) red->data[pos] = (NUMBERTYPE) srcdata[0] * inv;
          if(green) green->data[pos] = (NUMBERTYPE) srcdata[1] * inv;
          if(blue) blue->data[pos] = (NUMBERTYPE) srcdata[2] * inv;
#else
          if(red) red->data[pos] = (NUMBERTYPE) srcdata[0];
          if(green) green->data[pos] = (NUMBERTYPE) srcdata[1];
          if(blue) blue->data[pos] = (NUMBERTYPE) srcdata[2];
#endif
        }
      }
    } else
      	if(this->bpp == 32) {
          // Exports 32 bpp colors
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *srcdata   = &this->data[row * rowsize];
            unsigned int pos = row * this->width;
        		for(unsigned int i = 0; i < width; i++, srcdata += 4, pos++) {
#if FLOATING
              if(red) red->data[pos] = (NUMBERTYPE) srcdata[0] * inv;
              if(green) green->data[pos] = (NUMBERTYPE) srcdata[1] * inv;
              if(blue) blue->data[pos] = (NUMBERTYPE) srcdata[2] * inv;
              if(alpha) alpha->data[pos] = (NUMBERTYPE) srcdata[3] * inv;
#else
              if(red) red->data[pos] = (NUMBERTYPE) srcdata[0];
              if(green) green->data[pos] = (NUMBERTYPE) srcdata[1];
              if(blue) blue->data[pos] = (NUMBERTYPE) srcdata[2];
              if(alpha) alpha->data[pos] = (NUMBERTYPE) srcdata[3];
#endif
            }
          }
        } else
            if(this->bpp == 16) {
              // Exports 16 bpp colors
              for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
                unsigned short *srccolor = (unsigned short*) &this->data[currentRow * rowsize];
                unsigned int pos = currentRow * this->width;
                for(unsigned int i = 0; i < width; i++, srccolor++, pos++) {
#if FLOATING
                  if(red) red->data[pos] = (NUMBERTYPE) ((((*srccolor & colormasks[0]) >> rightshift[0]) * 255) / max[0]) * inv;
                  if(green) green->data[pos] = (NUMBERTYPE) ((((*srccolor & colormasks[1]) >> rightshift[1]) * 255) / max[1]) * inv;
                  if(blue) blue->data[pos] = (NUMBERTYPE) ((((*srccolor & colormasks[2]) >> rightshift[2]) * 255) / max[2]) * inv;
                  if(alpha) alpha->data[pos] = (NUMBERTYPE) ((colormasks[3]) ? ((((*srccolor & colormasks[3]) >> rightshift[3]) * 255) / max[3]) : 0) * inv;
#else
                  if(red) red->data[pos] = (NUMBERTYPE) (((*srccolor & colormasks[0]) >> rightshift[0]));
                  if(green) green->data[pos] = (NUMBERTYPE) (((*srccolor & colormasks[1]) >> rightshift[1]));
                  if(blue) blue->data[pos] = (NUMBERTYPE) (((*srccolor & colormasks[2]) >> rightshift[2]));
                  if(alpha) alpha->data[pos] = (NUMBERTYPE) ((colormasks[3]) ? ((*srccolor & colormasks[3]) >> rightshift[3]) : (int) 0);
#endif
                }
              }
            } else {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "exportChannels(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", this->bpp, GCG_VERSION, basename((char*)__FILE__), __LINE__);
                    return false;
                  }

  // Ok, it's done
	return true;
}


bool gcgIMAGE::importChannels(gcgDISCRETE2D<NUMBERTYPE> *red, gcgDISCRETE2D<NUMBERTYPE> *green, gcgDISCRETE2D<NUMBERTYPE> *blue, gcgDISCRETE2D<NUMBERTYPE> *alpha) {
  // Check source signals
  unsigned int _width, _height;
  if(red) {_width = red->width; _height = red->height; }
  else if(green) {_width = green->width; _height = green->height; }
       else if(blue) {_width = blue->width; _height = blue->height; }
            else if(alpha) {_width = alpha->width; _height = alpha->height; }
                 else {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "importChannels(): all sources are NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
                    return false; // Nothing to do
                 }

  if(red) if(red->width != _width || red->height != _height) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "importChannels(): red source is incompatible with another source. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(green) if(green->width != _width || green->height != _height) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "importChannels(): green source is incompatible with another source. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(blue) if(blue->width != _width || blue->height != _height) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "importChannels(): blue source is incompatible with another source. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(alpha) if(alpha->width != _width || alpha->height != _height) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "importChannels(): alpha source is incompatible with another source. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check destination;
  if(alpha) {
    // Alpha channel is needed. Destination must have 16 or 32 bpp.
    if(this->bpp != 16 || this->bpp != 32)
      if(!this->createImage(_width, _height, 32, true)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "importChannels(): a call to createImage() has failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
        return false;
      }
  } else
    // Destination must be 16, 24 or 32 bpp
    if(this->bpp != 16 || this->bpp != 24 || this->bpp != 32)
      if(!this->createImage(_width, _height, 24, false)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "importChannels(): a call to createImage() has failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
        return false;
      }

  // Check size
  if(this->width != _width || this->height != _height)
    if(!this->createImage(_width, _height, this->bpp, (alpha != NULL))) return false;

  unsigned int newrowsize = BITMAPWIDTHBYTES(this->width, 8);

  if(this->bpp == 24) {
    // Imports 24 bpp colors
    for(unsigned int row = 0; row < height; row++) {
      unsigned char *dstdata   = &this->data[row * rowsize];
      unsigned int pos = row * this->width;
      for(unsigned int i = 0; i < width; i++, dstdata += 3, pos++) {
#if FLOATING
        dstdata[0] = (unsigned char) ((red) ? ((red->data[pos] < 0) ? 0 : ((red->data[pos] >  1.0) ? 255 : (red->data[pos] * 255.0))) : 0);
        dstdata[1] = (unsigned char) ((green) ? ((green->data[pos] < 0) ? 0 : ((green->data[pos] >  1.0) ? 255 : (green->data[pos] * 255.0))) : 0);
        dstdata[2] = (unsigned char) ((blue) ? ((blue->data[pos] < 0) ? 0 : ((blue->data[pos] >  1.0) ? 255 : (blue->data[pos] * 255.0))) : 0);
#else
        dstdata[0] = (unsigned char) ((red) ? ((red->data[pos] < 0) ? 0 : ((red->data[pos] >  255) ? 255 : red->data[pos])) : 0);
        dstdata[1] = (unsigned char) ((green) ? ((green->data[pos] < 0) ? 0 : ((green->data[pos] >  255) ? 255 : green->data[pos])) : 0);
        dstdata[2] = (unsigned char) ((blue) ? ((blue->data[pos] < 0) ? 0 : ((blue->data[pos] >  255) ? 255 : blue->data[pos])) : 0);
#endif
      }
    }
  } else
      if(this->bpp == 32) {
        // Imports 32 bpp colors
        for(unsigned int row = 0; row < height; row++) {
          unsigned char *dstdata   = &this->data[row * rowsize];
          unsigned int pos = row * newrowsize;
          for(unsigned int i = 0; i < width; i++, dstdata += 4, pos++) {
#if FLOATING
            dstdata[0] = (unsigned char) ((red) ? ((red->data[pos] < 0) ? 0 : ((red->data[pos] >  1.0) ? 255 : (red->data[pos] * 255.0))) : 0);
            dstdata[1] = (unsigned char) ((green) ? ((green->data[pos] < 0) ? 0 : ((green->data[pos] >  1.0) ? 255 : (green->data[pos] * 255.0))) : 0);
            dstdata[2] = (unsigned char) ((blue) ? ((blue->data[pos] < 0) ? 0 : ((blue->data[pos] >  1.0) ? 255 : (blue->data[pos] * 255.0))) : 0);
            dstdata[3] = (unsigned char) ((alpha) ? ((alpha->data[pos] < 0) ? 0 : ((alpha->data[pos] >  1.0) ? 255 : (alpha->data[pos] * 255.0))) : 0);
#else
            dstdata[0] = (unsigned char) ((red) ? ((red->data[pos] < 0) ? 0 : ((red->data[pos] >  255) ? 255 : red->data[pos])) : 0);
            dstdata[1] = (unsigned char) ((green) ? ((green->data[pos] < 0) ? 0 : ((green->data[pos] >  255) ? 255 : green->data[pos])) : 0);
            dstdata[2] = (unsigned char) ((blue) ? ((blue->data[pos] < 0) ? 0 : ((blue->data[pos] >  255) ? 255 : blue->data[pos])) : 0);
            dstdata[3] = (unsigned char) ((alpha) ? ((alpha->data[pos] < 0) ? 0 : ((alpha->data[pos] >  255) ? 255 : alpha->data[pos])) : 0);
#endif
          }
        }
      } else
          if(this->bpp == 16) {
            // Imports 16 bpp colors
            for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
              unsigned short *dstcolor = (unsigned short*) &this->data[currentRow * rowsize];
              unsigned int pos = currentRow * newrowsize;
              for(unsigned int i = 0; i < width; i++, dstcolor++, pos++) {
#if FLOATING
                int cred   = (int) ((red) ? ((red->data[pos] < 0) ? 0 : ((red->data[pos] > 1.0) ? 255 : (red->data[pos] * 255.0))) : 0);
                int cgreen = (int) ((green) ? ((green->data[pos] < 0) ? 0 : ((green->data[pos] > 1.0) ? 255 : (green->data[pos] * 255.0))) : 0);
                int cblue  = (int) ((blue) ? ((blue->data[pos] < 0) ? 0 : ((blue->data[pos] >  1.0) ? 255 : (blue->data[pos] * 255.0))) : 0);
                int calpha = (int) ((alpha) ? ((alpha->data[pos] < 0) ? 0 : ((alpha->data[pos] > 1.0) ? 255 : (alpha->data[pos] * 255.0))) : 0);
#else
                int cred   = (int) ((red) ? ((red->data[pos] < 0) ? 0 : ((red->data[pos] > 255) ? 255 : red->data[pos])) : 0);
                int cgreen = (int) ((green) ? ((green->data[pos] < 0) ? 0 : ((green->data[pos] > 255) ? 255 : green->data[pos])) : 0);
                int cblue  = (int) ((blue) ? ((blue->data[pos] < 0) ? 0 : ((blue->data[pos] >  255) ? 255 : blue->data[pos])) : 0);
                int calpha = (int) ((alpha) ? ((alpha->data[pos] < 0) ? 0 : ((alpha->data[pos] >  255) ? 255 : alpha->data[pos])) : 0);
#endif
                *dstcolor = (unsigned short) ((((cred  * max[0]) / 255) << rightshift[0]) |
                              (((cgreen * max[1]) / 255) << rightshift[1]) |
                              (((cblue  * max[2]) / 255) << rightshift[2]) |
                              (((calpha * max[3]) / 255) << rightshift[3]));
              }
            }
          } else {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "importChannels(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", this->bpp, GCG_VERSION, basename((char*)__FILE__), __LINE__);
                    return false;
                  }

  // Ok, it's done
	return true;
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
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       float
  #define CROSSTYPE_MIN   -((float) INF)
  #define CROSSTYPE_MAX   ((float) INF)
  #define COERSIONTYPE    float
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       double
  #define CROSSTYPE_MIN   -INF
  #define CROSSTYPE_MAX   INF
  #define COERSIONTYPE    double
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC double
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       short
  #define CROSSTYPE_MIN   SHRT_MIN
  #define CROSSTYPE_MAX   SHRT_MAX
  #define COERSIONTYPE    float
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       int
  #define CROSSTYPE_MIN   INT_MIN
  #define CROSSTYPE_MAX   INT_MAX
  #define COERSIONTYPE    float
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       long
  #define CROSSTYPE_MIN   LONG_MIN
  #define CROSSTYPE_MAX   LONG_MAX
  #define COERSIONTYPE    float
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

// double crossed with all types
#elif TYPECOUNTER == 2

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       float
  #define CROSSTYPE_MIN   -((float) INF)
  #define CROSSTYPE_MAX   ((float) INF)
  #define COERSIONTYPE    double
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC double
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       double
  #define CROSSTYPE_MIN   -INF
  #define CROSSTYPE_MAX   INF
  #define COERSIONTYPE    double
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC double
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       short
  #define CROSSTYPE_MIN   SHRT_MIN
  #define CROSSTYPE_MAX   SHRT_MAX
  #define COERSIONTYPE    double
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC double
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       int
  #define CROSSTYPE_MIN   INT_MIN
  #define CROSSTYPE_MAX   INT_MAX
  #define COERSIONTYPE    double
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC double
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       long
  #define CROSSTYPE_MIN   LONG_MIN
  #define CROSSTYPE_MAX   LONG_MAX
  #define COERSIONTYPE    double
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC double
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

// short crossed with all types
#elif TYPECOUNTER == 3

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       float
  #define CROSSTYPE_MIN   -((float) INF)
  #define CROSSTYPE_MAX   ((float) INF)
  #define COERSIONTYPE    float
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       double
  #define CROSSTYPE_MIN   -INF
  #define CROSSTYPE_MAX   INF
  #define COERSIONTYPE    double
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC double
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       short
  #define CROSSTYPE_MIN   SHRT_MIN
  #define CROSSTYPE_MAX   SHRT_MAX
  #define COERSIONTYPE    short
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       int
  #define CROSSTYPE_MIN   INT_MIN
  #define CROSSTYPE_MAX   INT_MAX
  #define COERSIONTYPE    int
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       long
  #define CROSSTYPE_MIN   LONG_MIN
  #define CROSSTYPE_MAX   LONG_MAX
  #define COERSIONTYPE    long
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

// int crossed with all types
#elif TYPECOUNTER == 4

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       float
  #define CROSSTYPE_MIN   -((float) INF)
  #define CROSSTYPE_MAX   ((float) INF)
  #define COERSIONTYPE    float
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       double
  #define CROSSTYPE_MIN   -INF
  #define CROSSTYPE_MAX   INF
  #define COERSIONTYPE    double
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC double
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       short
  #define CROSSTYPE_MIN   SHRT_MIN
  #define CROSSTYPE_MAX   SHRT_MAX
  #define COERSIONTYPE    int
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       int
  #define CROSSTYPE_MIN   INT_MIN
  #define CROSSTYPE_MAX   INT_MAX
  #define COERSIONTYPE    int
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       long
  #define CROSSTYPE_MIN   LONG_MIN
  #define CROSSTYPE_MAX   LONG_MAX
  #define COERSIONTYPE    long
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  static inline COERSIONTYPE scalarproductsymmetricnorepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductsymmetricrepeat1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductperiodic1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductclamp1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline COERSIONTYPE scalarproductzero1D(int column1, int size1, CROSSTYPE *data1, int size2, NUMBERTYPE *data2, int origin2);
  static inline CROSSTYPE sample2D(int i, int j, int width, int height, int extensionX, int extensionY, CROSSTYPE *data);
  #include"signalcross.cpp"

// long crossed with all types
#elif TYPECOUNTER == 5

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       float
  #define CROSSTYPE_MIN   -((float) INF)
  #define CROSSTYPE_MAX   ((float) INF)
  #define COERSIONTYPE    float
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       double
  #define CROSSTYPE_MIN   -INF
  #define CROSSTYPE_MAX   INF
  #define COERSIONTYPE    double
  #define FLOATINGCROSS   1
  #define FLOATARITHMETIC double
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       short
  #define CROSSTYPE_MIN   SHRT_MIN
  #define CROSSTYPE_MAX   SHRT_MAX
  #define COERSIONTYPE    long
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       int
  #define CROSSTYPE_MIN   INT_MIN
  #define CROSSTYPE_MAX   INT_MAX
  #define COERSIONTYPE    long
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"

  #undef CROSSTYPE
  #undef CROSSTYPE_MIN
  #undef CROSSTYPE_MAX
  #undef COERSIONTYPE
  #undef FLOATINGCROSS
  #undef FLOATARITHMETIC
  #define CROSSTYPE       long
  #define CROSSTYPE_MIN   LONG_MIN
  #define CROSSTYPE_MAX   LONG_MAX
  #define COERSIONTYPE    long
  #define FLOATINGCROSS   0
  #define FLOATARITHMETIC float
  #include"signalcross.cpp"
#endif


// Loads several times to automatically include classes with distinct types
#if TYPECOUNTER < 5
  #include "signalmain.cpp"
#endif




/*
/// @brief Decomposes a 2D signal into its wavelet components.
/// @param[in] H low pass wavelet filter for decomposition (as 1D signal).
/// @param[in] G high pass wavelet filter for decomposition (as 1D signal).
/// @param[out] outlow 2D array (as 2D signal) that receives the scale components (low pass)
/// @return true if decomposition is successful.
/// @see DWTComposition
bool DWTDecomposition(gcgDISCRETE1D<NUMBERTYPE> *H,  gcgDISCRETE1D<NUMBERTYPE> *G) {
    // This image is valid?
    if (data == NULL || width == 0 || height == 0 || bpp == 0) {
        //errorcode = GCG_INVALID_OBJECT;
        return NULL;
    }

    //is the width or the height odd?
    bool addCol = (this->width % 2 == 1);
    bool addRow = (this->height % 2 == 1);
    //do we have to add one row or collumn?if so, we will create new images with larger row or collumn to do the convolution
    int w, h;
    w = this->width  + (int) addCol;
    h = this->height + (int) addRow;

    // Creates space for dwt image
    gcgIMAGE  *dwt = new gcgIMAGE(), *dwt2 = new gcgIMAGE(), *result = new gcgIMAGE();
    if((dwt->createImage(w, h, 8, false) && dwt2->createImage(w, h, 8, false)) && result->createImage(this->width, this->height, 8, false)) {
      this->periodicTransRows(dwt, nH, H, nG, G, addCol, addRow);
      dwt->periodicTransCols(dwt2, nH, H, nG, G, addCol, addRow);
    } else {
      if (dwt) delete dwt;
      if (dwt2) delete dwt2;
      if (result) delete result;
      //errorcode = GCG_INTERNAL_ERROR;
      return NULL;
    }

    // Remove new samples
    for(unsigned int i =0; i < result->height; i++)
      memcpy(&result->data[i * result->rowsize], &dwt2->data[i * dwt2->rowsize], result->rowsize);

    // frees first image
    delete dwt;
    delete dwt2;

    // Returns dwt coefficients
    return result;
}

////////////////////////////////////////////////////////////
// Composes an image from its wavelet components.
////////////////////////////////////////////////////////////
gcgIMAGE *gcgIMAGE::DWTComposition(int nscales, int ntH, float *tH, int ntG, float *tG) {
  // This image is valid?
  if (data == NULL || width == 0 || height == 0 || bpp == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return NULL;
  }

  // is really paletted?
  if (bpp != 8 || !isGrayscale()) {
    //errorcode = GCG_UNSUPPORTED_FORMAT;
    return NULL;
  }

  //is the width or the height odd?
  bool addCol = (this->width % 2 == 1);
  bool addRow = (this->height % 2 == 1);

  //do we have to add one row or collumn?
  int w, h;
  w = this->width  + (addCol) ? 1 : 0;
  h = this->height + (addRow) ? 1 : 0;


  // Creates space for dwt image
  gcgIMAGE  *dwt = new gcgIMAGE(), *dwt2 = new gcgIMAGE(), *result = new gcgIMAGE();
  if((dwt->createImage(w, h, 8, false) && dwt2->createImage(w, h, 8, false)) && result->createImage(this->width, this->height, 8, false)) {
    this->periodicSynthCols(dwt, ntH, tH, ntG, tG, addRow);
    dwt->periodicSynthRows(dwt2, ntH, tH, ntG, tG, addCol);
  } else {
    if (dwt) delete dwt;
    if (dwt2) delete dwt2;
    if (result) delete result;
    //errorcode = GCG_INTERNAL_ERROR;
    return NULL;
  }

  // Remove new samples
  for(unsigned int i =0; i < result->height; i++)
    memcpy(&result->data[i * result->rowsize], &dwt2->data[i * dwt2->rowsize], result->rowsize);

  // frees first image
  delete dwt;
  delete dwt2;

  // Returns the image reconstructed from dwt coefficients
  return result;
}




////////////////////////////////////////////////////////////
// Do the decomposition in y axis.
// The dimensions of the destiny image here are always even. If it was odd,
// we are now creating a new image with another row and/or collumn.
////////////////////////////////////////////////////////////
void gcgIMAGE::periodicTransRows(gcgIMAGE *dest, int nH, float *H, int nG, float *G,bool addCol,bool addRow) {
    unsigned char *row = this->data;
    const unsigned int centerH = ((nH % 2) ? (nH - 1) : (nH - 2)) / 2;
    const unsigned int centerG = ((nG % 2) ? (nG - 3) : (nG - 2)) / 2;
    const unsigned int windoww = dest->width;
    const unsigned int windowh = (addRow) ? dest->height - 1 : dest->height;
    const bool iseven = windoww % 2 == 0;
    const unsigned int halfw = ((iseven)  ? windoww : windoww + 1) >> 1; // Compute half width
    float s, d;
    int ns;
    float newSample;
    int n = 0;

    // For each source row...
    for(register unsigned int y = 0; y < windowh; y++, row += rowsize) {
        s = newSample = d = 0.0f;
        ns = 0;
        if(addCol) {
          //lets discover the new sample of this row
          //k is the last position on the high-pass convolution
          unsigned int k = windoww -2;
          for (int m = 0; m < nG; m++) {
            n = (int) k + (int) ((int) m - (int) (centerG));
            n = (n < 0) ? windoww + n: n;
            if(n == (int) windoww - 1) {
              newSample = G[m];
              continue;
            }
            s += G[m] * row[n % windoww]; // Periodic extension
          }
          newSample = -(s / newSample);

          // Store result
          ns =(int) (newSample + 0.5f);//(s >=0)?(s + 0.5f):(s-0.5f); // Low frequency
          ns = (ns >= 255) ? 255 : ((ns <= 0) ? 0 : (int) ns);
        }

        //now we can do the convolution over the rows
        for(register unsigned int k = 0, i = 0; i < halfw; i++, k += 2) { // Makes the diadic convolution
          s = d = 0.0f;

          unsigned char *coef = &dest->data[y * dest->rowsize + i];
          // Realiza a convolução do filtro G (wavelet)
          for (int m = 0; m < nG; m++) {
            n = (int) k + (int) ((int) m - (int) centerG);
            n = (n < 0) ? windoww + n : n;
            if (n == (int) windoww - 1 && addCol) {
              d += G[m] * ns;
              continue;
            }
            d += G[m] * row[n % windoww]; // Periodic extension
          }

          // Store result
          int di = 128 + (int)(d + 0.5f); // high frequency
          coef[halfw] = (di >= 255) ? 255 : ((di <= 0) ? 0 : (unsigned char) di);

          // Realiza a convolução do filtro H (escalamento)
          for (int m = 0; m < nH; m++) {
            n = (int) k + (int) ((int) m - (int) centerH);
            n = (n < 0) ? windoww + n : n;
            if (n == (int) windoww - 1 && addCol) {
              s += H[m] * ns;
              continue;
            }
            s += H[m] * row[n % windoww]; // Periodic extension
          }

          // Store result
          int si =(int) (s + 0.5f);//(s >=0)?(s + 0.5f):(s-0.5f); // Low frequency
          coef[0] = (si >= 255) ? 255 : ((si <= 0) ? 0 : (unsigned char) si);
        }
    }
}


////////////////////////////////////////////////////////////
// Do the decomposition in x axis.
////////////////////////////////////////////////////////////
void gcgIMAGE::periodicTransCols(gcgIMAGE *dest, int nH, float *H, int nG, float *G, bool addCol, bool addRow) {
    const unsigned char *row = this->data;
    const unsigned int centerH = ((nH % 2) ? (nH - 1) : (nH - 2)) / 2;
    const unsigned int centerG = ((nG % 2) ? (nG - 3) : (nG - 2)) / 2;
    const unsigned int windoww = (addCol) ? dest->width - 1 : dest->width;
    const unsigned int windowh = dest->height;
    const bool iseven = windowh % 2 == 0;
    const unsigned int halfh = ((iseven)  ? windowh : windowh + 1) >> 1; // Compute half height
    float s, d, newSample;
    int n, ns;

    // For each source column...
    for(register unsigned int k = 0; k < windoww; k++) {
        s = newSample = 0.0f;
        ns = 0;
        //lets compute the new sample of this collumn
        if (addRow) {
          unsigned int  y = windowh - 2;
          for(int m = 0; m < nG; m++) {
            n = (int) y + (int) ((int) m - (int) centerG);
            n = (n < 0) ? windowh + n : n;
            if (n == (int) windowh - 1) {
                newSample = G[m];
                continue;
            }
            s += G[m] * this->data[(n % (windowh)) * rowsize + k]; // Periodic extension
          }
          newSample = -(s/newSample);

          ns = (int) (newSample + 0.5f); // Low frequency
          ns = (ns >= 255) ? 255 : ((ns <= 0) ? 0 : (int) ns);
        }

        //now we continue with the convolution over the collumns
        for(register unsigned int y = 0, i = 0; i < halfh; i++, y += 2) { // Makes the diadic convolution
          s = d = 0.0f;

          unsigned char *coef = &dest->data[i * dest->rowsize + k];
          // Realiza a convolução do filtro G (wavelet)
          for (int m = 0; m < nG; m++) {
            n = (int) y +(int) ((int) m - (int) centerG);
            n = (n < 0) ? windowh + n : n;
            if (n == (int) windowh - 1 && addRow) {
              d += G[m] * ns;
              continue;
            }
            d += G[m] * row[(n % windowh) * rowsize + k]; // Periodic extension
          }

          // Store result
          int di = 128 + (int) (d + 0.5f);//(d>=0)?(d + 0.5f):(d - 0.5f));
          coef[halfh * rowsize] = (di >= 255) ? 255 : ((di <= 0) ? 0 : (unsigned char) di); // High frequency

          // Realiza a convolução do filtro H (escalamento)
          for (int m = 0; m < nH; m++) {
            n = (int) y  + (int) ((int) m - (int) centerH);
            n = (n < 0) ? windowh + n : n;
            if (n == (int) windowh - 1 && addRow) {
              s += H[m] * ns ;
              continue;
            }
            s += H[m] * row[(n % windowh) * rowsize + k]; // Periodic extension
          }

          // Store result
          int si = (int) (s + 0.5f);//(s>=0)?(s + 0.5f):(s - 0.5f);
          coef[0] = (si >= 255) ? 255 : ((si <= 0) ? 0 : (unsigned char) si); // Low frequency
        }
    }
}


////////////////////////////////////////////////////////////
// Do the composition in y axis.
////////////////////////////////////////////////////////////
void gcgIMAGE::periodicSynthRows(gcgIMAGE *dest, int ntH, float *tH, int ntG, float *tG, bool addCol) {
    const int centertH = (int) ((ntH % 2) ? (ntH - 1) : (ntH - 2)) / 2;
    const int centertG = (int) ((ntG % 2) ? (ntG - 3) : (ntG - 2)) / 2;
    const unsigned int windoww = dest->width;
    const unsigned int windowh = dest->height;
    const bool iseven = windoww % 2 == 0;
    const unsigned int halfw = ((iseven)  ? windoww : windoww + 1) >> 1; // Compute half width
    float s2k, s2k1;
    int n;

    // For each source row...
    for (register unsigned int y = 0; y < windowh; y++) {
        for (register unsigned int k = 0; k < halfw; k++) {
            s2k = s2k1 = 0.0f;

            // Even positions (low pass)
            for (int m = ntH - 1; m >= 0; m--) {
                register int pos = m - (int) centertH;
                n = (int) k - ((pos % 2) ? pos + 1 : pos) / 2;
                n = (n < 0) ? halfw + n : n;
                register unsigned char *pixel = &this->data[y * rowsize + (n % halfw)]; // Periodic extension
                if (pos % 2 == 0) s2k += tH[m] * pixel[0]; // Upsample low pass band
            }

            // Even positions (high pass)
            for (int m = ntG - 1; m >= 0; m--) {
                register int pos = m - (int) centertG;
                n = (int) k - ((pos % 2) ? pos + 1 : pos) / 2;
                n = (n < 0) ? halfw + n : n;
                register unsigned char *pixel = &this->data[y * rowsize + (n % halfw)]; // Periodic extension
                if (pos % 2 == 0) {
                    if (n == (int) halfw - 1 && addCol) continue;
                    s2k += tG[m] * (-128 + (int) pixel[halfw] ); // Add detail
                }
            }

            // Store even
            int s2ki  = (int)(s2k  + s2k  + 0.5f);
            unsigned char *coef = &dest->data[y * dest->rowsize + (k << 1)];
            coef[0] = (s2ki  >= 255) ? 255 : ((s2ki  <= 0) ? 0 : (unsigned char) s2ki);

            // Odd positions (low pass)
            for (int m = ntH - 1; m >= 0; m--) {
                register int pos = m - (int) centertH;
                n = (int) k + 1 - ((pos % 2) ? pos + 1 : pos) / 2;
                n = (n < 0) ? halfw + n : n;
                register unsigned char *pixel = &this->data[y * rowsize + (n % halfw)]; // Periodic extension
                if (pos % 2 != 0) s2k1 += tH[m] * pixel[0];  // Upsample low pass band
            }

            // Odd positions (high pass)
            for (int m = ntG - 1; m >= 0; m--) {
                register int pos = m - (int) centertG;
                n = (int) k + 1 - ((pos % 2) ? pos + 1 : pos) / 2;
                n = (n < 0) ? halfw + n: n;
                register unsigned char *pixel = &this->data[y * rowsize + (n % halfw)]; // Periodic extension
                if (pos % 2 != 0) {
                    if (n == (int) halfw - 1 && addCol) continue;
                    s2k1 += tG[m] * (-128 + (int) pixel[halfw]); // Add detail
                }
            }

            // Store odd
            int s2k1i = (int) (s2k1 + s2k1 + 0.5f);
            coef[1] = (s2k1i >= 255) ? 255 : ((s2k1i <= 0) ? 0 : (unsigned char) s2k1i);
        }
    }
}


////////////////////////////////////////////////////////////
// Do the composition in x axis.
////////////////////////////////////////////////////////////
void gcgIMAGE::periodicSynthCols(gcgIMAGE *dest, int ntH, float *tH, int ntG, float *tG, bool addRow) {
  const int centertH = (int) ((ntH % 2) ? (ntH - 1) : (ntH - 2)) / 2;
  const int centertG = (int) ((ntG % 2) ? (ntG - 3) : (ntG - 2)) / 2;
  const unsigned int windoww = dest->width;
  const unsigned int windowh = dest->height;
  const bool iseven = windowh % 2 == 0;
  const unsigned int halfh = ((iseven)  ? windowh : windowh + 1) >> 1; // Compute half height
  double s2y, s2y1;
  int n;

  // For each source column...
  for (register unsigned int k = 0; k < windoww; k++)
    for (register unsigned int y = 0; y < halfh; y++) {
      s2y = s2y1 = 0.0f;

      // Even positions(low pass)
      for (int m = ntH - 1; m >= 0; m--) {
        register int pos = m - (int) centertH;
        n = (int) y - ((pos % 2) ? pos + 1 : pos) / 2;
        n = (n < 0) ? halfh + n : n;
        register unsigned char *pixel = &this->data[(n % halfh) * rowsize + k]; // Periodic extension
        if (pos % 2 == 0) s2y += tH[m] * pixel[0]; // Upsample low pass band
      }

      // Even positions(high pass)
      for (int m = ntG - 1; m >= 0; m--) {
        register int pos = m - (int) centertG;
        n = (int) y - ((pos % 2) ? pos + 1 : pos) / 2;
        n = (n < 0) ? halfh + n : n;
        register unsigned char *pixel = &this->data[(n % halfh) * rowsize + k]; // Periodic extension
        if (pos % 2 == 0) {
          if (n == (int) halfh - 1 && addRow) continue;
          s2y += tG[m] * (-128 + (int) ((iseven || n % halfh < halfh - 1) ? pixel[halfh * rowsize] : this->data[((n % (halfh - 1)) + halfh) * rowsize + k])); // Add detail
        }
      }

      // Store even
      int s2yi  = (int) (s2y  + s2y  + 0.5f);
      unsigned char *coef = &dest->data[(y << 1) * dest->rowsize + k];
      coef[0] = (s2yi  >= 255) ? 255 : ((s2yi  <= 0) ? 0 : (unsigned char) s2yi);

      // Odd positions (low pass)
      for (int m = ntH - 1; m >= 0; m--) {
        register int pos = m - (int) centertH;
        n = (int) y + 1 - ((pos % 2) ? pos + 1 : pos) / 2;
        n = (n < 0) ? halfh + n : n;
        register unsigned char *pixel = &this->data[(n % halfh) * rowsize + k]; // Periodic extension
        if (pos % 2 != 0) s2y1 += tH[m] * pixel[0];  // Upsample low pass band
      }

      // Odd positions (high pass)
      for (int m = ntG - 1; m >= 0; m--) {
        register int pos = m - (int) centertG;
        n = (int) y + 1 - ((pos % 2) ? pos + 1 : pos) / 2;
        n = (n < 0) ? halfh + n : n;
        register unsigned char *pixel = &this->data[(n % halfh) * rowsize + k]; // Periodic extension
        if (pos % 2 != 0) {
          if (n == (int) halfh - 1 && addRow) continue;
          s2y1 +=tG[m] * (-128 + (int) pixel[halfh * rowsize]); // Add detail
        }
      }

      // Store odd
      int s2y1i = (int) (s2y1 + s2y1 + 0.5f);
      coef[rowsize] = (s2y1i >= 255) ? 255 : ((s2y1i <= 0) ? 0 : (unsigned char) s2y1i);
    }
}


*/
