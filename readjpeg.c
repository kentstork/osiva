
///////////////////////////////////////////////////////////////////////
//
// readjpeg.c
//
// Based on the example from the jpeg6b library: example.c
//
// Contains the single public function:
//
//   LLIMG *read_jpeg_file (char * filename)
//
//   Returns a pointer to a malloc'd Hybrid_Image
//   The caller takes ownership of the LLIMG.
//   The LLIMG may be free'd by the caller with llimg_release_llimg(LLIMG *)
//
///////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <setjmp.h>
#include "jpeglib.h"

/* The required jpeg6b library headers are: */
/*   jpeglib.h */
/*   jconfig.h */
/*   jmorecfg.h */

#include <stdlib.h>
#include <string.h>
#include "ll_image.h"

#include <windows.h>


///////////////////////////////////////////////////////////////////////
//


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



///////////////////////////////////////////////////////////////////////
//


LLIMG * read_jpeg_file ( char * filename ) {
  
  struct jpeg_decompress_struct cinfo;
  // struct jpeg_error_mgr jerr;
  struct my_error_mgr jerr;
 
  
  FILE * infile;		/* source file */
  int row_stride;		/* physical row width in output buffer */
  
  LLIMG *llimg = NULL;
  int line_bytes, y, x;
  unsigned char *dest_array[1], *cp0, *cp2, temp;
  
  
  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return NULL;
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
    return NULL;
  }
 
  /* ... */

  // cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  
  /* Step 2: specify data source (eg, a file) */
  
  jpeg_stdio_src(&cinfo, infile);
  
  /* Step 3: read file parameters with jpeg_read_header() */
  
  (void) jpeg_read_header(&cinfo, TRUE);
  
  /* Step 4: set parameters for decompression */
  
  /* Step 5: Start decompressor */
  
  (void) jpeg_start_decompress(&cinfo);
   
  /* We may need to do some setup of our own at this point before reading
  * the data.  After jpeg_start_decompress() we have the correct scaled
  * output image dimensions available, as well as the output colormap
  * if we asked for color quantization.
  * In this example, we need to make an output work buffer of the right size.
  */ 
  
  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;  
  
  line_bytes = 4*((row_stride+3)/4);  /* DIB requires long alignment */
  llimg = llimg_create_base();
  llimg->width = cinfo.output_width;
  llimg->height = cinfo.output_height;
  llimg->dib_height = -llimg->height;
  llimg->bits_per_pixel = cinfo.output_components * 8;
  
  llimg->data = malloc(llimg->height * line_bytes);
  if ( !llimg->data ) return NULL;
  
  llimg->line = (unsigned char **)                    
    malloc (llimg->height * sizeof (unsigned char *));
  llimg->line[0] = (llimg)->data;                       
  for (y = 1; y < llimg->height; y++)  
    llimg->line[y] = llimg->line[y - 1] + line_bytes;
    
  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */
  
  /* Here we use the library's state variable cinfo.output_scanline as the
  * loop counter, so that we don't have to keep track ourselves.
  */
  y = 0;
  while (cinfo.output_scanline < cinfo.output_height) {
  /* jpeg_read_scanlines expects an array of pointers to scanlines.
  * Here the array is only one element long, but you could ask for
  * more than one scanline at a time if that's more convenient.
    */
    dest_array[0] = llimg->line[y++];
    (void) jpeg_read_scanlines(&cinfo, dest_array, 1);

    if (cinfo.output_components != 3)
      continue;

    /* Microsoft DIB format stores the color data as BGR instead of RGB */

    cp0 = dest_array[0];
    cp2 = cp0 + 2;
    for ( x=0; x < llimg->width; x++){
      temp = *cp0;
      *cp0 = *cp2;
      *cp2 = temp;
      cp0 += 3;
      cp2 += 3;
    }

  }
  
  /* Step 7: Finish decompression */
  
  (void) jpeg_finish_decompress(&cinfo);
  
  /* Step 8: Release JPEG decompression object */
  
  jpeg_destroy_decompress(&cinfo);
  
  fclose(infile);

  /* Create a gray scale color table if it's a grayscale image */

  if (llimg->bits_per_pixel == 8 ) {
    int c;
    for (c = 0; c < 256; c++) {
      llimg->color[c].blue = c;
      llimg->color[c].green = c;
      llimg->color[c].red = c;
    }
  }
  
  return llimg;
}



