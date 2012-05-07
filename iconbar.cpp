/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * iconbar.cpp is part of Osiva.
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
// File: iconbar.cpp
//
///////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ll_image.h"
#include "windows.h"
#include "iconbar.h"   
#include "resource.h"
#include "smlstr.h"
#include "tooltip.h"

extern LLIMG *
expandGif (unsigned char *idata, int filebytes);

#define WM_USER_OPEN (WM_USER + 1)

///////////////////////////////////////////////////////////////////////////

static int find_bg_index (LLIMG *llimg) {
  if (!llimg) return -1;
  if (llimg->bits_per_pixel != 8)
    return -1;
  int first_i = *(llimg->data);
  int second_i;

  // Look for a different color through the center
  int y = llimg->height / 2;
  for (int x = 0; x < llimg->width; x++) 
    if ( (second_i = llimg->line[y][x]) != first_i)
      break;
 
  if (first_i == second_i)
    return first_i;
  // If the first color is black, assume the second color
  // is the background color
  if (llimg->color[first_i].red == 0 &&
      llimg->color[first_i].green == 0 &&
      llimg->color[first_i].blue == 0 )
      return second_i;
  // otherwise assume the first color is the background color
  return first_i;
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

///////////////////////////////////////////////////////////////////////////

static void
paintImage (HWND hwnd, HDC hdc, LLIMG * img, RECT *rect)
{
  int err;
  void *verr;
  if (img == NULL)
    return;
  BITMAPINFO *pBMI;

  // Manage color table setup, if there is one
  
  if (img->bits_per_pixel < 16)
  {
    if (!(img->hpalette))
    {
      LOGPALETTE *lp = (LOGPALETTE *) calloc (sizeof (LOGPALETTE) +
        sizeof (PALETTEENTRY) * 256, 1);
      lp->palVersion = 0x300;
      lp->palNumEntries = 256;
      for (int i = 0; i < 256; i++)
      {
        lp->palPalEntry[i].peRed = img->color[i].red;
        lp->palPalEntry[i].peGreen = img->color[i].green;
        lp->palPalEntry[i].peBlue = img->color[i].blue;
      }
      img->hpalette = (void *) CreatePalette (lp);
      free (lp);
    }
    verr = SelectPalette (hdc, (HPALETTE) img->hpalette, 0);
    if (verr == NULL)
      showLastSysError ("SelectPalette");
    err = RealizePalette (hdc);
    if (err == GDI_ERROR)
      showLastSysError ("RealizePalette");
  }

  // Manage the geometry

  int XDest = 0;
  int YDest = 0;
  DWORD dwWidth = img->width;
  DWORD dwHeight = img->height;
  int XSrc = 0;
  int YSrc = 0;
  if (rect) {
    XSrc = rect->left;
    YSrc = rect->top;
    XDest = XSrc;
    YDest = YSrc;
    dwWidth = rect->right - rect->left + 1;
    dwHeight = rect->bottom - rect->top + 1;
  }
  
  pBMI = (BITMAPINFO *) & (img->bih_top);
  err = SetDIBitsToDevice (hdc,
    XDest, YDest,
    dwWidth, dwHeight,
    XSrc, YSrc,
    0, abs (img->height),
    img->data, pBMI, DIB_RGB_COLORS);
  
  if (!err)
    showLastSysError ("SetDIBitsToDevice");
}

///////////////////////////////////////////////////////////////////////////

IconBar::IconBar() {
  _hotspot = NULL;
  _hotspots = 0;
  bar = NULL;
  bar_md = NULL;
  bar_mo = NULL;
  bar_na = NULL;
  _notify = NULL;
  behave = NULL;
  command = NULL;
  tooltip = NULL;
  tipper = NULL;
  in_move = 0;
  last_mo = -1;
  click_spot = -1;
  spot_active = NULL;
  custom_bg = 0;
}

///////////////////////////////////////////////////////////////////////////

IconBar::~IconBar() {
  llimg_release_llimg (bar);
  llimg_release_llimg (bar_mo);
  llimg_release_llimg (bar_md);
  llimg_release_llimg (bar_na);
  delete [] _hotspot;
  delete [] behave;
  delete [] spot_active;
  delete [] command;
  delete [] tooltip;
  delete tipper;
}

///////////////////////////////////////////////////////////////////////////

void IconBar::create (HINSTANCE hInstance){
  WNDCLASSEX wcl;
  wcl.cbSize = sizeof (WNDCLASSEX);
  wcl.hInstance = hInstance;
  wcl.lpszClassName = "iconbar";
  wcl.lpfnWndProc = main_wproc;
  wcl.style = CS_DBLCLKS;
  wcl.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_APP));
  wcl.hIconSm = NULL;
  wcl.hCursor = LoadCursor (NULL, IDC_ARROW);
  wcl.lpszMenuName = NULL;
  wcl.cbClsExtra = 0;
  wcl.cbWndExtra = 0;
  wcl.hbrBackground = (HBRUSH) GetStockObject (GRAY_BRUSH);
  RegisterClassEx (&wcl);
  
  hw_main = CreateWindowEx (WS_EX_TOPMOST,
    "iconbar", "osiva icon bar", WS_POPUP,
    100, 100, 200, 200, NULL,
    NULL, hInstance, NULL);
  if (!hw_main)
    showLastSysError ("CreateWindow");
  else
    SetWindowLong (hw_main, GWL_USERDATA, (LONG) this);

  DragAcceptFiles (hw_main, TRUE);

}

