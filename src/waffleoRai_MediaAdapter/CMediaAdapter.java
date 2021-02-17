package waffleoRai_MediaAdapter;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import waffleoRai_Utils.FileBuffer;

public class CMediaAdapter {
	
	public static final int OS_UNKNOWN = 0;
	public static final int OS_LINUX = 1;
	public static final int OS_WINDOWS = 2;
	public static final int OS_MACOSX = 3;
	
	public static final int ARCH_UNKNOWN = 0;
	public static final int ARCH_X86 = 1; //32 bit x86
	public static final int ARCH_X64 = 2; //64 bit x86
	public static final int ARCH_ARM32 = 3;
	public static final int ARCH_ARM64 = 4;
	public static final int ARCH_PPC64_BE = 5;
	public static final int ARCH_PPC64_LE = 6;

	public static class LibraryNotFoundException extends Exception{
		private static final long serialVersionUID = 1001869144493550797L;
		
		public LibraryNotFoundException(String s){
			super(s);
		}
	}
	
	private static int my_os = -1;
	private static int my_arch = -1;
	
	private static Set<String> attempt_map;
	private static Map<String, Exception> load_exp_map;
	private static List<String> temp_files;
	
	static{
		attempt_map = new HashSet<String>();
		load_exp_map = new HashMap<String, Exception>();
		temp_files = new LinkedList<String>();
		detectArch();
	}
	
	private static void detectArch() {
		String osname = System.getProperty("os.name").toLowerCase();
		String osarch = System.getProperty("os.arch").toLowerCase();
		
		System.err.println("CMediaAdapter.detectArch || -DEBUG- JVM Says: " + osname + " | " + osarch);
		
		if(osname.contains("windows")) {
			my_os = OS_WINDOWS;
			System.err.println("CMediaAdapter.detectArch || -DEBUG- OS Family Detected: Windows");
		}
		else if (osname.contains("linux") || osname.contains("ubuntu") || osname.contains("fedora")) {
			my_os = OS_LINUX;
			System.err.println("CMediaAdapter.detectArch || -DEBUG- OS Family Detected: Linux");
		}
		else if (osname.contains("osx")){
			my_os = OS_MACOSX;
			System.err.println("CMediaAdapter.detectArch || -DEBUG- OS Family Detected: Mac OSX");
		}
		else {
			my_os = OS_UNKNOWN;
			System.err.println("CMediaAdapter.detectArch || -DEBUG- OS Family Detected: <UNKNOWN>");
		}
		
		
		if(osarch.equals("amd64") || osarch.contains("x64") || osarch.contains("x86-64") || osarch.contains("x86_64")) {
			my_arch = ARCH_X64;
			System.err.println("CMediaAdapter.detectArch || -DEBUG- Architecture Family Detected: x86_64");
		}
		else if(osarch.equals("x86") || osarch.contains("i386") || osarch.contains("i486") || osarch.contains("i586") || osarch.contains("i686")) {
			my_arch = ARCH_X86;
			System.err.println("CMediaAdapter.detectArch || -DEBUG- Architecture Family Detected: x86");
		}
		else if(osarch.equals("arm")) {
			my_arch = ARCH_ARM32;
			System.err.println("CMediaAdapter.detectArch || -DEBUG- Architecture Family Detected: ARM 32");
		}
		else if(osarch.equals("aarch64")) {
			my_arch = ARCH_ARM64;
			System.err.println("CMediaAdapter.detectArch || -DEBUG- Architecture Family Detected: ARM 64");
		}
		else if(osarch.equals("ppc64")) {
			if(osarch.contains("le")) {
				my_arch = ARCH_PPC64_LE;
				System.err.println("CMediaAdapter.detectArch || -DEBUG- Architecture Family Detected: PowerPC 64 (LE)");
			}
			else {
				my_arch = ARCH_PPC64_BE;
				System.err.println("CMediaAdapter.detectArch || -DEBUG- Architecture Family Detected: PowerPC 64");
			}
		}
		else {
			my_arch = ARCH_UNKNOWN;
			System.err.println("CMediaAdapter.detectArch || -DEBUG- Architecture Family Detected: <UNKNOWN>");
		}
		
	}
	
	public static String getLibraryPath(String libname) {
		final String nat_pkg_path = "waffleoRai_MediaAdapter/nat/bin";
		String archdir = null;
		String osdir = null;
		String ext = null;
		
		switch(my_os) {
		case OS_LINUX:
			ext = "so";
			osdir = "lnx";
			break;
		case OS_MACOSX:
			ext = "so";
			osdir = "osx";
			break;
		case OS_WINDOWS:
			ext = "dll";
			osdir = "win";
			break;
		default: return null;
		}
		
		switch(my_arch) {
		case ARCH_X86: archdir = "x86"; break;
		case ARCH_X64: archdir = "x64"; break;
		case ARCH_ARM32: archdir = "arm32"; break;
		case ARCH_ARM64: archdir = "arm64"; break;
		case ARCH_PPC64_BE: archdir = "ppc64"; break;
		case ARCH_PPC64_LE: archdir = "ppc64le"; break;
		default: return null;
		}
		
		
		return nat_pkg_path + "/" + osdir + "/" + archdir + "/" + libname + "." + ext;
	}
	
	private static String extractLibrary(String libname) throws IOException {
		String jarpath = getLibraryPath(libname);
		if(jarpath == null) return null;
		
		String tempdir = FileBuffer.getTempDir();
		String temppath = tempdir + File.separator + jarpath.replace("/", "_");
		
		InputStream is = CMediaAdapter.class.getResourceAsStream(jarpath);
		BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(temppath));
		int b = -1;
		while((b = is.read()) != -1) bos.write(b);
		is.close();
		bos.close();
		
		if(temppath != null) temp_files.add(temppath);
		return temppath;
	}
	
	public static boolean loadLibrary(String libname){
		if(libname == null) return false;
		if(attempt_map.contains(libname)){return load_exp_map.get(libname) == null;}
		
		attempt_map.add(libname);
		try{
			String libtemppath = extractLibrary(libname);
			System.load(libtemppath);
			Files.deleteIfExists(Paths.get(libtemppath));
		}
		catch(Exception x){load_exp_map.put(libname, x); return false;}
		
		return true;
	}
	
	public static Exception getLoadException(String libname){
		if(libname == null) return null;
		return load_exp_map.get(libname);
	}
	
	public static void clearTempFiles() {
		for(String s : temp_files) {
			try {Files.deleteIfExists(Paths.get(s));}
			catch(IOException x) {x.printStackTrace(); return;}
		}
		temp_files.clear();
	}

	public static boolean libraryLoadable(String libname) {
		//Library must not be currently loaded and previous attempt must not have been made since last failure clear
		return !attempt_map.contains(libname) && !load_exp_map.containsKey(libname);
	}
	
	public static boolean libraryLoaded(String libname) {
		return attempt_map.contains(libname) && !load_exp_map.containsKey(libname);
	}
	
	public static void clearAttemptFailures() {
		//Successes and failures are both in attempt_map.
		//But only failures are in the exception map.
		//So delete any library names that are in the exception map AND attempt map
		//And clear exception map

		attempt_map.removeAll(load_exp_map.keySet());
		load_exp_map.clear();
	}
	
}
