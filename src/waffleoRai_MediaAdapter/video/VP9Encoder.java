package waffleoRai_MediaAdapter.video;

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.LinkedList;
import java.util.List;

import waffleoRai_MediaAdapter.CMediaAdapter;
import waffleoRai_MediaAdapter.IVideoEncoder;
import waffleoRai_Utils.FileBuffer;
import waffleoRai_Utils.VoidCallbackMethod;
import waffleoRai_MediaAdapter.CMediaAdapter.LibraryNotFoundException;
import waffleoRai_Video.IVideoSource;
import waffleoRai_Video.VideoFrameStream;
import waffleoRai_Video.VideoIO;


/*
 * bitfields format...
 * 		0   VP9? (If unset, then use VP8)
        1   Lossless?
        2-4 Color Model
            0   RGB
            1   ARGB
            2   YUV BT.601 (SD)
            3   YUV BT.701 (HD)
            4   YUV BT.2020
        5-7 Pixel Format
            0   By pixel (8 bits per channel ARGB, RGB, or YUV)
            1   YUV I420
            2   YUV I422
            3   Planar 444
 * 
 *
 */

public class VP9Encoder implements IVideoEncoder{
	
	protected static final String libname = "libwraimedaptnat";
	
	private boolean lossless;
	private int bitrate;
	private int keyint;
	
	private List<String> temp_paths;
	
	private VoidCallbackMethod write_callback; //optional
	private int frames_per_callback = -1; //optional
	
	public VP9Encoder() throws LibraryNotFoundException{
		if(CMediaAdapter.libraryLoadable(libname)) loadLibrary();
		if(!CMediaAdapter.libraryLoaded(libname)) throw new LibraryNotFoundException("VP9 native codec adapter not found!");
		
		temp_paths = new LinkedList<String>();
	}
	
	public int getCodec() {return IVideoEncoder.VCODEC_VP9;}
	public boolean isLossless() {return lossless;}
	
	protected static void loadLibrary() throws LibraryNotFoundException{
		if(!CMediaAdapter.loadLibrary(libname)){
			throw new LibraryNotFoundException("VP9 native codec adapter not found!");
		}
	}
	
	public void setLossless(boolean b) {lossless = b;}
	public void setBitrate(int val) {bitrate = val;}
	public void setKeyInterval(int val) {keyint = val;}
	public void setCallback(VoidCallbackMethod method, int framesPerCallback) {write_callback = method; frames_per_callback = framesPerCallback;}
	
	protected native long openLosslessStream(String path, int bitfields, int frameWidth, int frameHeight, float fps);
	protected native long openLossyStream(String path, int bitfields, int frameWidth, int frameHeight, float fps, int bitrate, int keyint);
	protected native int setCallbackTime(long handle, int framesPerCallback);
	protected native int writeFrame(long handle, byte[] framedata);
	protected native int writeFrames(long handle, byte[] framedata, int frameCount);
	protected native int closeStream(long handle);

	private int generateBitfield(int dataModelEnum, int clrspaceEnum, boolean analog_color) {
		int bitfield = 0x1; //Set VP9
		if(lossless) bitfield |= 0x2;
		if(!analog_color) bitfield |= 0x20;
		
		int fmtval = 0;
		switch(dataModelEnum) {
		case VideoIO.CLRFMT_ARGB8_BYPIX: fmtval = 0; break;
		case VideoIO.CLRFMT_RGB8_BYPIX: fmtval = 0; break;
		case VideoIO.CLRFMT_YUV420: fmtval = 1; break;
		case VideoIO.CLRFMT_YUV422: fmtval = 2; break;
		case VideoIO.CLRFMT_YUV444: fmtval = 3; break;
		case VideoIO.CLRFMT_RGB444: fmtval = 3; break;
		case VideoIO.CLRFMT_YUV420_16LE: fmtval = 1; bitfield |= 0x10; break;
		case VideoIO.CLRFMT_YUV422_16LE: fmtval = 2; bitfield |= 0x10; break;
		case VideoIO.CLRFMT_YUV444_16LE: fmtval = 3; bitfield |= 0x10; break;
		default: return -1;
		}
		bitfield |= fmtval << 5;
		
		int clrval = 0;
		switch(clrspaceEnum) {
		case VideoIO.CLRSPACE_ARGB8: clrval = 1; break;
		case VideoIO.CLRSPACE_RGB8: clrval = 0; break;
		case VideoIO.CLRSPACE_YUV_SD: clrval = 2; break;
		case VideoIO.CLRSPACE_YUV_HD: clrval = 3; break;
		case VideoIO.CLRSPACE_YUV_2020: clrval = 4; break;
		default: return -1;
		}
		bitfield |= clrval << 2;
		
		return bitfield;
	}
	
	public InputStream encode(IVideoSource video) throws IOException{
		if(video == null) return null;
		int fmtType = video.getRawDataFormat();
		int clrType = video.getRawDataColorspace();
		int fwidth = video.getWidth();
		int fheight = video.getHeight();
		double frate = video.getFrameRate();
		
		//Generate temp path
		String temppath = FileBuffer.generateTemporaryPath("vp9encoder_strtemp");
		temp_paths.add(temppath);
		int bitfields = generateBitfield(fmtType, clrType, video.rawOutputAnalogColor());
		
		//Open
		long handle = 0;
		if(lossless) handle = openLosslessStream(temppath, bitfields, fwidth, fheight, (float)frate);
		else handle = openLossyStream(temppath, bitfields, fwidth, fheight, (float)frate, bitrate, keyint);
		if(handle == 0L) throw new IOException("NULL handle - encode stream open failed!");
		
		//Write frames
		int wf = 0; //written frames
		VideoFrameStream vstr = video.openStream();
		while(!vstr.done()) {
			byte[] fdat = vstr.getNextFrameData();
			int rval = writeFrame(handle, fdat);
			if(rval != 0) {
				vstr.close();
				closeStream(handle);
				throw new IOException("ERROR Occurred while writing frame " + wf + " | error code: " + rval);
			}
			
			wf++;
			if(frames_per_callback > 0 && write_callback != null && wf % frames_per_callback == 0) write_callback.doMethod();
		}
		vstr.close();
		
		//Close
		int rval = closeStream(handle);
		if(rval != 0) throw new IOException("Failed to properly close encoding stream - error code: " + rval);
		
		//Reopen in Java
		BufferedInputStream bis = new BufferedInputStream(new FileInputStream(temppath));
		
		return bis;
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
