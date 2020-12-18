package waffleoRai_MediaAdapter;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class CMediaAdapter {

	public static class LibraryNotFoundException extends Exception{
		private static final long serialVersionUID = 1001869144493550797L;
		
		public LibraryNotFoundException(String s){
			super(s);
		}
	}
	
	private static Set<String> attempt_map;
	private static Map<String, Exception> load_exp_map;
	
	static{
		attempt_map = new HashSet<String>();
		load_exp_map = new HashMap<String, Exception>();
	}
	
	public static boolean loadLibrary(String libname){
		if(libname == null) return false;
		if(attempt_map.contains(libname)){return load_exp_map.get(libname) == null;}
		
		attempt_map.add(libname);
		try{System.loadLibrary(libname);}
		catch(Exception x){load_exp_map.put(libname, x); return false;}
		
		return true;
	}
	
	public static Exception getLoadException(String libname){
		if(libname == null) return null;
		return load_exp_map.get(libname);
	}
	
}
