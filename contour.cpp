/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * contour.cpp is part of Osiva.
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


////////////////////////////////////////////////////////////////////////////
//
// File: contour.cpp
//
////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ll_image.h"
#include "contour.h"


////////////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////////////

ContourPointNode *
llimg_traceContour (LLIMG * llimg, int x, int y, int cv)
{

/*
   * Purpose :    This function traces a contour around a binary segment
   *                  with value cv.
   *
   * Inputs:      line             array of pointers to line starts
   *              map              display area
   *              x, y             point on contour - seed point
   *              cv               contour segment value
   * 
   * Outputs:     n                number of points in contour
   *              ContourPointNode *  returns pointer to the contour
 */

  //        2            These are the direction #'s as coded in dx[], dy[].
  //     3  ^  1         3 is in the direction of the raster (0,0).
  //        |            1, 3, 5, and 7 are the directions of pixel corners.
  //    4< --- >0        Pixel corners are listed in pixel_dx[] and _dy[],
  //        |            e.g.: 1 has an offset of 1 in x and 0 in y.
  //     5  v  7         P_start[] is the pixel corner to start listing when
  //        6            a pixel was entered from the [] direction.
  //                     P_end[] is the corner to stop listing (and not list)
  //                     when a pixel was exited in the [] direction. 
  //                     Listing always occurs in the order 1, 3, 5, 7,
  //                     counterclockwise, with the interior to the left as
  //                     the region and its pixels are circulated.


  static int dx[8] =
  {1, 1, 0, -1, -1, -1, 0, 1};
  static int dy[8] =
  {0, -1, -1, -1, 0, 1, 1, 1};
  static int next_s[8] =
  {7, 7, 1, 1, 3, 3, 5, 5};

  static int pixel_dx[8] =
  {0, 1, 0, 0, 0, 0, 0, 1};
  static int pixel_dy[8] =
  {0, 0, 0, 0, 0, 1, 0, 1};
  static int p_start[8] =
  {7, 7, 1, 1, 3, 3, 5, 5};
  static int p_end[8] =
  {1, 3, 3, 5, 5, 7, 7, 1};

  unsigned char **line = llimg->line;
  int x_limit = llimg->width;
  int y_limit = llimg->height;
  
  ContourPointNode *cc = 0, *cc_start = 0;
  int last_dir, x_check, y_check;
  int i, j, s, x_start, y_start, x_corner, y_corner;
  
  
  if (x >= x_limit || y >= y_limit || x < 0 || y < 0)
  {
    return (0);
  }
  
  int n = 0;
  last_dir = 5;                 // 5
  s = 5;                        // 5
  
  for (;;)
  {    
    for (i = 0; i < 8; i++)
    {
      x_check = x + dx[s];
      y_check = y + dy[s];
      if (x_check >= 0 &&
        x_check < x_limit &&
        y_check >= 0 &&
        y_check < y_limit &&
        line[y_check][x_check] == cv)
      {
        j = p_start[last_dir];
        do
        {
          x_corner = x + pixel_dx[j];
          y_corner = y + pixel_dy[j];
          if (!cc_start)
          {
            cc_start = new ContourPointNode;
            cc = cc_start;
            x_start = x_corner;
            y_start = y_corner;
          }
          else
          {
            if (x_corner == x_start && y_corner == y_start)
              return (cc_start);
            cc->next = new ContourPointNode;
            cc = cc->next;
          }
          cc->x = x_corner;
          cc->y = y_corner;
          cc->next = 0;
          n++;
          j = (j + 2) % 8;
        }
        while (j != p_end[s]);
        x = x_check;
        y = y_check;
        last_dir = s;
        break;            /* point was found so break */
      }                   /* end of if point = cv */
      
      s = (s + 1) % 8;      /* else check next direction */
      
    }                       /* end of for each direction around (x,y) */
    
    if (i == 8)
    {                       /* single pixel contact ? */
      if (cc_start)
        return (0);
      cc_start = new ContourPointNode;
      cc = cc_start;
      cc->x = x;
      cc->y = y;
      cc->next = new ContourPointNode;
      cc = cc->next;
      cc->x = x + 1;
      cc->y = y;
      cc->next = new ContourPointNode;
      cc = cc->next;
      cc->x = x + 1;
      cc->y = y + 1;
      cc->next = new ContourPointNode;
      cc = cc->next;
      cc->x = x;
      cc->y = y + 1;
      n = 4;
      return (cc_start);
    }
    
    s = next_s[last_dir];
    
  }                           // end of forever

}                               /*  End of trace_contour  */



