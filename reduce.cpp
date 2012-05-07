/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * reduce.cpp is part of Osiva.
 *
 * Osiva is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Osiva is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Osiva.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/


/////////////////////////////////////////////////////////////////////////////
//
// File: reduce.cpp
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ll_image.h"

/////////////////////////////////////////////////////////////////////////////
//

// Desc: 8 bit "true color" definitions: 3 red, 3 green, 2 blue

/* 
    We want to maintain full color strength when
    extracting the (R, G, B) colors from the posterized
    8 bit pixel values.  Just doing the shifts will
    leave zeros in the LS bits, so, for instance, white
    will not be fully white.  OR'ing 1's in the LS is
    equally a problem, since there will always be a
    little of the other two colors in any primary.  So
    we do a conditional OR.
*/


static struct bgr_color 
pix2bgr (unsigned char pix)
{
  struct bgr_color col;
  col.red = pix;
  if (col.red & 0x80)
    col.red |= 0x1f;
  col.green = pix << 3;
  if (col.green & 0x80)
    col.green |= 0x1f;
  col.blue = pix << 6;
  if (col.blue & 0x80)
    col.blue |= 0x3f;
  col.reserved = 0;
  return col;
}

static unsigned char 
rgb2pix (unsigned char r, unsigned char g, unsigned char b)
{
  return ((r & 0xe0) | ((g & 0xe0) >> 3) | (b >> 6));
}


/////////////////////////////////////////////////////////////////////////////
//

int 
llimg_reduce256 (LLIMG *image, int reduction, LLIMG *reduced)
{
  unsigned char *rp, *ip;
  int i, y, x, y1;
  
  if (image->bits_per_pixel != 8)
    return (-1);
  
  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 24;
  
  reduced->width = image->width / reduction;
  reduced->height = abs (image->height) / reduction;
  reduced->dib_height = -reduced->height;
  
  int line_bytes = 4*(((3 * reduced->width)+3)/4); /* BGR, long aligned*/
  int reducedSize = line_bytes * abs (reduced->height);
  reduced->data = (unsigned char *) malloc (reducedSize);
  
  reduced->line = (unsigned char **)                    
    malloc (reduced->height * sizeof (unsigned char *));
  reduced->line[0] = (reduced)->data;                       
  for (y = 1; y < reduced->height; y++)  
    reduced->line[y] = reduced->line[y - 1] + line_bytes;
  
  int area = reduction * reduction;
  
  long *reducedDataBlue = new long[reduced->width];
  long *reducedDataGreen = new long[reduced->width];
  long *reducedDataRed = new long[reduced->width];

  long *rpLongBlue, *rpLongGreen, *rpLongRed;
  
  int max_y = reduced->height * reduction;
  for (y = 0; y < max_y; y += reduction)
  {
    memset (reducedDataBlue, 0, reduced->width * sizeof (long));
    memset (reducedDataGreen, 0, reduced->width * sizeof (long));
    memset (reducedDataRed, 0, reduced->width * sizeof (long));
    
    for (y1 = y; y1 < (y + reduction) && y1 < image->height; y1++)
    {
      ip = image->line[y1];
      rpLongRed = reducedDataRed;
      rpLongBlue = reducedDataBlue;
      rpLongGreen = reducedDataGreen;
      
      int counter = 0;
      for (x = 0; x < reduced->width; x++)
      {
        for (i = 0; i < reduction; i++)
        {
          *rpLongBlue += image->color[*ip].blue;
          *rpLongGreen += image->color[*ip].green;
          *rpLongRed += image->color[*ip].red;
          ip++;
        }
        rpLongRed++;
        rpLongBlue++;
        rpLongGreen++;
      }
      
    }
    
    // now move the long values into the image.
    rp = reduced->line[y/reduction];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (i = 0; i < (reduced->width); i++)
    {
      *rp++ = (unsigned char)((*rpLongBlue++)/area);
      *rp++ = (unsigned char)((*rpLongGreen++)/area);
      *rp++ = (unsigned char)((*rpLongRed++)/area);
    }
    
  }
  
  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);

  return 0; 
}


/////////////////////////////////////////////////////////////////////////////
//

int 
llimg_reduce24bit (LLIMG *image, int reduction, LLIMG *reduced)
{
  unsigned char *rp, *ip;
  int i, y, x, y1;
  
  if (image->bits_per_pixel != 24)
    return (-1);
  
  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 24;
  
  reduced->width = image->width / reduction;
  reduced->height = abs (image->height) / reduction;
  reduced->dib_height = -reduced->height;
  
  int line_bytes = 4*(((3 * reduced->width)+3)/4); /* BGR, long aligned*/
  int reducedSize = line_bytes * abs (reduced->height);
  reduced->data = (unsigned char *) malloc (reducedSize);
  
  reduced->line = (unsigned char **)                    
    malloc (reduced->height * sizeof (unsigned char *));
  reduced->line[0] = (reduced)->data;                       
  for (y = 1; y < reduced->height; y++)  
    reduced->line[y] = reduced->line[y - 1] + line_bytes;
  
  int area = reduction * reduction;
  
  long *reducedDataRed = new long[reduced->width];
  long *reducedDataBlue = new long[reduced->width];
  long *reducedDataGreen = new long[reduced->width];
  
  long *rpLongBlue, *rpLongGreen, *rpLongRed;

  int total_lines = reduced->height * reduction;
  for (y = 0; y < total_lines; y += reduction)
  {
    memset (reducedDataRed, 0, reduced->width * sizeof (long));
    memset (reducedDataBlue, 0, reduced->width * sizeof (long));
    memset (reducedDataGreen, 0, reduced->width * sizeof (long));
    
    for (y1 = y; y1 < (y + reduction) && y1 < image->height; y1++)
    {
      ip = image->line[y1];
      rpLongRed = reducedDataRed;
      rpLongBlue = reducedDataBlue;
      rpLongGreen = reducedDataGreen;
      
      int counter = 0;
      for (x = 0; x < reduced->width; x ++)
      {
        for (i = 0; i < reduction; i++)
        {
          *rpLongBlue += *ip++;
          *rpLongGreen += *ip++;
          *rpLongRed += *ip++;
        }
        rpLongBlue++;
        rpLongGreen++;
        rpLongRed++;
      }
    }
    
    // now move the long values into the image.
    rp = reduced->line[y/reduction];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (i = 0; i < reduced->width; i++)
    {
      *rp++ = (unsigned char)((*rpLongBlue++)/area);
      *rp++ = (unsigned char)((*rpLongGreen++)/area);
      *rp++ = (unsigned char)((*rpLongRed++)/area);
    }
  }
  
  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);
  
  return (0);
}

