/** Lower extremities fat quantification using PQCT data.
  @author: Sokratis Makrogiannis, 3T @ NIA
 */

import ij.IJ;
import ij.ImagePlus;
import ij.WindowManager;
import ij.gui.Roi;
import ij.io.OpenDialog;
import ij.plugin.Duplicator;
import ij.plugin.frame.PlugInFrame;
import ij.process.ImageProcessor;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Frame;
import java.awt.GridLayout;
import java.awt.Panel;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;
import java.io.BufferedReader;
import java.io.CharArrayWriter;
import java.io.File;
import java.io.PrintWriter;
import java.util.Scanner;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JTextField;

public class Tibia_Analysis_PQCT extends PlugInFrame implements ActionListener{

	Panel panel, parameterPanel;
	int previousID;
	static Frame instance;

	ImagePlus img;
	ImageProcessor ip;
	JTextField nClustersTextField;
	JComboBox anatomicalLocation;

	public Tibia_Analysis_PQCT() {
		super("PQCT Analysis");
		// Check version.
		if (IJ.versionLessThan("1.39t"))
			return;
		if (instance!=null) {
			instance.toFront();
			return;
		}
		// Set panels and components.
		instance = this;
		addKeyListener(IJ.getInstance());
		// setLayout(new FlowLayout());
		setLayout(new BorderLayout());

		add(new JLabel("PQCT Analysis App--3T NIA"), BorderLayout.NORTH);

		// Add control panel.
		panel = new Panel();
		panel.setLayout(new GridLayout(4, 2));
		panel.setBackground( Color.green );
		addButton("ReadImage");
		addButton("Denoising");
		addButton("Clustering");
		addButton("Tibia Selection");
		addButton("Fat separation");
		// For level set in Fiji use:
		// IJ.run(imp, "Level Sets", "method=[Active Contours] use use grey=50 distance=0.50 
		// advection=2.20 propagation=1 curvature=1 grayscale=30 convergence=0.0050 region=outside");
		addButton("Quantification");
		addButton("RunPipeline");
		addButton("ReadInfoFile");
		add(panel, BorderLayout.WEST);

		// Add parameter panel.
		this.addParameterPanel();

		setSize(600,400);
		// pack();
		// GUI.center(this);
		setVisible(true);
	}

	// Function for adding buttons.
	void addButton(String label) {
		JButton b = new JButton(label);
		b.addActionListener(this);
		b.addKeyListener(IJ.getInstance());
		panel.add(b);
	}

	// Method for adding parameters.
	void addParameterPanel() {
		// Construct panel and define layout and background.
		parameterPanel = new Panel();
		parameterPanel.setLayout(new GridLayout(2,2));
		parameterPanel.setBackground( Color.green );
		// Add label and text field of the anatomical location.
		parameterPanel.add(new JLabel("Choose anatomical location"));
		String[] anatomicalLocationStrings =
		{"66% Tibia", "38% Tibia", "14% Tibia", "4% Tibia"};
		anatomicalLocation = new JComboBox(anatomicalLocationStrings);
		// anatomicalLocation.addItem("66% Tibia");
		// anatomicalLocation.addItem("38% Tibia");
		// anatomicalLocation.addItem("14% Tibia");
		// anatomicalLocation.addItem("4% Tibia");
		anatomicalLocation.setSelectedIndex(0);
		// anatomicalLocation.addActionListener(this);
		parameterPanel.add(anatomicalLocation);

		// Add label and text field of cluster # parameter.
		parameterPanel.add( new JLabel("Cluster #"));
		nClustersTextField = new JTextField("4");
		nClustersTextField.addActionListener(this);
		parameterPanel.add(nClustersTextField);
		add(parameterPanel, BorderLayout.CENTER);
	}

