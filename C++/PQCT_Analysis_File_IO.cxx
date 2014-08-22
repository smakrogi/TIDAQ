/*===========================================================================

  Program:   Bone, muscle and fat quantification from PQCT data.
  Module:    $RCSfile: PQCT_Analysis_File_IO.cxx,v $
  Language:  C++
  Date:      $Date: 2012/07/02 11:13:00 $
  Version:   $Revision: 0.5 $
  Author:    S. K. Makrogiannis
  3T MRI Facility National Institute on Aging/National Institutes of Health.

  =============================================================================*/


#include <itkImageRegionIterator.h>
#include <itkImportImageFilter.h>
#include <itkImageDuplicator.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkConstantPadImageFilter.h>
#include <itkGDCMImageIO.h>
#include <itkMetaDataObject.h>
#include <gdcmGlobal.h>
#include <itkOrientImageFilter.h>

#include "PQCT_Datatypes.h"
#include "PQCT_Analysis.h"


//! Read dicom and retrieve specific information.
std::string PQCT_Analyzer::RetrieveDicomTagValue(std::string tagkey, 
						 itk::GDCMImageIO::Pointer gdcmImageIO) {
  std::string labelId, value;
  if( itk::GDCMImageIO::GetLabelFromTag( tagkey, labelId ) )
    {
    std::cout << labelId << " (" << tagkey << "): ";
    if( gdcmImageIO->GetValueFromTag(tagkey, value) )
      {
      std::cout << value;
      }
    else
      {
      std::cout << "(No Value Found in File)";
      }
    std::cout << std::endl;
    }
  else
    {
    std::cerr << "Trying to access inexistant DICOM tag." << std::endl;
    }

  return value;
}


//! Read dicom file of CT image.
int PQCT_Analyzer::ReadDicomCTImage() {

  typedef itk::ImageFileReader<PQCTImageType> CTImageFileReaderType;
  CTImageFileReaderType::Pointer reader = CTImageFileReaderType::New();
  reader->SetFileName( this->m_PQCTImageFilename.c_str() );

  typedef itk::GDCMImageIO           ImageIOType;
  ImageIOType::Pointer gdcmImageIO = ImageIOType::New();
  gdcmImageIO->SetMaxSizeLoadEntry(0xffff);
  reader->SetImageIO( gdcmImageIO );
  
  try
    {
    reader->Update();
    }
  catch (itk::ExceptionObject & e)
    {
    std::cerr << "Exception in file reader " << std::endl;
    std::cerr << e << std::endl;
    throw("Cannot read input file");
    return EXIT_FAILURE;
    }

  this->m_PQCTImage = reader->GetOutput();

  //! Re-orient image to RAI system.
  // this->m_PQCTImage = 
  //   ReorientImage<PQCTImageType>( reader->GetOutput(), 
  // 				   itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI 
  // 				   );

  //! Retrieve relevant metadata.
  //! Patient ID.
  std::string patientIDTagKey = "0010|0020";
  std::string patientbirthdayTagKey = "0010|0030";
  std::string sliceoriginTagKey = "0020|0032";
  std::string scandateTagKey = "0008|0022";
  this->ExtractSubjectID();
  std::cout << "Subject ID (from filename): " << this->m_SubjectID << std::endl;
  this->m_PatientInformation.PatientNumber = atoi(this->RetrieveDicomTagValue(patientIDTagKey, gdcmImageIO).c_str());
  this->m_PatientInformation.PatientName = "Anonymous";
  this->m_PatientInformation.PatientBirthDate = atoi(this->RetrieveDicomTagValue(patientbirthdayTagKey, gdcmImageIO).c_str());
  std::stringstream tempStringStream(this->RetrieveDicomTagValue(sliceoriginTagKey, gdcmImageIO));
  tempStringStream >> this->m_DetectorInformation.SliceOrigin;  
  this->m_DetectorInformation.ScanDate = atoi(this->RetrieveDicomTagValue(scandateTagKey, gdcmImageIO).c_str());


  //! Write orginal image to nifti file.
  itk::ImageFileWriter< PQCTImageType >::Pointer 
    InputWriter = itk::ImageFileWriter< PQCTImageType >::New();
  InputWriter->SetInput( this->m_PQCTImage );
  InputWriter->SetFileName( this->m_outputPath + this->m_SubjectID + inputImageFileExtension );
  InputWriter->Update();
  InputWriter = 0;


  return EXIT_SUCCESS;
}


