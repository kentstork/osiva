/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * resizer.cpp is part of Osiva.
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

///////////////////////////////////////////////////////////////////////
//
// resizer.cpp
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ll_image.h"

///////////////////////////////////////////////////////////////////////
//
//

// 10 --> 6
// iterations = 60
// step 10
// pix = i / 6
// accum lost, bring pix up
// wt = i % 6 / 6

// 5 -->3 example:

// >==>==>==>==>==> 5 in
// >>>>>>>>>>>>>>>> 15 its
// >====>====>====> 3 out

/***
i_in = 0;
i_out = 0;
n = 0;
for ( i = 5; i <= 15; i+=5) {
  // find split
  i_split = i / 3;
  // accumulate left side
  for (;i_in < i_split; i_in++) {
    out[i_out] += in[i_in];
    n += 3;
  }
  // distribute pixel across split
  // ..left side of split
  wt = i % 3;
  left = (wt * in[i_split]) / 3;
  out[i_out] += left;
  n += wt;
  out[i_out] = (3 * out[i_out])/n;
  // ..right side of split
  right = in[i_split] - left;
  i_out++;
  out[i_out] = right;
  n = 3 - wt;
}
***/


int 
llimg_narrow24bit (LLIMG *image, int new_width, LLIMG *reduced)
{
  unsigned char *rp, *ip;
  int y;
  
  if (image->bits_per_pixel != 24)
    return (-1);
  
  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 24;
  
  reduced->width = new_width - 1;
  reduced->height = abs (image->height);
  reduced->dib_height = -reduced->height;
  
  int line_bytes = 4*(((3 * reduced->width)+3)/4); /* BGR, long aligned*/
  int reducedSize = line_bytes * abs (reduced->height);
  reduced->data = (unsigned char *) malloc (reducedSize);
  
  reduced->line = (unsigned char **)                    
    malloc (reduced->height * sizeof (unsigned char *));
  reduced->line[0] = (reduced)->data;                       
  for (y = 1; y < reduced->height; y++)  
    reduced->line[y] = reduced->line[y - 1] + line_bytes;
  
  long *reducedDataRed = new long[new_width];
  long *reducedDataBlue = new long[new_width];
  long *reducedDataGreen = new long[new_width];
  
  long *rpLongBlue, *rpLongGreen, *rpLongRed;

  int i_max = image->width * new_width;
  int i_inc = image->width;
  int div = new_width;
  
  for (y = 0; y < image->height; y ++)
  {
    memset (reducedDataRed, 0, new_width * sizeof (long));
    memset (reducedDataBlue, 0, new_width * sizeof (long));
    memset (reducedDataGreen, 0, new_width * sizeof (long));
    
    ip = image->line[y];
    rpLongRed = reducedDataRed;
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;

    int i;
    int i_split;
    int i_in = 0;
    int i_out = 0;
    int n = 0;
    int wt, left, right;

    for ( i = i_inc; i < i_max; i += i_inc ) {
      // find split
      i_split = i/div;
      // accumulate left side
      for (; i_in < i_split; i_in++ ) {
        *rpLongBlue += *ip++;
        *rpLongGreen += *ip++;
        *rpLongRed += *ip++;
        n += div;
      }
      // distribute pixel across split
      wt = i % div;
      n += wt;
      // ...blue
      left = (wt * (*ip)) / div;
      right = *ip++ - left;
      *rpLongBlue += left;
      *rpLongBlue = (div * *rpLongBlue)/n;
      rpLongBlue++;
      *rpLongBlue = right;
      // ....green
      left = (wt * (*ip)) / div;
      right = *ip++ - left;
      *rpLongGreen += left;
      *rpLongGreen = (div * *rpLongGreen)/n;
      rpLongGreen++;
      *rpLongGreen = right;
      // ....red
      left = (wt * (*ip)) / div;
      right = *ip++ - left;
      *rpLongRed += left;
      *rpLongRed = (div * *rpLongRed)/n;
      rpLongRed++;
      *rpLongRed = right;
      // ...
      n = div - wt;
      i_in++;
    }
    
    // now move the long values into the image.
    rp = reduced->line[y];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (i = 0; i < reduced->width; i++)
    {
      *rp++ = (unsigned char)((*rpLongBlue++));
      *rp++ = (unsigned char)((*rpLongGreen++));
      *rp++ = (unsigned char)((*rpLongRed++));
      
    }
    
  }
  
  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);
  
  return (0);
}



