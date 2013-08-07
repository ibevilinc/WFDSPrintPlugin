/*
(c) Copyright 2013 Hewlett-Packard Development Company, L.P.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
/*
(c) Copyright 2013 Hewlett-Packard Development Company, L.P.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
/*
 * File: genJPEGStrips.c
 * Purpose: this file implements routines to compress and decompress jpeg image files.  It utilizes
 * libjpeg to do the "work".
 */

#include <stdio.h>
//#include <unistd.h>
#include <stdlib.h>
#include "common_defines.h"
#include "PCLmGenerator.h"

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */
extern "C"
{
#include "jpeglib.h"
}

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in
 * the second part of the example.
 */

#include <setjmp.h>

#include <malloc.h>
#include <memory.h>



/******************** JPEG COMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to feed data into the JPEG compressor.
 * We present a minimal version that does not worry about refinements such
 * as error recovery (the JPEG code will just exit() if it gets an error).
 */


/*
 * IMAGE DATA FORMATS:
 *
 * The standard input image format is a rectangular array of pixels, with
 * each pixel having the same number of "component" values (color channels).
 * Each pixel row is an array of JSAMPLEs (which typically are unsigned chars).
 * If you are working with color data, then the color values for each pixel
 * must be adjacent in the row; for example, R,G,B,R,G,B,R,G,B,... for 24-bit
 * RGB color.
 *
 * For this example, we'll assume that this data structure matches the way
 * our application has stored the image in memory, so we can just pass a
 * pointer to our image buffer.  In particular, let's say that the image is
 * RGB color and is described by:
 */

/* typedef char JSAMPLE;*/

int image_width;
int image_height;
int image_numComponents;
extern int DebugIt;
extern int DebugIt2;

unsigned char *myImageBuffer=NULL;

typedef struct jpeg_decompress_struct cinfoCompressType; 

// int put_scanline_someplace(jpeg_decompress_struct *cinfo, char *buffer, int physicalWidth)
int put_scanline_someplace(cinfoCompressType *cinfo, char *buffer, int physicalWidth)
{
  static unsigned char *destBuffer;
  if(myImageBuffer==NULL)
  {
     int width         =cinfo->image_width;
     int height        =cinfo->image_height;
     int numComponents =(int)cinfo->num_components;
     int allocSize     =width*height*numComponents;
#if 0
     printf("Image Stats\n");
     printf("  ImageWidth=%d\n",width);
     printf("  ImageHeight=%d\n",height);
     printf("  ImageNumComponents=%d\n",numComponents);
     // printf("  ImageColorSpace=%d\n");
     //printf("  ImageScale=%d\n",scale);
#endif
     myImageBuffer=(unsigned char*)malloc(allocSize*2);
/*     if(DebugIt2)
     {
        printf("myImageBuffer: allocated; size=%d\n",allocSize*2);
     }*/
     if(!myImageBuffer)
     {
        printf("Could not allocate enough memory for source JPEG file; exiting\n");
        exit(-1);
     }
     memset(myImageBuffer,0x0,allocSize);
     image_width=width;
     image_height=height;
     image_numComponents=numComponents;
     destBuffer=myImageBuffer;
  }
  memcpy(destBuffer,buffer,physicalWidth);
  destBuffer+=physicalWidth;
  return(1);
}

/* Sample routine for JPEG compression.  We assume that the target file name
 * and a compression quality factor are passed in. */

/* setup the buffer but we did that in the calling function */
void init_buffer(jpeg_compress_struct* cinfo) 
{}

/* what to do when the buffer is full; this should almost never
 * happen since we allocated our buffer to be big to start with */
boolean empty_buffer(jpeg_compress_struct* cinfo) { 
        return TRUE;
}

/* finalize the buffer and do any cleanup stuff */
void term_buffer(jpeg_compress_struct* cinfo) 
{}

