package waffleoRai_MediaAdapter.video;

import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.LinkedList;
import java.util.List;

import waffleoRai_MediaAdapter.CMediaAdapter;
import waffleoRai_MediaAdapter.IVideoEncoder;
import waffleoRai_MediaAdapter.CMediaAdapter.LibraryNotFoundException;
import waffleoRai_Video.IVideoSource;

public class VP9Encoder implements IVideoEncoder{
	
	protected static final String libname = "wrma_nat";

	private boolean lossless;
	private List<String> temp_paths;
	
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

	@Override
	public InputStream encode(IVideoSource video) {
		// TODO Auto-generated method stub
		return null;
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
