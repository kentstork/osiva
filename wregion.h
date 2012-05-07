/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * wregion.h is part of Osiva.
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
// File: wregion.h
//
//  #include <stdlib.h>
//  #include <string.h>
//  #include "ll_image.h"
//  #include "contour.h"
//  #include "wregion.h"
//  #include "windows.h"
//
///////////////////////////////////////////////////////////////////////////

class WRegion {
public:

  void createMask (LLIMG *llimg, int xm=0, int ym=0);
  LLIMG *get_mask() {return mask;}
  void extractRegion ();
  void extractRegions ();
  int edges () {return _edges;}
  int extractedOK () { return region?1:0; }
  void applyRegion (HWND hwnd);
  void plotTreeToMask ();
  void printRegionTree (char *filename);
  WRegion();
  ~WRegion();

private:

  void buildRegion(); // Builds HRGN region from contour_tree
  void addEdge (EdgeTreeNode *root); // Recursively builds HRGN region

private:

  LLIMG * mask;    // Mask image layer  
  EdgeTreeNode * contour_tree; // Head of a tree of ETN
  int _edges; // Number of edges in the contour_tree     
  HRGN region; // The region built out of the contour_tree

};