////////////////////////////////////////////////////////////////////////////
//
// extractCorners() removes non-corner points from region point lists
//
// The region must have  been "externally circulated"  for
// corner() to work correctly. This means that each  pixel
// must be circulated,  as well  as the  entire region.  A
// single pixel region, for  instance, will have 4  points
// in the list. A 4 pixel square will have 8 points in the
// list.
//
////////////////////////////////////////////////////////////////////////////

ContourPointNode *
llimg_extractCorners (ContourPointNode * poly, int &n_corners)
{
  ContourPointNode *polygon;
  ContourPointNode *pp, *p, *pn;
  ContourPointNode *corner_p, *corner_top;
  int corners = 0, type;

  /* make a list of the corner points */

  polygon = poly;

  pp = polygon;
  p = pp->next;
  pn = p->next;

  int dx, dy;

  do
    {

      dx = abs (pp->x - p->x);
      dy = abs (pp->y - p->y);
      if (dx && dy)
        fprintf (stderr, "\n** Diagonal points in lineutil:extractCorners **\n");
      if (!dx && !dy)
        fprintf (stderr, "\n** Redundant points in lineutil:extractCorners **\n");
      if (dx > 1)
        fprintf (stderr, "\n** x split in lineutil:extractCorners **\n");
      if (dy > 1)
        fprintf (stderr, "\n** y split in lineutil:extractCorners **\n");

      // 1 is apparently a "top" corner
      // 2 (we presume) is then a "bottom" corner 
      // Given: circulation is counter-clockwise

      type = 0;
      if (pp->x != p->x || pn->x != p->x)
        {
          if (pp->y != p->y)
            if (pp->y > p->y)
              type = 1;
            else
              type = 2;
          if (pn->y != p->y)
            if (pn->y > p->y)
              type = 1;
            else
              type = 2;
        }

      if (type)
        {
          if (corners)
            {
              corner_p->next = new ContourPointNode;
              corner_p = corner_p->next;
            }
          else
            {
              corner_top = new ContourPointNode;
              corner_p = corner_top;
            }
          corner_p->x = p->x;
          corner_p->y = p->y;
          corner_p->type = type;
          corner_p->next = 0;
          corners++;
        }                       /* end of corner point saving */

      pp = p;
      p = pn;
      pn = (p->next ? p->next : polygon);
    }
  while (pp != polygon);

  n_corners = corners;
  return (corner_top);
}

///////////////////////////////////////////////////////////////////////////
//
//  Support for the parity based fill operation used for contour scanning
//  and filling. Image regions must (1) have been *externally* circulated
//  for the fill to work, and (2) the contour must be filtered by "extract
//  Corners." 
//
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
//
// Used by qsort to order the corner arrays
//

static int
comp_corners (const void *l, const void *r)
{
  ContourPointNode **a = (ContourPointNode **) l;
  ContourPointNode **b = (ContourPointNode **) r;

  if ((*a)->y > (*b)->y ||
      ((*a)->y == (*b)->y && (*a)->x >= (*b)->x))
    return 1;
  else
    return -1;
}

///////////////////////////////////////////////////////////////////////////
//

static void
pn_insert (ContourPointNode * c_p, ContourPointNode ** l_p)
{
  ContourPointNode *trav_p, *trl_p, *curr_p;
  curr_p = new ContourPointNode;
  if (!curr_p)
    {
      fprintf (stderr, " ** Memory exhausted in lineutil:pn_insert() **\n");
      return;
    }
  curr_p->x = c_p->x;
  curr_p->y = c_p->y;
  curr_p->type = c_p->type;
  if (!*l_p)
    {
      *l_p = curr_p;
      curr_p->next = 0;
      return;
    }
  if (curr_p->x < (*l_p)->x)
    {
      curr_p->next = *l_p;
      *l_p = curr_p;
      return;
    }
  trav_p = *l_p;
  while ((trav_p) && trav_p->x <= curr_p->x)
    {
      trl_p = trav_p;
      trav_p = trav_p->next;
    }
  curr_p->next = trav_p;
  trl_p->next = curr_p;
}

///////////////////////////////////////////////////////////////////////////
//

