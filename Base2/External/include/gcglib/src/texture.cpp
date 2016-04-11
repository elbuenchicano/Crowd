/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    gcgTEXTURE2D: class for user transparent texture handling.

    Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"

// Computes the mininum power of 2 that is also greather than an integer
static int gcgSupremumPowerof2(int i) {
	int log = 0;

	if(i < 0) return -1;

	while(i) {
		i >>= 1;
		log++;
	}
	return log;
}

// Checks the compatibility of an image with the current texture.
static bool checkParameters(gcgTEXTURE2D *tex, gcgIMAGE *image, int *newinternalformat, int *newformat, int *newtype) {
  // Check image
  if(image == NULL) return false;
  if(image->data == NULL || image->height == 0 || image->width == 0 || image->bpp == 0) return false;

  // Choose parameters
  *newinternalformat = GL_RGB8;
  *newformat = GL_RGB;
  *newtype = GL_UNSIGNED_BYTE;
  switch(image->bpp) {
    case 4:  *newinternalformat = GL_LUMINANCE4;
             *newformat = GL_LUMINANCE;
             *newtype = GL_BITMAP;//*newtype = GL_UNSIGNED_BYTE_4;
             break;
    case 8:  *newinternalformat = GL_LUMINANCE8;
             *newformat = GL_LUMINANCE;
             break;
    case 16: if(image->colormasks[2] == 31 &&
                image->colormasks[0] == (31 << 10) &&
                image->colormasks[1] == (31 << 5) &&
                image->colormasks[3] == 0) {
                // RED = 5 bits  GREEN = 5 bits  BLUE = 5 bits ALPHA = 0
                *newinternalformat = GL_RGB5;
                *newformat = 0x80E1; // GL_BGRA
                *newtype = 0x8366; // GL_UNSIGNED_SHORT_1_5_5_5_REV
             } else if(image->colormasks[2] == 31 &&
                       image->colormasks[0] == (31 << 10) &&
                       image->colormasks[1] == (31 << 5)  &&
                       image->colormasks[3] == (1 << 15)) {
                       // RED = 5 bits  GREEN = 5 bits  BLUE = 5 bits ALPHA = 1
                       *newinternalformat = GL_RGB5_A1;
                       *newformat = 0x80E1; // GL_BGRA
                       *newtype = 0x8366; // GL_UNSIGNED_SHORT_1_5_5_5_REV
                   } else if(image->colormasks[2] == 31 &&
                             image->colormasks[0] == (31 << 11) &&
                             image->colormasks[1] == (63 << 5)) {
                            // RED = 5 bits  GREEN = 6 bits  BLUE = 5 bits
                            *newinternalformat = GL_RGB8;
                            *newformat = GL_RGB;
                            *newtype = 0x8363; // GL_UNSIGNED_SHORT_5_6_5
                          } else if(image->colormasks[2] == 15 &&
                                    image->colormasks[0] == (15 << 8) &&
                                    image->colormasks[1] == (15 << 4) &&
                                    image->colormasks[3] == 0) {
                                    *newinternalformat = GL_RGB4;
                                    *newformat = 0x80E1; // GL_BGRA
                                    *newtype = 0x8365; // GL_UNSIGNED_SHORT_4_4_4_4_REV
                                 } else if(image->colormasks[2] == 15 &&
                                          image->colormasks[0] == (15 << 8) &&
                                          image->colormasks[1] == (15 << 4) &&
                                          image->colormasks[3] == (15 << 12)) {
                                          *newinternalformat = GL_RGBA4;
                                          *newformat = 0x80E1; // GL_BGRA
                                          *newtype = 0x8365; // GL_UNSIGNED_SHORT_4_4_4_4_REV
                                       } else {
                                            gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_GRAPHICS, GCG_UNSUPPORTEDFORMAT), "checkParameters(): unsupported 16 bits format for textures. (%s:%d)", basename((char*)__FILE__), __LINE__);
                                            return false; // Unsupported 16 bits format
                                         }
             break;
    case 24: *newinternalformat = GL_RGB8;
             *newformat = GL_RGB;
             break;
    case 32: // gcgIMAGE internal format has 8bits for each channel
             *newformat = GL_RGBA;
             if(image->colormasks[3] == 0) *newinternalformat = GL_RGB8;
             else *newinternalformat = GL_RGBA8;
             break;
    default:
            gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_GRAPHICS, GCG_UNSUPPORTEDFORMAT), "checkParameters(): unsupported %d bits format for textures. (%s:%d)", image->bpp, basename((char*)__FILE__), __LINE__);
            return false; // Unsupported format
  }

  // Checks only OpenGL internal compatibility
  return tex->idOpengl == 0 || (tex->internalformat == *newinternalformat &&
                                tex->actualwidth >= image->width && tex->actualheight >= image->height);
}



