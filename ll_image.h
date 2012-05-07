/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * ll_image.h is part of Osiva.
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


/*/////////////////////////////////////////////////////////////////////////
//
// file: ll_image.h
//
// synopsis:
//
//  #include <stdlib.h>
//  #include <string.h>
//  #include <ll_image.h>
//
//////////////////////////////////////////////////////////////////////// */


#ifndef LL_IMAGE_H
#define LL_IMAGE_H


struct bgr_color { unsigned char blue, green, red, reserved ; } ;

typedef struct LLIMG_s {

    unsigned char * data ;     /* the raster data array -  malloc'd */
    unsigned char **line ;     /* an array of pointers to rows -  malloc'd */
    void * log_palette ;       /* holder for the windows LOGICALPALETTE */
    void * hpalette ;          /* holder for the windows HPALETTE */
    long height ;              /* normal, positive height (vs. dib_height) */
    unsigned char rotation ;   /* times to rot ClockWise; 3 => counter CW */
    long client;
    long client1;

    /* ... */

    long bih_top ;                  /* 40, sizeof(BITMAPINFOHEADER) */
    long width ;                    /* must be long aligned 4*((w+3)/4) */
    long dib_height ;               /* -height (must be + for clipboard) */
    unsigned short planes ;         /* always 1 */
    unsigned short bits_per_pixel ; /* 1, 4, 8, 16, 24, 32 are legal */
    long compression ;              /* usually 0: win32 BI_RGB */
    long sizeimage ;                /* width * height -- 0 works for BI_RGBs */
    long xpelsperm ;                /* x resolution   -- 0 works */
    long ypelsperm ;                /* y resolution   -- 0 works */
    long clrused ;                  /* number of colors used -- 0 works */
    long clrimportant ;             /*  -- 0 works */
    struct bgr_color color[256] ;   /* [1] under win32 */

} LLIMG ;



/* * * * * * * * * * * * * * * * * * * * * * */
/* frees pointer content for stack based Images */


#define llimg_prune_llimg(llimg)                  \
{                                                 \
  do {                                            \
    if (!(llimg))                                 \
      break;                                      \
    if ((llimg)->line)                            \
      free ((void *) ((llimg)->line));            \
    if ((llimg)->data)                            \
      free ((void *) ((llimg)->data));            \
  } while (0);                                    \
}

/* * * * * * * * * * * * * * * * * * * * * * */
/* completely frees a heap based Image */


#define llimg_release_llimg(llimg)                \
{                                                 \
  do {                                            \
    if (!(llimg))                                 \
      break;                                      \
    if ((llimg)->line)                            \
      free ((void *) ((llimg)->line));            \
    if ((llimg)->data)                            \
      free ((void *) ((llimg)->data));            \
    free ((void *) (llimg));                      \
  } while (0);                                    \
}

/* * * * * * * * * * * * * * * * * * * * * * */


#define llimg_zero_llimg(llimg)                    \
{                                                  \
  if (llimg) {                                     \
    memset ((void *) (llimg), 0, sizeof (LLIMG));  \
    (llimg)->bih_top = 40;                         \
    (llimg)->planes = 1;                           \
  }                                                \
}

/* * * * * * * * * * * * * * * * * * * * * * */
/* creates and fills the line array */


#define llimg_make_line_array(llimg)                             \
{                                                                \
  int line_bytes, y;                                             \
  do {                                                           \
    if (!(llimg))                                                \
      break;                                                     \
    line_bytes = ((llimg)->width * (llimg)->bits_per_pixel) / 8; \
    if (line_bytes <= 0)                                         \
      break;                                                     \
    (llimg)->line = (unsigned char **)                           \
      malloc ((llimg)->height * sizeof (unsigned char *));       \
    (llimg)->line[0] = (llimg)->data;                            \
    for (y = 1; y < (llimg)->height; y++)                        \
      (llimg)->line[y] = (llimg)->line[y - 1] + line_bytes;      \
  } while (0);                                                   \
}

/* * * * * * * * * * * * * * * * * * * * * * */

static LLIMG * llimg_create_base () { LLIMG *llimg; llimg = (LLIMG *) malloc (sizeof (LLIMG)); llimg_zero_llimg (llimg); return (llimg); }

#endif