/////////////////////////////////////////////////////////////////////////////
//

int 
llimg_reduce256_256 (LLIMG *image, int reduction, LLIMG *reduced)
{
  unsigned char *rp, *ip;
  int i, y, x, y1;

  if (image->bits_per_pixel != 8)
    return (-1);

  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 8;

  reduced->width = (image->width + (reduction - 1)) / reduction;
  reduced->width = ((reduced->width + 3) / 4) * 4;	/* Long align */
  reduced->height = (abs (image->height) + (reduction - 1)) / reduction;
  reduced->dib_height = -reduced->height;

  int reducedSize = reduced->width * abs (reduced->height);
  reduced->data = (unsigned char *) malloc (reducedSize);
  llimg_make_line_array (reduced);

  int area = reduction * reduction;

  long *reducedDataRed = new long[reduced->width];
  long *reducedDataBlue = new long[reduced->width];
  long *reducedDataGreen = new long[reduced->width];

  for (y = 0; y < abs (image->height); y += reduction)
    {
      memset (reducedDataRed, 0, reduced->width * sizeof (long));
      memset (reducedDataBlue, 0, reduced->width * sizeof (long));
      memset (reducedDataGreen, 0, reduced->width * sizeof (long));

      for (y1 = y; y1 < (y + reduction) && y1 < image->height; y1++)
	{
	  ip = image->line[y1];
	  long *rpLongRed = reducedDataRed;
	  long *rpLongBlue = reducedDataBlue;
	  long *rpLongGreen = reducedDataGreen;

	  int counter = 0;
	  for (x = 0; x < image->width - reduction; x += reduction)
	    {
	      ip += reduction;

	      for (i = 0; i < reduction; i++)
		{
		  *rpLongRed += image->color[ip[i]].red;
		  *rpLongBlue += image->color[ip[i]].blue;
		  *rpLongGreen += image->color[ip[i]].green;
		}
	      counter++;
	      rpLongRed++;
	      rpLongBlue++;
	      rpLongGreen++;
	    }

	  for (i = counter; i < reduced->width; i++)
	    {
	      // white out the extra
	      *rpLongRed += 0xff * reduction;
	      *rpLongBlue += 0xff * reduction;
	      *rpLongGreen += 0xff * reduction;
	      rpLongRed++;
	      rpLongBlue++;
	      rpLongGreen++;
	    }
	}

      // now move the long values into the image.
      rp = &reduced->data[((y / reduction) * reduced->width)];
      for (i = 0; i < (reduced->width); i++)
	{
	  unsigned char red = 0;
	  unsigned char blue = 0;
	  unsigned char green = 0;
	  red = (unsigned char) (reducedDataRed[i] / area);
	  blue = (unsigned char) (reducedDataBlue[i] / area);
	  green = (unsigned char) (reducedDataGreen[i] / area);
	  rp[i] = rgb2pix (red, green, blue);
	}
    }

  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);

  for (y = 0; y < 256; y++)
    {
      reduced->color[y] = pix2bgr ((unsigned char) y);
    }
  return (0);
}


/////////////////////////////////////////////////////////////////////////////
//
// llimg_dub 
//
// Dubs the input image, but converts it to BGR if it is color tabled

LLIMG *llimg_dub (LLIMG *img) {
  
  LLIMG *dubimg;
  dubimg = (LLIMG *) malloc (sizeof (LLIMG));
  llimg_zero_llimg (dubimg);
  dubimg->width = img->width;
  dubimg->height = img->height;
  dubimg->bits_per_pixel = 24;
  dubimg->dib_height = - dubimg->height;
  int linebytes = 3 * img->width;
  linebytes = 4 * ((linebytes + 3)/4);
  dubimg->data = (unsigned char *)
	  malloc (dubimg->height * linebytes);

  dubimg->line = (unsigned char **)
	  malloc (dubimg->height * sizeof (unsigned char *));
  dubimg->line[0] = dubimg->data;
  for (int l = 1; l < dubimg->height; l++)
	  dubimg->line[l] = dubimg->line[l-1] + linebytes;

  unsigned char *ip, *dp;
  int x;
  for (int y = 0; y < img->height; y++) {
	  switch (img->bits_per_pixel) {
	  case 8:
		  ip = img->line[y];
		  dp = dubimg->line[y];
		  for (x = 0; x < img->width; x++ ) {
        *dp++ = img->color[*ip].blue;
			  *dp++ = img->color[*ip].green;
			  *dp++ = img->color[*ip].red;
			  ip++;
		  }
		  break;
	  case 24:
		  memcpy (dubimg->line[y], img->line[y], linebytes);
		  break;
	  default:
		  // Default to repeating gradient on bad input
		  memset (dubimg->line[y], y%256, linebytes);
		  break;
	  } // switch on bits_per_pixel
  } // for each line y

  return dubimg;
}


