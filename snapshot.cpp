/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * snapshot.cpp is part of Osiva.
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
// snapshot.cpp
//
// Principle module of the osiva image viewing application
// Contains the implementation of SnapShotW
//
///////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>         // for sqrt()
#include <windows.h>

#include "ll_image.h"     // The image structure (DIB like)
#include "contour.h"      // Contour functions, no STL,API dependencies
#include <vector>
using namespace std;
#include "wregion.h"      // Nice contour interface, uses STL, API

#include "snapshotw.h"    // The Window Manager manages Snapshot Windows
#include "iconbar.h"      // The window Manager has an Icon Bar
#include "wndmgr.h"       // The application has a Window Manager

#include <crtdbg.h>       // MSVC debugging functions
#include "resource.h"

#include "ooptions.h"     // independent

#include "hotspot.h"
#include "dialogs.h"

#define GET_X_LPARAM(lp)   ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)   ((int)(short)HIWORD(lp))

// Decoders
extern "C" {
LLIMG * read_jpeg_file ( char * filename );
}
extern LLIMG *
read_gif_file ( char * filename );
extern LLIMG *
expandGif (unsigned char *idata, int filebytes);

// Zooming
extern int 
llimg_reduce256 (LLIMG *image, int reduction, LLIMG *reduced);
extern int 
llimg_reduce24bit (LLIMG *image, int reduction, LLIMG *reduced);
extern LLIMG * 
llimg_resize (LLIMG *image, int width, int height);
extern void
llimg_lock_aspect (LLIMG *image, int &width, int &height);

//Rotation
extern int 
llimg_rotate24bitR (LLIMG *image, LLIMG *rotated);
extern int 
llimg_rotate8bitR (LLIMG *image, LLIMG *rotated);
extern int 
llimg_rotate24bitL (LLIMG *image, LLIMG *rotated);
extern int 
llimg_rotate8bitL (LLIMG *image, LLIMG *rotated);

// Dissolve effect
extern LLIMG *
llimg_dub (LLIMG *img);
extern LLIMG *
llimg_cpscreen (const RECT *source);

// Shell integration
extern int
drag_file_out (const char *filename);


static HCURSOR g_hand_cursor  = NULL;
static HCURSOR g_grasp_cursor = NULL;
static HCURSOR g_arrow_cursor = NULL;
static HCURSOR g_index_cursor = NULL;

// The logo image is bound into a custom resource
// When found, loaded, and locked it will be *logo_image

static unsigned char *logo_image = NULL;
static int logo_image_sz = 0;
static unsigned char *error_image = NULL;
static int error_image_sz = 0;

static void
showLastSysError( char * mess );

///////////////////////////////////////////////////////////////////////////////
// GLOBAL SCOPE

// Dialogs

HWND hw_help = NULL;
FlipDialog *flip_dialog = NULL;
TransDialog *trans_dialog = NULL;

// The Options

OOptions *ooptions = NULL;

///////////////////////////////////////////////////////////////////////////////
//
// static::showLastSysError
//
///////////////////////////////////////////////////////////////////////////////

static void
showLastSysError (HDC hdc, char *mess, int yy)
{
  char str[128];
  char buff[256];
  FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError (),
    0, str, 128, 0);
  sprintf (buff, "%s: %s", mess, str);
  TextOut (hdc, 1, yy, buff, strlen (buff));
}

///////////////////////////////////////////////////////////////////////////

static void
showLastSysError( char * mess ) {
  char str[128] ;
  DWORD err = GetLastError();
  FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, 0, err,
    0, str, 128, 0 ) ;
  MessageBox( 0, str, mess, 0 ) ;
}


///////////////////////////////////////////////////////////////////////////////
//
// static::mkLogPalette
//
///////////////////////////////////////////////////////////////////////////////

static LOGPALETTE *
mkLogPalette (LLIMG * img)
{
  LOGPALETTE *lp;
  int i;
  
  lp = (LOGPALETTE *) malloc (sizeof (LOGPALETTE) +
    sizeof (PALETTEENTRY) * 256);
  if (!lp)
  {
    MessageBox (0, "Not enough memory to make a logical palette",
      "ImageView::mkLogPalette", 0);
    return NULL;
  }
  
  lp->palVersion = 0x300;
  lp->palNumEntries = 256;
  for (i = 0; i < 256; i++)
  {
    lp->palPalEntry[i].peRed = img->color[i].red;
    lp->palPalEntry[i].peGreen = img->color[i].green;
    lp->palPalEntry[i].peBlue = img->color[i].blue;
    lp->palPalEntry[i].peFlags = 0;
  }
  return lp;
}

///////////////////////////////////////////////////////////////////////////////
//
// paintImage
//
///////////////////////////////////////////////////////////////////////////////