//! Read the input image header.
void PQCT_Analyzer::ReadpQCTImageHeader() {

  //! Identify ID from filename.
  this->ExtractSubjectID();
  std::cout << "Subject ID: " << this->m_SubjectID << std::endl;

  //! Open input stream and set filename.
  std::ifstream inputFile;
  inputFile.open( this->m_PQCTImageFilename.c_str(), std::ios::binary );
  if (inputFile.fail()) {
    throw "Unable to open image file for reading";
    return;
  }

  //! Read header structures of CT image.
  
  //! 1: FilePreFix (header version, size).
 
  //! Header version.
  inputFile.read( (char*) &this->m_HeaderPrefix.HeaderVersion, sizeof(int) );

  //! Header total length.
  inputFile.read( (char*) &this->m_HeaderPrefix.HeaderLength, sizeof(int) );

  //! Exception handling, return if this is not the expected file type.
  if (this->m_HeaderPrefix.HeaderLength != headerLength) {\
    throw("Unrecognized input file format.");
    return;
}

  std::cout << "Header info"
  	    << "\t"
  	    << this->m_HeaderPrefix.HeaderVersion
  	    << "\t"
  	    << this->m_HeaderPrefix.HeaderLength
  	    << std::endl;


  //! 2: DetRec (detector's geometry).

  //! Total length of detector's geometry section.
  inputFile.read( (char*) &this->m_DetectorInformation.DetRecTypeLength, sizeof(int) );

  //! CT image voxel size.
  inputFile.read( (char*) &this->m_DetectorInformation.VoxelSize, sizeof(double) );

  //! Jump a few places in the header.
  inputFile.ignore(10, EOF);

  //! CT image number of slices (should be 1).
  inputFile.read( (char*) &this->m_DetectorInformation.NumberofSlices, sizeof(short) );

  //! CT image slice origin on superior inferior axis.
  inputFile.read( (char*) &this->m_DetectorInformation.SliceOrigin, sizeof(double) );

  std::cout << "Header info"
  	    << "\t"
  	    << this->m_DetectorInformation.DetRecTypeLength
  	    << "\t"
  	    << this->m_DetectorInformation.SliceOrigin
  	    << std::endl;

  //! Jump to the measurement info of this section.
  int bytestoSectionEnd = this->m_DetectorInformation.DetRecTypeLength - 
    (LONGINT + DOUBLE + WORD + DOUBLE + WORD + DOUBLE);
  int measurementInfoFromSectionEnd = (3 * LONGINT + 4 * SHORTINT + 2 * BYTE + BOOLEAN +  
				       MAXNUMDET * BYTE + 2 * SINGLE +
				       BOOLEAN + SINGLE + 13 * CHAR + CHAR +
				       2 * SINGLE + 2 * BYTE + 2 * BOOLEAN + BYTE);
  int bytesToJump = bytestoSectionEnd - measurementInfoFromSectionEnd;
  inputFile.ignore(bytesToJump, EOF);
  
  //! CT scan date.
  inputFile.read( (char*) &this->m_DetectorInformation.ScanDate, sizeof(int) );
  // std::cout << "Scan date: " << this->m_DetectorInformation.ScanDate << std::endl;

  //! Jump to the end of this section.
  bytestoSectionEnd = measurementInfoFromSectionEnd - LONGINT;
  inputFile.ignore(bytestoSectionEnd, EOF);
  

  //! 3: PatInfoRec (patient information).
  //! Total length of patient information section.
  inputFile.read( (char*) &this->m_PatientInformation.PatInfoRecTypeLength, sizeof(int) );

  //! // Jump a few places in the header.
  //! int bytesToJump = 2;
  //! inputFile.ignore(bytesToJump, EOF);

  //! Patient gender, ethnic group, measurement number, patient number and birthdate.
  inputFile.read( (char*) &this->m_PatientInformation.PatientGender, sizeof(short) );
  inputFile.read( (char*) &this->m_PatientInformation.PatientEthnicGroup, sizeof(short) );
  inputFile.read( (char*) &this->m_PatientInformation.PatientMeasurementNumber, sizeof(short) );
  inputFile.read( (char*) &this->m_PatientInformation.PatientNumber, sizeof(int) );
  inputFile.read( (char*) &this->m_PatientInformation.PatientBirthDate, sizeof(int) );

  //! Jump a few places in the header.
  bytesToJump = LONGINT;
  inputFile.ignore(bytesToJump, EOF);

  //! Patient name.
  char * charBuffer;
  charBuffer = new char[41];
  inputFile.read( (char*) charBuffer, 41 );
  int nameLength = static_cast<int> ( charBuffer[0] );
  this->m_PatientInformation.PatientName =  charBuffer;
  this->m_PatientInformation.PatientName = 
    this->m_PatientInformation.PatientName.substr(1, nameLength);
  delete [] charBuffer;

  //! Jump a few places in the header.
  bytesToJump = CHAR + 41 * CHAR + CHAR + 81 * CHAR + CHAR + LONGINT;
  inputFile.ignore(bytesToJump, EOF);

  //! User ID.
  charBuffer = new char[13];
  inputFile.read( (char*) charBuffer, 13 );
  this->m_PatientInformation.UserID =  charBuffer;
  delete [] charBuffer;

  //! Patient ID.
  charBuffer = new char[13];
  inputFile.read( (char*) charBuffer, 13 );
  this->m_PatientInformation.PatientID = charBuffer;
  delete [] charBuffer;

  //! Jump to the end of this section.
  // bytesToJump = 4 + 2 + 2 + 2 + 4 + 4 + 4 + 41 + 1 + 41 + 1 + 81 + 1 + 4 + 13 + 13;
  bytesToJump = ( LONGINT + WORD + WORD + WORD + LONGINT + LONGINT + LONGINT + 41 * CHAR +
		  CHAR + 41 * CHAR + CHAR + 81 * CHAR + CHAR + LONGINT + 13 * CHAR + 13 * CHAR );
  bytestoSectionEnd = this->m_PatientInformation.PatInfoRecTypeLength - 
    bytesToJump;
  inputFile.ignore(bytestoSectionEnd, EOF);
 
  // std::cout << "Header info"
  // 	    << "\t"
  // 	    << this->m_PatientInformation.PatInfoRecTypeLength
  // 	    << "\t"
  // 	    << this->m_PatientInformation.PatientNumber
  // 	    << "\t"
  // 	    << this->m_PatientInformation.PatientID
  // 	    << std::endl;


  //! 4: PicInfoRec (CT image information).
  // Total length of CT image information section.
  inputFile.read( (char*) &this->m_ImageInformation.PicInfoRecLength, sizeof(int) );
  //! Left edge of image.
  inputFile.read( (char*) &this->m_ImageInformation.PicX0, sizeof(short) );
  //! Upper edge of image.
  inputFile.read( (char*) &this->m_ImageInformation.PicY0, sizeof(short) );
  //! X-matrix size.
  inputFile.read( (char*) &this->m_ImageInformation.MatrixSize[0], sizeof(short) );
  //! Y-matrix size.
  inputFile.read( (char*) &this->m_ImageInformation.MatrixSize[1], sizeof(short) );

  //! Jump to the end of this section.
  // bytestoSectionEnd = this->m_ImageInformation.PicInfoRecLength - 
  //   (4 + 2 + 2 + 2 + 2);
  bytestoSectionEnd = this->m_ImageInformation.PicInfoRecLength - 
    ( LONGINT + WORD + WORD + WORD + WORD );
  inputFile.ignore(bytestoSectionEnd, EOF);

  // std::cout << "Header info"
  // 	    << "\t"
  // 	    << this->m_ImageInformation.PicInfoRecLength
  // 	    << "\t"
  // 	    << this->m_ImageInformation.MatrixSize
  // 	    << std::endl;

  //! Close file.
  inputFile.close();
}


