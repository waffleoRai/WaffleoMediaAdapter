#include "mediaadapterutils.h"

namespace waffleoMediaAdapter{
	
int32_t* jintArray_to_intArray(jint* array, int size){

	if(sizeof(jint) == 4){
		int32_t* newarr = reinterpret_cast<int32_t*>(array);
		return newarr;
	}
	else{
		//Will have to copy one by one :|
		  int32_t* newarr = new int32_t[size];
		  
		  for(int i = 0; i < size; i++){
			  *(newarr+i) = static_cast<int32_t>(*(array+i));
		  }
		  
		  return newarr;
	  }

}
	
}
