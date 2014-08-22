/**
 * This class displays a basic dialog for a MIPAV plug-in. 
 * @author Sokratis Makrogiannis, makrogianniss@mail.nih.gov
 * @version  July 31, 2012
 * @see      
 * @see      JDialogStandalonePlugin
 *
 * @author Sokratis Makrogiannis (makrogianniss@mail.nih.gov)
 */

import gov.nih.mipav.model.file.FileIO;
import gov.nih.mipav.model.file.FileInfoNIFTI;
// import gov.nih.mipav.model.file.FileInfoDicom;
// import gov.nih.mipav.model.file.FileDicom;
import gov.nih.mipav.model.file.FileNIFTI;
import gov.nih.mipav.model.structures.ModelImage;
// import gov.nih.mipav.model.structures.ModelStorageBase;
import gov.nih.mipav.model.structures.ModelLUT;
import gov.nih.mipav.plugins.JDialogStandalonePlugin;
import gov.nih.mipav.view.ViewJFrameImage;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dialog;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Frame;
import java.awt.GradientPaint;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.GridLayout;
import java.awt.Image;
import java.awt.event.ActionEvent;
import java.awt.event.WindowEvent;
import java.awt.geom.Rectangle2D;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.ArrayList;
import java.util.Locale;
import java.util.Scanner;

import javax.imageio.ImageIO;
import javax.swing.BoxLayout;
import javax.swing.DefaultListModel;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.ListSelectionModel;
import javax.swing.ScrollPaneConstants;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.filechooser.FileNameExtensionFilter;
import javax.swing.SwingWorker;

@SuppressWarnings("serial")
public class PlugInTibEsTDialog extends JDialogStandalonePlugin{

	// Static variables used in GUI and algorithms.
	public static final String[] TIBIA_LEVEL = {"4pct", "38pct", "66pct", "MidThigh"};
	public static final Color bgColor = new Color(204, 207, 239); // Color.white;
	public static final Color fontColor = Color.black;
	public static final Color bgColor2 = new Color(204, 239, 208);
	public static final String originalImageSuffix = ".Input.nii";
	public static final String labelImageSuffix = ".Labels.nii";
	public final String defaultOutputPath = System.getProperty("user.home") + 
		File.separator + 
		"mipav" + 
		File.separator + 
		"plugins" + 
		File.separator + 
		"results" + 
		File.separator; 
	
	// Fields of the class.
	int previousID;
	ArrayList<String> subjectList;
	String dataDirectory, filename, statusText, commandLabel;
	static Frame instance;
	DefaultListModel subjectJListModel;	
	JList subjectJList;  // MutableList 
	JTextArea quantificationJTextArea, subjectlistpathTextArea, 
		imagepathTextArea, parameterfilenamepathTextArea,
		outputpathTextArea;
	JTextField nClustersTextField;
	JComboBox anatomicalLocation;
	JScrollPane scrollPane;
	JPanel operationPanel, parameterPanel, subjectlistPanel, 
	quantificationPanel, imagePanel;
	JCheckBox toggleButton = null;
	JLabel statusLabel = null;
	LogoPanel logoPanel;
	ViewJFrameImage inputImageViewer=null, labelImageViewer=null;
	ModelImage originalImage=null, labelImage=null;
	Task task;
	
    /** This is your algorithm */
    // PlugInPQCTAnalysisAlgorithm genericAlgo = null;
    
    public PlugInTibEsTDialog() {
    	super();
    	javax.swing.SwingUtilities.invokeLater(new Runnable() {
    		public void run() { 
    			init();
    		}
    	});
    }

    public PlugInTibEsTDialog(boolean modal) {
    	super(modal);
    	javax.swing.SwingUtilities.invokeLater(new Runnable() {
    		public void run() { 
    			init();
    		}
    	});
    }
    
    public PlugInTibEsTDialog(Dialog parent, boolean modal) {
    	super(parent, modal);
    	javax.swing.SwingUtilities.invokeLater(new Runnable() {
    		public void run() { 
    			init();
    		}
    	});
    }
  
    
    public PlugInTibEsTDialog(Frame parentFrame, boolean modal) {
    	super(parentFrame, modal);
    	javax.swing.SwingUtilities.invokeLater(new Runnable() {
    		public void run() { 
    			init();
    		}
    	});
    }

