/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    gcgIMAGE: class for reading, writing and converting images.

    Marcelo Bernardes Vieira
    Tássio Knop de Castro
    Virgínia Fernandes Mota
    Thales Luiz Sabino
    Eder de Almeida Perez
**************************************************************************************/

#include "system.h"

// Image signatures
static const DWORD           SIGNATURE_BMP = 0x4D42;
static const unsigned short  SIGNATURE_PCX = 0x050A;
static const unsigned char   SIGNATURE_TGA_UNCOMPRESSED[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const unsigned char   SIGNATURE_TGA_COMPRESSED[12] = {0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const unsigned char   SIGNATURE_TGA_UNCOMPRESSED8BIT[12] = {0, 1, 1, 0, 0, 0, 1, 24, 0, 0, 0, 0};

///////////////////////////////////////////////////////////////////////

gcgIMAGE::gcgIMAGE() {
  	data = NULL;
  	palette = NULL;
  	width = 0;
  	height = 0;
  	bpp = 0;
  	rowsize = 0;
  	palettecolors = 0;
    memset(colormasks, 0, sizeof(colormasks));
    memset(rightshift, 0, sizeof(rightshift));
    memset(nbits, 0, sizeof(nbits));
    memset(max, 0, sizeof(max));
}

gcgIMAGE::~gcgIMAGE() {
  destroyImage();
}

bool gcgIMAGE::createImage(unsigned int iwidth, unsigned int iheight, unsigned int ibpp, bool usealpha) {
  // Check the parameters
  if(iheight == 0 || iwidth == 0 ||
    (ibpp != 1 && ibpp != 2 && ibpp != 4 && ibpp != 8 && ibpp != 16 && ibpp != 24 && ibpp != 32)) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "createImage(): unsupported dimensions (%d, %d) or bits per pixel (%d). (%s:%d)", iwidth, iheight, ibpp, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Release previous resources
  if(!destroyImage()) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_RELEASEERROR), "createImage(): could not release old image. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	// Image information
	width   = iwidth;
	height  = iheight;
	bpp     = ibpp;
	rowsize = BITMAPWIDTHBYTES(width, bpp);

 	// Create memory for image data
	this->data = new unsigned char [rowsize * height];
	if(this->data == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "createImage(): image data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
	  return false;
	}
	memset(this->data, 0, rowsize * height);

	// Initiate the palette
	if(bpp < 16) {
    palettecolors = (0x01 << bpp);
  	this->palette = new unsigned char [palettecolors * sizeof(RGBQUAD)];
  	if(this->palette == NULL) {
  	  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "createImage(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
  	  destroyImage();
  	  return false;
  	}

  	// Create an uniform grayscale palette
  	for(unsigned int i = 0, delta = 255 / (palettecolors - 1); i < palettecolors; i++) {
      int pos = i * sizeof(RGBQUAD);
      palette[pos] = palette[pos + 1] = palette[pos + 2] = (unsigned char) (i * delta);
      palette[pos + 3] = 0;
    }
  } else {
    palettecolors = 0;
    palette = NULL;
  }

  // Initiate color masks
  if(bpp == 16) {
    if(usealpha) {
      colormasks[2] = 15; // Four least significant bits = BLUE
      colormasks[0] = (colormasks[2] << 8);  // RED   = 4 bits
      colormasks[1] = (colormasks[2] << 4);  // GREEN = 4 bits
      colormasks[3] = (colormasks[2] << 12); // ALPHA = 4 bits
      rightshift[0] = 8;  rightshift[1] = 4;  rightshift[2] = 0; rightshift[3] = 12;
      nbits[0] = 4; nbits[1] = 4; nbits[2] = 4; nbits[3] = 4;
      max[0] = 15;  max[1] = 15;  max[2] = 15;  max[3] = 15;
    } else {
      colormasks[2] = 31; // Five least significant bits = BLUE
      colormasks[0] = (colormasks[2] << 11);  // RED   = 5 bits
      colormasks[1] = 63 << 5;                // GREEN = 6 bits
      colormasks[3] = 0x00;                   // ALPHA = 0 bits
      rightshift[0] = 11;  rightshift[1] = 5;  rightshift[2] = 0; rightshift[3] = 0;
      nbits[0] = 5; nbits[1] = 6; nbits[2] = 5; nbits[3] = 0;
      max[0] = 31;  max[1] = 63;  max[2] = 31;  max[3] = 0;
    }
  } else if(bpp == 24) {
          // Internal representation
          colormasks[0] = 0xff;     // RED
          colormasks[1] = 0xff00;   // GREEN
          colormasks[2] = 0xff0000; // BLUE
          colormasks[3] = 0x00;     // ALPHA
          rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 0;
          nbits[0] = 8;  nbits[1] = 8;  nbits[2] = 8; nbits[3] = 0;
          max[0] = 255;  max[1] = 255;  max[2] = 255;  max[3] = 0;
        } else if(bpp == 32) {
                  // Internal representation
                  colormasks[0] = 0xff;       // RED
                  colormasks[1] = 0xff00;     // GREEN
                  colormasks[2] = 0xff0000;   // BLUE
                  colormasks[3] = 0xff000000; // ALPHA
                  rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 24;
                  nbits[0] = 8; nbits[1] = 8; nbits[2] = 8; nbits[3] = 8;
                  max[0] = 255; max[1] = 255; max[2] = 255; max[3] = 255;
                }

  // Image Created
  return true;
}

bool gcgIMAGE::createSimilar(gcgIMAGE *source) {
  // Check source.
  if(source == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "createSimilar(): source image is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(source->data == NULL || source->width == 0 || source->height == 0 || source->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "createSimilar(): source image is not valid or allocated. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Structural incompatibility
  if(source->width != width || source->height != height || source->bpp != bpp || source->rowsize != rowsize) {
    if(!destroyImage()) { // Release old resources
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_RELEASEERROR), "createSimilar(): could not release old image. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

    // Copy main parameters
    width = source->width;
    height = source->height;
    bpp = source->bpp;
    rowsize = source->rowsize;

    // Duplicate memory for image data
    this->data = new unsigned char [rowsize * height];
    if(data == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "createSimilar(): image data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      destroyImage();
      return false;
    }
  }

  // Incompatible palettes
  if(source->palettecolors != palettecolors) {
    palettecolors = source->palettecolors;
    SAFE_DELETE_ARRAY(palette);

    if(palettecolors > 0) {
      // Allocate palette
      this->palette = new unsigned char [palettecolors * sizeof(RGBQUAD)];
      if(this->palette == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "createSimilar(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
  	    destroyImage();
  	    return false;
      }

      // Duplicate palette data
      memmove(this->palette, source->palette, palettecolors * sizeof(RGBQUAD));
    } else palette = NULL;
  }

  // Color masks: simply copy them
  memcpy(this->colormasks, source->colormasks, sizeof(colormasks));
  memcpy(this->rightshift, source->rightshift, sizeof(rightshift));
  memcpy(this->nbits, source->nbits, sizeof(nbits));
  memcpy(this->max, source->max, sizeof(max));

	// Ok, it's a similar image.
	return true;
}

bool gcgIMAGE::destroyImage() {
  SAFE_DELETE_ARRAY(data);
  SAFE_DELETE_ARRAY(palette);
  width = 0;
  height = 0;
  bpp = 0;
  rowsize = 0;
  palettecolors = 0;
  memset(colormasks, 0, sizeof(colormasks));
  memset(rightshift, 0, sizeof(rightshift));
  memset(nbits, 0, sizeof(nbits));
  memset(max, 0, sizeof(max));
  return true;
}

bool gcgIMAGE::isCompatibleWith(gcgIMAGE *source) {
  // Check source.
  if(source == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "isCompatibleWith(): source image is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(source->data == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "isCompatibleWith(): source image is not valid or allocated. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check equivalence between both images.
  if(source->width != this->width || source->height != this->height || source->bpp != this->bpp ||
     source->rowsize != this->rowsize || source->palettecolors != this->palettecolors ||
     memcmp(this->colormasks, source->colormasks, sizeof(colormasks)) != 0  ||
     memcmp(this->rightshift, source->rightshift, sizeof(rightshift)) != 0 ||
     memcmp(this->nbits, source->nbits, sizeof(nbits)) != 0 ||
     memcmp(this->max, source->max, sizeof(max)) != 0) return false;

  // Check palette
  int *srcpal, *dstpal;
  if(source->bpp <= 8 && (srcpal = (int*) source->palette) != NULL && (dstpal = (int*) this->palette) != NULL)
    for(unsigned int i = 0; i < this->palettecolors; i++, srcpal++, dstpal++)
      if(*srcpal != *dstpal) return false;

  return true;
}

bool gcgIMAGE::isValid() {
  if(this->data == NULL || this->width == 0 || this->height == 0 ||
     (this->bpp != 1 && this->bpp != 2 && this->bpp != 4 && this->bpp != 8 && this->bpp != 16 && this->bpp != 24 && this->bpp != 32)) return false;
  if(this->bpp <= 8 && (this->palettecolors == 0 || this->palette == NULL)) return false;
  return true;
}


bool gcgIMAGE::duplicateImage(gcgIMAGE *source) {
  // Check source.
  if(source == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "duplicateImage(): source image is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(source->data == NULL || source->width == 0 || source->height == 0 || source->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "duplicateImage(): source image is not valid or allocated. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(this == source) return true; // Nothing to do...

  // createSimilar creates only what is needed to make them compatible.
  if(!this->createSimilar(source)) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "duplicateImage(): a call to createSimilar() failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

 	// Copy image data
	memcpy(this->data, source->data, rowsize * height);

	// Ok, it's duplicated.
	return true;
}


bool gcgIMAGE::duplicateSubimage(gcgIMAGE *source, int left, int top, unsigned int _width, unsigned int _height) {
  // Check source.
  if(source == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "duplicateSubimage(): source image is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(source->data == NULL || source->width == 0 || source->height == 0 || source->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "duplicateSubimage(): source image is not valid or allocated. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameters
  if(_width == 0 || _height == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "duplicateSubimage(): invalid dimensions %d x %d. (%s:%d)", _width, _height, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check source and destination
  unsigned char *olddata = source->data;
  unsigned int oldwidth  = source->width;
  unsigned int oldheight = source->height;
  unsigned int oldrowsize = source->rowsize;

  if(this != source) {
    // Source is different than destination
    // Copy main parameters
    this->width = _width;
    this->height =_height;
    this->bpp = source->bpp;
    rowsize = BITMAPWIDTHBYTES(width, bpp);
    palettecolors = source->palettecolors;
    memcpy(this->colormasks, source->colormasks, sizeof(colormasks));
    memcpy(this->rightshift, source->rightshift, sizeof(rightshift));
    memcpy(this->nbits, source->nbits, sizeof(nbits));
    memcpy(this->max, source->max, sizeof(max));
    SAFE_DELETE_ARRAY(palette);
    SAFE_DELETE_ARRAY(data);

    // Duplicate palette data
    if(palettecolors > 0) {
      // Duplicate palette
      this->palette = new unsigned char [source->palettecolors * sizeof(RGBQUAD)];
      if(palette == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "duplicateSubimage(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
  	    destroyImage();
  	    return false;
      }
      memcpy(this->palette, source->palette, palettecolors * sizeof(RGBQUAD));
    } else palette = NULL;
  } else {
    // Adjusts size
    this->width = _width;
    this->height =_height;
    this->rowsize = BITMAPWIDTHBYTES(this->width, this->bpp);
  }

  // Make memory for subimage data
  this->data = new unsigned char [this->rowsize * this->height];
  if(this->data == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "duplicateSubimage(): image data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    destroyImage();
    return false;
  }

	// Compute real extents
	unsigned int xini = 0, yini = 0, xoff = 0, yoff = 0;

	if(left < 0) _width -= (xoff = abs(left));
	else xini = (unsigned int) left;

	if(top < 0) _height -= (yoff = abs(top));
	else yini = (unsigned int) top;

	if(xini + _width  > oldwidth)  _width  = oldwidth  - xini;
	if(yini + _height > oldheight) _height = oldheight - yini;

  // Copy subimage bits
  if(bpp < 8) {
    // Pixels are not byte aligned
    int pixelsperbyte = (8 / this->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << this->bpp) - 1;

    // Copies colors with less than 9 bits in data buffer
    for(unsigned int row = 0; row < _height; row++) {
      register unsigned char *dstrow = &this->data[(this->height - row - yoff - 1) * rowsize];
      register unsigned char *srcrow = &olddata[(oldheight - yini - row - 1) * oldrowsize];
      for(unsigned int col = 0; col < _width; col++) {
        int val = (int) ((srcrow[(xini + col) / pixelsperbyte] >> (((xini + col) % pixelsperbyte) * this->bpp)) & bitmask);

        // Set destination bits
        unsigned char *ind = &dstrow[(col + xoff) / pixelsperbyte];
        int rshift = ((col + xoff) % pixelsperbyte) * this->bpp;
        *ind = (unsigned char) ((*ind ^ (*ind & (bitmask << rshift))) | ((val & bitmask) << rshift));
      }
    }
  } else
    if(bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32) {
      // Pixels are byte aligned
      unsigned int nbytes = (_width * bpp) >> 3;
      xoff = (xoff * bpp) >> 3;
      xini = (xini * bpp) >> 3;

      // Copies 24bpp colors in data buffer
      for(unsigned int row = 0; row < _height; row++) {
        unsigned char *possource = &olddata[xini + (oldheight - yini - row - 1) * oldrowsize];
        unsigned char *posdata   = &data[xoff + (this->height - row - yoff - 1) * rowsize];
        memcpy(posdata, possource, nbytes);
      }
    } else {
      // Release old data
      if(this == source)
        SAFE_DELETE_ARRAY(olddata);
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "duplicateSubimage(): invalid format bpp = %d. (%s:%d)", this->bpp, basename((char*)(char*)__FILE__), __LINE__);
      destroyImage();
      return false;
    }

  // Release old data
  if(this == source)
    SAFE_DELETE_ARRAY(olddata);

	// Ok, it's copied.
	return true;
}

bool gcgIMAGE::copyImage(gcgIMAGE *source) {
  // Check source.
  if(source == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "copyImage(): source image is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(source->data == NULL || source->width == 0 || source->height == 0 || source->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "copyImage(): source image is not valid or allocated. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(this == source) return true; // Nothing to do...

  // Check equivalence between both images.
  if(source->width != width || source->height != height || source->bpp != bpp ||
     source->rowsize != rowsize || source->palettecolors != palettecolors ||
     memcmp(this->colormasks, source->colormasks, sizeof(colormasks)) != 0  ||
     memcmp(this->rightshift, source->rightshift, sizeof(rightshift)) != 0 ||
     memcmp(this->nbits, source->nbits, sizeof(nbits)) != 0 ||
     memcmp(this->max, source->max, sizeof(max)) != 0) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "copyImage(): source and destination are incompatible. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

 	// Copy image data
	memcpy(this->data, source->data, rowsize * height);

  // Copy palette data
  if(palettecolors > 0) memcpy(this->palette, source->palette, palettecolors * sizeof(RGBQUAD));

	// Ok, it's copied.
	return true;
}

bool gcgIMAGE::loadImage(const char *filename) {
  // Check parameters
  if(filename == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "loadImage(): file name is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	int  ssize = (int) strlen(filename);
	char ext[6];

	if(ssize < 5) {
	  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNRECOGNIZEDFORMAT), "loadImage(): file type could not be extracted from file name. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
	  return false; // File name too short
	}

  // Extracts the file extension
  strcpy(ext, filename + ssize - 5);
  strlwr(ext);
  if(strstr(ext, ".jpg") != NULL || strstr(ext, ".jpeg") != NULL) return loadJPG(filename);
  if(strstr(ext, ".bmp") != NULL) return loadBMP(filename);
  if(strstr(ext, ".tga") != NULL) return loadTGA(filename);
  if(strstr(ext, ".pcx") != NULL) return loadPCX(filename);

  // Failed using the extension
  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNRECOGNIZEDFORMAT), "loadImage(): file type is not supported. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
  return false;
}

bool gcgIMAGE::loadJPG(const char *filename) {
  // Check parameters
  if(filename == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "loadJPG(): file name is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	FILE *file = fopen(filename, "rb");
	if(file == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadJPG(): error opening file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
	  return false;
	}

	struct jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr; // IJPEG library error handler
	cinfo.err = jpeg_std_error(&jerr); 	// Use the error handler in the decompression object

	// Decompression object instantiation
	jpeg_create_decompress(&cinfo);

	// Set the data source
	jpeg_stdio_src(&cinfo, file);

	// Reading the header
	jpeg_read_header(&cinfo, true);

  // Choose output color space
  if(cinfo.jpeg_color_space != JCS_RGB && cinfo.jpeg_color_space != JCS_GRAYSCALE) {
    // We'll need to force color space
    cinfo.out_color_space = JCS_RGB; /* colorspace for output */
    bpp = 24; // Color channels * 8 bits
  } else bpp = (cinfo.num_components << 3); // Number of channels * 8 bits

	// Initiate parameters
	width = cinfo.image_width;
	height = cinfo.image_height;
	rowsize = BITMAPWIDTHBYTES(width, bpp);

	// Starts the decompression
	jpeg_start_decompress(&cinfo);

	// Clear the current data if object was already used
	destroyImage();

	// Allocate memory for image
	this->data = new unsigned char [rowsize * height];
	if(this->data == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "loadJPG(): image data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	// Extract the pixel data
	unsigned char *prow[1];
	for(int irow = 0; cinfo.output_scanline < cinfo.output_height; ) {
    prow[0] = &data[rowsize * irow];
		irow += jpeg_read_scanlines(&cinfo, (JSAMPARRAY) &prow[0], 1);
  }

	// Free jpg decompression object
	jpeg_destroy_decompress(&cinfo);
	fclose(file);

	if(bpp == 8) {
    // In this case we must have a palette color
    this->palettecolors = 256;

  	// Allocate space for palette
  	unsigned int palsize = palettecolors * sizeof(RGBQUAD);
  	this->palette = new unsigned char[palsize];
  	if(this->palette == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "loadJPG(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      destroyImage();
      return false;
    }

    // Fill in our gray scale palette. It's linear by default.
    for(unsigned int i = 0; i < palettecolors; i++) {
      int pos = i * sizeof(RGBQUAD);
      palette[pos] = palette[pos + 1] = palette[pos + 2] = (unsigned char) i;
      palette[pos + 3] = 0;
    }

    // Paletted does not use masks
    memset(colormasks, 0, sizeof(colormasks));
    memset(rightshift, 0, sizeof(rightshift));
    memset(nbits, 0, sizeof(nbits));
    memset(max, 0, sizeof(max));
  } else if(bpp == 24) {
      // Internal representation
      colormasks[0] = 0xff;     // RED
      colormasks[1] = 0xff00;   // GREEN
      colormasks[2] = 0xff0000; // BLUE
      colormasks[3] = 0x00;     // ALPHA
      rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 0;
      nbits[0] = 8; nbits[1] = 8;  nbits[2] = 8; nbits[3] = 0;
      max[0] = 255; max[1] = 255;  max[2] = 255; max[3] = 0;
    }

	// The image is upside up and default orientation is the inverse.
	verticalFlip();

	// Ok, it's loaded.
	return true;
}

bool gcgIMAGE::loadBMP(const char *filename) {
	FILE *file;
	BITMAPFILEHEADER fileHeader;

  // Check parameters
  if(filename == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "loadBMP(): file name is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	file = fopen(filename, "rb+");
	if(file == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadBMP(): error opening file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
	  return false;
	}

	// Read the bitmap header
	if(fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file) != 1) {
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadBMP(): invalid BMP file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

	// Check it's a bitmap
	if(fileHeader.bfType != SIGNATURE_BMP) {
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadBMP(): file %s has not BMP signature. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

  // Discover file size
  fseek(file, 0, SEEK_END);
  unsigned int filesize = ftell(file);

  // Allocate temporary memory
  unsigned char *bmpdata = new unsigned char[filesize - sizeof(BITMAPFILEHEADER)];
  if(bmpdata == NULL) {
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "loadBMP(): memory allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

  // Point file to the beginning of the data and loads it
	fseek(file, sizeof(BITMAPFILEHEADER), SEEK_SET);
  if(fread(bmpdata, filesize - sizeof(BITMAPFILEHEADER), 1, file) != 1) {
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadBMP(): invalid BMP file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

  // Closes the file
	fclose(file);

  bool res = unpackBMP(bmpdata, ((unsigned char*) bmpdata) + (WORD) (fileHeader.bfOffBits - sizeof(BITMAPFILEHEADER)),
	                             (fileHeader.bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER)) / sizeof(RGBQUAD),
	                             ((unsigned char*) bmpdata) + sizeof(BITMAPINFOHEADER));

  // Free temporary buffer
  SAFE_DELETE_ARRAY(bmpdata);

  if(!res) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_DECODEERROR), "loadBMP(): error unpacking bitmap data. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);

	return res;
}

bool gcgIMAGE::loadPCX(const char *filename) {
  // Check parameters
  if(filename == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "loadPCX(): file name is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	FILE *file = fopen(filename, "rb");
	if(file == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadPCX(): error opening file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
	  return false;
	}

 	// Retrieve header, first 4 bytes, first 2 should be 0x0A0C
	unsigned short header[2];
	if(fread(header, sizeof(header), 1, file) != 1) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadPCX(): invalid PCX file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
		fclose(file);
		return false;
	}

	if(header[0] != SIGNATURE_PCX)	{
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadPCX(): file %s has not PCX signature. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
		fclose(file);
		return false;
	}

  // Clear the current data if object was already used
	destroyImage();

	// Minimum x value
	int xMin = fgetc(file);		// Loword
	xMin |= fgetc(file) << 8;	// Hiword

	// Minimum y value
	int yMin = fgetc(file);		// Loword
	yMin |= fgetc(file) << 8;	// Hiword

	// Maximum x value
	int xMax = fgetc(file);		// Loword
	xMax |= fgetc(file) << 8;	// Hiword

	// Maximum y value
	int yMax = fgetc(file);		// Loword
	yMax |= fgetc(file) << 8;	// Hiword

	// Compute width and height
	width  = xMax - xMin + 1;
	height = yMax - yMin + 1;

  // Set bpp and compute rowsize
	bpp = 8;
	rowsize = BITMAPWIDTHBYTES(width, bpp);

	// Allocate memory for pixel data (paletted)
	this->data  = new unsigned char[rowsize * height];
	if(this->data == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "loadPCX(): image data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
  	fclose(file);
		return false;
	}

	// Points to the image data
	fseek(file, 128, SEEK_SET);

	// Decodes RLE data and stores the pixels
	unsigned int widthaligned = width + (width % 2);
 	for(unsigned int i = 0, size = height * widthaligned; i < size;) {
		int c = getc(file); // Next character or repeat code

		if(c > 0xBF) {
			int ntimes = (0x3f & c);
			c = getc(file);

			for(int j = 0; j < ntimes && i < size; j++) {
        data[(i / widthaligned) * rowsize + (i % widthaligned)] = (unsigned char) c;
        i++;
      }
		} else {
        data[(i / widthaligned) * rowsize + (i % widthaligned)] = (unsigned char) c;
        i++;
      }
	}

	// The palette for PCX format is the last 769 bytes of the file
	fseek(file, -769, SEEK_END);

	// The first character must be equal to 12
	int c = getc(file);
	if(c != 12)	{
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadPCX(): invalid PCX file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
		fclose(file);
    destroyImage();
		return false;
	}

	palettecolors = 256;

	// Allocate memory for the image palette
	this->palette = new unsigned char[sizeof(RGBQUAD) * palettecolors];
	unsigned char *temp = new unsigned char[3 * palettecolors];
	if(this->palette == NULL || temp == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "loadPCX(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		fclose(file);
		destroyImage();
		SAFE_DELETE_ARRAY(temp);
		return false;
	}

	// Reading palette
	if(fread(temp, sizeof(unsigned char) * 3, palettecolors, file) != palettecolors) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadPCX(): error reading file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
		fclose(file);
		destroyImage();
		SAFE_DELETE_ARRAY(temp);
		return false;
	}

	fclose(file);

	// Now, move temp colors to the effective palette
	for(unsigned int i = 0; i < palettecolors; i++) {
    palette[i * sizeof(RGBQUAD) + 0] = temp[i * 3 + 0];
    palette[i * sizeof(RGBQUAD) + 1] = temp[i * 3 + 1];
    palette[i * sizeof(RGBQUAD) + 2] = temp[i * 3 + 2];
    palette[i * sizeof(RGBQUAD) + 3] = 0;
  }
	SAFE_DELETE_ARRAY(temp);

  // Paletted does not use masks
  memset(colormasks, 0, sizeof(colormasks));
  memset(rightshift, 0, sizeof(rightshift));
  memset(nbits, 0, sizeof(nbits));
  memset(max, 0, sizeof(max));

	// The image is upside up and default orientation is the inverse.
	verticalFlip();

  // Ok, it's loaded.
	return true;
}


bool gcgIMAGE::loadTGA(const char *filename) {
  // Check parameters
  if(filename == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "loadTGA(): file name is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	FILE *file = fopen(filename, "rb");
	if(file == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadTGA(): error opening file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
	  return false;
	}

	unsigned char	header[12];
	if(fread(header, sizeof(header), 1, file) != 1) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadTGA(): invalid TGA file %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
    fclose(file);
    return false;
  }

  // Finishes image loading
  // Is a paletted uncompressed tga?
	if(memcmp(SIGNATURE_TGA_UNCOMPRESSED8BIT, header, sizeof(SIGNATURE_TGA_UNCOMPRESSED8BIT)) == 0)
		return loadUncompressed8BitTGA(file);

  // Is a true color uncompressed tga?
	if(memcmp(SIGNATURE_TGA_UNCOMPRESSED, header, sizeof(SIGNATURE_TGA_UNCOMPRESSED)) == 0)
		return loadUncompressedTrueColorTGA(file);

  // Is a true color compressed tga?
	if(memcmp(SIGNATURE_TGA_COMPRESSED, header, sizeof(SIGNATURE_TGA_COMPRESSED)) == 0)
		return loadCompressedTrueColorTGA(file);

  // Close file
  fclose(file);

  // Unsupported tga version
  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "loadTGA(): unsupported TGA version in %s. (%s:%d)", filename, basename((char*)(char*)__FILE__), __LINE__);
	return false;
}


bool gcgIMAGE::saveBMP(const char *arq) {
  BITMAPINFOHEADER bih;
	BITMAPFILEHEADER bfh;
	FILE *file;

  // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "saveBMP(): image is not valid and cannot be saved. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Writes into a .bmp
  file = fopen(arq, "wb+");
  if(file == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error opening file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Sets info data structure
  memset(&bih, 0, sizeof(BITMAPINFOHEADER));
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biHeight	= height;
	bih.biWidth	= width;
	bih.biBitCount = (WORD) bpp;
	bih.biPlanes = 1;
	bih.biClrUsed = palettecolors;
	bih.biXPelsPerMeter = 150;
	bih.biYPelsPerMeter = 150;
	bih.biSizeImage = DIBSIZE(bih);
	bih.biCompression = BI_RGB;

	// Compress RLE for 8 bpp
	unsigned char *bufferRLE = NULL;
	if(bpp == 8) {
	  unsigned int sizeRLE;

	  // Make room for compressed bytes
    bufferRLE = (unsigned char*) ALLOC(rowsize * height);
    if(bufferRLE != NULL)
      if((sizeRLE = gcgCompressImageRLE8(width, height, this->data, bufferRLE)) != 0) {
        // Ok, data was compressed
        bih.biSizeImage = sizeRLE;
        bih.biCompression = BI_RLE8;
      }
	}

  // Constructs the header
  memset(&bfh, 0, sizeof(bfh));
  bfh.bfType = SIGNATURE_BMP;
  bfh.bfSize = sizeof(bfh) + bih.biSizeImage + sizeof(BITMAPINFOHEADER);
  bfh.bfOffBits = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) + palettecolors * sizeof(RGBQUAD);

 	// Check bpp and colormasks
  if((bpp == 16 && (colormasks[2] != 31  || colormasks[1] != (31 << 5) || colormasks[0] != (31 << 10) || nbits[3] != 0)) ||
     (bpp == 32 && nbits[3] != 0)) {
    bih.biCompression = BI_BITFIELDS;     // Adjusts the compression type
    bfh.bfOffBits += sizeof(unsigned int) * ((nbits[3] == 0) ? 3 : 4);  // Adjusts the data offset
  }

  // Writes the header
  if(fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, file) != 1) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error writing to file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
    fclose(file);
    return false;
  }

  // and bitmap information
  if(fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, file) != 1) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error writing to file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
    fclose(file);
    return false;
  }

  // is paletted?
  if(palettecolors > 0 && palette != NULL) {
    unsigned char *pal = (unsigned char*) ALLOC(sizeof(RGBQUAD) * palettecolors);

    // Check buffer
    if(pal == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "saveBMP(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      fclose(file);
      return false;
    }

  	// Converts palette colors back to BGR
  	unsigned char *pospal = pal, *pospalette = palette;
  	for(unsigned int i = 0; i < palettecolors; i++, pospal += sizeof(RGBQUAD), pospalette += sizeof(RGBQUAD)) {
		  // Swap b and r
		  pospal[0] = pospalette[2];
		  pospal[1] = pospalette[1];
		  pospal[2] = pospalette[0];
		  pospal[3] = pospalette[3];
    }

    // Write palette
    fwrite(pal, palettecolors * sizeof(RGBQUAD), 1, file);
    SAFE_FREE(pal);

    // Check the bpp for RLE encoding
    if(bih.biCompression == BI_RLE8 && bufferRLE != NULL) {
      // Is RLE encoding
      if(fwrite(bufferRLE, bih.biSizeImage, 1, file) != 1) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error writing to file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
        fclose(file);
        return false;
      }
      SAFE_FREE(bufferRLE);  // Release RLE data
    } else
        if(fwrite(data, bih.biSizeImage, 1, file) != 1) {// Write the original data
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error writing to file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
          fclose(file);
          return false;
        }
  } else
    if(bpp == 24) {
      // No palette, true colors
      unsigned char *buffer = (unsigned char*) ALLOC(bih.biSizeImage);

      // Check buffer
      if(buffer == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "saveBMP(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        fclose(file);
        return false;
      }

    	// Converts back to BGR format
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *posbuffer = &buffer[row * rowsize];
        unsigned char *posdata   = &data[row * rowsize];
    		for(unsigned int i = 0; i < width; i++, posbuffer += 3, posdata += 3) {
    			posbuffer[0] = posdata[2];
    			posbuffer[1] = posdata[1];
    			posbuffer[2] = posdata[0];
        }
      }

      // Write the converted data
      if(fwrite(buffer, bih.biSizeImage, 1, file) != 1) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error writing to file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
        fclose(file);
        SAFE_FREE(buffer);
        return false;
      }
      SAFE_FREE(buffer);
    } else
      	if(bpp == 32) {
          // No palette, true colors
          unsigned char *buffer = (unsigned char*) ALLOC(bih.biSizeImage);

          // Check buffer
          if(buffer == NULL) {
            gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "saveBMP(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
            fclose(file);
            return false;
          }

          // Converts the 32bpp bitmap back to B G R A
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *posbuffer = &buffer[row * rowsize];
            unsigned char *posdata   = &data[row * rowsize];
        		for(unsigned int i = 0; i < width; i++, posbuffer += 4, posdata += 4) {
        			posbuffer[0] = posdata[2];
        			posbuffer[1] = posdata[1];
        			posbuffer[2] = posdata[0];
            }
          }

          // Write the color masks for 32bpp, if needed
          if(bih.biCompression == BI_BITFIELDS) {
            // Now we have new color masks.
            unsigned int masks[4] = {0xff0000, 0xff00, 0xff, (nbits[3] != 0) ? 0xff000000 : 0x00};
            if(fwrite(masks, (nbits[3] == 0) ? 3 : 4, sizeof(unsigned int), file) != sizeof(unsigned int)) {
              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error writing to file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
              fclose(file);
              SAFE_FREE(buffer);
              return false;
            }
          }

          // Write the converted data
          if(fwrite(buffer, bih.biSizeImage, 1, file) != 1) {
            gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error writing to file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
            fclose(file);
            SAFE_FREE(buffer);
            return false;
          }
          SAFE_FREE(buffer);
        } else {
            // Write the color masks for 16bpp, if needed
            if(bih.biCompression == BI_BITFIELDS) {
              if(fwrite(colormasks, (nbits[3] == 0) ? 3 : 4, sizeof(unsigned int), file) != sizeof(unsigned int)) {
                gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error writing to file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
                fclose(file);
                return false;
              }
            }

            // Write the original data for other cases
            if(fwrite(data, bih.biSizeImage, 1, file) != 1) {
              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveBMP(): error writing to file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
              fclose(file);
              return false;
            }
        }

  fclose(file);
  return true;
}



