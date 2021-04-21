#include "mediaadapterutils.h"

namespace waffleoMediaAdapter{
	
int32_t* jintArray_to_intArray(jint* jarr, size_t len){

	if(sizeof(jint) == 4){
		int32_t* newarr = reinterpret_cast<int32_t*>(jarr);
		return newarr;
	}
	else{
		//Will have to copy one by one :|
		  int32_t* newarr = new int32_t[len];
		  
		  for(int i = 0; i < len; i++){
			  *(newarr+i) = static_cast<int32_t>(*(jarr +i));
		  }
		  
		  return newarr;
	  }

}
	
}