///////////////////////////////////////////////////////////////////////
//
//



int 
llimg_shorten24bit (LLIMG *image, int new_height, LLIMG *reduced)
{
  unsigned char *rp, *ip;
  int y;
  
  if (image->bits_per_pixel != 24)
    return (-1);
  
  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 24;
  
  reduced->width = image->width;
  reduced->height = new_height-1;
  reduced->dib_height = -reduced->height;
  
  int line_bytes = 4*(((3 * reduced->width)+3)/4); /* BGR, long aligned*/
  int reducedSize = line_bytes * abs (reduced->height);
  reduced->data = (unsigned char *) malloc (reducedSize);
  
  reduced->line = (unsigned char **)                    
    malloc (reduced->height * sizeof (unsigned char *));
  reduced->line[0] = (reduced)->data;                       
  for (y = 1; y < reduced->height; y++)  
    reduced->line[y] = reduced->line[y - 1] + line_bytes;
  
  long *reducedDataRed = new long[image->width];
  long *reducedDataBlue = new long[image->width];
  long *reducedDataGreen = new long[image->width];
  long *bottomBuff = new long[image->width * 3];
  
  long *rpLongBlue, *rpLongGreen, *rpLongRed, *lp;
  
  int i_max = image->height * new_height;
  int i_inc = image->height;
  int div = new_height;
  
  int i;
  int i_split;
  int i_in = 0;
  int i_out = 0;
  int n = 0;
  int wt, top, bottom;
  int x;
  
  memset (reducedDataRed, 0, image->width * sizeof (long));
  memset (reducedDataBlue, 0, image->width * sizeof (long));
  memset (reducedDataGreen, 0, image->width * sizeof (long));

  y = 0;
  int zeros = 0;

  for ( i = i_inc; i < i_max; i += i_inc ) {
    
    
    // find split
    i_split = i/div;    
    // accumulate tops
    for (; i_in < i_split; i_in++ ) {
      ip = image->line[i_in];
      rpLongRed = reducedDataRed;
      rpLongBlue = reducedDataBlue;
      rpLongGreen = reducedDataGreen;
      for (x = 0; x < image->width; x++) {
        *rpLongBlue++ += *ip++;
        *rpLongGreen++ += *ip++;
        *rpLongRed++ += *ip++;
      }
      n += div;
    }
    // distribute index pixel across split
    wt = i % div;    
    n += wt;
    ip = image->line[i_split];
    rpLongRed = reducedDataRed;
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    lp = bottomBuff;
    for (x = 0; x < image->width; x++ ) {
      // ...blue
      top = (wt * (*ip)) / div;
      bottom = *ip++ - top;
      *rpLongBlue += top;
      *rpLongBlue = (div * *rpLongBlue)/n;
      rpLongBlue++;
      *lp++ = bottom;
      // ....green
      top = (wt * (*ip)) / div;
      bottom = *ip++ - top;
      *rpLongGreen += top;
      *rpLongGreen = (div * *rpLongGreen)/n;
      rpLongGreen++;
      *lp++ = bottom;
      // ....red
      top = (wt * (*ip)) / div;
      bottom = *ip++ - top;
      *rpLongRed += top;
      *rpLongRed = (div * *rpLongRed)/n;
      rpLongRed++;
      *lp++ = bottom;
    }
    rp = reduced->line[y];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < reduced->width; x++)
    {
      *rp++ = (unsigned char)((*rpLongBlue++));
      *rp++ = (unsigned char)((*rpLongGreen++));
      *rp++ = (unsigned char)((*rpLongRed++));   
    }
    lp = bottomBuff;
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < reduced->width; x++)
    {
      *rpLongBlue++ = *lp++;
      *rpLongGreen++ = *lp++;
      *rpLongRed++ = *lp++;   
    }
    n = div - wt;
    i_in++;
    y++;
  }

  
  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);
  delete (bottomBuff);
  
  return (0);
}




