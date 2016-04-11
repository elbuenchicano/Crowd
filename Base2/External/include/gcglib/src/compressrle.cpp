/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   COMPRESSRLE.CPP: functions for RLE data compression.

   Marcelo Bernardes Vieira
 **************************************************************************************/
#include "system.h"

unsigned int gcgPackRLE8(unsigned int fullsize, unsigned char *srcdata, unsigned char *RLEdata) {
  unsigned int srcindex, dstindex = sizeof(unsigned int), counter, i;

  // Check parameters
  if(fullsize == 0 || srcdata == NULL || RLEdata == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgPackRLE8(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  // First 4 bytes of RLEdata keeps original size: dstindex must be sizeof(unsigned int).
  *((unsigned int*) RLEdata) = 0;  // Initially zero

  // Scans every byte
  for(srcindex = 0; srcindex < fullsize;) {
state_start:
    // If next byte is the same, enter compress mode
    if(srcdata[srcindex] == srcdata[srcindex + 1]) goto state_compress;

    // We have two distinct bytes
    if(srcindex + 2 >= fullsize) { // Are these the last 2 bytes of the data?
      if(dstindex + 4 >= fullsize) {
        gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE8(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
        return 0; // It does not give a good compression.
      }
      RLEdata[dstindex++] = 1; // block of length 1
      RLEdata[dstindex++] = srcdata[srcindex++];
      RLEdata[dstindex++] = 1; // block of length 1
      RLEdata[dstindex++] = srcdata[srcindex++];
      goto end_of_data;
    }

    // Check if in next 3 bytes, the last 2 consecutive bytes are the same
    if(srcdata[srcindex + 1] == srcdata[srcindex + 2]) {
       if(dstindex + 2 >= fullsize) {
         gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE8(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
         return 0; // It does not give a good compression.
       }
       // Save first byte
       RLEdata[dstindex++] = 1; // block of length 1
       RLEdata[dstindex++] = srcdata[srcindex++];

       // Now, the last two bytes can be compressed
       goto state_compress;
    } else goto state_no_compress; // For the last 3 bytes, no 2 consecutive bytes are the same.

state_compress:  // Compress mode
    // srcindex is the first byte of a compressed block
    // counter is the number of additional bytes following current byte
    // (srcindex+counter) point to the last byte of current block
    for(counter = 1; counter <= 254; counter++)  {
       // We must check this condition first.
       if(srcdata[srcindex + counter] != srcdata[srcindex]) break;

       if(srcindex + counter + 2 >= fullsize) { // Did it reach the end of data?
          if(dstindex + 2 >= fullsize) {
            gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE8(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
            return 0; // It does not give a good compression.
          }
          RLEdata[dstindex++] = (unsigned char) (counter + 1);
          RLEdata[dstindex++] = srcdata[srcindex];
          srcindex = srcindex + counter + 1;

          goto end_of_data;
       }
    }

    // now srcdata[srcindex + counter] is out of the block
    if(dstindex + 2 >= fullsize) {
      gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE8(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return 0; // It does not give a good compression.
    }
    RLEdata[dstindex++] = (unsigned char) counter; // block length is counter
    RLEdata[dstindex++] = srcdata[srcindex];
    srcindex += counter;
    goto state_start;

state_no_compress:
      // srcindex points to the first byte of an uncompressed block that needs at least 3 bytes.
      // counter is the number of additional bytes following current byte.
      for(counter = 2; counter <= 254; counter++) {
        if(srcindex + counter + 2 >= fullsize) { // Did it reach the end of data?
           if(dstindex + counter + 4 >= fullsize) return 0; // It does not give a good compression.
           RLEdata[dstindex++] = 0; // Escape character for an uncompressed block
           RLEdata[dstindex++] = (unsigned char) (counter + 1); // block length is (counter + 1)
           for(i = 0; i <= counter; i++) RLEdata[dstindex++] = (unsigned char) srcdata[srcindex++];
           goto end_of_data;
        }

        if(srcindex + counter + 2 >= fullsize || srcdata[srcindex + counter] != srcdata[srcindex + counter + 1])
           continue; // Still no 2 consecutive bytes are the same, thus extend the block.

        // Here we found the last two bytes are the same.
        if(srcindex + counter + 3 >= fullsize || srcdata[srcindex + counter + 1] != srcdata[srcindex + counter + 2])
           continue; // But the third byte is not the same, thus go ahead in uncompressed mode

        // Here, we found the last three bytes are the same. We need to exit the uncompressed mode
        if(counter > 2) counter--; // we can shrink the block one byte (go backward)

        if(dstindex + counter + 2 >= fullsize) {
          gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE8(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
          return 0; // It does not give a good compression.
        }
        RLEdata[dstindex++] = 0; // Escape character for uncompressed block
        RLEdata[dstindex++] = (unsigned char) (counter + 1); // block length is (counter+1)
        for(i = 0; i <= counter; i++) RLEdata[dstindex++] = srcdata[srcindex++];
        goto state_compress;
      } // End of for (counter = 2; counter <= 254; counter++)

      // Now we have a full block of 255 bytes
      if(dstindex + counter + 2 >= fullsize) return 0; // It does not give a good compression.
      RLEdata[dstindex++] = 0; // escape character for uncompressed block
      RLEdata[dstindex++] = (unsigned char) counter; // block length is (counter)
      for(i = 0; i < counter; i++) RLEdata[dstindex++] = srcdata[srcindex++];

      goto state_start;
    }

   // Finish this compression
end_of_data:
  if(dstindex + 2 >= fullsize) {
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE8(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0; // It does not give a good compression.
  }
  RLEdata[dstindex++] = 0; // Mark end-of-data
  RLEdata[dstindex++] = 1;

  // First 4 bytes of RLEdata keeps original size
  *((unsigned int*) RLEdata) = fullsize; // Now it is compressed

  // Compressed RLE data size
  return dstindex;
}


////////////////////////////////////////////////////////////
// Decompress a 8 bit Run-Length Encoded data.
////////////////////////////////////////////////////////////
unsigned int gcgUnpackRLE8(unsigned char *RLEdata, unsigned char *dstdata) {
  // Decoder for RLE8 compression
  unsigned int srcindex = sizeof(unsigned int);
  volatile unsigned int dstindex = 0;

  // Check parameters
  if(RLEdata == NULL || dstdata == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgUnpackRLE8(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  // First four bytes keep the size of the data block: srcindex must be sizeof(unsigned int).
  unsigned int fullsize = *((unsigned int*) RLEdata);

  for(; srcindex < fullsize;) {
    int c = (unsigned char) RLEdata[srcindex++]; // Number of bytes or 00 code

    if(c > 0) {
      int ntimes = (int) c;  // Number of bytes
      c = RLEdata[srcindex++];
      if(dstindex + ntimes > fullsize) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_DECODEERROR), "gcgUnpackRLE8(): Invalid compressed data. (%s:%d)", basename((char*)__FILE__), __LINE__);
        return 0; // Invalid compressed data
      }
      for(int j = 0; j < ntimes; j++) dstdata[dstindex++] = (unsigned char) c;
    } else {
        c = RLEdata[srcindex++]; // Get code
        switch(c) {
          case  1: srcindex = fullsize; break; // End of data
          case  2: gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_DECODEERROR), "gcgUnpackRLE8(): Invalid compressed data. (%s:%d)", basename((char*)__FILE__), __LINE__);
                   return 0; // Unused. Indicates an error
          default:// Uncompressed indexes
                  int ntimes = (int) c;
                  if(dstindex + ntimes > fullsize) {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_DECODEERROR), "gcgUnpackRLE8(): Invalid compressed data. (%s:%d)", basename((char*)__FILE__), __LINE__);
                    return 0; // Invalid compressed data
                  }
                  for(int j = 0; j < ntimes; j++) dstdata[dstindex++] = (unsigned char) RLEdata[srcindex++];
                  break;
        }
    }
  }

  // Effective unpacked bytes
  return dstindex;
}



unsigned int gcgPackRLE32(unsigned int sizebytes, unsigned char *srcdata, unsigned char *RLEdata) {
  unsigned int srcindex, dstindex = sizeof(unsigned int), counter, i;
  unsigned int fullsize = sizebytes / sizeof(unsigned int); // Must be multiple of 4
  unsigned int *base = (unsigned int*) srcdata;

  // Check parameters
  if(fullsize == 0 || srcdata == NULL || RLEdata == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgPackRLE32(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  // First 4 bytes of RLEdata keeps original size: dstindex must be sizeof(unsigned int).
  *((unsigned int*) RLEdata) = 0; // Initially 0

  // Scans every integer
  for(srcindex = 0; srcindex < fullsize;) {
state_start:
    // If next integer is the same, enter compress mode
    if(base[srcindex] == base[srcindex + 1]) goto state_compress;

    // We have two distinct bytes
    if(srcindex + 2 >= fullsize) { // Are these the last 2 integers of the data?
      if(dstindex + 10 >= sizebytes) {
        gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE32(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
        return 0; // It does not give a good compression.
      }
      RLEdata[dstindex++] = 1; // block of length 1
      *((unsigned int*) (&RLEdata[dstindex])) = base[srcindex++]; dstindex += sizeof(unsigned int);
      RLEdata[dstindex++] = 1; // block of length 1
      *((unsigned int*) (&RLEdata[dstindex])) = base[srcindex++]; dstindex += sizeof(unsigned int);
      goto end_of_data;
    }

    // Check if in next 3 integers, the last 2 consecutive integers are the same
    if(base[srcindex + 1] == base[srcindex + 2]) {
       if(dstindex + 5 >= sizebytes) {
         gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE32(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
         return 0; // It does not give a good compression.
       }
       // Save first integer
       RLEdata[dstindex++] = 1; // block of length 1
       *((unsigned int*) (&RLEdata[dstindex])) = base[srcindex++]; dstindex += sizeof(unsigned int);

       // Now, the last two integers can be compressed
       goto state_compress;
    } else goto state_no_compress; // For the last 3 bytes, no 2 consecutive bytes are the same.

state_compress:  // Compress mode
    // srcindex is the first integer of a compressed block
    // counter is the number of additional integers following current integer
    // (srcindex+counter) point to the last byte of current block
    for(counter = 1; counter <= 254; counter++)  {
       // We must check this condition first.
       if(base[srcindex + counter] != base[srcindex]) break;

       if(srcindex + counter + 2 >= fullsize) { // Did it reach the end of data?
          if(dstindex + 5 >= sizebytes) {
            gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE32(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
            return 0; // It does not give a good compression.
          }
          RLEdata[dstindex++] = (unsigned char) (counter + 1);
          *((unsigned int*) (&RLEdata[dstindex])) = base[srcindex];
          dstindex += sizeof(unsigned int);
          srcindex = srcindex + counter + 1;

          goto end_of_data;
       }
    }

    // now base[srcindex + counter] is out of the block
    if(dstindex + 5 >= sizebytes) {
      gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE32(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return 0; // It does not give a good compression.
    }
    RLEdata[dstindex++] = (unsigned char) counter; // block length is counter
    *((unsigned int*) (&RLEdata[dstindex])) = base[srcindex]; dstindex += sizeof(unsigned int);
    srcindex += counter;
    goto state_start;

state_no_compress:
      // srcindex points to the first byte of an uncompressed block that needs at least 3 bytes.
      // counter is the number of additional integers following current byte.
      for(counter = 2; counter <= 254; counter++) {
        if(srcindex + counter + 2 >= fullsize) { // Did it reach the end of data?
           if(dstindex + counter * sizeof(unsigned int) + 2 >= sizebytes) return 0; // It does not give a good compression.
           RLEdata[dstindex++] = 0; // Escape character for an uncompressed block
           RLEdata[dstindex++] = (unsigned char) (counter + 1); // block length is (counter + 1)
           for(i = 0; i <= counter; i++) {
             *((unsigned int*) (&RLEdata[dstindex])) = base[srcindex++];
             dstindex += sizeof(unsigned int);
           }
           goto end_of_data;
        }

        if(srcindex + counter + 2 >= fullsize || base[srcindex + counter] != base[srcindex + counter + 1])
           continue; // Still no 2 consecutive integers are the same, thus extend the block.

        // Here we found the last two bytes are the same.
        if(srcindex + counter + 3 >= fullsize || base[srcindex + counter + 1] != base[srcindex + counter + 2])
           continue; // But the third integer is not the same, thus go ahead in uncompressed mode

        // Here, we found the last three bytes are the same. We need to exit the uncompressed mode
        if(counter > 2) counter--; // we can shrink the block one integer (go backward)

        if(dstindex + counter * sizeof(unsigned int) + 2 >= sizebytes) return 0; // It does not give a good compression.
        RLEdata[dstindex++] = 0; // Escape character for uncompressed block
        RLEdata[dstindex++] = (unsigned char) (counter + 1); // block length is (counter+1)
        for(i = 0; i <= counter; i++) {
          *((unsigned int*) (&RLEdata[dstindex])) = base[srcindex++];
          dstindex += sizeof(unsigned int);
        }
        goto state_compress;
      } // End of for (counter = 2; counter <= 254; counter++)

      // Now we have a full block of 255 bytes
      if(dstindex + counter * sizeof(unsigned int) + 2 >= sizebytes) return 0; // It does not give a good compression.
      RLEdata[dstindex++] = 0; // escape character for uncompressed block
      RLEdata[dstindex++] = (unsigned char) counter; // block length is (counter)

      for(i = 0; i < counter; i++) {
        *((unsigned int*) (&RLEdata[dstindex])) = base[srcindex++];
        dstindex += sizeof(unsigned int);
      }
      goto state_start;
    }

    // Finish this compression
end_of_data:
  if(dstindex + 2 >= sizebytes) {
    gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_MEMORY, GCG_NOBUFFER), "gcgPackRLE32(): Original data is smaller than compressed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0; // It does not give a good compression.
  }
  RLEdata[dstindex++] = 0; // Mark end-of-data
  RLEdata[dstindex++] = 1;

  // First 4 bytes of RLEdata keeps original size
  *((unsigned int*) RLEdata) = sizebytes; // Now it is compressed

  // Compressed RLE data size
  return dstindex;
}


////////////////////////////////////////////////////////////
// Decompress a 32 bit Run-Length Encoded data.
////////////////////////////////////////////////////////////
unsigned int gcgUnpackRLE32(unsigned char *RLEdata, unsigned char *dstdata) {
  // Decoder for RLE32 compression
  unsigned int srcindex = sizeof(unsigned int);
  volatile unsigned int dstindex = 0;
  unsigned int *intdata = (unsigned int*) dstdata;

  // Check parameters
  if(RLEdata == NULL || dstdata == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgUnpackRLE32(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  // First four bytes keep the size of the data block: srcindex must be sizeof(unsigned int).
  unsigned int fullsize = *((unsigned int*) RLEdata);
  unsigned int sizeint = fullsize / sizeof(unsigned int);

  for(; srcindex < fullsize;) {
    int c = (unsigned char) RLEdata[srcindex++]; // Number of integers or 00 code

    if(c > 0) {
      int ntimes = (int) c;  // Number of integers
      unsigned int i = *((unsigned int*) &RLEdata[srcindex]); srcindex += 4;
      if(dstindex + ntimes > sizeint) {
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_DECODEERROR), "gcgUnpackRLE32(): Invalid compressed data. (%s:%d)", basename((char*)__FILE__), __LINE__);
        return 0; // Invalid compressed data
      }
      for(int j = 0; j < ntimes; j++) intdata[dstindex++] = i;
    } else {
        c = RLEdata[srcindex++]; // Get code
        switch(c) {
          case  1: srcindex = fullsize; break; // End of data
          case  2: gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_DECODEERROR), "gcgUnpackRLE32(): Invalid compressed data. (%s:%d)", basename((char*)__FILE__), __LINE__);
                   return 0; // Unused. Indicates an error
          default:// Uncompressed indexes
                  int ntimes = (int) c;
                  if(dstindex + ntimes > sizeint) {
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_DECODEERROR), "gcgUnpackRLE32(): Invalid compressed data. (%s:%d)", basename((char*)__FILE__), __LINE__);
                    return 0; // Invalid compressed data
                  }
                  for(int j = 0; j < ntimes; j++) {
                    intdata[dstindex++] = *((unsigned int*) &RLEdata[srcindex]);
                    srcindex += 4;
                  }
                  break;
        }
    }
  }

  // Effective unpacked bytes
  return srcindex;
}


bool gcgSaveFloatsAsText(unsigned int size, float *data, char *outputname) {
  // Check parameters
  if(data == NULL || outputname == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgSaveFloatsAsText(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  if(size > 0) {
    // Create file
    FILE *outputarq = fopen(outputname, "wt+");
    if(!outputarq) {
      gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_FILE, GCG_OPENERROR), "gcgSaveFloatsAsText(): Error opening file %s. (%s:%d)", outputname, basename((char*)__FILE__), __LINE__);
      return false;
    }

    fprintf(outputarq, "\nstatic float data[%d] = {", size);
    for(unsigned int i = 0; i < size; i++) {
      fprintf(outputarq, "%f", (float) data[i]);
      if(i < size-1) fprintf(outputarq, ",");
      if(i > 0 && (i % 255) == 0) fprintf(outputarq, "\n          ");
    }
    fprintf(outputarq, "};");

    // Finished
    fclose(outputarq);
  }

	return true;
}


bool gcgSaveBytesAsText(unsigned int size, unsigned char *data, char *outputname) {
  // Check parameters
  if(data == NULL || outputname == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgSaveBytesAsText(): Invalid parameters. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return 0;
  }

  if(size > 0) {
    // Create file
    FILE *outputarq = fopen(outputname, "wt+");
    if(!outputarq) {
      gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_FILE, GCG_OPENERROR), "gcgSaveBytesAsText(): Error opening file %s. (%s:%d)", outputname, basename((char*)__FILE__), __LINE__);
      return false;
    }

    fprintf(outputarq, "\nstatic unsigned char data[%d] = {", size);
    for(unsigned int i = 0; i < size; i++) {
      fprintf(outputarq, "%d", (int) data[i]);
      if(i < size-1) fprintf(outputarq, ",");
      if(i > 0 && (i % 255) == 0) fprintf(outputarq, "\n          ");
    }
    fprintf(outputarq, "};");

    // Finished
    fclose(outputarq);
  }

	return true;
}
