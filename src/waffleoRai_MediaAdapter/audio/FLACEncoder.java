package waffleoRai_MediaAdapter.audio;

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.LinkedList;
import java.util.List;

import waffleoRai_MediaAdapter.CMediaAdapter;
import waffleoRai_MediaAdapter.CMediaAdapter.LibraryNotFoundException;
import waffleoRai_MediaAdapter.IAudioEncoder;
import waffleoRai_Sound.Sound;
import waffleoRai_SoundSynth.AudioSampleStream;
import waffleoRai_Utils.FileBuffer;

public class FLACEncoder implements IAudioEncoder{
	
	protected static final String libname = "libwraimedaptnat";
	
	private List<String> temp_paths;
	
	public FLACEncoder() throws LibraryNotFoundException{
		//Load lib on first attempt to construct. If fails, throw the LibraryNotFoundException
		
		temp_paths = new LinkedList<String>();
	}

	public int getCodec() {return IAudioEncoder.ACODEC_FLAC;}
	public boolean isLossless() {return true;}

	protected static void loadLibrary() throws LibraryNotFoundException{
		if(!CMediaAdapter.loadLibrary(libname)){
			throw new LibraryNotFoundException("FLAC native codec adapter not found!");
		}
	}
	
	protected native long openStream(String path, int sampleRate, int bitsPerSample, int channels);
	protected native long openStream(String path, int sampleRate, int bitsPerSample, int channels, int frames);
	protected native int getSamplesPerBlock(long strHandle);
	protected native int passSamples(long strHandle, int[] sample_32, int arraySize);
	protected native boolean closeStream(long strHandle);
	
	public InputStream encode(Sound src) throws IOException{
		if(temp_paths == null) throw new IOException("Encoder has been disposed of!");
		if(src == null) return null;
		
		String tpath = FileBuffer.generateTemporaryPath("waffleoMediaAdapter_FLACencoder");
		temp_paths.add(tpath);
		
		int ccount = src.totalChannels();
		int bitcount =src.getBitDepth().getBitCount();
		long handle = openStream(tpath, src.getSampleRate(), bitcount, ccount);
		if(handle == 0L){
			throw new IOException("FLACEncoder.encode || Failed to open stream!");
		}
		
		int bsz = getSamplesPerBlock(handle);
		int shamt = 32 - bitcount;
		AudioSampleStream str = src.createSampleStream(false);
		int passsize = bsz*ccount;
		while(!str.done()){
			int[] sblock = new int[passsize];
			for(int i = 0; i < bsz; i+=ccount){
				int[] samps = null;
				try{samps = str.nextSample();}
				catch(Exception x){
					x.printStackTrace();
					closeStream(handle);
					throw new IOException("FLACEncoder.encode || Error retrieving audio data. Output terminated.");
				}
				for(int c = 0; c < ccount; c++){
					sblock[i+c] = (samps[c] << shamt) >> shamt; //Ensure sign-extension
				}
			}
			if(passSamples(handle, sblock, passsize) < 0){
				closeStream(handle);
				throw new IOException("FLACEncoder.encode || Error writing stream. Output terminated.");
			}
		}
		
		closeStream(handle);
		
		//Reopen.
		BufferedInputStream is = new BufferedInputStream(new FileInputStream(tpath));
		
		return is;
	}

	public void dispose() {
		if(temp_paths == null) return;
		for(String p : temp_paths){
			try{Files.deleteIfExists(Paths.get(p));}
			catch(Exception x){
				System.err.println("ERROR: Failed to delete temp file " + p);
				x.printStackTrace();
			}
		}
		temp_paths = null;
	}

}