	public void actionPerformed(ActionEvent e) {
		ImagePlus imp = WindowManager.getCurrentImage();
		if ( imp==null &&
				( !(e.getActionCommand().equals("ReadImage")) &&
						!(e.getActionCommand().equals("RunPipeline")) &&
						!(e.getActionCommand().equals("ReadInfoFile")) ) ) {
			IJ.beep();
			IJ.showStatus("No input image");
			//IJ.log(e.getActionCommand());
			previousID = 0;
			return;
		}
		//    if (!imp.lock())
		//    {previousID = 0; return;}
		if(imp!=null) {
			ImageProcessor ip = imp.getProcessor();
			int id = imp.getID();
			if (id!=previousID)
				ip.snapshot();
			previousID = id;
		}
		// Retrieve the action command.
		String label = e.getActionCommand();
		// Retrieve the algorithm parameters.
		int nClusters = Integer.parseInt(nClustersTextField.getText());
		AlgorithmParameters parameters = new AlgorithmParameters(nClusters);

		if (label==null)
			return;
		new TibiaAnalysisPQCTRunner(label, imp, ip, e, parameters);
	}

	public void processWindowEvent(WindowEvent e) {
		super.processWindowEvent(e);
		if (e.getID()==WindowEvent.WINDOW_CLOSING) {
			instance = null;
		}
	}

}

class TibiaAnalysisPQCTRunner extends Thread {
	private String command;
	private ImagePlus imp, imp2;
	private ImageProcessor ip, ip2;
	private ActionEvent e;

	// static int nClusters = 4;
	static float pixelArea = 0.64f;
	// static float slope = 1724.0f, intercept = -322.0f;
	AlgorithmParameters parameters;

	TibiaAnalysisPQCTRunner(String command, ImagePlus imp, ImageProcessor ip) {
		super(command);
		this.command = command;
		this.imp = imp;
		this.ip = ip;
		setPriority(Math.max(getPriority()-2, MIN_PRIORITY));
		start();
	}

	TibiaAnalysisPQCTRunner(String command, ImagePlus imp,
			ImageProcessor ip, ActionEvent e, AlgorithmParameters parameters) {
		super(command);
		this.command = command;
		this.imp = imp;
		this.ip = ip;
		setPriority(Math.max(getPriority()-2, MIN_PRIORITY));
		this.e = e;
		this.parameters = parameters;
		start();
	}

	public void run() {
		try {runCommand(command, imp, ip);}
		catch(OutOfMemoryError e) {
			IJ.outOfMemory(command);
			if (imp!=null) imp.unlock();
		} catch(Exception e) {
			CharArrayWriter caw = new CharArrayWriter();
			PrintWriter pw = new PrintWriter(caw);
			e.printStackTrace(pw);
			// IJ.write(caw.toString());
			IJ.showStatus("");
			if (imp!=null) imp.unlock();
		}
	}

	void runCommand(String command, ImagePlus imp, ImageProcessor ip) {
		ImageProcessor mask = null;
		IJ.showStatus(command + "...");
		if (!command.equals("ReadImage") &&
				!command.equals("RunPipeline") &&
				!command.equals("ReadInfoFile") ) {
			Roi roi = imp.getRoi();
			mask =  roi!=null?roi.getMask():null;
		}
		if (command.equals("ReadImage")) {
			IJ.run(imp, "Read PQCT", "");
			//imp.setTitle("Input");
			IJ.showStatus("Image reading...done");
		}
		else if (command.equals("Denoising")) {
			// imp = WindowManager.getImage("Input");
			// ip.medianFilter();  // runs on 8bit or RGB images only.
			IJ.run(imp, "Despeckle", "");
			imp.setTitle("Denoised");
			IJ.showStatus("Denoising...done");
		}
		else if (command.equals("Clustering")) {
			// imp = WindowManager.getImage("Denoised");
			String nClusterString = parameters.getnClusters().toString();
			IJ.run(imp, "k-means Clustering ...", "number_of_clusters="+nClusterString+" cluster_center_tolerance=0.10000000 enable_randomization_seed randomization_seed=48 send_to_results_table");
			//imp.setTitle("Clustered");
			IJ.showStatus("Clustering...done");
		}
		else if (command.equals("Quantification")) {
			imp = WindowManager.getImage("Denoised");
			ip = imp.getProcessor();
			imp2 = WindowManager.getImage("Clusters");
			ip2 = imp2.getProcessor();
			if( imp!=null && imp2!=null) {
				computeStatistics(ip2, ip);
				IJ.showStatus("Quantification...done"); }
		}
		else if (command.equals("RunPipeline")) {
			quantificationPipeline("");
			IJ.showStatus("Quantification...done");
		}
		else if (command.equals("ReadInfoFile")) {
			BufferedReader reader = null;
			OpenDialog fileopenDialog = new OpenDialog("Select subject list file", null);
			String filename = fileopenDialog.getFileName();
			String directory = fileopenDialog.getDirectory();
			String fullPathFilename = directory + filename;

			if( filename == null ) return;
			IJ.showStatus("Opening: " + fullPathFilename);
			// IJ.showMessage("My_Plugin", fullPathFilename);

			File file = new File(fullPathFilename);
			Scanner scanner = null;

			try {
				// reader = new BufferedReader(new FileReader(fullPathFilename));
				// String dataPath = reader.readLine();
				// IJ.log(dataPath);
				// reader.close();

				// Read data path.
				scanner = new Scanner(file).useDelimiter("\n");
				String dataPath = scanner.next();
				IJ.log(dataPath);
				while(scanner.hasNext())
				{
					// Read image.
					String newImageFilename = dataPath + scanner.next();
					IJ.log(newImageFilename);
					// Apply image processing pipeline.
					quantificationPipeline(newImageFilename);
					//IJ.log(infoString);
					IJ.showStatus("Quantification...done");
				}
				scanner.close();
			}
			catch (java.io.FileNotFoundException e) {
				IJ.error("Cannot read file.", e.getMessage());
				return;
			}
			catch(java.io.IOException e) {
				IJ.error("Cannot read file.", e.getMessage());
			}

		}

	}


