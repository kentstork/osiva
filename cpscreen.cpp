/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * cpscreen.cpp is part of Osiva.
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
// File: cpscreen.cpp
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "ll_image.h"

///////////////////////////////////////////////////////////////////////////

static void
showLastSysError( char * mess ) {
  char str[128] ;
  DWORD err = GetLastError();
  FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, 0, err,
    0, str, 128, 0 ) ;
  MessageBox( 0, str, mess, 0 ) ;
}

/////////////////////////////////////////////////////////////////////////////
//

static 
HBITMAP CopyScreenToBitmap(const RECT *lpRect)
{
  HDC hScrDC, hMemDC;		// screen DC and memory DC     
  int nX, nY, nX2, nY2;		// coordinates of rectangle to grab     
  int nWidth, nHeight;		// DIB width and height     
  int xScrn, yScrn;		// screen resolution      

  HGDIOBJ hOldBitmap, hBitmap;

  // check for an empty rectangle 
  if (IsRectEmpty (lpRect))
    return NULL;
  // create a DC for the screen and create     
  // a memory DC compatible to screen DC          

  hScrDC = CreateDC ("DISPLAY", NULL, NULL, NULL);
  hMemDC = CreateCompatibleDC (hScrDC);	// get points of rectangle to grab  

  nX = lpRect->left;
  nY = lpRect->top;
  nX2 = lpRect->right;
  nY2 = lpRect->bottom;		// get screen resolution      

  xScrn = GetDeviceCaps (hScrDC, HORZRES);
  yScrn = GetDeviceCaps (hScrDC, VERTRES);

  //make sure bitmap rectangle is visible      

  /*
  if (nX < 0)
    nX = 0;

  if (nY < 0)
    nY = 0;

  if (nX2 > xScrn)
    nX2 = xScrn;

  if (nY2 > yScrn)
    nY2 = yScrn;
  */
  

  nWidth = nX2 - nX;
  nHeight = nY2 - nY;


  // create a bitmap compatible with the screen DC     

  hBitmap = CreateCompatibleBitmap (hScrDC, nWidth, nHeight);

  // select new bitmap into memory DC     

  hOldBitmap = SelectObject (hMemDC, hBitmap);

  // bitblt screen DC to memory DC     

  int success = BitBlt (hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY);

  // select old bitmap back into memory DC and get handle to     
  // bitmap of the screen          

  hBitmap = SelectObject (hMemDC, hOldBitmap);

  // clean up      

  DeleteDC (hScrDC);
  DeleteDC (hMemDC);

  // return handle to the bitmap      

  return (HBITMAP) hBitmap;

}


/////////////////////////////////////////////////////////////////////////////
//

LLIMG *llimg_cpscreen (const RECT *source) {
  LLIMG *llimg = NULL;
  
  HBITMAP hbm_screen = CopyScreenToBitmap(source);

  int success;

  // Get the basic information about the bitmap

  BITMAPINFO bmi;
  memset (&bmi, 0, sizeof(BITMAPINFO));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  HDC hdc = GetDC (NULL);
  success = GetDIBits (hdc, (HBITMAP) hbm_screen,
    0, 0, NULL, &bmi, DIB_RGB_COLORS);
  
  if (!success) {
    showLastSysError ("GetDIBits, info request");
    return NULL;
  }

  // Convert it to 24 bit open BGR
  
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biBitCount = 24;

  int line_bytes = 4*(((3 * bmi.bmiHeader.biWidth)+3)/4); /* BGR, long aligned*/
  int bgr_size = line_bytes * bmi.bmiHeader.biHeight;

  unsigned char *data = (unsigned char *) malloc (bgr_size);
  if (!data) {
    DeleteObject (hbm_screen);
    return NULL;
  }  
  success = GetDIBits (hdc, (HBITMAP) hbm_screen,
    0, bmi.bmiHeader.biHeight, data, &bmi, DIB_RGB_COLORS);

  if (!success) {
    showLastSysError ("GetDIBits, data request");
    return NULL;
  }

  // Move it into an llimg  

  llimg = llimg_create_base ();
  llimg->width = bmi.bmiHeader.biWidth;
  llimg->height = bmi.bmiHeader.biHeight;
  llimg->dib_height = bmi.bmiHeader.biHeight;
  llimg->bits_per_pixel = bmi.bmiHeader.biBitCount;
  llimg->data = data;
  llimg->line = (unsigned char **)                    
    malloc (llimg->height * sizeof (unsigned char *));
  llimg->line[0] = llimg->data;                       
  for (int y = 1; y < llimg->height; y++)  
    llimg->line[y] = llimg->line[y - 1] + line_bytes;

  
  DeleteObject (hbm_screen);

  // Flip the image to make it raster instead of cartesian

  if (llimg->height < 2)
    return NULL;
  unsigned char *save = (unsigned char *) malloc (line_bytes);
  if (!save)
    return (NULL);
  int yt, yb;
  for (yt = 0, yb = llimg->height - 1; yt < yb; yt++, yb--)
    {
      memcpy (save, llimg->line[yt], line_bytes);
      memcpy (llimg->line[yt], llimg->line[yb], line_bytes);
      memcpy (llimg->line[yb], save, line_bytes);
    }
  free (save);

  llimg->dib_height = -llimg->height;


  return llimg;
}