gcgTEXTURE2D::gcgTEXTURE2D() {
    idOpengl = 0;
    actualwidth = 0;
    actualheight = 0;
    width = 0;
    height = 0;
    internalformat = 0;
    format = 0;
    type = 0;
}

gcgTEXTURE2D::~gcgTEXTURE2D() {
  destroyTexture();
}

bool gcgTEXTURE2D::destroyTexture() {
    if(idOpengl > 0) {
      glDeleteTextures(1, &idOpengl);
      idOpengl = 0;
    }
    actualwidth = 0;
    actualheight = 0;
    width = 0;
    height = 0;
    internalformat = 0;
    format = 0;
    type = 0;

    return true;
}

bool gcgTEXTURE2D::createTexture(unsigned int _width, unsigned int _height, unsigned char _bpp, bool usealpha, bool mipmapped) {
  // Check parameters
  if(_width == 0 || _height == 0 || _bpp == 0 ||
    (_bpp != 4 && _bpp != 8 && _bpp != 16 && _bpp != 24 && _bpp != 32)) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_BADPARAMETERS), "createTexture(): bad parameters for texture creation. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(!destroyTexture()) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_RELEASEERROR), "createTexture(): could not release old texture. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(ISPOWEROF2(_width) && ISPOWEROF2(_height)) {
    // Ok, if image is power of 2
    actualwidth  = _width;
    actualheight = _height;
  } else {
      // not a power of 2, we must adjust dimensions
      actualwidth  = (ISPOWEROF2(_width )) ? _width : (1 << gcgSupremumPowerof2(_width));
      actualheight = (ISPOWEROF2(_height)) ? _height : (1 << gcgSupremumPowerof2(_height));
    }

  // create a temporary image for uploading
  gcgIMAGE dummy;

  if(!dummy.createImage(actualwidth, actualheight, _bpp, usealpha)) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_CREATIONFAILED), "createTexture(): could not create temporary texture image. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Obtain internal information
  int newinternalformat, newformat, newtype;
  if(!checkParameters(this, &dummy, &newinternalformat, &newformat, &newtype)) {
    dummy.destroyImage();
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_INCOMPATIBILITYERROR), "createTexture(): parameters are incompatible for texture generation. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Create texture
  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &idOpengl);
  if(glGetError() != GL_NO_ERROR) {
    dummy.destroyImage();
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_CREATIONFAILED), "createTexture(): could not generate texture with glGenTextures(). (%s:%d)", basename((char*)__FILE__), __LINE__);
    idOpengl = 0;
    return false;
  }

  glBindTexture(GL_TEXTURE_2D, idOpengl);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Upload texture
  if(mipmapped) gluBuild2DMipmaps(GL_TEXTURE_2D, newinternalformat, actualwidth, actualheight,	newformat, newtype, dummy.data);
  else glTexImage2D(GL_TEXTURE_2D, 0, newinternalformat, actualwidth, actualheight,	0, newformat, newtype, dummy.data);

  // Texture created, set internal information
  width = dummy.width;
  height = dummy.height;
  internalformat = newinternalformat;
  format = newformat;
  type = newtype;

  // Free temporary image
  dummy.destroyImage();

  // Check texture initialization
  if(glGetError() != GL_NO_ERROR) {
    glDisable(GL_TEXTURE_2D);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_INITERROR), "createTexture(): could not init texture. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Ok, it's created
  glDisable(GL_TEXTURE_2D);
  return true;
}