///////////////////////////////////////////////////////////////////////////

void IconBar::
set_callback (int client,
              void (*notify)
              (int client, int type, UINT, WPARAM, LPARAM)) {
  _notify = notify;
  _client = client;
}

///////////////////////////////////////////////////////////////////////////

void IconBar::set_images (int IDR_bar, int IDR_barmo,
                          int IDR_barmd, int IDR_barna) {

  HRSRC hrsrc = FindResource (NULL, MAKEINTRESOURCE(IDR_bar), "GIF");
  HGLOBAL hglobal = LoadResource (NULL, hrsrc);
  unsigned char * img_data = (unsigned char *) LockResource (hglobal);
  int img_data_sz = SizeofResource (NULL, hrsrc);
  bar = expandGif (img_data, img_data_sz);
  if (!bar) {
    MessageBox (0, "Failed to decode bar image", "IconBar", 0);
    return;
  }

  hrsrc = FindResource (NULL, MAKEINTRESOURCE(IDR_barmo), "GIF");
  hglobal = LoadResource (NULL, hrsrc);
  img_data = (unsigned char *) LockResource (hglobal);
  img_data_sz = SizeofResource (NULL, hrsrc);
  bar_mo = expandGif (img_data, img_data_sz);
  if (!bar) {
    MessageBox (0, "Failed to decode barmo image", "IconBar", 0);
    return;
  }

  hrsrc = FindResource (NULL, MAKEINTRESOURCE(IDR_barmd), "GIF");
  hglobal = LoadResource (NULL, hrsrc);
  img_data = (unsigned char *) LockResource (hglobal);
  img_data_sz = SizeofResource (NULL, hrsrc);
  bar_md = expandGif (img_data, img_data_sz);
  if (!bar) {
    MessageBox (0, "Failed to decode barmd image", "IconBar", 0);
    return;
  }

  hrsrc = FindResource (NULL, MAKEINTRESOURCE(IDR_barna), "GIF");
  hglobal = LoadResource (NULL, hrsrc);
  img_data = (unsigned char *) LockResource (hglobal);
  img_data_sz = SizeofResource (NULL, hrsrc);
  bar_na = expandGif (img_data, img_data_sz);
  if (!bar) {
    MessageBox (0, "Failed to decode barna image", "IconBar", 0);
    return;
  }

  bar_bgi = find_bg_index (bar);
  bar_mo_bgi = find_bg_index (bar_mo);
  bar_md_bgi = find_bg_index (bar_md);

  bar_red = bar->color[bar_bgi].red;
  bar_green = bar->color[bar_bgi].green;
  bar_blue = bar->color[bar_bgi].blue;

  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  MoveWindow (hw_main, (scrn.right-bar->width)/2, 0,
    bar->width, bar->height, FALSE);
  ShowWindow (hw_main, SW_SHOW);
  InvalidateRect (hw_main, NULL, TRUE);
  UpdateWindow (hw_main);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::set_bar_color (int red, int green, int blue) {

  if (bar_bgi >= 0) {
    bar->color[bar_bgi].red = red;
    bar->color[bar_bgi].green = green;
    bar->color[bar_bgi].blue = blue;
  }
  if (bar_mo_bgi >= 0) {
    bar_mo->color[bar_mo_bgi].red = red;
    bar_mo->color[bar_mo_bgi].green = green;
    bar_mo->color[bar_mo_bgi].blue = blue;
  }
  if (bar_mo_bgi >= 0) {
    bar_md->color[bar_md_bgi].red = red;
    bar_md->color[bar_md_bgi].green = green;
    bar_md->color[bar_md_bgi].blue = blue;
  }
  custom_bg = 1;
  InvalidateRect (hw_main, NULL, TRUE);
  UpdateWindow (hw_main);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::reset_bar_color () {
  set_bar_color (bar_red, bar_green, bar_blue);
  custom_bg = 0;
  InvalidateRect (hw_main, NULL, TRUE);
  UpdateWindow (hw_main);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::set_hotspots (int hotspots, RECT *hotspot){
  delete [] _hotspot;
  _hotspot = new RECT [hotspots];
  memcpy (_hotspot, hotspot, hotspots * sizeof(RECT));
  _hotspots = hotspots;
  behave = new int [_hotspots];
  memset (behave, 0, _hotspots * sizeof(int));
  command = new int [_hotspots];
  memset (command, 0, _hotspots * sizeof(int));
  spot_active = new int [_hotspots];
  for (int i = 0; i < hotspots; i++)
    spot_active[i] = 1;
  tooltip = new SmlStr [_hotspots];
  tipper = new ToolTip;
  tipper->init (hw_main, 15, "Verdana");
  InvalidateRect (hw_main, NULL, FALSE);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::set_repeating (int spot){
  if (spot < _hotspots)
    behave[spot] |= BE_REPEATS;
}

///////////////////////////////////////////////////////////////////////////

void IconBar::set_isdrag (int spot){
  if (spot < _hotspots)
    behave[spot] |= BE_DRAG;
}

///////////////////////////////////////////////////////////////////////////

void IconBar::set_command (int spot, int _command){
  if (spot < _hotspots)
    command[spot] = _command;
}

///////////////////////////////////////////////////////////////////////////

void IconBar::set_tooltip (int spot, char *tip){
  if (spot < 0)
    return;
  if (spot < _hotspots)
    tooltip[spot] = tip;
}

///////////////////////////////////////////////////////////////////////////

void IconBar::enable_tips () {
  tipper->enable();
}

///////////////////////////////////////////////////////////////////////////

void IconBar::disable_tips () {
  tipper->disable();
}

///////////////////////////////////////////////////////////////////////////

void IconBar::enable_spot (int spot) {
  spot_active[spot] = 1;
  RECT rect = _hotspot[spot];
  InflateRect (&rect, 1, 1);
  InvalidateRect (hw_main, &rect, FALSE);
  UpdateWindow (hw_main);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::disable_spot (int spot) {
  spot_active[spot] = 0;
  RECT rect = _hotspot[spot];
  InflateRect (&rect, 1, 1);
  InvalidateRect (hw_main, &rect, FALSE);
  UpdateWindow (hw_main);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::set_ondown (int spot){
  if (spot < _hotspots)
    behave[spot] |= BE_ONDOWN;
}

///////////////////////////////////////////////////////////////////////////

void IconBar::refresh_bar (HDC hdc) {
  int spot;
  LLIMG *img = NULL;
  RECT rect;
  for (spot = 0; spot < _hotspots; spot++) {
    if (spot_active[spot] || custom_bg)
      img = bar;
    else
      img = bar_na;
    rect = _hotspot[spot];
    paintImage (hw_main, hdc, img, &rect);
  }
}

///////////////////////////////////////////////////////////////////////////

void IconBar::paint_hotspot (int spot, int state) {
  if (!spot_active[spot])
    return;
  LLIMG *img;
  switch (state) {
  case NORMAL:
    img = bar;
    break;
  case MOUSE_OVER:
    img = bar_mo;
    break;
  case MOUSE_DOWN:
    img = bar_md;
    break;
  }
  HDC hdc = GetDC (hw_main);
  paintImage (hw_main, hdc, img, &(_hotspot[spot]));
  ReleaseDC (hw_main, hdc);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_kill_focus () {
  if (click_spot >= 0 || click_spot < _hotspots) {
    if (behave) {
      if (behave[click_spot] & BE_ONDOWN) {
        paint_hotspot (click_spot, NORMAL);
        click_spot = -1;
      }
    }
  }
  in_move = 0;
  ReleaseCapture ();
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_mouse_move (int cli_x, int cli_y) {
  POINT pt;
  RECT rect;
  pt.x = cli_x;
  pt.y = cli_y;

  // Valid click_spot means the mouse button is down
  if (click_spot >= 0)
    return;

  // Move the window, if moving is active
  if (in_move)
  {
    ClientToScreen (hw_main, &pt);
    if (pt.x == base_pt.x && pt.y == base_pt.y)
      return;
    GetWindowRect (hw_main, &rect);
    int x = pt.x - click_pt.x;
    int y = pt.y - click_pt.y;
    if (y < 5)
      y = 0;
    MoveWindow (hw_main, x, y,
      rect.right - rect.left, rect.bottom - rect.top, TRUE);
    base_pt = pt;
    return;
  }
  
  // Otherwise search for a spot undr the pointer
  for (int spot = 0; spot < _hotspots; spot++) {
    if (PtInRect (&(_hotspot[spot]), pt))
      break;
  }
  
  // If the pointer is still in the same spot don't do anything
  if (spot < _hotspots && spot == last_mo)
    return;
  
  // If the pointer is in a new hotspot light it up
  if (spot < _hotspots){
    if (last_mo >= 0)
      paint_hotspot (last_mo, NORMAL);
    paint_hotspot (spot, MOUSE_OVER);
    last_mo = spot;
    tipper->set (tooltip[spot]);
    SetCapture (hw_main);
    // Arrange to extinquish the lamp
    SetTimer (hw_main, LAMP_TIMER, 3000, NULL);
    // Arrange to display the tooltip
    SetTimer (hw_main, TIP_TIMER, 950, NULL);
    return;
  } // if the pointer is in a hotspot
  
  // If the pointer is not in a hotspot let go
  if (last_mo >= 0) {
    paint_hotspot (last_mo, NORMAL);
    last_mo = -1;
    KillTimer (hw_main, LAMP_TIMER);
    KillTimer (hw_main, TIP_TIMER);
    tipper->hide ();
    ReleaseCapture ();
  }
  
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_left_click (int cli_x, int cli_y) {
  POINT pt;
  pt.x = cli_x;
  pt.y = cli_y;

  // If they clicked they don't need tips
  tipper->hide();
  KillTimer (hw_main, TIP_TIMER);

  for (int spot = 0; spot < _hotspots; spot++) {
    if (PtInRect (&(_hotspot[spot]), pt))
      break;
  }
  if (spot < _hotspots){
    if ((behave[spot] & BE_DRAG) == 0) {
      KillTimer (hw_main, LAMP_TIMER);
      if (last_mo >= 0)
        paint_hotspot (last_mo, NORMAL);
      paint_hotspot (spot, MOUSE_DOWN);
      click_spot = spot;
      if ((behave[click_spot]&BE_REPEATS) && _notify) {
        _notify (_client, ICON_SELECTED, 0, 0, command[click_spot]);
        SetTimer(hw_main, REPEAT_TIMER, 1, NULL);
      }
      if ((behave[click_spot]&BE_ONDOWN) && _notify) {
        _notify (_client, ICON_SELECTED, 0, 0, command[click_spot]);
        paint_hotspot (click_spot, NORMAL);
        click_spot = -1;
      }
      return;
    }
  }
  
  // Otherwise the iconbar is being moved
  click_pt = pt;
  base_pt = pt;
  ClientToScreen (hw_main, &base_pt);
  SetCapture (hw_main);
  in_move = 1;
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_left_unclick (int cli_x, int cli_y, WORD fwKeys) {
  ReleaseCapture ();
  POINT pt;
  pt.x = cli_x;
  pt.y = cli_y;
  for (int spot = 0; spot < _hotspots; spot++) {
    if (PtInRect (&(_hotspot[spot]), pt))
      break;
  }
  WORD wparam = LEFT_BUTTON;
  if (fwKeys & MK_SHIFT) 
    wparam = SHIFT_LEFT;
  if (spot < _hotspots){  
    if (spot == click_spot && 
        spot_active[spot] &&
        !(behave[spot]&BE_REPEATS) &&
        !(behave[spot]&BE_ONDOWN) ) {
      if (_notify) 
        _notify (_client, ICON_SELECTED, 0, wparam, command[spot]);
    }
  }

  if (click_spot >=0 )
    paint_hotspot (click_spot, NORMAL);
  click_spot = -1;
  in_move = 0;
  KillTimer (hw_main, LAMP_TIMER);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_right_click (int cli_x, int cli_y) {
  POINT pt;
  pt.x = cli_x;
  pt.y = cli_y;

  // If they clicked they don't need tips
  tipper->hide();
  KillTimer (hw_main, TIP_TIMER);

  for (int spot = 0; spot < _hotspots; spot++) {
    if (PtInRect (&(_hotspot[spot]), pt))
      break;
  }
  if (spot < _hotspots){  
    KillTimer (hw_main, LAMP_TIMER);
    if (last_mo >= 0)
      paint_hotspot (last_mo, NORMAL);
    paint_hotspot (spot, MOUSE_DOWN);
    click_spot = spot;
    if ((behave[click_spot]&BE_REPEATS) && _notify) {
      _notify (_client, ICON_SELECTED, 0, 0, command[click_spot]);
      SetTimer(hw_main, REPEAT_TIMER, 1, NULL);
    }
    return;
  }
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_right_unclick (int cli_x, int cli_y, WORD fwKeys) {
  ReleaseCapture ();
  POINT pt;
  pt.x = cli_x;
  pt.y = cli_y;
  for (int spot = 0; spot < _hotspots; spot++) {
    if (PtInRect (&(_hotspot[spot]), pt))
      break;
  }
  WORD wparam = RIGHT_BUTTON;
  if (fwKeys & MK_SHIFT) 
    wparam = SHIFT_RIGHT;
  if (spot < _hotspots){  
    if (spot == click_spot && !(behave[spot]&BE_REPEATS)) {
      if (_notify) 
        _notify (_client, ICON_SELECTED, 0, wparam, command[spot]);
    }
  }
  else {
    ShowWindow (hw_main, SW_MINIMIZE);
  }
  if (click_spot >=0 )
    paint_hotspot (click_spot, NORMAL);
  click_spot = -1;
  KillTimer (hw_main, LAMP_TIMER);
}


///////////////////////////////////////////////////////////////////////////

void IconBar::handle_timer (WPARAM wparam, LPARAM lparam) {
  
  switch (wparam) {
    
  case LAMP_TIMER:
    if (last_mo >= 0) {
      paint_hotspot (last_mo, NORMAL);
      KillTimer (hw_main, LAMP_TIMER);
      tipper->hide();
    }  
    break;
    
  case REPEAT_TIMER:
    if (click_spot >= 0 ) {
      if ((behave[click_spot]&BE_REPEATS) && _notify) {
        _notify (_client, ICON_SELECTED, 0, 0, command[click_spot]);
      }
    }  
    break;
      
  case TIP_TIMER:
    tipper->show();
    KillTimer (hw_main, TIP_TIMER);
    break;
    
  default:
    if (_notify)
      _notify (_client, WM_MESSAGE, WM_TIMER, wparam, lparam);
    break;
    
  } // switch on wparam, the TIMER ID
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_dde_initiate (WPARAM wparam, LPARAM lparam) {
  char server[80];
  int lens = GlobalGetAtomName (LOWORD(lparam), server, 40);
  char topic[80];
  int lent = GlobalGetAtomName (HIWORD(lparam), topic, 40);
  if (strcmp(server, "osiva"))
    return;
  if (strcmp(topic, "open"))
    return;

  ATOM atomApplication;
  ATOM atomTopic;
  // Following from MSDN library: Initiating a Conversation
  if ((atomApplication = GlobalAddAtom("osiva")) != 0) 
  { 
    if ((atomTopic = GlobalAddAtom("open")) != 0) 
    { 
      SendMessage((HWND) wparam, 
        WM_DDE_ACK, 
        (WPARAM) hw_main, 
        MAKELONG(atomApplication, atomTopic)); 
      GlobalDeleteAtom(atomApplication); 
    }    
    GlobalDeleteAtom(atomTopic); 
  } 
  if ((atomApplication == 0) || (atomTopic == 0)) 
  { 
    // Handle errors.     
  } 
 
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_dde_execute (WPARAM wparam, LPARAM lparam) {
  char *command = (char *) GlobalLock ( (void *)lparam);
  char *fname = strdup (command);
  GlobalUnlock ((void *)lparam);
  if (strcmp (fname, "ignore"))
    _notify (_client, DDE_FILENAME, 0, 0, (LPARAM) fname);
  free (fname);
  
  DDEACK DdeAck;
  DdeAck.bAppReturnCode = 0 ;
  DdeAck.reserved       = 0 ;
  DdeAck.fBusy          = FALSE ;
  DdeAck.fAck           = TRUE ;

  WORD wStatus = * (WORD *) & DdeAck ;
  LPARAM lp = MAKELONG (wStatus, lparam);
  PostMessage ((HWND)wparam, WM_DDE_ACK, (WPARAM)hw_main, lp);

}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_dde_terminate (WPARAM wparam, LPARAM lparam) {
  PostMessage ((HWND)wparam, WM_DDE_TERMINATE, (WPARAM)hw_main, 0);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_copydata (WPARAM wparam, LPARAM lparam) {
  COPYDATASTRUCT *pcds = (COPYDATASTRUCT *) lparam;
  char *filename = strdup ((char *)(pcds->lpData));
  PostMessage (hw_main, WM_USER_OPEN, 0, (LPARAM)filename); 
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_user_open (WPARAM wparam, LPARAM lparam) {
  if (_notify)
    _notify (_client, DDE_FILENAME, 0, 0, lparam);
  free ((void *)lparam);
}

///////////////////////////////////////////////////////////////////////////

void IconBar::handle_drop (WPARAM wparam, LPARAM lparam){
  int f;
  char filename[512];
  HDROP hdrop = (HDROP) wparam;
  int files = DragQueryFile (hdrop, 0xFFFFFFFF, 0, 0);
  for (f = 0; f < files; f++) {
    DragQueryFile (hdrop, f, filename, 512);
    if (_notify)
      _notify (_client, DDE_FILENAME, 0, 0, (LPARAM)filename);
  }
  DragFinish (hdrop);   
}

///////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK IconBar::
main_wproc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  IconBar *ib = (IconBar *) GetWindowLong (hwnd, GWL_USERDATA);
  
  switch (message)
  {
  case WM_SETCURSOR:
    break;

  case WM_TIMER:
    ib->handle_timer(wParam, lParam);
    return 0;
    
  case WM_LBUTTONDOWN:
    ib->handle_left_click ((int)(short)LOWORD(lParam),
      (int)(short)HIWORD(lParam));
    return 0;

  case WM_RBUTTONDOWN:
    ib->handle_right_click ((int)(short)LOWORD(lParam),
      (int)(short)HIWORD(lParam));
    return 0;
    
  case WM_MOUSEMOVE:
    ib->handle_mouse_move ((int)(short)LOWORD(lParam),
      (int)(short)HIWORD(lParam));
    return 0;
    
  case WM_LBUTTONUP:
    ib->handle_left_unclick ((int)(short)LOWORD(lParam),
      (int)(short)HIWORD(lParam), wParam);
    return 0;
       
  case WM_RBUTTONUP:
    ib->handle_right_unclick ((int)(short)LOWORD(lParam),
      (int)(short)HIWORD(lParam), wParam);
    return 0;
    
  case WM_KILLFOCUS:
    ib->handle_kill_focus();
    return 0;
    
  case WM_MOUSEACTIVATE:
    return 0;

  case WM_APP:
  case WM_CHAR:
  case WM_KEYDOWN:
  case WM_COMMAND:
    ib->_notify (ib->_client, WM_MESSAGE, message, wParam, lParam);
    return 0;
    
  case WM_COPYDATA:
    ib->handle_copydata (wParam, lParam);
    return 0;

  case WM_USER_OPEN:
    ib->handle_user_open (wParam, lParam);
    return 0;
        
  case WM_DROPFILES:
    ib->handle_drop (wParam, lParam);
    return 0;
    
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint (hwnd, &ps);
      ib->refresh_bar (hdc);
      EndPaint (hwnd, &ps);
    }
    return 0;
    
  }
  
  return DefWindowProc (hwnd, message, wParam, lParam);
  
}
