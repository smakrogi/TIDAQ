/** Lower extremities fat quantification using PQCT data.
 @author: Sokratis Makrogiannis, 3T @ NIA
 */

// import gov.nih.mipav.plugins.PlugIn;
import ij.IJ;
import ij.ImagePlus;
import ij.io.FileInfo;
import ij.io.FileOpener;
import ij.io.OpenDialog;
import ij.plugin.PlugIn;
import ij.process.ImageProcessor;

import java.awt.Rectangle;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * PQCT reader. 
 *
 * @author Sokratis Makrogiannis (makrogianniss@mail.nih.gov)
 * @see    PlugIn
 */

public class Read_PQCT implements PlugIn{

	ImagePlus img;
	ImageProcessor ip;
	static float slope = 1724.0f, intercept = -322.0f;

	public void run(String arg){

		// 1. Read image header.
		OpenDialog fileopenDialog = new OpenDialog("Select PQCT image", null);
		String filename = fileopenDialog.getFileName();
		String directory = fileopenDialog.getDirectory();
		String fullPathFilename = directory + filename;
		if( filename == null ) return;
		IJ.showStatus("Opening: " + fullPathFilename);
		// IJ.showMessage("My_Plugin", fullPathFilename);

		try {
			this.readPQCTImage(fullPathFilename);
		}
		catch (java.io.FileNotFoundException e) {
			IJ.error("Cannot read file.", e.getMessage());
			return;
		}
		catch(java.io.IOException e) {
			IJ.error("Cannot read file.", e.getMessage());
			return;
		}

		IJ.showStatus("Image Reading...done");

		this.calibrateImage(this.img, this.img.getProcessor());
		IJ.showStatus("Image Calibration...done");
		img.show();

		String argumentString = "get=[" + fullPathFilename + "]";
		//  IJ.run("Creation Date", "get=[C:\\Documents and Settings\\makrogianniss\\My Documents\\Data\\Sardinia\\raw.092410\\I0036038.M01]");
		IJ.run("Creation Date", argumentString);

	}


	public void readPQCTImage(String fullPathFilename) throws java.io.FileNotFoundException,IOException{

		FileInputStream input = new FileInputStream(fullPathFilename);
		if(input == null) throw new FileNotFoundException();
		int number = 0;
		float count = 0F;
		long headerLength = 1609L;

		input.skip(headerLength);
		do{
			number = input.read();
			// IJ.log( number+" \n" );
			count++;
		}
		while(number != -1);
		input.close();
		IJ.log( "Elements "+count );
		int imageSide = (int) Math.sqrt((double)count/2);
		// imageSide = 283;
		FileInfo imageInfo = new FileInfo();
		imageInfo.fileFormat = FileInfo.RAW;
		imageInfo.fileName = fullPathFilename;
		imageInfo.directory = "";
		imageInfo.fileType = FileInfo.GRAY16_SIGNED;
		imageInfo.intelByteOrder = true;
		imageInfo.offset = 1609; 
		imageInfo.width = imageSide;
		imageInfo.height = imageSide;
		imageInfo.nImages = 1;
		imageInfo.pixelHeight = 0.8;
		imageInfo.pixelWidth = 0.8;
		IJ.log(imageInfo.toString());
		// new FileOpener(imageInfo).open();

		FileOpener fo = new FileOpener(imageInfo);
		img = fo.open(true);
		if (img!=null) img.unlock();

		// ImageReader readImage = new ImageReader(imageInfo);
		// input = new FileInputStream(fullPathFilename);  

		// ip = NewImage.createShortImage("test", imageSide, imageSide, 1, NewImage.FILL_BLACK);
		// ip = readImage.readPixels(input, headerLength);

	}

	public void calibrateImage(ImagePlus img, ImageProcessor ip) {
		// ImageProcessor ip = img.getProcessor();
		Rectangle r = ip.getRoi();
		for (int y=r.y; y<(r.y+r.height); y++)
			for (int x=r.x; x<(r.x+r.width); x++) {
				float originalValue = (float)ip.get(x,y) - 32768.0F;
				int calibratedValue = (int)Math.round((slope * ((float)originalValue/1000.0F)) + intercept);
				// IJ.log("vals: " + originalValue + " " + calibratedValue);
				short shortCalibratedValue = (short) (calibratedValue - 32768);
				ip.set(x, y, (short)shortCalibratedValue);
			}
	}

}
