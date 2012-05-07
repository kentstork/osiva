/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * tooltip.cpp is part of Osiva.
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


//////////////////////////////////////////////////////////////////////////////
//
// File: tooltip.cpp
//

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "tooltip.h"

//////////////////////////////////////////////////////////////////////////////
//

ToolTip::
ToolTip()
{ 
  note = 0 ;
  hThisWnd = 0 ;
  hfont = 0 ;
  _enable = 1;
}

//////////////////////////////////////////////////////////////////////////////
//
  
ToolTip::  
~ToolTip() 
{ 
  delete[] note ;
  HINSTANCE inst = (HINSTANCE) GetWindowLong (hParentWnd, GWL_HINSTANCE);
  DestroyWindow( hThisWnd ) ;
  UnregisterClass( "POPWIN", inst ) ;
  if (hfont) DeleteObject( (HGDIOBJ) hfont ) ;
}

//////////////////////////////////////////////////////////////////////////////
//

void ToolTip::
init (HWND hParentWin, int font_height, const char *face_name)
{
  hParentWnd = hParentWin ;
  
  hInstance =
    (HINSTANCE) GetWindowLong (hParentWin, GWL_HINSTANCE);
  
  WNDCLASSEX wc;
  wc.cbSize = sizeof (WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC) TTProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  wc.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor (NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = "TTWIN";
  RegisterClassEx (&wc);
  
  hThisWnd = CreateWindow ("TTWIN", "", WS_POPUP, 0, 0,
    100, 33, hParentWnd, NULL, hInstance, NULL);
  
  SetWindowLong (hThisWnd, GWL_USERDATA, (LONG) this);
  // ToolTip * pn = (ToolTip *) GetWindowLong( hwnd, GWL_USERDATA ) ;
  
  LOGFONT logfont ;
  memset(&logfont, 0, sizeof(LOGFONT) ) ;
  logfont.lfHeight = font_height ;
  sprintf (logfont.lfFaceName, face_name);
  hfont = CreateFontIndirect( &logfont ) ;
}

//////////////////////////////////////////////////////////////////////////////
//

void ToolTip::
set (const char * noteText)
{
  if ( !noteText )
    return;
  GetCursorPos (&hitPoint);
  if ( note ) delete [] note ;
  note = new char [ strlen (noteText) + 1 ] ;
  strcpy( note, noteText ) ;
  
  RECT desk ;
  HFONT holdfont ;

  // Move the window to pointer and to fit text
  GetClientRect( GetDesktopWindow(), &desk ) ;
  HDC hdc = GetDC(hThisWnd) ;
  holdfont = (HFONT) SelectObject(hdc, hfont) ;
  SIZE sz ;
  GetTextExtentPoint32(hdc, note, strlen(note), &sz ) ;
  SelectObject(hdc, holdfont);
  ReleaseDC( hThisWnd, hdc ) ;
  int x = hitPoint.x + 8 ;
  int y = hitPoint.y + 16 ;
  int w = sz.cx + 22 ;
  int h = sz.cy + 5 ;
  int clipped = (x + w) - desk.right ;
  if ( clipped > 0 )
    x -= clipped ;
  if ( y + h > desk.bottom )
    y = hitPoint.y - 16 - h ;
  InvalidateRect (hThisWnd, NULL, TRUE);
  MoveWindow( hThisWnd, x, y, w, h, TRUE ) ;

}

//////////////////////////////////////////////////////////////////////////////
//

void ToolTip::
hide ()
{
  ShowWindow (hThisWnd, SW_HIDE);  
}

//////////////////////////////////////////////////////////////////////////////
//

void ToolTip::
show ()
{
  if (_enable)
    ShowWindow (hThisWnd, SW_SHOWNA);  
}

//////////////////////////////////////////////////////////////////////////////
//

LRESULT CALLBACK ToolTip::
TTProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hdc;
  RECT rt;
  char *txt ;
  HFONT holdfont ;
  
  ToolTip * pn = (ToolTip *) GetWindowLong( hWnd, GWL_USERDATA ) ;
  
  switch (message)
  {
  case WM_PAINT:
    hdc = BeginPaint (hWnd, &ps);
    holdfont = (HFONT) SelectObject(hdc, pn->hfont);
    GetClientRect (hWnd, &rt);
    FrameRect (hdc, &rt, (HBRUSH) GetStockObject (BLACK_BRUSH));
    txt = pn->note ;
    SetBkMode (hdc, TRANSPARENT);
    DrawText (hdc, txt, strlen (txt), &rt,
      DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(hdc, holdfont);
    EndPaint (hWnd, &ps);
    break;
  default:
    return DefWindowProc (hWnd, message, wParam, lParam);
  }
  return 0;
}

