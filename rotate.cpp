/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * rotate.cpp is part of Osiva.
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
// File: rotate.cpp
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ll_image.h"

/////////////////////////////////////////////////////////////////////////////
//

int 
llimg_rotate24bitR (LLIMG *image, LLIMG *rotated)
{
  int y;
  
  if (image->bits_per_pixel != 24)
    return (-1);
  
  llimg_zero_llimg (rotated);
  rotated->bits_per_pixel = 24;
  
  rotated->width = image->height;
  rotated->height = image->width;
  rotated->dib_height = -rotated->height;
  
  int line_bytes = 4*(((3 * rotated->width)+3)/4); /* BGR, long aligned*/
  int rotatedSize = line_bytes * abs (rotated->height);
  rotated->data = (unsigned char *) malloc (rotatedSize);
  
  rotated->line = (unsigned char **)                    
    malloc (rotated->height * sizeof (unsigned char *));
  rotated->line[0] = rotated->data;                       
  for (y = 1; y < rotated->height; y++)  
    rotated->line[y] = rotated->line[y - 1] + line_bytes;
   

  unsigned char *ip, *rp;
  int x, ry;
  // The source image is read from top to bottom
  // The rotated image is filled from right to left
  int rot_offset = (rotated->width - 1) * 3;
  
  for (y = 0; y < image->height; y++ ) {
    ip = image->line[y];
    ry = 0;
    for (x = 0; x < image->width; x++) {
      rp = rotated->line[ry++] + rot_offset;
      *rp++ = *ip++;  // blue
      *rp++ = *ip++;  // green
      *rp++ = *ip++;  // red
    } // For each pixel in the source line
    rot_offset -= 3;
  } // For each line in the source image

  return (0);
}



/////////////////////////////////////////////////////////////////////////////
//

int 
llimg_rotate8bitR (LLIMG *image, LLIMG *rotated)
{
  int y;
  
  if (image->bits_per_pixel != 8)
    return (-1);
  
  llimg_zero_llimg (rotated);
  rotated->bits_per_pixel = 8;
  
  rotated->width = image->height;
  rotated->height = image->width;
  rotated->dib_height = -rotated->height;
  
  int line_bytes = 4*((rotated->width+3)/4); /* 8 bit, long aligned*/
  int rotatedSize = line_bytes * abs (rotated->height);
  rotated->data = (unsigned char *) malloc (rotatedSize);
  
  rotated->line = (unsigned char **)                    
    malloc (rotated->height * sizeof (unsigned char *));
  rotated->line[0] = rotated->data;                       
  for (y = 1; y < rotated->height; y++)  
    rotated->line[y] = rotated->line[y - 1] + line_bytes;
   

  unsigned char *ip, *rp;
  int x, ry;
  // The source image is read from top to bottom
  // The rotated image is filled from right to left
  int rot_offset = rotated->width - 1;
  
  for (y = 0; y < image->height; y++ ) {
    ip = image->line[y];
    ry = 0;
    for (x = 0; x < image->width; x++) {
      rp = rotated->line[ry++] + rot_offset;
      *rp = *ip++;
    } // For each pixel in the source line
    rot_offset --;
  } // For each line in the source image

  // Copy the color table
  memcpy (rotated->color, image->color, 256 * sizeof(struct bgr_color));


  return (0);
}




/////////////////////////////////////////////////////////////////////////////
//

int 
llimg_rotate24bitL (LLIMG *image, LLIMG *rotated)
{
  int y;
  
  if (image->bits_per_pixel != 24)
    return (-1);
  
  llimg_zero_llimg (rotated);
  rotated->bits_per_pixel = 24;
  
  rotated->width = image->height;
  rotated->height = image->width;
  rotated->dib_height = -rotated->height;
  
  int line_bytes = 4*(((3 * rotated->width)+3)/4); /* BGR, long aligned*/
  int rotatedSize = line_bytes * abs (rotated->height);
  rotated->data = (unsigned char *) malloc (rotatedSize);
  
  rotated->line = (unsigned char **)                    
    malloc (rotated->height * sizeof (unsigned char *));
  rotated->line[0] = rotated->data;                       
  for (y = 1; y < rotated->height; y++)  
    rotated->line[y] = rotated->line[y - 1] + line_bytes;
   

  unsigned char *ip, *rp;
  int x, ry;

  
  int rot_offset = 0;
  
  for (y = 0; y < image->height; y++ ) {
    ip = image->line[y];
    ry = rotated->height - 1;
    for (x = 0; x < image->width; x++) {
      rp = rotated->line[ry--] + rot_offset;
      *rp++ = *ip++;  // blue
      *rp++ = *ip++;  // green
      *rp++ = *ip++;  // red
    } // For each pixel in the source line
    rot_offset += 3;
  } // For each line in the source image

  return (0);
}



/////////////////////////////////////////////////////////////////////////////
//

int 
llimg_rotate8bitL (LLIMG *image, LLIMG *rotated)
{
  int y;
  
  if (image->bits_per_pixel != 8)
    return (-1);
  
  llimg_zero_llimg (rotated);
  rotated->bits_per_pixel = 8;
  
  rotated->width = image->height;
  rotated->height = image->width;
  rotated->dib_height = -rotated->height;
  
  int line_bytes = 4*((rotated->width+3)/4); /* 8 bit, long aligned*/
  int rotatedSize = line_bytes * abs (rotated->height);
  rotated->data = (unsigned char *) malloc (rotatedSize);
  
  rotated->line = (unsigned char **)                    
    malloc (rotated->height * sizeof (unsigned char *));
  rotated->line[0] = rotated->data;                       
  for (y = 1; y < rotated->height; y++)  
    rotated->line[y] = rotated->line[y - 1] + line_bytes;
   

  unsigned char *ip, *rp;
  int x, ry;

  int rot_offset = 0;
  
  for (y = 0; y < image->height; y++ ) {
    ip = image->line[y];
    ry = rotated->height - 1;
    for (x = 0; x < image->width; x++) {
      rp = rotated->line[ry--] + rot_offset;
      *rp = *ip++;
    } // For each pixel in the source line
    rot_offset ++;
  } // For each line in the source image

  // Copy the color table
  memcpy (rotated->color, image->color, 256 * sizeof(struct bgr_color));


  return (0);
}

