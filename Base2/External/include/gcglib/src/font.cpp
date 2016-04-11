/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    FONT.CPP: functions for font handling.

    Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////// gcgFONT: class for font handling. ///////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
// Font simple construction.
/////////////////////////////////////////////////////////////////////////////////////
gcgFONT::gcgFONT() {
  nchars = 0;
  firstASCII = 0;
  textureID = 0;
  font_base = 0;
  basewidths = NULL;
  charheight = 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// Font destruction.
/////////////////////////////////////////////////////////////////////////////////////
gcgFONT::~gcgFONT() {
  releaseFont();
}


/////////////////////////////////////////////////////////////////////////////////////
// Load a font from a .gff file.
/////////////////////////////////////////////////////////////////////////////////////
bool gcgFONT::loadGFF(char *fontname) {
  // Check font name
  if(fontname == NULL) return false;

  // Opens the .gff file
  FILE *fontfile = fopen(fontname, "rb");
  if(fontfile == NULL) return false;

  fseek(fontfile, 0, SEEK_END);
  unsigned int filelength = ftell(fontfile);
  fseek(fontfile, 0, SEEK_SET);

  // Load raw data
  GCGFONTDATA *font = (GCGFONTDATA*) ALLOC(filelength);
  if(font == NULL) {
    fclose(fontfile);
    return false;
  }
  if(fread(font, sizeof(unsigned char), filelength, fontfile) != sizeof(unsigned char) * filelength) {
    fclose(fontfile);
    return false;
  }
  fclose(fontfile);

  // Check version. File size must match fontdatasize field.
  if(font->version != GCG_FONT_VERSION || font->fontdatasize != filelength) {
    SAFE_FREE(font);
    return false;
  }

  // Now, construct font object from data
  bool result = createFromData(font);

  // Free font file info
  SAFE_FREE(font);

  if(!result) releaseFont(); // Errors at this point should invalidate the font

  // Return creation result
  return result;
}

/////////////////////////////////////////////////////////////////////////////////////
// Create the font from a file structure.
/////////////////////////////////////////////////////////////////////////////////////
bool gcgFONT::createFromData(GCGFONTDATA *font) {
  // Check font information
  if(font == NULL) return false;

  // Check version
  if(font->version != GCG_FONT_VERSION) return false;

#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  if(wglGetCurrentContext() == NULL) return false;
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  if(glXGetCurrentContext() == NULL) return false;
#else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif

  // Free old resources
  releaseFont();

  // Copy basic information
  firstASCII = font->firstASCII;  // ASCII code of the first character in this font
  charheight = font->charheight;  // Character height in pixels

  // Compute how many characters are available
  unsigned int ncharsperrow = (unsigned int) font->width / font->cellwidth;
  unsigned int ncharspercol = (unsigned int) font->height / font->cellheight;
  nchars = ncharsperrow * ncharspercol;         // Number of characters in this font
  if(nchars + font->firstASCII > 255) nchars -= nchars + font->firstASCII - 255;

  // Compute pointers to the maps: character widths, base widths and font image data
  unsigned char *charwidths  = &(((unsigned char*) font)[sizeof(GCGFONTDATA)]);
  unsigned char *fontmap     = &charwidths[nchars + nchars];

  // Copy base widths
  this->basewidths = (unsigned char*) ALLOC(nchars * sizeof(unsigned char));
  if(this->basewidths == NULL) {
    releaseFont();
    return false;
  }
  memcpy(this->basewidths, &charwidths[nchars], nchars * sizeof(unsigned char));

  unsigned char *imagedata;
  // Check compression
  switch(font->compression) {
   case GCG_FONT_UNCOMPRESSED:  // Uncompressed. Simply use the data pointed by fontmap.
                                imagedata = fontmap;
                                break;

   case GCG_FONT_COMPRESSED_RLE:// RLE compressed. Read the packed data.
                                imagedata = (unsigned char*) ALLOC(font->height * font->width);
                                if(imagedata == NULL) {
                                  SAFE_FREE(this->basewidths);
                                  return false;
                                }

                                // Read and decompress
                                gcgDecompressImageRLE8(font->width, font->height, fontmap, imagedata);
                                break;

    default:                    // Unsupported compression
                                SAFE_FREE(this->basewidths);
                                return false;
  }

	// Gera textura para a fonte
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, font->width, font->height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, imagedata);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA8, font->width, font->height, GL_ALPHA, GL_UNSIGNED_BYTE, imagedata);

	// Muda a funcao de textura para MODULATE
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// Cria a uma lista para cada caractere
	font_base = glGenLists(nchars);

  float colfactor = (float) font->cellwidth  / (float) font->width;
  float rowfactor = (float) font->cellheight / (float) font->height;

	// Laço para criar as listas de cada caractere
	for(unsigned int ncaractere = 0; ncaractere < nchars; ncaractere++) {
		// Calcula coordenadas da textura
		float cx = (float) (ncaractere % ncharsperrow) * colfactor + (float) font->xoffset / font->width;
		float cy = (float) (ncaractere / ncharsperrow) * rowfactor + (float) font->yoffset / font->height;

		// Cria nova lista
		glNewList(font_base + ncaractere, GL_COMPILE);
			glBegin(GL_TRIANGLE_FAN);
				// Coordenadas do canto inferior/direito
				glTexCoord2f(cx + (float) charwidths[ncaractere] / (float) font->width, cy + (float) font->charheight  / (float) font->height);// da textura
				glVertex2i(charwidths[ncaractere], 0);							            // do poligono

				// Coordenadas do canto superior/direito
				glTexCoord2f(cx + (float) charwidths[ncaractere] / (float) font->width, cy); // da textura
				glVertex2i(charwidths[ncaractere], - (int) charheight);              // do poligono

				// Coordenadas do canto superior/esquerdo
				glTexCoord2f(cx, cy);					                // da textura
				glVertex2i(0, - (int) charheight);					  // do poligono

				// Coordenadas do canto inferior/esquerdo
				glTexCoord2f(cx, cy + (float) font->charheight  / (float) font->height);// da textura
				glVertex2i(0, 0);							                // do poligono
			glEnd();
			glTranslatef((float) this->basewidths[ncaractere], 0.f, 0.f);				// Move para a direita do caractere
		glEndList();
	}

  // Free uncompressed data
	if(imagedata != fontmap) SAFE_FREE(imagedata);
	// Created
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Release all font resources
/////////////////////////////////////////////////////////////////////////////////////