//! Calibrate PQCT image using pixel arithmetic.
void PQCT_Analyzer::CalibrateImage() {

  //  Rectangle r = ip.getRoi();
  //  for (int y=r.y; y<(r.y+r.height); y++)
  //   for (int x=r.x; x<(r.x+r.width); x++) {
  //   float originalValue = (float)ip.get(x,y) - 32768.0F;
  //   int calibratedValue = (int)Math.round((slope * ((float)originalValue/1000.0F)) + intercept);
  //   // IJ.log("vals: " + originalValue + " " + calibratedValue);
  //   short shortCalibratedValue = (short) (calibratedValue - 32768);
  //   ip.set(x, y, (short)shortCalibratedValue);
  //  }
  // }

  typedef itk::ImageRegionIterator<PQCTImageType> PQCTImageIteratorType;
  PQCTImageIteratorType itImage(this->m_PQCTImage, 
				this->m_PQCTImage->GetBufferedRegion());

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    float originalValue = (float) itImage.Get(); // - 32768.0F;
    // int calibratedValue = (int) MY_ROUND(((slope * (float)originalValue) + intercept) /1000.0F );
    int calibratedValue = (int) MY_ROUND((this->m_AUtoDensitySlope * 
					  ((float)originalValue/1000.0F)) + 
					 this->m_AUtoDensityIntercept); 
    itImage.Set( (short) calibratedValue );
  }

}


