#ifndef DEBUG_H
#define DEBUG_H



#include <Arduino.h>
#include <Streaming.h>


#define __DEBUG__ 1

#if __DEBUG__ || _DEBUG

class DebugStackClass {
	typedef const __FlashStringHelper* pgmptr;

	pgmptr _fn;
	const char * _func;
public:
	DebugStackClass(pgmptr fn, const char * func, int l) {
		_fn = strrchr_P(fn, '\\');
		_func = func;
		Serial << (">Begin: ") << _fn << ":" << _func << ":" << l << endl;
	};
	~DebugStackClass() {
		Serial << ("<End:   ") << _fn << ":" << _func << endl;
	};
	static const pgmptr strrchr_P(pgmptr ptr, char c) {
		PGM_P t = NULL;
		PGM_P buf = (PGM_P)ptr;
	
		for (char tmpchar = 0; tmpchar = pgm_read_byte((PGM_P)buf); buf++) {
			if (tmpchar == c)
				t = buf;
		}
//#ifndef PGMPTR
//#define PGMPTR( pgm_ptr ) ( reinterpret_cast< pgmptr >( pgm_ptr ) )
//#endif
		//return PGMPTR(t);
		return (reinterpret_cast<pgmptr>(t));
	}

};


#define DEBUG_PRINT(x) Serial << strrchr((__FILE__), '\\') << ":" << __FUNCTION__ << ":" << __LINE__ << ": " << x << endl

#define DEBUG_STACK() DebugStackClass __DebugStack(F(__FILE__), __FUNCTION__, __LINE__)
#define DEBUG_STACK_PRINT(x) DebugStackClass __DebugStack(F(__FILE__), __FUNCTION__, __LINE__); Serial << x << endl

#else

#define DEBUG_PRINT(x) 

#define DEBUG_STACK 
#define DEBUG_STACK_PRINT(x)



#endif


#endif //DEBUG_H
