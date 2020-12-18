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
	
	protected static final String libname = "wrma_nat";
	
	private List<String> temp_paths;
	
	public FLACEncoder(){
		temp_paths = new LinkedList<String>();
	}

	public int getCodec() {return IAudioEncoder.ACODEC_FLAC;}
	public boolean isLossless() {return true;}

	protected static void loadLibrary() throws LibraryNotFoundException{
		if(!CMediaAdapter.loadLibrary(libname)){
			throw new LibraryNotFoundException("FLAC native codec adapter not found!");
		}
	}
	
	protected native boolean openStream(String path, int sampleRate, int bitsPerSample, int channels);
	protected native int getSamplesPerBlock();
	protected native boolean passSamples(int[][] sample_32, int sampleCount);
	protected native void closeStream();
	
	public InputStream encode(Sound src) throws IOException{
		if(temp_paths == null) throw new IOException("Encoder has been disposed of!");
		if(src == null) return null;
		
		String tpath = FileBuffer.generateTemporaryPath("waffleoMediaAdapter_FLACencoder");
		temp_paths.add(tpath);
		
		int ccount = src.totalChannels();
		if(!openStream(tpath, src.getSampleRate(), src.getBitDepth().getBitCount(), ccount)){
			throw new IOException("FLACEncoder.encode || Failed to open stream!");
		}
		
		int bsz = getSamplesPerBlock();
		AudioSampleStream str = src.createSampleStream(false);
		while(!str.done()){
			int[][] sblock = new int[ccount][bsz];
			for(int i = 0; i < bsz; i++){
				int[] samps = null;
				try{samps = str.nextSample();}
				catch(Exception x){
					x.printStackTrace();
					closeStream();
					throw new IOException("FLACEncoder.encode || Error retrieving audio data. Output terminated.");
				}
				for(int c = 0; c < ccount; i++){
					sblock[c][i] = samps[c];
				}
			}
			if(!passSamples(sblock, bsz)){
				closeStream();
				throw new IOException("FLACEncoder.encode || Error writing stream. Output terminated.");
			}
		}
		
		closeStream();
		
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