//! Read the input image in the native format 
//! including the header (new version).
void PQCT_Analyzer::ReadPQCTImage() {

  // // Identify ID from filename.
  // this->ExtractSubjectID();
  // std::cout << "Subject ID: " << this->m_SubjectID << std::endl;

  // // Open input stream and set filename.
  std::ifstream inputFile;
  // inputFile.open( this->m_PQCTImageFilename.c_str(), std::ios::binary );
  // if (inputFile.fail()) {
  //   throw "Unable to open image file for reading";
  //   return;
  // }

  // // Set file size counters to 0.
  // int number = 0;
  // int count = 0;
  // // int headerLength = 1609;

  // // Skip the first 1609 elements.
  // inputFile.ignore(headerLength, EOF);
  // // Read until EOF and count number of elements.
  // do{
  //   inputFile.read( (char*) &number, sizeof(short) );
  //   count++;
  //   // std::cout << number << "," << count << std::endl;
  // }
  // while(!inputFile.eof());
  // inputFile.close();
  // std::cout << "Number of elements: " << count << std::endl;

  // // Compute image dimensions.
  // int imageSide = (int) sqrt( (double)count );
  // // imageSide = 283;

  //! New header reader.
  this->ReadpQCTImageHeader();

  //! Read-in the file and store pixels in array.
  inputFile.open( m_PQCTImageFilename.c_str(),  std::ios::binary );
  if (inputFile.fail()) {
    throw "Unable to open image file for reading";
    return;
  }
  inputFile.ignore(headerLength, EOF);

  int count = this->m_ImageInformation.MatrixSize[0] *
    this->m_ImageInformation.MatrixSize[1];
  short* buffer =  new short[count];
  count = 0;
  // Read until EOF and count number of elements.
  do{
    inputFile.read( (char*)buffer+sizeof(short)*count, sizeof(short) );
    count++;
  }
  while(!inputFile.eof());

  //! Close file.
  inputFile.close();
  // std::cout << "Image side: " << imageSide << std::endl;
  

  //! Then create an itk image  
  //! copy pixel intensities and
  //! add metadata.
  PQCTImageType::RegionType inputRegion;
  PQCTImageType::SizeType size_var;
  PQCTImageType::IndexType index_var;
  double origin[ pixelDimensions ];

  for (int i = 0; i < pixelDimensions; i++) {
    size_var[i] = this->m_ImageInformation.MatrixSize[i];
    index_var[i] = 0;
    origin[i] = 0.0;	
  }
  inputRegion.SetIndex(index_var);
  inputRegion.SetSize(size_var);

  //! Import PQCT image using itk's import image filter.
  typedef itk::ImportImageFilter<PQCTPixelType, pixelDimensions> PQCTImportFilter;
  PQCTImportFilter::Pointer importPQCT = PQCTImportFilter::New();

  importPQCT->SetRegion(inputRegion);
  importPQCT->SetSpacing( this->m_DetectorInformation.VoxelSize );
  importPQCT->SetOrigin( origin );
  importPQCT->ReleaseDataFlagOn();
  importPQCT->SetImportPointer(buffer, count, false); 
  importPQCT->Update();


  //! Use duplicator to detach from file stream.
  typedef itk::ImageDuplicator< PQCTImageType > PQCTDuplicatorType;
  PQCTDuplicatorType::Pointer pqctDuplicator = PQCTDuplicatorType::New();
  pqctDuplicator->SetInputImage( importPQCT->GetOutput() );
  pqctDuplicator->Update();


  //! Pad image to facilitate subsequent segmentation.
  typedef itk::ConstantPadImageFilter <PQCTImageType, PQCTImageType>
    ConstantPadImageFilterType;
  unsigned int padLength = 5;
  PQCTImageType::SizeType extendRegion;
  extendRegion[0] = padLength;
  extendRegion[1] = padLength;
  PQCTImageType::PixelType backgroundValue = BACKGROUND;
  ConstantPadImageFilterType::Pointer padFilter
    = ConstantPadImageFilterType::New();
  padFilter->SetInput(pqctDuplicator->GetOutput());
  padFilter->SetPadBound(extendRegion); // Calls SetPadLowerBound(region) and SetPadUpperBound(region)
  // padFilter->SetPadLowerBound(extendRegion);
  // padFilter->SetPadUpperBound(extendRegion);
  padFilter->SetConstant(backgroundValue);
  padFilter->Update();
  this->m_PQCTImage = padFilter->GetOutput();


  //! Calibrate itk image (convert from attenutation units to densities).
  this->CalibrateImage();


  //! Write image to file (debug).
  itk::ImageFileWriter< PQCTImageType >::Pointer 
    InputWriter = itk::ImageFileWriter< PQCTImageType >::New();
  InputWriter->SetInput( this->m_PQCTImage );
  InputWriter->SetFileName( this->m_outputPath + this->m_SubjectID + inputImageFileExtension );
  InputWriter->Update();
  InputWriter = 0;

  delete[] buffer;
}