GLOBAL(void)
write_JPEG_Buff (ubyte * buffPtr, int quality, int image_width, int image_height, JSAMPLE *imageBuffer, int resolution, colorSpaceDisposition destCS, int *numCompBytes)
{

  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;
  struct jpeg_destination_mgr dmgr;

  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */


  /* here is the magic */
  dmgr.init_destination    = init_buffer;
  dmgr.empty_output_buffer = empty_buffer;
  dmgr.term_destination    = term_buffer;
  dmgr.next_output_byte    = (JOCTET *)buffPtr;
  dmgr.free_in_buffer      = image_width * image_height * 3;

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  cinfo.dest = &dmgr;

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = image_width; 	  /* image width and height, in pixels */
  cinfo.image_height = image_height;
  cinfo.input_components = 3;		  /* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	  /* colorspace of input image */

  if(destCS==deviceRGB || destCS==adobeRGB)
  {
    cinfo.in_color_space   = JCS_RGB; 	    /* colorspace of input image */
    cinfo.jpeg_color_space = JCS_RGB; 	    /* colorspace of input image */
    cinfo.input_components = 3;
  }
  else
  {
    cinfo.in_color_space   = JCS_GRAYSCALE; /* colorspace of input image */
    cinfo.jpeg_color_space = JCS_GRAYSCALE; /* colorspace of input image */
    cinfo.input_components = 1;
  }
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);


  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  /* Set the density so that the JFIF header has the correct settings */
  cinfo.density_unit=1;      /* 1=dots-per-inch, 2=dots per cm */
  cinfo.X_density=(UINT16)resolution;
  cinfo.Y_density=(UINT16)resolution;

  /* set the rows/columns setting to reflect the resolution */
  /* MCU = Minimum Coded Unit */
#if 0
  cinfo.MCUs_per_row    =image_width;
  cinfo.MCU_rows_in_scan=image_height;
#else
  //cinfo.MCUs_per_row    =16;
  cinfo.MCUs_per_row    =image_width;
  // cinfo.MCU_rows_in_scan=16;
  cinfo.MCU_rows_in_scan=image_height;
#endif

/*  if(DebugIt2)
  {
     printf("cinfo:\n");
     printf("  width=%d\n",cinfo.image_width);
     printf("  height=%d\n",cinfo.image_height);
     printf("  input_components=%d\n",cinfo.input_components);
     printf("  input_in_color_space=%d\n",cinfo.in_color_space);
     printf("  input_out_color_space=%d\n",cinfo.jpeg_color_space);
     printf("  num_components=%d\n",cinfo.num_components);
    
     printf("  X_density=%d\n",cinfo.X_density);
     printf("  Y_density=%d\n",cinfo.Y_density);
     printf("  MCUs_per_row=%d\n",cinfo.MCUs_per_row);
     printf("  MCUs_rows_in_scan=%d\n",cinfo.MCU_rows_in_scan);
  }*/

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = image_width * cinfo.input_components;	/* JSAMPLEs per row in , imageBuffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    //printf("image_height=%d, image_width=%d\n",cinfo.image_height, cinfo.image_width);
    row_pointer[0] = & imageBuffer[cinfo.next_scanline * row_stride];
 
/*    if(DebugIt)
    {
       printf("putting JPEG scanline at address 0x%x\n",imageBuffer[cinfo.next_scanline * row_stride]);
       JSAMPLE *i,*j;
       i=(JSAMPLE*)(imageBuffer[cinfo.next_scanline * row_stride]);
       j=(JSAMPLE*)(imageBuffer);
       printf("  offset=%ld\n",i-j);
    }*/
    
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  *numCompBytes=(int)(cinfo.dest->next_output_byte - buffPtr);

  /* And we're done! */
}

#if 1
GLOBAL(void)
write_JPEG_file (char * filename, int quality, int image_width, int image_height, JSAMPLE *imageBuffer, int resolution, colorSpaceEnum destCS)
{
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;

  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  FILE * outfile;		/* target file */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */

  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  if ((outfile = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    exit(1);
  }
  jpeg_stdio_dest(&cinfo, outfile);

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = image_width; 	  /* image width and height, in pixels */
  cinfo.image_height = image_height;
  cinfo.input_components = 3;		  /* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	  /* colorspace of input image */
#if 0
  if(destCS==RGB)
    cinfo.in_color_space = JCS_RGB; 	  /* colorspace of input image */
  else
    cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace of input image */
#endif
  if(destCS==RGB)
  {
    cinfo.in_color_space = JCS_RGB; 	  /* colorspace of input image */
    cinfo.jpeg_color_space = JCS_RGB; 	  /* colorspace of input image */
    cinfo.input_components=3;
  }
  else
  {
    cinfo.in_color_space = JCS_RGB;         /* colorspace of input image */
    cinfo.jpeg_color_space = JCS_GRAYSCALE; /* colorspace of input image */
    cinfo.input_components=3;
  }
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);

   
/*  if(DebugIt)
  {
     printf("cinfo:\n");
     printf("  width=%d\n",cinfo.image_width);
     printf("  height=%d\n",cinfo.image_height);
     printf("  input_components=%d\n",cinfo.input_components);
     printf("  input_in_color_space=%d\n",cinfo.in_color_space);
     printf("  input_out_color_space=%d\n",cinfo.jpeg_color_space);
     printf("  num_components=%d\n",cinfo.num_components);
    
  }*/

  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  /* Set the density so that the JFIF header has the correct settings */
  cinfo.X_density=(UINT16)resolution;
  cinfo.Y_density=(UINT16)resolution;

  /* set the rows/columns setting to reflect the resolution */
  cinfo.MCUs_per_row    =16;
  cinfo.MCU_rows_in_scan=16;

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = image_width * 3;	/* JSAMPLEs per row in , imageBuffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    //printf("image_height=%d, image_width=%d\n",cinfo.image_height, cinfo.image_width);
    row_pointer[0] = & imageBuffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);
  /* After finish_compress, we can close the output file. */
  fclose(outfile);

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);

  /* And we're done! */
}
#endif


