/*
Some quick functions for converting between Java/JNI and C++ native.
*/

#ifndef MEDIAADAPTERUTILS_H_INCLUDED
#define MEDIAADAPTERUTILS_H_INCLUDED

#include "jni.h"

#include "mediaadapterDefs.h"
#include "wr_cpp_utils.h"

namespace waffleoMediaAdapter{
	
	WRMA_DLL_API int32_t* WRMA_CDECL jintArray_to_intArray(jint* jarr, size_t len);

}

#endif 