	// Run the complete quantification pipeline.
	public void quantificationPipeline(String newImageFilename) {
		//IJ.run("Read PQCT", newImageFilename);
		Read_PQCT readingObject= new Read_PQCT();
		readingObject.run(newImageFilename);
		imp = IJ.getImage();
		imp = new Duplicator().run(imp);
		imp.show();
		imp.setTitle("Input");
		imp = WindowManager.getImage("Input");
		IJ.run(imp, "Despeckle", "");
		imp.setTitle("Denoised");
		IJ.showStatus("Denoising...done");
		String nClusterString = parameters.getnClusters().toString();
		IJ.run(imp, "k-means Clustering ...", "number_of_clusters="+nClusterString+" cluster_center_tolerance=0.10000000 enable_randomization_seed randomization_seed=48 send_to_results_table");
		IJ.showStatus("Clustering...done");
		imp = WindowManager.getImage("Denoised");
		ip = imp.getProcessor();
		imp2 = WindowManager.getImage("Clusters");
		ip2 = imp2.getProcessor();
		if( imp!=null && imp2!=null) {
			computeStatistics(ip2, ip);
			IJ.showStatus("Quantification...done"); 
		}
		// IJ.showMessage("My_Plugin", fullPathFilename);
	}

	// Compute average.
	public void computeStatistics(ImageProcessor ip, ImageProcessor ip2) {
		String infoString = null;
		// ImageProcessor ip = img.getProcessor();
		float[] sum = new float[this.parameters.getnClusters()];
		float[] count= new float[this.parameters.getnClusters()];
		float[] average= new float[this.parameters.getnClusters()];

		Rectangle r = ip.getRoi();
		for (int i=0; i<this.parameters.getnClusters(); i++) {
			sum[i] = 0.0F;
			count[i] = 0.0F;
			average[i] = 0.0F;
		}

		for (int y=r.y; y<(r.y+r.height); y++)
			for (int x=r.x; x<(r.x+r.width); x++) {
				int pixelValue = ip.get(x,y);
				// IJ.log("val " + pixelValue);
				count[pixelValue]++;
				sum[pixelValue] += ip2.get(x,y);
			}

		for (int i=0; i<this.parameters.getnClusters(); i++) {
			average[i] = sum[i] / count[i];
			float area = count[i] * pixelArea;
			infoString = "Cluster #" + i + " stats: ";
			infoString += "area =  [" + count[i] + "px, " + area + " mm^2], ";
			infoString += "average density = " + (average[i]-32768);
			IJ.log(infoString);
		}
	}


}


// Use this class to pass the algorithm parameters to the image
// analysis pipeline.
class AlgorithmParameters {

	Integer nClusters;

	public AlgorithmParameters( int nClusters ) {
		this.nClusters = nClusters;
	}


	Integer getnClusters() {
		return this.nClusters;
	}

}