///////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////
//
//



llimg_narrow8bit (LLIMG *image, int new_width, LLIMG *reduced)
{
  unsigned char *rp, *ip;
  int y;
  
  if (image->bits_per_pixel != 8)
    return (-1);
  
  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 24;
  
  reduced->width = new_width - 1;
  reduced->height = abs (image->height);
  reduced->dib_height = -reduced->height;
  
  int line_bytes = 4*(((reduced->width*3)+3)/4); /* BGR, long aligned*/
  int reducedSize = line_bytes * abs (reduced->height);
  reduced->data = (unsigned char *) malloc (reducedSize);
  
  reduced->line = (unsigned char **)                    
    malloc (reduced->height * sizeof (unsigned char *));
  reduced->line[0] = (reduced)->data;                       
  for (y = 1; y < reduced->height; y++)  
    reduced->line[y] = reduced->line[y - 1] + line_bytes;
  
  long *reducedDataRed = new long[new_width];
  long *reducedDataBlue = new long[new_width];
  long *reducedDataGreen = new long[new_width];
  
  long *rpLongBlue, *rpLongGreen, *rpLongRed;

  int i_max = image->width * new_width;
  int i_inc = image->width;
  int div = new_width;

  struct bgr_color *clr = image->color;
  unsigned char c;
  
  for (y = 0; y < image->height; y ++)
  {
    memset (reducedDataRed, 0, new_width * sizeof (long));
    memset (reducedDataBlue, 0, new_width * sizeof (long));
    memset (reducedDataGreen, 0, new_width * sizeof (long));
    
    ip = image->line[y];
    rpLongRed = reducedDataRed;
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;

    int i;
    int i_split;
    int i_in = 0;
    int i_out = 0;
    int n = 0;
    int wt, left, right;

    for ( i = i_inc; i < i_max; i += i_inc ) {
      // find split
      i_split = i/div;
      // accumulate left side
      for (; i_in < i_split; i_in++ ) {
        *rpLongBlue += clr[*ip].blue;
        *rpLongGreen += clr[*ip].green;
        *rpLongRed += clr[*ip].red;
        ip++;
        n += div;
      }
      // distribute pixel across split
      wt = i % div;
      n += wt;
      // ...blue
      c = clr[*ip].blue;
      left = (wt * c) / div;
      right = c - left;
      *rpLongBlue += left;
      *rpLongBlue = (div * *rpLongBlue)/n;
      rpLongBlue++;
      *rpLongBlue = right;
      // ....green
      c = clr[*ip].green;
      left = (wt * c) / div;
      right = c - left;
      *rpLongGreen += left;
      *rpLongGreen = (div * *rpLongGreen)/n;
      rpLongGreen++;
      *rpLongGreen = right;
      // ....red
      c = clr[*ip].red;
      left = (wt * c) / div;
      right = c - left;
      *rpLongRed += left;
      *rpLongRed = (div * *rpLongRed)/n;
      rpLongRed++;
      *rpLongRed = right;
      // ...
      ip++;
      n = div - wt;
      i_in++;
    }
    
    // now move the long values into the image.
    rp = reduced->line[y];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (i = 0; i < reduced->width; i++)
    {
      *rp++ = (unsigned char)((*rpLongBlue++));
      *rp++ = (unsigned char)((*rpLongGreen++));
      *rp++ = (unsigned char)((*rpLongRed++));
      
    }
    
  }
  
  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);
  
  return (0);
}


///////////////////////////////////////////////////////////////////////
//
//


