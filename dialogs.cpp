/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * dialog.cpp is part of Osiva.
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
// dialog.cpp
//
///////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>          // For floor()
#include <windows.h>

#include <vector>
using namespace std;
#include "ooptions.h"

#include "resource.h"

#include "hotspot.h"
#include "dialogs.h"

#define WM_FLIPPER (WM_USER+5)
#define WM_FLIPPER_R (WM_USER+6)

///////////////////////////////////////////////////////////////////////////
// Global Scope

extern OOptions *ooptions;

#define OPTIONS_CHANGED 1

///////////////////////////////////////////////////////////////////////////

static void screen_to_window (HWND window, RECT &screen){
  POINT tl;
  tl.x = screen.left;
  tl.y = screen.top;
  POINT br;
  br.x = screen.right;
  br.y = screen.bottom;
  ScreenToClient (window, &tl);
  ScreenToClient (window, &br);
  screen.left = tl.x;
  screen.top = tl.y;
  screen.right = br.x;
  screen.bottom = br.y;
}

///////////////////////////////////////////////////////////////////////////
// FlipDialog
///////////////////////////////////////////////////////////////////////////

void FlipDialog::init (HINSTANCE hInstance) {

  hwnd_main = CreateDialog (hInstance, (LPCTSTR) IDD_FLIP,
                            NULL, DialogProcProxy);
  SetWindowLong (hwnd_main, GWL_USERDATA, (long) this);
  eb_seconds = GetDlgItem (hwnd_main, IDC_EDIT_SEC);
  eb_seconds_dislv = GetDlgItem (hwnd_main, IDC_EDIT_DISLV);
  cb_flip = GetDlgItem (hwnd_main, IDC_FLIP);
  cb_dissolve = GetDlgItem (hwnd_main, IDC_DISSOLVE);
  cm_dissolve_bits = GetDlgItem (hwnd_main, IDC_DISSOLVE_BITS);
  bt_hotspot1 = GetDlgItem (hwnd_main, IDC_HOTSPOT1);
  bt_hotspot2 = GetDlgItem (hwnd_main, IDC_HOTSPOT2);
  bt_apply = GetDlgItem (hwnd_main, IDC_B_APPLY);

  RECT r_scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &r_scrn, 0);
  RECT rect;
  GetWindowRect (hwnd_main, &rect);
  int dx = rect.right - rect.left;
  int dy = rect.bottom - rect.top;
  int x = (r_scrn.right - dx)/2;
  int y = (r_scrn.bottom - dy)/2;
  y = max (0, y);

  SetWindowPos (hwnd_main, HWND_TOPMOST, x, y, dx, dy,
    SWP_NOACTIVATE);

  RECT rHotSpot1;
  GetWindowRect (bt_hotspot1, &rHotSpot1);
  screen_to_window (hwnd_main, rHotSpot1);
  int ht = rHotSpot1.bottom - rHotSpot1.top;
  rHotSpot1.top += ht/3;
  rHotSpot1.bottom -= ht/3;
  hotSpot1.init (hwnd_main, rHotSpot1, WM_FLIPPER);

  RECT rHotSpot2;
  GetWindowRect (bt_hotspot2, &rHotSpot2);
  screen_to_window (hwnd_main, rHotSpot2);
  ht = rHotSpot2.bottom - rHotSpot2.top;
  rHotSpot2.top += ht/3;
  rHotSpot2.bottom -= ht/3;
  hotSpot2.init (hwnd_main, rHotSpot2, WM_FLIPPER_R);
  
  SendMessage (cm_dissolve_bits, CB_ADDSTRING, 0, (LPARAM) "8 frames");
  SendMessage (cm_dissolve_bits, CB_ADDSTRING, 0, (LPARAM) "16 frames");
  SendMessage (cm_dissolve_bits, CB_ADDSTRING, 0, (LPARAM) "32 frames");
  SendMessage (cm_dissolve_bits, CB_ADDSTRING, 0, (LPARAM) "64 frames");
  SendMessage (cm_dissolve_bits, CB_ADDSTRING, 0, (LPARAM) "128 frames");
  SendMessage (cm_dissolve_bits, CB_ADDSTRING, 0, (LPARAM) "256 frames");
  SendMessage (cm_dissolve_bits, CB_ADDSTRING, 0, (LPARAM) "512 frames");

  refresh ();

  ooptions->subscribe (hwnd_main, OPTIONS_CHANGED);

}

