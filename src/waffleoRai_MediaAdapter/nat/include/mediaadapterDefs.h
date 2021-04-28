#ifndef WRMEDADAPDEFS_H_INCLUDED
#define WRMEDADAPDEFS_H_INCLUDED

//https://www.oreilly.com/library/view/c-cookbook/0596007612/ch01s01.html#cplusplusckbk-CHP-1-EX-2
# if defined(_WIN32) && !defined(__GNUC__)
#  ifdef WRMA_DLL_BUILD
#   define WRMA_DLL_API _declspec(dllexport)
#  else
#   define WRMA_DLL_API _declspec(dllimport)
#  endif 
#	define WRMA_CDECL __cdecl
#	define WRMA_STDCALL __stdcall
#else
#	define WRMA_DLL_API
#	define WRMA_CDECL
#	define WRMA_STDCALL
# endif

#include "wr_c_utils.h"

#endif //WRMEDADAPDEFS_H_INCLUDED
