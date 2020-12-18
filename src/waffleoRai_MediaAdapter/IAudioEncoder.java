package waffleoRai_MediaAdapter;

import java.io.IOException;
import java.io.InputStream;

import waffleoRai_Sound.Sound;

public interface IAudioEncoder {
	
	public static final int ACODEC_PCM = 1;
	public static final int ACODEC_FLAC = 2;
	public static final int ACODEC_VORBIS = 3;
	
	public int getCodec(); //Pseudo-enum
	public boolean isLossless();

	public InputStream encode(Sound src) throws IOException;
	
	public void dispose(); //Expected to write to disk. This should wipe any temp files

}