bool gcgIMAGE::saveJPG(const char *arq, int quality) {
	FILE *file;

  if(arq == NULL || quality < 0 || quality > 100) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "saveJPG(): file name is NULL or quality outside interval [0, 100]: %d. (%s:%d)", quality, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "saveJPG(): image is not valid and cannot be saved. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Writes into a .jpg
  file = fopen(arq, "wb+");
  if(file == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "saveJPG(): error opening file %s. (%s:%d)", arq, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	struct jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr; // IJPEG library error handler
	cinfo.err = jpeg_std_error(&jerr); 	// Use the error handler in the decompression object

	// Compression object instantiation
	jpeg_create_compress(&cinfo);

	// Set the data destiny
	jpeg_stdio_dest(&cinfo, file);

	// Define input parameters
  cinfo.image_width = width;
	cinfo.image_height = height;

	// Set colorspaces
	gcgIMAGE *source = this;
	bool isgray = isGrayScale();

	if(bpp <= 8 && isgray) {
    if(bpp < 8) {
      source = new gcgIMAGE();
      source->convertBits(this, 8);
    }
    source->forceLinearPalette();
  } else if(bpp != 24) {
            source = new gcgIMAGE();
            source->convertBits(this, 24);
         }

	if(source->bpp == 24) {
    cinfo.in_color_space = JCS_RGB; /* colorspace for input */
    cinfo.input_components = 3;	/* # of color components per pixel */
  } else {
    cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace for input */
    cinfo.input_components = 1;	/* # of color components per pixel */
  }

  // Check gray scale image
  if(isgray) {
      cinfo.num_components = 1;		/* # of color components in JPEG image */
      cinfo.jpeg_color_space = JCS_GRAYSCALE; /* colorspace of JPEG image */
  }

  // Set parameters and quality
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, true);

	// Starts the compression
	jpeg_start_compress(&cinfo, TRUE);

	// Send pixel data
	unsigned char *prow[1];
	while(cinfo.next_scanline < cinfo.image_height) {
    prow[0] = &source->data[source->rowsize * (cinfo.image_height - cinfo.next_scanline - 1)];
		jpeg_write_scanlines(&cinfo, (JSAMPARRAY) &prow[0], 1);
  }

  // Finish compression
  jpeg_finish_compress(&cinfo);

	// Free jpg compression object
	jpeg_destroy_compress(&cinfo);
	fclose(file);

	if(source != this) SAFE_DELETE(source);

	// Ok, it's saved.
	return true;
}


bool gcgIMAGE::getPixelColor(unsigned int i, unsigned int j, VECTOR4 color) {
  // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "getPixelColor(): image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Coordinates are valid?
  if(color == NULL || i >= width || j >= height) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "getPixelColor(): color is NULL or accessing pixel outside image limits (%d, %d). (%s:%d)", i, j, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  register unsigned char r, g, b, a;

  // Computes the color for each possible bpp
  if(bpp == 24 || bpp == 32) {
      unsigned char *pixel = &data[j * rowsize + i * (bpp >> 3)];
      r = pixel[0];
      g = pixel[1];
      b = pixel[2];
      a = (bpp == 32) ? pixel[3] : 0;
  } else if(bpp <= 8 && palette != NULL) {
            // Using index functions
            return getPaletteColor(getPixelIndex(i, j), color);
         } else if(bpp == 16) {
                    unsigned short pixel = *((unsigned short*) &data[j * rowsize + i + i]);
                    int comp = (((pixel & colormasks[0]) >> rightshift[0]) * 255) / (colormasks[0] >> rightshift[0]);
                    if(comp > 255) comp = 255;
                    r = (unsigned char) comp; // RED

                    comp = (((pixel & colormasks[1]) >> rightshift[1]) * 255) / (colormasks[1] >> rightshift[1]);
                    if(comp > 255) comp = 255;
                    g = (unsigned char) comp;  // GREEN

                    comp = (((pixel & colormasks[2]) >> rightshift[2]) * 255) / (colormasks[2] >> rightshift[2]);
                    if(comp > 255) comp = 255;
                    b = (unsigned char) comp;  // BLUE

                		comp = (colormasks[3]) ? ((((pixel & colormasks[3]) >> rightshift[3]) * 255) / (colormasks[3] >> rightshift[3])) : 0;
                    if(comp > 255) comp = 255;
                    a = (unsigned char) comp;  // ALPHA
                } else {
                    // Unavailable request
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "getPixelColor(): unsupported bits per pixel %d. (%s:%d)", bpp, basename((char*)(char*)__FILE__), __LINE__);
                    return false;
                }

  // Normalize color
  color[0] = ((float) r) * ((float) (1.0 / 255.0));
  color[1] = ((float) g) * ((float) (1.0 / 255.0));
  color[2] = ((float) b) * ((float) (1.0 / 255.0));
  color[3] = ((float) a) * ((float) (1.0 / 255.0));

  // Ok, got a valid color
  return true;
}

bool gcgIMAGE::setPixelColor(unsigned int i, unsigned int j, VECTOR4 color) {
  // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "setPixelColor(): image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Coordinates are valid?
  if(color == NULL || i >= width || j >= height) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "setPixelColor(): color is NULL or accessing pixel outside image limits (%d, %d). (%s:%d)", i, j, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  register unsigned char r = (unsigned char) (color[0] * 255.0);
  register unsigned char g = (unsigned char) (color[1] * 255.0);
  register unsigned char b = (unsigned char) (color[2] * 255.0);
  register unsigned char a = (unsigned char) (color[3] * 255.0);

  // Sets the color for each possible bpp
  if(bpp == 24 || bpp == 32) {
      unsigned char *pixel = &data[j * rowsize + i * (bpp >> 3)];
      pixel[0] = r;
      pixel[1] = g;
      pixel[2] = b;
      if(bpp == 32) pixel[3] = a;
  } else if(bpp <= 8 && palette != NULL && palettecolors > 0) {
            // There is no sense in using this function for this...
            // We need first to find the closest color in palette
            unsigned int index = 0;
            int distance = SQR(r - palette[0]) + SQR(g - palette[1]) +
                           SQR(b - palette[2]) + SQR(a - palette[3]);

            // Find nearest color
            for(unsigned int k = 1; k < palettecolors; k++) {
              int dist = SQR(r - palette[k * sizeof(RGBQUAD)]) +
                         SQR(g - palette[k * sizeof(RGBQUAD) + 1]) +
                         SQR(b - palette[k * sizeof(RGBQUAD) + 2]) +
                         SQR(a - palette[k * sizeof(RGBQUAD) + 3]);
              if(dist < distance) {
                // found out a closer color
                index = k;
                distance = dist;
              }
            }

            // Now, set the appropriate index
            this->setPixelIndex(i, j, index);
         } else if(bpp == 16) {
                unsigned short *pixel = (unsigned short*) (&data[j * rowsize + i + i]);
                *pixel = (unsigned short) ((r >> (8 - nbits[0])) << rightshift[0]) |
                              ((g >> (8 - nbits[1])) << rightshift[1]) |
                              ((b >> (8 - nbits[2])) << rightshift[2]) |
                              ((a >> (8 - nbits[3])) << rightshift[3]);
                } else {
                    // Unavailable request
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "setPixelColor(): unsupported bits per pixel %d. (%s:%d)", bpp, basename((char*)(char*)__FILE__), __LINE__);
                    return false;
                }

  // Ok, the color was set
  return true;
}


int gcgIMAGE::getPixelIndex(unsigned int i, unsigned int j) {
 // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "getPixelIndex(): image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return -1;
  }

  // Coordinates are valid?
  if(i >= width || j >= height) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "getPixelIndex(): accessing pixel outside image limits (%d, %d). (%s:%d)", i, j, basename((char*)(char*)__FILE__), __LINE__);
    return -1;
  }


  // This image has palette? Index is valid?
  if(bpp > 8 || palette == NULL || palettecolors == 0) {
    // Unavailable request
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "getPixelIndex(): unsupported bits per pixel %d or image has no palette. (%s:%d)", bpp, basename((char*)(char*)__FILE__), __LINE__);
    return -1;
  }

  // Compute mask
  int  pixelsperbyte = (8 / bpp); // Compute the number of colors per data byte.

  // Ok, got a valid index
  return (int) ((data[j * rowsize + i / pixelsperbyte] >> ((i % pixelsperbyte) * bpp)) & ((0x01 << bpp) - 1));
}



