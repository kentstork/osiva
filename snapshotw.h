/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * snapshotw.h is part of Osiva.
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
// File: snapshotw.h
//
// Synopsis:
//
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <windows.h>
// #include "ll_image.h"
// #include "snapshotw.h"
//


class SnapShotW {

public:

  SnapShotW();
  ~SnapShotW();
  void SnapShotW::init(HINSTANCE hInstance,
                       LPSTR lpCmdLine, int nCmdShow);
  void load_image (char *filename, int x, int y);
  void show_centered_img (int x, int y);
  void show_img_fix_corner (int x, int y);
  void show_small_img (int x, int y, int reduce = 0);
  void move_img (const RECT *rect, int fix_aspect = 1);
  void scale_to_area (int area);
  void set_wndmgr (class WndMgr *wm) {wndmgr = wm;}
  void toggle_trans (int x=0, int y=0);
  void apply_trans (int x=0, int y=0);
  void rotate (int clockwise = 1);
  HWND get_hwnd () {return hw_main;}
  HINSTANCE instance () {return hInst;}

  // For supporting frame to frame dissolve
  // ---------------------------------------
  // Return pointer to the currently showing image
  // This pointer is only good for a local read
  // Don't store it -- it could easily become invalid
  const LLIMG *get_image_ptr () { return g_image; }
  // Returns pointer to dubbed image; client gets ownership
  // The image is the one currently showing (g_image)
  LLIMG *dub_image ();
  // Just lay an image into the DC; no persistence
  void paint_temp_image (LLIMG *llimg);
  // Grab the screen over the window and use it as the g_image
  // The current g_image gets saved
  void show_screen ();
  // Show the saved image and delete the screen image
  void show_saved ();
  
  // For report generation
  const char *get_file_path () { return curr_file; }
  int get_transparent () { return transparent; }
  int get_tolerance () { return tolerance; }
  int get_erosions () { return erosions; }
  int get_mask_depth () { return mask_depth; }
  int get_width () { return g_image->width; }
  int get_height () {return g_image->height;}
  int get_rotation () {return rotation;}

  int get_in_logo () {return in_logo;}

private:

  class WndMgr *wndmgr;

  HINSTANCE hInst;    // Application instance
  HWND hw_main;       // Handle to this window
  HMENU hmenu;        // The context menu
  LLIMG *g_image;      // Currently displaying image
  LLIMG *g_llimg;      // Full size, as read version of the image
  LLIMG *g_llimg_x8;   // Reduced 1/8 size version of the image
  LLIMG *g_saved;      // Temp * for image while showing screen (for dissolve)
  int g_x8_up;        // Flag meaning the 1/8 size image is showing
  int reduction;      // Reduction factor of cached small image, 2 to 9
                      // reduction =  0 ==> custom reduction
  int transparent;    // Flag meaning transparent borders are showing
  int tolerance;      // Transparency background tolerance
  int erosions;       // Transparency mask erosions
  int mask_depth;     // Transparency nesting depth, like ooptions->depth
  int rotation;       // User rotation, clockwise, 0, 1, 2, or 3
  int in_logo;        // The splash logo is showing
  int in_init;        // During posting of initial image
  int in_error;       // The error image is showing
  int in_move;        // Click - drag operation in progress
  int in_resize;      // Click at bottom left - window resize in progress
  int in_dragout;     // Right Click - a drag out if moved far enough
  int in_rotate;      // Click - at top center maybe a rotate
  int has_moved;      // Mouse dragged enough to not be just a click
  int has_rotated;    // Mouse dragged enougth to rotate and the rotate is done
  int act_state;      // Activation state: bit 1=WM_MOUSEACTIVATION 2=WM_PAINT
  POINT click_pt;     // The point initally left clicked on
  POINT base_pt;      // The latest reference point for the drag
  int shift_key;      // Flag meaning the shift key is down
  char *curr_file;    // Path of the currently viewing file
  int paint_stretch;  // Flag requesting a StretchDIBits when painting

  int handle_click (int x, int y, int keymod = 0);
  int handle_drop (HDROP hdrop);
  int handle_left_down (HWND, UINT, WPARAM, LPARAM);
  int handle_mouse_move (HWND, UINT, WPARAM, LPARAM);
  int handle_left_up (HWND, UINT, WPARAM, LPARAM);
  int handle_keydown (HWND, UINT, WPARAM, LPARAM);
  int handle_keyup (HWND, UINT, WPARAM, LPARAM);
  int handle_command (HWND, UINT, WPARAM, LPARAM);
  int test ();


  LRESULT CALLBACK WindowProc (HWND, UINT, WPARAM, LPARAM);

  static LRESULT CALLBACK WindowProcProxy (HWND h, UINT u, WPARAM w, LPARAM l){
    SnapShotW *p = 
      (SnapShotW *) GetWindowLong(h, GWL_USERDATA) ;
    if (p)
      return p->WindowProc (h, u, w, l);
    else
      return DefWindowProc (h, u, w, l);
  }


};