///////////////////////////////////////////////////////////////////////////

void FlipDialog::apply () {

  char buff[80];

  GetWindowText (eb_seconds, buff, 80);
  float f = atof (buff);
  float s = floor (f);
  float ms = (f - s) * 1000.0;
  if (s == 0.0 && ms < 10.0)
    ms = 10.0;
  ooptions->flip_sec = (int) s;
  ooptions->flip_ms = (int) ms;

  GetWindowText (eb_seconds_dislv, buff, 80);
  f = atof (buff);
  s = floor (f);
  ms = (f - s) * 1000.0;
  if (s == 0.0 && ms < 1.0)
    ms = 1;
  ooptions->dissolve_sec = (int) s;
  ooptions->dissolve_ms = (int) ms;

  int bstate = SendMessage (cb_flip, BM_GETCHECK, 0, 0);
  if (bstate == BST_CHECKED) {
    ooptions->flip = 1;
  }
  else { 
    ooptions->flip = 0;
  }

  bstate = SendMessage (cb_dissolve, BM_GETCHECK, 0, 0);
  if (bstate == BST_CHECKED) {
    ooptions->dissolve = 1;
  }
  else {
    ooptions->dissolve = 0;
  }

  int cm_item = SendMessage (cm_dissolve_bits, CB_GETCURSEL, 0, 0);
  ooptions->dissolve_bits = cm_item + 3; // 8 is the first item

  // Broadcast will cause a refresh
  ooptions->broadcast (OOptions::FLIP);
}

///////////////////////////////////////////////////////////////////////////

void FlipDialog::refresh () {

  float f = (float) ooptions->flip_ms / 1000.0;
  f += ooptions->flip_sec;
  char buff[80];
  sprintf (buff, "%0.3f s.", f);
  SetWindowText (eb_seconds, buff);

  f = (float) ooptions->dissolve_ms / 1000.0;
  f += ooptions->dissolve_sec;
  sprintf (buff, "%0.3f s.", f);
  SetWindowText (eb_seconds_dislv, buff);

  WPARAM bstate = ooptions->flip? BST_CHECKED: BST_UNCHECKED;
  SendMessage (cb_flip, BM_SETCHECK, bstate, 0);
  bstate = ooptions->dissolve? BST_CHECKED: BST_UNCHECKED;
  SendMessage (cb_dissolve, BM_SETCHECK, bstate, 0);
  SendMessage (cm_dissolve_bits, CB_SETCURSEL,
    ooptions->dissolve_bits - 3, 0); // The list starts at 8 = 3 bits

  EnableWindow (bt_apply, FALSE);

}

///////////////////////////////////////////////////////////////////////////


BOOL CALLBACK FlipDialog::
DialogProc ( HWND hwnd, UINT m, WPARAM w, LPARAM l)
{

  switch (m) {

  // TODO: Find out why this recieves WM_USER on activation
  case WM_USER:
    return 1;

  case WM_FLIPPER:
    ooptions->broadcast (OOptions::FLIP_NOW, hwnd_main);
    return 1;

  case WM_FLIPPER_R:
    ooptions->broadcast (OOptions::FLIP_NOW_R, hwnd_main);
    return 1;

  case WM_MOUSEMOVE:
    if (!hotSpot1.armed())
      hotSpot1.arm();
    if (!hotSpot2.armed())
      hotSpot2.arm();
    break;

  case WM_INITDIALOG:
    // This will never be seen using this architecture
    return 1;
    
  case WM_APP:
    // WM_APP is generated by OOptions::broadcast()
    if (w == OPTIONS_CHANGED && l == OOptions::FLIP) 
      refresh ();
    return 1;
    
  case WM_COMMAND:
    {
      int bstate;
      switch (LOWORD (w)) {
      case IDOK:
        apply();
        ShowWindow (hwnd, SW_HIDE);
        return 1;
      case IDC_B_APPLY:
        apply();
        return 1;
      case IDC_DISSOLVE:
        bstate = SendMessage (cb_dissolve, BM_GETCHECK, 0, 0);
        // if (bstate == BST_CHECKED) 
        //   SendMessage (cb_flip, BM_SETCHECK, BST_CHECKED, 0);
        apply();
        return 1;
      case IDC_FLIP:
        bstate = SendMessage (cb_flip, BM_GETCHECK, 0, 0);
        // if (bstate == BST_UNCHECKED) 
        //   SendMessage (cb_dissolve, BM_SETCHECK, BST_UNCHECKED, 0);
        apply();
        return 1;
      case IDC_EDIT_SEC:
        if (HIWORD (w) == EN_CHANGE)
          EnableWindow (bt_apply, TRUE);
        break;
      case IDC_DISSOLVE_BITS:
        if (HIWORD (w) == CBN_SELCHANGE)
          EnableWindow (bt_apply, TRUE);
        break;
      case IDC_EDIT_DISLV:
        if (HIWORD (w) == EN_CHANGE)
          EnableWindow (bt_apply, TRUE);
        break;
      }
      break;
    }
    
  case WM_CLOSE:
    ShowWindow (hwnd, SW_HIDE);
    return 1;    
    
  } // Switch on the message
  
  return 0;
  
}
 
