/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * hotspot.h is part of Osiva.
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
// File: hotspot.h
//
///////////////////////////////////////////////////////////////////////////


class HotSpot {

public:

  HotSpot ();
  ~HotSpot ();

  void init( HWND parentHwnd, RECT &enclosure, int wm_message ) ;
  void arm () {mArmed = 1;};
  int armed () {return mArmed;}
  void paint ();
  void unpaint ();


private:

  HWND mHwnd;       // HWND of this instance
  HWND mParentHwnd; // Parent window 
  int mMessageID;   // Message expected by parent
  int mArmed;       // The hotspot triggers if it is armed


  LRESULT CALLBACK WindowProc (HWND, UINT, WPARAM, LPARAM);

  static LRESULT CALLBACK WindowProcProxy (HWND h, UINT u, WPARAM w, LPARAM l){
    HotSpot *p = 
      (HotSpot *) GetWindowLong(h, GWL_USERDATA) ;
    if (p)
      return p->WindowProc (h, u, w, l);
    else
      return DefWindowProc (h, u, w, l);
  }


};


