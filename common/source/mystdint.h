#ifndef __mystdint_h
#define __mystdint_h

#ifdef WIN32
	typedef __int64 int64_t;
	typedef short int16_t;
#else
	#include <stdint.h>
#endif

#endif
