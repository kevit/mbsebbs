/*****************************************************************************
 *
 * $Id$
 * Purpose ...............: Change integer value CPU endian independant
 *
 *****************************************************************************
 * Copyright (C) 1997-2003
 *   
 * Michiel Broek		FIDO:	2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../config.h"
#include "libs.h"
#include "memwatch.h"
#include "structs.h"
#include "common.h"


/* 
 * Return an integer in endian independent format. This is used to make menu
 * files portable on little and big-endian systems. Normal datafiles are
 * not portable, only the menus.
 */
int le_int(int val)
{
#ifdef BYTE_ORDER
    if (BYTE_ORDER == 1234) {
	return val;
    } else if (BYTE_ORDER == 4321) {
	return ((val & 0xff) << 24) | (((val >> 8) & 0xff) << 16) | (((val >> 16) & 0xff) << 8) | ((val >> 24) & 0xff);
    } else {
#endif
#ifdef __i386__
	return val;
#else
	return ((val & 0xff) << 24) | (((val >> 8) & 0xff) << 16) | (((val >> 16) & 0xff) << 8) | ((val >> 24) & 0xff);
#endif
#ifdef BYTE_ORDER
    }
#endif
    return val;
}