static int
pn_remove (ContourPointNode * c_p, ContourPointNode ** l_p)
{
  ContourPointNode *trav_p, *trl_p;
  if (!(*l_p))
    return 0;
  trav_p = *l_p;
  if (c_p->x == trav_p->x)
    {
      *l_p = (*l_p)->next;
      delete trav_p;
      return 1;
    }
  while ((trav_p) && trav_p->x != c_p->x)
    {
      trl_p = trav_p;
      trav_p = trav_p->next;
    }
  if (trav_p)
    {
      trl_p->next = trav_p->next;
      delete trav_p;
      return 1;
    }
  return 0;
}

///////////////////////////////////////////////////////////////////////////
//

static void
scanDiagnostics (ContourPointNode * c_p, ContourPointNode * l_p,
                 ContourPointNode ** sorted, int corners,
                 ContourPointNode * contour)
{
  FILE *fp = fopen ("scandiagnostics.txt", "a");
  if (!fp)
    fp = stderr;
  else
    printf ("** diagnostics are being added to scandiagnostics.txt **\n");
  fprintf (fp, "\n**** scanPolygon diagnostics ****\n");
  fprintf (fp, "Looking for : (%d, %d) type %d\n", c_p->x, c_p->y, c_p->type);
  fprintf (fp, "Looking in the current scan list\n");
  for (; l_p; l_p = l_p->next)
    fprintf (fp, "   (%4d, %4d) Type %d\n", l_p->x, l_p->y, l_p->type);
  fprintf (fp, "Working through the current sorted corner list: \n");
  int i;
  for (i = 0; i < corners; i++)
    fprintf (fp, " (%4d, %4d) Type %d\n",
             sorted[i]->x, sorted[i]->y, sorted[i]->type);
  fprintf (fp, "All based on the following contour\n");
  ContourPointNode *curr;
  int pts = 0;
  int xmin = contour->x;
  int xmax = xmin;
  for (curr = contour; curr; curr = curr->next)
    {
      fprintf (fp, "   (%4d, %4d) Type %d\n", curr->x, curr->y, curr->type);
      xmin = curr->x < xmin ? curr->x : xmin;
      xmax = curr->x > xmax ? curr->x : xmax;
      pts++;
    }
  if (xmax - xmin < 150)
    llimg_dumpContour (contour, fp);
  fclose (fp);
}


///////////////////////////////////////////////////////////////////////////
//

int
llimg_fillContour (LLIMG * llimg, ContourPointNode * contour, int color)
{
  ContourPointNode *corner_p, **corner_p_a;
  ContourPointNode *current_top = 0, *current_p;
  int i, x, y, x_start, x_end;
  int diagnostics_dumped = 0;
  
  if (!llimg)
    return 1;
  if (!contour)
    return 1;
  
  // Make a polyline for the "corner parity" scanline fill routine
  // (a polyline differs from a contour in that it only has corners)
  
  int corners;
  ContourPointNode *corner_top;
  corner_top =
    llimg_extractCorners (contour, corners);
  
  // ...
  
  unsigned char **line = llimg->line;
  
  // build and sort an array of pointers to corners by y major, x
  
  corner_p_a = new ContourPointNode *[corners];
  if (!corner_p_a)
  {
    fprintf (stderr, " ** Memory exhausted in fillContour **\n");
    return 1;
  }
  corner_p = corner_top;
  for (i = 0; i < corners; i++)
  {
    corner_p_a[i] = corner_p;
    corner_p = corner_p->next;
  }
  qsort (corner_p_a, corners, sizeof (ContourPointNode *), comp_corners);
  
  // merge the scan lines and corner array together
  
  i = 0;
  y = corner_p_a[0]->y;
  while (i < corners)           /* loop once per scan line (y) */
  {
    while (i < corners && corner_p_a[i]->y == y)
    {
      if (corner_p_a[i]->type == 1)
        pn_insert (corner_p_a[i], &current_top);
      else if (!pn_remove (corner_p_a[i], &current_top))
      {
        fprintf (stderr, "fillContour(): could not remove corner %d \n",
          i);
        if (!diagnostics_dumped++)
          scanDiagnostics (corner_p_a[i], current_top, corner_p_a,
          corners, contour);
      }
      i++;
    }
    
    // scan across the current line segments
    
    for (current_p = current_top; current_p; current_p = current_p->next)
    {
      x_start = current_p->x;
      if (!(current_p = current_p->next))
      {
        fprintf (stderr, "** Scan parity violated **\n");
        break;
      }
      x_end = current_p->x;
      for (x = x_start; x < x_end; x++)
        line[y][x] = color;
    }                       // for each segment in this scan line
    
    y++;
  }                           // for each multi-segment scan line
  
  llimg_deleteContour (corner_top);
  delete[]corner_p_a;
  return 0;
}

