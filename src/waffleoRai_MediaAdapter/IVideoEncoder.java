package waffleoRai_MediaAdapter;

import java.io.IOException;
import java.io.InputStream;

import waffleoRai_Video.IVideoSource;

public interface IVideoEncoder {
	
	public static final int VCODEC_H264 = 1;
	public static final int VCODEC_VP9 = 2;
	
	public int getCodec(); //Pseudo-enum
	public boolean isLossless();

	public InputStream encode(IVideoSource video) throws IOException;
	
	public void dispose(); //Expected to write to disk. This should wipe any temp files

}