bool gcgIMAGE::setPixelIndex(unsigned int i, unsigned int j, int index) {
 // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "setPixelIndex(): image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // This image has a palette? Index is valid?
  if(bpp > 8 || palette == NULL || palettecolors == 0) {
    // Unavailable request
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "setPixelIndex(): image has not a palette. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(i >= width || j >= height || index < 0 || (unsigned int) index >= palettecolors) {
    // Unavailable request
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "setPixelIndex(): coordinates outside image limits (%d, %d) or index outside palette size %d. (%s:%d)", i, j, index, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Compute mask
  int  pixelsperbyte = (8 / bpp); // Compute the number of colors per data byte.
  unsigned int bitmask = (0x01 << bpp) - 1; // Computes de index mask

  int rshift = (i % pixelsperbyte) * bpp;
  unsigned char *ind = &data[j * rowsize + i / pixelsperbyte];

  *ind = (unsigned char) ((*ind ^ (*ind & (bitmask << rshift))) | ((index & bitmask) << rshift));

  // Ok, the index was set
  return true;
}



bool gcgIMAGE::getPaletteColor(int index, VECTOR4 color) {
 // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "getPaletteColor(): image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // This image has palette? Index is valid?
  if(bpp > 8 || palette == NULL || palettecolors == 0) {
    // Unavailable request
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "getPaletteColor(): image has not a palette. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(color == NULL || index < 0 || (unsigned int) index >= palettecolors) {
    // Unavailable request
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "getPaletteColor(): color is NULL or index outside palette size %d. (%s:%d)", index, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  unsigned char *pos = &palette[sizeof(RGBQUAD) * index];
  // Normalize color
  color[0] = (float) pos[0] * (float) (1.0 / 255.0);
  color[1] = (float) pos[1] * (float) (1.0 / 255.0);
  color[2] = (float) pos[2] * (float) (1.0 / 255.0);
  color[3] = (float) pos[3] * (float) (1.0 / 255.0);

  // Ok, got the palette color
  return true;
}


bool gcgIMAGE::setPaletteColor(int index, VECTOR4 color) {
 // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "setPaletteColor(): image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // This image has palette? Index is valid?
  if(bpp > 8 || palette == NULL || palettecolors == 0) {
    // Unavailable request
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "setPaletteColor(): image has not a palette. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(color == NULL || index < 0 || (unsigned int) index >= palettecolors) {
    // Unavailable request
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "setPaletteColor(): color is NULL or index outside palette size %d. (%s:%d)", index, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  unsigned char *pos = &palette[sizeof(RGBQUAD) * index];
  // Normalize color
  color[0] = (float) pos[0] * (float) (1.0 / 255.0);
  color[1] = (float) pos[1] * (float) (1.0 / 255.0);
  color[2] = (float) pos[2] * (float) (1.0 / 255.0);
  color[3] = (float) pos[3] * (float) (1.0 / 255.0);

  // Ok, palette color was set
  return true;
}


bool gcgIMAGE::convolutionX(gcgIMAGE *src, gcgDISCRETE1D<float> *mask, float addthis) {
  // Source image is valid?
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convolutionX(): source image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(mask == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convolutionX(): mask is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(mask->length == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convolutionX(): mask has zero length. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // is paletted?
  if(src->palettecolors > 0 && src->palette != NULL) {
    int pixelsperbyte = (8 / src->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << src->bpp) - 1;

    if(src->isGrayScale()) {
      // Check destination image: destination must be compatible with the source
      unsigned char *dstdata = NULL;
      if(this != src) {
         // createSimilar creates only what is needed to make them compatible.
        if(!this->createSimilar(src)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convolutionX(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
      } else {
        // We need an output buffer
        dstdata = new unsigned char[rowsize * height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convolutionX(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      // Special case: we can make the convolution with the indexes
      src->forceLinearPalette(); // Needed to assure the bitmap can be filtered directly

      int rshift;
      addthis *= bitmask;

    	// Convolution with colors with less than 9 bits in data buffer
      unsigned char *srcrow = src->data;
    	for(unsigned int row = 0; row < src->height; row++, srcrow += src->rowsize) {
        unsigned char *dstrow = &dstdata[row * rowsize];
    		for(unsigned int i = 0; i < width; i++) {
    		  register float sum = addthis;
          register int j = i - (int) mask->origin;
          for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
            register int k = (j < 0) ? -j : ((j < (int) width) ? j : (int) width + (int) width - j - 2);
            sum += mask->data[pos] * ((srcrow[k / pixelsperbyte] >> ((k % pixelsperbyte) * bpp)) & bitmask);
          }

          // Check bounds
          unsigned char *ind = &dstrow[i / pixelsperbyte], val = (unsigned char) ((sum < 0.0) ? 0 : ((sum > bitmask) ? bitmask : sum));
          rshift = (i % pixelsperbyte) * bpp;
          *ind = (*ind ^ (*ind & (bitmask << rshift))) | ((val & bitmask) << rshift);
    		}
      }

      // Commit filtered data
      if(this == src) {
        // We have new data
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;
      } else memcpy(this->palette, src->palette, sizeof(RGBQUAD) * palettecolors);
    } else {
      unsigned char *dstdata = NULL;
      unsigned int newrowsize = 0;
      // We can't filter using a RGBA palette: destiny must have at least 24bits/pixel
      if(this != src) {
        if(!this->createImage(src->width, src->height, 32, true)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convolutionX(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
        newrowsize = this->rowsize;
      } else {
        newrowsize = BITMAPWIDTHBYTES(src->width, 32);
        dstdata = new unsigned char[newrowsize * src->height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convolutionX(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      addthis *= 255.0; // 8 bits/channel
    	// Convolution with 32 bits colors from palette
      unsigned char *srcrow = src->data;
    	for(unsigned int row = 0; row < src->height; row++, srcrow += src->rowsize) {
        unsigned char *dstrow = &dstdata[row * newrowsize];
    		for(unsigned int i = 0; i < width; i++, dstrow += 4) {
    		  register float sumR = addthis, sumG = addthis, sumB = addthis, sumA = addthis;
          register int j = i - (int) mask->origin;
          for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
            register int k = (j < 0) ? -j : ((j < (int) src->width) ? j : (int) src->width + (int) src->width - j - 2);
            unsigned char *color = &src->palette[sizeof(RGBQUAD) * ((srcrow[k / pixelsperbyte] >> ((k % pixelsperbyte) * src->bpp)) & bitmask)];
            sumR += mask->data[pos] * color[0];
            sumG += mask->data[pos] * color[1];
            sumB += mask->data[pos] * color[2];
            sumA += mask->data[pos] * color[3];
          }

          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
          dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
    		}
      }

      // Now commit the changes if destiny == source
      if(this == src) {
        bpp     = 32;
        rowsize = newrowsize;
        SAFE_DELETE_ARRAY(this->palette);
        palettecolors = 0;
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;

        // Internal representation
        colormasks[0] = 0xff;       // RED
        colormasks[1] = 0xff00;     // GREEN
        colormasks[2] = 0xff0000;   // BLUE
        colormasks[3] = 0xff000000; // ALPHA
        rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 24;
        nbits[0] = 8;  nbits[1] = 8;  nbits[2] = 8; nbits[3] = 8;
        max[0] = 255;  max[1] = 255;  max[2] = 255;  max[3] = 255;
      }
    }
  } else {
    // Check destination image: destination must be compatible with the source
    unsigned char *dstdata = NULL;
    if(this != src) {
       // createSimilar creates only what is needed to make them compatible.
      if(!this->createSimilar(src)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convolutionX(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
      dstdata = this->data;
    } else {
      // We need an output buffer
      dstdata = new unsigned char[rowsize * height];
      if(dstdata == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convolutionX(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
    }

    if(src->bpp == 24) {
      addthis *= 255.0; // 8 bits/channel

    	// Convolution with 24bits colors in data buffer
      unsigned char *srcrow = src->data;
    	for(unsigned int row = 0; row < height; row++, srcrow += rowsize) {
        unsigned char *dstrow = &dstdata[row * rowsize];
    		for(unsigned int i = 0; i < width; i++, dstrow += 3) {
    		  register float sumR = addthis, sumG = addthis, sumB = addthis;
          register int j = i - (int) mask->origin;
          for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
            register int k = (j < 0) ? -j : ((j < (int) width) ? j : (int) width + (int) width - j - 2);
            sumR += mask->data[pos] * srcrow[3 * k + 0];
            sumG += mask->data[pos] * srcrow[3 * k + 1];
            sumB += mask->data[pos] * srcrow[3 * k + 2];
          }

          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
    		}
      }
    } else
      	if(src->bpp == 32) {
      	  addthis *= 255.0; // 8 bits/channel

          // Convolution with 32 bits colors in data buffer
          unsigned char *srcrow = src->data;
          for(unsigned int row = 0; row < height; row++, srcrow += rowsize) {
            unsigned char *dstrow = &dstdata[row * rowsize];
            for(unsigned int i = 0; i < width; i++, dstrow += 4) {
              register float sumR = addthis, sumG = addthis, sumB = addthis, sumA = addthis;
              register int j = i - (int) mask->origin;
              for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
                register int k = (j < 0) ? -j : ((j < (int) width) ? j : (int) width + (int) width - j - 2);
                sumR += mask->data[pos] * srcrow[(k << 2) + 0];
                sumG += mask->data[pos] * srcrow[(k << 2) + 1];
                sumB += mask->data[pos] * srcrow[(k << 2) + 2];
                sumA += mask->data[pos] * srcrow[(k << 2) + 3];
              }

              // Check bounds
              dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
              dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
              dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
              dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
            }
          }
        } else
          if(src->bpp == 16) {
              // Convolution with 16 bits colors in data buffer
              float addthisR = addthis * max[0];
              float addthisG = addthis * max[1];
              float addthisB = addthis * max[2];
              float addthisA = addthis * max[3];
              for(unsigned int row = 0; row < height; row++) {
                unsigned short *srcrow = (unsigned short*) &src->data[row * rowsize];
                unsigned short *dstrow = (unsigned short*) &dstdata[row * rowsize];
                for(unsigned int i = 0; i < width; i++, dstrow++) {
                  register float sumR = addthisR, sumG = addthisG, sumB = addthisB, sumA = addthisA;
                  register int j = i - (int) mask->origin;
                  for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
                    register int color = srcrow[(j < 0) ? -j : ((j < (int) width) ? j : (int) width + (int) width - j - 2)];
                    int red   = (color & colormasks[0]) >> rightshift[0],
                        green = (color & colormasks[1]) >> rightshift[1],
                        blue  = (color & colormasks[2]) >> rightshift[2],
                        alpha = (colormasks[3]) ? ((color & colormasks[3]) >> rightshift[3]) : 0;

                    sumR += mask->data[pos] * red;
                    sumG += mask->data[pos] * green;
                    sumB += mask->data[pos] * blue;
                    sumA += mask->data[pos] * alpha;
                  }

                  *dstrow = (unsigned short)
                      (((sumR < 0.0) ? 0 : ((sumR > max[0]) ? colormasks[0] : (((unsigned char) sumR) << rightshift[0]))) |
                      ((sumG < 0.0) ? 0 : ((sumG > max[1]) ? colormasks[1] : (((unsigned char) sumG) << rightshift[1]))) |
                      ((sumB < 0.0) ? 0 : ((sumB > max[2]) ? colormasks[2] : (((unsigned char) sumB) << rightshift[2]))) |
                      ((sumA < 0.0) ? 0 : ((sumA > max[3]) ? colormasks[3] : (((unsigned char) sumA) << rightshift[3]))));
                }
              }
            } else {
                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convolutionX(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                  return false;
                }

    // Commit filtered data
    if(this == src) {
      // We have new data
      SAFE_DELETE_ARRAY(this->data);
      this->data = dstdata;
    }
  }

  // Ok, it's done
	return true;
}


bool gcgIMAGE::convolutionY(gcgIMAGE *src, gcgDISCRETE1D<float> *mask, float addthis) {
  // Source image is valid?
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convolutionY(): source image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(mask == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convolutionY(): mask is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(mask->length == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convolutionY(): mask has zero length. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // is paletted?
  if(src->palettecolors > 0 && src->palette != NULL) {
    int pixelsperbyte = (8 / src->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << src->bpp) - 1;

    if(src->isGrayScale()) {
      // Check destination image: destination must be compatible with the source
      unsigned char *dstdata = NULL;
      if(this != src) {
         // createSimilar creates only what is needed to make them compatible.
        if(!this->createSimilar(src)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convolutionY(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
      } else {
        // We need an output buffer
        dstdata = new unsigned char[rowsize * height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convolutionY(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      // Special case: we can make the convolution with the indexes
      src->forceLinearPalette(); // Needed to assure the bitmap can be filtered directly

      int rshift;
      addthis *= bitmask;

    	// Convolution with colors with less than 9 bits in data buffer
    	for(unsigned int row = 0; row < src->height; row++) {
        unsigned char *dstrow = &dstdata[row * rowsize];
    		for(unsigned int i = 0; i < width; i++) {
    		  register float sum = addthis;
          register int j = row - (int) mask->origin;
          for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
            register int k = (j < 0) ? -j : ((j < (int) height) ? j : (int) height + (int) height - j - 2);
            sum += mask->data[pos] * ((src->data[k * src->rowsize + i / pixelsperbyte] >> ((i % pixelsperbyte) * src->bpp)) & bitmask);
          }

          // Check bounds
          unsigned char *ind = &dstrow[i / pixelsperbyte], val = (unsigned char) ((sum < 0.0) ? 0 : ((sum > bitmask) ? bitmask : sum));
          rshift = (i % pixelsperbyte) * bpp;
          *ind = (*ind ^ (*ind & (bitmask << rshift))) | ((val & bitmask) << rshift);
    		}
      }

      // Commit filtered data
      if(this == src) {
        // We have new data
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;
      } else memcpy(this->palette, src->palette, sizeof(RGBQUAD) * palettecolors);
    } else {
      unsigned char *dstdata = NULL;
      unsigned int newrowsize = 0;
      // We can't filter using a RGBA palette: destiny must have at least 32 bits/pixel
      if(this != src) {
        if(!this->createImage(src->width, src->height, 32, true)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convolutionY(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
        newrowsize = this->rowsize;
      } else {
        newrowsize = BITMAPWIDTHBYTES(src->width, 32);
        dstdata = new unsigned char[newrowsize * src->height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convolutionY(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      addthis *= 255.0; // 8 bits/channel
    	// Convolution with 32 bits colors from palette
      for(unsigned int row = 0; row < src->height; row++) {
        unsigned char *dstrow = &dstdata[row * newrowsize];
    		for(unsigned int i = 0; i < src->width; i++, dstrow += 4) {
    		  register float sumR = addthis, sumG = addthis, sumB = addthis, sumA = addthis;
          register int j = row - (int) mask->origin;
          for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
            register int k = (j < 0) ? -j : ((j < (int) src->height) ? j : (int) src->height + (int) src->height - j - 2);
            unsigned char *color = &src->palette[sizeof(RGBQUAD) * ((src->data[src->rowsize * k + i / pixelsperbyte] >> ((i % pixelsperbyte) * src->bpp)) & bitmask)];
            sumR += mask->data[pos] * color[0];
            sumG += mask->data[pos] * color[1];
            sumB += mask->data[pos] * color[2];
            sumA += mask->data[pos] * color[3];
          }

          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
          dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
    		}
      }

      // Now commit the changes if destiny == source
      if(this == src) {
        bpp     = 32;
        rowsize = newrowsize;
        SAFE_DELETE_ARRAY(this->palette);
        palettecolors = 0;
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;

        // Internal representation
        colormasks[0] = 0xff;       // RED
        colormasks[1] = 0xff00;     // GREEN
        colormasks[2] = 0xff0000;   // BLUE
        colormasks[3] = 0xff000000; // ALPHA
        rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 24;
        nbits[0] = 8;  nbits[1] = 8;  nbits[2] = 8; nbits[3] = 8;
        max[0] = 255;  max[1] = 255;  max[2] = 255;  max[3] = 255;
      }
    }
  } else {
    // Check destination image: destination must be compatible with the source
    unsigned char *dstdata = NULL;
    if(this != src) {
       // createSimilar creates only what is needed to make them compatible.
      if(!this->createSimilar(src)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convolutionY(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
      dstdata = this->data;
    } else {
      // We need an output buffer
      dstdata = new unsigned char[rowsize * height];
      if(dstdata == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convolutionY(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
    }

    if(src->bpp == 24) {
      addthis *= 255.0; // 8 bits/channel

    	// Convolution with 24bits colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *dstrow = &dstdata[row * rowsize];
    		for(unsigned int i = 0; i < width; i++, dstrow += 3) {
    		  register float sumR = addthis, sumG = addthis, sumB = addthis;
          register int j = row - (int) mask->origin;
          for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
            register int k = (j < 0) ? -j : ((j < (int) height) ? j : (int) height + (int) height - j - 2);
            register unsigned char *color = &src->data[rowsize * k + i * 3];
            sumR += mask->data[pos] * color[0];
            sumG += mask->data[pos] * color[1];
            sumB += mask->data[pos] * color[2];
          }

          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
    		}
      }
    } else
      	if(src->bpp == 32) {
      	  addthis *= 255.0; // 8 bits/channel

          // Convolution with 32 bits colors in data buffer
          for(unsigned int row = 0; row < height; row++) {
            unsigned char *dstrow = &dstdata[row * rowsize];
            for(unsigned int i = 0; i < width; i++, dstrow += 4) {
              register float sumR = addthis, sumG = addthis, sumB = addthis, sumA = addthis;
              register int j = row - (int) mask->origin;
              for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
                register int k = (j < 0) ? -j : ((j < (int) height) ? j : (int) height + (int) height - j - 2);
                register unsigned char *color = &src->data[k * rowsize + (i << 2)];
                sumR += mask->data[pos] * color[0];
                sumG += mask->data[pos] * color[1];
                sumB += mask->data[pos] * color[2];
                sumA += mask->data[pos] * color[3];
              }

              // Check bounds
              dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
              dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
              dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
              dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
            }
          }
        } else
          if(src->bpp == 16) {
              // Convolution with 16 bits colors in data buffer
              float addthisR = addthis * max[0];
              float addthisG = addthis * max[1];
              float addthisB = addthis * max[2];
              float addthisA = addthis * max[3];
              for(unsigned int row = 0; row < height; row++) {
                unsigned short *dstrow = (unsigned short*) &dstdata[row * rowsize];
                for(unsigned int i = 0; i < width; i++, dstrow++) {
                  register float sumR = addthisR, sumG = addthisG, sumB = addthisB, sumA = addthisA;
                  register int j = row - (int) mask->origin;
                  for(register unsigned int pos = 0; pos < mask->length; j++, pos++) {
                    register int color = ((unsigned short*) &src->data[rowsize * ((j < 0) ? -j : ((j < (int) height) ? j : (int) height + (int) height - j - 2))])[i];
                    int red   = (color & colormasks[0]) >> rightshift[0],
                        green = (color & colormasks[1]) >> rightshift[1],
                        blue  = (color & colormasks[2]) >> rightshift[2],
                        alpha = (colormasks[3]) ? ((color & colormasks[3]) >> rightshift[3]) : 0;

                    sumR += mask->data[pos] * red;
                    sumG += mask->data[pos] * green;
                    sumB += mask->data[pos] * blue;
                    sumA += mask->data[pos] * alpha;
                  }

                  *dstrow = (unsigned short)
                      (((sumR < 0.0) ? 0 : ((sumR > max[0]) ? colormasks[0] : (((unsigned char) sumR) << rightshift[0]))) |
                      ((sumG < 0.0) ? 0 : ((sumG > max[1]) ? colormasks[1] : (((unsigned char) sumG) << rightshift[1]))) |
                      ((sumB < 0.0) ? 0 : ((sumB > max[2]) ? colormasks[2] : (((unsigned char) sumB) << rightshift[2]))) |
                      ((sumA < 0.0) ? 0 : ((sumA > max[3]) ? colormasks[3] : (((unsigned char) sumA) << rightshift[3]))));
                }
              }
            } else {
                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convolutionY(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                  return false;
                }

    // Commit filtered data
    if(this == src) {
      // We have new data
      SAFE_DELETE_ARRAY(this->data);
      this->data = dstdata;
    }
  }

  // Ok, it's done
	return true;
}

bool gcgIMAGE::convolutionXY(gcgIMAGE *src, gcgDISCRETE2D<float> *mask, float addthis) {
  // Source image is valid?
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convolutionY(): source image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(mask == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convolutionY(): mask is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(mask->width == 0 || mask->height == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convolutionY(): mask has zero length. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // is paletted?
  if(src->palettecolors > 0 && src->palette != NULL) {
    int pixelsperbyte = (8 / src->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << src->bpp) - 1;

    if(src->isGrayScale()) {
      // Check destination image: destination must be compatible with the source
      unsigned char *dstdata = NULL;
      if(this != src) {
         // createSimilar creates only what is needed to make them compatible.
        if(!this->createSimilar(src)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convolutionXY(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
      } else {
        // We need an output buffer
        dstdata = new unsigned char[rowsize * height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convolutionXY(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      // Special case: we can make the convolution with the indexes
      src->forceLinearPalette(); // Needed to assure the bitmap can be filtered directly
      addthis *= bitmask;

    	// Convolution with colors with less than 9 bits in data buffer
    	for(unsigned int row = 0; row < src->height; row++) {
        unsigned char *dstrow = &dstdata[row * rowsize];
    		for(unsigned int col = 0; col < width; col++) {
    		  register float sum = addthis;
          register int j = row - (int) mask->originY;
          for(register unsigned int posY = 0; posY < mask->height; j++, posY++) {
            register int k = (j < 0) ? -j : ((j < (int) height) ? j : (int) height + (int) height - j - 2);
            register unsigned char *srcrow = &src->data[src->rowsize * k];
            register float *maskrow = &mask->data[posY * mask->width];
            register int i = col - (int) mask->originX;
            for(register unsigned int posX = 0; posX < mask->width; i++, posX++) {
              register int l = (i < 0) ? -i : ((i < (int) width) ? i : (int) width + (int) width - i - 2);
              sum += maskrow[posX] * ((srcrow[l / pixelsperbyte] >> ((l % pixelsperbyte) * src->bpp)) & bitmask);
            }
          }

          // Check bounds
          unsigned char *ind = &dstrow[col / pixelsperbyte], val = (unsigned char) ((sum < 0.0) ? 0 : ((sum > bitmask) ? bitmask : sum));
          int rshift = (col % pixelsperbyte) * bpp;
          *ind = (*ind ^ (*ind & (bitmask << rshift))) | ((val & bitmask) << rshift);
    		}
      }

      // Commit filtered data
      if(this == src) {
        // We have new data
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;
      } else memcpy(this->palette, src->palette, sizeof(RGBQUAD) * palettecolors);
    } else {
      unsigned char *dstdata = NULL;
      unsigned int newrowsize = 0;
      // We can't filter using a RGBA palette: destiny must have at least 32 bits/pixel
      if(this != src) {
        if(!this->createImage(src->width, src->height, 32, true)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convolutionXY(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
        newrowsize = this->rowsize;
      } else {
        newrowsize = BITMAPWIDTHBYTES(src->width, 32);
        dstdata = new unsigned char[newrowsize * src->height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convolutionXY(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      addthis *= 255.0; // 8 bits/channel
    	// Convolution with 32 bits colors from palette
      for(unsigned int row = 0; row < src->height; row++) {
        unsigned char *dstrow = &dstdata[row * newrowsize];
    		for(unsigned int col = 0; col < src->width; col++, dstrow += 4) {
    		  register float sumR = addthis, sumG = addthis, sumB = addthis, sumA = addthis;
          register int j = row - (int) mask->originY;
          for(register unsigned int posY = 0; posY < mask->height; j++, posY++) {
            register int k = (j < 0) ? -j : ((j < (int) src->height) ? j : (int) src->height + (int) src->height - j - 2);
            register unsigned char *srcrow = &src->data[src->rowsize * k];
            register float *maskrow = &mask->data[posY * mask->width];
            register int i = col - (int) mask->originX;
            for(register unsigned int posX = 0; posX < mask->width; i++, posX++) {
              register int l = (i < 0) ? -i : ((i < (int) src->width) ? i : (int) src->width + (int) src->width - i - 2);
              unsigned char *color = &src->palette[sizeof(RGBQUAD) * ((srcrow[l / pixelsperbyte] >> ((l % pixelsperbyte) * src->bpp)) & bitmask)];
              sumR += maskrow[posX] * color[0];
              sumG += maskrow[posX] * color[1];
              sumB += maskrow[posX] * color[2];
              sumA += maskrow[posX] * color[3];
            }
          }

          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
          dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
    		}
      }

      // Now commit the changes if destiny == source
      if(this == src) {
        bpp     = 32;
        rowsize = newrowsize;
        SAFE_DELETE_ARRAY(this->palette);
        palettecolors = 0;
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;

        // Internal representation
        colormasks[0] = 0xff;       // RED
        colormasks[1] = 0xff00;     // GREEN
        colormasks[2] = 0xff0000;   // BLUE
        colormasks[3] = 0xff000000; // ALPHA
        rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 24;
        nbits[0] = 8;  nbits[1] = 8;  nbits[2] = 8; nbits[3] = 8;
        max[0] = 255;  max[1] = 255;  max[2] = 255;  max[3] = 255;
      }
    }
  } else {
    // Check destination image: destination must be compatible with the source
    unsigned char *dstdata = NULL;
    if(this != src) {
       // createSimilar creates only what is needed to make them compatible.
      if(!this->createSimilar(src)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convolutionXY(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
      dstdata = this->data;
    } else {
      // We need an output buffer
      dstdata = new unsigned char[rowsize * height];
      if(dstdata == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convolutionXY(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
    }

    if(src->bpp == 24) {
      addthis *= 255.0; // 8 bits/channel

    	// Convolution with 24bits colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *dstrow = &dstdata[row * rowsize];
    		for(unsigned int col = 0; col < width; col++, dstrow += 3) {
    		  register float sumR = addthis, sumG = addthis, sumB = addthis;
          register int j = row - (int) mask->originY;
          for(register unsigned int posY = 0; posY < mask->height; j++, posY++) {
            register int k = (j < 0) ? -j : ((j < (int) height) ? j : (int) height + (int) height - j - 2);
            register unsigned char *srcrow = &src->data[rowsize * k];
            register float *maskrow = &mask->data[posY * mask->width];
            register int i = col - (int) mask->originX;
            for(register unsigned int posX = 0; posX < mask->width; i++, posX++) {
              register int l = (i < 0) ? -i : ((i < (int) width) ? i : (int) width + (int) width - i - 2);
              register unsigned char *color = &srcrow[l * 3];
              sumR += maskrow[posX] * color[0];
              sumG += maskrow[posX] * color[1];
              sumB += maskrow[posX] * color[2];
            }
          }

          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
    		}
      }
    } else
      	if(src->bpp == 32) {
      	  addthis *= 255.0; // 8 bits/channel

          // Convolution with 32 bits colors in data buffer
          for(unsigned int row = 0; row < height; row++) {
            unsigned char *dstrow = &dstdata[row * rowsize];
            for(unsigned int col = 0; col < width; col++, dstrow += 4) {
              register float sumR = addthis, sumG = addthis, sumB = addthis, sumA = addthis;
              register int j = row - (int) mask->originY;
              for(register unsigned int posY = 0; posY < mask->height; j++, posY++) {
                register int k = (j < 0) ? -j : ((j < (int) height) ? j : (int) height + (int) height - j - 2);
                register unsigned char *srcrow = &src->data[k * rowsize];
                register float *maskrow = &mask->data[posY * mask->width];
                register int i = col - (int) mask->originX;
                for(register unsigned int posX = 0; posX < mask->width; i++, posX++) {
                  register int l = (i < 0) ? -i : ((i < (int) width) ? i : (int) width + (int) width - i - 2);
                  register unsigned char *color = &srcrow[l << 2];
                  sumR += maskrow[posX] * color[0];
                  sumG += maskrow[posX] * color[1];
                  sumB += maskrow[posX] * color[2];
                  sumA += maskrow[posX] * color[3];
                }
              }

              // Check bounds
              dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
              dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
              dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
              dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
            }
          }
        } else
            if(src->bpp == 16) {
              // Convolution with 16 bits colors in data buffer
              float addthisR = addthis * max[0];
              float addthisG = addthis * max[1];
              float addthisB = addthis * max[2];
              float addthisA = addthis * max[3];
              for(unsigned int row = 0; row < height; row++) {
                unsigned short *dstrow = (unsigned short*) &dstdata[row * rowsize];
                for(unsigned int col = 0; col < width; col++, dstrow++) {
                  register float sumR = addthisR, sumG = addthisG, sumB = addthisB, sumA = addthisA;
                  register int j = row - (int) mask->originY;
                  for(register unsigned int posY = 0; posY < mask->height; j++, posY++) {
                    register int k = (j < 0) ? -j : ((j < (int) height) ? j : (int) height + (int) height - j - 2);
                    register unsigned char *srcrow = &src->data[k * rowsize];
                    register float *maskrow = &mask->data[posY * mask->width];
                    register int i = col - (int) mask->originX;
                    for(register unsigned int posX = 0; posX < mask->width; i++, posX++) {
                      register int l = (i < 0) ? -i : ((i < (int) width) ? i : (int) width + (int) width - i - 2);
                      register int color = ((unsigned short*) srcrow)[l];
                      int red   = (color & colormasks[0]) >> rightshift[0],
                          green = (color & colormasks[1]) >> rightshift[1],
                          blue  = (color & colormasks[2]) >> rightshift[2],
                          alpha = (colormasks[3]) ? ((color & colormasks[3]) >> rightshift[3]) : 0;

                      sumR += maskrow[posX] * red;
                      sumG += maskrow[posX] * green;
                      sumB += maskrow[posX] * blue;
                      sumA += maskrow[posX] * alpha;
                    }
                  }

                  *dstrow = (unsigned short)
                      (((sumR < 0.0) ? 0 : ((sumR > max[0]) ? colormasks[0] : (((unsigned char) sumR) << rightshift[0]))) |
                      ((sumG < 0.0) ? 0 : ((sumG > max[1]) ? colormasks[1] : (((unsigned char) sumG) << rightshift[1]))) |
                      ((sumB < 0.0) ? 0 : ((sumB > max[2]) ? colormasks[2] : (((unsigned char) sumB) << rightshift[2]))) |
                      ((sumA < 0.0) ? 0 : ((sumA > max[3]) ? colormasks[3] : (((unsigned char) sumA) << rightshift[3]))));
                }
              }
            } else {
                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convolutionXY(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                  return false;
                }

    // Commit filtered data
    if(this == src) {
      // We have new data
      SAFE_DELETE_ARRAY(this->data);
      this->data = dstdata;
    }
  }

  // Ok, it's done
	return true;
}


bool gcgIMAGE::templateMatching(unsigned int imgleft, unsigned int imgtop, unsigned int imgwidth, unsigned int imgheight, gcgDISCRETE2D<float> *mask, VECTOR2 position) {
  if(this->data == NULL || this->width == 0 || this->height == 0 || this->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "templateMatching(): current image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameters
  if(mask == NULL || imgwidth == 0 || imgheight == 0 || position == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "templateMatching(): invalid parameters. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(mask->data == NULL || mask->width == 0 || mask->height == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "templateMatching(): invalid mask. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Computing limits
  unsigned int iniX = mask->originX;
  unsigned int endX = width - ((int) mask->width - mask->originX);
  unsigned int iniY = mask->originY;
  unsigned int endY = height - ((int) mask->height - mask->originY);
  if(imgleft > iniX) iniX = imgleft;
  if(imgtop  > iniY) iniY = imgtop;
  if(imgleft + imgwidth - 1 < endX) endX = imgleft + imgwidth - 1;
  if(imgtop + imgheight - 1 < endY) endY = imgtop + imgheight - 1;

  // is paletted?
  if(palettecolors > 0 && palette != NULL) {
    int pixelsperbyte = (8 / bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << bpp) - 1;

    if(this->isGrayScale()) {
      // Special case: we can make the matching with the indexes
      this->forceLinearPalette(); // Needed to assure the bitmap can be filtered directly
      int bestSAD = INT_MAX; // Stores the smallest Sum of Absolute Differences

    	// Matching with colors with less than 9 bits in data buffer
    	for(unsigned int row = iniY; row < endY; row++) {
    		for(unsigned int col = iniX; col < endX; col++) {
    		  register int sum = 0;
          register int k = row - (int) mask->originY;
          for(register unsigned int posY = 0; posY < mask->height; k++, posY++) {
            register unsigned char *srcrow = &this->data[rowsize * k];
            register float *maskrow = &mask->data[posY * mask->width];
            register int l = col - (int) mask->originX;
            for(register unsigned int posX = 0; posX < mask->width; l++, posX++)
              sum += abs((int) ((maskrow[posX] * bitmask) - ((srcrow[l / pixelsperbyte] >> ((l % pixelsperbyte) * bpp)) & bitmask)));
          }

          // Check SAD
          if(bestSAD > sum) {
            bestSAD = sum;
            position[0] = (float) col;
            position[1] = (float) row;
          }
    		}
      }
    } else {
    	// Template matching with 24 bits colors from the palette
    	int bestSAD = INT_MAX; // Stores the smallest Sum of Absolute Differences
    	for(unsigned int row = iniY; row < endY; row++) {
    		for(unsigned int col = iniX; col < endX; col++) {
          int sum = 0;
          register int k = row - (int) mask->originY;
          for(register unsigned int posY = 0; posY < mask->height; k++, posY++) {
            register unsigned char *srcrow = &this->data[rowsize * k];
            register float *maskrow = &mask->data[posY * mask->width];
            register int l = col - (int) mask->originX;
            for(register unsigned int posX = 0; posX < mask->width; l++, posX++) {
              unsigned char *color = &palette[sizeof(RGBQUAD) * ((srcrow[l / pixelsperbyte] >> ((l % pixelsperbyte) * bpp)) & bitmask)];
              register int value = (int) (maskrow[posX] * bitmask);
              sum += abs(value - color[0]) + abs(value - color[1]) + abs(value - color[2]);
            }
          }

          // Check SAD
          if(bestSAD > sum) {
            bestSAD = sum;
            position[0] = (float) col;
            position[1] = (float) row;
          }
    		}
      }
    }
  } else
    if(bpp == 24) {
    	int bestSAD = INT_MAX; // Stores the smallest Sum of Absolute Differences
    	for(unsigned int row = iniY; row < endY; row++) {
    		for(unsigned int col = iniX; col < endX; col++) {
    		  register int sum = 0;
          register int k = row - (int) mask->originY;
          for(register unsigned int posY = 0; posY < mask->height; k++, posY++) {
            register unsigned char *srcrow = &this->data[rowsize * k];
            register float *maskrow = &mask->data[posY * mask->width];
            register int l = col - (int) mask->originX;
            for(register unsigned int posX = 0; posX < mask->width; l++, posX++) {
              register unsigned char *color = &srcrow[l * 3];
              register int value = (int) (maskrow[posX] * 255.0);
              sum += abs(value - color[0]) + abs(value - color[1]) + abs(value - color[2]);
            }
          }

          // Check SAD
          if(bestSAD > sum) {
            bestSAD = sum;
            position[0] = (float) col;
            position[1] = (float) row;
          }
    		}
      }
    } else
      	if(bpp == 32) {
          int bestSAD = INT_MAX; // Stores the smallest Sum of Absolute Differences
          // Matching with 32 bits colors in data buffer
          for(unsigned int row = iniY; row < endY; row++) {
            for(unsigned int col = iniX; col < endX; col++) {
              register int sum = 0;
              register int k = row - (int) mask->originY;
              for(register unsigned int posY = 0; posY < mask->height; k++, posY++) {
                register unsigned char *srcrow = &this->data[k * rowsize];
                register float *maskrow = &mask->data[posY * mask->width];
                register int l = col - (int) mask->originX;
                for(register unsigned int posX = 0; posX < mask->width; l++, posX++) {
                  register unsigned char *color = &srcrow[l << 2];
                  register int value = (int) (maskrow[posX] * 255.0);
                  sum += abs(value - color[0]) + abs(value - color[1]) + abs(value - color[2]);
                }
              }

              // Check SAD
              if(bestSAD > sum) {
                bestSAD = sum;
                position[0] = (float) col;
                position[1] = (float) row;
              }
            }
          }
        } else
            if(bpp == 16) {
              // Matching with 16 bits colors in data buffer
              int bestSAD = INT_MAX; // Stores the smallest Sum of Absolute Differences
              for(unsigned int row = iniY; row < endY; row++) {
                for(unsigned int col = iniX; col < endX; col++) {
                  register int sum = 0;
                  register int k = row - (int) mask->originY;
                  for(register unsigned int posY = 0; posY < mask->height; k++, posY++) {
                    register unsigned char *srcrow = &this->data[k * rowsize];
                    register float *maskrow = &mask->data[posY * mask->width];
                    register int l = col - (int) mask->originX;
                    for(register unsigned int posX = 0; posX < mask->width; l++, posX++) {
                      register int color = ((unsigned short*) srcrow)[l];
                      int red   = (color & colormasks[0]) >> rightshift[0],
                          green = (color & colormasks[1]) >> rightshift[1],
                          blue  = (color & colormasks[2]) >> rightshift[2];
                          //alpha = (colormasks[3]) ? ((color & colormasks[3]) >> rightshift[3]) : 0;

                      sum += abs((int) (maskrow[posX] * max[0]) - red) +  abs((int) (maskrow[posX] * max[1]) - green) + abs((int) (maskrow[posX] * max[2]) - blue);
                    }
                  }

                  // Check SAD
                  if(bestSAD > sum) {
                    bestSAD = sum;
                    position[0] = (float) col;
                    position[1] = (float) row;
                  }
                }
              }
            } else {
                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "templateMatching(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", this->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                  return false;
                }

  // Ok, it's done
	return true;
}


bool gcgIMAGE::transformColorSpace(gcgIMAGE *src, MATRIX4 matrix) {
  // Check parameters
  if(matrix == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "transformColorSpace(): invalid matrix. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "transformColorSpace(): source image is invalid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check destination image: destination must be compatible with the source
  if(this != src)
    if(!this->createSimilar(src)) {
      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "transformColorSpace(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

  // is paletted?
  if(src->palettecolors > 0 && src->palette != NULL) {
    // We need to copy the data
    if(this != src) memcpy(this->data, src->data, rowsize * height);

  	// Transform palette colors
  	unsigned char *srcpalette = src->palette;
  	unsigned char *dstpalette = this->palette;
  	for(unsigned int i = 0; i < palettecolors; i++, srcpalette += sizeof(RGBQUAD), dstpalette += sizeof(RGBQUAD)) {
      register int resR = (int) ((float) srcpalette[0] * matrix[0]  + (float) srcpalette[1] * matrix[1]  + (float) srcpalette[2] * matrix[2]  + (float) srcpalette[3] * matrix[3]);
      register int resG = (int) ((float) srcpalette[0] * matrix[4]  + (float) srcpalette[1] * matrix[5]  + (float) srcpalette[2] * matrix[6]  + (float) srcpalette[3] * matrix[7]);
      register int resB = (int) ((float) srcpalette[0] * matrix[8]  + (float) srcpalette[1] * matrix[9]  + (float) srcpalette[2] * matrix[10] + (float) srcpalette[3] * matrix[11]);
      register int resA = (int) ((float) srcpalette[0] * matrix[12] + (float) srcpalette[1] * matrix[13] + (float) srcpalette[2] * matrix[14] + (float) srcpalette[3] * matrix[15]);

  	  // Check bounds
      dstpalette[0] = (resR < 0) ? 0 : ((resR > 255) ? 255 : (unsigned char) resR);
      dstpalette[1] = (resG < 0) ? 0 : ((resG > 255) ? 255 : (unsigned char) resG);
      dstpalette[2] = (resB < 0) ? 0 : ((resB > 255) ? 255 : (unsigned char) resB);
      dstpalette[3] = (resA < 0) ? 0 : ((resA > 255) ? 255 : (unsigned char) resA);
    }
  } else
    if(src->bpp == 24) {
    	// Converts 24 bpp colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *srcdata = &src->data[row * rowsize];
        unsigned char *dstdata = &this->data[row * rowsize];
    		for(unsigned int i = 0; i < width; i++, srcdata += 3, dstdata += 3) {
          register int resR = (int) ((float) srcdata[0] * matrix[0]  + (float) srcdata[1] * matrix[1]  + (float) srcdata[2] * matrix[2]);
          register int resG = (int) ((float) srcdata[0] * matrix[4]  + (float) srcdata[1] * matrix[5]  + (float) srcdata[2] * matrix[6]);
          register int resB = (int) ((float) srcdata[0] * matrix[8]  + (float) srcdata[1] * matrix[9]  + (float) srcdata[2] * matrix[10]);

          // Check bounds
          dstdata[0] = (resR < 0) ? 0 : ((resR > 255) ? 255 : (unsigned char) resR);
          dstdata[1] = (resG < 0) ? 0 : ((resG > 255) ? 255 : (unsigned char) resG);
          dstdata[2] = (resB < 0) ? 0 : ((resB > 255) ? 255 : (unsigned char) resB);
        }
      }
    } else
      	if(src->bpp == 32) {
          // Converts the 32 bpp bitmap
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *srcdata = &src->data[row * rowsize];
            unsigned char *dstdata = &this->data[row * rowsize];
        		for(unsigned int i = 0; i < width; i++, srcdata += 4, dstdata += 4) {
              register int resR = (int) ((float) srcdata[0] * matrix[0]  + (float) srcdata[1] * matrix[1]  + (float) srcdata[2] * matrix[2]  + (float) srcdata[3] * matrix[3]);
              register int resG = (int) ((float) srcdata[0] * matrix[4]  + (float) srcdata[1] * matrix[5]  + (float) srcdata[2] * matrix[6]  + (float) srcdata[3] * matrix[7]);
              register int resB = (int) ((float) srcdata[0] * matrix[8]  + (float) srcdata[1] * matrix[9]  + (float) srcdata[2] * matrix[10] + (float) srcdata[3] * matrix[11]);
              register int resA = (int) ((float) srcdata[0] * matrix[12] + (float) srcdata[1] * matrix[13] + (float) srcdata[2] * matrix[14] + (float) srcdata[3] * matrix[15]);

              // Check bounds
              dstdata[0] = (resR < 0) ? 0 : ((resR > 255) ? 255 : (unsigned char) resR);
              dstdata[1] = (resG < 0) ? 0 : ((resG > 255) ? 255 : (unsigned char) resG);
              dstdata[2] = (resB < 0) ? 0 : ((resB > 255) ? 255 : (unsigned char) resB);
              dstdata[3] = (resA < 0) ? 0 : ((resA > 255) ? 255 : (unsigned char) resA);
            }
          }
        } else
            if(src->bpp == 16) {
              // Converts the 16bpp colors
              for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
                unsigned short *srccolor = (unsigned short*) &src->data[currentRow * rowsize];
                unsigned short *dstcolor = (unsigned short*) &this->data[currentRow * rowsize];
                for(unsigned int i = 0; i < width; i++, srccolor++, dstcolor++) {
                  int red   = (((*srccolor & colormasks[0]) >> rightshift[0]) * 255) / max[0],
                      green = (((*srccolor & colormasks[1]) >> rightshift[1]) * 255) / max[1],
                      blue  = (((*srccolor & colormasks[2]) >> rightshift[2]) * 255) / max[2],
                      alpha = (colormasks[3]) ? ((((*srccolor & colormasks[3]) >> rightshift[3]) * 255) / max[3]) : 0;

                  register int resR = (int) ((float) red * matrix[0]  + (float) green * matrix[1]  + (float) blue * matrix[2]  + (float) alpha * matrix[3]);
                  register int resG = (int) ((float) red * matrix[4]  + (float) green * matrix[5]  + (float) blue * matrix[6]  + (float) alpha * matrix[7]);
                  register int resB = (int) ((float) red * matrix[8]  + (float) green * matrix[9]  + (float) blue * matrix[10] + (float) alpha * matrix[11]);
                  register int resA = (int) ((float) red * matrix[12] + (float) green * matrix[13] + (float) blue * matrix[14] + (float) alpha * matrix[15]);

                  // Check bounds
                  *dstcolor = (unsigned short)
                              ((((((resR < 0) ? 0 : ((resR > 255) ? 255 : (unsigned char) resR)) * max[0]) / 255) << rightshift[0]) |
                              (((((resG < 0) ? 0 : ((resG > 255) ? 255 : (unsigned char) resG)) * max[1]) / 255) << rightshift[1]) |
                              (((((resB < 0) ? 0 : ((resB > 255) ? 255 : (unsigned char) resB)) * max[2]) / 255) << rightshift[2]) |
                              (((((resA < 0) ? 0 : ((resA > 255) ? 255 : (unsigned char) resA)) * max[3]) / 255) << rightshift[3]));
                }
              }
            } else {
                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "transformColorSpace(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                  return false;
                }

  // Ok, it's done
	return true;
}

bool gcgIMAGE::convertBits(gcgIMAGE *src, unsigned int newbpp) {
  // Check parameter
  if(newbpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convertBits(): new bits per pixel is zero. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check source image
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convertBits(): source image is invalid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check the parameters
  if(newbpp == src->bpp) {
    // Nothing to do...
    if(this != src) return this->duplicateImage(src); // Simply copy
    return true;
  }

  // Choose proper converter
  if(newbpp ==  8 && bpp <  8 && palette != NULL) return convertPalettedto8Bits(src);
  if(newbpp == 24 && bpp <= 8 && palette != NULL) return convertPalettedto24Bits(src);
  if(newbpp == 24 && bpp == 16) return convert16Bitsto24Bits(src);
  if(newbpp == 32 && bpp == 16) return convert16Bitsto32Bits(src);
  if(newbpp == 24 && bpp == 32) return convert32Bitsto24Bits(src);

  // Unsupported conversion, yet...
  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convertBits(): unsupported conversion from %d bpp to %d bpp in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, newbpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
  return false;
}


bool gcgIMAGE::convertGrayScale(gcgIMAGE *src) {
  // Check source image
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convertGrayScale(): source image is invalid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check destination image: destination must be compatible with the source
  if(this != src)
    if(!this->createSimilar(src)) {
      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convertGrayScale(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

  // is paletted?
  if(src->palettecolors > 0 && src->palette != NULL) {
    // We need to copy the data
    if(this != src) memcpy(this->data, src->data, rowsize * height);

  	// Converts palette colors
  	unsigned char *srcpalette = src->palette;
  	unsigned char *dstpalette = this->palette;
  	bool islinear = true;
  	for(unsigned int i = 0; i < palettecolors; i++, srcpalette += sizeof(RGBQUAD), dstpalette += sizeof(RGBQUAD)) {
      unsigned int gray = (srcpalette[0] + srcpalette[1] + srcpalette[2]) / 3;
      dstpalette[0] = dstpalette[1] = dstpalette[2] = (unsigned char) gray;

      if(islinear && i != gray) islinear = false;// Indices match gray color?
    }

    // Forces the palette linearity
    if(!islinear) this->forceLinearPalette();
  } else
    if(src->bpp == 24) {
    	// Converts 24 bpp colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *srcdata   = &src->data[row * rowsize];
        unsigned char *dstdata   = &this->data[row * rowsize];
    		for(unsigned int i = 0; i < width; i++, srcdata += 3, dstdata += 3) {
          int gray = (srcdata[0] + srcdata[1] + srcdata[2]) / 3;
          dstdata[0] = dstdata[1] = dstdata[2] = (unsigned char) gray;
        }
      }
    } else
      	if(src->bpp == 32) {
          // Converts the 32bpp bitmap
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *srcdata   = &src->data[row * rowsize];
            unsigned char *dstdata   = &this->data[row * rowsize];
        		for(unsigned int i = 0; i < width; i++, srcdata += 4, dstdata += 4) {
              int gray = (srcdata[0] + srcdata[1] + srcdata[2]) / 3;
              dstdata[0] = dstdata[1] = dstdata[2] = (unsigned char) gray;
              dstdata[3] = srcdata[3];  // Copy alpha channel
            }
          }
        } else
            if(src->bpp == 16) {
              // Converts the 16bpp colors
              for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
                unsigned short *srccolor = (unsigned short*) &src->data[currentRow * rowsize];
                unsigned short *dstcolor = (unsigned short*) &this->data[currentRow * rowsize];
                for(unsigned int i = 0; i < width; i++, srccolor++, dstcolor++) {
                  unsigned int alpha = (colormasks[3]) ? (((*srccolor & colormasks[3]) >> rightshift[3]) * 255) / max[3] : 0;
                  unsigned int gray  = ((((*srccolor & colormasks[0]) >> rightshift[0]) * 255) / max[0] +
                                        (((*srccolor & colormasks[1]) >> rightshift[1]) * 255) / max[1] +
                                        (((*srccolor & colormasks[2]) >> rightshift[2]) * 255) / max[2]) / 3;
                  *dstcolor = (unsigned short) ((((gray  * (colormasks[0] >> rightshift[0])) / 255) << rightshift[0]) |
                              (((gray  * (colormasks[1] >> rightshift[1])) / 255) << rightshift[1]) |
                              (((gray  * (colormasks[2] >> rightshift[2])) / 255) << rightshift[2]) |
                              (((alpha * (colormasks[3] >> rightshift[3])) / 255) << rightshift[3]));
                }
              }
            } else {
                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convertGrayScale(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                  return false;
                }

  // Ok, it's done
	return true;
}



bool gcgIMAGE::convertGrayScale8bits(gcgIMAGE *src) {
  // Check source image
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "convertGrayScale8bits(): source image is invalid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check destination image: destination must be 8 bpp
  unsigned int newrowsize = BITMAPWIDTHBYTES(src->width, 8);
  unsigned char *srcdata = src->data;
  unsigned char *srcpal  = src->palette;

  if(this != src) {
    // Creates a 8 bpp destiny image
    if(!this->createImage(src->width, src->height, 8, false)) {
      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convertGrayScale8bits(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }
  } else {
    // Allocate space for the new image data
    data = new unsigned char[newrowsize * src->height];
    if(data == NULL)	{
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convertGrayScale8bits(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

    // Allocate space for the new palette
    palette = new unsigned char[256 * sizeof(RGBQUAD)];
    if(palette == NULL)	{
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convertGrayScale8bits(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      this->destroyImage();
      return false;
    }

   	// Create an uniform grayscale palette
  	for(unsigned int i = 0; i < 256; i++) {
      int pos = i * sizeof(RGBQUAD);
      palette[pos] = palette[pos + 1] = palette[pos + 2] = (unsigned char) i;
      palette[pos + 3] = 0;
    }
  }

  // is paletted?
  if(src->palettecolors > 0 && srcpal != NULL) {
  	// Converts palette colors
    int pixelsperbyte = (8 / src->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << bpp) - 1; // Computes de index mask

    // Set the indices.
    for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
      unsigned char *dstdat = &this->data[currentRow * newrowsize];
      unsigned char *srcdat = &srcdata[currentRow * src->rowsize];
      for(unsigned int i = 0; i < width; srcdat++)
        for(int j = pixelsperbyte-1; j >= 0; j--, i++, dstdat++) {
          unsigned char *color = &srcpal[(((*srcdat) >> (j * bpp)) & bitmask) * sizeof(RGBQUAD)];
          int gray = (color[0] + color[1] + color[2]) / 3;
          *dstdat = (unsigned char) gray;
        }
    }
  } else
    if(src->bpp == 24) {
    	// Converts 24bpp colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *dstdat = &this->data[row * newrowsize];
        unsigned char *srcdat = &srcdata[row * src->rowsize];
    		for(unsigned int i = 0; i < width; i++, srcdat += 3, dstdat++) {
          int gray = (srcdat[0] + srcdat[1] + srcdat[2]) / 3;
          *dstdat = (unsigned char) gray;
        }
      }
    } else
      	if(src->bpp == 32) {
          // Converts the 32bpp bitmap
        	for(unsigned int row = 0; row < height; row++) {
        	  unsigned char *dstdat = &this->data[row * newrowsize];
            unsigned char *srcdat = &srcdata[row * src->rowsize];
        		for(unsigned int i = 0; i < width; i++, srcdat += 4, dstdat++) {
              int gray = (srcdat[0] + srcdat[1] + srcdat[2]) / 3;
              *dstdat = (unsigned char) gray;
            }
          }
        } else
            if(src->bpp == 16) {
              // Converts the 16bpp colors
              for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
                unsigned char  *dstcolor = &this->data[currentRow * newrowsize];
                unsigned short *srccolor = (unsigned short*) &srcdata[currentRow * src->rowsize];
                for(unsigned int i = 0; i < width; i++, srccolor++, dstcolor++) {
                  unsigned int gray  = ((((*srccolor & colormasks[0]) >> rightshift[0]) * 255) / max[0] +
                                        (((*srccolor & colormasks[1]) >> rightshift[1]) * 255) / max[1] +
                                        (((*srccolor & colormasks[2]) >> rightshift[2]) * 255) / max[2]) / 3;
                  *dstcolor = (unsigned char) gray;
                }
              }
            } else {
                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convertGrayScale8bits(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                  return false;
                }

  // Commit conversion
  if(this == src) {
    // Converted successfully
    bpp = 8;
    rowsize = newrowsize;
    palettecolors = 256;
    SAFE_DELETE_ARRAY(srcdata);
    SAFE_DELETE_ARRAY(srcpal);

    // Paletted does not use masks
    memset(colormasks, 0, sizeof(colormasks));
    memset(rightshift, 0, sizeof(rightshift));
    memset(nbits, 0, sizeof(nbits));
    memset(max, 0, sizeof(max));
  }

  // Ok, it's done
	return true;
}



bool gcgIMAGE::isGrayScale() {
  // This image is valid?
  if(this->data == NULL || this->width == 0 || this->height == 0 || this->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "isGrayScale(): current image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // is paletted?
  if(palettecolors > 0 && palette != NULL) {
    bool isgray = true;

  	// Checks palette colors
  	unsigned char *pospalette = palette;
  	for(unsigned int i = 0; i < palettecolors; i++ && isgray, pospalette += sizeof(RGBQUAD))
		  if(pospalette[0] != pospalette[1] || pospalette[0] != pospalette[2] || pospalette[1] != pospalette[2])
		    isgray = false; // At least one color in palette is not gray

		// If the palette is not formed by gray tones, then we should check all indexed data.
		if(!isgray) {
      int  pixelsperbyte = (8 / bpp); // Compute the number of colors per data byte.
      unsigned int bitmask = (0x01 << bpp) - 1; // Computes de index mask

     	// Checking indexed data
    	for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
     	  unsigned char *posindex = &data[currentRow * rowsize];
    		for(unsigned int i = 0; i < width; posindex++)
          for(int j = pixelsperbyte-1; j >= 0; j--, i++) {
            unsigned char *color = &palette[(((*posindex) >> (j * bpp)) & bitmask) * sizeof(RGBQUAD)];
      			if(color[0] != color[1] || color[0] != color[2] || color[1] != color[2])
              return false; // At least one pixel is not gray
      		}
      }
    }
  } else
    if(bpp == 24) {
    	// Checks all 24bpp colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *posdata   = &data[row * rowsize];
    		for(unsigned int i = 0; i < width; i++, posdata += 3)
    			if(posdata[0] != posdata[1] || posdata[0] != posdata[2] || posdata[1] != posdata[2])
            return false; // At least one pixel is not gray
      }
    } else
      	if(bpp == 32) {
          // Checks the 32bpp bitmap
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *posdata   = &data[row * rowsize];
        		for(unsigned int i = 0; i < width; i++, posdata += 4)
        		  if(posdata[0] != posdata[1] || posdata[0] != posdata[2] || posdata[1] != posdata[2])
                return false; // At least one pixel is not gray
          }
        } else
            if(bpp == 16) {
              // Checks the 16bpp colors
              for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
                unsigned short *poscolor = (unsigned short*) &data[currentRow * rowsize];
                for(unsigned int i = 0; i < width; i++, poscolor++) {
                  unsigned int c0 = (((*poscolor & colormasks[0]) >> rightshift[0]) * 255) / max[0];
                  unsigned int c1 = (((*poscolor & colormasks[1]) >> rightshift[1]) * 255) / max[1];
                  unsigned int c2 = (((*poscolor & colormasks[2]) >> rightshift[2]) * 255) / max[2];

                  if(c0 != c1 || c0 != c2 || c1 != c2)
                    return false; // At least one pixel is not gray
                }
              }
            } else {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "isGrayScale(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", this->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                    return false;
                  }

  // All data is formed by gray tones
  return true;
}


bool gcgIMAGE::forceLinearPalette() {
  // This image is valid?
  if(this->data == NULL || this->width == 0 || this->height == 0 || this->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "forceLinearPalette(): current image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // is really paletted?
  if(this->palettecolors == 0 || this->palette == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "forceLinearPalette(): current image has not a palette. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(this->bpp == 8 && this->palettecolors == 256) {
    bool isgray = true;

  	// Checks palette colors
  	unsigned char *pospalette = palette;
  	for(unsigned int i = 0; i < this->palettecolors; i++ && isgray, pospalette += sizeof(RGBQUAD))
		  if(pospalette[0] != pospalette[1] || pospalette[0] != pospalette[2] || pospalette[1] != pospalette[2])
		    isgray = false; // At least one color in palette is not gray

		// If the palette is not formed by gray tones, then we should check all indexed data.
		if(isgray) {
      // This is a very special case: adjust bitmap for linearity
      for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
        unsigned char *posdata  = &this->data[currentRow * rowsize];
        for(unsigned int i = 0; i < this->width; i++, posdata++) *posdata = this->palette[*posdata * sizeof(RGBQUAD)];
      }

      // Allocate space for the new palette
      unsigned char *pal = new unsigned char[256 * sizeof(RGBQUAD)];
      if(pal == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "forceLinearPalette(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

      // Init new palette. Alpha/channel is opaque by default
      unsigned char *ppal = pal;
      for(unsigned int i = 0; i < 256; i++, ppal += sizeof(RGBQUAD)) {
        ppal[0] = ppal[1] = ppal[2] = (unsigned char) i;
        ppal[3] = 255;
      }

      // Copy alpha/exponent channel from previous palette
      ppal = this->palette;
      for(unsigned int i = 0; i < 256; i++, ppal += sizeof(RGBQUAD))
        pal[ppal[0] * sizeof(RGBQUAD) + 3] = ppal[3];

      // Commit new palette
      SAFE_DELETE_ARRAY(this->palette);
      this->palette = pal;

      // All palette colors are in ascending order of gray tones
      return true;
    }
  }

	// First, classify palette colors in ascending gray tones
	int *pos = new int[palettecolors + palettecolors]; // Two arrays in one, last array keeps inverted positions
	if(pos == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "forceLinearPalette(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  for(unsigned int i = 0; i < palettecolors; i++) pos[i] = i;

  // Using bubble sort... good when the palette is already sorted.
  int limit = palettecolors;
  while(limit > 0) {
    int bubble = 0;
    for(int i = 0; i < limit - 1; i++)
      if(palette[pos[i]   * sizeof(RGBQUAD)] + palette[pos[i]   * sizeof(RGBQUAD) + 1] + palette[pos[i]   * sizeof(RGBQUAD) + 2] >
         palette[pos[i+1] * sizeof(RGBQUAD)] + palette[pos[i+1] * sizeof(RGBQUAD) + 1] + palette[pos[i+1] * sizeof(RGBQUAD) + 2]) {
        SWAP(pos[i], pos[i+1]);
        bubble = i;
      }
    limit = bubble;
  }

  // Allocate space for the new palette
	unsigned char *pal = new unsigned char[palettecolors * sizeof(RGBQUAD)];
  if(pal == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "forceLinearPalette(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    SAFE_DELETE_ARRAY(pos);
    return false;
  }

  // Reorder palette colors and compute inverted positions
  unsigned char *pospal = pal;
  for(unsigned int i = 0; i < this->palettecolors; i++, pospal += sizeof(RGBQUAD)) {
    pospal[0] = this->palette[pos[i] * sizeof(RGBQUAD) + 0];
    pospal[1] = this->palette[pos[i] * sizeof(RGBQUAD) + 1];
    pospal[2] = this->palette[pos[i] * sizeof(RGBQUAD) + 2];
    pospal[3] = this->palette[pos[i] * sizeof(RGBQUAD) + 3];
    pos[palettecolors + pos[i]] = i;  // Inverted positions
  }
  SAFE_DELETE_ARRAY(this->palette);
  this->palette = pal;

  // Now, adjusts all data indices for the new palette.
  if(this->bpp < 8) {
    int  pixelsperbyte = (8 / bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << bpp) - 1; // Computes de index mask

    for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
      unsigned char *posdata  = &this->data[currentRow * rowsize];
      for(unsigned int i = 0; i < width; posdata++) {
        int newbyte = 0;
        for(int j = 0; j < pixelsperbyte; j++, i++)
          newbyte |= (pos[this->palettecolors + ((*posdata >> (j * bpp)) & bitmask)] << (j * bpp));
        *posdata = (unsigned char) newbyte;
      }
    }
  } else {
      // Speed up for the very common case where bpp = 8
      for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
        unsigned char *posdata  = &this->data[currentRow * rowsize];
        for(unsigned int i = 0; i < width; posdata++) *posdata = (unsigned char) (pos[this->palettecolors + *posdata]);
      }
    }

  // Release positions
  SAFE_DELETE_ARRAY(pos);

  // All palette colors are in ascending order of gray tones
  return true;
}


bool gcgIMAGE::scale(float weight1, gcgIMAGE *srcimage1, float addthis) {
  // Check source
  if(srcimage1 == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "scale(): source image is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check source image
  if(srcimage1->data == NULL || srcimage1->width == 0 || srcimage1->height == 0 || srcimage1->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "scale(): source image is invalid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // is paletted?
  if(srcimage1->palettecolors > 0 && srcimage1->palette != NULL) {
    int pixelsperbyte = (8 / srcimage1->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << srcimage1->bpp) - 1;

    if(srcimage1->isGrayScale()) {
      // Special case: we can scale the indices
      // We need to assure the bitmap can be scaled directly
      srcimage1->forceLinearPalette();

      // Destination must be compatible with the source
      if(this != srcimage1)
        if(!this->createSimilar(srcimage1)) { // createSimilar creates only what is needed to make them compatible.
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "scale(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      addthis *= bitmask;

    	// Composition with colors with less than 9 bits in data buffer
    	for(unsigned int row = 0; row < this->height; row++) {
        register unsigned char *dstrow  = &this->data[row * rowsize];
        register unsigned char *srcrow1 = &srcimage1->data[row * rowsize];
    		for(unsigned int col = 0; col < width; col++) {
    		  register float sum = addthis +
                               weight1 * (float) ((srcrow1[col / pixelsperbyte] >> ((col % pixelsperbyte) * this->bpp)) & bitmask);
          // Check bounds
          unsigned char *ind = &dstrow[col / pixelsperbyte], val = (unsigned char) ((sum < 0.0) ? 0 : ((sum > bitmask) ? bitmask : sum));
          int rshift = (col % pixelsperbyte) * bpp;
          *ind = (*ind ^ (*ind & (bitmask << rshift))) | ((val & bitmask) << rshift);
    		}
      }
    } else {
      unsigned char *dstdata = NULL;
      unsigned int newrowsize = 0;
      // We can't scale using a RGBA palette: destiny must have at least 32 bits/pixel
      if(this != srcimage1) {
        if(!this->createImage(srcimage1->width, srcimage1->height, 32, true)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "scale(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
        newrowsize = this->rowsize;
      } else {
        newrowsize = BITMAPWIDTHBYTES(srcimage1->width, 32);
        dstdata = new unsigned char[newrowsize * srcimage1->height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "scale(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      addthis *= 255.0; // 8 bits/channel
    	// Scaling with 32 bits colors from palette
      for(unsigned int row = 0; row < srcimage1->height; row++) {
        unsigned char *dstrow = &dstdata[row * newrowsize];
        register unsigned char *srcrow1 = &srcimage1->data[row * srcimage1->rowsize];
    		for(unsigned int col = 0; col < srcimage1->width; col++, dstrow += 4) {
          unsigned char *color1 = &srcimage1->palette[sizeof(RGBQUAD) * ((srcrow1[col / pixelsperbyte] >> ((col % pixelsperbyte) * srcimage1->bpp)) & bitmask)];
          register float sumR = addthis + weight1 * (float) color1[0];
          register float sumG = addthis + weight1 * (float) color1[1];
          register float sumB = addthis + weight1 * (float) color1[2];
          register float sumA = addthis + weight1 * (float) color1[3];

          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
          dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
    		}
      }

      // Now commit the changes if destiny == source
      if(this == srcimage1) {
        this->bpp     = 32;
        this->rowsize = newrowsize;
        SAFE_DELETE_ARRAY(this->palette);
        this->palettecolors = 0;
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;

        // Internal representation
        this->colormasks[0] = 0xff;       // RED
        this->colormasks[1] = 0xff00;     // GREEN
        this->colormasks[2] = 0xff0000;   // BLUE
        this->colormasks[3] = 0xff000000; // ALPHA
        this->rightshift[0] = 0;  this->rightshift[1] = 8;  this->rightshift[2] = 16; this->rightshift[3] = 24;
        this->nbits[0] = 8;  this->nbits[1] = 8;  this->nbits[2] = 8; this->nbits[3] = 8;
        this->max[0] = 255;  this->max[1] = 255;  this->max[2] = 255;  this->max[3] = 255;
      }
    }
  } else {
    // Destination must be compatible with the source
    if(this != srcimage1)
      if(!this->createSimilar(srcimage1)) { // createSimilar creates only what is needed to make them compatible.
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "scale(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

    if(srcimage1->bpp == 24) {
      addthis *= 255.0; // 8 bits/channel
    	// Scaling with 24 bits colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        register unsigned char *dstrow  = &this->data[row * rowsize];
        register unsigned char *srcrow1 = &srcimage1->data[row * rowsize];
    		for(unsigned int col = 0; col < width; col++, dstrow += 3, srcrow1 += 3) {
          register float sumR = addthis + weight1 * (float) srcrow1[0];
          register float sumG = addthis + weight1 * (float) srcrow1[1];
          register float sumB = addthis + weight1 * (float) srcrow1[2];
          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
    		}
      }
    } else
      	if(srcimage1->bpp == 32) {
          addthis *= 255.0; // 8 bits/channel
          // Scaling with 32 bits colors in data buffer
          for(unsigned int row = 0; row < height; row++) {
            register unsigned char *dstrow  = &this->data[row * rowsize];
            register unsigned char *srcrow1 = &srcimage1->data[row * rowsize];
            for(unsigned int col = 0; col < width; col++, dstrow += 4, srcrow1 += 4) {
              register float sumR = addthis + weight1 * (float) srcrow1[0];
              register float sumG = addthis + weight1 * (float) srcrow1[1];
              register float sumB = addthis + weight1 * (float) srcrow1[2];
              register float sumA = addthis + weight1 * (float) srcrow1[3];

              // Check bounds
              dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
              dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
              dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
              dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
            }
          }
        } else
            if(srcimage1->bpp == 16) {
              // Scaling with 16 bits colors in data buffer
              float addthisR = addthis * max[0];
              float addthisG = addthis * max[1];
              float addthisB = addthis * max[2];
              float addthisA = addthis * max[3];
              for(unsigned int row = 0; row < height; row++) {
                register unsigned short *dstrow  = (unsigned short*) &this->data[row * rowsize];
                register unsigned short *srcrow1 = (unsigned short*) &srcimage1->data[row * rowsize];
                for(unsigned int col = 0; col < width; col++, dstrow++) {
                  register int color1 = srcrow1[col];
                  register float sumR = addthisR + weight1 * (float) ((color1 & srcimage1->colormasks[0]) >> srcimage1->rightshift[0]);
                  register float sumG = addthisG + weight1 * (float) ((color1 & srcimage1->colormasks[1]) >> srcimage1->rightshift[1]);
                  register float sumB = addthisB + weight1 * (float) ((color1 & srcimage1->colormasks[2]) >> srcimage1->rightshift[2]);
                  register float sumA = addthisA + weight1 * (float) ((srcimage1->colormasks[3]) ? ((color1 & srcimage1->colormasks[3]) >> srcimage1->rightshift[3]) : 0);
                  *dstrow = (unsigned short)
                      (((sumR < 0.0) ? 0 : ((sumR > max[0]) ? srcimage1->colormasks[0] : (((unsigned char) sumR) << srcimage1->rightshift[0]))) |
                      ((sumG < 0.0) ? 0 : ((sumG > max[1]) ? srcimage1->colormasks[1] : (((unsigned char) sumG) << srcimage1->rightshift[1]))) |
                      ((sumB < 0.0) ? 0 : ((sumB > max[2]) ? srcimage1->colormasks[2] : (((unsigned char) sumB) << srcimage1->rightshift[2]))) |
                      ((sumA < 0.0) ? 0 : ((sumA > max[3]) ? srcimage1->colormasks[3] : (((unsigned char) sumA) << srcimage1->rightshift[3]))));
                }
              }
            } else {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "scale(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", srcimage1->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                    return false;
                  }
  }

  // Ok, it's done
	return true;
}


bool gcgIMAGE::combineAdd(gcgIMAGE *srcimage1, gcgIMAGE *srcimage2, float weight1, float weight2, float addthis) {
  // Check sources
  if(srcimage1 == NULL || srcimage2 == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "combineAdd(): any source image is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(!srcimage1->isCompatibleWith(srcimage2)) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "combineAdd(): the sources are not compatible. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // is paletted?
  if(srcimage1->palettecolors > 0 && srcimage1->palette != NULL) {
    int pixelsperbyte = (8 / srcimage1->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << srcimage1->bpp) - 1;

    if(srcimage1->isGrayScale()) {
      // Special case: we can combine the indexes
      // We need to assure the bitmap can be combined directly
      srcimage1->forceLinearPalette();
      srcimage2->forceLinearPalette();

      // Destination must be compatible with the source
      if(this != srcimage1 && this != srcimage2)
        if(!this->createSimilar(srcimage1)) { // createSimilar creates only what is needed to make them compatible.
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "combineAdd(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      addthis *= bitmask;

    	// Composition with colors with less than 9 bits in data buffer
    	for(unsigned int row = 0; row < this->height; row++) {
        register unsigned char *dstrow  = &this->data[row * rowsize];
        register unsigned char *srcrow1 = &srcimage1->data[row * rowsize];
        register unsigned char *srcrow2 = &srcimage2->data[row * rowsize];
    		for(unsigned int col = 0; col < width; col++) {
    		  register float sum = addthis +
                               weight1 * (float) ((srcrow1[col / pixelsperbyte] >> ((col % pixelsperbyte) * this->bpp)) & bitmask) +
                               weight2 * (float) ((srcrow2[col / pixelsperbyte] >> ((col % pixelsperbyte) * this->bpp)) & bitmask);
          // Check bounds
          unsigned char *ind = &dstrow[col / pixelsperbyte], val = (unsigned char) ((sum < 0.0) ? 0 : ((sum > bitmask) ? bitmask : sum));
          int rshift = (col % pixelsperbyte) * bpp;
          *ind = (*ind ^ (*ind & (bitmask << rshift))) | ((val & bitmask) << rshift);
    		}
      }
    } else {
      unsigned char *dstdata = NULL;
      unsigned int newrowsize = 0;
      // We can't combine using a RGBA palette: destiny must have at least 32 bits/pixel
      if(this != srcimage1 && this != srcimage2) {
        if(!this->createImage(srcimage2->width, srcimage1->height, 32, true)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "combineAdd(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
        newrowsize = this->rowsize;
      } else {
        newrowsize = BITMAPWIDTHBYTES(srcimage1->width, 32);
        dstdata = new unsigned char[newrowsize * srcimage1->height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "combineAdd(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      addthis *= 255.0; // 8 bits/channel
    	// Combination with 32 bits colors from the palette
      for(unsigned int row = 0; row < srcimage1->height; row++) {
        unsigned char *dstrow = &dstdata[row * newrowsize];
        register unsigned char *srcrow1 = &srcimage1->data[row * srcimage1->rowsize];
        register unsigned char *srcrow2 = &srcimage2->data[row * srcimage1->rowsize];
    		for(unsigned int col = 0; col < srcimage1->width; col++, dstrow += 4) {
          unsigned char *color1 = &srcimage1->palette[sizeof(RGBQUAD) * ((srcrow1[col / pixelsperbyte] >> ((col % pixelsperbyte) * srcimage1->bpp)) & bitmask)];
          unsigned char *color2 = &srcimage2->palette[sizeof(RGBQUAD) * ((srcrow2[col / pixelsperbyte] >> ((col % pixelsperbyte) * srcimage2->bpp)) & bitmask)];
          register float sumR = addthis + weight1 * (float) color1[0] + weight2 * (float) color2[0];
          register float sumG = addthis + weight1 * (float) color1[1] + weight2 * (float) color2[1];
          register float sumB = addthis + weight1 * (float) color1[2] + weight2 * (float) color2[2];
          register float sumA = addthis + weight1 * (float) color1[3] + weight2 * (float) color2[3];

          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
          dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
    		}
      }

      // Now commit the changes if destiny == source
      if(this == srcimage1 || this == srcimage2) {
        this->bpp     = 32;
        this->rowsize = newrowsize;
        SAFE_DELETE_ARRAY(this->palette);
        this->palettecolors = 0;
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;

        // Internal representation
        this->colormasks[0] = 0xff;       // RED
        this->colormasks[1] = 0xff00;     // GREEN
        this->colormasks[2] = 0xff0000;   // BLUE
        this->colormasks[3] = 0xff000000; // ALPHA
        this->rightshift[0] = 0;  this->rightshift[1] = 8;  this->rightshift[2] = 16; this->rightshift[3] = 24;
        this->nbits[0] = 8;  this->nbits[1] = 8;  this->nbits[2] = 8; this->nbits[3] = 8;
        this->max[0] = 255;  this->max[1] = 255;  this->max[2] = 255;  this->max[3] = 255;
      }
    }
  } else {
    // Destination must be compatible with the source
    if(this != srcimage1 && this != srcimage2)
      if(!this->createSimilar(srcimage1)) {
        //errorcode = GCG_INTERNAL_ERROR;
        return false; // createSimilar creates only what is needed to make them compatible.
      }

    if(srcimage1->bpp == 24) {
      addthis *= 255.0; // 8 bits/channel
    	// Combination with 24bits colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        register unsigned char *dstrow  = &this->data[row * rowsize];
        register unsigned char *srcrow1 = &srcimage1->data[row * rowsize];
        register unsigned char *srcrow2 = &srcimage2->data[row * rowsize];
    		for(unsigned int col = 0; col < width; col++, dstrow += 3, srcrow1 += 3, srcrow2 += 3) {
          register float sumR = addthis + weight1 * (float) srcrow1[0] + weight2 * (float) srcrow2[0];
          register float sumG = addthis + weight1 * (float) srcrow1[1] + weight2 * (float) srcrow2[1];
          register float sumB = addthis + weight1 * (float) srcrow1[2] + weight2 * (float) srcrow2[2];
          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
    		}
      }
    } else
      	if(srcimage1->bpp == 32) {
          addthis *= 255.0; // 8 bits/channel
          // Combination with 32bits colors in data buffer
          for(unsigned int row = 0; row < height; row++) {
            register unsigned char *dstrow  = &this->data[row * rowsize];
            register unsigned char *srcrow1 = &srcimage1->data[row * rowsize];
            register unsigned char *srcrow2 = &srcimage2->data[row * rowsize];
            for(unsigned int col = 0; col < width; col++, dstrow += 4, srcrow1 += 4, srcrow2 += 4) {
              register float sumR = addthis + weight1 * (float) srcrow1[0] + weight2 * (float) srcrow2[0];
              register float sumG = addthis + weight1 * (float) srcrow1[1] + weight2 * (float) srcrow2[1];
              register float sumB = addthis + weight1 * (float) srcrow1[2] + weight2 * (float) srcrow2[2];
              register float sumA = addthis + weight1 * (float) srcrow1[3] + weight2 * (float) srcrow2[3];

              // Check bounds
              dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
              dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
              dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
              dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
            }
          }
        } else
            if(srcimage1->bpp == 16) {
              // Combination with 16 bits colors in data buffer
              float addthisR = addthis * max[0];
              float addthisG = addthis * max[1];
              float addthisB = addthis * max[2];
              float addthisA = addthis * max[3];
              for(unsigned int row = 0; row < height; row++) {
                register unsigned short *dstrow  = (unsigned short*) &this->data[row * rowsize];
                register unsigned short *srcrow1 = (unsigned short*) &srcimage1->data[row * rowsize];
                register unsigned short *srcrow2 = (unsigned short*) &srcimage2->data[row * rowsize];
                for(unsigned int col = 0; col < width; col++, dstrow++) {
                  register int color1 = srcrow1[col], color2 = srcrow2[col];
                  register float sumR = addthisR + weight1 * (float) ((color1 & srcimage1->colormasks[0]) >> srcimage1->rightshift[0]) +
                                                   weight2 * (float) ((color2 & srcimage2->colormasks[0]) >> srcimage2->rightshift[0]);
                  register float sumG = addthisG + weight1 * (float) ((color1 & srcimage1->colormasks[1]) >> srcimage1->rightshift[1]) +
                                                   weight2 * (float) ((color2 & srcimage2->colormasks[1]) >> srcimage2->rightshift[1]);
                  register float sumB = addthisB + weight1 * (float) ((color1 & srcimage1->colormasks[2]) >> srcimage1->rightshift[2]) +
                                                   weight2 * (float) ((color2 & srcimage2->colormasks[2]) >> srcimage2->rightshift[2]);
                  register float sumA = addthisA + weight1 * (float) ((srcimage1->colormasks[3]) ? ((color1 & srcimage1->colormasks[3]) >> srcimage1->rightshift[3]) : 0) +
                                                   weight2 * (float) ((srcimage2->colormasks[3]) ? ((color2 & srcimage2->colormasks[3]) >> srcimage2->rightshift[3]) : 0);
                  *dstrow = (unsigned short)
                      (((sumR < 0.0) ? 0 : ((sumR > max[0]) ? srcimage1->colormasks[0] : (((unsigned char) sumR) << srcimage1->rightshift[0]))) |
                      ((sumG < 0.0) ? 0 : ((sumG > max[1]) ? srcimage1->colormasks[1] : (((unsigned char) sumG) << srcimage1->rightshift[1]))) |
                      ((sumB < 0.0) ? 0 : ((sumB > max[2]) ? srcimage1->colormasks[2] : (((unsigned char) sumB) << srcimage1->rightshift[2]))) |
                      ((sumA < 0.0) ? 0 : ((sumA > max[3]) ? srcimage1->colormasks[3] : (((unsigned char) sumA) << srcimage1->rightshift[3]))));
                }
              }
            } else {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "combineAdd(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", srcimage1->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                    return false;
                  }
  }

  // Ok, it's done
	return true;
}



bool gcgIMAGE::combineMult(gcgIMAGE *srcimage1, gcgIMAGE *srcimage2, float add1, float add2, float addthis) {
  // Check sources
  if(srcimage1 == NULL || srcimage2 == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "combineMult(): any source image is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(!srcimage1->isCompatibleWith(srcimage2)) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "combineMult(): the sources are not compatible. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // is paletted?
  if(srcimage1->palettecolors > 0 && srcimage1->palette != NULL) {
    int pixelsperbyte = (8 / srcimage1->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << srcimage1->bpp) - 1;

    if(srcimage1->isGrayScale()) {
      // Special case: we can combine the indexes
      // We need to assure the bitmap can be combined directly
      srcimage1->forceLinearPalette();
      srcimage2->forceLinearPalette();

      // Destination must be compatible with the source
      if(this != srcimage1 && this != srcimage2)
        if(!this->createSimilar(srcimage1)) { // createSimilar creates only what is needed to make them compatible.
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "combineMult(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      addthis *= bitmask;
      float inv = (float) (1.0 / bitmask);
      add1 *= bitmask;
      add2 *= bitmask;

    	// Composition with colors with less than 9 bits in data buffer
    	for(unsigned int row = 0; row < this->height; row++) {
        register unsigned char *dstrow  = &this->data[row * rowsize];
        register unsigned char *srcrow1 = &srcimage1->data[row * rowsize];
        register unsigned char *srcrow2 = &srcimage2->data[row * rowsize];
    		for(unsigned int col = 0; col < width; col++) {
    		  register float sum = addthis +
                               ((add1 + (float) ((srcrow1[col / pixelsperbyte] >> ((col % pixelsperbyte) * this->bpp)) & bitmask)) *
                                (add2 + (float) ((srcrow2[col / pixelsperbyte] >> ((col % pixelsperbyte) * this->bpp)) & bitmask))) * inv;
          // Check bounds
          unsigned char *ind = &dstrow[col / pixelsperbyte], val = (unsigned char) ((sum < 0.0) ? 0 : ((sum > bitmask) ? bitmask : sum));
          int rshift = (col % pixelsperbyte) * bpp;
          *ind = (*ind ^ (*ind & (bitmask << rshift))) | ((val & bitmask) << rshift);
    		}
      }
    } else {
      unsigned char *dstdata = NULL;
      unsigned int newrowsize = 0;
      // We can't combine using a RGBA palette: destiny must have at least 32 bits/pixel
      if(this != srcimage1 && this != srcimage2) {
        if(!this->createImage(srcimage2->width, srcimage1->height, 32, true)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "combineMult(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
        newrowsize = this->rowsize;
      } else {
        newrowsize = BITMAPWIDTHBYTES(srcimage1->width, 32);
        dstdata = new unsigned char[newrowsize * srcimage1->height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "combineMult(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      addthis *= 255.0; // 8 bits/channel
      float inv = (float) (1.0 / 255.0);
      add1 *= 255.0;
      add2 *= 255.0;

    	// Combination with 32 bits colors from palette
      for(unsigned int row = 0; row < srcimage1->height; row++) {
        unsigned char *dstrow = &dstdata[row * newrowsize];
        register unsigned char *srcrow1 = &srcimage1->data[row * srcimage1->rowsize];
        register unsigned char *srcrow2 = &srcimage2->data[row * srcimage1->rowsize];
    		for(unsigned int col = 0; col < srcimage1->width; col++, dstrow += 4) {
          unsigned char *color1 = &srcimage1->palette[sizeof(RGBQUAD) * ((srcrow1[col / pixelsperbyte] >> ((col % pixelsperbyte) * srcimage1->bpp)) & bitmask)];
          unsigned char *color2 = &srcimage2->palette[sizeof(RGBQUAD) * ((srcrow2[col / pixelsperbyte] >> ((col % pixelsperbyte) * srcimage2->bpp)) & bitmask)];
          register float sumR = addthis + ((add1 + (float) color1[0]) * (add2 + (float) color2[0])) * inv;
          register float sumG = addthis + ((add1 + (float) color1[1]) * (add2 + (float) color2[1])) * inv;
          register float sumB = addthis + ((add1 + (float) color1[2]) * (add2 + (float) color2[2])) * inv;
          register float sumA = addthis + ((add1 + (float) color1[3]) * (add2 + (float) color2[3])) * inv;

          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
          dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
    		}
      }

      // Now commit the changes if destiny == source
      if(this == srcimage1 || this == srcimage2) {
        this->bpp     = 32;
        this->rowsize = newrowsize;
        SAFE_DELETE_ARRAY(this->palette);
        this->palettecolors = 0;
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;

        // Internal representation
        this->colormasks[0] = 0xff;       // RED
        this->colormasks[1] = 0xff00;     // GREEN
        this->colormasks[2] = 0xff0000;   // BLUE
        this->colormasks[3] = 0xff000000; // ALPHA
        this->rightshift[0] = 0;  this->rightshift[1] = 8;  this->rightshift[2] = 16; this->rightshift[3] = 24;
        this->nbits[0] = 8;  this->nbits[1] = 8;  this->nbits[2] = 8; this->nbits[3] = 8;
        this->max[0] = 255;  this->max[1] = 255;  this->max[2] = 255;  this->max[3] = 255;
      }
    }
  } else {
    // Destination must be compatible with the source
    if(this != srcimage1 && this != srcimage2)
      if(!this->createSimilar(srcimage1)) { // createSimilar creates only what is needed to make them compatible.
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "combineMult(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

    if(srcimage1->bpp == 24) {
      addthis *= 255.0; // 8 bits/channel
      float inv = (float) (1.0 / 255.0);
      add1 *= 255.0;
      add2 *= 255.0;

    	// Combination with 24bits colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        register unsigned char *dstrow  = &this->data[row * rowsize];
        register unsigned char *srcrow1 = &srcimage1->data[row * rowsize];
        register unsigned char *srcrow2 = &srcimage2->data[row * rowsize];
    		for(unsigned int col = 0; col < width; col++, dstrow += 3, srcrow1 += 3, srcrow2 += 3) {
          register float sumR = addthis + ((add1 + (float) srcrow1[0]) * (add2 + (float) srcrow2[0])) * inv;
          register float sumG = addthis + ((add1 + (float) srcrow1[1]) * (add2 + (float) srcrow2[1])) * inv;
          register float sumB = addthis + ((add1 + (float) srcrow1[2]) * (add2 + (float) srcrow2[2])) * inv;
          // Check bounds
          dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
          dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
          dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
    		}
      }
    } else
      	if(srcimage1->bpp == 32) {
          addthis *= 255.0; // 8 bits/channel
          float inv = (float) (1.0 / 255.0);
          add1 *= 255.0;
          add2 *= 255.0;

          // Combination with 32bits colors in data buffer
          for(unsigned int row = 0; row < height; row++) {
            register unsigned char *dstrow  = &this->data[row * rowsize];
            register unsigned char *srcrow1 = &srcimage1->data[row * rowsize];
            register unsigned char *srcrow2 = &srcimage2->data[row * rowsize];
            for(unsigned int col = 0; col < width; col++, dstrow += 4, srcrow1 += 4, srcrow2 += 4) {
              register float sumR = addthis + ((add1 + (float) srcrow1[0]) * (add2 + (float) srcrow2[0])) * inv;
              register float sumG = addthis + ((add1 + (float) srcrow1[1]) * (add2 + (float) srcrow2[1])) * inv;
              register float sumB = addthis + ((add1 + (float) srcrow1[2]) * (add2 + (float) srcrow2[2])) * inv;
              register float sumA = addthis + ((add1 + (float) srcrow1[3]) * (add2 + (float) srcrow2[3])) * inv;

              // Check bounds
              dstrow[0] = (sumR < 0.0) ? 0 : ((sumR > 255.0) ? 255 : (unsigned char) sumR);
              dstrow[1] = (sumG < 0.0) ? 0 : ((sumG > 255.0) ? 255 : (unsigned char) sumG);
              dstrow[2] = (sumB < 0.0) ? 0 : ((sumB > 255.0) ? 255 : (unsigned char) sumB);
              dstrow[3] = (sumA < 0.0) ? 0 : ((sumA > 255.0) ? 255 : (unsigned char) sumA);
            }
          }
        } else
            if(srcimage1->bpp == 32) {
              // Combination with 16 bits colors in data buffer
              float addthisR = addthis * max[0];
              float addthisG = addthis * max[1];
              float addthisB = addthis * max[2];
              float addthisA = addthis * max[3];
              float invR = (float) (1.0 / (float) max[0]);
              float invG = (float) (1.0 / (float) max[1]);
              float invB = (float) (1.0 / (float) max[2]);
              float invA = (float) (1.0 / (float) max[3]);
              float add1R = add1 * max[0];
              float add1G = add1 * max[1];
              float add1B = add1 * max[2];
              float add1A = add1 * max[3];
              float add2R = add2 * max[0];
              float add2G = add2 * max[1];
              float add2B = add2 * max[2];
              float add2A = add2 * max[3];

              for(unsigned int row = 0; row < height; row++) {
                register unsigned short *dstrow  = (unsigned short*) &this->data[row * rowsize];
                register unsigned short *srcrow1 = (unsigned short*) &srcimage1->data[row * rowsize];
                register unsigned short *srcrow2 = (unsigned short*) &srcimage2->data[row * rowsize];
                for(unsigned int col = 0; col < width; col++, dstrow++) {
                  register int color1 = srcrow1[col], color2 = srcrow2[col];
                  register float sumR = addthisR + ((add1R + (float) ((color1 & srcimage1->colormasks[0]) >> srcimage1->rightshift[0])) *
                                                    (add2R + (float) ((color2 & srcimage2->colormasks[0]) >> srcimage2->rightshift[0]))) * invR;
                  register float sumG = addthisG + ((add1G + (float) ((color1 & srcimage1->colormasks[1]) >> srcimage1->rightshift[1])) *
                                                    (add2G + (float) ((color2 & srcimage2->colormasks[1]) >> srcimage2->rightshift[1]))) * invG;
                  register float sumB = addthisB + ((add1B + (float) ((color1 & srcimage1->colormasks[2]) >> srcimage1->rightshift[2])) *
                                                    (add2B + (float) ((color2 & srcimage2->colormasks[2]) >> srcimage2->rightshift[2]))) * invB;
                  register float sumA = addthisA + ((add1A + (float) ((srcimage1->colormasks[3]) ? ((color1 & srcimage1->colormasks[3]) >> srcimage1->rightshift[3]) : 0)) *
                                                    (add2A + (float) ((srcimage2->colormasks[3]) ? ((color2 & srcimage2->colormasks[3]) >> srcimage2->rightshift[3]) : 0))) * invA;
                  *dstrow = (unsigned short)
                      (((sumR < 0.0) ? 0 : ((sumR > max[0]) ? srcimage1->colormasks[0] : (((unsigned char) sumR) << srcimage1->rightshift[0]))) |
                      ((sumG < 0.0) ? 0 : ((sumG > max[1]) ? srcimage1->colormasks[1] : (((unsigned char) sumG) << srcimage1->rightshift[1]))) |
                      ((sumB < 0.0) ? 0 : ((sumB > max[2]) ? srcimage1->colormasks[2] : (((unsigned char) sumB) << srcimage1->rightshift[2]))) |
                      ((sumA < 0.0) ? 0 : ((sumA > max[3]) ? srcimage1->colormasks[3] : (((unsigned char) sumA) << srcimage1->rightshift[3]))));
                }
              }
            } else {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "combineMult(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", srcimage1->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                    return false;
                  }
  }

  // Ok, it's done
	return true;
}

bool gcgIMAGE::splitChannels(gcgIMAGE *red, gcgIMAGE *green, gcgIMAGE *blue, gcgIMAGE *alpha) {
  // Check source image
  if(this->data == NULL || this->width == 0 || this->height == 0 || this->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_INVALIDOBJECT), "splitChannels(): current image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameters
  if(red == NULL && green == NULL && blue == NULL && alpha == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "splitChannels(): all destinations are NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return true; // Nothing to do
  }

  if(red == this || green == this || blue == this || alpha == this) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "splitChannels(): no destination should be the current image (source). (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check destination images
  if(red) if(!red->createImage(this->width, this->height, 8)) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "splitChannels(): a call to createImage() has failed for red channel. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(green) if(!green->createImage(this->width, this->height, 8)) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "splitChannels(): a call to createImage() has failed for green channel. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(blue) if(!blue->createImage(this->width, this->height, 8)) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "splitChannels(): a call to createImage() has failed for blue channel. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  if(alpha) if(!alpha->createImage(this->width, this->height, 8)) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "splitChannels(): a call to createImage() has failed for alpha channel. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  unsigned int newrowsize = BITMAPWIDTHBYTES(this->width, 8);

  // is paletted?
  if(this->palettecolors > 0 && this->palette != NULL) {
  	// Converts palette colors
    int pixelsperbyte = (8 / this->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << this->bpp) - 1; // Computes de index mask

    // Set the indices.
    for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
      unsigned char *srcdata = &this->data[currentRow * this->rowsize];
      unsigned int pos = currentRow * newrowsize;
      for(unsigned int i = 0; i < width; srcdata++)
        for(int j = pixelsperbyte-1; j >= 0; j--, i++, pos++) {
          unsigned char *color = &this->palette[(((*srcdata) >> (j * this->bpp)) & bitmask) * sizeof(RGBQUAD)];
          if(red) red->data[pos] = color[0];
          if(green) green->data[pos] = color[1];
          if(blue) blue->data[pos] = color[2];
          if(alpha) alpha->data[pos] = color[3];
        }
    }
  } else
    if(this->bpp == 24) {
    	// Splits the 24 bpp colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *srcdata   = &this->data[row * rowsize];
        unsigned int pos = row * newrowsize;
    		for(unsigned int i = 0; i < width; i++, srcdata += 3, pos++) {
          if(red) red->data[pos] = srcdata[0];
          if(green) green->data[pos] = srcdata[1];
          if(blue) blue->data[pos] = srcdata[2];
        }
      }
    } else
      	if(this->bpp == 32) {
          // Splits the 32 bpp bitmap
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *srcdata   = &this->data[row * rowsize];
            unsigned int pos = row * newrowsize;
        		for(unsigned int i = 0; i < width; i++, srcdata += 4, pos++) {
              if(red) red->data[pos] = srcdata[0];
              if(green) green->data[pos] = srcdata[1];
              if(blue) blue->data[pos] = srcdata[2];
              if(alpha) alpha->data[pos] = srcdata[3];
            }
          }
        } else
            if(this->bpp == 16) {
              // Splits the 16 bpp colors
              for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
                unsigned short *srccolor = (unsigned short*) &this->data[currentRow * rowsize];
                unsigned int pos = currentRow * newrowsize;
                for(unsigned int i = 0; i < width; i++, srccolor++, pos++) {
                  if(red) red->data[pos] = (unsigned char) ((int) (((*srccolor & colormasks[0]) >> rightshift[0]) * 255) / max[0]);
                  if(green) green->data[pos] = (unsigned char) ((int) (((*srccolor & colormasks[1]) >> rightshift[1]) * 255) / max[1]);
                  if(blue) blue->data[pos] = (unsigned char) ((int) (((*srccolor & colormasks[2]) >> rightshift[2]) * 255) / max[2]);
                  if(alpha) alpha->data[pos] = (unsigned char) ((int) (colormasks[3]) ? ((((*srccolor & colormasks[3]) >> rightshift[3]) * 255) / max[3]) : 0);
                }
              }
            } else {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "splitChannels(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", this->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                    return false;
                  }

  // Ok, it's done
	return true;
}


bool gcgIMAGE::mergeChannels(gcgIMAGE *red, gcgIMAGE *green, gcgIMAGE *blue, gcgIMAGE *alpha) {
  // Check parameters
  if(red == this || green == this || blue == this || alpha == this) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "mergeChannels(): no source should be the current image. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check source images
  unsigned int _width, _height;
  if(red) {_width = red->width; _height = red->height; }
  else if(green) {_width = green->width; _height = green->height; }
       else if(blue) {_width = blue->width; _height = blue->height; }
            else if(alpha) {_width = alpha->width; _height = alpha->height; }
                 else {
                      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "mergeChannels(): all sources are NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
                      return false; // Nothing to do
                    }

  if(red) if(red->width != _width || red->height != _height || red->bpp != 8) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "mergeChannels(): red source is incompatible with another source. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(green) if(green->width != _width || green->height != _height || green->bpp != 8) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "mergeChannels(): green source is incompatible with another source. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(blue) if(blue->width != _width || blue->height != _height || blue->bpp != 8) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "mergeChannels(): blue source is incompatible with another source. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }
  if(alpha) if(alpha->width != _width || alpha->height != _height || alpha->bpp != 8) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "mergeChannels(): alpha source is incompatible with another source. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check destination
  if(alpha) {
    // Alpha channel is needed. Destination must have 16 or 32 bpp.
    if(this->bpp != 16 || this->bpp != 32)
      if(!this->createImage(_width, _height, 32, true)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "mergeChannels(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
  } else
    // Destination must be 16, 24 or 32 bpp
    if(this->bpp != 16 || this->bpp != 24 || this->bpp != 32)
      if(!this->createImage(_width, _height, 24, false)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "mergeChannels(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

  // Check size
  if(this->width != _width || this->height != _height)
    if(!this->createImage(_width, _height, this->bpp, (alpha != NULL))) {
      gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "mergeChannels(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

  unsigned int newrowsize = BITMAPWIDTHBYTES(this->width, 8);

  if(this->bpp == 24) {
    // Merges to the 24 bpp bitmap
    for(unsigned int row = 0; row < height; row++) {
      unsigned char *dstdata   = &this->data[row * rowsize];
      unsigned int pos = row * newrowsize;
      for(unsigned int i = 0; i < width; i++, dstdata += 3, pos++) {
        dstdata[0] = (red) ? red->data[pos] : 0;
        dstdata[1] = (green) ? green->data[pos] : 0;
        dstdata[2] = (blue) ? blue->data[pos] : 0;
      }
    }
  } else
      if(this->bpp == 32) {
        // Merges to the 32 bpp bitmap
        for(unsigned int row = 0; row < height; row++) {
          unsigned char *dstdata   = &this->data[row * rowsize];
          unsigned int pos = row * newrowsize;
          for(unsigned int i = 0; i < width; i++, dstdata += 4, pos++) {
            dstdata[0] = (red) ? red->data[pos] : 0;
            dstdata[1] = (green) ? green->data[pos] : 0;
            dstdata[2] = (blue) ? blue->data[pos] : 0;
            dstdata[3] = (alpha) ? alpha->data[pos] : 0;
          }
        }
      } else
          if(this->bpp == 16) {
            // Merges to the 16 bpp bitmap
            for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
              unsigned short *dstcolor = (unsigned short*) &this->data[currentRow * rowsize];
              unsigned int pos = currentRow * newrowsize;
              for(unsigned int i = 0; i < width; i++, dstcolor++, pos++) {
                int cred   = (red) ? red->data[pos] : 0;
                int cgreen = (green) ? green->data[pos] : 0;
                int cblue  = (blue) ? blue->data[pos] : 0;
                int calpha = (alpha) ? alpha->data[pos] : 0;

                *dstcolor = (unsigned short) ((((cred  * max[0]) / 255) << rightshift[0]) |
                              (((cgreen * max[1]) / 255) << rightshift[1]) |
                              (((cblue  * max[2]) / 255) << rightshift[2]) |
                              (((calpha * max[3]) / 255) << rightshift[3]));
              }
            }
          } else {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "mergeChannels(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", this->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                    return false;
                  }

  // Ok, it's done
	return true;
}

////////////////////////////////////////////////////////////
// Computes a per channel histogram of a RGB image.
// Returns true if histogram was computed.
////////////////////////////////////////////////////////////
bool gcgIMAGE::histogramRGBA(unsigned int hvectorR[256], unsigned int hvectorG[256], unsigned int hvectorB[256], unsigned int hvectorA[256]) {
  // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Init each vector
  if(hvectorR != NULL) memset(hvectorR, 0, sizeof(unsigned int) * 256);
  if(hvectorG != NULL) memset(hvectorG, 0, sizeof(unsigned int) * 256);
  if(hvectorB != NULL) memset(hvectorB, 0, sizeof(unsigned int) * 256);
  if(hvectorA != NULL) memset(hvectorA, 0, sizeof(unsigned int) * 256);

  // Is paletted?
  if(palettecolors > 0 && palette != NULL) {
    int  pixelsperbyte = (8 / bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << bpp) - 1; // Computes de index mask

    // Checking indexed data
    for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
      unsigned char *posindex = &data[currentRow * rowsize];
      for(unsigned int i = 0; i < width; posindex++)
        for(int j = pixelsperbyte-1; j >= 0; j--, i++) {
          unsigned char *color = &palette[(((*posindex) >> (j * bpp)) & bitmask) * sizeof(RGBQUAD)];
          if(hvectorR != NULL) hvectorR[color[0]]++;
          if(hvectorG != NULL) hvectorG[color[1]]++;
          if(hvectorB != NULL) hvectorB[color[2]]++;
          if(hvectorA != NULL) hvectorA[color[3]]++;
        }
    }
  } else
    if(bpp == 24) {
    	// Check each color in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *posdata   = &data[row * rowsize];
    		for(unsigned int i = 0; i < width; i++, posdata += 3) {
          if(hvectorR != NULL) hvectorR[posdata[0]]++;
          if(hvectorG != NULL) hvectorG[posdata[1]]++;
          if(hvectorB != NULL) hvectorB[posdata[2]]++;
        }
      }
    } else
      	if(bpp == 32) {
          // Converts the 32bpp bitmap
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *posdata   = &data[row * rowsize];
        		for(unsigned int i = 0; i < width; i++, posdata += 4) {
              if(hvectorR != NULL) hvectorR[posdata[0]]++;
              if(hvectorG != NULL) hvectorG[posdata[1]]++;
              if(hvectorB != NULL) hvectorB[posdata[2]]++;
              if(hvectorA != NULL) hvectorA[posdata[3]]++;
            }
          }
        } else
            // Check the 16bpp colors
            for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
           	  unsigned short *poscolor = (unsigned short*) &data[currentRow * rowsize];
          		for(unsigned int i = 0; i < width; i++, poscolor++) {
                if(hvectorR != NULL) hvectorR[(((*poscolor & colormasks[0]) >> rightshift[0]) * 255) / max[0]]++;
                if(hvectorG != NULL) hvectorG[(((*poscolor & colormasks[1]) >> rightshift[1]) * 255) / max[1]]++;
                if(hvectorB != NULL) hvectorB[(((*poscolor & colormasks[2]) >> rightshift[2]) * 255) / max[2]]++;
                if(hvectorA != NULL)
                  if(colormasks[3]) hvectorA[(((*poscolor & colormasks[3]) >> rightshift[3]) * 255) / max[3]]++;
              }
            }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}


////////////////////////////////////////////////////////////
// Computes a gray scale histogram of a RGB image. But does
// not convert current image to grayscale.
// Returns true if histogram was computed.
////////////////////////////////////////////////////////////
bool gcgIMAGE::histogramGray(unsigned int hvector[256]) {
  // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Init each vector
  memset(hvector, 0, sizeof(unsigned int) * 256);

  // Is paletted?
  if(palettecolors > 0 && palette != NULL) {
    int  pixelsperbyte = (8 / bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << bpp) - 1; // Computes de index mask

    // Checking indexed data
    for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
      unsigned char *posindex = &data[currentRow * rowsize];
      for(unsigned int i = 0; i < width; posindex++)
        for(int j = pixelsperbyte-1; j >= 0; j--, i++) {
          unsigned char *color = &palette[(((*posindex) >> (j * bpp)) & bitmask) * sizeof(RGBQUAD)];
          hvector[((int) color[0] + color[1] + color[2]) / 3]++;
        }
    }
  } else
    if(bpp == 24) {
    	// Check each color in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *posdata   = &data[row * rowsize];
    		for(unsigned int i = 0; i < width; i++, posdata += 3)
          hvector[((int) posdata[0] + posdata[1] + posdata[2]) / 3]++;
      }
    } else
      	if(bpp == 32) {
          // Converts the 32bpp bitmap
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *posdata   = &data[row * rowsize];
        		for(unsigned int i = 0; i < width; i++, posdata += 4)
              hvector[((int) posdata[0] + posdata[1] + posdata[2]) / 3]++;
          }
        } else
            // Check the 16bpp colors
            for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
           	  unsigned short *poscolor = (unsigned short*) &data[currentRow * rowsize];
          		for(unsigned int i = 0; i < width; i++, poscolor++)
                hvector[(int) (((((*poscolor & colormasks[0]) >> rightshift[0]) * 255) / max[0] +
                                 (((*poscolor & colormasks[1]) >> rightshift[1]) * 255) / max[1] +
                                 (((*poscolor & colormasks[2]) >> rightshift[2]) * 255) / max[2]) / 3)]++;
            }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;
	return true;
}



////////////////////////////////////////////////////////////
// Computes the index histogram of a paletted image.
// Paletted images only. Returns the number of bins or zero
// if image is invalid.
////////////////////////////////////////////////////////////
int gcgIMAGE::histogramIndex(unsigned int vectorsize, unsigned int *histovector) {
  // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Is really paletted?
  //errorcode = GCG_UNSUPPORTED_FORMAT;
  if(this->palettecolors == 0 || this->palette == NULL) return 0; // Only paletted images

  // Get the number of bins
  vectorsize = MIN(vectorsize, (unsigned int) 0x1 << bpp);
  vectorsize = MIN(vectorsize, this->palettecolors);
  memset(histovector, 0, sizeof(unsigned int) * vectorsize);

  // Now, adjusts all data indices for the new palette.
  int  pixelsperbyte = (8 / bpp); // Compute the number of colors per data byte.
  unsigned int bitmask = (0x01 << bpp) - 1; // Computes de index mask

	for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
    unsigned char *posdata  = &data[currentRow * rowsize];
		for(unsigned int i = 0; i < width; posdata++)
      for(int j = 0, k; j < pixelsperbyte; j++, i++)
        if(((k = *posdata >> (j * bpp)) & bitmask) < vectorsize) histovector[k]++;
  }

  // Histogram computed
  return vectorsize;
}


////////////////////////////////////////////////////////////
// Binarize this image using the RGB average.
////////////////////////////////////////////////////////////
bool gcgIMAGE::binarizeGray(gcgIMAGE *src, int threshold) {
  // Nothing to do...
  if(threshold < 0) return true;

  // Check source image
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Check destination image: destination must be compatible with the source
  if(this != src)
    if(!this->createSimilar(src)) return false; // createSimilar creates only what is needed to make them compatible.

  // is paletted?
  if(palettecolors > 0 && palette != NULL) {
    if(this != src) memcpy(this->data, src->data, rowsize * height);

  	// Converts palette colors
  	unsigned char *srcpalette = src->palette;
  	unsigned char *dstpalette = this->palette;
  	for(unsigned int i = 0; i < palettecolors; i++, srcpalette += sizeof(RGBQUAD), dstpalette += sizeof(RGBQUAD))
      dstpalette[0] = dstpalette[1] = dstpalette[2] = ((int) (srcpalette[0] + srcpalette[1] + srcpalette[2]) / 3 > threshold) ? 255 : 0;
  } else
    if(bpp == 24) {
    	// Converts 24bpp colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *srcdata   = &src->data[row * rowsize];
        unsigned char *dstdata   = &this->data[row * rowsize];

    		for(unsigned int i = 0; i < width; i++, srcdata += 3, dstdata += 3)
          dstdata[0] = dstdata[1] = dstdata[2] = ((int) (srcdata[0] + srcdata[1] + srcdata[2]) / 3 > threshold) ? 255 : 0;
      }
    } else
      	if(bpp == 32) {
          // Converts the 32bpp bitmap
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *srcdata   = &src->data[row * rowsize];
            unsigned char *dstdata   = &this->data[row * rowsize];
        		for(unsigned int i = 0; i < width; i++, srcdata += 4, dstdata += 4)
              dstdata[0] = dstdata[1] = dstdata[2] = ((int) (srcdata[0] + srcdata[1] + srcdata[2]) / 3 > threshold) ? 255 : 0;
          }
        } else {
            // Converts the 16bpp colors
            for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
           	  unsigned short *srccolor = (unsigned short*) &src->data[currentRow * rowsize];
           	  unsigned short *dstcolor = (unsigned short*) &this->data[currentRow * rowsize];
          		for(unsigned int i = 0; i < width; i++, srccolor++, dstcolor++) {
                unsigned int alpha = (colormasks[3]) ? (((*srccolor & colormasks[3]) >> rightshift[3]) * 255) / max[3] : 0;
                unsigned int gray  = ((int) (((((*srccolor & colormasks[0]) >> rightshift[0]) * 255) / max[0] +
                                      (((*srccolor & colormasks[1]) >> rightshift[1]) * 255) / max[1] +
                                      (((*srccolor & colormasks[2]) >> rightshift[2]) * 255) / max[2]) / 3) > threshold) ? 255 : 0;
                *dstcolor = (unsigned short)  (((((gray  * max[0]) / 255) << rightshift[0]) |
                                              (((gray  * max[1]) / 255) << rightshift[1]) |
                                              (((gray  * max[2]) / 255) << rightshift[2]) |
                                              (((alpha * max[3]) / 255) << rightshift[3])));
              }
            }
          }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}



////////////////////////////////////////////////////////////
// Binarize RGBA channels independently. Negative values
// indicate that the channel should not be binarized.
////////////////////////////////////////////////////////////
bool gcgIMAGE::binarizeRGBA(gcgIMAGE *src, int threR, int threG, int threB, int threA) {
  // Nothing to do...
  if(threR < 0 && threG < 0 && threB < 0 && threA < 0) {
    //errorcode = GCG_SUCCESS;
    return true;
  }

  // Check source image
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Check destination image: destination must be compatible with the source
  if(this != src)
    if(!this->createSimilar(src)) return false; // createSimilar creates only what is needed to make them compatible.

  // is paletted?
  if(palettecolors > 0 && palette != NULL) {
    if(this != src) memcpy(this->data, src->data, rowsize * height);

  	// Converts palette colors
  	unsigned char *srcpalette = src->palette;
  	unsigned char *dstpalette = this->palette;
  	for(unsigned int i = 0; i < palettecolors; i++, srcpalette += sizeof(RGBQUAD), dstpalette += sizeof(RGBQUAD)) {
      if(threR >= 0) dstpalette[0] = (srcpalette[0] >= threR) ? 255 : 0; else dstpalette[0] = srcpalette[0];
      if(threG >= 0) dstpalette[1] = (srcpalette[1] >= threG) ? 255 : 0; else dstpalette[1] = srcpalette[1];
      if(threB >= 0) dstpalette[2] = (srcpalette[2] >= threB) ? 255 : 0; else dstpalette[2] = srcpalette[2];
      if(threA >= 0) dstpalette[3] = (srcpalette[3] >= threA) ? 255 : 0; else dstpalette[3] = srcpalette[3];
    }
  } else
    if(bpp == 24) {
    	// Converts 24bpp colors in data buffer
    	for(unsigned int row = 0; row < height; row++) {
        unsigned char *srcdata   = &src->data[row * rowsize];
        unsigned char *dstdata   = &this->data[row * rowsize];
    		for(unsigned int i = 0; i < width; i++, srcdata += 3, dstdata += 3) {
          if(threR >= 0) dstdata[0] = (srcdata[0] >= threR) ? 255 : 0; else dstdata[0] = srcdata[0];
          if(threG >= 0) dstdata[1] = (srcdata[1] >= threG) ? 255 : 0; else dstdata[1] = srcdata[1];
          if(threB >= 0) dstdata[2] = (srcdata[2] >= threB) ? 255 : 0; else dstdata[2] = srcdata[2];
        }
      }
    } else
      	if(bpp == 32) {
          // Converts the 32bpp bitmap
        	for(unsigned int row = 0; row < height; row++) {
            unsigned char *srcdata   = &src->data[row * rowsize];
            unsigned char *dstdata   = &this->data[row * rowsize];
        		for(unsigned int i = 0; i < width; i++, srcdata += 4, dstdata += 4) {
              if(threR >= 0) dstdata[0] = (srcdata[0] >= threR) ? 255 : 0; else dstdata[0] = srcdata[0];
              if(threG >= 0) dstdata[1] = (srcdata[1] >= threG) ? 255 : 0; else dstdata[1] = srcdata[1];
              if(threB >= 0) dstdata[2] = (srcdata[2] >= threB) ? 255 : 0; else dstdata[2] = srcdata[2];
              if(threA >= 0) dstdata[3] = (srcdata[3] >= threA) ? 255 : 0; else dstdata[3] = srcdata[3];
            }
          }
        } else {
            // Converts the 16bpp colors
            for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
           	  unsigned short *srccolor = (unsigned short*) &src->data[currentRow * rowsize];
           	  unsigned short *dstcolor = (unsigned short*) &this->data[currentRow * rowsize];
          		for(unsigned int i = 0; i < width; i++, srccolor++, dstcolor++) {
                int red   = (((*srccolor & colormasks[0]) >> rightshift[0]) * 255) / max[0];
                int green = (((*srccolor & colormasks[1]) >> rightshift[1]) * 255) / max[1];
                int blue  = (((*srccolor & colormasks[2]) >> rightshift[2]) * 255) / max[2];
                int alpha = (colormasks[3]) ? ((((*srccolor & colormasks[3]) >> rightshift[3]) * 255) / max[3]) : 0;
                if(threR >= 0) red   = (red >= threR) ? 255 : 0;
                if(threG >= 0) green = (green >= threG) ? 255 : 0;
                if(threB >= 0) blue  = (blue >= threB) ? 255 : 0;
                if(threA >= 0) alpha = (alpha >= threA) ? 255 : 0;

                *dstcolor = (unsigned short) ((((red  * max[0]) / 255) << rightshift[0]) |
                            (((green * max[1]) / 255) << rightshift[1]) |
                            (((blue  * max[2]) / 255) << rightshift[2]) |
                            (((alpha * max[3]) / 255) << rightshift[3]));
              }
            }
          }

  // Ok, it's done
	//errorcode = GCG_SUCCESS;

	return true;
}

////////////////////////////////////////////////////////////
// Computes a binary image using the threshold.
// Paletted images only. Returns a 1 bit binary image or NULL
// if this image is invalid.
////////////////////////////////////////////////////////////
bool gcgIMAGE::binarizeIndex(gcgIMAGE *src, int threshold) {
  // Check source image
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

  // Is really paletted?
  //errorcode = GCG_UNSUPPORTED_FORMAT;
  if(src->palettecolors == 0 || src->palette == NULL) return false; // Only paletted images

  // If bpp = 1, there is nothing to do
  if(bpp == 1) {
    if(this != src) return this->duplicateImage(src); // Simply copy image
    //errorcode = GCG_SUCCESS;
    return true;
  }

  // Check destination image: destination must be compatible with the source
  if(this != src)
    if(!this->createSimilar(src)) return false; // createSimilar creates only what is needed to make them compatible.

 	// Check all indices
  int  pixelsperbyte = (8 / bpp); // Compute the number of colors per data byte.
  unsigned int bitmask = (0x01 << bpp) - 1; // Computes de index mask
  int white = (palettecolors - 1) & bitmask;

	for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
    unsigned char *srcdata  = &src->data[currentRow * rowsize];
    unsigned char *dstdata  = &this->data[currentRow * rowsize];
		for(unsigned int i = 0; i < width; srcdata++, dstdata++) {
      int newbyte = 0;
      for(int j = 0, k; j < pixelsperbyte; j++, i++) {
        k = (*srcdata >> (j * bpp)) & bitmask;
        if(k >= threshold) newbyte |= (white << (j * bpp)); // Receives maximal index = mask. Else the minimum index = 0.
      }
      *dstdata = (unsigned char) newbyte;
  	}
  }

  // Adjust palette
  palette[0] = palette[1] = palette[2] = palette[3] = 0;  // Black
  palette[white * sizeof(RGBQUAD) + 0] = palette[white * sizeof(RGBQUAD) + 1] = palette[white * sizeof(RGBQUAD) + 2] = 255;
  palette[white * sizeof(RGBQUAD) + 3] = 0;

  // Binary image computed
  //errorcode = GCG_SUCCESS;
  return true;
}

////////////////////////////////////////////////////////////
// Load a windows bitmap from a BITMAPINFO memory structure.
////////////////////////////////////////////////////////////
bool gcgIMAGE::unpackBMP(void *bmpHeader, unsigned char *bmpdata, unsigned int npalcolors, unsigned char *paldata) {
  // Check bitmap pointer
  if(bmpHeader == NULL || bmpdata == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "unpackBMP(): header or data is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  BITMAPINFOHEADER *infoHeader = (BITMAPINFOHEADER*) bmpHeader;

  // Bitmap header
	bool flip = false, res;

	if(infoHeader->biHeight < 0) {
      infoHeader->biHeight = -infoHeader->biHeight;
      flip = true;
  }

	// Data reading by bpp
	switch(infoHeader->biBitCount) {
    case  1:
    case  2:
    case  4:
    case  8: res = unpackPalettedBMP(bmpHeader, bmpdata, npalcolors, paldata);  break;
    case 16: res = unpack16BitBMP(bmpHeader, bmpdata, npalcolors, paldata);     break;
    case 24: res = unpack24BitBMP(bmpHeader, bmpdata);                          break;
    case 32: res = unpack32BitBMP(bmpHeader, bmpdata, npalcolors, paldata);     break;

    default:  // Bits unsupported in this version.
              gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "unpackBMP(): unsupported bits per pixel %d. (%s:%d)", infoHeader->biBitCount, basename((char*)(char*)__FILE__), __LINE__);
              return false;
  }

  if(res) {
    if(flip) verticalFlip();
  } else destroyImage();

	return res;
}


////////////////////////////////////////////////////////////
// Flips the image vertically
////////////////////////////////////////////////////////////
bool gcgIMAGE::verticalFlip() {
  // This image is valid?
  if(data == NULL || width == 0 || height == 0 || bpp == 0) {
    //errorcode = GCG_INVALID_OBJECT;
    return false;
  }

	int nrows = (height % 2 == 1) ? ((height-1) >> 1) : (height >> 1);

	//create space for a temporary row
	unsigned char *tmp = new unsigned char[rowsize];
	//errorcode = GCG_INTERNAL_ERROR;
	if(!tmp) return false;

 	// Swaping rows
	for(int i = 0; i < nrows; i++) {
		memcpy(tmp, &data[i * rowsize], rowsize);
		memcpy(&data[i * rowsize], &data[(height-i-1) * rowsize], rowsize);
		memcpy(&data[(height-i-1) * rowsize], tmp, rowsize);
	}

	SAFE_DELETE_ARRAY(tmp);

	//errorcode = GCG_SUCCESS;
	return true;
}


bool gcgIMAGE::loadUncompressed8BitTGA(FILE *file) {
	unsigned char		header[6];

	if(fread(header, sizeof(header), 1, file) != 1) {
    fclose(file);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadUncompressed8BitTGA(): invalid TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	// Clear the current data if object was already used
	destroyImage();

	// Get image information
	width  = header[1] * 256 + header[0];
	height = header[3] * 256 + header[2];
	bpp    = header[4];

  // Check information
	if(width <= 0	|| height <= 0	||	bpp != 8)	{
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "loadUncompressed8BitTGA(): image is empty or has not 8 bits per pixel. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

	// Compute rowsize
	rowsize = BITMAPWIDTHBYTES(width, bpp);
 	palettecolors = 256;

 // Allocate memory for the image palette
	this->palette = new unsigned char[sizeof(RGBQUAD) * palettecolors];
	unsigned char *temp = new unsigned char[3 * palettecolors];
	if(this->palette == NULL || temp == NULL) {
		fclose(file);
		destroyImage();
		SAFE_DELETE_ARRAY(temp);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "loadUncompressed8BitTGA(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

	// Reading palette
	if(fread(temp, sizeof(unsigned char) * 3, palettecolors, file) != palettecolors) {
		fclose(file);
		destroyImage();
		SAFE_DELETE_ARRAY(temp);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadUncompressed8BitTGA(): error reading TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

	// Now, move temp colors to the effective palette
	for(unsigned int i = 0; i < palettecolors; i++) {
    palette[i * sizeof(RGBQUAD) + 0] = temp[i * 3 + 0];
    palette[i * sizeof(RGBQUAD) + 1] = temp[i * 3 + 1];
    palette[i * sizeof(RGBQUAD) + 2] = temp[i * 3 + 2];
    palette[i * sizeof(RGBQUAD) + 3] = 0;
  }
	SAFE_DELETE_ARRAY(temp);

	// Allocate memory for pixel data (paletted)
	this->data  = new unsigned char[rowsize * height];
	if(this->data == NULL) {
    destroyImage();
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "loadUncompressed8BitTGA(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

	// Load each scanline of indices
	for(unsigned int i = 0; i < height; i++)
	 if(fread(&data[i * rowsize], width, 1, file) != 1) {
    destroyImage();
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadUncompressed8BitTGA(): error reading TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	 }

	// Close the file
	fclose(file);

	// Converts palette colors to RGB
	unsigned char *color = palette;
	for(unsigned int i = 0; i < palettecolors; i++, color += sizeof(RGBQUAD))
		SWAP(color[0], color[2]); // Swap b and r:  repeated XOR to swap bytes 0 and 2

  // Paletted does not use masks
  memset(colormasks, 0, sizeof(colormasks));
  memset(rightshift, 0, sizeof(rightshift));
  memset(nbits, 0, sizeof(nbits));
  memset(max, 0, sizeof(max));

  // Ok, it's loaded.
	return true;
}


bool gcgIMAGE::loadUncompressedTrueColorTGA(FILE *file) {
	unsigned char	header[6];

	if(fread(header, sizeof(header), 1, file) != 1) {
    fclose(file);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadUncompressedTrueColorTGA(): invalid TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	// Clear the current data if object was already used
	destroyImage();

	// Get image information
	width  = header[1] * 256 + header[0];
	height = header[3] * 256 + header[2];
	bpp    = header[4];

  // Check information
	if(width <= 0	|| height <= 0	||	(bpp != 24 && bpp != 32))	{
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "loadUncompressedTrueColorTGA(): image is empty or has not 24 or 32 bits per pixel. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

	// Compute rowsize
	rowsize = BITMAPWIDTHBYTES(width, bpp);
 	palettecolors = 0;
 	palette = NULL;

	// Allocate memory for pixel data
	this->data  = new unsigned char[rowsize * height];
	if(this->data == NULL) {
    destroyImage();
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "loadUncompressedTrueColorTGA(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

	// Load each scanline of pixels
	int nbytes = bpp / 8;
	for(unsigned int i = 0; i < height; i++)
	 if(fread(&data[i * rowsize], width * nbytes, 1, file) != 1) {
    destroyImage();
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadUncompressedTrueColorTGA(): error reading TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	 }

	// Close the file
	fclose(file);

	// Set appropriate masks
  colormasks[0] = 0xff;     // RED
  colormasks[1] = 0xff00;   // GREEN
  colormasks[2] = 0xff0000; // BLUE
  colormasks[3] = 0x00;     // ALPHA
  rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 0;
  nbits[0] = 8; nbits[1] = 8; nbits[2] = 8; nbits[3] = 0;
  max[0] = 255; max[1] = 255; max[2] = 255; max[3] = 0;

  if(bpp == 32) {
    colormasks[3] = 0xff000000; // ALPHA
    rightshift[3] = 24;
    nbits[3] = 8;
    max[3] = 255;
  }

	// Bitmap data is in BGR format
	for(unsigned int row = 0; row < height; row++) {
    unsigned char *color = &data[row * rowsize];
		for(unsigned int i = 0; i < width; i++, color += nbytes)
			SWAP(color[0], color[2]);  // Swap b and r:  repeated XOR to swap bytes 0 and 2
  }

  // Ok, it's loaded.
	return true;
}


////////////////////////////////////////////////////////////
// Load a 24bpp or 32bpp TGA image, compressed
////////////////////////////////////////////////////////////
bool gcgIMAGE::loadCompressedTrueColorTGA(FILE *file) {
	unsigned char	header[6];

	if(fread(header, sizeof(header), 1, file) != 1) {
    fclose(file);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_FORMATMISMATCH), "loadCompressedTrueColorTGA(): invalid TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

	// Clear the current data if object was already used
	destroyImage();

	// Get image information
	width  = header[1] * 256 + header[0];
	height = header[3] * 256 + header[2];
	bpp    = header[4];

  // Check information
	if(width <= 0	|| height <= 0	||	(bpp != 24 && bpp != 32))	{
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "loadCompressedTrueColorTGA(): image is empty or has not 24 or 32 bits per pixel. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

	// Compute rowsize
	rowsize = BITMAPWIDTHBYTES(width, bpp);
 	palettecolors = 0;
 	palette = NULL;

	// Allocate memory for pixel data
	data  = new unsigned char[rowsize * height];
	if(!data) {
    destroyImage();
		fclose(file);
		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "loadCompressedTrueColorTGA(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
		return false;
	}

	// Read pixel data
	int npixels = height * width;
	int ipixel = 0;
	unsigned char color[4];
	unsigned int nbytes = bpp / 8;

  // RLE decompression
	do {
		unsigned char chunkHeader = 0;

		if(fread(&chunkHeader, sizeof(unsigned char), 1, file) != 1) {
      destroyImage();
  		fclose(file);
  		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadCompressedTrueColorTGA(): error reading TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
  		return false;
		}

    // Read raw color values
		if(chunkHeader < 128)	{
			chunkHeader++;

			for(int i = 0; i < chunkHeader; i++) {
				if(fread(color, nbytes, 1, file) != 1) {
          destroyImage();
      		fclose(file);
 		  		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadCompressedTrueColorTGA(): error reading TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      		return false;
				}

				// Copies pixel color to data swapping RED and BLUE values
				unsigned int pos = (ipixel / width)*rowsize + (ipixel % width) * nbytes;
				data[pos]     = color[2];
				data[pos + 1] = color[1];
				data[pos + 2] = color[0];
				if(nbytes == 4) data[pos + 3] = color[3];

				if(++ipixel > npixels) {
          destroyImage();
      		fclose(file);
 		  		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadCompressedTrueColorTGA(): error reading TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      		return false;
				}
			}
		} else { // Run length if ChunkHeader >= 128
			chunkHeader -= 127;

      // Read color for this run
			if(fread(color, nbytes, 1, file) != 1) {
        destroyImage();
    		fclose(file);
	  		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadCompressedTrueColorTGA(): error reading TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    		return false;
			}

			for(int i = 0; i < chunkHeader; i++) {
				// Copies pixel color to data swapping RED and BLUE values
				unsigned int pos = (ipixel / width)*rowsize + (ipixel % width) * nbytes;
				data[pos]     = color[2];
				data[pos + 1] = color[1];
				data[pos + 2] = color[0];
				if(nbytes == 4) data[pos + 3] = color[3];

				if(++ipixel > npixels) {
          destroyImage();
      		fclose(file);
 		  		gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_IOERROR), "loadCompressedTrueColorTGA(): error reading TGA file. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      		return false;
				}
			}
		}
	} while(ipixel < npixels);

	// Close the file
	fclose(file);

	// Set appropriate masks
  colormasks[0] = 0xff;     // RED
  colormasks[1] = 0xff00;   // GREEN
  colormasks[2] = 0xff0000; // BLUE
  colormasks[3] = 0x00;     // ALPHA
  rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 0;
  nbits[0] = 8; nbits[1] = 8; nbits[2] = 8; nbits[3] = 0;
  max[0] = 255; max[1] = 255; max[2] = 255; max[3] = 0;

  if(bpp == 32) {
    colormasks[3] = 0xff000000; // ALPHA
    rightshift[3] = 24;
    nbits[3] = 8;
    max[3] = 255;
  }

  // Ok, it's loaded.
	return true;
}


bool gcgIMAGE::unpackPalettedBMP(void *bmpHeader, unsigned char *bmpdata, unsigned int npalcolors, unsigned char *pal) {
  // Check compression
  BITMAPINFOHEADER *infoHeader = (BITMAPINFOHEADER*) bmpHeader;
  if(infoHeader->biCompression != BI_RGB && infoHeader->biCompression != BI_RLE8 && infoHeader->biCompression != BI_RLE4) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "unpackPalettedBMP(): invalid compression %d. (%s:%d)", infoHeader->biCompression, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check image equivalence
  if(infoHeader->biWidth != (int) width || infoHeader->biHeight != (int) height ||
     infoHeader->biBitCount != bpp || palettecolors != npalcolors) {
     // Destroy old image
     destroyImage();

     // Image information
     width   = infoHeader->biWidth;
     height  = infoHeader->biHeight;
     bpp     = infoHeader->biBitCount;
     rowsize = BITMAPWIDTHBYTES(width, bpp); // Compute the stride in bytes between one row and the next
     palettecolors = npalcolors;

     // Allocate palette
     if(palettecolors > 0) {
  	   palette = new unsigned char[palettecolors * sizeof(RGBQUAD)];
  	   if(palette == NULL) {
  	     gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "unpackPalettedBMP(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
  	     return false;
  	   }
     } else palette = NULL;

     // Allocate memory
     data = new unsigned char[rowsize * height];
     if(data == NULL) {
       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "unpackPalettedBMP(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
       destroyImage();
       return false;
     }
  }

  if(palettecolors > 0) {
  	// Load the palette and convert colors to RGB
    unsigned char *color = palette;
    for(unsigned int i = 0; i < palettecolors; i++, color += sizeof(RGBQUAD), pal += sizeof(RGBQUAD)) {
      color[0] = pal[2]; // Swap b and r
      color[1] = pal[1];
      color[2] = pal[0];
      color[3] = pal[3];
    }
  }

	// Allocate space for color indices
  if(bpp == 8 && infoHeader->biCompression == BI_RLE8) gcgDecompressImageRLE8(width, height, bmpdata, this->data);
  else
    if(bpp == 4 && infoHeader->biCompression == BI_RLE4) gcgDecompressImageRLE4(width, height, bmpdata, this->data);
    else memcpy(data, bmpdata, rowsize * height); // Load indices to colors in the palette

  // Paletted does not use masks
  memset(colormasks, 0, sizeof(colormasks));
  memset(rightshift, 0, sizeof(rightshift));
  memset(nbits, 0, sizeof(nbits));
  memset(max, 0, sizeof(max));

	// Ok, it's decoded.
  return true;
}


bool gcgIMAGE::unpack16BitBMP(void *bmpHeader, unsigned char *bmpdata, unsigned int npalcolors, unsigned char *pal) {
  // Check compression
  BITMAPINFOHEADER *infoHeader = (BITMAPINFOHEADER*) bmpHeader;
  unsigned int newcolormasks[4];

 	if(infoHeader->biCompression == BI_RGB) {
    newcolormasks[2] = 31; // Five least significant bits = BLUE
    newcolormasks[0] = (newcolormasks[2] << 10);  // RED
    newcolormasks[1] = (newcolormasks[2] << 5);   // GREEN
    newcolormasks[3] = 0x00;           // ALPHA
  } else
    if(infoHeader->biCompression == BI_BITFIELDS) {
      // The masks are informed by the next three unsigned int in the file
      if(npalcolors < 3) {
        // Not enough masks. Corrupted?
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "unpack16BitBMP(): small number of colors in palette %d. (%s:%d)", npalcolors, basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

      // Reading masks. Up to 4 masks are copied: RED GREEN BLUE ALPHA
      newcolormasks[3] = 0x00;
    	memcpy(newcolormasks, pal, sizeof(unsigned int) * ((npalcolors == 3) ? 3 : 4));
      npalcolors -= (npalcolors == 3) ? 3 : 4; // Adjusts number of colors in the palette
    } else {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "unpack16BitBMP(): invalid compression %d. (%s:%d)", infoHeader->biCompression, basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

  // Check image equivalence
  if(infoHeader->biWidth != (int) width || infoHeader->biHeight != (int) height ||
     infoHeader->biBitCount != bpp || palettecolors != npalcolors) {
     // Destroy old image
     destroyImage();

     // Image information
     width   = infoHeader->biWidth;
     height  = infoHeader->biHeight;
     bpp     = infoHeader->biBitCount;
     rowsize = BITMAPWIDTHBYTES(width, bpp); // Compute the stride in bytes between one row and the next
     palettecolors = 0;  // We won't need the available palette information. It was conceived
                         // for optimization on paletted devices.
     // Allocate memory
     data = new unsigned char[rowsize * height];
     if(data == NULL) {
       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "unpack16BitBMP(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
       return false;
     }
  }

  // Check color masks
  if(memcmp(this->colormasks, newcolormasks, sizeof(colormasks)) != 0) {
    memcpy(colormasks, newcolormasks, sizeof(colormasks));

    // Compute required shift and number of bit in each mask
    for(int i = 0; i < 4; i++) {
     unsigned int mask;
     for(mask = colormasks[i], rightshift[i] = 0; colormasks[i] > 0 && rightshift[i] < 32; rightshift[i]++, mask = (mask >> 1))
       if((mask & 0x01) != 0) break;

     for(nbits[i] = 0; nbits[i] < 32; nbits[i]++, mask = (mask >> 1))
       if((mask & 0x01) == 0) break;

     max[i] = (colormasks[i] >> rightshift[i]);

     // Check number of bits of each channel
     if(i < 3 && nbits[i] > 8) {
       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "unpack16BitBMP(): number of bits for a channel is greater than 8. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
       return false;
     }
    }
  }

	// Read in the data
	memcpy(this->data, bmpdata, rowsize * height);

  // Ok, it's loaded.
	return true;
}

bool gcgIMAGE::unpack24BitBMP(void *bmpHeader, unsigned char *bmpdata) {
  // Check compression
  BITMAPINFOHEADER *infoHeader = (BITMAPINFOHEADER*) bmpHeader;
  if(infoHeader->biCompression != BI_RGB) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "unpack24BitBMP(): invalid compression %d. (%s:%d)", infoHeader->biCompression, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Check image equivalence
  if(infoHeader->biWidth != (int) width || infoHeader->biHeight != (int) height ||
    infoHeader->biBitCount != bpp || palettecolors != 0) {
    // Destroy old image
    destroyImage();

    // Image information
    width   = infoHeader->biWidth;
    height  = infoHeader->biHeight;
    bpp     = infoHeader->biBitCount;
    rowsize = BITMAPWIDTHBYTES(width, bpp); // Compute the stride in bytes between one row and the next

    // We won't need the available palette information. It was conceived
    // for optimization on paletted devices.
    palettecolors = 0;

    // Allocate memory
    data = new unsigned char[rowsize * height];
    if(data == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "unpack24BitBMP(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }
  }

	// Bitmap data is in BGR format
	for(unsigned int row = 0; row < height; row++) {
    unsigned char *color = &data[row * rowsize];
    unsigned char *src   = &bmpdata[row * rowsize];
		for(unsigned int i = 0; i < width; i++, color += 3, src += 3) {
			color[0] = src[2]; // Swap b and r
			color[1] = src[1];
			color[2] = src[0];
		}
  }

  // Internal representation
  colormasks[0] = 0xff;     // RED
  colormasks[1] = 0xff00;   // GREEN
  colormasks[2] = 0xff0000; // BLUE
  colormasks[3] = 0x00;     // ALPHA
  rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 0;
  nbits[0] = 8; nbits[1] = 8; nbits[2] = 8; nbits[3] = 0;
  max[0] = 255; max[1] = 255; max[2] = 255; max[3] = 0;

  // Ok, it's loaded.
	return true;
}


bool gcgIMAGE::unpack32BitBMP(void *bmpHeader, unsigned char *bmpdata, unsigned int npalcolors, unsigned char *pal) {
  BITMAPINFOHEADER *infoHeader = (BITMAPINFOHEADER*) bmpHeader;
  unsigned int newcolormasks[4];

 	if(infoHeader->biCompression == BI_RGB) {
    newcolormasks[0] = 0xff0000;   // RED
    newcolormasks[1] = 0xff00;     // GREEN
    newcolormasks[2] = 0xff;       // Eight least significant bits = BLUE
    newcolormasks[3] = 0xff000000; // ALPHA
  } else
    if(infoHeader->biCompression == BI_BITFIELDS) {
      // The masks are informed by the next three unsigned int in the file
      if(npalcolors < 3) {
        // Not enough masks. Corrupted?
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "unpack32BitBMP(): small number of colors in palette %d. (%s:%d)", npalcolors, basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

      // Reading masks. Up to 4 masks are read: RED GREEN BLUE ALPHA
      newcolormasks[3] = 0x00;
    	memcpy(newcolormasks, pal, sizeof(unsigned int) * ((npalcolors == 3) ? 3 : 4));
      npalcolors -= (npalcolors == 3) ? 3 : 4; // Adjusts number of colors in the palette
    } else {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "unpack32BitBMP(): invalid compression %d. (%s:%d)", infoHeader->biCompression, basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

  // Check image equivalence
  if(infoHeader->biWidth != (int) width || infoHeader->biHeight != (int) height ||
     infoHeader->biBitCount != bpp || palettecolors != npalcolors) {
     // Destroy old image
     destroyImage();

     // Image information
     width   = infoHeader->biWidth;
     height  = infoHeader->biHeight;
     bpp     = infoHeader->biBitCount;
     rowsize = BITMAPWIDTHBYTES(width, bpp); // Compute the stride in bytes between one row and the next
     palettecolors = 0;  // We won't need the available palette information. It was conceived
                         // for optimization on paletted devices.
     // Allocate memory
     data = new unsigned char[rowsize * height];
     if(data == NULL) {
       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "unpack32BitBMP(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
       return false;
     }
  }

  // Compute required shift and number of bits in each mask
  for(int i = 0; i < 4; i++) {
    unsigned int mask;
    for(mask = newcolormasks[i], rightshift[i] = 0; newcolormasks[i] > 0 && rightshift[i] < 32; rightshift[i]++, mask = (mask >> 1))
      if((mask & 0x01) != 0) break;

    for(nbits[i] = 0; nbits[i] < 32; nbits[i]++, mask = (mask >> 1))
      if((mask & 0x01) == 0) break;

    max[i] = (colormasks[i] >> rightshift[i]);

    // Check number of bits of each channel
    if(i < 3 && nbits[i] > 8) {
       gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDFORMAT), "unpack32BitBMP(): number of bits for a channel is greater than 8. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
       return false;
     }
  }

	// Converts the bitmap to R G B A
	for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
 	  unsigned char *posdata = (unsigned char*) &data[currentRow * rowsize];
 	  unsigned char *srcdata = (unsigned char*) &bmpdata[currentRow * rowsize];
		for(unsigned int i = 0; i < width; i++, posdata += 4, srcdata += 4) {
		  unsigned int color = *((unsigned int*) srcdata), comp;
      for(int j = 0; j < 4; j++) {  // RGBA
        comp = (newcolormasks[j]) ? ((((color & newcolormasks[j]) >> rightshift[j]) * 255) / (newcolormasks[j] >> rightshift[j])) : 0;
        if(comp > 255) comp = 255;
  			posdata[j] = (unsigned char) comp;
  		}
    }
  }

  // Now we have new color masks.
  colormasks[0] = 0xff;     // RED
  colormasks[1] = 0xff00;   // GREEN
  colormasks[2] = 0xff0000; // Eight least significant bits = BLUE
  colormasks[3] = (nbits[3] != 0) ? 0xff000000 : 0x00;     // ALPHA
  rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 24;
  nbits[0] = 8; nbits[1] = 8; nbits[2] = 8; nbits[3] = (nbits[3] == 0) ? 0 : 8;
  max[0] = 255; max[1] = 255; max[2] = 255; max[3] = (nbits[3] == 0) ? 0 : 255;

  // Ok, it's loaded.
	return true;
}


bool gcgIMAGE::convertPalettedto8Bits(gcgIMAGE *src) {
  if(src->bpp != 1 && src->bpp != 2 && src->bpp != 4) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convertPalettedto8Bits(): unsupported conversion from %d bpp in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  unsigned char *indices = src->data; // We have indices to the palette
  unsigned char *pal = src->palette;  // We have RGBA colors
  unsigned int newrowsize = BITMAPWIDTHBYTES(src->width, 8);
  int  pixelsperbyte = (8 / src->bpp); // Compute the number of colors per data byte.

  if(this == src) {
    // Allocate space for the new image data
    data = new unsigned char[newrowsize * src->height];
    if(data == NULL)	{
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convertPalettedto8Bits(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

    // Allocate space for the new palette
    palette = new unsigned char[256 * sizeof(RGBQUAD)];
    if(palette == NULL)	{
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convertPalettedto8Bits(): palette allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }
  } else
      if(!this->createImage(src->width, src->height, 8, false)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convertPalettedto8Bits(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

  unsigned int bitmask = (0x01 << src->bpp) - 1; // Computes de index mask

 	// Move the indices.
	for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
    unsigned char *dstdata = &data[currentRow * newrowsize];
 	  unsigned char *srcdata = &indices[currentRow * rowsize];
		for(unsigned int i = 0; i < width; srcdata++)
      for(int j = pixelsperbyte-1; j >= 0; j--, i++, dstdata++)
  			*dstdata = (((*srcdata) >> (j * src->bpp)) & bitmask);
  }

  // Move the colors
  unsigned char *p = pal;
  unsigned char *q = palette;
  for(unsigned int i = 0; i < 256; i++, q += sizeof(RGBQUAD))
    if(i < palettecolors) {
      q[0] = p[0];  q[1] = p[1];  q[2] = p[2]; q[3] = p[3];
      p += sizeof(RGBQUAD);
    } else { q[0] = q[1] = q[2] = (unsigned char) i; q[3] = 0; }

  if(this == src) {
    // Converted successfully
    bpp = 8;
    rowsize = newrowsize;
    palettecolors = 256;
    SAFE_DELETE_ARRAY(indices);
    SAFE_DELETE_ARRAY(pal);

    // Paletted does not use masks
    memset(colormasks, 0, sizeof(colormasks));
    memset(rightshift, 0, sizeof(rightshift));
    memset(nbits, 0, sizeof(nbits));
    memset(max, 0, sizeof(max));
  }

	// Ok, it's done
  return true;
}


bool gcgIMAGE::convertPalettedto24Bits(gcgIMAGE *src) {
  if(src->bpp != 1 && src->bpp != 2 && src->bpp != 4 && src->bpp != 8) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convertPalettedto24Bits(): unsupported conversion from %d bpp in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  unsigned char *indices = src->data; // We have indices to the palette
  unsigned int  newrowsize = BITMAPWIDTHBYTES(src->width, 24);
  int  pixelsperbyte = (8 / src->bpp); // Compute the number of colors per data byte.

  if(this == src) {
    // Allocate space for the new image data
    data = new unsigned char[newrowsize * src->height];
    if(data == NULL)	{
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convertPalettedto24Bits(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }
  } else
      if(!this->createImage(src->width, src->height, 24, false)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convertPalettedto24Bits(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

  unsigned int bitmask = (0x01 << bpp) - 1; // Computes de index mask

 	// Move the colors.
	for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
    unsigned char *dstdata = &data[currentRow * newrowsize];
 	  unsigned char *srcdata = &indices[currentRow * rowsize];
		for(unsigned int i = 0; i < width; srcdata++)
      for(int j = pixelsperbyte-1; j >= 0; j--, i++, dstdata += 3) {
        unsigned char *color = &palette[(((*srcdata) >> (j * bpp)) & bitmask) * sizeof(RGBQUAD)];
  			dstdata[0] = color[0];
  			dstdata[1] = color[1];
  			dstdata[2] = color[2];
  		}
  }

  if(this == src) {
    // Converted successfully
    bpp = 24;
    rowsize = newrowsize;
    SAFE_DELETE_ARRAY(indices);
    SAFE_DELETE_ARRAY(palette);
    palettecolors = 0;

    // Internal mask representation
    colormasks[0] = 0xff;     // RED
    colormasks[1] = 0xff00;   // GREEN
    colormasks[2] = 0xff0000; // BLUE
    colormasks[3] = 0x00;     // ALPHA
    rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 0;
    nbits[0] = 8; nbits[1] = 8; nbits[2] = 8; nbits[3] = 0;
    max[0] = 255; max[1] = 255; max[2] = 255; max[3] = 0;
  }

	// Ok, it's done
  return true;
}

bool gcgIMAGE::convert16Bitsto24Bits(gcgIMAGE *src) {
  if(src->bpp != 16) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convert16Bitsto24Bits(): unsupported conversion from %d bpp in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  unsigned char *colors = src->data;
  unsigned int newrowsize = BITMAPWIDTHBYTES(src->width, 24);

  if(this == src) {
    // Allocate space for the new image data
    data = new unsigned char[newrowsize * src->height];
    if(data == NULL)	{
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convert16Bitsto24Bits(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }
  } else
      if(!this->createImage(src->width, src->height, 24, false)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convert16Bitsto24Bits(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

 	// Move colors
	for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
 	  unsigned short *srccolor = (unsigned short*) &colors[currentRow * rowsize];
 	  unsigned char  *dstdata  = &data[currentRow * newrowsize];
		for(unsigned int i = 0; i < width; i++, srccolor++, dstdata += 3) {
		  unsigned int color = (unsigned int) (*srccolor), comp;
      for(int j = 0; j < 3; j++) {  // Only RGB
        comp = (((color & colormasks[j]) >> rightshift[j]) * 255) / (colormasks[j] >> rightshift[j]);
        if(comp > 255) comp = 255;
  			dstdata[j] = (unsigned char) comp;
  		}
    }
  }

  if(this == src) {
    // Converted successfully
    bpp = 24;
    rowsize = newrowsize;
    SAFE_DELETE_ARRAY(colors);
    SAFE_DELETE_ARRAY(palette);
    palettecolors = 0;

    // Internal mask representation
    colormasks[0] = 0xff;     // RED
    colormasks[1] = 0xff00;   // GREEN
    colormasks[2] = 0xff0000; // BLUE
    colormasks[3] = 0x00;     // ALPHA
    rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 0;
    nbits[0] = 8; nbits[1] = 8; nbits[2] = 8; nbits[3] = 0;
    max[0] = 255; max[1] = 255; max[2] = 255; max[3] = 0;
  }

	// Ok, it's done
  return true;
}

bool gcgIMAGE::convert32Bitsto24Bits(gcgIMAGE *src) {
  if(src->bpp != 32) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convert32Bitsto24Bits(): unsupported conversion from %d bpp in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  unsigned char *colors = src->data; // We have 32bits colors
  unsigned int newrowsize = BITMAPWIDTHBYTES(src->width, 24);

  if(this == src) {
    // Allocate space for the new image data
    data = new unsigned char[newrowsize * src->height];
    if(data == NULL)	{
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convert32Bitsto24Bits(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }
  } else
      if(!this->createImage(src->width, src->height, 24, false)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convert32Bitsto24Bits(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

 	// Move colors
	for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
 	  unsigned char *srccolor = (unsigned char*) &colors[currentRow * rowsize];
 	  unsigned char *dstdata  = &data[currentRow * newrowsize];
		for(unsigned int i = 0; i < width; i++, srccolor += 4, dstdata += 3) {
      dstdata[0] = srccolor[0];
      dstdata[1] = srccolor[1];
      dstdata[2] = srccolor[2];
    }
  }

  if(this == src) {
    // Converted successfully
    bpp = 24;
    rowsize = newrowsize;
    SAFE_DELETE_ARRAY(colors);
    SAFE_DELETE_ARRAY(palette);
    palettecolors = 0;

    // Internal representation
    colormasks[0] = 0xff;     // RED
    colormasks[1] = 0xff00;   // GREEN
    colormasks[2] = 0xff0000; // BLUE
    colormasks[3] = 0x00;     // ALPHA
    rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 0;
    nbits[0] = 8;  nbits[1] = 8;  nbits[2] = 8; nbits[3] = 0;
    max[0] = 255; max[1] = 255; max[2] = 255; max[3] = 0;
  }

  // Ok, it's done
  return true;
}

bool gcgIMAGE::convert16Bitsto32Bits(gcgIMAGE *src) {
  if(src->bpp != 16) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "convert16Bitsto32Bits(): unsupported conversion from %d bpp in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  unsigned char *colors = src->data; // We have 16bits colors
  unsigned int  newrowsize = BITMAPWIDTHBYTES(src->width, 32);

  if(this == src) {
    // Allocate space for the new image data
    data = new unsigned char[newrowsize * src->height];
    if(data == NULL)	{
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "convert16Bitsto32Bits(): data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }
  } else
      if(!this->createImage(src->width, src->height, 32, false)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "convert16Bitsto32Bits(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }

 	// Move colors
	for(unsigned int currentRow = 0; currentRow < height; currentRow++) {
 	  unsigned short *srccolor = (unsigned short*) &colors[currentRow * rowsize];
 	  unsigned char  *dstdata  = &data[currentRow * newrowsize];
		for(unsigned int i = 0; i < width; i++, srccolor++, dstdata += 4) {
		  unsigned int color = (unsigned int) (*srccolor), comp;
      for(int j = 0; j < 4; j++) {  // RGBA
        comp = (colormasks[j]) ? ((((color & colormasks[j]) >> rightshift[j]) * 255) / (colormasks[j] >> rightshift[j])) : 0;
        if(comp > 255) comp = 255;
  			dstdata[j] = (unsigned char) comp;
  		}
    }
  }

  if(this == src) {
    // Converted successfully
    bpp = 32;
    rowsize = newrowsize;
    SAFE_DELETE_ARRAY(colors);
    SAFE_DELETE_ARRAY(palette);
    palettecolors = 0;

    // Internal mask representation
    colormasks[0] = 0xff;       // RED
    colormasks[1] = 0xff00;     // GREEN
    colormasks[2] = 0xff0000;   // BLUE
    colormasks[3] = 0xff000000; // ALPHA
    rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 24;
    nbits[0] = 8;  nbits[1] = 8;  nbits[2] = 8; nbits[3] = 8;
    max[0] = 255; max[1] = 255; max[2] = 255; max[3] = 255;
  }

	// Ok, it's done
  return true;
}


////////////////////////////////////////////////////////////
// Compress a 8 bit sequence as a Run-Lenght Encoded image.
// Returns the byte size of compressed block or 0 if the
// compressed version is greather than original data.
////////////////////////////////////////////////////////////
unsigned int gcgCompressImageRLE8(unsigned int width, unsigned int height, unsigned char *srcdata, unsigned char *RLEdata) {
  unsigned int line, srcindex = 0, dstindex = 0, counter, i;
  unsigned int rowsize = BITMAPWIDTHBYTES(width, 8);
  unsigned int fullsize = width * height;
  unsigned char *base = srcdata;

  // Check parameters
  if(width == 0 || height == 0 || srcdata == NULL || RLEdata == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgCompressImageRLE8(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  // Scans every image row
  for(line = 0;  line < height; line++, srcindex = 0, base = &srcdata[line * rowsize]) {
state_start:
    if(srcindex + 1 >= width) { // Is this the last pixel of the line?
       if(dstindex + 2 >= fullsize) return 0; // It does not give a good compression.
       RLEdata[dstindex++] = 1; // Start a block of length 1
       RLEdata[dstindex++] = base[srcindex++];
       goto end_of_line;
    }

    // If next pixel is the same, enter compress mode
    if(base[srcindex] == base[srcindex + 1]) goto state_compress;

    // We have two distinct pixels
    if(srcindex + 2 >= width) { // Are these the last 2 pixels of the line?
      if(dstindex + 4 >= fullsize) {
        gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgCompressImageRLE8(): Uncompressed image is smaller than the RLE compressed version. (%s:%d)", basename((char*)__FILE__), __LINE__);
        return 0; // It does not give a good compression.
      }
      RLEdata[dstindex++] = 1; // block of length 1
      RLEdata[dstindex++] = base[srcindex++];
      RLEdata[dstindex++] = 1; // block of length 1
      RLEdata[dstindex++] = base[srcindex++];
      goto end_of_line;
    }

    // Check if in next 3 pixels, the last 2 consecutive pixels are the same
    if(base[srcindex + 1] == base[srcindex + 2]) {
       if(dstindex + 2 >= fullsize) {
         gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgCompressImageRLE8(): Uncompressed image is smaller than the RLE compressed version. (%s:%d)", basename((char*)__FILE__), __LINE__);
         return 0; // It does not give a good compression.
       }
       // Save first pixel
       RLEdata[dstindex++] = 1; // block of length 1
       RLEdata[dstindex++] = base[srcindex++];

       // Now, the last two pixels can be compressed
       goto state_compress;
    } else goto state_no_compress; // For the last 3 pixels, no 2 consecutive pixels are the same.

state_compress:  // Compress mode
    // srcindex is the first pixel of a compressed block
    // counter is the number of additional pixels following current pixel
    // (srcindex+counter) point to the last pixel of current block
    for(counter = 1; counter <= 254; counter++)  {
      // We must check this condition first.
      if(base[srcindex + counter] != base[srcindex]) break;

      if(srcindex + counter + 1 >= width) { // Did it reach the end of line?
        if(dstindex + 2 >= fullsize) {
          gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgCompressImageRLE8(): Uncompressed image is smaller than the RLE compressed version. (%s:%d)", basename((char*)__FILE__), __LINE__);
          return 0; // It does not give a good compression.
        }
        RLEdata[dstindex++] = (unsigned char) (counter + 1);
        RLEdata[dstindex++] = base[srcindex];
        srcindex = srcindex + counter + 1;

        goto end_of_line;
      }
    }

    // now base[srcindex + counter] is out of the block
    if(dstindex + 2 >= fullsize) return 0; // It does not give a good compression.
    RLEdata[dstindex++] = (unsigned char) counter; // block length is counter
    RLEdata[dstindex++] = base[srcindex];
    srcindex += counter;
    goto state_start;

state_no_compress:
    // srcindex points to the first pixel of an uncompressed block that needs at least 3 pixels.
    // counter is the number of additional pixels following current pixel.
    for(counter = 2; counter <= 254; counter++) {
      if(srcindex + counter + 2 >= width) { // Did it reach the end of line?
        if(dstindex + counter + 3 >= fullsize) {
          gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgCompressImageRLE8(): Uncompressed image is smaller than the RLE compressed version. (%s:%d)", basename((char*)__FILE__), __LINE__);
          return 0; // It does not give a good compression.
        }
        RLEdata[dstindex++] = 0; // Escape character for an uncompressed block
        RLEdata[dstindex++] = (unsigned char) (counter + 1); // block length is (counter + 1)
        for(i = 0; i <= counter; i++) RLEdata[dstindex++] = base[srcindex++];
        // If block length is odd, padd with a zero byte
        if(counter % 2 == 0) RLEdata[dstindex++] = 0;
        goto end_of_line;
      }

      if(srcindex + counter + 2 >= width || base[srcindex + counter] != base[srcindex + counter + 1])
        continue; // Still no 2 consecutive pixel are the same, thus extend the block.

      // Here we found the last two pixels are the same.
      if(srcindex + counter + 3 >= width || base[srcindex + counter + 1] != base[srcindex + counter + 2])
        continue; // But the third pixel is not the same, thus go ahead in uncompressed mode

      // Here, we found the last three pixels are the same. We need to exit the uncompressed mode
      if(counter > 2) counter--; // we can shrink the block one pixel (go backward)

      if(dstindex + counter + 3 >= fullsize) {
        gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgCompressImageRLE8(): Uncompressed image is smaller than the RLE compressed version. (%s:%d)", basename((char*)__FILE__), __LINE__);
        return 0; // It does not give a good compression.
      }
      RLEdata[dstindex++] = 0; // Escape character for uncompressed block
      RLEdata[dstindex++] = (unsigned char) (counter + 1); // block length is (counter+1)
      for(i = 0; i <= counter; i++) RLEdata[dstindex++] = base[srcindex++];
      // If block length is odd, padd with a zero byte
      if(counter % 2 == 0) RLEdata[dstindex++] = 0;
      goto state_compress;
    } // End of for (counter = 2; counter <= 254; counter++)

    // Now we have a full block of 255 pixels
    if(dstindex + counter + 3 >= fullsize) {
      gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgCompressImageRLE8(): Uncompressed image is smaller than the RLE compressed version. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return 0; // It does not give a good compression.
    }
    RLEdata[dstindex++] = 0; // escape character for uncompressed block
    RLEdata[dstindex++] = (unsigned char) counter; // block length is (counter)
    for(i = 0; i < counter; i++) RLEdata[dstindex++] = base[srcindex++];
    // If block length is odd, padd with a zero byte
    if(counter % 2 != 0) RLEdata[dstindex++] = 0;
    goto state_start;

end_of_line:
    // Finish this line
    if(dstindex + 2 >= fullsize) {
      gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgCompressImageRLE8(): Uncompressed image is smaller than the RLE compressed version. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return 0; // It does not give a good compression.
    }
    RLEdata[dstindex++] = 0; // Mark end-of-line
    RLEdata[dstindex++] = 0;
  }

  if(dstindex + 2 >= fullsize) {
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgCompressImageRLE8(): Uncompressed image is smaller than the RLE compressed version. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0; // It does not give a good compression.
  }
  RLEdata[dstindex++] = 0; // Mark end-of-bitmap
  RLEdata[dstindex++] = 1;

  // Compressed RLE image size
  return dstindex;
}

////////////////////////////////////////////////////////////
// Decompress a 4 bit Run-Length Encoded sequence.
////////////////////////////////////////////////////////////
unsigned int gcgDecompressImageRLE4(unsigned int width, unsigned int height, unsigned char *RLEdata, unsigned char *dstdata) {
  // Decoder for RLE4 compression in windows bitmaps
  unsigned int srcindex = 0;
  unsigned int rowsize = BITMAPWIDTHBYTES(width, 4);

  // Check parameters
  if(width == 0 || height == 0 || RLEdata == NULL || dstdata == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgDecompressImageRLE4(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  memset(dstdata, 0, rowsize * height);

  for(unsigned int col = 0, lin = 0; lin < height;) {
    unsigned int c = (unsigned char) RLEdata[srcindex++]; // Number of bytes or 00 code

    if(c > 0) {
      unsigned int ntimes = c;  // Number of pixels
      c = RLEdata[srcindex++];
      unsigned char pix[2] = { (unsigned char) (c & 0x0f), (unsigned char) ((c >> 4) & 0x0f)};
      for(unsigned int j = 0; j < ntimes; j++) {
        if(col % 2 == 0) dstdata[lin * rowsize + col / 2] = (dstdata[lin * rowsize + col / 2] & 0xf0) | pix[j % 2];
        else dstdata[lin * rowsize + col / 2] = (dstdata[lin * rowsize + col / 2] & 0x0f) | (pix[j % 2] << 4);

        if(col < width) col++;
        //else { col = 0; lin++; }
      }
    } else {
        c = RLEdata[srcindex++]; // Get code
        switch(c) {
          case  0: col = 0;      lin++;        break; // End of line
          case  1: col = width;  lin = height; break; // End of bitmap
          case  2: col += RLEdata[srcindex++]; lin += RLEdata[srcindex++];  break; // Delta
          default:// Uncompressed indexes
                  int ntimes = c;
                  unsigned char pix[2];
                  for(int j = 0; j < ntimes; j++) {
                    if(j % 2 == 0) {
                      c = RLEdata[srcindex++]; // Gets two new nibbles
                      pix[0] = c & 0x0f;
                      pix[1] = ((c >> 4) & 0x0f);
                    }

                    if(col % 2 == 0) dstdata[lin * rowsize + col / 2] = (dstdata[lin * rowsize + col / 2] & 0xf0) | pix[j % 2];
                    else dstdata[lin * rowsize + col / 2] = (dstdata[lin * rowsize + col / 2] & 0x0f) | (pix[j % 2] << 4);

                    if(col < width) col++;
                    //else { col = 0; lin++; }
                  }

                  if((ntimes / 2) % 2 == 1) RLEdata[srcindex++] = 0;// Align byte. Should be 0.
                  break;
        }
    }
  }

  // Return number of decoded bytes
  return rowsize * height;
}


////////////////////////////////////////////////////////////
// Decompress a 8 bit Run-Length Encoded sequence.
////////////////////////////////////////////////////////////
unsigned int gcgDecompressImageRLE8(unsigned int width, unsigned int height, unsigned char *RLEdata, unsigned char *dstdata) {
  // Decoder for RLE8 compression in windows bitmaps
  unsigned int srcindex = 0;
  unsigned int rowsize = BITMAPWIDTHBYTES(width, 8);

  if(width == 0 || height == 0 || RLEdata == NULL || dstdata == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgDecompressImageRLE8(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  memset(dstdata, 0, rowsize * height);

  for(unsigned int col = 0, lin = 0; lin < height;) {
    int c = (unsigned char) RLEdata[srcindex++]; // Number of bytes or 00 code

    if(c > 0) {
      int ntimes = (int) c;  // Number of bytes
      c = RLEdata[srcindex++];
      for(int j = 0; j < ntimes; j++) {
        dstdata[lin * rowsize + col] = (unsigned char) c;
        if(col < width) col++;
        //else { col = 0; lin++; }
      }
    } else {
        c = RLEdata[srcindex++]; // Get code
        switch(c) {
          case  0: col = 0;      lin++;        break; // End of line
          case  1: col = width;  lin = height; break; // End of bitmap
          case  2: col += (unsigned char) RLEdata[srcindex++]; lin += (unsigned char) RLEdata[srcindex++];  break; // Delta
          default:// Uncompressed indexes
                  int ntimes = (int) c;
                  for(int j = 0; j < ntimes; j++) {
                    dstdata[lin * rowsize + col] = (unsigned char) RLEdata[srcindex++];
                    if(col < width) col++;
                    //else { col = 0; lin++; }
                  }
                  if(ntimes % 2 == 1) srcindex++;// Align byte. Should be 0.
                  break;
        }
    }
  }

  // Return number of decoded bytes
  return rowsize * height;
}


bool gcgSaveBMPcode(char *inputname, char *outputname) {
	FILE *file;
	BITMAPFILEHEADER fileHeader;

  // Check parameters
  if(inputname == NULL || outputname == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgSaveBMPcode(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

	file = fopen(inputname, "rb+");
	if(file == NULL) {
	  gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_FILE, GCG_OPENERROR), "gcgSaveBMPcode(): Error opening file %s. (%s:%d)", inputname, basename((char*)__FILE__), __LINE__);
	  return false;
	}

	// Read the bitmap header
	if(fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, file) != 1) {
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_FILE, GCG_OPENERROR), "gcgSaveBMPcode(): Error loading file %s. (%s:%d)", inputname, basename((char*)__FILE__), __LINE__);
	  fclose(file);
	  return false;
	}

	// Check it's a bitmap
	if(fileHeader.bfType != SIGNATURE_BMP) {
	  gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_FILE, GCG_FORMATMISMATCH), "gcgSaveBMPcode(): file %s is not a windows bitmap image. (%s:%d)", inputname, basename((char*)__FILE__), __LINE__);
		fclose(file);
		return false;
	}

  // Discover file size
  fseek(file, 0, SEEK_END);
  unsigned int filesize = ftell(file);

  // Allocate temporary memory
  unsigned char *filedata = new unsigned char[filesize - sizeof(BITMAPFILEHEADER)];
  if(filedata == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgSaveBMPcode(): allocation of input buffer failed. (%s:%d)", basename((char*)__FILE__), __LINE__);
		fclose(file);
		return false;
	}

  // Point file to the beginning of the data and loads it
	fseek(file, sizeof(BITMAPFILEHEADER), SEEK_SET);
  if(fread(filedata, filesize - sizeof(BITMAPFILEHEADER), 1, file) != 1) {
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_FILE, GCG_OPENERROR), "gcgSaveBMPcode(): Error loading file %s. (%s:%d)", inputname, basename((char*)__FILE__), __LINE__);
		fclose(file);
		return false;
  }

  // Close the file
	fclose(file);

  void *bmpHeader = filedata;
  unsigned char *bmpdata = ((unsigned char*) filedata) + (WORD) (fileHeader.bfOffBits - sizeof(BITMAPFILEHEADER));
  unsigned int npalcolors = (fileHeader.bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER)) / sizeof(RGBQUAD);
  unsigned char *paldata = ((unsigned char*) filedata) + sizeof(BITMAPINFOHEADER);

  	// Create file
  FILE *outputarq = fopen(outputname, "wt+");
  if(!outputarq) {
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_FILE, GCG_OPENERROR), "gcgSaveBMPcode(): Error creating file %s. (%s:%d)", outputname, basename((char*)__FILE__), __LINE__);

    // Free temporary buffer
    SAFE_DELETE_ARRAY(filedata);
    return false;
  }

  fprintf(outputarq, "\nstatic struct {unsigned char bmpHeader[%d]; unsigned char bmpdata[%d]; unsigned int npalcolors; unsigned char paldata[%d];}", (int) sizeof(BITMAPINFOHEADER), (int) ((BITMAPINFOHEADER*) bmpHeader)->biSizeImage, (int) (sizeof(RGBQUAD) * npalcolors));
  fprintf(outputarq, "\n  %s = { ", outputname);

  fprintf(outputarq, "\n   {");
  for(unsigned int i = 0; i < sizeof(BITMAPINFOHEADER); i++) {
    fprintf(outputarq, "%d", ((unsigned char*) bmpHeader)[i]);
    if(++i < sizeof(BITMAPINFOHEADER)) fprintf(outputarq, ",");
  }
  fprintf(outputarq, "},");

  unsigned int imagesize = ((BITMAPINFOHEADER*) bmpHeader)->biSizeImage;
  fprintf(outputarq, "\n   {");
  for(unsigned int i = 0; i < imagesize;) {
    if(i > 0 && i % 256 == 0) fprintf(outputarq, "\n    ");
    fprintf(outputarq, "%d", bmpdata[i]);
    if(++i < imagesize) fprintf(outputarq, ",");
  }
  fprintf(outputarq, "}, %d,", npalcolors);

  imagesize = sizeof(RGBQUAD) * npalcolors;
  fprintf(outputarq, "\n   {");
  for(unsigned int i = 0; i < imagesize;) {
    if(i > 0 && i % 256 == 0) fprintf(outputarq, "\n    ");
    fprintf(outputarq, "%d", paldata[i]);
    if(++i < imagesize) fprintf(outputarq, ",");
  }
  fprintf(outputarq, "}\n };");

  // Finished
  fclose(outputarq);
	return true;
}






bool gcgIMAGE::equalizeHistogram(gcgIMAGE *src) {
  // Source image is valid?
  if(src->data == NULL || src->width == 0 || src->height == 0 || src->bpp == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_BADPARAMETERS), "equalizeHistogram(): source image is not valid. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // is paletted?
  if(src->palettecolors > 0 && src->palette != NULL) {
    int pixelsperbyte = (8 / src->bpp); // Compute the number of colors per data byte.
    unsigned int bitmask = (0x01 << src->bpp) - 1;

    if(src->isGrayScale()) {
      // Check destination image: destination must be compatible with the source
      unsigned char *dstdata = NULL;
      if(this != src) {
         // createSimilar creates only what is needed to make them compatible.
        if(!this->createSimilar(src)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "equalizeHistogram(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
      } else {
        // We need an output buffer
        dstdata = new unsigned char[rowsize * height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "equalizeHistogram(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      // Special case: we can make the computation with the indices
      src->forceLinearPalette(); // Needed to assure the bitmap can be equalized directly

      // Compute histograms
      unsigned int gray[256];
      unsigned int nbins = src->histogramIndex(256, gray);

      // Build a LUT containing scale factor
      float factor = (float) nbins / ((float) src->height * (float) src->width);
      unsigned int sum = 0;
      unsigned char lut[256];
      for(unsigned int i = 0; i < nbins; i++) {
        sum += gray[i];
        lut[i] = (unsigned char) ((float) sum * factor);
      }

    	// Equalization with colors with less than 9 bits in data buffer
    	for(unsigned int row = 0; row < this->height; row++) {
        register unsigned char *dstrow  = &this->data[row * rowsize];
        register unsigned char *srcrow1 = &src->data[row * rowsize];
    		for(unsigned int col = 0; col < width; col++) {
    		  unsigned char val = lut[((srcrow1[col / pixelsperbyte] >> ((col % pixelsperbyte) * this->bpp)) & bitmask)];
          unsigned char *ind = &dstrow[col / pixelsperbyte];
          int rshift = (col % pixelsperbyte) * bpp;
          *ind = (*ind ^ (*ind & (bitmask << rshift))) | ((val & bitmask) << rshift);
    		}
      }

      // Commit equalized data
      if(this == src) {
        // We have new data
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;
      } else memcpy(this->palette, src->palette, sizeof(RGBQUAD) * palettecolors);
    } else {
      unsigned char *dstdata = NULL;
      unsigned int newrowsize = 0;
      // We can't equalize using a RGBA palette: destiny must have at least 32 bits/pixel
      if(this != src) {
        if(!this->createImage(src->width, src->height, 32, true)) {
          gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "equalizeHistogram(): a call to createImage() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
        dstdata = this->data;
        newrowsize = this->rowsize;
      } else {
        newrowsize = BITMAPWIDTHBYTES(src->width, 32);
        dstdata = new unsigned char[newrowsize * src->height];
        if(dstdata == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "equalizeHistogram(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
          return false;
        }
      }

      // Compute histograms
      unsigned int red[256], green[256], blue[256], alpha[256];
      src->histogramRGBA(red, green, blue, alpha);

      // Build a LUT containing scale factor
      float factor = 256.0f / ((float) src->height * (float) src->width);
      unsigned int sumR = 0, sumG = 0, sumB = 0, sumA = 0;
      unsigned char lutR[256], lutG[256], lutB[256], lutA[256];
      for(int i = 0; i < 256; i++) {
        sumR += red[i]; sumG += green[i]; sumB += blue[i]; sumA += alpha[i];
        lutR[i] = (unsigned char) ((float) sumR * factor);
        lutG[i] = (unsigned char) ((float) sumG * factor);
        lutB[i] = (unsigned char) ((float) sumB * factor);
        lutA[i] = (unsigned char) ((float) sumA * factor);
      }


    	// Equalization with 32 bits colors from palette using LUTs
      for(unsigned int row = 0; row < src->height; row++) {
        unsigned char *dstrow = &dstdata[row * newrowsize];
        register unsigned char *srcrow1 = &src->data[row * src->rowsize];
    		for(unsigned int col = 0; col < src->width; col++, dstrow += 4) {
          unsigned char *color1 = &src->palette[sizeof(RGBQUAD) * ((srcrow1[col / pixelsperbyte] >> ((col % pixelsperbyte) * src->bpp)) & bitmask)];
          dstrow[0] = lutR[color1[0]];
          dstrow[1] = lutG[color1[1]];
          dstrow[2] = lutB[color1[2]];
          dstrow[3] = lutA[color1[3]];
    		}
      }

      // Now commit the changes if destiny == source
      if(this == src) {
        bpp     = 32;
        rowsize = newrowsize;
        SAFE_DELETE_ARRAY(this->palette);
        palettecolors = 0;
        SAFE_DELETE_ARRAY(this->data);
        this->data = dstdata;

        // Internal representation
        colormasks[0] = 0xff;       // RED
        colormasks[1] = 0xff00;     // GREEN
        colormasks[2] = 0xff0000;   // BLUE
        colormasks[3] = 0xff000000; // ALPHA
        rightshift[0] = 0;  rightshift[1] = 8;  rightshift[2] = 16; rightshift[3] = 24;
        nbits[0] = 8;  nbits[1] = 8;  nbits[2] = 8; nbits[3] = 8;
        max[0] = 255;  max[1] = 255;  max[2] = 255;  max[3] = 255;
      }
    }
  } else {
    // Check destination image: destination must be compatible with the source
    unsigned char *dstdata = NULL;
    if(this != src) {
       // createSimilar creates only what is needed to make them compatible.
      if(!this->createSimilar(src)) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_IMAGE, GCG_FUNCTIONCALLFAILED), "equalizeHistogram(): a call to createSimilar() has failed. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
      dstdata = this->data;
    } else {
      // We need an output buffer
      dstdata = new unsigned char[rowsize * height];
      if(dstdata == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "equalizeHistogram(): buffer allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
        return false;
      }
    }

    if(src->bpp == 24) {
      // Compute histograms
      unsigned int red[256], green[256], blue[256];
      src->histogramRGBA(red, green, blue, NULL);

      // Build a LUT containing scale factor
      float factor = 256.0f / ((float) src->height * (float) src->width);
      unsigned int sumR = 0, sumG = 0, sumB = 0;
      unsigned char lutR[256], lutG[256], lutB[256];
      for(int i = 0; i < 256; i++) {
        sumR += red[i]; sumG += green[i]; sumB += blue[i];
        lutR[i] = (unsigned char) ((float) sumR * factor);
        lutG[i] = (unsigned char) ((float) sumG * factor);
        lutB[i] = (unsigned char) ((float) sumB * factor);
      }

      // Equalization with 24 bits colors in data buffer using LUTs
      for(unsigned int row = 0; row < height; row++) {
        register unsigned char *dstrow  = &this->data[row * rowsize];
        register unsigned char *srcrow1 = &src->data[row * rowsize];
        for(unsigned int col = 0; col < width; col++, dstrow += 3, srcrow1 += 3) {
          dstrow[0] = lutR[srcrow1[0]];
          dstrow[1] = lutG[srcrow1[1]];
          dstrow[2] = lutB[srcrow1[2]];
        }
      }
    } else
      	if(src->bpp == 32) {
          // Compute histograms
          unsigned int red[256], green[256], blue[256], alpha[256];
          src->histogramRGBA(red, green, blue, alpha);

          // Build a LUT containing scale factor
          float factor = 256.0f / ((float) src->height * (float) src->width);
          unsigned int sumR = 0, sumG = 0, sumB = 0, sumA = 0;
          unsigned char lutR[256], lutG[256], lutB[256], lutA[256];
          for(int i = 0; i < 256; i++) {
            sumR += red[i]; sumG += green[i]; sumB += blue[i]; sumA += alpha[i];
            lutR[i] = (unsigned char) ((float) sumR * factor);
            lutG[i] = (unsigned char) ((float) sumG * factor);
            lutB[i] = (unsigned char) ((float) sumB * factor);
            lutA[i] = (unsigned char) ((float) sumA * factor);
          }

          // Equalization with 32 bits colors in data buffer using LUTs
          for(unsigned int row = 0; row < height; row++) {
            register unsigned char *dstrow  = &this->data[row * rowsize];
            register unsigned char *srcrow1 = &src->data[row * rowsize];
            for(unsigned int col = 0; col < width; col++, dstrow += 4, srcrow1 += 4) {
              dstrow[0] = lutR[srcrow1[0]];
              dstrow[1] = lutG[srcrow1[1]];
              dstrow[2] = lutB[srcrow1[2]];
              dstrow[3] = lutA[srcrow1[3]];
            }
          }
        } else
            if(src->bpp == 16) {
              // Compute histograms
              unsigned int red[256], green[256], blue[256], alpha[256];
              src->histogramRGBA(red, green, blue, alpha);

              // Build a LUT containing scale factor
              float factor = 256.0f / ((float) src->height * (float) src->width);
              unsigned int sumR = 0, sumG = 0, sumB = 0, sumA = 0;
              unsigned char lutR[256], lutG[256], lutB[256], lutA[256];
              for(int i = 0; i < 256; i++) {
                sumR += red[i]; sumG += green[i]; sumB += blue[i]; sumA += alpha[i];
                lutR[i] = (unsigned char) ((float) sumR * factor);
                lutG[i] = (unsigned char) ((float) sumG * factor);
                lutB[i] = (unsigned char) ((float) sumB * factor);
                lutA[i] = (unsigned char) ((float) sumA * factor);
              }

              // Equalization with 16 bits colors in data buffer using LUTs
              for(unsigned int row = 0; row < this->height; row++) {
                register unsigned short *dstrow  = (unsigned short*) &this->data[row * rowsize];
                register unsigned short *srcrow1 = (unsigned short*) &src->data[row * rowsize];
                for(unsigned int col = 0; col < width; col++, dstrow++) {
                  int color1 = srcrow1[col];
                  unsigned char R = lutR[(int) ((color1 & src->colormasks[0]) >> src->rightshift[0])];
                  unsigned char G = lutG[(int) ((color1 & src->colormasks[1]) >> src->rightshift[1])];
                  unsigned char B = lutB[(int) ((color1 & src->colormasks[2]) >> src->rightshift[2])];
                  unsigned char A = lutA[(int) ((color1 & src->colormasks[3]) >> src->rightshift[3])];
                  *dstrow = (unsigned short) ((((unsigned char) R) << src->rightshift[0]) |
                                              (((unsigned char) G) << src->rightshift[1]) |
                                              (((unsigned char) B) << src->rightshift[2]) |
                                              (((unsigned char) A) << src->rightshift[3]));
                }
              }
            } else {
                  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_IMAGE, GCG_UNSUPPORTEDOPERATION), "equalizeHistogram(): invalid format bpp = %d in GCGlib %s. Check for newer versions in www.gcg.ufjf.br. (%s:%d)", src->bpp, GCG_VERSION, basename((char*)(char*)__FILE__), __LINE__);
                  return false;
                }

    // Commit filtered data
    if(this == src) {
      // We have new data
      SAFE_DELETE_ARRAY(this->data);
      this->data = dstdata;
    }
  }

  // Ok, it's done
	return true;
}
