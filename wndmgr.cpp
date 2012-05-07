/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * wndmgr.cpp is part of Osiva.
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
// File: wndmgr.cpp
//
///////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>    // For _msize()
#include <windows.h>

#include "ll_image.h"  // The image structure (DIB like)
#include "snapshotw.h"

#include "iconbar.h"   // The window manager has an Icon Bar
#include <vector>
using namespace std;
#include "wndmgr.h"

#include <sys/types.h> // Used to seed random window distribution
#include <sys/timeb.h> // Used to seed random window distribution
#include <process.h>   // For getpid

#include "resource.h"

#include "ooptions.h"

#include "hotspot.h"
#include "dialogs.h"

#include <ddraw.h>        // For WaitFor VerticalBlank()
#include "ddproxy.h"

#include "cmdcodes.h"     // The icon-bar command possibilities

// The iconbar acts as the messaging window for WndMgr
static const int DISSOLVE_TIMER = (IconBar::CLIENT_TIMER + 1);

// Message received from ooptions
static const int OPTIONS_CHANGED = 1;

// Layout file header and "magic"
static const char const *layout_header = "Osiva layout file...";



///////////////////////////////////////////////////////////////////////////
// GLOBAL Scope

extern OOptions *ooptions;
extern HWND hw_help;
extern FlipDialog *flip_dialog;
extern TransDialog *trans_dialog;


  /////////
// WndMgr //////////////////////////////////
/////////

WndMgr::
WndMgr() {
  frame = NULL;
  screen = NULL;
  future = NULL;
  reduction = 8;
  PID = _getpid();
  in_menu = 0;
  in_close = 0;
  exe_path = NULL;
  ddproxy = NULL;
  menu_icon_left = 0;
}

  /////////
// WndMgr //////////////////////////////////
/////////