int 
llimg_shorten8bit (LLIMG *image, int new_height, LLIMG *reduced)
{
  unsigned char *rp, *ip;
  int y;
  
  if (image->bits_per_pixel != 8)
    return (-1);
  
  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 24;
  
  reduced->width = image->width;
  reduced->height = new_height-1;
  reduced->dib_height = -reduced->height;
  
  int line_bytes = 4*(((3 * reduced->width)+3)/4); /* BGR, long aligned*/
  int reducedSize = line_bytes * abs (reduced->height);
  reduced->data = (unsigned char *) malloc (reducedSize);
  
  reduced->line = (unsigned char **)                    
    malloc (reduced->height * sizeof (unsigned char *));
  reduced->line[0] = (reduced)->data;                       
  for (y = 1; y < reduced->height; y++)  
    reduced->line[y] = reduced->line[y - 1] + line_bytes;
  
  long *reducedDataRed = new long[image->width];
  long *reducedDataBlue = new long[image->width];
  long *reducedDataGreen = new long[image->width];
  long *bottomBuff = new long[image->width * 3];
  
  long *rpLongBlue, *rpLongGreen, *rpLongRed, *lp;
  
  int i_max = image->height * new_height;
  int i_inc = image->height;
  int div = new_height;
  
  int i;
  int i_split;
  int i_in = 0;
  int i_out = 0;
  int n = 0;
  int wt, top, bottom;
  int x;
  
  memset (reducedDataRed, 0, image->width * sizeof (long));
  memset (reducedDataBlue, 0, image->width * sizeof (long));
  memset (reducedDataGreen, 0, image->width * sizeof (long));

  struct bgr_color *clr = image->color;
  unsigned char c;
  
  y = 0;
  int zeros = 0;

  for ( i = i_inc; i < i_max; i += i_inc ) {
    
    
    // find split
    i_split = i/div;    
    // accumulate tops
    for (; i_in < i_split; i_in++ ) {
      ip = image->line[i_in];
      rpLongRed = reducedDataRed;
      rpLongBlue = reducedDataBlue;
      rpLongGreen = reducedDataGreen;
      for (x = 0; x < image->width; x++) {
        *rpLongBlue++ += clr[*ip].blue;
        *rpLongGreen++ += clr[*ip].green;
        *rpLongRed++ += clr[*ip].red;
        ip++;
      }
      n += div;
    }
    // distribute index pixel across split
    wt = i % div;    
    n += wt;
    ip = image->line[i_split];
    rpLongRed = reducedDataRed;
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    lp = bottomBuff;
    for (x = 0; x < image->width; x++ ) {
      // ...blue
      c = clr[*ip].blue;
      top = (wt * c) / div;
      bottom = c - top;
      *rpLongBlue += top;
      *rpLongBlue = (div * *rpLongBlue)/n;
      rpLongBlue++;
      *lp++ = bottom;
      // ....green
      c = clr[*ip].green;
      top = (wt * c) / div;
      bottom = c - top;
      *rpLongGreen += top;
      *rpLongGreen = (div * *rpLongGreen)/n;
      rpLongGreen++;
      *lp++ = bottom;
      // ....red
      c = clr[*ip].red;
      top = (wt * c) / div;
      bottom = c - top;
      *rpLongRed += top;
      *rpLongRed = (div * *rpLongRed)/n;
      rpLongRed++;
      *lp++ = bottom;
      // ...
      ip++;
    }
    rp = reduced->line[y];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < reduced->width; x++)
    {
      *rp++ = (unsigned char)((*rpLongBlue++));
      *rp++ = (unsigned char)((*rpLongGreen++));
      *rp++ = (unsigned char)((*rpLongRed++));   
    }
    lp = bottomBuff;
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < reduced->width; x++)
    {
      *rpLongBlue++ = *lp++;
      *rpLongGreen++ = *lp++;
      *rpLongRed++ = *lp++;   
    }
    n = div - wt;
    i_in++;
    y++;
  }

  
  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);
  delete (bottomBuff);
  
  return (0);
}


///////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////
//
//


// >====>====>====> 3 in
// >>>>>>>>>>>>>>>> 15 its
// >==>==>==>==>==> 5 out