void gcgFONT::releaseFont() {
  SAFE_FREE(basewidths);

#if defined(GCG_WINDOWS32) || defined(GCG_WINDOWS64)
  if(wglGetCurrentContext() != NULL) {
#elif defined(GCG_LINUX32) || defined(GCG_LINUX64)
  if(glXGetCurrentContext() != NULL) {
#else
  #error "GCGLIB: unsupported operating system. Define GCG_WINDOWS32, GCG_LINUX32, GCG_WINDOWS64 or GCG_LINUX64."
#endif
    glDeleteTextures(1, &textureID);
    //glDeleteLists(font_base, nchars);
  }

  //nchars = 0;
  firstASCII = 0;
  textureID = 0;
  font_base = 0;
  charheight = 0;
  basewidths = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////
// Computes the box spanned by this string
/////////////////////////////////////////////////////////////////////////////////////
void gcgFONT::textLength(unsigned int *strwidth, unsigned int *strheight, char *str, int strlength) {
  // Check font
  if(basewidths == NULL || textureID == 0) {
    *strwidth = *strheight = 0;
    return;
  }

  register unsigned int w = 0;
  *strheight = charheight;
  for(register int i = 0; i < strlength && str[i] != 0; i++)
     if((unsigned char) str[i] >= firstASCII && (unsigned char) str[i] < firstASCII+nchars) w += basewidths[(unsigned char) str[i] - firstASCII];

  *strwidth  = w;
}

/////////////////////////////////////////////////////////////////////////////////////
// Computes how many chars fit to the width.
/////////////////////////////////////////////////////////////////////////////////////
int gcgFONT::charsThatFit(float maxwidth, char *str, int strlength) {
  // Check font
  if(basewidths == NULL || textureID == 0) return 0;

  register unsigned int n = 0;
  for(register int i = 0, sum = 0; i < strlength && str[i] != 0; i++)
   if((unsigned char) str[i] >= firstASCII && (unsigned char) str[i] < firstASCII + nchars) {
    if((float) basewidths[(unsigned char) str[i] - firstASCII] + sum < maxwidth) {
      sum += basewidths[(unsigned char) str[i] - firstASCII];
      n++;
    } else break;
   }

  return n;
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
////////////////// Utilities for GCGFONT file loading (.gff) ////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
// Utility: saves a GCG font file .gff.
/////////////////////////////////////////////////////////////////////////////////////
bool gcgSaveGFF(char *name, GCGFONTDATA *gcgfont) {
  // Check font information and parameters
  if(name == NULL || gcgfont == NULL) return false;
  if(gcgfont->cellwidth == 0 || gcgfont->cellheight == 0 || gcgfont->width == 0 || gcgfont->height == 0) return false;

  // Check version
  if(gcgfont->version != GCG_FONT_VERSION) return false;

	// Create file
  FILE *fontearq;
	fontearq = fopen(name, "wb");
	if(!fontearq) return false;

  unsigned char *imagedata = NULL;
  switch(gcgfont->compression) {
    case GCG_FONT_UNCOMPRESSED: // Try to compress the font data to save space
                                imagedata = (unsigned char*) ALLOC(gcgfont->width * gcgfont->height);
                                if(imagedata != NULL) {
                                  unsigned int imagesize = 0;
                                  int nchars = ((unsigned int) gcgfont->width / gcgfont->cellwidth) * ((unsigned int) gcgfont->height / gcgfont->cellheight);
                                  if(nchars + gcgfont->firstASCII > 255) nchars -= nchars + gcgfont->firstASCII - 255;

                                  if((imagesize = gcgCompressImageRLE8(gcgfont->width, gcgfont->height, &((unsigned char*) gcgfont)[sizeof(GCGFONTDATA)+nchars+nchars], imagedata)) > 0) {
                                    // Compressed, copy header information
                                    GCGFONTDATA newfont;
                                    memcpy(&newfont, gcgfont, sizeof(GCGFONTDATA));

                                    // Adjust and save information
                                    newfont.compression  = GCG_FONT_COMPRESSED_RLE;
                                    newfont.fontdatasize = sizeof(GCGFONTDATA) + nchars + nchars + imagesize;
                                    fwrite(&newfont, 1, sizeof(GCGFONTDATA), fontearq);
                                    // Save char and base widths
                                    fwrite(&((unsigned char*) gcgfont)[sizeof(GCGFONTDATA)], 1, nchars + nchars, fontearq);
                                    // Save compressed data
                                    fwrite(imagedata, 1, imagesize, fontearq);
                                  } else // Not a good compression, save original font
                                      fwrite(gcgfont, 1, gcgfont->fontdatasize, fontearq);

                                  // Release compressed data
                                  SAFE_FREE(imagedata);
                                } else // Allocation error, save original font
                                      fwrite(gcgfont, 1, gcgfont->fontdatasize, fontearq);
                                break;

    default:  // Assume gcgfont has all font data information
              fwrite(gcgfont, 1, gcgfont->fontdatasize, fontearq);
              break;
  }

  // Finished
  fclose(fontearq);

  // Ok, it's saved
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////
// Utility: imports a .bff font file.
/////////////////////////////////////////////////////////////////////////////////////
GCGFONTDATA *gcgImportBFF(char *name) {
#pragma pack(push, 1)  // Must have an aligment of 1 bytes
  typedef struct _BFF {
    unsigned short version;
    unsigned int width;
    unsigned int height;
    unsigned int cellwidth;
    unsigned int cellheight;
    unsigned char bpp;
    unsigned char basechar;
    unsigned char basewidths[256];
  } BFF;
#pragma pack(pop)  // Must have an aligment of 1 bytes

	// Abre arquivo da fonte e verifica sua validade
  FILE *fontearq;
	fontearq = fopen(name, "rb");
	if(fontearq == NULL) return NULL;

	// Load and check header
	BFF header;
	if(fread(&header, 1, sizeof(BFF), fontearq) != sizeof(BFF)) {
	  fclose(fontearq);
	  return NULL;
	}

	if(header.bpp != 8) {
	  fclose(fontearq);
	  return NULL;
	}

	  // Create and load the font map
  unsigned char *fontmap = (unsigned char*) ALLOC(header.width * header.height * sizeof(unsigned char));
  if(fontmap == NULL) {
    fclose(fontearq);
    return NULL;
  }

  if(fread(fontmap, sizeof(unsigned char), header.width * header.height, fontearq) != header.width * header.height) {
    fclose(fontearq);
    return NULL;
  }
  fclose(fontearq);

	// Compute number of chars
  int nchars = ((unsigned int) header.width / header.cellwidth) * ((unsigned int) header.height / header.cellheight);
  if(nchars + header.basechar > 255) nchars -= nchars + header.basechar - 255;

  unsigned char compression = GCG_FONT_UNCOMPRESSED;
  unsigned int  imagesize = 0;
  // Try to compress the bitmap using RLE encoding
  unsigned char *imagedata = (unsigned char*) ALLOC(header.width * header.height);
  if(imagedata != NULL)
    if((imagesize = gcgCompressImageRLE8(header.width, header.height, fontmap, imagedata)) > 0) compression = GCG_FONT_COMPRESSED_RLE;


  // Now we know the GCGFONTDATA size
  unsigned int datasize = sizeof(GCGFONTDATA) + nchars + nchars + ((compression == GCG_FONT_COMPRESSED_RLE) ? imagesize : header.width * header.height);
  GCGFONTDATA *gcgfont = (GCGFONTDATA*) ALLOC(datasize);
  if(gcgfont == NULL) {
    SAFE_FREE(fontmap);
    SAFE_FREE(imagedata);
    return NULL;
  }

  // Copy info
  gcgfont->version = GCG_FONT_VERSION;
  gcgfont->fontdatasize = datasize;
  gcgfont->compression = compression;
	gcgfont->width = header.width;
  gcgfont->height = header.height;
  gcgfont->cellwidth = header.cellwidth;
  gcgfont->cellheight = header.cellheight;
  gcgfont->firstASCII = header.basechar;

  // Compute pointers to the maps: character widths, base widths and font image data
  unsigned char *charwidths = &(((unsigned char*) gcgfont)[sizeof(GCGFONTDATA)]);
  unsigned char *basewidths = charwidths + nchars;

  // Copy base array
  for(int i = 0; i < nchars; i++) basewidths[i] = header.basewidths[i + header.basechar];

  // Compute information about each character texture inside the cell
  gcgfont->xoffset = gcgfont->cellheight;
  gcgfont->yoffset = gcgfont->cellwidth;
  gcgfont->charheight = 0;

  unsigned int ncharsperrow = (unsigned int) gcgfont->width / gcgfont->cellwidth;

	unsigned int minX = gcgfont->cellwidth, minY = gcgfont->cellheight, maxY = 0;
	// Get info for every character
	for(int ncaractere = 0; ncaractere < nchars; ncaractere++) {
      unsigned int cx = (ncaractere % ncharsperrow) * gcgfont->cellwidth, cy = ncaractere / ncharsperrow  * gcgfont->cellheight;

      // Check every char pixel
      unsigned int minx = gcgfont->cellwidth, maxx = 0, miny = gcgfont->cellheight, maxy = 0;
      for(unsigned int m = 0; m < gcgfont->cellwidth; m++)
        for(unsigned int n = 0; n < gcgfont->cellheight; n++)
          if(fontmap[(n + cy) * gcgfont->width + m + cx] != 0) {
            // Non-zero pixel
            if(minx > m) minx = m;
            if(maxx < m) maxx = m;
            if(miny > n) miny = n;
            if(maxy < n) maxy = n;
          }

      charwidths[ncaractere] = (unsigned char) (maxx + 1);
      if(minX > minx) minX = minx;
      if(minY > miny) minY = miny;
      if(maxY < maxy) maxY = maxy;
    }
  // Update information
  gcgfont->xoffset = minX;
  gcgfont->yoffset = minY;
  gcgfont->charheight = maxY + 1 - minY;
  for(int i = 0; i < nchars; i++)
    if(charwidths[i] > 0) charwidths[i] = (unsigned char) (charwidths[i] - gcgfont->xoffset);

  // Copy image information
  if(imagesize == 0) memcpy(basewidths + nchars, fontmap, gcgfont->height * gcgfont->width); // Save the raw font data
  else memcpy(basewidths + nchars, imagedata, imagesize); // Save the compressed font data

  SAFE_FREE(fontmap);    // Release temporary uncompressed font
  SAFE_FREE(imagedata);  // Release temporary compressed font

	return gcgfont;
}
