/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * ddproxy.h is part of Osiva.
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
// File: ddproxy.h
//
// Synopsis:
//
// #include <windows.h>
// #include <ddraw.h>
//

class DDProxy {

public:

  // Loads ddraw.dll and creates a Direct Draw object
  void init ();
  // Invokes ddraw->WaitForVerticalBlank() if ddraw is present
  void wait_for_vertical_blank ();

  DDProxy ();
  ~DDProxy ();


private:

  // DDraw.dll is loaded dynamically; DirectDrawCreate()
  // is extracted using GetProcAddress()

  typedef HRESULT (WINAPI *LPDDC) (GUID FAR *, LPDIRECTDRAW FAR *, IUnknown FAR *);


  HINSTANCE ddraw;  // the handle to the loaded DDraw.dll
  LPDDC lpDDC;      // the address of the DirectDrawCreate() function
  LPDIRECTDRAW lpDirectDraw;  // the created DirectDraw Object
  enum {PREINIT, LOADED, ABSENT, FAILURE};
  int status;


};