    void init() {
    	// Basic properties of GUI.
        setTitle("Leg Composition Assessment from (pQ)CT - NIA, NIH (by Sokratis Makrogiannis, PhD)");
    	setForeground(Color.blue);
//        try {
//			setIconImage(MipavUtil.getIconImage("divinci.gif"));
//		} catch (FileNotFoundException e) {
//			Preferences.debug("Failed to load default icon", Preferences.DEBUG_MINOR);
//		}

		// Set panels and components.
		// setLayout(new FlowLayout());
		setLayout(new BorderLayout());

		// Logo panel.
		this.addLogoPanel();
		
		// Add control panel.
		this.addOperationPanel();
		
		// Add parameter panel.
		this.addParameterPanel();
		
		// Add list of images.
		this.addSubjectlistPanel();
		
		// Add quantification panel.
		this.addQuantificationPanel();
		
		// Geometry.
        pack();
        setSize(1200, 700);
        setVisible(true);
        setResizable(true);
		System.gc();
	}
    
	

// Function for adding buttons.
void addButton(String label, JPanel currentPanel) {
	JButton b = new JButton(label);
	b.setFont(serif12B);
	b.setForeground(fontColor);
	b.setBackground(bgColor);
	b.addActionListener(this);
	currentPanel.add(b);
}

//Function for adding buttons.
void addToggleButton(String label, JPanel currentPanel) {
	this.toggleButton = new JCheckBox(label);
	this.toggleButton.setFont(serif12B);
	this.toggleButton.setForeground(fontColor);
	this.toggleButton.setBackground(bgColor);
	this.toggleButton.setBorderPainted(true);
	this.toggleButton.addActionListener(this);
	currentPanel.add(toggleButton);
}

void addOperationPanel() {
	// Add control panel.
	this.operationPanel = new JPanel();
	this.operationPanel.setLayout(new GridLayout(5, 1));
	this.operationPanel.setForeground(fontColor);
	this.operationPanel.setBackground(bgColor);
	this.addButton("Read Info File", this.operationPanel);
	this.addButton("Select Image List", this.operationPanel);
	this.addButton("Quantify Selected", this.operationPanel);
	this.addButton("Quantify All", this.operationPanel);
	this.addToggleButton("Image Viewer", this.operationPanel);
	this.getContentPane().add(this.operationPanel, BorderLayout.WEST);
	
}

// Method for adding parameters.
void addParameterPanel() {
	// Construct panel and define layout and background.
	this.parameterPanel = new JPanel();
	this.parameterPanel.setLayout(new GridLayout(5,2));
//	this.parameterPanel.setForeground(fontColor);
//	this.parameterPanel.setBackground( bgColor );
	// Add label and text field of the anatomical location.
	this.parameterPanel.add(new JLabel("Choose anatomical location"));
	String[] anatomicalLocationStrings =
	{"pQCT 4% Tibia","pQCT 38% Tibia","pQCT 66% Tibia", "CT Middle Thigh","pQCT ANONYMIZATION"};
	this.anatomicalLocation = new JComboBox(anatomicalLocationStrings);
	this.anatomicalLocation.setSelectedIndex(2);
//	anatomicalLocation.setForeground(fontColor);
//	anatomicalLocation.setBackground(bgColor);
	this.parameterPanel.add(this.anatomicalLocation);

	// Add label and text field of cluster # parameter.
//	this.parameterPanel.add( new JLabel("Cluster #"));
//	this.nClustersTextField = new JTextField("4");
//	this.nClustersTextField.addActionListener(this);
//	this.parameterPanel.add(this.nClustersTextField);
	
	// Add subject list path title.
	this.parameterPanel.add(new JLabel("Subject List File Path"));
	// Add subject list path path.
	this.subjectlistpathTextArea = new JTextArea("", 3, 20);
	this.subjectlistpathTextArea.setBackground(bgColor);
	this.subjectlistpathTextArea.setEditable(false);
	this.subjectlistpathTextArea.setLineWrap(true);
	this.parameterPanel.add(this.subjectlistpathTextArea);

	// Add image path title.
	this.parameterPanel.add(new JLabel("Image Path"));
	// Add subject list path path.
	this.imagepathTextArea = new JTextArea("", 3, 20);
	this.imagepathTextArea.setBackground(bgColor);
	this.imagepathTextArea.setEditable(false);
	this.imagepathTextArea.setLineWrap(true);
	this.parameterPanel.add(this.imagepathTextArea);
	
	// Button for selection of parameter filename and default value.
	this.addButton("Parameter Filename", this.parameterPanel);
	this.parameterfilenamepathTextArea = new JTextArea("", 3, 20);
	this.parameterfilenamepathTextArea.setBackground(bgColor);
	this.parameterfilenamepathTextArea.setEditable(false);
	this.parameterfilenamepathTextArea.setLineWrap(true);
	String currentDirectory = System.getProperty("user.home");
	String nativeLibraryDirectory = currentDirectory +
	File.separator + "mipav" +
	File.separator + "plugins" +  
	File.separator + "libs";
	String parameterFilename = nativeLibraryDirectory + File.separator + "PQCT_Analysis_Params.txt";
	this.parameterfilenamepathTextArea.setText(parameterFilename);
	this.parameterPanel.add(this.parameterfilenamepathTextArea);
	
	// Button for selection of output path and default value.
	this.addButton("Output Path", this.parameterPanel);
	this.outputpathTextArea = new JTextArea("", 3, 20);
	this.outputpathTextArea.setBackground(bgColor);
	this.outputpathTextArea.setEditable(false);
	this.outputpathTextArea.setLineWrap(true);
	this.outputpathTextArea.setText(this.defaultOutputPath);
	this.parameterPanel.add(this.outputpathTextArea);
		
	this.getContentPane().add(this.parameterPanel, BorderLayout.CENTER);
}


// Panel with subject list.
void addSubjectlistPanel() {
	// Subjectlist panel.
	this.subjectlistPanel = new JPanel();
	this.subjectlistPanel.setLayout(new BoxLayout(this.subjectlistPanel, BoxLayout.Y_AXIS));
	this.subjectlistPanel.setFont(new Font("Times New Roman", Font.BOLD, 14));
	// Add subject list title.
	this.subjectlistPanel.add(new JLabel("Subject List Images"));
	this.subjectlistPanel.setForeground(fontColor);
	this.subjectlistPanel.setBackground( bgColor );
	// Instantiate array list.
	this.subjectList = new ArrayList<String>();
	// List component.
	this.subjectJListModel = new DefaultListModel();
	this.subjectJList = new JList(this.subjectJListModel); // MutableList();
	this.subjectJList.setVisibleRowCount(30);
	this.subjectJList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
	this.subjectJList.setFont(new Font("Times New Roman", Font.BOLD, 12));
	this.subjectJList.addListSelectionListener(new ListListener());
	// Scrollpane component.
	this.scrollPane = new JScrollPane(subjectJList);
	this.scrollPane.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED);
	this.scrollPane.setHorizontalScrollBarPolicy(ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);
	this.subjectlistPanel.add(this.scrollPane);
	this.getContentPane().add(this.subjectlistPanel, BorderLayout.EAST);
}