/*
 * SOME FINE POINTS:
 *
 * In the above loop, we ignored the return value of jpeg_write_scanlines,
 * which is the number of scanlines actually written.  We could get away
 * with this because we were only relying on the value of cinfo.next_scanline,
 * which will be incremented correctly.  If you maintain additional loop
 * variables then you should be careful to increment them properly.
 * Actually, for output to a stdio stream you needn't worry, because
 * then jpeg_write_scanlines will write all the lines passed (or else exit
 * with a fatal error).  Partial writes can only occur if you use a data
 * destination module that can demand suspension of the compressor.
 * (If you don't know what that's for, you don't need it.)
 *
 * If the compressor requires full-image buffers (for entropy-coding
 * optimization or a multi-scan JPEG file), it will create temporary
 * files for anything that doesn't fit within the maximum-memory setting.
 * (Note that temp files are NOT needed if you use the default parameters.)
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 *
 * Scanlines MUST be supplied in top-to-bottom order if you want your JPEG
 * files to be compatible with everyone else's.  If you cannot readily read
 * your data in that order, you'll need an intermediate array to hold the
 * image.  See rdtarga.c or rdbmp.c for examples of handling bottom-to-top
 * source data using the JPEG code's internal virtual-array mechanisms.
 */



/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to read data from the JPEG decompressor.
 * It's a bit more refined than the above, in that we show:
 *   (a) how to modify the JPEG library's standard error-reporting behavior;
 *   (b) how to allocate workspace using the library's memory manager.
 *
 * Just to make this example a little different from the first one, we'll
 * assume that we do not intend to put the whole image into an in-memory
 * buffer, but to send it line-by-line someplace else.  We need a one-
 * scanline-high JSAMPLE array as a work buffer, and we will let the JPEG
 * memory manager allocate it for us.  This approach is actually quite useful
 * because we don't need to remember to deallocate the buffer separately: it
 * will go away automatically when the JPEG object is cleaned up.
 */


/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


/*
 * Sample routine for JPEG decompression.  We assume that the source file name
 * is passed in.  We want to return 1 on success, 0 on error.
 */


extern colorSpaceEnum sourceColorSpace;

GLOBAL(int)
read_JPEG_file (char * filename)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;


  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my_error_mgr jerr;
  /* More stuff */
  FILE * infile;		/* source file */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return 0;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return 0;
  }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);

/*  if(DebugIt2)
  {
    printf("Source Image Stats\n");
    printf("  Width=%d\n",cinfo.image_width);
    printf("  height=%d\n",cinfo.image_height);
    printf("  num_components=%d\n",cinfo.num_components);
    if(cinfo.out_color_space==JCS_RGB)
    {
      sourceColorSpace=RGB;      
      printf("  JPEG color space==RGB\n");
    }
    else if(cinfo.out_color_space==JCS_GRAYSCALE)
    {
      sourceColorSpace=GRAY;      
      printf("  JPEG color space==RGB\n");
    }
    else
    {
      sourceColorSpace=unknown;      
      printf("  JPEG color space==RGB\n");
    }
  }*/
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;
  /* Make a one-row-high sample array that will go away when done with image */
  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
    /* Assume put_scanline_someplace wants a pointer and sample count. */
    put_scanline_someplace(&cinfo, (char *)buffer[0], row_stride);
  }

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose(infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
  return 1;
}



