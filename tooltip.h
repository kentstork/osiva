/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * tooltip.h is part of Osiva.
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
// File: tooltip.h
//
// Synopsis:
//
//     #include <windows.h>
//     #include <string.h>
//
//  



class ToolTip
{
  public:
  ToolTip();
  ~ToolTip();
  void init (HWND hParentWin, int font_height, const char *face_name);
  void set (const char * noteText);
  void show ();
  void hide ();
  void enable () {_enable = 1;}
  void disable () {_enable = 0;}

  private:
  HWND hThisWnd;
  HWND hParentWnd;
  HINSTANCE hInstance;
  POINT hitPoint;
  //UINT timer;
  char * note;
  HFONT hfont;
  int _enable;

  static LRESULT CALLBACK TTProc (HWND, UINT, WPARAM, LPARAM);
};

