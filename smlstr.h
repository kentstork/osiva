/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * smlstr.h is part of Osiva.
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


/////////////////////////////////////////////////////////////////////////////
//
// File: smlstr.h
//
// A very small string class that just deletes itself
//

class SmlStr {

  char *_buff;

public:

  SmlStr() {
    _buff = NULL;
  }
  SmlStr (const SmlStr& rhs) {
    char *cp = rhs._buff;
    _buff = new char [strlen(cp) + 1];
    strcpy (_buff, cp);
  }
  SmlStr (char *str) {
    _buff = new char [strlen(str) + 1];
   strcpy (_buff, str);
  }
  ~SmlStr() {
    delete [] _buff;
  }

  char *buff() {return _buff;}
  operator char* ()	{return _buff;}
  operator const char* ()	const {return _buff;}

  SmlStr &operator = (const char *rhs) {
    delete[] _buff;
    _buff = new char [strlen(rhs) + 1];
    strcpy (_buff, rhs);
    return *this;
  }

};

