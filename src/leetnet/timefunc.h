/*
 *  This file is part of Outgun.
 *
 *  Outgun is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Outgun is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Outgun; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright (C) 2002 - Fabio Reis Cecin <fcecin@inf.ufrgs.br>
 */
 
/*

	time util

*/

#ifndef _timefunc_h_
#define _timefunc_h_

// returns time elapsed since program start, in seconds
// 
// --> isso aqui nao tem garantia de precisao. usa o clock() do time.h que no windows
//		 98 tem precisao de 50 a 100ms (==lixo)
//
//	seria MUITO LEGAL achar um jeito de programar um relogio de alta precisao que
//		fosse cross-platform!
//
double get_timeh();

#endif // _timefunc_h_