static void
paintImage (HWND hwnd, HDC hdc, LLIMG * img,
            PRECT prect, int xCurrentScroll, int yCurrentScroll,
            int stretch = 0)
{

  int err;
  void *verr;
  BITMAPINFO *pBMI;

  if (img == NULL)
    return;

  int yScrollPix = yCurrentScroll;
  int xScrollPix = xCurrentScroll;

  if (img->bits_per_pixel < 16)
    {
      if (!(img->hpalette))
        {
          img->log_palette = (void *) mkLogPalette (img);
          img->hpalette = (void *) CreatePalette ((LOGPALETTE *) img->
                                                  log_palette);
          // The logical palette does not have to be stored
          free(img->log_palette);
          img->log_palette = NULL;
        }
      verr = SelectPalette (hdc, (HPALETTE) img->hpalette, 0);
      if (verr == NULL)
        showLastSysError (hdc, "SelectPalette", 1);
      err = RealizePalette (hdc);
      if (err == GDI_ERROR)
        showLastSysError (hdc, "RealizePalette", 40);
    }                           /* do the contained for color tabled image */


  pBMI = (BITMAPINFO *) & (img->bih_top);
  
  if (stretch) {
    RECT rcli;
    GetClientRect (hwnd, &rcli);
    err = StretchDIBits (hdc,
      0, 0, rcli.right, rcli.bottom,
      0, 0, img->width, img->height,
      img->data, pBMI, DIB_RGB_COLORS,
      SRCCOPY);
  }
  else {
    err = SetDIBitsToDevice (hdc,
      -xScrollPix, -yScrollPix,
      img->width, img->height,
      0, 0,
      0, abs (img->height),
      img->data, pBMI, DIB_RGB_COLORS);
  }

  
  if (!err)
    showLastSysError (hdc, "SetDIBitsToDevice", 80);
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

SnapShotW::SnapShotW () {
  hInst = NULL;       // Application instance
  hw_main = NULL;     // Handle to this window
  g_image = NULL;     // Currently displaying image
  g_llimg = NULL;     // Full size, as read version of the image
  g_llimg_x8 = NULL;  // Reduced 1/8 size version of the image
  g_saved = NULL;     // Holder for * to g_image while dissolve frame 0 shows
                      // Above also serves as flag saying frame 0 is showing
  g_x8_up = 0;        // Flag meaning the 1/8 size image is showing
  reduction = 8;
  transparent = 0;
  tolerance = 0;
  erosions = 0;
  mask_depth = 0;
  rotation = 0;

  wndmgr = NULL;

  in_logo = 0;
  in_init = 1;
  in_error = 0;
  in_move = 0;
  in_dragout = 0;
  in_rotate = 0;
  in_resize = 0;
  has_moved = 0;
  has_rotated = 0;
  act_state = 0;
  shift_key = 0;
  curr_file = NULL;
  paint_stretch = 0;
};


SnapShotW::~SnapShotW () {

  llimg_release_llimg (g_llimg);
  llimg_release_llimg (g_llimg_x8);
  delete [] curr_file;

};

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

int SnapShotW::test () {
  RECT rect;
  GetClientRect (hw_main, &rect);
  LLIMG *llimg = llimg_cpscreen (&rect);
  paint_temp_image (llimg);
  llimg_release_llimg (llimg);
  return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////


void SnapShotW::toggle_trans (int x, int y) {
  if (transparent) {
    SetWindowRgn (hw_main, NULL, TRUE);
    transparent = 0;
    return;
  }
  apply_trans(x, y);
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

void SnapShotW::apply_trans (int x, int y) {

  WRegion wregion;
  wregion.createMask (g_image, x, y);

  /***
  RECT r;
  SetRectEmpty (&r);
  HDC hdc = GetDC(hw_main);
  paintImage (hw_main, hdc, wregion.get_mask(), &r, 0, 0);
  ReleaseDC (hw_main, hdc);
  MessageBox (0, "sup", 0, 0);
  ***/

  wregion.extractRegions ();
  if (!wregion.extractedOK()) {
    // wregion.printRegionTree ("region_tree.txt");
    wregion.plotTreeToMask();
    RECT r;
    SetRectEmpty (&r);
    HDC hdc = GetDC(hw_main);
    paintImage (hw_main, hdc, wregion.get_mask(), &r, 0, 0);
    ReleaseDC (hw_main, hdc);
  }
  else {
    wregion.applyRegion (hw_main);
    transparent = 1;
    tolerance = ooptions->bg_diff;
    erosions = ooptions->erosions;
    mask_depth = ooptions->depth;
  }
  return;
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

LLIMG *SnapShotW::dub_image () {
  LLIMG *dubimg = llimg_dub (g_image);
  return dubimg;
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

int SnapShotW::handle_click (int x, int y, int keymod) {
  // If there was no drag, treat click as a zoom click
  // But only reduce an image if it was the topmost window
  // i.e. if it did not need painting during the activation
  POINT pt;
 
  if (in_logo) {
    ShowWindow (hw_help, SW_SHOW);
    BringWindowToTop (hw_help);
    return 0;
  }

  if (keymod & MK_SHIFT) {
    SetWindowPos (hw_main, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    return 0;
  }
  if (keymod & MK_CONTROL) {
    pt.x = x;
    pt.y = y;
    ScreenToClient (hw_main, &pt);
    toggle_trans (pt.x, pt.y);
    return 0;
  }

  // If the effect of the click was to show the image, do not zoom it
  if (act_state == 3)
    return 0;

  if (ooptions->suppress_zoom)
    return 0;

  SetWindowRgn (hw_main, NULL, FALSE);
  transparent = 0;

  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  x = max (16, x);
  y = max (16, y);
  x = min (x, scrn.right - 16);
  y = min (y, scrn.bottom - 16);
   
  // A reduction of 0 means the current image is a custom reduction

  // If the native image is showing and the cached image is
  // standard (eg. 1-9) show the existing small reduction
  if (!g_x8_up && reduction > 0) {
    show_small_img (x, y, reduction);
    return 0;
  }

  // If the native image is showing and there is a custom reduction,
  // go ahead and use it if it is n% or less of the native area
  unsigned long native_area;
  unsigned long custom_area;
  unsigned long area_ratio;
  if (!g_x8_up){
    if (g_llimg_x8) {
      native_area = g_llimg->width * g_llimg->height;
      custom_area = g_llimg_x8->width * g_llimg_x8->height;
      area_ratio = (100 * custom_area) / native_area;
      if (area_ratio < 45) {
        show_small_img (x, y, 0);
        return 0;
      }
    } // There is a custom reduced image in cache
    show_small_img (x, y, wndmgr->reduction);
    return 0;
  } // The native size image is showing

  
  // A non-zero reduction means the standard reduction is showing
  // so just toggle
  if (reduction) {
    show_centered_img (x, y);
    return 0;
  }
  
  // If the zoomed image is showing at a custom scale and is n%
  // or less of the image area, use the native image

  native_area = g_llimg->width * g_llimg->height;
  custom_area = g_llimg_x8->width * g_llimg_x8->height;
  area_ratio = (100 * custom_area) / native_area;
  if (area_ratio < 45) {
    show_centered_img (x, y);
    return 0;
  }

  // If the custom image is bigger than n%, choose the other scale
  // (reduction or original) that is most different

  int reduce = wndmgr->reduction;
  unsigned long reduced_area = 
    (g_llimg->width/reduce) * (g_llimg->height/reduce);

  unsigned long reduced_ratio;
  if (custom_area > reduced_area)
    reduced_ratio = (100 * custom_area) / reduced_area;
  else
    reduced_ratio = (100 * reduced_area) / custom_area;

  unsigned long original_ratio;
  if (custom_area > native_area)
    original_ratio = (100 * custom_area) / native_area;
  else
    original_ratio = (100 * native_area) / custom_area;

  if (original_ratio > reduced_ratio)
    show_centered_img (x, y);
  else
    show_small_img (x, y, reduce);

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

int SnapShotW::
handle_left_down (HWND hwnd, UINT uint, WPARAM wparam, LPARAM lparam)
{  
  if ((GetAsyncKeyState (VK_MENU) & 0x8000) 
    && curr_file) {
    int rslt = drag_file_out (curr_file);
    if (rslt == 2) { 
      if (wndmgr)
        wndmgr->close_window (this);
    }
    return 0;
  }

  RECT clnt;

  SetCapture (hwnd);
  GetClientRect (hwnd, &clnt);
  base_pt.x = GET_X_LPARAM (lparam);
  base_pt.y = GET_Y_LPARAM (lparam);

  if (clnt.right - base_pt.x < 12 && clnt.bottom - base_pt.y < 12) {
    in_resize = 1;
    if (transparent) {
      SetWindowRgn (hwnd, NULL, TRUE);
      transparent = 0;
    }
  }

  else if (base_pt.y < 12 && abs(base_pt.x - clnt.right/2) < clnt.right/6) {
    in_rotate = 1;
    has_rotated = 0;
    if (transparent) {
      SetWindowRgn (hwnd, NULL, TRUE);
      transparent = 0;
    }
  }


  else {
    in_move = 1;
    SetCursor (g_grasp_cursor);
  }
  
  ClientToScreen (hwnd, &base_pt);
  has_moved = 0;
  click_pt = base_pt; 

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

int SnapShotW::
handle_mouse_move (HWND hwnd, UINT uint, WPARAM wparam, LPARAM lparam)
{
 
  POINT pt;
  RECT rect;
  int x, y;

  pt.x = GET_X_LPARAM (lparam);
  pt.y = GET_Y_LPARAM (lparam);
  
  if (in_move)
  {
    ClientToScreen (hwnd, &pt);
    if (pt.x == base_pt.x && pt.y == base_pt.y)
      return 0;
    GetWindowRect (hwnd, &rect);
    int dx = pt.x - base_pt.x;
    int dy = pt.y - base_pt.y;
    MoveWindow (hwnd, rect.left + dx, rect.top + dy,
      rect.right - rect.left, rect.bottom - rect.top, TRUE);
    base_pt = pt;
    if (!has_moved) {
      dx = abs(pt.x - click_pt.x);
      dy = abs(pt.y - click_pt.y);
      if (dx > 4 || dy > 4) has_moved = 1;
    }
    return 0;
  }

  if (in_rotate)
  {
    if (has_rotated)
      return 0;
    ClientToScreen (hwnd, &pt);
    if (pt.x == base_pt.x && pt.y == base_pt.y)
      return 0;
    GetWindowRect (hwnd, &rect);
    base_pt = pt;
    int dx = pt.x - click_pt.x;
    if (abs(dx) > 16) {
      if (dx > 0)
        rotate (1);
      else
        rotate (0);
      has_rotated = 1;
    }
    return 0;
  }
  
  if (in_dragout)
  {
    ClientToScreen (hwnd, &pt);
    int dx = abs (pt.x - click_pt.x);
    int dy = abs (pt.y - click_pt.y);
    if ( dx > 5 || dy > 5) {
      ReleaseCapture ();
      in_dragout = 0;
      int rslt = drag_file_out (curr_file);
      if (rslt == 2) { 
        if (wndmgr)
          wndmgr->close_window (this);
      }
    }   
    return 0;
  }
  
  if (in_resize == 1)
  {
    x =  GET_X_LPARAM (lparam);
    y =  GET_Y_LPARAM (lparam);
    GetWindowRect (hwnd, &rect);
    if (x < 32)
      x = 32;
    if (y < 32)
      y = 32;
    base_pt.x = x;
    base_pt.y = y;
    if (!(wparam & MK_SHIFT))
      llimg_lock_aspect (g_llimg, x, y);
    MoveWindow (hwnd, rect.left, rect.top, x, y, TRUE);
    paint_stretch = 1;
    InvalidateRect (hw_main, NULL, FALSE);
    UpdateWindow (hw_main);
    paint_stretch = 0;
  }
  


  // Set the cursor
  
  GetClientRect (hwnd, &rect);
  
  if (rect.right - pt.x < 12 && rect.bottom - pt.y < 12) {
    SetCursor (LoadCursor (NULL, IDC_SIZENWSE)); 
  }
  else if (pt.y < 12 && abs(pt.x - rect.right/2) < rect.right/6) {
    SetCursor (LoadCursor (NULL, IDC_SIZEWE));
  }
  else if (in_logo)
  {
    int dlr = min (pt.x, rect.right - pt.x);
    int dtb = min (pt.y, rect.bottom - pt.y);
    if (dlr > 10 && dtb > 16) {
      SetCursor (g_index_cursor);
    }
    else {
      SetCursor (g_hand_cursor);
    }
  }
  else {
    SetCursor (g_hand_cursor);
  }

  return 0;
  
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

int SnapShotW::
handle_left_up (HWND hwnd, UINT uint, WPARAM wparam, LPARAM lparam)
{
  POINT pt;
  RECT clnt;

  ReleaseCapture ();
  
  if (in_move) {
    // If there was no drag, treat click as a zoom click
    // But only reduce an image if it was the topmost window
    // i.e. if it did not need painting during the activation
    in_move = 0;
    SetCursor (g_hand_cursor);
    pt.x = GET_X_LPARAM (lparam);
    pt.y = GET_Y_LPARAM (lparam);
    ClientToScreen (hwnd, &pt);
    if (!has_moved)
      handle_click (pt.x, pt.y, (int) wparam);
  }
  
  if (in_resize)
  {
    in_resize = 0;
    GetClientRect (hwnd, &clnt);
    SetCursor (LoadCursor (NULL, IDC_WAIT));
    llimg_release_llimg (g_llimg_x8);
    g_llimg_x8 = 
      llimg_resize (g_llimg, clnt.right+1, clnt.bottom+1);
    SetCursor (g_hand_cursor);    
    g_image = g_llimg_x8;
    //SetWindowRgn (hw_main, NULL, FALSE);
    //transparent = 0;
    InvalidateRect (hwnd, NULL, FALSE);
    UpdateWindow (hwnd);
    g_x8_up = 1;
    reduction = 0;
  } 

  act_state = 0;
  has_moved = 0;
  in_rotate = 0;

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

/* TODO: It is wrong for ShapShotW to know about wndmgr at all. SnapShot should
 * just handle the events it knows about, and forward the rest to it's parent
 * hwnd. The parent Hwnd should be an invisible window associated with wndmgr
 * to get events through.  */

int SnapShotW::
handle_keydown (HWND hwnd, UINT uint, WPARAM wparam, LPARAM lparam)
{
  RECT rect;
  RECT scrn;

  GetWindowRect (hwnd, &rect);
  int rw = rect.right - rect.left;
  int rh = rect.bottom - rect.top;
  SystemParametersInfo 
    (SPI_GETWORKAREA, 0, &scrn, 0);
  int step = 1;
  if (shift_key) step = 4;
  if (GetAsyncKeyState (VK_CONTROL) & 0x8000) {
    step  = 10;
    if (shift_key) step = 32;
  }

  switch (wparam)
  {
  default:
    break;
  case VK_SHIFT:
    if (in_resize) {
      if (!shift_key) {
        MoveWindow (hwnd, rect.left, rect.top, base_pt.x, base_pt.y, TRUE);
      }
    }
    shift_key = 1;
    return 0;
  case 'P':
    if (GetAsyncKeyState (VK_CONTROL) & 0x8000) {
      wndmgr->dump_info();
    }
    return 0;    
  case VK_UP:
    if ( rect.bottom > 16 )
      MoveWindow (hwnd, rect.left,
      rect.top - step, rw, rh, TRUE);
    return 0;
  case VK_DOWN:
    if ( rect.top < scrn.bottom - 16 )
      MoveWindow (hwnd, rect.left,
      rect.top + step, rw, rh, TRUE);
    return 0;
  case VK_LEFT:
    if ( rect.right > 16 )
      MoveWindow (hwnd, rect.left - step,
      rect.top, rw, rh, TRUE);
    return 0;
  case VK_RIGHT:
    if ( rect.left < scrn.right - 16 )
      MoveWindow (hwnd, rect.left + step,
      rect.top, rw, rh, TRUE);
    return 0;
  case VK_PRIOR:
    wndmgr->cluster(in_move?this:0);
    return 0;
  case VK_NEXT:
    wndmgr->distribute(in_move?this:0);
    return 0;
  case VK_HOME:
    wndmgr->center(in_move?this:0);
    return 0;
  case VK_END:
    wndmgr->corner(in_move?this:0);
    return 0;
  case VK_BACK:
    wndmgr->reduce_energy(VK_BACK, WndMgr::ATTRACT);
    return 0;
  case VK_SPACE:
    wndmgr->reduce_energy(VK_SPACE);
    return 0;
  case VK_ADD:
    wndmgr->reduce_energy(VK_ADD);
    return 0;
  case VK_SUBTRACT:
    wndmgr->reduce_energy(VK_SUBTRACT, WndMgr::ATTRACT);
    return 0;
  case VK_DECIMAL:
    SetWindowPos (hwnd, HWND_BOTTOM, 0, 0, 0, 0,
      SWP_NOMOVE | SWP_NOSIZE);
    return 0;
  case VK_F1:
    ShowWindow (hw_help, SW_SHOW);
    return 0;
  }
  
  return 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

int SnapShotW::
handle_keyup (HWND hwnd, UINT uint, WPARAM wparam, LPARAM lparam)
{
  RECT rect;
  GetWindowRect (hwnd, &rect);
  int x, y;

  switch (wparam)
  {
  default:
    break;
  case VK_SHIFT:
    shift_key = 0;
    if (!in_resize)
      break;
    x = base_pt.x;
    y = base_pt.y;
    llimg_lock_aspect (g_llimg, x, y);
    MoveWindow (hwnd, rect.left, rect.top, x, y, TRUE);
    return 0;
  }  
  return 1;
}



///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

int SnapShotW::
handle_command (HWND hwnd, UINT uint, WPARAM wparam, LPARAM lparam)
{
  RECT rect;
  int wID = LOWORD (wparam);

  switch (wID) {
  case ID_CLOSE:
    if (wndmgr)
        wndmgr->close_window (this);
      else
        PostQuitMessage (0);
    break;
  case ID_FITSCREEN:
    SystemParametersInfo (SPI_GETWORKAREA, 0, &rect, 0);
    move_img (&rect);
    break;
  case ID_CONTEXT_ZOOM12:
    show_small_img (click_pt.x, click_pt.y, 2);
    break;
  case ID_CONTEXT_ZOOM13:
    show_small_img (click_pt.x, click_pt.y, 3);
    break;
  case ID_CONTEXT_ZOOM14:
    show_small_img (click_pt.x, click_pt.y, 4);
    break;
  case ID_CONTEXT_ZOOM18:
    show_small_img (click_pt.x, click_pt.y, 8);
    break;
  case ID_REPLACETILE:
    if (wndmgr)
      wndmgr->tile_screen (this);
    break;
  };

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

void SnapShotW::show_img_fix_corner (int x, int y) {
  // Expand the corner that the drop is in
  g_image = g_llimg;
  if (g_image) {
    int w = max( 16, g_image->width );
    int h = max( 16, g_image->height );
    RECT rect;
    GetWindowRect (hw_main, &rect);
    int dx = x - rect.left;
    int dy = y - rect.top;
    int dw = rect.right - rect.left;
    int dh = rect.bottom - rect.top;
    // 1 bit means right; 2 bit means bottom
    // 0:top-left 1:top-right 2:bottom-left 3:bottom-right
    int type = 0;
    if ( dx > dw/2 ) type |= 1;
    if ( dy > dh/2 ) type |= 2 ;
    int new_x;
    int new_y;
    switch (type) {
    case 0:
      // top-left
      new_x = rect.left;
      new_y = rect.top;
      break;
    case 1:
      // top-right
      new_x = rect.right - w;
      new_y = rect.top;
      break;
    case 2:
      // bottom-left
      new_x = rect.left;
      new_y = rect.bottom - h;
      break;
    case 3:
      // bottom-right;
      new_x = rect.right - w;
      new_y = rect.bottom - h;
      break;
    } // switch on type

    // Make sure the resized window is on screen

    RECT scrn;
    SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
    if ((new_x + w) < 8)
      new_x = 0;
    else if (new_x > (scrn.right - 8))
      new_x = scrn.right - w;
    if ((new_y + h) < 8)
      new_y = 0;
    else if (new_y > (scrn.bottom - 8))
      new_y = scrn.bottom - h;
    
    MoveWindow (hw_main, new_x, new_y, w, h, TRUE);

  }
  g_x8_up = 0;
  InvalidateRect (hw_main, NULL, TRUE);
  UpdateWindow (hw_main);
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

int SnapShotW::handle_drop (HDROP hdrop){
  int f;
  POINT pt;
  char filename[512];
  int files = DragQueryFile (hdrop, 0xFFFFFFFF, 0, 0);
  if ((files == 1 && g_x8_up == 0) || !wndmgr) {
    DragQueryPoint (hdrop, &pt);
    ClientToScreen (hw_main, &pt);
    DragQueryFile (hdrop, 0, filename, 512);
    load_image (filename, pt.x, pt.y);
  }
  else {
    for (f = 0; f < files; f++) {
      DragQueryFile (hdrop, f, filename, 512);
      if (wndmgr->is_layout_file (filename))
        wndmgr->load_layout_file (filename);
      else
        wndmgr->new_window(hInst, filename, SW_SHOW);
    }
  }
  DragFinish (hdrop);

  return 0;
}
  
///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

void SnapShotW::load_image ( char *filename, int x, int y ) {
  int _in_error = 0;
  int _in_logo = 0;
  SetForegroundWindow (hw_main);
  SetCursor (LoadCursor (NULL, IDC_WAIT));    
  LLIMG *llimg = NULL;
  // Use the logo image if no filename was specified
  if ( strlen(filename) == 0 ) {
    _in_logo = 1;
    llimg = expandGif (logo_image, logo_image_sz);
    //WRegion wregion;
    //wregion.createMask (llimg);
    //wregion.extractRegion ();
    //wregion.applyRegion (hw_main);
    //transparent = 1;
  }
  // Otherwise try the decompressors until one works
  else {
    SetWindowRgn (hw_main, NULL, FALSE);
    transparent = 0;
    llimg = read_jpeg_file (filename);
    if ( !llimg )
      llimg = read_gif_file (filename);
  }
  // Maybe it is a layout file?
  if (!llimg) {
    if (wndmgr->is_layout_file (filename)){
      wndmgr->load_layout_file (filename);
      SetCursor (g_hand_cursor);    
      return;
    }
  }
  // If there's still no image use the error image
  if (!llimg) {
    _in_error = 1;
    llimg = expandGif (error_image, error_image_sz);
  }

  llimg_release_llimg (g_llimg);
  g_llimg = llimg;
  llimg_release_llimg (g_llimg_x8);
  g_llimg_x8 = NULL;
    in_error = _in_error;
  in_logo = _in_logo;
 
  SetCursor (g_hand_cursor);    
  if (x < 0 || in_error)
    show_centered_img (x, y);
  else 
    show_img_fix_corner (x, y);

  if (in_logo || in_error)
    toggle_trans ();

  delete [] curr_file;
  static char *resource_name = "Internal Resource";
  curr_file = NULL;
  if (!in_logo && !in_error) {
    curr_file = new char [strlen(filename) +1];
    strcpy (curr_file, filename);
  }
  else {
    curr_file = new char [strlen(resource_name) +1];
    strcpy (curr_file, resource_name);
  }

  rotation = 0;

}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

void SnapShotW::rotate (int clockwise) {
  SetForegroundWindow (hw_main);
  HCURSOR currcur = GetCursor ();
  SetCursor (LoadCursor (NULL, IDC_WAIT));    
  SetWindowRgn (hw_main, NULL, FALSE);
  transparent = 0;
  
  // We want to rotate around the center, holding size constant

  // Remember the display size and position of the current image

  int w_old = g_image->width;
  int h_old = g_image->height;
  RECT rcurr;
  GetWindowRect (hw_main, &rcurr);
  int cx = (rcurr.right + rcurr.left)/2;
  int cy = (rcurr.bottom + rcurr.top)/2;

  // Roate the image in memory

  LLIMG *llimg = llimg_create_base ();
  
  int ret = 0;
  
  if (g_llimg->bits_per_pixel == 24)
  {
    if (clockwise) {
      ret = llimg_rotate24bitR (g_llimg, llimg);
    }
    else {
      ret = llimg_rotate24bitL (g_llimg, llimg);
    }
  }
  else // bits_per_pixel = 8
  {
    if (clockwise) {
      ret = llimg_rotate8bitR (g_llimg, llimg);
    }
    else {
      ret = llimg_rotate8bitL (g_llimg, llimg);
    }
  }


  if (ret) {
    llimg_release_llimg (llimg);
    SetCursor (currcur);    
    return ;
  }
  
  
  // Show the new display geometry, but with a stretched image for feedback
  // Since resizing might take a long time


  SetWindowRgn (hw_main, NULL, FALSE);
  transparent = 0;


  int left = cx - (h_old/2);
  int top = cy - (w_old/2);
  
  g_image = llimg;
  paint_stretch = 1;
  MoveWindow (hw_main, left, top, h_old, w_old, TRUE); // w and h are rotated
  InvalidateRect (hw_main, NULL, FALSE);
  UpdateWindow (hw_main);
  paint_stretch = 0;
  
  llimg_release_llimg (g_llimg);
  g_llimg = llimg;
  llimg_release_llimg (g_llimg_x8);
  g_llimg_x8 = NULL;
  g_image = g_llimg; 
  
  // Resize the already rotated image into the rotated window
  // If the image isn't the nominal size
  // g_image is the one painted onto the display
  
  if (g_x8_up) {
    g_llimg_x8 = llimg_resize (g_llimg, h_old+1, w_old+1); // w and h are rotated
    g_image = g_llimg_x8;
  }

  InvalidateRect (hw_main, NULL, FALSE);
  UpdateWindow (hw_main);
  SetCursor (currcur);    

  // Remember the rotation for report generation. Rotation is clockwise increasing.

  if (clockwise) {
    rotation = (rotation + 1) % 4;
  }
  else {
    rotation = (rotation - 1 + 4) % 4;
  }


}


///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

void SnapShotW::show_small_img (int x, int y, int reduce) {
  int err;
  
  if (!g_llimg) return;
  if (in_error) return;

  // Force a new reduced image if necessary
  // reduce = 0 means use the current g_llimg_x8 if there is one
  if (reduce > 0 && reduction != reduce) {
    llimg_release_llimg (g_llimg_x8);
    g_llimg_x8 = NULL;
    reduction = reduce;
  }

  // Build a new reduced image if needed

  if (!g_llimg_x8) {
    if (!reduction)
      reduction = wndmgr->reduction;
    switch (g_llimg->bits_per_pixel){
    case 8:
      SetCursor (LoadCursor (NULL, IDC_WAIT));
      g_llimg_x8 = (LLIMG *) malloc(sizeof(LLIMG));
      err = llimg_reduce256 (g_llimg, reduction, g_llimg_x8);
      SetCursor (g_hand_cursor);    
      if (err) return;
      break;
    case 24:
      SetCursor (LoadCursor (NULL, IDC_WAIT));    
      g_llimg_x8 = (LLIMG *) malloc(sizeof(LLIMG));
      err = llimg_reduce24bit (g_llimg, reduction, g_llimg_x8);
      SetCursor (g_hand_cursor);    
      if (err) return;
      break;
    default:
      return;
    } // switch on bits per pixel

    // mark the image as a reduction
    // KLUDGE, used in WRegion::createMask to suppress dilation
    g_llimg_x8->client1 = reduction ;
  } // if reduced image does not exist
  
  g_image = g_llimg_x8;  
  int new_w = max (16, g_image->width);
  int new_h = max (16, g_image->height);

  // Center the image about the click point
  // Negative x means maintain center

  RECT r;
  GetWindowRect (hw_main, &r);
  int new_x, new_y;
  if ( x >= 0 ) {
    new_x = x - (new_w * (x - r.left))/(r.right - r.left);
    new_y = y - (new_h * (y - r.top))/(r.bottom - r.top);
  }
  else {
    new_x = (r.left + r.right)/2 - new_w/2;
    new_y = (r.top + r.bottom)/2 - new_h/2;
  }

  // Make sure the image shows onscreen

  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  if (new_x + new_w < 16)
    new_x = 0;
  if (new_y + new_h < 16)
    new_y = 0;
  if (new_x > scrn.right - 16)
    new_x = scrn.right -16;
  if (new_y > scrn.bottom - 16)
    new_y = scrn.bottom - 16;

  
  SetWindowRgn (hw_main, NULL, FALSE);
  transparent = 0;
  SetWindowPos (hw_main, HWND_TOP, new_x, new_y, new_w, new_h, 0);
  InvalidateRect (hw_main, NULL, TRUE);
  UpdateWindow (hw_main);
  g_x8_up = 1;
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

void SnapShotW::move_img (const RECT *rect, int fix_aspect) {
  
  int w = rect->right - rect->left + 1;
  int h = rect->bottom - rect->top + 1;
  // Remember the target dimensions
  int targ_w = w;
  int targ_h = h;
  // ...
  if (fix_aspect)
    llimg_lock_aspect (g_llimg, w, h);
  // Center the display in the target area
  int left = rect->left + (targ_w - w)/2;
  int top = rect->top + (targ_h - h)/2;
  // ...
  if (w == g_llimg->width && h == g_llimg->height) {
    g_image = g_llimg;  
    g_x8_up = 0;
  }
  else {
    int matches = 0;
    if (g_llimg_x8) {
      if (w == g_llimg_x8->width && h == g_llimg_x8->height)
        matches = 1;
    }
    if (!matches) {
      SetCursor (LoadCursor (NULL, IDC_WAIT));
      llimg_release_llimg (g_llimg_x8);
      g_llimg_x8 = 
        llimg_resize (g_llimg, w, h);
      SetCursor (g_hand_cursor);
    }
    g_image = g_llimg_x8;
    reduction = 0;
    g_x8_up = 1;
  }
  SetWindowRgn (hw_main, NULL, FALSE);
  transparent = 0;
  InvalidateRect (hw_main, NULL, FALSE);
  UpdateWindow (hw_main);
  MoveWindow (hw_main, left, top, w-1, h-1, TRUE);
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////


void SnapShotW::paint_temp_image (LLIMG *llimg) {
  RECT r;
  SetRectEmpty (&r);
  HDC hdc = GetDC(hw_main);
  paintImage (hw_main, hdc, llimg, &r, 0, 0);
  ReleaseDC (hw_main, hdc);
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

void SnapShotW::show_screen () {
  RECT rect;
  GetWindowRect (hw_main, &rect);
  LLIMG *llimg = llimg_cpscreen (&rect);
  if (!llimg) return;
  g_saved = g_image;
  g_image = llimg;
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

void SnapShotW::show_saved () {
  if (g_saved) {
    llimg_release_llimg (g_image);  // Delete the screen shot
    g_image = g_saved;
    g_saved = NULL;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

/*
 a = xy
 x/y = i/j
 x = y (i/j)
 x = (a/x)(i/j)
 xx = a(i/j);
*/

// (area * width) easily overflows an integer for large images

void SnapShotW::scale_to_area (int area) {
  SetCursor (LoadCursor (NULL, IDC_WAIT));
  double aw = double(area) * double(g_llimg->width);
  double xx = aw/g_llimg->height;
  int w = (int)(sqrt(xx) + 0.5);
  int h = area / w;
  llimg_lock_aspect (g_llimg, w, h);
  llimg_release_llimg (g_llimg_x8);
  g_llimg_x8 = 
    llimg_resize (g_llimg, w, h);
  SetCursor (g_hand_cursor);    
  g_image = g_llimg_x8;
  SetWindowRgn (hw_main, NULL, FALSE);
  transparent = 0;
  InvalidateRect (hw_main, NULL, FALSE);
  UpdateWindow (hw_main);
  g_x8_up = 1;
  reduction = 0;
  RECT rect;
  GetWindowRect (hw_main, &rect);
  MoveWindow (hw_main, rect.left, rect.top, w-1, h-1, TRUE);
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

// Expand the image around the mouse click

void SnapShotW::show_centered_img (int x, int y) {
  g_image = g_llimg;
  if (!g_image)
    return;
  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  int w = max( 16, g_image->width );
  int h = max( 16, g_image->height );

  // Hold the click point (x, y) over the same image point
  RECT r;
  GetWindowRect (hw_main, &r);
  int xclk = (w * (x - r.left)) / (r.right - r.left);
  int yclk = (h * (y - r.top)) / (r.bottom - r.top);
  int new_x = x - xclk;
  int new_y = y - yclk;

  // Negative x or y indicate a request to center
  if (x < 0)
    new_x = (scrn.right - w)/2;
  if (y < 0)
    new_y = (scrn.bottom - h)/2;

  // Put the Logo so that it shows around the help dialog
  if (in_logo && in_init) {
    GetWindowRect (hw_help, &r);
    new_x = r.right - w + 32;
  }

  if (transparent) {
    SetWindowRgn (hw_main, NULL, FALSE);
    transparent = 0;
  }
    
  MoveWindow (hw_main, new_x, new_y, w, h, TRUE);
  g_x8_up = 0;
  InvalidateRect (hw_main, NULL, TRUE);
  UpdateWindow (hw_main);
}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

void SnapShotW::init(HINSTANCE hInstance,
                     LPSTR lpCmdLine, int nCmdShow) {

  WNDCLASSEX wcl;
  wcl.cbSize = sizeof (WNDCLASSEX);
  wcl.hInstance = hInstance;
  wcl.lpszClassName = "osiva";
  wcl.lpfnWndProc = WindowProcProxy;
  wcl.style = CS_DBLCLKS;
  wcl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  wcl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
  wcl.hCursor = g_hand_cursor;
  wcl.lpszMenuName = NULL;      /* no main menu */
  wcl.cbClsExtra = 0;
  wcl.cbWndExtra = 0;
  wcl.hbrBackground = (HBRUSH) GetStockObject (GRAY_BRUSH);
  if (!RegisterClassEx (&wcl))
    {
      //showLastSysError ("RegisterClassEx");
    }

  hw_main = CreateWindowEx (WS_EX_TOOLWINDOW,
                            "osiva", "osiva", WS_POPUP,
                            100, 100, 200, 200, NULL,
                            NULL, hInstance, NULL);
  if (!hw_main)
    {
      showLastSysError ("CreateWindow");
      return;
    }

  SetWindowLong (hw_main, GWL_USERDATA, (LONG) this);
  DragAcceptFiles (hw_main, TRUE);

  load_image(lpCmdLine, -1, -1);
  ShowWindow (hw_main, nCmdShow);
  UpdateWindow (hw_main);

  hmenu = GetSubMenu (
    LoadMenu (hInstance, MAKEINTRESOURCE(IDR_MENU)), 1);

  // In case ooptions isn't up yet, protect against fault
  if (ooptions)
    reduction = ooptions->reduction;

  in_init = 0;

}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////


LRESULT CALLBACK SnapShotW::
WindowProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  RECT rect;
  POINT pt;
  int rslt;

  switch (message)
    {

    case WM_DROPFILES:
      handle_drop ((HDROP) wParam);
      return 0;

    case WM_RBUTTONDOWN:
      if (curr_file) {
        SetCapture (hwnd);
        click_pt.x = GET_X_LPARAM (lParam);
        click_pt.y = GET_Y_LPARAM (lParam);
        ClientToScreen (hwnd, &click_pt);
        in_dragout = 1;
      }
      return 0;

    case WM_COMMAND:
      handle_command (hwnd, message, wParam, lParam);
      return 0;

    case WM_LBUTTONDOWN:
      handle_left_down (hwnd, message, wParam, lParam);
      return 0;

    case WM_MOUSEACTIVATE:
      act_state = 1;
      return 0;
      
    case WM_MOUSEMOVE:
      handle_mouse_move (hwnd, message, wParam, lParam);
      return 0;
      
    case WM_SETFOCUS:
      if (wndmgr)
        wndmgr->show();
      
      return 0;
      
    case WM_KILLFOCUS:
      in_move = 0;
      in_resize = 0;
      in_dragout = 0;
      in_rotate = 0;
      ReleaseCapture ();
      SetCursor (g_hand_cursor);
      if (wndmgr)
        wndmgr->show(0, (HWND) wParam);
      
      
      return 0;
      
    case WM_LBUTTONUP:
      handle_left_up (hwnd, message, wParam, lParam);
      return 0;
      
    case WM_RBUTTONUP:
      in_dragout = 0;
      if (ooptions->right_click_closes == 0) {
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ClientToScreen (hwnd, &pt);
        TrackPopupMenuEx (hmenu, TPM_LEFTALIGN|TPM_TOPALIGN,
          pt.x, pt.y, hwnd, 0);
        ReleaseCapture ();
        return 0;
      }
      if (wndmgr)
        wndmgr->close_window (this);
      else
        PostQuitMessage (0);
      return 0;

    case WM_KEYDOWN:
      rslt = handle_keydown (hwnd, message, wParam, lParam);
      if (!rslt)
        return 0;
      break;

    case WM_KEYUP:
      rslt = handle_keyup (hwnd, message, wParam, lParam);
      if (!rslt)
        return 0;
      break;

    case WM_CHAR:
      switch (wParam)
        {
        default:
          break;
        case 'r':
        case 'R':
          rotate (1);
          return 0;
        case 'l':
        case 'L':
          rotate (0);
          return 0;
        case '*':
          test ();
          return 0;
        case 'x':
        case 'X':
          if (wndmgr)
            wndmgr->fit_top_window (WndMgr::FIT_EXACTLY);
          return 0;
        case 'q':
        case 'Q':
          if (wndmgr)
            wndmgr->dissolve_init ();
          return 0;
        case 'f':
        case 'F':
          if (wndmgr)
            wndmgr->set_flip();
          return 0;
        case 'h':
        case 'H':
        case '?':
          ShowWindow (hw_help, SW_SHOW);
          BringWindowToTop (hw_help);
          return 0;
        case 'd':
        case 'D':
          if (wndmgr)
            wndmgr->distribute();
          return 0;
        case 's':
        case 'S':
          SetWindowPos (hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE);
          return 0;
        case '0':
        case 't':
        case 'T':
          toggle_trans ();
          return 0;
        case 'z':
        case 'Z':
          GetWindowRect (hwnd, &rect);
          handle_click ((rect.left + rect.right)/2,
            (rect.top + rect.bottom)/2);
          return 0;
        case VK_ESCAPE:
          if (wndmgr)
            wndmgr->close_window (this);
          else
            PostQuitMessage (0);
          return 0;
        case VK_RETURN:
        case VK_TAB:
          if (wndmgr)
            wndmgr->tab (this);
          return 0;
        case '1':
          if (wndmgr)
            wndmgr->all_big();
          return 0;
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          if (wndmgr) {
            wndmgr->reduction = (int)(wParam - '0');
            wndmgr->all_small(in_move?this:0);
          }
          return 0;
        }
      break;

    case WM_PAINT:
      {
        PRECT prect;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint (hwnd, &ps);
        prect = &ps.rcPaint;
        act_state |= 2;
        paintImage (hwnd, hdc, g_image, prect, 0, 0, paint_stretch);
        EndPaint (hwnd, &ps);
      }
      return 0;


    }

  return DefWindowProc (hwnd, message, wParam, lParam);

}

///////////////////////////////////////////////////////////////////////////////
//
// 
//
///////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
help_proc ( HWND hwnd, UINT m, WPARAM w, LPARAM l)
{
  int cmd;

  switch (m) {
  case WM_INITDIALOG:
    {
      HWND eb = GetDlgItem (hwnd, IDC_HELPEDIT);
      RECT cr;
      GetClientRect (hwnd, &cr);
      MoveWindow (eb, 8, 8, cr.right - 16,
                  cr.bottom - 16, TRUE);
    }
    return 1;

  case WM_SIZE:
    {
      HWND eb = GetDlgItem (hwnd, IDC_HELPEDIT);
      RECT cr;
      GetClientRect (hwnd, &cr);
      MoveWindow (eb, 8, 8, cr.right - 16,
                  cr.bottom - 16, TRUE);
    }
    break;
    
  case WM_COMMAND:
    cmd = LOWORD (w);
    if (cmd == 1003)
      return 0;
    switch (cmd) {
    case IDCANCEL:
    case IDOK:
      ShowWindow (hwnd, SW_HIDE);
      return 1;
    }
  }
  return 0;
}
 

///////////////////////////////////////////////////////////////////////////////
//
// parse_command_line
//
// Destructively parses the buffer in "line"
// returns a NULL terminated array of char *'s
// the array has been allocated with "new", so delete[] it
//

static char **parse_command_line (char *line){
  
  // Count the parameters
  // If a parameter starts with a quote it ends with a quote
  // Otherwise it ends with a space
  // Parameters are always space delimited
  // (But only quoted if they contain spaces, on an icon drop)

  int toks = 0;
  char *cp = line;
  
  for (;;) {
    for (; *cp == ' '; cp++);
    if (*cp == 0) break;
    if (*cp == '\"') 
      for (cp++; *cp && *cp != '\"'; cp++);
    else 
      for (cp++; *cp && *cp != ' '; cp++);
    if (*cp) cp++;
    toks++;
  }
  
  char **tok = new char * [toks+1]; // extra for NULL termination

  int t = 0;
  cp = line;
  for (;;) {
    for (; *cp == ' '; cp++);
    if (*cp == 0) break;
    if (*cp == '\"') {
      tok[t] = ++cp;
      for (; *cp && *cp != '\"'; cp++);
    }
    else {
      tok[t] = cp++;
      for (; *cp && *cp != ' '; cp++);
    }
    if (*cp) *cp++ = 0;
    t++;
  }
  
  tok[t] = 0;

  return tok;
}

///////////////////////////////////////////////////////////////////////////////



int APIENTRY
WinMain (HINSTANCE hInstance,
         HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  MSG msg;

  // Pre-process the command line

  char *cl = GetCommandLine ();
  char *cl2 = strdup (cl);
  char **tok = parse_command_line (cl2);
  char *buff = new char [strlen (cl) *2];
  *buff = 0;
  for (int toks = 0; tok[toks]; toks++) {
    strcat (buff, tok[toks]);
    strcat (buff, "\n");
  }
  delete [] buff;

  int tok_base = 1;
  int force_instance = 0;
  if (toks > 1) {
    if (!strcmp (tok[1], "+")) {
      force_instance = 1;
      tok_base++;
    }
  }

  // Look for an existing instance (unless forced otherwise)

  HWND hw_iconbar =  NULL;
  HANDLE mutex = CreateMutex (NULL, FALSE, "osivas_mutex");
  if ( mutex ) {
    DWORD wait_rtn = WaitForSingleObject (mutex, 0L);
    switch (wait_rtn) {
    case WAIT_ABANDONED:
    case WAIT_OBJECT_0:
      break;
    case WAIT_TIMEOUT:
      if (force_instance) break;
      for (int check = 0 ; check < 20 ; check++) {
        hw_iconbar = FindWindow ("iconbar", "osiva icon bar");
        if (hw_iconbar) break;
        Sleep (100);
      }
      if (hw_iconbar) {
        SetForegroundWindow (hw_iconbar);
        COPYDATASTRUCT cds;
        for (int t = 1; t < toks; t++ ) {
          cds.cbData = strlen(tok[t]) + 1;
          cds.dwData = 0;
          cds.lpData = tok[t];
          SendMessage (hw_iconbar, WM_COPYDATA, NULL, (LPARAM) &cds);
        }
        CloseHandle (mutex);
        _CrtDumpMemoryLeaks();
        return 0;
      } // if HWND of existing instance was found
      else {
        MessageBox (0, "Cannot find the running program", "Osiva", 0);
      }
    } // switch on wait return
  } //  if mutex handle obtained
  else {
    MessageBox (0, "Cannot create the mutex", "Osiva", 0);
  }

 
  // This instance will be THE instance (or ANOTHER instance...)

  OleInitialize (NULL); // Very Important, NOT CoInitialize()

  // OOptions is an Observer Subject, so it has to get up first
  // 
  ooptions = new OOptions;
  ooptions->default_options();
  
  RECT r_scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &r_scrn, 0);

  // Setup the Help dialog

  {
    hw_help = CreateDialog (hInstance, (LPCTSTR) IDD_HELP,
      NULL, help_proc);
    RECT r_help;
    GetWindowRect (hw_help, &r_help);
    int dx = r_help.right - r_help.left;
    int dy = r_help.bottom - r_help.top;
    int x = (r_scrn.right - dx)/2;
    int y = (r_scrn.bottom - dy)/2;
    y = max (0, y);
    MoveWindow (hw_help, x, y, dx, dy, FALSE);
    HRSRC hrsrc = FindResource (NULL, 
      MAKEINTRESOURCE(IDR_TXT1),"TXT");
    HGLOBAL hglobal = LoadResource (NULL, hrsrc);
    char * help_text = (char *) LockResource (hglobal);
    int help_text_sz = SizeofResource (NULL, hrsrc);
    char *text_buff = new char [help_text_sz + 1];
    memcpy (text_buff, help_text, help_text_sz);
    text_buff[help_text_sz] = 0;
    SetDlgItemText(hw_help, IDC_HELPEDIT, text_buff);
    delete [] text_buff;   
  }

  // Create the other dialogs

  flip_dialog = new FlipDialog ();
  flip_dialog->init (hInstance);
  trans_dialog = new TransDialog ();
  trans_dialog->init (hInstance);
  
  // Load the Logo

  HRSRC hrsrc = FindResource (NULL, MAKEINTRESOURCE(IDR_GIF1),
    "GIF");
  HGLOBAL hglobal = LoadResource (NULL, hrsrc);
  logo_image = (unsigned char *) LockResource (hglobal);
  logo_image_sz = SizeofResource (NULL, hrsrc);

  // Load the Error Image

  hrsrc = FindResource (NULL, MAKEINTRESOURCE(IDR_GIF2),
    "GIF");
  hglobal = LoadResource (NULL, hrsrc);
  error_image = (unsigned char *) LockResource (hglobal);
  error_image_sz = SizeofResource (NULL, hrsrc);

  // ...

  g_arrow_cursor = LoadCursor (NULL, IDC_ARROW);
  g_hand_cursor  = LoadCursor (hInstance, MAKEINTRESOURCE(IDC_HAND));
  g_grasp_cursor = LoadCursor (hInstance, MAKEINTRESOURCE(IDC_GRASP));
  g_index_cursor = LoadCursor (hInstance, MAKEINTRESOURCE(IDC_INDEX));

  // Create the Window Manager

  WndMgr *wm = new WndMgr;
  wm->init(hInstance);

  char *clez = strrchr (tok[0], '\\');
  if (clez)
    *clez = 0;
  wm->exe_path = new char[strlen(tok[0])+1];
  strcpy (wm->exe_path, tok[0]);

  // Load the Images (or default)
    
  if (toks <= 1) {
    wm->new_window (hInstance, "", nCmdShow);
  }
  else {
    for (int t = tok_base; t < toks; t++ ) {
      if (wm->is_layout_file (tok[t]))
        wm->load_layout_file (tok[t]);
      else
        wm->new_window (hInstance, tok[t], nCmdShow);
    }
  }
  
  while (GetMessage (&msg, NULL, 0, 0))
    {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }

  delete wm;

  delete flip_dialog;
  delete trans_dialog;
  delete ooptions;

  delete [] tok;
  free (cl2);


  ReleaseMutex (mutex);
  CloseHandle (mutex);
  _CrtDumpMemoryLeaks();
  return msg.wParam;
}




