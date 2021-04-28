#ifndef WRVIDUTILSUTILS_H_INCLUDED
#define WRVIDUTILSUTILS_H_INCLUDED

#include "videoDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

	WRMA_DLL_API const vid_frame_rate_t framerate_from_float(float framerate);
	WRMA_DLL_API const vid_frame_rate_t framerate_from_rational(int num, int denom);
	WRMA_DLL_API const int getCycleFrameCount(vid_frame_rate_t framerate);
	WRMA_DLL_API const uint32_t getCycleFrameLength(vid_frame_rate_t framerate, int pos);
	WRMA_DLL_API const uint32_t getCycleMillsLength(vid_frame_rate_t framerate);
	WRMA_DLL_API const double getAverageMillisPerFrame(vid_frame_rate_t framerate);

#ifdef __cplusplus
}
#endif

#endif //WRVIDUTILSUTILS_H_INCLUDED