///////////////////////////////////////////////////////////////////////////
//
// Contour scan routine for marking and measuring a blob
// OR's the specified value into the image to mark an area as "seen"
// Returns the area of the contour and the "weight" -- the number
// of pixels in the contour with the foreground color.
//

int
llimg_markContour (LLIMG * llimg, ContourPointNode * contour, int or_color,
                  int &area, int fg_color, int &weight)
{
  ContourPointNode *corner_p, **corner_p_a;
  ContourPointNode *current_top = 0, *current_p;
  int i, x, y, x_start, x_end;
  int diagnostics_dumped = 0;
  area = 0;
  weight = 0;
  
  if (!llimg)
    return 1;
  if (!contour)
    return 1;
  
  // Make a polyline for the "corner parity" scanline fill routine
  // (a polyline differs from a contour in that it only has corners)
  
  int corners;
  ContourPointNode *corner_top;
  corner_top =
    llimg_extractCorners (contour, corners);
  
  // ...
  
  unsigned char **line = llimg->line;
  
  // build and sort an array of pointers to corners by y major, x
  
  corner_p_a = new ContourPointNode *[corners];
  if (!corner_p_a)
  {
    fprintf (stderr, " ** Memory exhausted in fillContour **\n");
    return 1;
  }
  corner_p = corner_top;
  for (i = 0; i < corners; i++)
  {
    corner_p_a[i] = corner_p;
    corner_p = corner_p->next;
  }
  qsort (corner_p_a, corners, sizeof (ContourPointNode *), comp_corners);
  
  // merge the scan lines and corner array together
  
  i = 0;
  y = corner_p_a[0]->y;
  while (i < corners)           /* loop once per scan line (y) */
  {
    while (i < corners && corner_p_a[i]->y == y)
    {
      if (corner_p_a[i]->type == 1)
        pn_insert (corner_p_a[i], &current_top);
      else if (!pn_remove (corner_p_a[i], &current_top))
      {
        fprintf (stderr, "fillContour(): could not remove corner %d \n",
          i);
        if (!diagnostics_dumped++)
          scanDiagnostics (corner_p_a[i], current_top, corner_p_a,
          corners, contour);
      }
      i++;
    }
    
    // scan across the current line segments
    
    for (current_p = current_top; current_p; current_p = current_p->next)
    {
      x_start = current_p->x;
      if (!(current_p = current_p->next))
      {
        fprintf (stderr, "** Scan parity violated **\n");
        break;
      }
      x_end = current_p->x;
      for (x = x_start; x < x_end; x++)
      {
        if (line[y][x] == fg_color)
          weight++;
        line[y][x] |= or_color;
      }
      area += x_end - x_start;
    }                       // for each segment in this scan line
    
    y++;
  }                           // for each multi-segment scan line
  
  llimg_deleteContour (corner_top);
  delete[]corner_p_a;
  return 0;
}

///////////////////////////////////////////////////////////////////////////
//
// buildChildEdgeTree
//
// Desc:
//
//   Recursively builds a tree of the edge contours contained in
//   the source image. The initial EdgeTreeNode parameter should
//   have a newly new'd EdgeTreeNode, with 0 for a parent. This 
//   will seed the initial search area with a contour comprised
//   of the rectangle outlining the image: (0, 0) to (width, height.)
//

