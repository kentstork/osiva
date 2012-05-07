/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * hotspot.cpp is part of Osiva.
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


///////////////////////////////////////////////////////////////////////////////
//
// File: hotspot.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "hotspot.h"


#define TIMER1 1

///////////////////////////////////////////////////////////////////////////////
//
// 

HotSpot::HotSpot () {
  mArmed = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// 

HotSpot::~HotSpot () {


}

///////////////////////////////////////////////////////////////////////////////
//
// 

void HotSpot::
init (HWND      parentHwnd, RECT &enclosure, int wm_message ) {
    
  mParentHwnd = parentHwnd ;
  mMessageID = wm_message ;
  
  
  HINSTANCE hinst = 
    (HINSTANCE) GetWindowLong( mParentHwnd, GWL_HINSTANCE ) ;
  
  
  WNDCLASSEX wcl;
  memset (&wcl, 0, sizeof(WNDCLASSEX));
  wcl.cbSize            = sizeof(WNDCLASSEX);
  wcl.hInstance         = hinst;
  wcl.lpszClassName     = "OvaHotSpot";
  wcl.lpfnWndProc       = WindowProcProxy;
  wcl.hCursor           = LoadCursor(hinst, IDC_CROSS);
  wcl.hbrBackground     = (HBRUSH) GetStockObject(BLACK_BRUSH); 
  if(!RegisterClassEx(&wcl)) {
  }
    
  mHwnd = CreateWindow(
    /* class name  */  "OvaHotSpot",
    /* title       */  "KeyPad",
    /* style flags */  WS_CHILD,
    /* x start     */  enclosure.left,
    /* y start     */  enclosure.top,
    /* width       */  enclosure.right - enclosure.left + 1,
    /* height      */  enclosure.bottom - enclosure.top + 1,
    /* parent hwnd */  mParentHwnd,
    /* new menu    */  NULL,
    /* prog inst   */  hinst,
    /* termination */  NULL
    );
    
  SetWindowLong( mHwnd, GWL_USERDATA, (LONG) this ) ;
  
  ShowWindow( mHwnd, SW_RESTORE );
  UpdateWindow( mHwnd );
  
  mArmed = 1;

  return ;
}

///////////////////////////////////////////////////////////////////////////////
//
// 

void HotSpot::paint () {
  HDC hdc = GetDC (mHwnd);
  RECT rect;
  GetClientRect (mHwnd, &rect);
  FillRect(hdc, &rect, (HBRUSH) (COLOR_WINDOW+1));
  ReleaseDC (mHwnd, hdc);
}

///////////////////////////////////////////////////////////////////////////////
//
// 

void HotSpot::unpaint () {
  HDC hdc = GetDC (mHwnd);
  RECT rect;
  GetClientRect (mHwnd, &rect);
  FillRect(hdc, &rect, (HBRUSH) (COLOR_BTNTEXT+1));
  ReleaseDC (mHwnd, hdc);
}

///////////////////////////////////////////////////////////////////////////////
//
// 


LRESULT CALLBACK HotSpot::
WindowProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  
  switch (message)
  {
  case WM_SETCURSOR:
    if (mArmed) {
      paint();
      mArmed = 0;
      SetTimer (mHwnd, TIMER1, 100, NULL);
      SendMessage (mParentHwnd, mMessageID, 0, 0);
    }
    break;

  case WM_TIMER:
    unpaint ();
    break;
    
  default:
    break;
  }
  
  return DefWindowProc (hwnd, message, wParam, lParam);
  
}