//! Anonymize pQCT image.
void PQCT_Analyzer::AnonymizePQCTImage() {

  //! Identify ID from filename.
  this->ExtractSubjectID();
  std::cout << "Subject ID: " << this->m_SubjectID << std::endl;

  //! Open input stream.
  std::ifstream inputFile;
  inputFile.open( this->m_PQCTImageFilename.c_str(), std::ios::binary );
  if (inputFile.fail()) {
    throw "Unable to open image file for reading";
    return;
  }

  //! Open output stream.
  std::ofstream outputFile;
  std::string fullPath = this->m_outputPath + 
    this->m_SubjectID + 
    anonymizedImageFileExtension ;

  outputFile.open( fullPath.c_str(), std::ios::binary );
  if (outputFile.fail()) {
    throw "Unable to open image file for writing";
    return;
  }

  //! Write input to output file and close both files.
  if(inputFile.is_open() && outputFile.is_open())
      while(!inputFile.eof())
	  outputFile.put(inputFile.get());
 
  // outputFile << inputFile.rdbuf();

  // Close both files.
  inputFile.close();
  outputFile.close();


  //! Then open again the output file.
  std::fstream anonymizationFile;
  anonymizationFile.open( fullPath.c_str(), std::ios::in|std::ios::out|std::ios::binary );
  if (anonymizationFile.fail()) {
    throw "Unable to open image file for reading/writing";
    return;
  }

  //! Read header structures of CT image.
  
  //! 1: FilePreFix (header version, size).
  //! Header version.
  anonymizationFile.read( (char*) &this->m_HeaderPrefix.HeaderVersion, sizeof(int) );

  //! Header total length.
  anonymizationFile.read( (char*) &this->m_HeaderPrefix.HeaderLength, sizeof(int) );

  //! Exception handling, return if this is not the expected file type.
  if (this->m_HeaderPrefix.HeaderLength != headerLength) {\
    throw("Unrecognized input file format.");
    return;
}
  std::cout << "Header info"
  	    << "\t"
  	    << this->m_HeaderPrefix.HeaderVersion
  	    << "\t"
  	    << this->m_HeaderPrefix.HeaderLength
  	    << std::endl;

  //! 2: DetRec (detector's geometry).
  //! Total length of detector's geometry section.
  anonymizationFile.read( (char*) &this->m_DetectorInformation.DetRecTypeLength, sizeof(int) );

  //! Jump to the end of this section.
  int bytestoSectionEnd = this->m_DetectorInformation.DetRecTypeLength - 4;
  anonymizationFile.ignore(bytestoSectionEnd, EOF);
  // std::cout << "Bytes to section's end: " << bytestoSectionEnd << std::endl;

  //! 3: PatInfoRec (patient information).
  //! Total length of patient information section.
  anonymizationFile.read( (char*) &this->m_PatientInformation.PatInfoRecTypeLength, sizeof(int) );

  //! Jump a few places in the header.
  int bytestoJump = WORD + WORD + WORD + LONGINT + LONGINT + LONGINT;
  anonymizationFile.ignore(bytestoJump, EOF);

  //! Count bytes to end and write the anonymization character to file.
  char anonymizationString[40] = "Anonymoys";
  char anonymizationLength = static_cast<char>( strlen(anonymizationString) );
  
  //! Get read position and set to write position.
  int position = (int) anonymizationFile.tellg();
  anonymizationFile.seekp( position, std::ios::beg );

  //! Write string of characters.
  anonymizationFile.write((char*) &anonymizationLength, sizeof(char));
  anonymizationFile.write((char*) anonymizationString, 40);
  anonymizationFile.seekg( 0, std::fstream::end );

  //! Alternate way of anonymization.
  // int bytestoAnonymize = 40;
  // char anonymizationCharacter = '-';
  // char stringEnd = '\0';
  // std::cout << "Bytes to anonymize: " << bytestoAnonymize 
  // 	    << "\t Get pointer position: " << position
  // 	    << std::endl;
  // int count = 0;
  // while( count < bytestoAnonymize ) {
  //   anonymizationFile.write((char*) &anonymizationCharacter, sizeof(char));
  //   count++;
  // }
  // anonymizationFile.write((char*) &stringEnd, sizeof(char));

  //! Close file.
  anonymizationFile.close();
  std::cout << "Anonymization completed." << std::endl;

}


