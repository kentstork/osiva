/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * ddproxy.cpp is part of Osiva.
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
// File: ddproxy.cpp
//

#include <windows.h>
#include <ddraw.h>
#include "ddproxy.h"



///////////////////////////////////////////////////////////////////////////

DDProxy::DDProxy () {
  status = PREINIT;
  lpDDC = NULL;
  lpDirectDraw = NULL;
  ddraw = NULL;
}

///////////////////////////////////////////////////////////////////////////

DDProxy::~DDProxy () {
  if (ddraw)
    FreeLibrary (ddraw);
}

///////////////////////////////////////////////////////////////////////////

void DDProxy::init () {
  if (status != PREINIT)
    return;
  ddraw = LoadLibrary ("DDraw.dll");
  if (!ddraw) {
    status = ABSENT;
    return;
  }
  lpDDC = (LPDDC) (GetProcAddress (ddraw, "DirectDrawCreate"));
  if (!lpDDC) {
    status = FAILURE;
    return;
  }
  int ddrval = lpDDC(NULL, &lpDirectDraw, NULL); 
  if (ddrval != DD_OK) {
    status = FAILURE;
  }
  else {
    status = LOADED;
  }

}

///////////////////////////////////////////////////////////////////////////

void DDProxy::wait_for_vertical_blank () {
  if (status == LOADED) {
    lpDirectDraw->WaitForVerticalBlank (DDWAITVB_BLOCKBEGIN, NULL);
  }
}



