/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * wregion.cpp is part of Osiva.
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


///////////////////////////////////////////////////////////////////////////
//
// File: wregion.cpp
//
// Interface to the region detection routines in contour.cpp
// For making transparent windows using the Windows API
//
///////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ll_image.h"
#include "contour.h"
#include <windows.h>
#include "wregion.h"

#include <vector>
using namespace std;
#include "ooptions.h"

///////////////////////////////////////////////////////////////////////////
// Global Scope

extern OOptions *ooptions;

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

void 
oimg_dialate_index (LLIMG * image, int index)
{

  int x, y;
  unsigned char *cp, *cp0, *cp1, *qp;
  unsigned char **Q;            /* space for neighborhood operation results */
  int Qs = 2;                   /* we're just going to do 3x3's */
  int q;

  Q = (unsigned char **) malloc (Qs * sizeof (unsigned char *));
  for (q = 0; q < Qs; q++)
    Q[q] = (unsigned char *) calloc (image->width, 1);


  for (y = 1; y < image->height - 2; y++)
    {
      cp = image->line[y] + 1;
      cp0 = image->line[y - 1] + 1;
      cp1 = image->line[y + 1] + 1;
      qp = Q[0] + 1;
      for (x = 1; x < image->width - 1; x++, cp++, cp0++, cp1++, qp++)
        {
          if (*cp==index)
            *qp = *cp;
          else if (*cp0==index)
            *qp = *cp0;
          else if (*cp1==index)
            *qp = *cp1;
          else if (*(cp - 1)==index)
            *qp = *(cp - 1);
          else if (*(cp + 1)==index)
            *qp = *(cp + 1);
          else
            *qp = *cp;
        }                       /* for each sample (x) */
      cp = Q[Qs - 1];
      memcpy (image->line[y - 1], cp, image->width);
      for (q = Qs - 1; q > 0; q--)
        Q[q] = Q[q - 1];
      Q[0] = cp;
    }                           /* for each line (y) */

  for (q = 0; q < Qs; q++)
    free ((char *) Q[q]);
  free ((char *) Q);

}

/////////////////////////////////////////////////////////////////////////////

void 
oimg_dehair_index (LLIMG * image, int z)
{

  int x, y;
  unsigned char *cp, *cp0, *cp1, *qp;
  unsigned char **Q;            /* space for neighborhood operation results */
  int Qs = 2;                   /* we're just going to do 3x3's */
  int q;

  Q = (unsigned char **) malloc (Qs * sizeof (unsigned char *));
  for (q = 0; q < Qs; q++)
    Q[q] = (unsigned char *) calloc (image->width, 1);


  for (y = 1; y < image->height - 2; y++)
    {
      cp = image->line[y] + 1;
      cp0 = image->line[y - 1] + 1;
      cp1 = image->line[y + 1] + 1;
      qp = Q[0] + 1;
      for (x = 1; x < image->width - 1; x++, cp++, cp0++, cp1++, qp++)
        if (*cp == z)
          *qp = *cp;
        else if (*cp0 == z && *cp1 == z && (*(cp - 1) == z || *(cp + 1) == z))
          *qp = z;
        else if (*(cp - 1) == z && *(cp + 1) == z && (*cp0 == z || *cp1 == z))
          *qp = z;
        else
          *qp = *cp;
      cp = Q[Qs - 1];
      if ( y > Qs )
        memcpy (image->line[y - 1]+1, cp+1, image->width-2);
      for (q = Qs - 1; q > 0; q--)
        Q[q] = Q[q - 1];
      Q[0] = cp;
    }                           /* for each line (y) */

  for (q = 0; q < Qs; q++)
    free ((char *) Q[q]);
  free ((char *) Q);

}

///////////////////////////////////////////////////////////////////////////

static int nodeCount (EdgeTreeNode *root) {
  if (!root)
    return 0;
  int c = 0;
  if (root->contour)
    c = 1;
  c += nodeCount (root->sibling);
  c += nodeCount (root->child);
  return c;
}


///////////////////////////////////////////////////////////////////////////

