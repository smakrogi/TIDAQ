/**
 * JNI for PQCT analysis pipelines written in ITK. 
 *
 * @author Sokratis Makrogiannis (smakrogiannis@desu.edu,makrogianniss@mail.nih.gov)
 * @see    JNI
 */

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;

// import ij.plugin.PlugIn;
// import java.util.*;


class PQCTAnalysisJNI {

    // Native method declarations.
    native void execute(String inputImage, short workflowID, String parameterFile, String outputPath);

	// Load library.
	static {
		try{
			// Add libs to java.library.path.
			String currentDirectory = System.getProperty("user.home");
			String nativeLibraryDirectory = currentDirectory + 
				File.separator + "mipav" +
				File.separator + "plugins" + 
				File.separator + "libs";
//			System.out.println(System.getProperty("java.library.path"));
			PQCTAnalysisJNI.addLibraryPath(nativeLibraryDirectory);
//			System.out.println("Native lib dir: " + nativeLibraryDirectory);
//			System.out.println(System.getProperty("java.library.path"));
			// Load libraries depending on the operating system.
			String operatingSystem = System.getProperty("os.name");
			System.out.println(operatingSystem);
			if (operatingSystem.startsWith("Win"))
				System.loadLibrary("ITKCommon-4.1");
			System.loadLibrary("PQCT_Analysis");
		}
		catch(UnsatisfiedLinkError e) {
			System.err.println("Native code library failed to load.\n" + e);
		} 
		catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}


	/**
	* Adds the specified path to the java library path
	*
	* @param pathToAdd the path to add
	* @throws IOException
	*/
	public static void addLibraryPath(String pathToAdd) throws IOException {
		try {
			// This enables the java.library.path to be modified at runtime
			// From a Sun engineer at http://forums.sun.com/thread.jspa?threadID=707176
			//
			Field field = ClassLoader.class.getDeclaredField("usr_paths");
			field.setAccessible(true);
			String[] paths = (String[])field.get(null);
			for (int i = 0; i < paths.length; i++) {
				if (pathToAdd.equals(paths[i])) {
					return;
				}
			}
			System.setProperty("java.library.path", System.getProperty("java.library.path") + File.pathSeparator + pathToAdd);
			String[] tmp = new String[paths.length+1];
			System.arraycopy(paths,0,tmp,0,paths.length);
			tmp[paths.length] = pathToAdd;
			field.set(null,tmp);
			// Alternate method with the same purpose.
//			System.setProperty("java.library.path", System.getProperty("java.library.path") + File.pathSeparator + pathToAdd);
//			Field field = ClassLoader.class.getDeclaredField("sys_paths");
//			field.setAccessible(true);
//			field.set(null,null);
		} catch (IllegalAccessException e) {
			throw new IOException("Failed to get permissions to set library path");
		} catch (NoSuchFieldException e) {
			throw new IOException("Failed to get field handle to set library path");
		}
	}
	
	
	// Main function.
	public static void main(String args[]) {


		// Instantiate class.
		PQCTAnalysisJNI  pqctAnalyzer = new PQCTAnalysisJNI();

		// Default arguments that will be over-riden by command-line input.
		String pqctimageFilename = "AAA.I03";
		short workflowID = 0;
		String currentDirectory = System.getProperty("user.dir");
		String nativeLibraryDirectory = currentDirectory + File.separator + "plugins" + File.separator + "libs";
		String parameterFilename = nativeLibraryDirectory + File.separator + "PQCT_Analysis_Params.txt";
		String outputPath = currentDirectory + File.separator;
		
		// Parse arguments and assign them to variables.
		if (args.length < 3) {
			System.out.println("Usage: java <input image> <workflow number (0: 4% tibia, 1: 38% tibia, 2: 66% tibia)>" +
					"<parameter filename>");
			System.exit(1);
		}
		pqctimageFilename = args[0];
		workflowID =  Short.parseShort(args[1]);
		parameterFilename = args[2];

		// Call native method.
		try{
			pqctAnalyzer.execute( pqctimageFilename, workflowID, parameterFilename, outputPath);
		} catch(Exception e) {
			System.err.println("JNI error: "+e.getMessage());
		}

	}

}
