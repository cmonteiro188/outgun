/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin <fcecin@inf.ufrgs.br>
 */

/*

	simple log

*/

#ifndef _log_h_
#define _log_h_

#include "stdio.h"

#ifndef LOG_NOLOG

#include "timefunc.h"

#ifndef LOG_NOTIMEPRINT
#define LOG_TIME_PRINT fprintf(LOG_EXPR, "%9.2f: ", LOG_TIMEFUNC)
#else
#define LOG_TIME_PRINT 
#endif

#define LOG_OPEN(y) { LOG_EXPR = fopen(y, "w"); }
#define LOG(y) { LOG_TIME_PRINT; fprintf(LOG_EXPR, y); fflush(LOG_EXPR); }
#define LOG1(y, p1) { LOG_TIME_PRINT; fprintf(LOG_EXPR, y, p1); fflush(LOG_EXPR); }
#define LOG2(y, p1,p2) { LOG_TIME_PRINT; fprintf(LOG_EXPR, y, p1,p2); fflush(LOG_EXPR); }
#define LOG3(y, p1,p2,p3) { LOG_TIME_PRINT; fprintf(LOG_EXPR, y, p1,p2,p3); fflush(LOG_EXPR); }
#define LOG4(y, p1,p2,p3,p4) { LOG_TIME_PRINT; fprintf(LOG_EXPR, y, p1,p2,p3,p4); fflush(LOG_EXPR); }
#define LOG5(y, p1,p2,p3,p4,p5) { LOG_TIME_PRINT; fprintf(LOG_EXPR, y, p1,p2,p3,p4,p5); fflush(LOG_EXPR); }
#define LOG6(y, p1,p2,p3,p4,p5,p6) { LOG_TIME_PRINT; fprintf(LOG_EXPR, y, p1,p2,p3,p4,p5,p6); fflush(LOG_EXPR); }
#define LOG_CLOSE() { fclose(LOG_EXPR); }

#else

#define LOG_OPEN(y)
#define LOG(y)
#define LOG1(y, p1)
#define LOG2(y, p1,p2)
#define LOG3(y, p1,p2,p3)
#define LOG4(y, p1,p2,p3,p4)
#define LOG5(y, p1,p2,p3,p4,p5)
#define LOG6(y, p1,p2,p3,p4,p5,p6)
#define LOG_CLOSE()

#endif


#endif // _log_h_