// >=====>=====>=====  3 in
// >>>>>>>>>>>>>>>>>> 18 its
// >==>==>==>==>==>==  6 out

/***
i_in = 0;
i_out = 0;
n = 0;
for ( i = 5; i <= 15; i+=5) {
  // find split
  i_split = i / 3;
  // distribute left side
  for (;i_out < i_split; i_out++) {
    out[i_out] = in[i_in];
    n += 3;
  }
  // distribute pixel across split
  // ..left side of split
  wt = i % 3;
  left = (wt * in[i_split]) / 3;
  out[i_out] = left;
  n += wt;
  out[i_out] = (3 * out[i_out])/n;
  // ..right side of split
  right = in[i_split] - left;
  out[i_out] += right;
  n = 3 - wt;
}
***/



int 
llimg_widen24bit (LLIMG *image, int new_width, LLIMG *reduced)
{
  unsigned char *rp, *ipb, *ipr, *ipg;
  int y;
  
  if (image->bits_per_pixel != 24)
    return (-1);
  
  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 24;
  
  int pad = new_width / image->width;
  reduced->width = new_width - pad;
  reduced->height = abs (image->height);
  reduced->dib_height = -reduced->height;
  
  int line_bytes = 4*(((3 * reduced->width)+3)/4); /* BGR, long aligned*/
  int reducedSize = line_bytes * abs (reduced->height);
  reduced->data = (unsigned char *) malloc (reducedSize);
  
  reduced->line = (unsigned char **)                    
    malloc (reduced->height * sizeof (unsigned char *));
  reduced->line[0] = (reduced)->data;                       
  for (y = 1; y < reduced->height; y++)  
    reduced->line[y] = reduced->line[y - 1] + line_bytes;
  
  long *reducedDataRed = new long[new_width];
  long *reducedDataBlue = new long[new_width];
  long *reducedDataGreen = new long[new_width];
  
  long *rpLongBlue, *rpLongGreen, *rpLongRed;

  int i_max = image->width * new_width;
  int i_inc = new_width;
  int div = image->width;
  
  for (y = 0; y < image->height; y ++)
  {    
    ipb = image->line[y];
    ipg = ipb + 1;
    ipr = ipg + 1;
    rpLongRed = reducedDataRed;
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;

    int i;
    int i_split;
    int i_out = 0;
    int n = 0;
    int wl, wr;

    for ( i = i_inc; i < i_max; i += i_inc ) {
      // find split
      i_split = i/div;
      // distribute left side
      for (; i_out < i_split; i_out++ ) {
        *rpLongBlue++ = *ipb;
        *rpLongGreen++ = *ipg;
        *rpLongRed++ = *ipr;
      }
      // accumulate pixels across split
      wl = i % div;
      wr = div - wl;
      // ...blue
      *rpLongBlue = wl * (*ipb);
      ipb += 3;
      *rpLongBlue += wr * (*ipb);
      *rpLongBlue++ /= div;
      // ....green
      *rpLongGreen = wl * (*ipg);
      ipg += 3;
      *rpLongGreen += wr * (*ipg);
      *rpLongGreen++ /= div;
      // ....red
      *rpLongRed = wl * (*ipr);
      ipr += 3;
      *rpLongRed += wr * (*ipr);
      *rpLongRed++ /= div;
      // ...
      i_out++;
    }
  
    
    // now move the long values into the image.
    rp = reduced->line[y];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (i = 0; i < reduced->width; i++)
    {
      *rp++ = (unsigned char)((*rpLongBlue++));
      *rp++ = (unsigned char)((*rpLongGreen++));
      *rp++ = (unsigned char)((*rpLongRed++));
      
    }
    
  }
  
  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);
  
  return (0);
}



///////////////////////////////////////////////////////////////////////
//
//


