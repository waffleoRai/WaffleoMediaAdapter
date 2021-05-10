#include "waffleoRai_MediaAdapter_audio_FLACEncoder.h"
#include "quickDefs.h"
#include "FLAC++/encoder.h"
#include "mediaadapterutils.h"
#include <cstdint>

using namespace FLAC;

/*
Opens the stream and returns the stream pointer as a Java long.
If that works, no idea if it works.
*/
JNIEXPORT jlong JNICALL Java_waffleoRai_1MediaAdapter_audio_FLACEncoder_openStream__Ljava_lang_String_2III
  (JNIEnv * env, jobject obj, jstring filepath, jint samplerate, jint bitdepth, jint channels){
	  return Java_waffleoRai_1MediaAdapter_audio_FLACEncoder_openStream__Ljava_lang_String_2IIII(env, obj, filepath, samplerate, bitdepth, channels, 0);
}

/*
Stream open overload variant where number of frames is set beforehand.
*/
JNIEXPORT jlong JNICALL Java_waffleoRai_1MediaAdapter_audio_FLACEncoder_openStream__Ljava_lang_String_2IIII
  (JNIEnv * env, jobject obj, jstring filepath, jint samplerate, jint bitdepth, jint channels, jint framecount){
	  
	  //Allocate encoder
	  /*Encoder::File* enc = new Encoder::File();
	  
	  //Set encoder parameters
	  enc->set_sample_rate(static_cast<u32>(samplerate));
	  enc->set_bits_per_sample(static_cast<u32>(bitdepth));
	  enc->set_channels(static_cast<u32>(channels));
	  enc->set_total_samples_estimate(static_cast<u32>(framecount));
	  
	  //Convert string and open stream.
	  //If fail, delete encoder.
	  const char* cpath = env->GetStringUTFChars(filepath, NULL);
	  FLAC__StreamEncoderInitStatus stat = enc->init(cpath);
	  env->ReleaseStringUTFChars(filepath, cpath);
	  if(stat != FLAC__STREAM_ENCODER_INIT_STATUS_OK){
		  //Fail
		  enc->finish();
		  delete enc;
		  return 0;
	  }
	  
	  //Convert encoder pointer and return
	   return static_cast<jlong>(reinterpret_cast<uintptr_t>(enc));*/
	   return 0;
}
 
 /*
*/
JNIEXPORT jint JNICALL Java_waffleoRai_1MediaAdapter_audio_FLACEncoder_getSamplesPerBlock
  (JNIEnv * env, jobject obj, jlong ehandle){
	  /*if(ehandle == 0) return -1;
	  
	  //Resolve pointer
	  Encoder::File* enc = reinterpret_cast<Encoder::File*>(static_cast<uintptr_t>(ehandle));
	  if(enc == NULL) return -1;
	  
	  //Return result
	  uint32_t bsz = enc->get_blocksize();
	  return static_cast<jint>(bsz);*/
	  return 0;
}

/*
*/
JNIEXPORT jint JNICALL Java_waffleoRai_1MediaAdapter_audio_FLACEncoder_passSamples
  (JNIEnv * env, jobject obj, jlong ehandle, jintArray samp32, jint arrSize){
	  
	  //Resolve pointer
	 /* if(ehandle == 0) return -1;
	  Encoder::File* enc = reinterpret_cast<Encoder::File*>(static_cast<uintptr_t>(ehandle));
	  if(enc == NULL) return -1;
	  
	  //Extract array
	  if(arrSize < 1) return 0;
	  jint scount = arrSize/(static_cast<jint>(enc->get_channels()));
	  jint* csamps = env->GetIntArrayElements(samp32, NULL);
	  if(sizeof(jint) == 4){
		  FLAC__int32* fsamps = reinterpret_cast<FLAC__int32*>(csamps);
		  bool b = enc->process_interleaved(fsamps, static_cast<uint32_t>(scount));
		  env->ReleaseIntArrayElements(samp32, csamps, 0);
		  if(b) return scount;
		  else return -1;
	  }
	  else{
		  //Will have to copy one by one :|
		  FLAC__int32* fsamps = new FLAC__int32[arrSize];
		  
		  for(int i = 0; i < arrSize; i++){
			  *(fsamps+i) = static_cast<FLAC__int32>(*(csamps+i));
		  }
		  
		  bool b = enc->process_interleaved(fsamps, static_cast<uint32_t>(scount));
		  delete[] fsamps;
		  env->ReleaseIntArrayElements(samp32, csamps, 0);
		  if(b) return scount;
		  else return -1;
	  }
	  
	  return scount;*/
	  return 0;
 }

/*
*/
JNIEXPORT jboolean JNICALL Java_waffleoRai_1MediaAdapter_audio_FLACEncoder_closeStream
  (JNIEnv * env, jobject obj, jlong ehandle){
	  
	  //Resolve pointer
	 /* if(ehandle == 0) return false;
	  Encoder::File* enc = reinterpret_cast<Encoder::File*>(static_cast<uintptr_t>(ehandle));
	  if(enc == NULL) return false;
	  
	  enc->finish();
	  delete enc;*/
	  
	  return true;
  }