bool gcgTEXTURE2D::uploadImage(gcgIMAGE *image, int posX, int posY, unsigned int level, bool mipmapped) {
  // Check image
  int newinternalformat, newformat, newtype;

  // Check OpenGL context
#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  if(wglGetCurrentContext() == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_GRAPHICS, GCG_INITERROR), "uploadImage(): no OpenGL context. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  if(glXGetCurrentContext() == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_GRAPHICS, GCG_INITERROR), "uploadImage(): no OpenGL context. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }
#else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

  // Check parameters
  if(!checkParameters(this, image, &newinternalformat, &newformat, &newtype)) {
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_GRAPHICS, GCG_INCOMPATIBILITYERROR), "uploadImage(): image is incompatible with current texture. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  glEnable(GL_TEXTURE_2D);
  // Check if texture was not created
  if(idOpengl == 0) {
    // Create texture ONCE
    glGenTextures(1, &idOpengl);
    if(glGetError() != GL_NO_ERROR) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_CREATIONFAILED), "uploadImage(): could not generate texture with glGenTextures(). (%s:%d)", basename((char*)__FILE__), __LINE__);
      idOpengl = 0;
      return false;
    }

    // Binds texture
    glBindTexture(GL_TEXTURE_2D, idOpengl);
    if(mipmapped) {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Uploads texture
    if(ISPOWEROF2(image->width) && ISPOWEROF2(image->height)) {
      // Ok, if image is power of 2
      if(mipmapped) gluBuild2DMipmaps(GL_TEXTURE_2D, newinternalformat, image->width, image->height, newformat, newtype, image->data);
      else glTexImage2D(GL_TEXTURE_2D, level, newinternalformat, image->width, image->height, 0, newformat, newtype, image->data);
      actualwidth  = image->width;
      actualheight = image->height;
    } else {
        // not a power of 2, we must adjust dimensions
        actualwidth  = (ISPOWEROF2(image->width )) ? image->width : (1 << gcgSupremumPowerof2(image->width));
        actualheight = (ISPOWEROF2(image->height)) ? image->height : (1 << gcgSupremumPowerof2(image->height));
        gcgIMAGE dummy;
        if(!dummy.createImage(actualwidth, actualheight, image->bpp, image->colormasks[3] == 0)) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_CREATIONFAILED), "uploadImage(): could not create temporary texture image. (%s:%d)", basename((char*)__FILE__), __LINE__);
          return false;
        }

        if(mipmapped) gluBuild2DMipmaps(GL_TEXTURE_2D, newinternalformat, actualwidth, actualheight,	newformat, newtype, dummy.data);
        else glTexImage2D(GL_TEXTURE_2D, level, newinternalformat, actualwidth, actualheight,	0, newformat, newtype, dummy.data);
        glTexSubImage2D(GL_TEXTURE_2D, level, posX, posY, image->width, image->height, newformat, newtype, image->data);

        dummy.destroyImage();
      }
    // Texture created, set internal information
    width = image->width;
    height = image->height;
    internalformat = newinternalformat;
    format = newformat;
    type = newtype;
  } else  {
      // Texture exists and is compatible with the image
      glBindTexture(GL_TEXTURE_2D, idOpengl);

      // Simply try to update
      glTexSubImage2D(GL_TEXTURE_2D, level, posX, posY,image->width, image->height, newformat, newtype, image->data);
  }

  // Check last texture upload
  if(glGetError() != GL_NO_ERROR) {
    // Upload error
    glDisable(GL_TEXTURE_2D);
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_GRAPHICS, GCG_CREATIONFAILED), "uploadImage(): could not upload texture with glTexSubImage2D(). (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Ok, it's loaded
  glDisable(GL_TEXTURE_2D);
  return true;
}

// Load the texture object from an image
bool gcgTEXTURE2D::isCompatibleWith(gcgIMAGE *image) {
  // Check image
  int newinternalformat, newformat, newtype;
  return (image == NULL) ? false : (checkParameters(this, image, &newinternalformat, &newformat, &newtype) && image->height == height && image->width == width);
}

// Bind texture to be used. unbind() must be called when it is done.
bool gcgTEXTURE2D::bind() {
  if(this->idOpengl == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "bind(): trying to bind an invalid texture. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, idOpengl);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glScalef((float) this->width / (float) this->actualwidth, (float) this->height / (float) this->actualheight, 1.0);
  glMatrixMode(GL_MODELVIEW);
  return true;
}

// Unbind texture.
bool gcgTEXTURE2D::unbind() {
  if(idOpengl == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_INVALIDOBJECT), "unbind(): trying to unbind an invalid texture. (%s:%d)", basename((char*)__FILE__), __LINE__);
    glDisable(GL_TEXTURE_2D);
    return false;
  }

  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glDisable(GL_TEXTURE_2D);
  return true;
}