int 
llimg_heighten24bit (LLIMG *image, int new_height, LLIMG *reduced)
{
  unsigned char *rp, *ip;
  int y;
  
  if (image->bits_per_pixel != 24)
    return (-1);
  
  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 24;
  
  reduced->width = image->width;
  reduced->height = new_height-1;
  reduced->dib_height = -reduced->height;
  
  int line_bytes = 4*(((3 * reduced->width)+3)/4); /* BGR, long aligned*/
  int reducedSize = line_bytes * new_height;
  reduced->data = (unsigned char *) malloc (reducedSize);
  
  reduced->line = (unsigned char **)                    
    malloc (new_height * sizeof (unsigned char *));
  reduced->line[0] = (reduced)->data;                       
  for (y = 1; y < new_height; y++)  
    reduced->line[y] = reduced->line[y - 1] + line_bytes;
  
  long *reducedDataRed = new long[image->width];
  long *reducedDataBlue = new long[image->width];
  long *reducedDataGreen = new long[image->width];
  
  long *rpLongBlue, *rpLongGreen, *rpLongRed;
  
  int i_max = image->height * new_height;
  int i_inc = new_height;
  int div = image->height;
  
  int i;
  int i_split;
  int i_in = 0;
  int i_out = 0;
  int wt, wb;
  int x;
  
  memset (reducedDataRed, 0, image->width * sizeof (long));
  memset (reducedDataBlue, 0, image->width * sizeof (long));
  memset (reducedDataGreen, 0, image->width * sizeof (long));

  for ( i = i_inc; i < i_max; i += i_inc ) {
        
    // find split
    i_split = i/div;    
    // distribute tops
    for (; i_out < i_split; i_out++ ) {
      ip = image->line[i_in];
      rp = reduced->line[i_out];
      for (x = 0; x < image->width; x++) {
        *rp++ = *ip++;
        *rp++ = *ip++;
        *rp++ = *ip++;     
      }
    }
    // accumulate index pixel over split
    wt = i % div;   // weight of top contribution
    wb = div - wt;  // weight of bottom contribution
    ip = image->line[i_in];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < image->width; x++ ) {
      *rpLongBlue++ = wt * *ip++;
      *rpLongGreen++ = wt * *ip++;
      *rpLongRed++ = wt * *ip++;
    }
    i_in++;
    ip = image->line[i_in];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < image->width; x++ ) {
      *rpLongBlue += wb * *ip++;
      *rpLongBlue++ /= div;
      *rpLongGreen += wb * *ip++;
      *rpLongGreen++ /= div;
      *rpLongRed += wb * *ip++;
      *rpLongRed++ /= div;
    }
    rp = reduced->line[i_out];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < reduced->width; x++)
    {
      *rp++ = (unsigned char)((*rpLongBlue++));
      *rp++ = (unsigned char)((*rpLongGreen++));
      *rp++ = (unsigned char)((*rpLongRed++));   
    }
    i_out++;
  }

  
  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);
  
  return (0);
}




///////////////////////////////////////////////////////////////////////
//
//


