/* ----------------------------------------------------------------------------
   textlock - text console lock utility
   Copyright (C) 1994-2011  Mark A Lindner

   This file is part of misctools.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see
   <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------------
*/

#include "config.h"

#define TL_TIMEOUT 10 * 5    /* 5 second timeout */
#define TL_INPUT_TIMEOUT 10 * 10
#define TL_CLOCKSZ 14
#define TL_MAXPASS 16

#define TL_USAGE "[ -h ]"
#define TL_SHORTID "Textlock " VERSION
#define TL_ID TL_SHORTID " - Mark Lindner"
#define TL_INSTR "Type in password, or hit SPACE to lock."

#define KEY_DEL 127
#define KEY_BS 8
#define KEY_CR 10
#define KEY_CTRLU 21
#define KEY_CTRLT 20

/* end of header file */
