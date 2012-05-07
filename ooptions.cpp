/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * ooptions.cpp is part of Osiva.
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
// File: ooptions.cpp
//

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <vector>
using namespace std;
#include "ooptions.h"

///////////////////////////////////////////////////////////////////////////

OOptions::OOptions () {
  program_path = NULL;
  reg_gif_command = NULL;
  reg_jpg_command = NULL;
  reg_option_string = NULL;
  default_options () ;
}

///////////////////////////////////////////////////////////////////////////

OOptions::~OOptions () {
  delete [] program_path;
  delete [] reg_gif_command;
  delete [] reg_jpg_command;
  delete [] reg_option_string;
}

///////////////////////////////////////////////////////////////////////////

int OOptions::default_options () {

  flip = 0;
  flip_sec = 3;
  flip_ms = 0;

  dissolve = 0;
  dissolve_bits = 6;
  dissolve_sec = 0;
  dissolve_ms = 10;

  minimize = 0;
  suppress_zoom = 0;
  stayup = 0;
  ontop = 1;
  right_click_closes = 0;

  depth = 2;
  erosions = 1;
  bg_diff = 15;

  reduction = 8;

  osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
  GetVersionEx (&osvi);
  
  return 0;
}

///////////////////////////////////////////////////////////////////////////

int OOptions::read_reg_commands () {

  return 0;
}

///////////////////////////////////////////////////////////////////////////

int OOptions::write_reg_commands () {

  return 0;
}

///////////////////////////////////////////////////////////////////////////

int OOptions::read_reg_options () {

  return 0;
}

///////////////////////////////////////////////////////////////////////////

int OOptions::write_reg_options () {

  return 0;
}

///////////////////////////////////////////////////////////////////////////

void OOptions::subscribe (HWND client_hwnd, int wparam) {
  // TODO: make sure a HWND cannot subscribe more than once
  Client cli;
  cli.hwnd = client_hwnd;
  cli.message = wparam;
  client.push_back (cli);
}

///////////////////////////////////////////////////////////////////////////

void OOptions::broadcast (int option_set, HWND sender) {
  for (int c = 0; c < client.size(); c++)
    if (client[c].hwnd != sender)
      SendMessage (client[c].hwnd, WM_APP,
      client[c].message, option_set);
}



