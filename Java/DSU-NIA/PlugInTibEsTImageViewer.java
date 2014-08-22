/**
 * PQCT reader. 
 *
 * @author Sokratis Makrogiannis (makrogianniss@mail.nih.gov)
 * @see    ViewJFrameImage
 */

//import java.awt.*;
import gov.nih.mipav.model.structures.ModelImage;
import gov.nih.mipav.view.ViewJFrameImage;
import gov.nih.mipav.view.dialogs.JDialogWinLevel;

import java.awt.event.AdjustmentEvent;
import java.awt.event.AdjustmentListener;
import java.awt.event.MouseListener;

import javax.swing.JFrame;
import javax.swing.event.ChangeListener;


public class PlugInTibEsTImageViewer extends ViewJFrameImage implements MouseListener, AdjustmentListener, ChangeListener
{
	private static final long serialVersionUID = 1L;
	
//	private ModelImage segmentedImage;
//	private ModelImage originalImage;
//	private float original[];
//	
//	private ViewJFrameImage imageFrame = null;

	public PlugInTibEsTImageViewer(ModelImage imageA) {
		super(imageA);
		// TODO Auto-generated constructor stub
		// this.originalImage = imageA;
		this.initProperties();
	}
	
	public PlugInTibEsTImageViewer(ModelImage imageA, ModelImage imageB) {
		super(imageA);
		// this.segmentedImage = imageB;
		this.setImageB(imageB);
		this.initProperties();
	}
	
	@Override
	public void adjustmentValueChanged(AdjustmentEvent arg0) {
		// TODO Auto-generated method stub
		updateImages(true);
	}
	
	public PlugInTibEsTImageViewer openFrame(ModelImage image) {
		return new PlugInTibEsTImageViewer(image);
	}
	
	// Initialize properties.
	void initProperties() throws OutOfMemoryError{
		setResizable(true);

//		int dimExtentsLUT[] = new int[2];
//		dimExtentsLUT[0] = 4;
//		dimExtentsLUT[1] = 256;
//		setLUTa(new ModelLUT(ModelLUT.GRAY, 4096, dimExtentsLUT));		

		this.LUTa = initLUT(imageA);

		initResolutions();
		initZoom();
		int[] extents = createBuffers();
		initComponentImage(extents);
		initExtentsVariables(imageA);
		
		// MUST register frame to image models
		imageA.addImageDisplayListener(this);
		if (imageB != null)
		{
		    imageB.addImageDisplayListener(this);
		}
	
		windowLevel = new JDialogWinLevel[2];
	
		this.setLocation(100, 50);
	
		setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);		
		
		userInterface.registerFrame(this);
		this.updateImages(true);
		addComponentListener(this);
		setVisible(true);
	}

}