//! Copy all parameter names and values to text file for validation.
void PQCT_Analyzer::WriteToTextFile() {

  //! Set format of output file.
  this->m_textFile.setf(std::ios::fixed, std::ios::floatfield);
  this->m_textFile.precision(FLOAT_PRECISION);

  // Form the measurement text file.
  // Pass headers
  this->m_textFile << this->m_TissueShapeEntries.headerString.str();
  this->m_textFile << this->m_TissueIntensityEntries.headerString.str();
  this->m_textFile << std::endl;

  // Pass values.
  this->m_textFile << this->m_TissueShapeEntries.valueString.str();
  this->m_textFile << this->m_TissueIntensityEntries.valueString.str();
  this->m_textFile << std::endl;

}


//! Copy segmentation parameters from the static variables to
//! the corresponing class member variables.

void PQCT_Analyzer::SetParameters()
{

  //! Initialize array of parameter names using the static variable definitions.

  this->m_numberofParameters = sizeof(parameterValues) / sizeof(parameterValues[0]);

  std::copy( parameterIDs, 
             parameterIDs + this->m_numberofParameters, 
             std::back_inserter(m_parameterIDs) ) ;

  //! Initialize parameter values using the static variable definitions.

  std::copy( parameterValues, 
             parameterValues + this->m_numberofParameters, 
             std::back_inserter(m_parameterValues) ) ;

  //! Copy values to class variables.
  this->CopyParameterValuesToClassVariables();
	
}