///////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////
// TransDialog
///////////////////////////////////////////////////////////////////////////

void TransDialog::init (HINSTANCE hInstance) {

  hwnd_main = CreateDialog (hInstance, (LPCTSTR) IDD_TRANSPARENCY,
                            NULL, DialogProcProxy);
  SetWindowLong (hwnd_main, GWL_USERDATA, (long) this);
  eb_bgdiff = GetDlgItem (hwnd_main, IDC_EDIT_BGDIFF);
  eb_depth = GetDlgItem (hwnd_main, IDC_EDIT_DEPTH);
  eb_erosions = GetDlgItem (hwnd_main, IDC_EDIT_EROSIONS);

  RECT r_scrn;
  SystemParametersInfo (SPI_GETWORKAREA, 0, &r_scrn, 0);
  RECT rect;
  GetWindowRect (hwnd_main, &rect);
  int dx = rect.right - rect.left;
  int dy = rect.bottom - rect.top;
  int x = (r_scrn.right - dx)/2;
  int y = (r_scrn.bottom - dy)/2;
  y = max (0, y);
  SetWindowPos (hwnd_main, HWND_TOPMOST, x, y, dx, dy,
    SWP_NOACTIVATE);
  
  char buff[80];
  sprintf (buff, "%d", ooptions->bg_diff);
  SetWindowText (eb_bgdiff, buff);
  sprintf (buff, "%d", ooptions->depth - 1);
  SetWindowText (eb_depth, buff);    
  sprintf (buff, "%d", ooptions->erosions);
  SetWindowText (eb_erosions, buff);    

}

///////////////////////////////////////////////////////////////////////////

void TransDialog::apply () {

  char buff[80];
  GetWindowText (eb_bgdiff, buff, 80);
  int bgdiff = atoi (buff);
  GetWindowText (eb_depth, buff, 80);
  int depth = atoi (buff) + 1;
  GetWindowText (eb_erosions, buff, 80);
  int erosions = atoi (buff);
  ooptions->bg_diff = bgdiff;
  ooptions->depth = depth;
  ooptions->erosions = erosions;
  ooptions->broadcast (OOptions::TRANSPARENCY);

}

///////////////////////////////////////////////////////////////////////////


BOOL CALLBACK TransDialog::
DialogProc ( HWND hwnd, UINT m, WPARAM w, LPARAM l)
{

  switch (m) {
  case WM_INITDIALOG:
    // This will never be seen using this architecture
    return 1;
    
  case WM_COMMAND:
    switch (LOWORD (w)) {
    case IDOK:
      apply();
      ShowWindow (hwnd, SW_HIDE);
      return 1;
    case IDCANCEL:
      // TODO: set dialog options back to those in ooptions
      ShowWindow (hwnd, SW_HIDE);
      return 1;
    case IDC_APPLY:
      apply();
      return 1;
    }
  }
  return 0;

}
 
///////////////////////////////////////////////////////////////////////////









