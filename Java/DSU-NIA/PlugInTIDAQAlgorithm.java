/**
 * This class implements a basic algorithm that performs operations on 2D and 3D images. 
 * By extending AlgorithmBase, it has no more functionality than any other algorithm in MIPAV.
 * No functionality specifically makes it a plug-in.
 * 
 * @author Sokratis Makrogiannis (makrogianniss@mail.nih.gov)
 * @see http://mipav.cit.nih.gov
 */

// import java.awt.Rectangle;
import gov.nih.mipav.model.algorithms.AlgorithmBase;
import gov.nih.mipav.model.structures.ModelImage;

public class PlugInTIDAQAlgorithm extends AlgorithmBase {

  	/** X dimension of the image */
    private int xDim;

    /** Y dimension of the image */
    private int yDim;

    /** Slice size for xDim*yDim */
    private int sliceSize;

    /**
     * Constructor.
     *
     * @param  resultImage  Result image model
     * @param  srcImg       Source image model.
     */
    public PlugInTIDAQAlgorithm(ModelImage resultImage, ModelImage srcImg) {
        super(resultImage, srcImg);
        init();
    }
    
	//  ~ Methods --------------------------------------------------------------------------------------------------------
	
	/**
	 * Prepares this class for destruction.
	 */
	public void finalize() {
	    destImage = null;
	    srcImage = null;
	    super.finalize();
	}

	/**
     * Starts the algorithm.  At the conclusion of this method, AlgorithmBase reports to any
     * algorithm listeners that this algorithm has completed.  This method is not usually called explicitly by
     * a controlling dialog.  Instead, see AlgorithmBase.run() or start().
     */
    public void runAlgorithm() {
    	this.calc2D();
    	setCompleted(true); //indicating to listeners that the algorithm completed successfully
    	
    } // end runAlgorithm()
    
//  ~ Methods --------------------------------------------------------------------------------------------------------

    private void calc2D() {
    	fireProgressStateChanged("Message 2D: "+srcImage.getImageName());
    	
    	for(int i=1; i<100; i++) {
    		fireProgressStateChanged(i);
    	}
    }
    

	private void init() {
		xDim = srcImage.getExtents()[0];
        yDim = srcImage.getExtents()[1];
        sliceSize = xDim * yDim;
	}


	// Run the complete quantification pipeline.
	public void quantificationPipeline(String newImageFilename) {
		// Call pipeline using JNI.
	}

	
}