int
llimg_buildChildEdgeTree (LLIMG * llimg, EdgeTreeNode * parent,
                         int limit, int depth)
{

  int error = 0;
  ContourPointNode *corner_p, **corner_p_a;
  ContourPointNode *current_top = 0, *current_p;
  int i, x, y, x_start, x_end;
  int diagnostics_dumped = 0;
  
  if (!llimg)
    return 1;
  if (!parent)
    return 1;

  // Check to see how big this thing is getting
  
  llimg->client++;
  if (llimg->client > 1000)
    return 3;

  // Make a polyline for the "corner parity" scanline fill routine
  // (a polyline differs from a contour in that it only has corners)
  // If this is the root node synthesize a square polyline
  
  int corners;
  ContourPointNode *corner_top;
  
  if (parent->parent)
  {
    corner_top =
      llimg_extractCorners (parent->contour, corners);
  }
  else
  {
    ContourPointNode *pn = new ContourPointNode;
    pn->type = 1;             // top, 0, 0
    corner_top = pn;
    pn->next = new ContourPointNode;
    pn = pn->next;
    pn->y = llimg->height - 1;
    pn->type = 2;             // bottom, 0, height
    pn->next = new ContourPointNode;
    pn = pn->next;
    pn->x = llimg->width - 1;
    pn->y = llimg->height - 1;
    pn->type = 2;             // bottom, width, height
    pn->next = new ContourPointNode;
    pn = pn->next;
    pn->x = llimg->width - 1;
    pn->type = 1;             // bottom, width, height
    corners = 4;
  }
  
  // Determine the background and foreground color values
  // An image is treated as overlaid regions of alternating white and black
  // The colors will alternate with each level of depth in the tree
  
  unsigned char **line = llimg->line;
  int bg, fg;
  if (parent->contour)
    bg = line[parent->contour->y][parent->contour->x];
  else
    bg = 0;
  if (bg == 0)
    fg = 1;
  else if (bg == 1)
    fg = 0;
  else
  {
    fprintf (stderr, "** illegal background value: %d **\n", bg);
    return 2;
  }
  
  // build and sort an array of pointers to corners by y major, x
  
  corner_p_a = new ContourPointNode *[corners];
  if (!corner_p_a)
  {
    fprintf (stderr, " ** Memory exhausted in buildChildEdgeTree **\n");
    return 1;
  }
  corner_p = corner_top;
  for (i = 0; i < corners; i++)
  {
    corner_p_a[i] = corner_p;
    corner_p = corner_p->next;
  }
  qsort (corner_p_a, corners, sizeof (ContourPointNode *), comp_corners);
  
  // ...
  
  if (parent->child)
  {
    fprintf (stderr, "** EdgeTreeNode already has kids here? **\n");
  }
  EdgeTreeNode *etn = 0;
  
  // merge the scan lines and corner array together
  
  i = 0;
  y = corner_p_a[0]->y;
  while (i < corners)           /* loop once per scan line (y) */
  {
    while (i < corners && corner_p_a[i]->y == y)
    {
      if (corner_p_a[i]->type == 1)
        pn_insert (corner_p_a[i], &current_top);
      else if (!pn_remove (corner_p_a[i], &current_top))
      {
        printf ("\n...EdgeTree(): could not remove corner %d \n", i);
        if (!diagnostics_dumped++)
          scanDiagnostics (corner_p_a[i], current_top, corner_p_a,
          corners, parent->contour);
      }
      i++;
    }
    
    // scan across the current line segments
    
    for (current_p = current_top; current_p; current_p = current_p->next)
    {
      x_start = current_p->x;
      if (!(current_p = current_p->next))
      {
        fprintf (stderr, "** Scan parity violated **\n");
        break;
      }
      x_end = current_p->x;
      for (x = x_start; x < x_end; x++)
      {
        if (line[y][x] & 0x02)
          continue;       // already seen
        if ( depth >= limit) {
          line[y][x] |= 0x02;
          continue;
        }
        if (line[y][x] == fg)
        {
          ContourPointNode *contour = llimg_traceContour (llimg, x, y, fg);
          if (!etn)
          {
            etn = new EdgeTreeNode;
            parent->child = etn;
          }
          else
          {
            etn->sibling = new EdgeTreeNode;
            etn = etn->sibling;
          }
          etn->parent = parent;
          etn->contour = contour;
          etn->fill_color = fg;
          error = llimg_buildChildEdgeTree (llimg, etn, limit, depth+1);
          if (error) goto exit;
        }               // if there is an internal region, recurse
        line[y][x] |= 0x02;
      }                   // for each pixel in this segment
    }                       // for each segment in this scan line
    
    y++;
  }                           // for each multi-segment scan line
  
exit:
  llimg_deleteContour (current_top); // This is exceptional
  llimg_deleteContour (corner_top);
  delete[]corner_p_a;
  return error;
}


///////////////////////////////////////////////////////////////////////////
//

