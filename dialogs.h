/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * dialogs.h is part of Osiva.
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
// File: dialogs.h
//
// Synopsis:
//
// #include <windows.h>
// #include "hotspot.h"
//
//////////////////////////////////////////////////////////////////////////////

class FlipDialog {

public:

  void init (HINSTANCE);
  void apply ();
  void refresh ();
  HWND hwnd_main;

protected:

  HWND eb_seconds;
  HWND eb_seconds_dislv;
  HWND cb_flip;
  HWND cb_dissolve;
  HWND cm_dissolve_bits;
  HWND bt_hotspot1;
  HWND bt_hotspot2;
  HWND bt_apply;

  HotSpot hotSpot1;
  HotSpot hotSpot2;

  virtual BOOL CALLBACK DialogProc (HWND, UINT, WPARAM, LPARAM);

  static BOOL CALLBACK DialogProcProxy (HWND h, UINT u, WPARAM w, LPARAM l){
    FlipDialog *p = 
      (FlipDialog *) GetWindowLong(h, GWL_USERDATA) ;
    if (p)
      return p->DialogProc (h, u, w, l);
    else
      return FALSE;
  }

};


//////////////////////////////////////////////////////////////////////////////


class TransDialog {

public:

  void init (HINSTANCE);
  void apply ();
  HWND hwnd_main;

protected:

  HWND eb_bgdiff;
  HWND eb_depth;
  HWND eb_erosions;

  virtual BOOL CALLBACK DialogProc (HWND, UINT, WPARAM, LPARAM);

  static BOOL CALLBACK DialogProcProxy (HWND h, UINT u, WPARAM w, LPARAM l){
    TransDialog *p = 
      (TransDialog *) GetWindowLong(h, GWL_USERDATA) ;
    if (p)
      return p->DialogProc (h, u, w, l);
    else
      return FALSE;
  }

};


