/*
Some quick functions for converting between Java/JNI and C++ native.
*/

#ifndef MEDIAADAPTERUTILS_H_INCLUDED
#define MEDIAADAPTERUTILS_H_INCLUDED

//https://www.oreilly.com/library/view/c-cookbook/0596007612/ch01s01.html#cplusplusckbk-CHP-1-EX-2
# if defined(_WIN32) && !defined(__GNUC__)
#  ifdef WAFFLEORAIMEDIADAPTER_DLL
#   define WAFFLEORAIMEDIADAPTER_DECL _  _declspec(dllexport)
#  else
#   define WAFFLEORAIMEDIADAPTER_DECL _  _declspec(dllimport)
#  endif 
# endif

#include <jni.h>
#include "quickDefs.h"


namespace waffleoMediaAdapter{
	
int32_t* jintArray_to_intArray(jint* array, int size);
	
}

#endif 