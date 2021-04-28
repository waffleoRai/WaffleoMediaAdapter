#include "vidUtils.h"

const vid_frame_rate_t framerate_from_float(float framerate) {

	if (framerate == 60.0) return FPS_60;
	else if (framerate == 59.94) return FPS_59_94;
	else if (framerate == 50.0) return FPS_50;
	else if (framerate == 30.0) return FPS_30;
	else if (framerate ==29.97) return FPS_29_97;
	else if (framerate == 25.0) return FPS_25;
	else if (framerate == 24.0) return FPS_24;
	else if (framerate == 23.976) return FPS_23_976;
	else if (framerate == 20.0) return FPS_20;
	else if (framerate == 15.0) return FPS_15;
	else if (framerate == 14.985) return FPS_14_985;

	return FPS_UNKNOWN;
}

const vid_frame_rate_t framerate_from_rational(int num, int denom) {

	if (num == 1) {
		switch (denom) {
		case 15: return FPS_15;
		case 20: return FPS_20;
		case 24: return FPS_24;
		case 25: return FPS_25;
		case 30: return FPS_30;
		case 50: return FPS_50;
		case 60: return FPS_60;
		}
	}
	else {
		//Hell if I know
		if (num == 2997) {
			if(denom == 100) return FPS_29_97;
			if(denom == 125) return FPS_23_976;
			if (denom == 50) return FPS_59_94;
			if(denom == 200) return FPS_14_985;
		}
	}

	return FPS_UNKNOWN;
}

const int getCycleFrameCount(vid_frame_rate_t framerate) {

	switch (framerate) {
	case FPS_14_985: return 2997; //Complex
	case FPS_15: return 3; //67 66 67
	case FPS_20: return 1; //50
	case FPS_23_976: return 2997;
	case FPS_24: return 3; //42 41 42
	case FPS_25: return 1; //40
	case FPS_29_97: return 2997;
	case FPS_30: return 3; //33 34 33
	case FPS_50: return 1; //20
	case FPS_59_94: return 2997;
	case FPS_60: return 3; //17 16 17
	}

	return -1;
}

const uint32_t getCycleFrameLength(vid_frame_rate_t framerate, int pos) {

	uint32_t a = 0, b = 0;
	int group = 0;

	switch (framerate) {
	case FPS_20: return 50;
	case FPS_25: return 40;
	case FPS_50: return 20;
	case FPS_15: group = 1; a = 67; b = 66; break;
	case FPS_24: group = 1; a = 42; b = 41; break;
	case FPS_30: group = 1; a = 33; b = 34; break;
	case FPS_60: group = 1; a = 17; b = 16; break;
	case FPS_14_985: group = 2; a = 67; b = 66; break;
	case FPS_23_976: group = 2; a = 42; b = 41; break;
	case FPS_29_97: group = 2; a = 33; b = 34; break;
	case FPS_59_94: group = 2; a = 17; b = 16; break;
	}

	if (group == 1) {
		int mod = pos % 3;
		if (mod == 1) return b;
		return a;
	}
	else if (group == 2) {
		int mod = pos % 2997;
		bool ablock = true;
		int blockpos = 0;
		if (mod >= 2970) {
			//Last A block
			blockpos = mod - 2970;
		}
		else {
			mod = mod % 330;
			if (mod >= 324) {
				ablock = false;
				blockpos = mod - 324;
			}
			else {
				mod = mod % 27;
				blockpos = mod;
			}
		}
		if (ablock) {
			mod = blockpos % 27;
			int mblock = mod / 9;
			mod = mod % 3;
			if (mblock == 4) {
				if (mod == 1) return a;
				return b;
			}
			else {
				if (mod == 1) return b;
				return a;
			}
		}
		else {
			mod = blockpos % 6;
			if (mod == 2) return b;
			return a;
		}
	}

	return 0;
}

const uint32_t getCycleMillsLength(vid_frame_rate_t framerate) {
	switch (framerate) {
	case FPS_14_985: return 200000;
	case FPS_15: return 200;
	case FPS_20: return 50;
	case FPS_23_976: return 125000;
	case FPS_24: return 125;
	case FPS_25: return 40;
	case FPS_29_97: return 100000;
	case FPS_30: return 100;
	case FPS_50: return 20;
	case FPS_59_94: return 50000;
	case FPS_60: return 50;
	}

	return 0;
}

const double getAverageMillisPerFrame(vid_frame_rate_t framerate) {

	switch (framerate) {
	case FPS_14_985: return 1000.0 / 14.985;
	case FPS_15: return 1000.0 / 15.0;
	case FPS_20: return 50;
	case FPS_23_976: return 1000.0 / 23.976;
	case FPS_24: return 1000.0 / 24.0;
	case FPS_25: return 40;
	case FPS_29_97: return 1000.0 / 29.97;
	case FPS_30: return 1000.0 / 30.0;
	case FPS_50: return 20;
	case FPS_59_94: return 1000.0 / 59.94;
	case FPS_60: return 1000.0 / 60.0;
	}

	return 0.0;
}