void addLogoPanel() {
	// Create panel.
	this.logoPanel = new LogoPanel();
	// Set layout to specific order.
	this.logoPanel.setLayout(new BoxLayout(this.logoPanel, BoxLayout.Y_AXIS));
	// Add logo script.
	JLabel logoScript = new JLabel("<HTML>TibEsT+ (Tibia Estimation Tool) v.1.4 <BR> CRB/NIA/NIH</HTML> <BR>");
	logoScript.setFont(new Font("Serif", Font.ITALIC + Font.BOLD, 24));
	logoScript.setHorizontalAlignment(JLabel.CENTER);
	this.logoPanel.add(logoScript);
	// Add status label.
	this.statusLabel =  new JLabel("<HTML>Status: OK</HTML>");
	this.statusLabel.setFont(new Font("Courier", Font.ITALIC + Font.BOLD, 16));
	this.statusLabel.setHorizontalAlignment(JLabel.CENTER);
	this.logoPanel.add(this.statusLabel);
	// Add panel to frame content pane.
	this.getContentPane().add(this.logoPanel, BorderLayout.NORTH);
}

// Nested class for painting the logo area.
class LogoPanel extends JPanel {
	Image logoImage = null;
//	java.net.URL imageURL = 
//		PlugInPQCTAnalysisMIPAV.class.getResource("images/NIA_logo.png");
	String currentDirectory = System.getProperty("user.home");
	String imagePath = currentDirectory +
		File.separator + "mipav" +	
		File.separator + "plugins" + 
		File.separator + "images" + 
		File.separator + "NIA_logo.png";

	
	public void paintComponent(Graphics g) {

		// Type casting for 2d functionality.
		Graphics2D g2d = (Graphics2D) g;
		
		// A few design experiments.
		Rectangle2D rectangle =
	        new Rectangle2D.Double(0.0, 0.0, this.getWidth(), this.getHeight());
		GradientPaint backgroundPaint = 
			new GradientPaint(0, 0, bgColor, 
					this.getWidth(), this.getHeight(), bgColor2);
		g2d.setPaint(backgroundPaint);
		g2d.fill(rectangle);
		
		// Add logo image.
		try {
//			logoImage = ImageIO.read(new File(imageURL.getPath()));
			logoImage = ImageIO.read(new File(imagePath));
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		g2d.drawImage(logoImage, 2, 2, 128, 128, this);
	}

    public Dimension getPreferredSize() {
        if (logoImage == null) {
             return new Dimension(700, 32);
        } else {
           return new Dimension(128, 128);
       }
    }
	
}

//Panel with subject list.
void updateSubjectlistPanel() {
//	DefaultListModel listModel = (DefaultListModel)this.subjectJList.getModel();
//	listModel.clear();  
	this.subjectJListModel.clear();
	for(int i=0; i<this.subjectList.size(); i++)  
	{  
		this.subjectJListModel.addElement((String)this.subjectList.get(i));  
	}
	this.subjectJList.setSelectedIndex(0);
	this.subjectlistPanel.repaint();
}


// Implement listener for events related to selection of subjects.
class ListListener implements ListSelectionListener {
	