WndMgr::
~WndMgr() {
  for ( int i = 0; i < snapwin.size() ; i++ )
    delete snapwin[i];
  delete [] exe_path;
  if (ddproxy)
    delete ddproxy;
  llimg_release_llimg (frame);
  llimg_release_llimg (screen);
  llimg_release_llimg (future);
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::
init(HINSTANCE hInstance) {

  // Setup the icon bar
  
  // The following are measured from the iconbar.gif resource
  static int hs_start[] = {
    0, 24, 
    52, 84, 108, 132, 156,
    186, 216, 240, 264, 288, 312,
    347, 372,
    396, 437, 477, 504
  };
  static int hs_end[] = {
    23, 51, 
    83, 107, 131, 155, 185,
    215, 239, 263, 287, 311, 346, 
    371, 395,
    436, 476, 503, 527
  };
  int hotspots = 19;
  static char *hs_tok[] = {
      "tab",
      "tile",
      "size-screen",
      "size-1:1",
      "size-bigger",
      "size-smaller",
      "size-1:8",
      "push-apart",
      "pull-together",
      "distribute",
      "collect",
      "center",
      "corner",
      "rotate",
      "transparent",
      "menu",
      "drag",
      "minimize",
      "close",
      0
  };
  
  _hInstance = hInstance;
  iconbar.create (_hInstance);
  iconbar.set_images (IDR_BAR, IDR_BAR_MO, IDR_BAR_MD, IDR_BAR_NA);
  LLIMG *bar = iconbar.get_image();
  if (!bar)
    return;
  RECT *hotspot = new RECT [hotspots];
  for (int i = 0; i < hotspots; i++) {
    hotspot[i].left = hs_start[i];
    hotspot[i].top = 0;
    hotspot[i].right = hs_end[i];
    hotspot[i].bottom = bar->height-1;
  }
  
  iconbar.set_hotspots (hotspots, hotspot);
  
  int spot, cmd;
  for (spot = 0; spot < hotspots; spot++) {
    for (cmd = 0; CC_command_name[cmd]; cmd++) {
      if (!strcmp (hs_tok[spot], CC_command_name[cmd]) ) break;
    }
    if (!CC_command_name[cmd]) {
      MessageBox (NULL, hs_tok[spot], "No such command", MB_OK);
      continue;
    }
    iconbar.set_command (spot, cmd);
    iconbar.set_tooltip (spot, CC_tooltip[cmd]);
    if (CC_flags[cmd] & CMDCODE_MSK_REPEATS)
      iconbar.set_repeating (spot);
    if (CC_flags[cmd] & CMDCODE_MSK_ONDOWN)
      iconbar.set_ondown (spot);
    if (CC_flags[cmd] & CMDCODE_MSK_DRAGAREA)
      iconbar.set_isdrag (spot);
    if (cmd == CMDCODE_MENU)
      menu_icon_left = hotspot[spot].left;
    // Remember the icons that need more than one window
    if (cmd == CMDCODE_TILE)
      group_icon.push_back (spot);
    if (cmd == CMDCODE_TAB)
      group_icon.push_back (spot);
    if (cmd == CMDCODE_BACKSPACE)
      group_icon.push_back (spot);
  }

  iconbar.set_callback ((int)this, iconbar_notify);

  hmenu = GetSubMenu (
    LoadMenu (_hInstance, MAKEINTRESOURCE(IDR_MENU)), 0);
  
  ooptions->subscribe (iconbar.get_hwnd(), OPTIONS_CHANGED);
  
  for (i = 0; i < group_icon.size(); i++)
    iconbar.disable_spot (group_icon[i]);
  
  if (ooptions->ontop) {
    CheckMenuItem (hmenu, ID_STAYONTOP, MF_CHECKED);
  }
  if (ooptions->right_click_closes) {
    CheckMenuItem (hmenu, ID_RIGHTCLICKCLOSES, MF_CHECKED);
  }
  
  
  delete [] hotspot;
}

  /////////
// WndMgr //////////////////////////////////
/////////

HWND WndMgr::
new_window (HINSTANCE hInstance, LPSTR lpCmdLine, 
            int nCmdShow) {

  SnapShotW *ssw = new SnapShotW;
  ssw->set_wndmgr (this);
  ssw->init (hInstance, lpCmdLine, nCmdShow);
  snapwin.push_back(ssw);
  if (snapwin.size() > 1) {
    for (int i = 0; i < group_icon.size(); i++)
      iconbar.enable_spot (group_icon[i]);
  }
  return ssw->get_hwnd();
  
}
  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::
close_window (SnapShotW * sswindow) {
  int in_logo = sswindow->get_in_logo();
  vector<class SnapShotW *>::iterator p;
  p = snapwin.begin();
  while (p != snapwin.end()) {
    if ( *p == sswindow ) break;
    p++;
  }
  // Cancel dissolve in case it is in process
  dissolve_clear();
  // Bury the window before killing it
  HWND xwnd = (*p)->get_hwnd();
  SetWindowPos (xwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
  HWND hwnd = find_top_window();
  in_close = 1;
  SetForegroundWindow (hwnd);   
  DestroyWindow(xwnd);
  in_close = 0;
  delete *p;
  p = snapwin.erase(p);
  if (snapwin.empty()) {
    if (!ooptions->stayup && !in_logo)
      PostQuitMessage(0);
    return;
  }
  // Restart the dissolve
  if (ooptions->dissolve && ooptions->flip)
    dissolve_init();
  // Disable group icons if necessary
  if (snapwin.size() < 2) {
    for (int i = 0; i < group_icon.size(); i++)
      iconbar.disable_spot (group_icon[i]);
  }
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::tab (SnapShotW *sswindow, int activate, int reverse) {
  if (sswindow == NULL ) {
    HWND hw = find_top_window();
    if (hw) 
      sswindow = (SnapShotW *) GetWindowLong (hw, GWL_USERDATA);   
  }

  SHORT state = GetKeyState (VK_SHIFT);
  int shifted = state & 0x8000?1:0;
  int i;
  for (i = 0; i < snapwin.size(); i++ )
    if (snapwin[i] == sswindow)
      break;
  if (i == snapwin.size())
    return;
  if (!shifted && !reverse) {
    i++;
    if (i == snapwin.size())
      i = 0;
  }
  else {
    i--;
    if (i < 0)
      i = snapwin.size() - 1;
  }

  HWND hwnd = snapwin[i]->get_hwnd();

  if (activate) {
    SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0,
      SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
  }
  else {
    SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0,
      SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
  }
  InvalidateRect (hwnd, NULL, FALSE);
  // if (ddproxy)
  //   ddproxy->wait_for_vertical_blank();
  UpdateWindow (hwnd);
  
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::all_small (SnapShotW *exclude){
  // Cancel dissolve in case it is in process
  dissolve_clear();
  SnapShotW *ssw;
  for (int i = 0; i < snapwin.size(); i++) {
    ssw = snapwin[i];
    if (ssw == exclude)
      continue;
    ssw->show_small_img (-1, -1, reduction);
    // KLUDGE: reduction lives in 3 places, ooptions, wndmgr, and snapshotw
    ooptions->reduction = reduction;
  }
  // Restart the dissolve
  if (ooptions->dissolve && ooptions->flip)
    dissolve_init();
}


  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::all_rotate (SnapShotW *exclude, int clockwise){
  // Cancel dissolve in case it is in process
  dissolve_clear();
  SnapShotW *ssw;
  for (int i = 0; i < snapwin.size(); i++) {
    ssw = snapwin[i];
    if (ssw == exclude)
      continue;
    ssw->rotate (clockwise);
    // KLUDGE: reduction lives in 3 places, ooptions, wndmgr, and snapshotw
    ooptions->reduction = reduction;
  }
  // Restart the dissolve
  if (ooptions->dissolve && ooptions->flip)
    dissolve_init();
}


  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::all_big (){
  // Cancel dissolve in case it is in process
  dissolve_clear();
  for (int i = 0; i < snapwin.size(); i++) {
    RECT rect;
    GetWindowRect (snapwin[i]->get_hwnd(), &rect);
    int x = (rect.left + rect.right)/2;
    int y = (rect.top + rect.bottom)/2;
    snapwin[i]->show_centered_img (x, y);
  }
  // Restart the dissolve
  if (ooptions->dissolve && ooptions->flip)
    dissolve_init();
}


  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::fit_top_window (int how){
  
  HWND hwnd = find_top_window();
  if (!hwnd) return;
  SnapShotW * ssw;
  ssw = (SnapShotW *) GetWindowLong (hwnd, GWL_USERDATA);
  RECT rect;
  GetWindowRect (hwnd, &rect);
  int area = (rect.right - rect.left) * (rect.bottom - rect.top);
  for (int i = 0; i < snapwin.size(); i++) {
    if (snapwin[i] == ssw)
      continue;
    switch (how) {
    case FIT_INSIDE:
      snapwin[i]->move_img (&rect, 1);
      break;
    case FIT_EXACTLY:
      snapwin[i]->move_img (&rect, 0);
      break;
    case MATCH_AREA:
      snapwin[i]->scale_to_area (area);
      break;
    } // switch on how
    BringWindowToTop (snapwin[i]->get_hwnd());
    UpdateWindow (snapwin[i]->get_hwnd());
    
  } // for each window in snapwin
}

  /////////
// WndMgr //////////////////////////////////
/////////

static int int_compare (const void *e1, const void *e2){
  return  *(int *)e1 - *(int *)e2;
}

int * WndMgr::random_index() {
  struct _timeb timebuffer;
  _ftime (&timebuffer);
  srand(timebuffer.millitm ^ timebuffer.time);
  rand();
  int *rindex = new int [snapwin.size()];
  for (int i = 0; i < snapwin.size(); i++)
    rindex[i] = (rand()<< 16)|i;
  qsort ((void *)rindex, snapwin.size(), sizeof(int), int_compare);
  for (i = 0; i < snapwin.size(); i++)
    rindex[i] &= 0xffff;
  return rindex;
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::center (SnapShotW *exclude){
  int *rindex = random_index();
  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  for (int i = 0; i < snapwin.size(); i++) {
    if (snapwin[rindex[i]] == exclude) continue;
    RECT rect;
    HWND hwnd = snapwin[rindex[i]]->get_hwnd();
    GetWindowRect (hwnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    int x = (scrn.right - w)/2;
    int y = (scrn.bottom - h)/2;
    SetWindowPos (hwnd, HWND_TOP, x, y, w, h, SWP_NOSIZE);
  }
  delete [] rindex;
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::corner (SnapShotW *exclude){
  reduction = 8;
  all_small(exclude);
  
  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  int xr = scrn.right;
  int yt = 0;
  for (int i = 0; i < snapwin.size(); i++) {
    if (snapwin[i] == exclude) continue;
    RECT rect;
    HWND hwnd = snapwin[i]->get_hwnd();
    GetWindowRect (hwnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    int x = xr - w;
    int y = yt;
    SetWindowPos (hwnd, HWND_TOP, x, y, w, h, SWP_NOSIZE);
  }
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::distribute (SnapShotW *exclude){
  int *rindex = random_index();
  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  RECT rect;
  HWND hwnd;
  int x, y, w, h, maxx, cl, cr, ct, cb, maxy;
  for (int i = 0; i < snapwin.size(); i++) {
    if (snapwin[rindex[i]] == exclude) continue;
    // Find a random point on the screen (with 16 pixel border)
    x = 16 + (rand() * (scrn.right - 32))/RAND_MAX;
    y = 16 + (rand() * (scrn.bottom - 32))/RAND_MAX;
    // Consider centering the image there
    hwnd = snapwin[rindex[i]]->get_hwnd();
    GetWindowRect (hwnd, &rect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;
    x -= w/2;
    y -= h/2;
    // Now make sure not too much of the image is clipped
    int denom = 8;
    int numer = denom-1;
    maxx = w/denom;
    cl = -x;
    cr = (x + w) - scrn.right;
    if (cl > maxx)
      x = -maxx;
    else if (cr > maxx)
      x = scrn.right - (numer*w)/denom;
    maxy = h/denom;
    ct = -y;
    cb = (y + h) - scrn.bottom;
    if (ct  > maxy)
      y = -maxy;
    else if (cb > maxy)
      y = scrn.bottom - (numer*h)/denom;

    SetWindowPos (hwnd, HWND_TOP, x, y, w, h, 0); 
  }
  delete [] rindex;
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::cluster (SnapShotW *exclude){

  // Randomly distribute the windows into a small area 

  SHORT state = GetKeyState (VK_SHIFT);
  int shifted = state & 0x8000?1:0;

  // The default is the top right quarter of the screen height
  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  int left, top, width, height;
  height = scrn.bottom/4 - 16;
  width = height;
  left = scrn.right - width - 16;
  top = 16;

  RECT rect;
  HWND hwnd;

  // If the shift key is held down, use the top window as the area
  if (shifted) {
    hwnd = find_top_window();
    if (!hwnd) return;
    SnapShotW * ssw;
    ssw = (SnapShotW *) GetWindowLong (hwnd, GWL_USERDATA);
    GetWindowRect (hwnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    left = rect.left;
    top = rect.top;
  }


  int *rindex = random_index();
  int x, y, w, h;
  for (int i = 0; i < snapwin.size(); i++) {
    if (snapwin[rindex[i]] == exclude) continue;
    x = left + (rand() * width)/RAND_MAX;
    y = top + (rand() * height)/RAND_MAX;
    hwnd = snapwin[rindex[i]]->get_hwnd();
    GetWindowRect (hwnd, &rect);
    w = rect.right - rect.left;
    h = rect.bottom - rect.top;
    x -= w/2;
    y -= h/2;
    SetWindowPos (hwnd, HWND_TOP, x, y, w, h, 0); 
  }
  delete [] rindex;
}


  /////////
// WndMgr //////////////////////////////////
/////////

// Distribute images within square tiles

// rect is (w, h), width and height
// tile distribution is (r, c), rows and cols, with n tiles
// the size of a tile is x
// (1) cx = w
// (2) rx = h
// (3) rc = n
// ------
// (1,2) w/c = h/r
// (1,2) r = hc/w
// (3) r = n/c
// ==> cc h/w = n
// ==> c = sqrt (n w/h)
// ------
// (1,2) c = wr/h
// (3)   c = n/r
// ==> n= wrr/h
// ==> rr = nh/w
// ==> r = sqrt (n h/w)

void WndMgr::tile_rect (RECT *rect, int lock_aspect, SnapShotW * ssw){
  int w = rect->right - rect->left + 1;
  int h = rect->bottom - rect->top + 1;
  int n = snapwin.size();
  if (!n)
    return;

  if (lock_aspect && n < 6)
    n = 6;

  double fc1 = n * w;
  fc1 /= h;
  fc1 = sqrt (fc1);
 
  int c1 = (int) ceil (fc1);
  double fr1 = n;
  fr1 /= c1;
  int r1 = (int) ceil (fr1);
  int xc1 = w/c1;
  int xr1 = h/r1;
  int x1 = xc1 < xr1? xc1 : xr1;
  
  int x2 = 0;
  int c2 = (int) fc1;
  if (c2) {
    double fr2 = n;
    fr2 /= c2;
    int r2 = (int) ceil (fr2);
    int xc2 = w/c2;
    int xr2 = h/r2;
    x2 = xc2 < xr2? xc2 : xr2;
  }
  
  int x, c;
  if ( x2 > x1 ) {
    x = x2;
    c = c2;
  }
  else {
    x = x1;
    c = c1;
  }
 
  if (x < 8)
    return;

  // Distibute extra space

  int rs = (n + c - 1)/ c;

  int xextra = w - (c * x);
  int yextra = h - (rs * x);

  int dx = xextra / (c + 1);
  int dy = yextra / (rs + 1);

  // ...

  int col;
  int row;
  int offx, offy;
  RECT dest;
  for (int i = 0; i < snapwin.size(); i++) {
    if (ssw) 
      if (snapwin[i] != ssw)
        continue;
    col = i % c;
    row = i / c;
    if (row == rs - 1) {
      int c0 = n % c;
      if (c0) {
        xextra = w - (c0 * x); 
        dx = xextra / (c0 + 1);
      }
    }
    offx = dx + (x + dx) * col;
    offy = dy + (x + dy) * row;
    dest.left = rect->left + offx;
    dest.top = rect->top + offy;
    dest.right = dest.left + x;
    dest.bottom = dest.top + x;
    snapwin[i]->move_img (&dest, lock_aspect);
    BringWindowToTop (snapwin[i]->get_hwnd());
    UpdateWindow (snapwin[i]->get_hwnd());
  }
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::
tile_screen (SnapShotW *ssw) {

  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  InflateRect (&scrn, -16, -16);
  tile_rect (&scrn, 1, ssw);

}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::
iconbar_notify (int client, int type, UINT u, WPARAM w, LPARAM l) {
  HWND hwnd, topwnd;
  SnapShotW *ssw;
  WndMgr *wm = (WndMgr *)client;
  
  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);
  InflateRect (&scrn, -16, -16);

  switch (type) {
    
  case IconBar::ICON_SELECTED:
    switch (l) {
    case CMDCODE_BIGGER:
      if (wm->reduction == 2)
        wm->all_big ();
      else {
        wm->reduction --;
        wm->all_small ();
      }
      break;
    case CMDCODE_SMALLER:
      if (wm->reduction < 10)
        wm->reduction ++;
      wm->all_small ();
      break;
    case CMDCODE_ROTATE:
      hwnd = wm->find_top_window();
      if (hwnd) {
        ssw = (SnapShotW *) GetWindowLong (hwnd, GWL_USERDATA);
        switch (w) {
        case IconBar::LEFT_BUTTON:
          ssw->rotate (0);
          break;
        case IconBar::SHIFT_LEFT:
        case IconBar::RIGHT_BUTTON:
          ssw->rotate (1);
          break;
        }
      }
      break;
    case CMDCODE_TAB:
      wm->tab (NULL);
      break;
    case CMDCODE_TILE:
      wm->tile_screen ();
      break;
    case CMDCODE_FITSCREEN:
      hwnd = wm->find_top_window();
      if (hwnd) {
        ssw = (SnapShotW *) GetWindowLong (hwnd, GWL_USERDATA);
        ssw->move_img (&scrn, 1); 
      }
      break;
    case CMDCODE_SPACE:
      wm->reduce_energy (0);
      break;
    case CMDCODE_BACKSPACE:
      wm->reduce_energy (0, ATTRACT);
      break;
    case CMDCODE_PGUP:
      wm->cluster();
      break;
    case CMDCODE_PGDWN:
      wm->distribute();
      break;
    case CMDCODE_HOME:
      wm->center();
      break;
    case CMDCODE_END:
      wm->corner();
      break;
    case CMDCODE_SINK:
      hwnd = wm->find_top_window();
      if (hwnd) {
        BringWindowToTop (hwnd);
        SetWindowPos (hwnd, HWND_BOTTOM, 0, 0, 0, 0,
          SWP_NOMOVE | SWP_NOSIZE);        
      }
      break;
    case CMDCODE_TRANS:
      hwnd = wm->find_top_window();
      if (hwnd) {
        ssw = (SnapShotW *) GetWindowLong (hwnd, GWL_USERDATA);
        ssw->toggle_trans();
      }
      break;
    case CMDCODE_R1:
      wm->all_big();
      break;
    case CMDCODE_R2:
    case CMDCODE_R3:
    case CMDCODE_R4:
    case CMDCODE_R5:
    case CMDCODE_R6:
    case CMDCODE_R7:
    case CMDCODE_R8:
    case CMDCODE_R9:
      wm->reduction = l - CMDCODE_R2 + 2;
      wm->all_small();
      break;
    case CMDCODE_MINIMIZE:
      topwnd = wm->find_top_window ();
      hwnd = wm->iconbar.get_hwnd();
      if (topwnd) {
        ShowWindow (hwnd, SW_SHOWMINNOACTIVE);
        BringWindowToTop (topwnd);
      }
      else {
        ShowWindow (hwnd, SW_MINIMIZE);
      }
      break;
    case CMDCODE_CLOSE:
      PostQuitMessage (0);
      break;
    case CMDCODE_MENU:
      wm->do_menu();
      break;
    } // switch on l, the icon index
    break;
    
    case IconBar::WM_MESSAGE:
      {
        switch (u) {
        case WM_APP:
          if (w == OPTIONS_CHANGED && l == OOptions::TRANSPARENCY) {
            hwnd = wm->find_top_window();
            if (hwnd) {
              ssw = (SnapShotW *) GetWindowLong (hwnd, GWL_USERDATA);
              ssw->apply_trans ();
            }
          }
          else if (w == OPTIONS_CHANGED && l == OOptions::FLIP) {
            if (ooptions->flip)
              wm->set_flip (FLIP_ON);
            else 
              wm->set_flip (FLIP_OFF);
          }
          else if (w == OPTIONS_CHANGED && l == OOptions::FLIP_NOW) {
            wm->tab (0, 0);            
          }
          else if (w == OPTIONS_CHANGED && l == OOptions::FLIP_NOW_R) {
            wm->tab (0, 0, 1);            
          }
          break;
        case WM_COMMAND:
          wm->handle_command (w, l);
          break;
        case WM_TIMER:
          wm->handle_timer (w, l);
          break;
        case WM_KEYDOWN:
          hwnd = wm->find_top_window();
          if (hwnd)
            PostMessage (hwnd, u, w, l);
          break;
        } // switch on U - THE WM_message code (WPARAM)
      }
      break;
      
    case IconBar::DDE_FILENAME:
      {
        int lfile = wm->is_layout_file ((char *)l);
        if (lfile) 
          wm->load_layout_file ((char *)l);
        else
          wm->new_window (wm->_hInstance, (LPSTR) l, SW_SHOW);
      }
      break;
  } // switch on type 
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::reduce_energy(int vKey, forceType force) {
  // Force is applied while a key is held down
  // The key is indicated by the Win32 Virtual-Key Code
  static int dx[] = {0,  0,  0,  -1,  1};
  static int dy[] = {0,  1, -1,   0,  0};
  RECT scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &scrn, 0);

  // Fmin and dmin are named for a repulsive force
  // For an attractive force they are actually max values

  int i, j, d, dmin;
  int dist2;

  // An unsigned long can count to 4,294,967,295
  // An _int64 can count to 9,223,372,036,854,775,807
  // This models an inverse square force, 1/(d*d)
  // d are in pixels, so the num can easily be 50,000,000
  // The bigger "one" is, the farther the force can be felt
  // But the sum will overflow if there are a lot of particles

  unsigned long F, Fmin;
  unsigned long one = 60000000;
 
  int rx, ry;  // Reference point of current window 
  int tx, ty;  // Current window test point
  int x, y;    // Center point of the other window(s)
  int n = snapwin.size() - 1;
  RECT rrect;
  RECT rect;
  int iterations = 5;
  int its;
  int changed;
  // Give the windows a chance to repaint
  if (GetQueueStatus(QS_PAINT))
    return;
  for ( its = 0; its < iterations ; its++) {
    // Stop processing if the key is released
    if (vKey){
      if ( !(GetAsyncKeyState (vKey) & 0x8000) )
        break;
    }
    changed = 0;
    for ( j = 0; j < snapwin.size(); j++) {
      GetWindowRect (snapwin[j]->get_hwnd(), &rrect);
      rx = (rrect.left + rrect.right)/2;
      ry = (rrect.top + rrect.bottom)/2;
      Fmin = (force == ATTRACT?0:0xffffffff);
      dmin = 0;
      for ( d = 0; d < 5; d++ ) {
        F = 0;
        tx = rx + dx[d];
        ty = ry + dy[d];
        for ( i = 0; i < snapwin.size(); i++ ) {
          if ( i == j )
            continue;
          GetWindowRect (snapwin[i]->get_hwnd(), &rect);
          x = (rect.left + rect.right)/2 - tx;
          y = (rect.top + rect.bottom)/2 - ty;
          dist2 = x*x + y*y;
          if (dist2)
            F += one / dist2;
          else
            F += one + 1;
        } // For each window [i] except the current one
        if (force == REPULSE) {
          // Add in a repulsion from the screen edges
          dist2 = tx?tx:1;
          F += one / (dist2 * dist2);
          dist2 = ty?ty:1;
          F += one / (dist2 * dist2);
          dist2 = (tx == scrn.right?1:scrn.right - tx);
          F += one / (dist2 * dist2);
          dist2 = (ty == scrn.bottom?1:scrn.bottom - ty);
          F += one / (dist2 * dist2);
          if (F < Fmin) {
              Fmin = F;
              dmin = d;
          }
        }
        else {
          if (F > Fmin) {
            Fmin = F;
            dmin = d;
          }
        }
      } // For each test point 
      if (dmin) {
        MoveWindow (snapwin[j]->get_hwnd(),
          rrect.left + dx[dmin],
          rrect.top + dy[dmin],
          rrect.right - rrect.left,
          rrect.bottom - rrect.top,
          TRUE);
        changed++;
      }
    } // Using each window as the reference window
    if (!changed)
      break;
  } // For each iteration on this entry (1 entry per key)
}

  /////////
// WndMgr //////////////////////////////////
/////////

HWND WndMgr::find_top_window() {
  HWND hwnd = snapwin[0]->get_hwnd();
  DWORD wpid;
  char buff[32];
  hwnd = GetWindow (hwnd, GW_HWNDFIRST);
  for ( ; hwnd; hwnd = GetWindow (hwnd, GW_HWNDNEXT )) {
    GetWindowThreadProcessId (hwnd, &wpid);
    if (wpid == PID ) {
      GetWindowText (hwnd, buff, 31);
      if (!strcmp(buff, "osiva"))
        return hwnd;
    }
  }
  return NULL;    
}


  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::do_menu (void) {
  HWND hw = iconbar.get_hwnd();
  RECT r;
  GetWindowRect (hw, &r);
  int x = r.left + menu_icon_left;
  in_menu = 1;
  TrackPopupMenuEx (hmenu, 0, x, r.bottom, hw, 0);
  in_menu = 0;
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::handle_command (WPARAM wparam, LPARAM lparam) {
  int wID = LOWORD (wparam);
  RECT rect;
  HWND hwnd;

  switch (wID) {
  case ID_RIGHTCLICKCLOSES:
    if (ooptions->right_click_closes) {
      ooptions->right_click_closes = 0;
      CheckMenuItem (hmenu, ID_RIGHTCLICKCLOSES, MF_UNCHECKED);
    }
    else {
      ooptions->right_click_closes = 1;
      CheckMenuItem (hmenu, ID_RIGHTCLICKCLOSES, MF_CHECKED);
    } 
    break;
  case ID_TILE:
    hwnd = find_top_window();
    GetWindowRect (hwnd, &rect);
    tile_rect (&rect);
    break;
  case ID_HELPWIN:
    ShowWindow(hw_help, SW_SHOW);
    BringWindowToTop (hw_help);
    break;
  case ID_FLIP:
    set_flip ();
    break;
  case ID_FLIPTIME:
    ShowWindow (flip_dialog->hwnd_main, SW_SHOW);
    break;
  case ID_TRANSPARENCY:
    ShowWindow (trans_dialog->hwnd_main, SW_SHOW);
    break;
  case ID_STAYOPEN:
    if (ooptions->stayup) {
      ooptions->stayup = 0;
      CheckMenuItem (hmenu, ID_STAYOPEN, MF_UNCHECKED);
      iconbar.reset_bar_color ();
      iconbar.enable_tips ();
    }
    else {
      ooptions->stayup = 1;
      CheckMenuItem (hmenu, ID_STAYOPEN, MF_CHECKED);
      iconbar.set_bar_color (140, 122, 70);
      iconbar.disable_tips ();
    } 
    break;
  case ID_SAVELAYOUT:
    save_layout ();
    break;
  case ID_BATCH_CONSOLIDATE:
    save_layout (1);
    break;
  case ID_ROTATE_CCW:
    all_rotate (NULL, 0);
    break;
  case ID_ROTATE_CW:
    all_rotate (NULL, 1);
    break;
  case ID_FIT_INSIDE:
    fit_top_window (FIT_INSIDE);
    break;
  case ID_FIT_EXACTLY:
    fit_top_window (FIT_EXACTLY);
    break;
  case ID_FIT_MATCHAREA:
    fit_top_window (MATCH_AREA);
    break;
  case ID_SUPPRESSZOOM:
    if (ooptions->suppress_zoom) {
      ooptions->suppress_zoom = 0;
      CheckMenuItem (hmenu, ID_SUPPRESSZOOM, MF_UNCHECKED);
    }
    else {
      ooptions->suppress_zoom = 1;
      CheckMenuItem (hmenu, ID_SUPPRESSZOOM, MF_CHECKED);
    } 
    break;
  case ID_STAYONTOP:
    if (ooptions->ontop) {
      SetWindowPos (iconbar.get_hwnd(), HWND_NOTOPMOST,0,0,0,0,
        SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
      ooptions->ontop = 0;
      CheckMenuItem (hmenu, ID_STAYONTOP, MF_UNCHECKED);
    }
    else {
      ooptions->ontop = 1;
      SetWindowPos (iconbar.get_hwnd(), HWND_TOPMOST,0,0,0,0,
        SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
      CheckMenuItem (hmenu, ID_STAYONTOP, MF_CHECKED);
    } 
    break;

  } // switch on wID

}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::handle_timer (WPARAM wparam, LPARAM lparam) {

  int bd = GetKeyState(VK_LBUTTON) < 0 || GetKeyState(VK_RBUTTON) < 0;
  
  switch (wparam) {
  case IconBar::CLIENT_TIMER:
    {
      KillTimer (iconbar.get_hwnd(), IconBar::CLIENT_TIMER);
      if (ooptions->dissolve) {
        dissolve_init ();
      }
      else {
        tab (0, 0); // do not activate the window when flipping
        if (ooptions->flip)
          SetTimer (iconbar.get_hwnd(), IconBar::CLIENT_TIMER,
          ooptions->flip_sec * 1000 + ooptions->flip_ms, NULL);
      }
    }
    break;
  case DISSOLVE_TIMER:
    {
      KillTimer (iconbar.get_hwnd(), DISSOLVE_TIMER);
      if (frame_num < frames) {
        dissolve_frame ();
      }
      else {
        dissolve_clear ();
        SetTimer (iconbar.get_hwnd(), IconBar::CLIENT_TIMER,
          ooptions->flip_sec * 1000 + ooptions->flip_ms, NULL);
      }
    }
    break;
  } // switch on wparam, the TIMER ID
  
}

  /////////
// WndMgr //////////////////////////////////
/////////

static UINT CALLBACK
OFNHookProcNT (HWND h, UINT u, WPARAM w, LPARAM l){
  HWND dlg;
  RECT rdlg;
  RECT rscrn;
  int left, top;
  int width, height;
  switch (u) {
  case WM_INITDIALOG:
    dlg = GetParent (h);
    GetWindowRect (dlg, &rdlg);
    width = rdlg.right - rdlg.left;
    height = rdlg.bottom - rdlg.top;
    SystemParametersInfo (SPI_GETWORKAREA, 0, &rscrn, 0);
    left = (rscrn.right - width)/2;
    top = (rscrn.bottom - height)/2;
    MoveWindow (dlg, left, top, width, height, TRUE);
    break;
  }
  return 0;
}

// --- --- --- --- --- --- --- --- --- --- ---

static UINT CALLBACK
OFNHookProc9x (HWND h, UINT u, WPARAM w, LPARAM l){
  HWND dlg;
  RECT rdlg;
  RECT rscrn;
  int left, top;
  int width, height;
  switch (u) {
  case WM_INITDIALOG:
    dlg = h;
    GetWindowRect (dlg, &rdlg);
    width = rdlg.right - rdlg.left;
    height = rdlg.bottom - rdlg.top;
    SystemParametersInfo (SPI_GETWORKAREA, 0, &rscrn, 0);
    left = (rscrn.right - width)/2;
    top = (rscrn.bottom - height)/2;
    MoveWindow (dlg, left, top, width, height, TRUE);
    break;
  }
  return 0;
}

// --- --- --- --- --- --- --- --- --- --- ---

// Windows 2000 expects a longer structure:
/***
  struct OPENFILENAMEEX : public OPENFILENAME {
    void * pvReserved;
    DWORD dwReserved;
    DWORD FlagsEx;
  };
***/



/***

FlagsEx

Windows 2000/XP: A set of bit flags you can use to initialize
the dialog box. This member can be a combination of the following
flags. Flag Meaning OFN_EX_NOPLACESBAR If this flag is set, the
places bar is not displayed. If this flag is not set, Explorer-style
dialog boxes include a places bar containing icons for commonly-used
folders, such as Favorites and Desktop.


lStructSize
Specifies the length, in bytes, of the structure.
Windows NT 4.0: In an application that is compiled with WINVER
and _WIN32_WINNT >= 0x0500, use OPENFILENAME_SIZE_VERSION_400
for this member.

Windows 2000/XP: Use sizeof (OPENFILENAME) for this parameter.

***/

// --- --- --- --- --- --- --- --- --- --- ---
 
void WndMgr::save_layout (int consolidate) {
  
  OPENFILENAME ofn;       // common dialog box structure
  char szFile[260];       // buffer for file name
  char szPath[260];
  *szFile = 0;

  // Initialize OPENFILENAME
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  if (consolidate)
    ofn.lpstrTitle = "Consolidate -- copy images to new location with layout";
  else
    ofn.lpstrTitle = "Save layout";
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hInstance = _hInstance;
  ofn.hwndOwner = iconbar.get_hwnd();
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Osiva Layout\0*.osiva\0All\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_ENABLEHOOK|OFN_HIDEREADONLY|OFN_NONETWORKBUTTON|
    OFN_OVERWRITEPROMPT;
  ofn.lpfnHook = OFNHookProc9x;

  int version_nt = VER_PLATFORM_WIN32_NT;
  if (ooptions->osvi.dwPlatformId == version_nt) {
    // Use the "modern" dialog
    ofn.Flags |= OFN_EXPLORER;
    ofn.lpfnHook = OFNHookProcNT;
    if (ooptions->osvi.dwMajorVersion > 4) {
      // KLUDGE:
      // use the windows 2000 version with LHS gizmos
      ofn.lStructSize += 12;
    }
  }


  int ret = GetSaveFileName (&ofn);
  int err = 0;
  if (!ret) {
    err = CommDlgExtendedError();
    return;
  }

  // Construct the destination path

  strcpy (szPath, szFile);
  char *cp = strrchr (szPath, '\\');
  if (cp) *cp = 0;

  // Construct a good file name, making
  // assumptions about the extension

  cp = strrchr (szFile, '.');
  int extlen = 0;
  if (cp)
    extlen = strlen (cp);
  if (extlen == 0 || extlen > 6) {
    if (260 - strlen(szFile) > 7) {
      strcat (szFile, ".osiva");
    }
  }

  // ...
  
  FILE *fp = fopen (szFile, "w");
  if (!fp) {
    MessageBox (iconbar.get_hwnd(), szFile,
      "Osiva cannot write the file", MB_OK);
    return;
  }

  // Print the header line

  fprintf (fp, "%s", layout_header);
  if (consolidate)
    fprintf (fp, " 2R\n");
  else
    fprintf (fp, " 2\n");

  // Get an index to the windows in Z order;

  unsigned long PID = GetCurrentProcessId ();
  unsigned long wPID, wTID;
  char class_name [80];
  vector <HWND> zorder;
  HWND hwz = GetTopWindow (NULL);
  for ( ; hwz; hwz = GetNextWindow (hwz, GW_HWNDNEXT)) {
    wTID = GetWindowThreadProcessId (hwz, &wPID);
    if (wPID == PID ) {
      GetClassName (hwz, class_name, 80);
      if (!strcmp (class_name, "osiva")) {
        zorder.push_back (hwz);
      }
    }    
  }

  int zorders = zorder.size();
  int snapwins = snapwin.size();

  SnapShotW *ssw;

  // A filename cannot contain any of the following characters
  // \ / : * ? " < > | and [TAB]

  // The fields are:
  // Path, left, top, width, trans, bg_tol, erosion, mask_depth, rotation

  const char *src, *dst;
  vector<HWND>::reverse_iterator p;
  for (p = zorder.rbegin(); p != zorder.rend(); p++) {
    ssw = (SnapShotW *) GetWindowLong (*p, GWL_USERDATA);
    src = ssw->get_file_path();
    dst = src;
    // Use a relative path for consolidation
    // Also copy the file to the consolidation path destination
    if (consolidate) {
      cp = strrchr (dst, '\\');
      if (cp) dst = cp + 1;
      char *buff = new char [sizeof(szPath) + sizeof(dst) + 10];
      sprintf (buff, "%s\\%s", szPath, dst);
      CopyFile (src, dst, TRUE);
      delete [] buff;
    }
    if (dst == NULL)
      continue;
    if (strlen (dst) < 3)
      continue;
    fprintf (fp, "%s", dst);
    RECT rect;
    GetWindowRect (*p, &rect);
    fprintf (fp, "|%d", rect.left);
    fprintf (fp, "|%d", rect.top);
    fprintf (fp, "|%d", ssw->get_width());
    fprintf (fp, "|%d", ssw->get_height());
    fprintf (fp, "|%d", ssw->get_transparent());
    fprintf (fp, "|%d", ssw->get_tolerance());
    fprintf (fp, "|%d", ssw->get_erosions());
    fprintf (fp, "|%d", ssw->get_mask_depth());
    fprintf (fp, "|%d", ssw->get_rotation());
    fprintf (fp, "\n");
  }
  fclose (fp);
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::set_flip (int type) {

   // if (!ddproxy) {
   //   ddproxy = new DDProxy ;
   //   ddproxy->init();
   // }
  
  switch (type) {
  case FLIP_TOGGLE:
    if (ooptions->flip) {
      ooptions->flip = 0;
      CheckMenuItem (hmenu, ID_FLIP, MF_UNCHECKED);
      KillTimer (iconbar.get_hwnd(), IconBar::CLIENT_TIMER);
      dissolve_clear ();
    }
    else {
      ooptions->flip = 1;
      CheckMenuItem (hmenu, ID_FLIP, MF_CHECKED);
      SetTimer (iconbar.get_hwnd(), IconBar::CLIENT_TIMER,
        ooptions->flip_sec * 1000 + ooptions->flip_ms, NULL);
    }
    break;
  case  FLIP_ON:
    ooptions->flip = 1;
    CheckMenuItem (hmenu, ID_FLIP, MF_CHECKED);
    SetTimer (iconbar.get_hwnd(), IconBar::CLIENT_TIMER,
      ooptions->flip_sec * 1000 + ooptions->flip_ms, NULL);
    break;
  case FLIP_OFF:
    ooptions->flip = 0;
    CheckMenuItem (hmenu, ID_FLIP, MF_UNCHECKED);
    KillTimer (iconbar.get_hwnd(), IconBar::CLIENT_TIMER); 
    dissolve_clear ();
    break;
  } // switch on type
  
  ooptions->broadcast (OOptions::FLIP, iconbar.get_hwnd());
  
}



  /////////
// WndMgr //////////////////////////////////
/////////

// Returns version -- negative for relative paths

int WndMgr::is_layout_file (char *path) {

  FILE *fp = fopen (path, "r");
  if (!fp) return 0;

  char *buff = new char [80];
  fgets (buff, 80, fp);
  fclose (fp);
  if (buff[strlen(buff)-1] == '\n')
    buff[strlen(buff)-1] = 0;

  int cmplen = strlen (layout_header);
  int res = strncmp (buff, layout_header, cmplen); 
  if (res) {
    delete [] buff;
    return 0;
  }

  int bufflen = strlen (buff);
  int version = 1;
  if (bufflen > cmplen) {
    version = atoi (buff+cmplen);
    if (buff[bufflen - 1] == 'R')
      version = -version;
  }
  
  delete [] buff;
  return version;
}

  /////////
// WndMgr //////////////////////////////////
/////////

// Destructively parse a line

static int parseN (char *line, int N, char *toks[]) {
  char *seps = "|\n";
  // Original layout files had comma for the delimeter
  if (N == 9)
    seps = ",|\n";
  char *token = strtok( line, seps );
  int n = 0;
  if (token)
    toks[n++] = token;
  while( token != NULL )
  {
    token = strtok( NULL, seps );
    if (token)
      toks[n++] = token;
    if (n == N)
      break;
  }
  return n;
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::load_layout_file (char *path) {

  int version = is_layout_file (path);
  if (!version)
    return;
  int relative = 0;
  if (version < 0) {
    relative = 1;
    version = -version;
  }
  char *rpath = NULL;
  char *rbuff = NULL;
  int rlen = 0;
  int max_path = _MAX_PATH;
  if (relative) {
    rpath = strdup (path);
    char *cp = strrchr (rpath, '\\');
    if (cp) *cp = 0;
    rlen = strlen (rpath);
    rbuff = (char *) malloc (max_path + 1);
  }

  char buff[256];
  FILE *fp = fopen (path, "r");
  fgets (buff, 256, fp); // First line has the file type
  int n;
  HWND hwnd;
  SnapShotW *ssw;
  RECT rect;
  char *imgpath = 0;

  // Path, left, top, width, height, trans, bg_tol, erosion, mask_depth,
  // rotation
  
  char *toks[14]; // WATCH OUT <----
  int ntoks = 9;
  if (version > 1)
    ntoks = 10;

  char* fline = fgets (buff, 256, fp);
  while (fline) {
    n = parseN (buff, ntoks, toks);
    if (n < 9)
      continue;
    if (relative) {
      if (rlen + strlen(toks[0]) > max_path)
        continue;
      sprintf (rbuff, "%s\\%s", rpath, toks[0]);
      imgpath = rbuff;
    }
    else {
      imgpath = toks[0];
    }
    hwnd = new_window (_hInstance, imgpath, SW_HIDE);
    int left = atoi (toks[1]);
    int top = atoi (toks[2]);
    int width = atoi (toks[3]);
    int height = atoi (toks[4]);
    int trans = atoi (toks[5]);
    int bg_tol = atoi (toks[6]);
    int erosion = atoi (toks[7]);
    int mask_depth = atoi (toks[8]);
    int rotation = 0;
    if (n > 9)
      rotation = atoi (toks[9]);

    ssw = (SnapShotW *) GetWindowLong (hwnd, GWL_USERDATA);
    switch (rotation) {
    case 2:
      ssw->rotate ();
    case 1:
      ssw->rotate ();
      break;
    case 3:
      ssw->rotate (0);
      break;
    }   
    rect.left = left;
    rect.top = top;
    rect.right = left + width - 1;
    rect.bottom = rect.top + height - 1;
    ssw->move_img (&rect, 0);
    if (trans) {
      ooptions->bg_diff = bg_tol;
      ooptions->erosions = erosion;
      ooptions->depth = mask_depth;
      ssw->apply_trans ();
    }
    ShowWindow (hwnd, SW_SHOW);
    UpdateWindow (hwnd);
    fline = fgets (buff, 256, fp);
  }

  fclose (fp);
  free (rpath);
  free (rbuff);
}

  /////////
// WndMgr //////////////////////////////////
/////////

// Write current status information into the help dialog
// This is for debug and test

void WndMgr::dump_info () {

  HWND hw_edit = GetDlgItem(hw_help, IDC_HELPEDIT);
  SendMessage (hw_edit, WM_SETTEXT, 0, (LPARAM) "");
  SendMessage (hw_edit, EM_REPLACESEL, 0, 
    (LPARAM) "Executable path:\r\n");
  SendMessage (hw_edit, EM_REPLACESEL, 0,
    (LPARAM) exe_path);
  SendMessage (hw_edit, EM_REPLACESEL, 0, (LPARAM) "\r\n");
  ShowWindow (hw_help, SW_SHOW);

  SendMessage (hw_edit, EM_REPLACESEL, 0, (LPARAM) "\r\n");
  SendMessage (hw_edit, EM_REPLACESEL, 0, (LPARAM) "Image Paths:\r\n");
  SnapShotW *ssw;
  int sss = snapwin.size();
  for (int i = 0; i < sss; i++) {
    ssw = snapwin[i];
    SendMessage (hw_edit, EM_REPLACESEL, 0,
      (LPARAM) ssw->get_file_path());
    SendMessage (hw_edit, EM_REPLACESEL, 0, (LPARAM) "\r\n");
  }
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::show (int flag, HWND curr_hwnd) {
  
  HWND  hwnd = iconbar.get_hwnd();
  WINDOWPLACEMENT wndpl;
  GetWindowPlacement (hwnd, &wndpl);
    
  if (flag) {
    //if (wndpl.showCmd == SW_SHOWNORMAL || wndpl.showCmd == SW_SHOW) return;
    ShowWindow (hwnd, SW_SHOWNA);
    return;
  }
  
  // This is an attempt to emulate MS Word hiding the
  // floating toolbars when it loses focus
  
  //if (curr_hwnd == NULL ) return;
  if (curr_hwnd == iconbar.get_hwnd()) 
    return;
  
  // Don't hide a minimized iconbar  
  if (wndpl.showCmd == SW_SHOWMINIMIZED) {
    return;
  }
  // Don't hide when we are closing an image
  if (in_close) {
    return;
  }
  // Don't hide if the new focus is still ours
  for (int i = 0; i < snapwin.size(); i++) {
    if (snapwin[i]->get_hwnd() == curr_hwnd) {
      return;
    }
  }
  
  /***** Bad Idea
  ShowWindow (hwnd, SW_HIDE);
  *****/

}

  /////////
// WndMgr //////////////////////////////////
/////////


void WndMgr::dissolve_init () {
  
  if (snapwin.size() < 2) {
    set_flip (FLIP_OFF);
    return;
  }
    
  HWND hwnd = find_top_window ();
  prev_ssw = (SnapShotW *) GetWindowLong (hwnd, GWL_USERDATA);
  
  // Find the next window in tab order 
  SHORT state = GetKeyState (VK_SHIFT);
  int shifted = state & 0x8000?1:0;
  int i;
  for (i = 0; i < snapwin.size(); i++ )
    if (snapwin[i] == prev_ssw)
      break;
  if (i == snapwin.size())
    return;
  if (!shifted) {
    i++;
    if (i == snapwin.size())
      i = 0;
  }
  else {
    i--;
    if (i < 0)
      i = snapwin.size() - 1;
  }
    
  next_ssw = snapwin[i];

  // Grab the image from the next window -- the future frame
  // Grab the screen from above the next window -- the current frame
  // Then TAB to the next window (with the screen placed in it)

  llimg_release_llimg (future);
  future = next_ssw->dub_image ();
  llimg_release_llimg (frame);
  frame = next_ssw->dub_image ();  // A lazy way to create the buffer
  next_ssw->show_screen ();        // Copies the screen into the window
  llimg_release_llimg (screen);
  screen = next_ssw->dub_image (); // Dissolve is from "screen" to "future"
  hwnd = next_ssw->get_hwnd();
  SetWindowPos (hwnd, HWND_TOP, 0, 0, 0, 0,
     SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOREDRAW);
  InvalidateRect (hwnd, NULL, FALSE);
  UpdateWindow (hwnd);
  next_ssw->show_saved ();
    
  // ...
 
  frame_num = 0;
  frames = 1;
  for (int n = 0; n < ooptions->dissolve_bits; n++)
    frames *= 2;


  SetTimer (iconbar.get_hwnd(), DISSOLVE_TIMER, 50, NULL);


}

  /////////
// WndMgr //////////////////////////////////
/////////

// dissolve_frame() is entered on a WM_TIMER 
// This can occur in the event queue at any point. For
// instance, it can occur between resizing a window and
// updating the window with the new image that fits it.
// You cannot assume that the window rectangle matches
// the image. You cannot assume that either match the
// cached  frames. 

void WndMgr::dissolve_frame () {
  
  frame_num++;
  
  // Dissolve Equation: x = p + (n-p)t
  
  int B = ooptions->dissolve_bits;
  
  unsigned char *fp, *np, *pp;
  int x, y;
  bgr_color nbgr, pbgr;
  LLIMG *pimg = screen;
  LLIMG *nimg = future;
  int yy = frame->height < screen->height ?
    frame->height : screen->height;
  int xx = frame->width < screen->width ?
    frame->width : screen->width;
  
  if (pimg->bits_per_pixel == 24 && nimg->bits_per_pixel == 24 ) {
    for (y = 0; y < yy; y++) {
      fp = frame->line[y];
      pp = pimg->line[y];
      np = nimg->line[y];
      for (x = 0; x < xx; x++) {
        *fp++ = *pp++ + ((frame_num * (*np++ - *pp)) >> B);
        *fp++ = *pp++ + ((frame_num * (*np++ - *pp)) >> B);
        *fp++ = *pp++ + ((frame_num * (*np++ - *pp)) >> B);
      } // For each pixel x
    } // For each line y
  } // Both images are 24 bbp
  
  else if (pimg->bits_per_pixel == 8 && nimg->bits_per_pixel == 8 ) {
    for (y = 0; y < yy; y++) {
      fp = frame->line[y];
      pp = pimg->line[y];
      np = nimg->line[y];
      for (x = 0; x < xx; x++) {
        pbgr = pimg->color[*pp++];
        nbgr = nimg->color[*np++];
        *fp++ = pbgr.blue + ((frame_num * (nbgr.blue - pbgr.blue)) >> B);
        *fp++ = pbgr.green + ((frame_num * (nbgr.green - pbgr.green)) >> B);
        *fp++ = pbgr.red + ((frame_num * (nbgr.red - pbgr.red) >> B));
      } // For each pixel x
    } // For each line y
  } // Both images are 8 bbp
  
  else if (pimg->bits_per_pixel == 24 && nimg->bits_per_pixel == 8 ) {
    for (y = 0; y < yy; y++) {
      fp = frame->line[y];
      pp = pimg->line[y];
      np = nimg->line[y];
      for (x = 0; x < xx; x++) {
        nbgr = nimg->color[*np++];
        *fp++ = *pp++ + ((frame_num * (nbgr.blue - *pp)) >> B);
        *fp++ = *pp++ + ((frame_num * (nbgr.green - *pp)) >> B);
        *fp++ = *pp++ + ((frame_num * (nbgr.red - *pp) >> B));
      } // For each pixel x
    } // For each line y
  } // Next is color tabled
  
  else if (pimg->bits_per_pixel == 8 && nimg->bits_per_pixel == 24 ) {
    for (y = 0; y < yy; y++) {
      fp = frame->line[y];
      pp = pimg->line[y];
      np = nimg->line[y];
      for (x = 0; x < xx; x++) {
        pbgr = pimg->color[*pp++];
        *fp++ = pbgr.blue + ((frame_num * (*np++ - pbgr.blue)) >> B);
        *fp++ = pbgr.green + ((frame_num * (*np++ - pbgr.green)) >> B);
        *fp++ = pbgr.red + ((frame_num * (*np++ - pbgr.red) >> B));
      } // For each pixel x
    } // For each line y
  } // Previous is color tabled
  
  next_ssw->paint_temp_image (frame);
  
  SetTimer (iconbar.get_hwnd(), DISSOLVE_TIMER,
    ooptions->dissolve_sec * 1000 + ooptions->dissolve_ms, NULL);
  
}

  /////////
// WndMgr //////////////////////////////////
/////////

void WndMgr::dissolve_clear () {

  KillTimer (iconbar.get_hwnd(), DISSOLVE_TIMER); 

}

  /////////
// WndMgr //////////////////////////////////
/////////


  /////////
// WndMgr //////////////////////////////////
/////////


  /////////
// WndMgr //////////////////////////////////
/////////

