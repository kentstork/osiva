/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * contour.h is part of Osiva.
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


//////////////////////////////////////////////////////////////////////////
//
// contour.h
//
// Synopsis
//
//  #include <stdio.h>
//  #include <stdlib.h>
//  #include <string.h>
//  #include "ll_image.h"
//  #include "contour.h"
//
// Description
//
//  Types and functions for extracting and filling contours and contour
//  trees within Hybrid Images.
//
//  A contour is a list of connected points around the edge of an image
//  region. The region is defined by a single foreground color different
//  from a single background color. Usually this means black pixels on
//  a white background.

// Contour form for Win32 compatible arrays
// This form can be used with CreatePolygonRgn()

typedef struct tagContourPoint { 
    int x;  // LONG under win32 API
    int y;  // LONG under win32 API
} ContourPoint; 

// Contour form for listing and dynamic operations

class ContourPointNode
{
  public:
  unsigned short x, y;
  enum { 
    CPN_TYPE_NONE,
    CPN_TYPE_TOP_CRNR,
    CPN_TYPE_BOT_CRNR
  } ;
  unsigned char type;
  ContourPointNode *next;
  ContourPointNode ()
  {
    x = y = 0;
    type = CPN_TYPE_NONE;
    next = 0;
  }
};


void 
llimg_deleteContour (ContourPointNode * contour) ;

ContourPointNode *
llimg_traceContour (LLIMG *llimg, int x, int y, int cv) ;

int
llimg_fillContour (LLIMG * llimg, ContourPointNode * contour, int color) ;

int
llimg_markContour (LLIMG * llimg, ContourPointNode * contour, int or_color,
                  int &area, int fg_color, int &weight ) ;

void
llimg_measureContour (ContourPointNode * contour,
                     int &perim, int &l, int &t, int &r, int &b) ;

ContourPointNode *
llimg_extractCorners (ContourPointNode *poly, int &n_corners) ;

int 
llimg_dumpContour (ContourPointNode *contour, FILE *fp=stdout) ;

int 
llimg_plotContour (unsigned char **line, int xmax, int ymax,
             ContourPointNode * contour, int cv) ;

// Caller must delete [] the returned array 

ContourPoint *
llimg_makeContourPointArray (ContourPointNode *contour, int &n_pts);

                            

// Edge Tree 

class EdgeTreeNode
{
  public:
  ContourPointNode * contour;
  unsigned char fill_color ;
  int area ;
  unsigned short left, top, right, bottom ;
  EdgeTreeNode *parent;
  EdgeTreeNode *sibling;
  EdgeTreeNode *child;
  EdgeTreeNode ()
  {
    contour = 0;
    fill_color = 0;
    area = 0;
    left = top = right = bottom = 0;
    parent = 0;
    sibling = 0;
    child = 0;
  }
  ~EdgeTreeNode()
  {
    llimg_deleteContour (contour);
  }
  void deleteEdgeTree () {
    deleteEdgeTree (this);
  }
  void deleteEdgeTree (EdgeTreeNode *root)
  {
    if (!root) return;
    deleteEdgeTree (root->sibling);
    deleteEdgeTree (root->child);
    delete root;
  }  
};

// Create and initialize an EdgeTreeNode with NULL as a parent
// to seed this recursive call.

int
llimg_buildChildEdgeTree (LLIMG *llimg, EdgeTreeNode *parent,
                         int depth, int level=0);

void 
llimg_printChildEdgeTree (EdgeTreeNode *root, FILE *fp, int level);