	public void valueChanged(ListSelectionEvent lse){
		if(!lse.getValueIsAdjusting()) {
			if( toggleButton.isSelected() ) {
				loadOriginalImage();
				try {
					Thread.sleep(50);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				displayOriginalImage();
				loadLabelImage();				
				try {
					Thread.sleep(50);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				displayLabelImage();
				inputImageViewer.updateImages();
			}
		}
	}
}

// Text area with quantification results.
void addQuantificationPanel() {
	//	Read text file or capture stream.
	this.quantificationPanel = new JPanel();
	this.quantificationJTextArea = new JTextArea("",15,140);
	this.quantificationJTextArea.setBackground(bgColor);
	this.quantificationJTextArea.setEditable(false);
	this.quantificationJTextArea.setLineWrap(false);
	this.quantificationJTextArea.setFont(new Font("Courier", 
			Font.BOLD, 12));
	JScrollPane scrollPane = new JScrollPane(this.quantificationJTextArea);
	scrollPane.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED);
	scrollPane.setHorizontalScrollBarPolicy(ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
	JPanel leftPanel = new JPanel();
	leftPanel.add(scrollPane);
	JPanel rightPanel = new JPanel();
	rightPanel.setLayout(new BoxLayout(rightPanel, BoxLayout.Y_AXIS));
//	this.quantificationPanel.add(scrollPane);
	this.addButton("Clear Results", rightPanel);
	this.addButton("Save Results", rightPanel);
	this.quantificationPanel.add(leftPanel);
	this.quantificationPanel.add(rightPanel);
	this.getContentPane().add(this.quantificationPanel, BorderLayout.SOUTH);
}

// Display the original image.
void loadOriginalImage() {
	// Form filename.
	String selection = (String) this.subjectJList.getSelectedValue();
	File selectionFile = new File(selection);
	selection = selectionFile.getName();	
	short workflowID = (short) this.anatomicalLocation.getSelectedIndex();

	// Read input image (different formats).
	String  filename;
	if( workflowID == 3) {
		String filePrefix = "";
		int dotPosition = selection.lastIndexOf('.');
		if (dotPosition != -1)
			filePrefix = selection.substring(0, dotPosition);
		else
			filePrefix = selection;
		filename = filePrefix + originalImageSuffix;
	}
	else {
		filename = selection + originalImageSuffix;
	}

	// Read original image file.
	System.out.println("Reading " + this.outputpathTextArea.getText() + filename);
	try {
		FileInfoNIFTI fileInfo = new FileInfoNIFTI(filename, this.outputpathTextArea.getText(), 1);
		FileIO inputFile = new FileIO();
		this.originalImage = inputFile.readImage(filename, this.outputpathTextArea.getText(), false, fileInfo);
		// FileInfoDicom fileInfo = new FileInfoDicom(filename, this.outputpathTextArea.getText(), 1);
		// FileDicom inputFile = new FileDicom(filename, this.outputpathTextArea.getText());
		// short[] buffer = null;
		// inputFile.readImage(buffer, ModelImage.SHORT, 0);
	} catch (OutOfMemoryError e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}

}

void displayOriginalImage() {
	// Display original image.
	if (this.inputImageViewer.isShowing()) {
		this.inputImageViewer.getComponentImage().setImageA(this.originalImage);
		this.originalImage.addImageDisplayListener(inputImageViewer);
		ModelLUT newLUTa = ViewJFrameImage.initLUT(this.originalImage);
		this.inputImageViewer.initResolutions();
		this.inputImageViewer.initZoom();
		this.inputImageViewer.initExtentsVariables(this.originalImage);
		this.inputImageViewer.setLUTa(newLUTa);
		this.inputImageViewer.setTitle(this.originalImage.getImageName());
		this.originalImage.notifyImageDisplayListeners();
		this.inputImageViewer.updateImages(true);
	}
}

// Display segmentation output.
void loadLabelImage(){
	// Form filename.
	String selection = (String) this.subjectJList.getSelectedValue();
	File selectionFile = new File(selection);
	selection = selectionFile.getName();	
	short workflowID = (short) this.anatomicalLocation.getSelectedIndex();
	String  filename;
	if( workflowID == 3) {
		String filePrefix = "";
		int dotPosition = selection.lastIndexOf('.');
		if (dotPosition != -1)
			filePrefix = selection.substring(0, dotPosition);
		else
			filePrefix = selection;
		filename = filePrefix + "_"	+ 
		TIBIA_LEVEL[workflowID] +
		labelImageSuffix;
	}
	else {
		filename = selection + "_" + 
		TIBIA_LEVEL[workflowID] + 
		labelImageSuffix;
	}

	// Read label image.
	System.out.println("Reading " + this.outputpathTextArea.getText() + filename);
	FileNIFTI inputFile = new FileNIFTI(filename, this.outputpathTextArea.getText());
	try {
		this.labelImage = inputFile.readImage(true, false);
	} catch (OutOfMemoryError e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	} catch (IOException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}
}

void displayLabelImage(){
	// Display label image..
	if (this.inputImageViewer.isShowing()) {
		int dimExtentsLUT[] = new int[2];
		dimExtentsLUT[0] = 4;
		dimExtentsLUT[1] = 256;
		ModelLUT labelModelLUT = new ModelLUT(ModelLUT.STRIPED, 32, dimExtentsLUT);
		this.labelImage.addImageDisplayListener(inputImageViewer);
		this.inputImageViewer.getComponentImage().setImageB(this.labelImage);
		this.inputImageViewer.setLUTb(labelModelLUT);
		// this.inputImageViewer.setAlphaBlend(25);
		this.labelImage.notifyImageDisplayListeners();
		this.inputImageViewer.updateImages(true);
	}
}

public void processWindowEvent(WindowEvent e) {
	super.processWindowEvent(e);
	if (e.getID()==WindowEvent.WINDOW_CLOSING) {
		instance = null;
	}
}

class Task extends SwingWorker<Void, Void> {
	
	@Override
	public Void doInBackground() {
		if (commandLabel==null)
			return null;
		//Run with exception handling.
		try {
			statusLabel.setText("<HTML> Status: Executing Command..." + commandLabel + " </HTML>");
			runCommand(commandLabel);
			statusLabel.setText("<HTML> Status: Completed " + commandLabel + " </HTML>");
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		return null;
	}
	
	@Override
	public void done() {
		// Toolkit.getDefaultToolkit().beep();
		setCursor(null); // turn off the wait cursor
	}
}
//! Handle events coming from buttons and other components.
public void actionPerformed(ActionEvent e) {
	
	// Retrieve the action command.
	this.commandLabel = e.getActionCommand();

	// Execute processing in background.
	task = new Task();
    task.execute();

	this.repaint();
}

//! Format double in specific format.
String formatString(double inValue) {
	String shortString = "";
	DecimalFormat threeDec = new DecimalFormat("0.000", new	DecimalFormatSymbols(Locale.US));
	shortString = (threeDec.format(inValue));
	return shortString;	
}

//! Run commands given by the user though the GUI.
void runCommand(String command) throws IOException {
	if (command.equals("Read Info File")) {
		//! Disable viewer to avoid annoying iofile exception messages. 
		this.toggleButton.setSelected( false );
		//! Read subject list.
		this.readSubjectListFile();
		System.out.println(this.subjectList.toString());
		//! Update JList.
		this.updateSubjectlistPanel();
	}
	if (command.equals(	"Select Image List")) {
		//! Disable viewer to avoid annoying iofile exception messages. 
		this.toggleButton.setSelected( false );
		//! Select subject list.
		this.selectSubjectList();
		System.out.println(this.subjectList.toString());
		//! Update JList.
		this.updateSubjectlistPanel();
	}
	else if (command.equals("Quantify Selected")) {
		//! Call quantification library for the selected entry.
		//! Disable viewer to avoid annoying iofile exception messages. 
		if(this.toggleButton.isSelected()) {
			this.toggleButton.setSelected( false );
			this.stopImageViewer();
		}
		// Quantify selected subject.
		this.callQuantifier();
	}
	else if (command.equals("Quantify All")) {
		//! Read each line of list from 
		//! selected subject downwards and quantify.
		//! Disable viewer to avoid annoying iofile exception messages. 
		if(this.toggleButton.isSelected()) {
			this.toggleButton.setSelected( false );
			this.stopImageViewer();
		}
		//! Quantify all subjects from selected index to the end of list.
		int initialIndex = this.subjectJList.getSelectedIndex();
		for(int i=initialIndex;
		i<(this.subjectList.size());
		i++) {
			double percent = 100 * ( (i - initialIndex + 1) / (double) (this.subjectList.size() - initialIndex) );
			String percentString = formatString(percent);
			// System.out.println(percentString);
			String infoString = "<HTML> Status: Analyzing Number " + i + " Entry " + "(" + percentString + " %) </HTML>";
			this.statusLabel.setText(infoString);
			this.subjectJList.setSelectedIndex(i);
			this.callQuantifier();
		}
	}
	else if (command.equals("Image Viewer")) {
		//! If image viewer current state is ON, 
		//! display input and output images.
		if( toggleButton.isSelected() ) {	
			this.startImageViewer();
		}
		else{
			this.stopImageViewer();
		}
	}
	else if (command.equals("Parameter Filename")) {
		//! Set parameter filename other than the default value.
		this.setParameterFilename();
	}
	else if (command.equals("Output Path")) {
		//! Let user select output path.
		this.setOutputPath();
	}
	else if (command.equals("Clear Results")) {
		//! Clear results text area.
		this.quantificationJTextArea.setText("");
	}
	else if (command.equals("Save Results")) {
		//! Save results text to file.
		this.saveQuantificationResults();
	}

}


//! Start image viewer frame.
void startImageViewer() {
	this.loadOriginalImage();
	try {
		Thread.sleep(50);
	} catch (InterruptedException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}
	this.inputImageViewer = new ViewJFrameImage( (ModelImage)this.originalImage.clone());
	// this.inputImageViewer.setLocation(100, 250);
	this.displayOriginalImage();
	this.loadLabelImage();
	try {
		Thread.sleep(50);
	} catch (InterruptedException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}
	this.displayLabelImage();
	this.inputImageViewer.updateImages();
}


//! Stop image viewer and close frame.
void stopImageViewer() {
	this.inputImageViewer.close();
	try {
		this.inputImageViewer.finalize();
	} catch (Throwable e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}
	this.inputImageViewer = null;
}


//! Method for reading subject lists.
void readSubjectListFile() throws IOException {
	//! Choose subject list text file.
    JFileChooser chooser = new JFileChooser();
    chooser.setDialogTitle("Select Subject List File");
    // Note: source for ExampleFileFilter can be found in FileChooserDemo,
    // under the demo/jfc directory in the Java 2 SDK, Standard Edition.
    FileNameExtensionFilter filter = 
    	new FileNameExtensionFilter("Text file", "txt");
    chooser.setFileFilter(filter);
    int returnVal = chooser.showOpenDialog(getParent());
    if(returnVal == JFileChooser.APPROVE_OPTION) {
       System.out.println("You chose to open this file: " +
            chooser.getSelectedFile().getCanonicalPath());
    }
    else
    	return;
    String fullPathFilename = chooser.getSelectedFile().getCanonicalPath();
    
    //! Display subject list path.
    this.subjectlistpathTextArea.setText(fullPathFilename);
    
    //! Read subject list file.
	File file = new File(fullPathFilename);
	Scanner scanner = null;
	try {
		// Read data path.
		// Instantiate array list.
//		this.subjectList = new ArrayList<String>();
		this.subjectList.clear();

		scanner = new Scanner(file).useDelimiter("\n");
		this.dataDirectory = scanner.next().toString();
		System.out.println("Data directory is: " +
				this.dataDirectory);
		// IJ.log(dataPath);
		while(scanner.hasNext())
		{
			// Read image.
			// String newImageFilename = this.dataPath + scanner.next();
			this.subjectList.add(scanner.next().toString());
		}
			scanner.close();
		}
		catch(java.io.IOException e) {
			// IJ.error("Cannot read file.", e.getMessage());
			e.printStackTrace();
		}
		//! Display image path.
		this.imagepathTextArea.setText(this.dataDirectory);
}


//! Selecting subjects to be analyzed.
void selectSubjectList() throws IOException {
	//! Choose subject list text file.
    JFileChooser chooser = new JFileChooser();
    chooser.setDialogTitle("Select Images for Analysis");
    chooser.setMultiSelectionEnabled(true);
    
    // Note: source for ExampleFileFilter can be found in FileChooserDemo,
    // under the demo/jfc directory in the Java 2 SDK, Standard Edition.
    int returnVal = chooser.showOpenDialog(getParent());
    if(returnVal == JFileChooser.APPROVE_OPTION) {
       System.out.println("You chose to open this file: " +
            chooser.getSelectedFiles().toString());
    }
    else
    	return;

    //! Get path and set this as the subject list path text in
    //! parameter panel.
    this.dataDirectory = chooser.getCurrentDirectory().getCanonicalPath();
    this.imagepathTextArea.setText(this.dataDirectory);
    
	//! Get selected files and set them to subject list text.
    this.subjectList.clear();
    int listLength = chooser.getSelectedFiles().length;
    File[] subjectListFiles = new File[listLength];
    subjectListFiles = chooser.getSelectedFiles();
    for(int i=0;i<subjectListFiles.length;i++){
    	this.subjectList.add(subjectListFiles[i].getName());
    }

   
}


//! Set filename with algorithm parameter settings.
void setParameterFilename() throws IOException {
	//! Choose parameter text file.
    JFileChooser chooser = new JFileChooser();
    chooser.setDialogTitle("Select Parameter File");
    chooser.setCurrentDirectory(new java.io.File(this.parameterfilenamepathTextArea.getText()));
    // Note: source for ExampleFileFilter can be found in FileChooserDemo,
    // under the demo/jfc directory in the Java 2 SDK, Standard Edition.
    FileNameExtensionFilter filter = 
    	new FileNameExtensionFilter("Text file", "txt");
    chooser.setFileFilter(filter);
    int returnVal = chooser.showOpenDialog(getParent());
    if(returnVal == JFileChooser.APPROVE_OPTION) {
       System.out.println("You chose this parameter file: " +
            chooser.getSelectedFile().getCanonicalPath());
    }
    else
    	return;
    
    String fullPathFilename = chooser.getSelectedFile().getCanonicalPath();
    
    //! Display and set parameter file path.
    this.parameterfilenamepathTextArea.setText(fullPathFilename);
}


//! Set output path.
void setOutputPath() throws IOException {
	//! Choose output path.
    JFileChooser chooser = new JFileChooser();
    // Note: source for ExampleFileFilter can be found in FileChooserDemo,
    // under the demo/jfc directory in the Java 2 SDK, Standard Edition.
    chooser.setCurrentDirectory(new java.io.File(this.outputpathTextArea.getText()));
    chooser.setDialogTitle("Select output path");
    chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
    chooser.setAcceptAllFileFilterUsed(false);
    
    int returnVal = chooser.showOpenDialog(getParent());
    if(returnVal == JFileChooser.APPROVE_OPTION) {
       System.out.println("You chose this output path: " +
            chooser.getSelectedFile());
    }
    else
    	return;

    //! Display and set parameter file path.
    this.outputpathTextArea.setText(chooser.getSelectedFile().getCanonicalPath() + 
    		File.separator);
}


//! Run quantification through instantiation of JNI class.
void callQuantifier() throws IOException {
	//! Copy subject entry (complete file path) to string.
	//! Debug.
//	FileWriter logFile = new FileWriter("log.txt");
//	PrintWriter logfileWriter = new PrintWriter(logFile);
	String selection = (String) this.subjectJList.getSelectedValue();
	String path = this.dataDirectory + File.separator + selection;
	File pathFile = new File(path);
	path = pathFile.getPath();
//	logfileWriter.println(selection);
//	logfileWriter.println(path);

	//! Copy workflow ID to string.
	short workflowID = (short) this.anatomicalLocation.getSelectedIndex();
	
	//! Call quantification algorithm.
	PQCTAnalysisJNI quantifierJNI = new PQCTAnalysisJNI();
	quantifierJNI.execute(path, workflowID, 
			this.parameterfilenamepathTextArea.getText(),
			this.outputpathTextArea.getText());

	//! Update text area.
	File selectionFile = new File(selection);
	selection = selectionFile.getName();	
	// String[] subDirs = selection.split(File.separator);
	// selection = subDirs[subDirs.length - 1];
	String quantificationfilePath = "";

//	logfileWriter.println(selection);
	
	if( workflowID < 4 ) {
		if( workflowID < 3) {
			quantificationfilePath = this.outputpathTextArea.getText() + 
			selection + "_"	+ 
			TIBIA_LEVEL[workflowID] +
			".Quantification.txt";
		}
		else if (workflowID == 3) {
			String filePrefix = "";
			int dotPosition = selection.lastIndexOf('.');
			if (dotPosition != -1)
				filePrefix = selection.substring(0, dotPosition);
			else
				filePrefix = selection;
			quantificationfilePath = this.outputpathTextArea.getText() + 
			filePrefix + "_"	+ 
			TIBIA_LEVEL[workflowID] +
			".Quantification.txt";
		}
//		logfileWriter.println(quantificationfilePath);
//		logfileWriter.close();
		File pathFile2 =  new File(quantificationfilePath);
		quantificationfilePath = pathFile2.getPath();
		BufferedReader reader = null;
		try {
			reader = new BufferedReader(new FileReader(quantificationfilePath));
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		//! Display subject ID in quantification panel.
		// this.quantificationJTextArea.append(selection + "\n");
		//! Display quantification results in the same panel.
		String currentLine = null;
		try {
			while((currentLine = reader.readLine()) != null	) {
				this.quantificationJTextArea.append(currentLine);
				this.quantificationJTextArea.append("\n");
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		try {
			reader.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	else if (workflowID == 4)
		this.quantificationJTextArea.append(selection + " was anonymized.\n");

}	

//! Save quantification text area contents to file chosen by user.
void saveQuantificationResults() throws IOException {
	//! Choose subject list text file.
	JFileChooser chooser = new JFileChooser();
	// Note: source for ExampleFileFilter can be found in FileChooserDemo,
	// under the demo/jfc directory in the Java 2 SDK, Standard Edition.
	FileNameExtensionFilter filter = 
		new FileNameExtensionFilter("Text file", "txt");
	chooser.setFileFilter(filter);
	int returnVal = chooser.showSaveDialog(getParent());
	if(returnVal == JFileChooser.APPROVE_OPTION) {
		System.out.println("You chose to save as file: " +
				chooser.getSelectedFile().getCanonicalPath());
	}
	else
		return;

	//! Save quantification results in chosen file.
	File file = chooser.getSelectedFile();
	
	//! If we chose a file then write quantification results to it.
	System.out.println(file.getName());
	FileWriter writer = new FileWriter(file, false);
	writer.write(this.quantificationJTextArea.getText());
	writer.close();

}

}


//// Extend JList to be able to access JList elements. 
//@SuppressWarnings("serial")
//class MutableList extends JList {
//    MutableList() {
//	super(new DefaultListModel());
//    }
//    DefaultListModel getContents() {
//	return (DefaultListModel)getModel();
//    }
//}   
