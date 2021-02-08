package waffleoRai_MediaAdapter.video;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.LinkedList;
import java.util.List;

import waffleoRai_MediaAdapter.CMediaAdapter;
import waffleoRai_MediaAdapter.IVideoEncoder;
import waffleoRai_Utils.VoidCallbackMethod;
import waffleoRai_MediaAdapter.CMediaAdapter.LibraryNotFoundException;
import waffleoRai_Video.IVideoSource;
import waffleoRai_Video.VideoIO;


/*
 * bitfields format...
 * 
 * 
 *
 */

public class VP9Encoder implements IVideoEncoder{
	
	protected static final String libname = "libwraimedaptnat";

	private boolean lossless;
	private List<String> temp_paths;
	
	private VoidCallbackMethod write_callback; //optional
	private int frames_per_callback = -1; //optional
	
	public VP9Encoder(){
		temp_paths = new LinkedList<String>();
	}
	
	public int getCodec() {return IVideoEncoder.VCODEC_VP9;}
	public boolean isLossless() {return lossless;}
	
	protected static void loadLibrary() throws LibraryNotFoundException{
		if(!CMediaAdapter.loadLibrary(libname)){
			throw new LibraryNotFoundException("VP9 native codec adapter not found!");
		}
	}
	
	protected native long openLosslessStream(String path, int bitfields, int frameWidth, int frameHeight, float fps);
	protected native long openLossyStream(String path, int bitfields, int frameWidth, int frameHeight, float fps, int bitrate, int keyint);
	protected native int setCallbackTime(int framesPerCallback);
	protected native int writeFrame(byte[] framedata);
	protected native int writeFrames(byte[] framedata, int frameCount);
	protected native int closeStream(long handle);

	private int generateBitfield(int dataModelEnum) {
		//TODO
		int bitfield = 0;
		//TODO Set VP9 and lossless flags as needed
		
		switch(dataModelEnum) {
		case VideoIO.CLRFMT_ARGB8_STD: break;
		case VideoIO.CLRFMT_RGB8_STD: break;
		case VideoIO.CLRFMT_YUV420: break;
		case VideoIO.CLRFMT_YUV422: break;
		case VideoIO.CLRFMT_YUV444: break;
		case VideoIO.CLRFMT_RGB444: break;
		default: return -1;
		}
		
		return 0;
	}
	
	@Override
	public InputStream encode(IVideoSource video) throws IOException{
		// TODO Auto-generated method stub
		if(video == null) return null;
		
		return null;
	}
	
	public void setWriteCallback(VoidCallbackMethod callback, int framesPerCallback) {
		write_callback = callback;
		frames_per_callback = framesPerCallback;
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
