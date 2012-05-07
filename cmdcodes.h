/*******************************************************************************
 * Copyright 2002, 2003, 2004, 2005, 2006, 2012 Kent Stork
 *
 * cmdcodes.h is part of Osiva.
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
// File: cmdcodes.h
//
// Synopsis:
//
// Description:
//
//  Provides the "command codes" for commands that can show up on the 
//  icon-bar. Contains an array of strings which allows mapping
//  between a string token and the command code. The index of the 
//  string is (had better be) the same as the command code.
//  


// The command codes as used by the command selection "switch" statement
 
enum {          /* Comments are counts, not index values */
  CMDCODE_SPACE,
  CMDCODE_BACKSPACE,
  CMDCODE_PGUP,
  CMDCODE_PGDWN,
  CMDCODE_HOME, /* 5 */
  CMDCODE_END,
  CMDCODE_SINK,
  CMDCODE_TRANS,
  CMDCODE_R1,
  CMDCODE_R2, /* 10 */
  CMDCODE_R3,
  CMDCODE_R4,
  CMDCODE_R5,
  CMDCODE_R6,
  CMDCODE_R7, /* 15 */
  CMDCODE_R8,
  CMDCODE_R9,
  CMDCODE_MENU,
  CMDCODE_MINIMIZE,
  CMDCODE_CLOSE, /* 20 */
  CMDCODE_FITSCREEN,
  CMDCODE_BIGGER,
  CMDCODE_SMALLER,
  CMDCODE_TILE,
  CMDCODE_ROTATE, /* 25 */
  CMDCODE_TAB,
  CMDCODE_DRAG,
  CMDCODE_EOA

};

// The tokens used to bind a command to a command code

static char *CC_command_name[] = {
  "push-apart",
  "pull-together",
  "collect",
  "distribute",
  "center", /* 5 */
  "corner",
  "sink",
  "transparent",
  "size-1:1",
  "size-1:2", /* 10 */
  "size-1:3",
  "size-1:4",
  "size-1:5",
  "size-1:6",
  "size-1:7", /* 15 */
  "size-1:8",
  "size-1:9",
  "menu",
  "minimize",
  "close", /* 20 */
  "size-screen",
  "size-bigger",
  "size-smaller",
  "tile",
  "rotate", /* 25 */
  "tab",
  "drag",
  0
};


// Tooltip text

static char *CC_tooltip[] = {
"Push Images Apart [Space]",
"Pull Images Together [Back Space]",
"Collect Images Top-Right [Page Up]",
"Distribute Images Across Screen [Page Down]",
"Center Images [Home]", /* 5 */
"Snug Images Top-Right, 1/8 [End]",
"Sink Image to Bottom of Stack [S]",
"Make Image Transparent [T]",
"Actual Pixels [1]",
"Zoom All 1/2 [2]", /* 10 */
"Zoom All 1/3 [3]",
"Zoom All 1/4 [4]",
"Zoom All 1/5 [5]",
"Zoom All 1/6 [6]",
"Zoom All 1/7 [7]", /* 15 */
"Zoom All 1/8 [8]",
"Zoom All 1/9 [9]",
"Menu",
"Minimize",
"Close and Quit", /* 20 */
"Fit Top Image To Screen",
"Make Default Thumbnail Bigger [2-9]",
"Make Default Thumbnail Smaller [2-9]",
"Tile Images Across Screen",
"Rotate Image [L:R]", /* 25 */
"Select Next Image [Tab]",
"Move Iconbar",
0
};


// Flags

#define CMDCODE_MSK_REPEATS 0x01
#define CMDCODE_MSK_ONDOWN 0x02
#define CMDCODE_MSK_DRAGAREA 0x04

static unsigned char CC_flags[] = {
  0x01,
  0x01,
  0x00,
  0x00,
  0x00, /* 5 */
  0x00,
  0x00,
  0x00,
  0x00,
  0x00, /* 10 */
  0x00,
  0x00,
  0x00,
  0x00,
  0x00, /* 15 */
  0x00,
  0x00,
  0x02,
  0x00,
  0x00, /* 20 */
  0x00,
  0x00,
  0x00,
  0x00,
  0x00, /* 25 */
  0x00,
  0x04,
  0x00
};