int
llimg_dumpContour (ContourPointNode * contour, FILE * fp)
{
  char **screen = 0;
  ContourPointNode *top_p, *point_p;
  int i, j, x0, y0, x1, y1;
  int x_range, y_range;

  top_p = contour;

  /* find the range of the polygon */

  point_p = top_p;
  x0 = x1 = point_p->x;
  y0 = y1 = point_p->y;
  while (point_p)
    {
      x0 = (point_p->x < x0 ? point_p->x : x0);
      y0 = (point_p->y < y0 ? point_p->y : y0);
      x1 = (point_p->x > x1 ? point_p->x : x1);
      y1 = (point_p->y > y1 ? point_p->y : y1);
      point_p = point_p->next;
    }
  x_range = x1 - x0 + 1;
  y_range = y1 - y0 + 1;
  x_range += 3;                 // for \0 and index room

  /* allocate the screen */

  screen = new char *[y_range];
  for (i = 0; i < y_range; i++)
    screen[i] = new char[x_range + 6];

  /* clear the screen */

  for (j = 0; j < y_range; j++)
    {
      for (i = 0; i < x_range; i++)
        screen[j][i] = ' ';
      sprintf (screen[j] + x_range, "%4d", y0 + j);
    }

  /* draw the polygon */

  point_p = top_p;
  while (point_p)
    {
      screen[point_p->y - y0][point_p->x - x0] = '+';
      point_p = point_p->next;
    }

  /* print the screen */

  fprintf (fp, "\n");
  for (j = 0; j < y_range; j++)
    fprintf (fp, "%s\n", screen[j]);
  fprintf (fp, "\n");

  /* de-allocate the screen */

  for (i = 0; i < y_range; i++)
    delete[]screen[i];
  delete[]screen;
  return (x1 > y1 ? x1 : y1);
}

////////////////////////////////////////////////////////////////////////////
//

llimg_plotContour (unsigned char **line, int xmax, int ymax,
                  ContourPointNode * contour, int cv)
{
  ContourPointNode *pt;
  for (pt = contour; pt; pt = pt->next)
    if (pt->y < ymax && pt->x < xmax)
      line[pt->y][pt->x] = cv;
  return 0;
}

////////////////////////////////////////////////////////////////////////////
//

void
llimg_deleteContour (ContourPointNode * contour)
{
  ContourPointNode *curr, *nxt;
  for (curr = contour; curr; curr = nxt)
    {
      nxt = curr->next;
      delete curr;
    }
}

////////////////////////////////////////////////////////////////////////////
//

void
llimg_measureContour (ContourPointNode * contour,
                     int &perim, int &l, int &t, int &r, int &b)
{
  if ( ! contour ) {
    perim = l = t = r = b = 0;
    return ;
  }
  ContourPointNode *cpn = contour ;
  l = cpn->x ;
  t = cpn->y ;
  r = l ;
  b = t ;
  perim = 0 ;
  for ( ; cpn ; cpn = cpn->next ) {
    if ( l > cpn->x ) l = cpn->x ;
    if ( t > cpn->y ) t = cpn->y ;
    if ( r < cpn->x ) r = cpn->x ;
    if ( b < cpn->y ) b = cpn->y ;
    perim++ ;
  }
}

////////////////////////////////////////////////////////////////////////////
//
// The caller must delete[] the returned array
//

ContourPoint *
llimg_makeContourPointArray
(ContourPointNode *contour, int &n_pts)
{
  ContourPoint * array = NULL;
  n_pts = 0;
  ContourPointNode *curr;
  for (curr = contour ; curr; curr = curr->next)
    n_pts++;
  if (!n_pts)
    return NULL;
  array = new ContourPoint [n_pts];
  if (!array)
    return NULL;
  int i = 0;
  for (curr = contour; curr; curr = curr->next) {
    array[i].x = curr->x;
    array[i].y = curr->y;
    i++;
  }
  return array;
}

////////////////////////////////////////////////////////////////////////////
//

void 
llimg_printChildEdgeTree (EdgeTreeNode *root,
                         FILE *fp, int level)
{

  int i;
  for ( i = 0 ; i < level ; i++ )
    fprintf (fp, "  ");
  int p, l, t, r, b;
  p = l = t = r = b = 0;
  llimg_measureContour (root->contour, p, l, t, r, b);
  fprintf ( fp, "%d (%d, %d) - (%d, %d) %s %d\n", p, l, t, r, b,
    root->fill_color?"black":"white", root);
  EdgeTreeNode *curr;
  for (curr = root->child; curr; curr = curr->sibling)
    llimg_printChildEdgeTree (curr, fp, level+1);

}



