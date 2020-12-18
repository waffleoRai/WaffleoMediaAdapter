package waffleoRai_MediaAdapter.video;

import java.io.IOException;
import java.util.ArrayList;

import waffleoRai_MediaAdapter.CMediaAdapter;
import waffleoRai_MediaAdapter.IAudioEncoder;
import waffleoRai_MediaAdapter.IVideoEncoder;
import waffleoRai_MediaAdapter.CMediaAdapter.LibraryNotFoundException;
import waffleoRai_Sound.Sound;
import waffleoRai_Video.IVideoSource;

public class MatroskaAdapter {
	
	/*----- Constants -----*/
	
	/*----- Instance Variables -----*/
	
	private ArrayList<IVideoSource> vtracks;
	private ArrayList<Sound> atracks;
	
	private ArrayList<IVideoEncoder> vcodecs;
	private ArrayList<IAudioEncoder> acodecs;
	
	/*----- Native Interface -----*/
	
	/*----- Java Interface -----*/
	
	public MatroskaAdapter(){
		vtracks = new ArrayList<IVideoSource>();
		atracks = new ArrayList<Sound>();
		vcodecs = new ArrayList<IVideoEncoder>();
		acodecs = new ArrayList<IAudioEncoder>();
	}

	public static boolean checkLibrary(){
		//TODO
		//Checks for the adapter library. If not there, return false.
		return false;
	}
	
	public void addVideoTrack(IVideoSource video, IVideoEncoder codec){
		if(video == null) return;
		vtracks.add(video);
		//if(codec == null) codec = new VideoCodecInfo();
		vcodecs.add(codec);
	}
	
	public void addAudioTrack(Sound sound, IAudioEncoder codec){
		if(sound == null) return;
		atracks.add(sound);
		//if(codec == null) codec = new AudioCodecInfo();
		acodecs.add(codec);
	}
	
	public void clearVideoTracks(){
		vtracks.clear();
		vcodecs.clear();
	}
	
	public void clearAudioTracks(){
		atracks.clear();
		acodecs.clear();
	}
	
	public int videoTrackCount(){return vtracks.size();}
	public int audioTrackCount(){return atracks.size();}
	
	public boolean writeMatroskaFile(String outPath) throws IOException, LibraryNotFoundException{
		//TODO
		return false;
	}
	
}