static int
plotContour (LLIMG * llimg, ContourPointNode * contour, int color)
{
  ContourPointNode *pt;
  int c=color;
  for (pt = contour; pt; pt = pt->next) {
    if (pt->y < llimg->height && pt->x < llimg->width)
      if ( !(c%2))
        llimg->line[pt->y][pt->x] = color;
      c++;
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////


static void
plotBranch (LLIMG * llimg, EdgeTreeNode * root, int level = 0)
{
  if (root->contour)
    plotContour (llimg, root->contour, level+3);
  EdgeTreeNode *curr;
  for (curr = root->child; curr; curr = curr->sibling)
    plotBranch (llimg, curr, level+1);
}

///////////////////////////////////////////////////////////////////////////

WRegion::
WRegion() {

  mask = NULL;
  contour_tree = NULL;
  _edges = 0;
  region = NULL;
  
}

///////////////////////////////////////////////////////////////////////////

WRegion::
~WRegion() {

  llimg_release_llimg (mask);
  contour_tree->deleteEdgeTree();
  if (region)
    DeleteObject (region);
  
}

///////////////////////////////////////////////////////////////////////////

void WRegion::createMask (LLIMG * llimg, int xm, int ym) {
  
  // We will use 0 for the background and 1 for the region

  llimg_release_llimg (mask);
  mask = NULL;

  if (!llimg)
    return;
  if (llimg->bits_per_pixel != 8 &&
      llimg->bits_per_pixel != 24)
    return;
  
  mask = llimg_create_base ();
  mask->width = 4*((llimg->width+3)/4);
  mask->height = llimg->height;
  mask->dib_height = -mask->height;
  mask->bits_per_pixel = 8;  // We need quick access and tag bits
  mask->data = (unsigned char *) malloc (mask->width * mask->height);
  if (!llimg->data) {
    llimg_release_llimg (mask);
    MessageBox (0, "Not enough memory for mask.", "osiva", MB_OK);
    return;
  }
  llimg_make_line_array (mask);
  mask->width = llimg->width;
  mask->color[0].blue = 255;
  mask->color[0].green = 255;
  mask->color[0].red = 255;
  mask->color[1].blue = 0;
  mask->color[1].green = 0;
  mask->color[1].red = 0;
  mask->color[2].blue = 180;
  mask->color[2].green = 180;
  mask->color[2].red = 255;
  mask->color[3].blue = 0;
  mask->color[3].green = 170;
  mask->color[3].red = 0;
  mask->color[4].blue = 255;
  mask->color[4].green = 255;
  mask->color[4].red = 0;
  mask->color[5].blue = 0;
  mask->color[5].green = 255;
  mask->color[5].red = 255;
  mask->color[6].blue = 255;
  mask->color[6].green = 0;
  mask->color[6].red = 255;

  // Scan through the image looking for non-background pixels

  int x, y;
  if (llimg->bits_per_pixel == 8) {
    int bg_value = llimg->line[ym][xm];
    unsigned char *cpm, *cph;
    for ( y = 0; y < llimg->height; y++ ) {
      cph = llimg->line[y];
      cpm = mask->line[y];
      for ( x = 0; x < llimg->width; x++ ) {
        if (*cph++ == bg_value)
          *cpm++ = 0;
        else
          *cpm++ = 1;
      }
    }
  }
  else if (llimg->bits_per_pixel == 24) {
    int bg_diff = ooptions->bg_diff;
    xm *= 3;
    unsigned char bgb = llimg->line[ym][xm+0];
    unsigned char bgg = llimg->line[ym][xm+1];
    unsigned char bgr = llimg->line[ym][xm+2];
    unsigned char *cpm, *cph;
    for ( y = 0; y < llimg->height; y++ ) {
      cph = llimg->line[y];
      cpm = mask->line[y];
      for ( x = 0; x < llimg->width; x++) {
        *cpm = 0;
        if (abs(*cph++ - bgb) > bg_diff)
          *cpm |= 1;
        if (abs(*cph++ - bgg) > bg_diff)
          *cpm |= 1;
        if (abs(*cph++ - bgr) > bg_diff)
          *cpm |= 1;
        cpm++;
      }
    }
  }

  // Thin the mask (by thickening the background)
  // Do this for JPEG images to reduce the dithering noise
  // around the edges

  if (llimg->bits_per_pixel == 24 && llimg->client1 < 2) {
    for (int i = 0; i < ooptions->erosions; i++)
      oimg_dialate_index (mask, 0);
  }

  // Clear out single pixel glints

  unsigned char *cpt, *cpl, *cpr, *cpb, *cp;

  for ( y = 1; y < mask->height-1; y++ ) {
    cpt = mask->line[y-1]+1;
    cpl = mask->line[y];
    cp  = mask->line[y]+1;
    cpr = mask->line[y]+2;
    cpb = mask->line[y+1]+1;
    for ( x = 1; x < mask->width-1; x++ ) {
      if ( *cpt == 0 && *cpl == 0 && *cpr == 0 && *cpb == 0)
        *cp = 0;
      cpt++;
      cpl++;
      cp++;
      cpr++;
      cpb++;
    }
  }

  for ( y = 1; y < mask->height-1; y++ ) {
    cpt = mask->line[y-1]+1;
    cpl = mask->line[y];
    cp  = mask->line[y]+1;
    cpr = mask->line[y]+2;
    cpb = mask->line[y+1]+1;
    for ( x = 1; x < mask->width-1; x++ ) {
      if ( *cpt == 1 && *cpl == 1 && *cpr == 1 && *cpb == 1)
        *cp = 1;
      cpt++;
      cpl++;
      cp++;
      cpr++;
      cpb++;
    }
  }

  // Clear out structures that the contour circulation cannot handle
  //  0X   X0
  //  X0   0X   these two cases cause leaks

  unsigned char *cp0_0, *cp1_0, *cp0_1, *cp1_1;

  for ( y = 0; y < mask->height-1; y++ ) {
    cp0_0 = mask->line[y];
    cp1_0 = cp0_0 + 1;
    cp0_1 = mask->line[y+1];
    cp1_1 = cp0_1 + 1;
    for ( x = 0; x < mask->width-1; x++) {
      if ( *cp0_0==0 && *cp1_0==1 && *cp0_1==1  && *cp1_1==0 )
        *cp0_0 = 1;
      cp0_0++;
      cp1_0++;
      cp0_1++;
      cp1_1++;
    }
  }


  for ( y = 0; y < mask->height-1; y++ ) {
    cp0_0 = mask->line[y];
    cp1_0 = cp0_0 + 1;
    cp0_1 = mask->line[y+1];
    cp1_1 = cp0_1 + 1;
    for ( x = 0; x < mask->width-1; x++) {
      if ( *cp0_0==1 && *cp1_0==0 && *cp0_1==0  && *cp1_1==1 )
        *cp0_0 = 0;
      cp0_0++;
      cp1_0++;
      cp0_1++;
      cp1_1++;
    }
  }


}

///////////////////////////////////////////////////////////////////////////

void WRegion::
extractRegion () {

  if (region)
    DeleteObject (region);
  region = NULL;

  ContourPointNode *contour = NULL;
  ContourPointNode *corners = NULL;
  POINT *poly = NULL;

  if (!mask)
    return;
  if (!mask->width || !mask->height)
    return;
  if ( mask->bits_per_pixel != 8)
    return;
    
  int x, y;
  unsigned char *cp;
  for (y = 0; y < mask->height; y++) {
    cp = mask->line[y];
    for (x = 0; x < mask->width; x++) {
      if (*cp++)
        goto found_contour_start;
    }
  }
  
  return; // The image is all background

found_contour_start:

  // Non-background pixel was found at (x, y)

  contour = llimg_traceContour(mask, x, y, 1);
  if ( !contour)
    return;

  // Ignore images without good sized regions

  int perim, l, t, r, b;
  llimg_measureContour (contour, perim, l, t, r, b);
  int ratio = ((r-l) * (b-t) * 100)/(mask->width * mask->height);
  if ( ratio < 20 )
    return;
  if ( (r-l) < 16 && (b-t) < 16 )
    return;

  // Transform the contour into a Windows HRGN

  int num_corners = 0;
  corners = llimg_extractCorners(contour, num_corners);
  llimg_deleteContour (contour);
  poly = (POINT *) llimg_makeContourPointArray (corners, num_corners);
  llimg_deleteContour (corners);
  region = CreatePolygonRgn (poly, num_corners, ALTERNATE);
  delete [] poly;

  return;
}

///////////////////////////////////////////////////////////////////////////


void WRegion::extractRegions () {

  if (region)
    DeleteObject (region);
  region = NULL;
  
  if (!mask)
    return;
  if (!mask->width || !mask->height)
    return;
  if ( mask->bits_per_pixel != 8)
    return;

  if (contour_tree) 
    contour_tree->deleteEdgeTree();
  _edges = 0;

  mask->client = 0; // Being used to count tree size
  contour_tree = new EdgeTreeNode;
  int error =
    llimg_buildChildEdgeTree (mask, contour_tree, ooptions->depth);
  if (error == 3) {
    return;
  }

  _edges = nodeCount (contour_tree);
  if (_edges == 0)
    return;

  // Ignore images without good sized regions

  int area = 0;
  int minx = mask->width;
  int maxx = 0;
  int miny = mask->height;
  int maxy = 0;
  int p, l, t, r, b;
  EdgeTreeNode *curr;
  for (curr = contour_tree->child; curr; curr = curr->sibling) {
    llimg_measureContour (curr->contour, p, l, t, r, b);
    area += (r - l)*(b - t);
    minx = min(minx, l);
    miny = min(miny, t);
    maxx = max(maxx, r);
    maxy = max(maxy, b);
  }
  int ratio = (100 * area)/(mask->width * mask->height);
  if ((maxx - minx) < 16 && (maxy - miny) < 16)
    return;
  if (ratio < 10) return;

  // Create the Windows API required HRGN from the contour_tree

  buildRegion();

  return;
}

///////////////////////////////////////////////////////////////////////////

void WRegion::plotTreeToMask()
{
  if (!mask)
    return;
  plotBranch (mask, contour_tree);
}


///////////////////////////////////////////////////////////////////////////

void WRegion::printRegionTree (char *filename)
{

FILE * fp = fopen (filename, "w");
if ( !fp )
  return;
llimg_printChildEdgeTree (contour_tree, fp, 0);
fclose (fp);
}

///////////////////////////////////////////////////////////////////////////

void WRegion::buildRegion ()
{
  if (region)
    DeleteObject (region);
  region = NULL;
  
  // First make "region' into an empty region
  
  region = CreateRectRgn (0, 0, 10, 10);
  HRGN r2 = CreateRectRgn (20, 20, 40, 40);
  int msg = CombineRgn (region, region, r2, RGN_AND);
  if (msg != NULLREGION)
    MessageBox (0, "Non-null base region", "WRegion::buildRegion", MB_OK);
  addEdge (contour_tree);   
}

///////////////////////////////////////////////////////////////////////////

void WRegion::addEdge (EdgeTreeNode *root)
{
  if (root->contour) {
    int n_pts = 0;
    POINT *pt_array = (POINT *)
      llimg_makeContourPointArray (root->contour, n_pts);
    HRGN rgn = CreatePolygonRgn (pt_array, n_pts, ALTERNATE);
    delete [] pt_array;
    if (root->fill_color == 1 )
      CombineRgn (region, region, rgn, RGN_OR);
    else
      CombineRgn (region, region, rgn, RGN_XOR);
    DeleteObject (rgn);
  }

  EdgeTreeNode *curr;
  for (curr = root->child; curr; curr = curr->sibling)
    addEdge (curr);

}

///////////////////////////////////////////////////////////////////////////

void WRegion::applyRegion (HWND hwnd)
{
  if (!region)
    return;

  SetWindowRgn (hwnd, region, TRUE);

  // After SetWindowRgn, Windows takes ownership of the region
  // Windows will delete it; the HRGN cannot be used for anything
  region = NULL;
}