int 
llimg_heighten8bit (LLIMG *image, int new_height, LLIMG *reduced)
{
  unsigned char *rp, *ip;
  int y;
  
  if (image->bits_per_pixel != 8)
    return (-1);
  
  llimg_zero_llimg (reduced);
  reduced->bits_per_pixel = 24;
  
  reduced->width = image->width-1;
  reduced->height = new_height-1;
  reduced->dib_height = -reduced->height;
  
  int line_bytes = 4*(((3 * reduced->width)+3)/4); /* BGR, long aligned*/
  int reducedSize = line_bytes * new_height;
  reduced->data = (unsigned char *) malloc (reducedSize);
  
  reduced->line = (unsigned char **)                    
    malloc (new_height * sizeof (unsigned char *));
  reduced->line[0] = (reduced)->data;                       
  for (y = 1; y < new_height; y++)  
    reduced->line[y] = reduced->line[y - 1] + line_bytes;
  
  long *reducedDataRed = new long[image->width];
  long *reducedDataBlue = new long[image->width];
  long *reducedDataGreen = new long[image->width];
  
  long *rpLongBlue, *rpLongGreen, *rpLongRed;
  
  int i_max = image->height * new_height;
  int i_inc = new_height;
  int div = image->height;
  
  int i;
  int i_split;
  int i_in = 0;
  int i_out = 0;
  int wt, wb;
  int x;
  
  memset (reducedDataRed, 0, image->width * sizeof (long));
  memset (reducedDataBlue, 0, image->width * sizeof (long));
  memset (reducedDataGreen, 0, image->width * sizeof (long));

  struct bgr_color *clr = image->color;

  for ( i = i_inc; i < i_max; i += i_inc ) {
        
    // find split
    i_split = i/div;    
    // distribute tops
    for (; i_out < i_split; i_out++ ) {
      ip = image->line[i_in];
      rp = reduced->line[i_out];
      for (x = 0; x < image->width; x++) {
        *rp++ = clr[*ip].blue;
        *rp++ = clr[*ip].green;
        *rp++ = clr[*ip].red;
        ip++;
      }
    }
    // accumulate index pixel over split
    wt = i % div;   // weight of top contribution
    wb = div - wt;  // weight of bottom contribution
    ip = image->line[i_in];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < image->width; x++ ) {
      *rpLongBlue++ = wt * clr[*ip].blue;
      *rpLongGreen++ = wt * clr[*ip].green;
      *rpLongRed++ = wt * clr[*ip].red;
      ip++;
    }
    i_in++;
    ip = image->line[i_in];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < image->width; x++ ) {
      *rpLongBlue += wb * clr[*ip].blue;
      *rpLongBlue++ /= div;
      *rpLongGreen += wb * clr[*ip].green;
      *rpLongGreen++ /= div;
      *rpLongRed += wb * clr[*ip].red;
      *rpLongRed++ /= div;
      ip++;
    }
    rp = reduced->line[i_out];
    rpLongBlue = reducedDataBlue;
    rpLongGreen = reducedDataGreen;
    rpLongRed = reducedDataRed;
    for (x = 0; x < reduced->width; x++)
    {
      *rp++ = (unsigned char)((*rpLongBlue++));
      *rp++ = (unsigned char)((*rpLongGreen++));
      *rp++ = (unsigned char)((*rpLongRed++));   
    }
    i_out++;
  }

  
  delete (reducedDataRed);
  delete (reducedDataBlue);
  delete (reducedDataGreen);
  
  return (0);
}


///////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////
//
//


LLIMG * llimg_resize (LLIMG *image, int width, int height) {
  if (!image)
    return NULL;
  LLIMG *resized = (LLIMG *) malloc(sizeof(LLIMG));
  if (!resized)
    return NULL;
  llimg_zero_llimg (resized);
  LLIMG *llimg_temp = (LLIMG *) malloc(sizeof(LLIMG));
  if (!llimg_temp)
    return NULL;
  llimg_zero_llimg (llimg_temp);

  if (height < image->height) {
    if (image->bits_per_pixel == 8)
      llimg_shorten8bit (image, height, llimg_temp);
    else
      llimg_shorten24bit (image, height, llimg_temp);
  }
  else {
    if (image->bits_per_pixel == 8)
      llimg_heighten8bit (image, height, llimg_temp);
    else
      llimg_heighten24bit (image, height, llimg_temp);
  }
  if (width < image->width) {
    llimg_narrow24bit (llimg_temp, width, resized);
  }
  else {
    llimg_widen24bit (llimg_temp, width, resized);
  }
  
  llimg_release_llimg (llimg_temp);

  return resized;
}

///////////////////////////////////////////////////////////////////////
//
//

void llimg_lock_aspect (LLIMG *image, int &width, int &height){
  
  // h/ih = w/iw
  // w = (iw * h)/ih
  // h = (ih * w)/iw
  if (!image)
    return;
  int lock_width = (image->width * height)/image->height;
  int lock_height = (image->height * width)/image->width; 
  if (lock_width < width) {
    width = lock_width;    
  }
  else {
    height = lock_height;
  }
}

