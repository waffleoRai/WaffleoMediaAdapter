package waffleoRai_MediaAdapter.video;

import java.io.InputStream;

import waffleoRai_MediaAdapter.IVideoEncoder;
import waffleoRai_Video.IVideoSource;

public class VP9Encoder implements IVideoEncoder{

	private boolean lossless;
	
	public int getCodec() {return IVideoEncoder.VCODEC_VP9;}
	public boolean isLossless() {return lossless;}

	@Override
	public InputStream encode(IVideoSource video) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void dispose() {
		// TODO Auto-generated method stub
		
	}

}