//! Read and parse segmentation parameters to accomodate experiments
//! with different algorithm settings.

int PQCT_Analyzer::ParseSegmentationParameterFile(std::string paramsFilename,
						  bool debugFlag)
{
  std::string bufferID;
  float bufferValue;
  std::ifstream paramFile;
  //std::string commentString = "##";

  //! Read in parameter file line by line.
  this->m_parameterFilename = paramsFilename;
  paramFile.open( this->m_parameterFilename.c_str() );

  if( paramFile.bad() || ( ! paramFile.is_open() ) ) {
    std::cout << "Cannot open " << this->m_parameterFilename << ", "
              << "will use default parameter values." << std::endl;
    return EXIT_FAILURE;
  }


  //! Repeat until the last line.
  while (!paramFile.eof() ) {
    paramFile >> bufferID >>  bufferValue;
    int i = 0;
    int flag = 0;

    //! Scan the parameter ids.
    while( i < m_parameterIDs.size() && flag == 0 ) {

      //! If match found, replace corresponding values in m_parameterValues.
      if( bufferID.compare(m_parameterIDs[i]) == 0 ) {
	this->m_parameterValues[i] = bufferValue;
	
	if(debugFlag)
            std::cout << parameterIDs[i] << ":"
                      << this->m_parameterValues[i]
                      << std::endl;

	flag = 1;
      }
      i++;
    }
  }

  //! Return 0 if successfully completed.
  paramFile.close();

  //! Copy values to class variables.
  this->CopyParameterValuesToClassVariables();
  
  return EXIT_SUCCESS;
}



//! Copy parameter values to program's variables.
void PQCT_Analyzer::CopyParameterValuesToClassVariables() {
  this->m_AUtoDensitySlope = this->m_parameterValues[0];
  this->m_AUtoDensityIntercept = this->m_parameterValues[1];
  this->m_gradientSigma = this->m_parameterValues[2];
  this->m_medianFilterKernelLength = this->m_parameterValues[3];
  this->m_sigmoidBeta = this->m_parameterValues[4];
  this->m_sigmoidAlpha = (-1) * (this->m_sigmoidBeta / this->m_parameterValues[5] );
  this->m_fastmarchingStoppingTime = this->m_parameterValues[6];
  this->m_levelsetPropagationScalingFactor = this->m_parameterValues[7];
  this->m_levelsetCurvatureScalingFactor = this->m_parameterValues[8];
  this->m_levelsetAdvectionScalingFactor = this->m_parameterValues[9];
  this->m_levelsetMaximumIterations = this->m_parameterValues[10];
  this->m_levelsetMaximumRMSError = this->m_parameterValues[11];
  this->m_SAT_IMFAT_SeparationAlgorithm = this->m_parameterValues[12];
  this->m_CT_LegThreshold = this->m_parameterValues[13];
}
