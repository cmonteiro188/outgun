/*

	time routine

*/

#include "time.h"

// returns time elapsed since program start, in seconds
// 
// --> isso aqui nao tem garantia de precisao. usa o clock() do time.h que no windows
//		 98 tem precisao de 50 a 100ms (==lixo)
//
//	seria MUITO LEGAL achar um jeito de programar um relogio de alta precisao que
//		fosse cross-platform!
//
double get_timeh() { 
	return (double)((double)clock()) / ((double)CLOCKS_PER_SEC); 
}
