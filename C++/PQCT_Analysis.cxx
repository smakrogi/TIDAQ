/*===========================================================================

  Program:   Bone, muscle and fat quantification from PQCT data.
  Module:    $RCSfile: PQCT_Analysis.h,v $
  Language:  C++
  Date:      $Date: 2012/08/03 11:12:00 $
  Version:   $Revision: 0.5 $
  Author:    S. K. Makrogiannis
  3T MRI Facility National Institute on Aging/National Institutes of Health.

  =============================================================================*/

#include <iostream>
// #include <iomanip>
#include <fstream>
#include <cmath>

#include <itkImageDuplicator.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionIteratorWithIndex.h>

#include <itkCurvatureAnisotropicDiffusionImageFilter.h>
#include <itkMedianImageFilter.h>
#include <itkRecursiveGaussianImageFilter.h>
#include <itkScalarImageKmeansImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkGrayscaleFillholeImageFilter.h>

// Level-set related header files.
#include <itkSignedMaurerDistanceMapImageFilter.h> 
#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>
#include <itkFastMarchingImageFilter.h>
#include <itkSigmoidImageFilter.h>
#include <itkGeodesicActiveContourLevelSetImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>

// Shape and intensity attributes.
#include <itkShapeLabelObject.h>
#include <itkLabelMap.h>
#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkLabelImageToStatisticsLabelMapFilter.h>

#include "PQCT_Datatypes.h"
#include "PQCT_Analysis.h"


//! PQCT analysis class.
//! This should implement all common image analysis operations.


//! Execute the workflow.
void PQCT_Analyzer::Execute() {

  //! Read file with parameter values.
  this->ParseSegmentationParameterFile(this->m_parameterFilename,
				       true);

  //! Read original image according to anatomical site.
  try {
    switch(this->m_WorkflowID) {
    case PQCT_FOUR_PCT_TIBIA: 
    case PQCT_THIRTYEIGHT_PCT_TIBIA:
    case PQCT_SIXTYSIX_PCT_TIBIA:
      this->ReadPQCTImage();
      break;
    case CT_MID_THIGH:
      this->ReadDicomCTImage();
      break;
    case PQCT_ANONYMIZE:
      this->ReadPQCTImage();
      break;
    default:
      std::cerr << "Unknown workflow number." 
		<< std::endl;
      // exit(1);
      break;
    }
  }
  catch(const char * Message) {
    std::cerr << "Error:" << Message << std::endl;
    return;
  }
  
  try {
    //! Switch to algorithm according to anatomical site.
    switch(this->m_WorkflowID) {
    case PQCT_FOUR_PCT_TIBIA://! 4%
      this->Analyze4PCT();
      break;
    case PQCT_THIRTYEIGHT_PCT_TIBIA://! 38%
      this->Analyze38PCT();
      break;
    case PQCT_SIXTYSIX_PCT_TIBIA://! 66%
      this->Analyze66PCT();
      break;
    case CT_MID_THIGH:
      this->AnalyzeCTMidThigh();
      break;
    case PQCT_ANONYMIZE://! Proceed to file anonymization.
      this->AnonymizePQCTImage();
    default:
      std::cerr << "Unknown workflow number." 
		<< std::endl;
      // exit(1);
      break;
    }
  }
  catch (itk::ExceptionObject & e)
    {
      std::cerr << "Exception in quantification algorithm " << std::endl;
      std::cerr << e << std::endl;
      return;
    }
}


//! Smooth out the input image.
FloatImageType::Pointer PQCT_Analyzer::SmoothInputVolume( PQCTImageType::Pointer inputImage,
							  int denoisingMethod )
{
  FloatImageType::Pointer outputVolume = NULL;

  switch( denoisingMethod ) {
  case GAUSSIAN:
    {
      // Linear gaussian blurring.

      // Type definition of recursive gaussian filtering.
      typedef itk::RecursiveGaussianImageFilter<PQCTImageType,
	FloatImageType> 
	GaussianFilterType;

      // Instantiate filter.
      GaussianFilterType::Pointer smoothFilter = GaussianFilterType::New();

      // Set input and parameters.
      smoothFilter->SetInput( inputImage );
      smoothFilter->SetSigma( (double)DIFFUSIONFILTERTIMESTEP *
			      (double)DIFFUSIONFILTERITERATIONS ); // previously: SMOOTHINGSIGMA
      // smoothFilter->SetNormalizeAcrossScale( true );
      smoothFilter->Update();

      // Display execution info.
      std::cout << "Recursive Gaussian filtering completed." << std::endl;
      outputVolume = smoothFilter->GetOutput();
    }
    break;
  case DIFFUSION: 
    {
    // Non-linear diffusion-based smoothing.
    int nIterations = DIFFUSIONFILTERITERATIONS;
    double timestep = DIFFUSIONFILTERTIMESTEP;
    double conductanceParameter = DIFFUSIONFILTERCONDUCTANCEPARAMETER;

    typedef itk::CurvatureAnisotropicDiffusionImageFilter<PQCTImageType, 
	FloatImageType> 
      CurvatureAnisotropicDiffusionImageFilterType;
    CurvatureAnisotropicDiffusionImageFilterType::Pointer smoothFilter = 
      CurvatureAnisotropicDiffusionImageFilterType::New();
    smoothFilter->SetInput( inputImage );
    smoothFilter->SetNumberOfIterations( nIterations );
    smoothFilter->SetTimeStep( timestep );
    smoothFilter->SetConductanceParameter( conductanceParameter );
    smoothFilter->Update();
    std::cout << "Non-linear diffusion filtering completed." << std::endl;
    outputVolume = smoothFilter->GetOutput();
    }
    break;
  case MEDIAN:
    {
      // Statistical median filtering.
      typedef itk::MedianImageFilter<PQCTImageType, 
	FloatImageType>
	MedianImageFilterType;
      MedianImageFilterType::Pointer smoothFilter = 
	MedianImageFilterType::New();
      smoothFilter->SetInput( inputImage );
      FloatImageType::SizeType indexRadius;
      indexRadius[0] = (unsigned int)this->m_medianFilterKernelLength;
      indexRadius[1] = (unsigned int)this->m_medianFilterKernelLength;
      smoothFilter->SetRadius( indexRadius );
      smoothFilter->Update();
      std::cout << "Median filtering completed." << std::endl;
      outputVolume = smoothFilter->GetOutput();
    }
  }
  
  // Return smoothed image.
  return outputVolume;
}


//! Foreground/Background segmentation by fast marching.
LabelImageType::Pointer PQCT_Analyzer::ForegroundBackgroundSegmentationByFastMarching() {

  //! 0. Apply denoising.
  FloatImageType::Pointer smoothedImage = 
    this->SmoothInputVolume( this->m_PQCTImage, 
			     DIFFUSION );


  //! Gradient magnitude.
  typedef   itk::GradientMagnitudeRecursiveGaussianImageFilter<FloatImageType, 
    FloatImageType >  GradientFilterType;

  FloatImageType::Pointer gradientImage = 0; 
  GradientFilterType::Pointer  gradientMagnitude = GradientFilterType::New();
  gradientMagnitude->SetSigma( this->m_gradientSigma ); // 0.0625
  gradientMagnitude->SetInput( smoothedImage );
  gradientMagnitude->Update();
  gradientImage = gradientMagnitude->GetOutput();

  // // Write gradient image to file.
  // std::string gradientImageFilename = this->m_outputPath +
  //   this->m_SubjectID + "_" + "gradientmag1.nii";
  // itk::ImageFileWriter<FloatImageType>::Pointer floatWriter = 
  //   itk::ImageFileWriter<FloatImageType>::New();
  // floatWriter->SetInput( gradientImage ); 
  // floatWriter->SetFileName( gradientImageFilename );
  // floatWriter->Update();
  // floatWriter = 0; 

  //! Speed image.
  typedef   itk::SigmoidImageFilter<FloatImageType, FloatImageType >  SigmoidFilterType;
  SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();
  sigmoid->SetAlpha( -4 );   
  sigmoid->SetBeta( 30 );  
  sigmoid->SetOutputMinimum( 0.0 );
  sigmoid->SetOutputMaximum( 1.0 );
  sigmoid->SetInput( gradientImage ); 
  sigmoid->Update();

  //! Fast marching segmentation.
  // Use fast marching to initialize the segmentation process.
  LabelImageType::IndexType middleIdx;
  for( int i = 0; i < LabelImageType::ImageDimension; i++ ) {
    // middleIdx[i] = (int) MY_ROUND(this->m_PQCTImage->GetLargestPossibleRegion().GetSize()[i] / 2.0F);
    middleIdx[i] = 0;
  }

  LabelImageType::Pointer roiVolume = 
    this->InitializeROIbyFastMarching( sigmoid->GetOutput(),
				       middleIdx,
				       500.0F );
  std::cout << "FM-based ROI generation." << std::endl;

  // Invert pixel values before feature computation.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(roiVolume, 
				 roiVolume->GetBufferedRegion());
  LabelImageType::IndexType idx;

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) 
    if ( itImage.Get() == BACKGROUND ) 
      itImage.Set( TOT_AREA );    
    else if ( itImage.Get() == FOREGROUND )
      itImage.Set( BACKGROUND ); 

  std::string fmImageFilename = this->m_outputPath + 
    this->m_SubjectID + "_"  + 
    "FB_Mask.nii";
  itk::ImageFileWriter<LabelImageType>::Pointer maskWriter = 
    itk::ImageFileWriter<LabelImageType>::New();
  maskWriter->SetInput( roiVolume ); 
  maskWriter->SetFileName( fmImageFilename );
  maskWriter->Update();
  maskWriter = 0;

  return roiVolume;
}


//! Foreground/Background segmentation after K-means.
LabelImageType::Pointer PQCT_Analyzer::ForegroundBackgroundSegmentationAfterKMeans() {

  // Allocate new itk image.
  LabelImageType::Pointer outputlabelImage = LabelImageType::New();
  outputlabelImage->CopyInformation( this->m_KmeansLabelImage );
  outputlabelImage->SetRegions( this->m_KmeansLabelImage->GetLargestPossibleRegion() );
  outputlabelImage->Allocate();
  outputlabelImage->FillBuffer( BACKGROUND );

  // Mark non-background pixels.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_KmeansLabelImage, 
				 this->m_KmeansLabelImage->GetBufferedRegion());
  LabelImageType::IndexType idx;

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    idx = itImage.GetIndex();
    if ( itImage.Get() != BACKGROUND ) 
      outputlabelImage->SetPixel(idx, TOT_AREA );    
  }

  std::string fmImageFilename = this->m_outputPath + 
    this->m_SubjectID + "_"  + 
    "FB_Mask.nii";
  itk::ImageFileWriter<LabelImageType>::Pointer maskWriter = 
    itk::ImageFileWriter<LabelImageType>::New();
  maskWriter->SetInput( outputlabelImage ); 
  maskWriter->SetFileName( fmImageFilename );
  maskWriter->Update();
  maskWriter = 0;

  return outputlabelImage;
}




// Use fast marching to initialize the level-set segmentation process.
LabelImageType::Pointer
PQCT_Analyzer::
InitializeROIbyFastMarching(FloatImageType::Pointer speedImage,
			    LabelImageType::IndexType medianIdx,
			    float fastmarchingStoppingTime) 
{
  // Use fast marching to initialize the segmentation process.

  // Declare fast marching filter type.
  typedef itk::FastMarchingImageFilter< FloatImageType,
    FloatImageType >    
    FastMarchingFilterType;
  FastMarchingFilterType::Pointer fastMarching = 
    FastMarchingFilterType::New();

  // Declare the node containers for initialization.
  typedef FastMarchingFilterType::NodeContainer        NodeContainer;
  typedef FastMarchingFilterType::NodeType             NodeType;
  NodeContainer::Pointer seeds = NodeContainer::New();
  NodeType node;
  // Set the seed value and index.
  //   const double seedValue = (double) (-1)* this->m_seedbasedROI.GetSize()[0];
  const double seedValue = 0.0;
  node.SetValue( seedValue );
  FloatImageType::IndexType seedIndex = medianIdx;
  // speedImage->TransformPhysicalPointToIndex(medianIdx, 
  // 					    seedIndex);
  node.SetIndex( seedIndex );

  // Add index to container.
  seeds->Initialize();
  seeds->InsertElement( 0, node );

  // Set the fast marching filter attributes.
  std::cout << "FM stop time: " 
	    << fastmarchingStoppingTime
	    << std::endl;
  fastMarching->SetTrialPoints( seeds );
  fastMarching->SetInput( speedImage );
  // Set stopping time for fast marching.
  fastMarching->SetStoppingValue( fastmarchingStoppingTime );
  fastMarching->SetOutputSize( speedImage->GetBufferedRegion().GetSize() );
  fastMarching->Update();

  // // Write final level set to file.
  // itk::ImageFileWriter<FloatImageType>::Pointer floatWriter = 
  //   itk::ImageFileWriter<FloatImageType>::New();
  // std::string fmoutputFilename = this->m_outputPath +
  //   this->m_SubjectID + "_" + "fmoutput.nii";
  // floatWriter = 
  //   itk::ImageFileWriter<FloatImageType>::New();
  // floatWriter->SetInput( fastMarching->GetOutput() ); 
  // floatWriter->SetFileName( fmoutputFilename );
  // floatWriter->Update();
  // floatWriter = 0; 

  // Apply thresholding.
  typedef itk::BinaryThresholdImageFilter< FloatImageType,
    LabelImageType >    ThresholdingFilterType;
  ThresholdingFilterType::Pointer thresholder = ThresholdingFilterType::New();
  thresholder->SetLowerThreshold( 0.0 );
  thresholder->SetUpperThreshold( fastmarchingStoppingTime );
  thresholder->SetOutsideValue( BACKGROUND );
  thresholder->SetInsideValue( FOREGROUND );
  thresholder->SetInput( fastMarching->GetOutput() );
  thresholder->Update();

  // std::string fmImageFilename = this->m_outputPath + 
  //   this->m_SubjectID + "_"  + 
  //   "FM_Mask.nii";
  // itk::ImageFileWriter<LabelImageType>::Pointer maskWriter = 
  //   itk::ImageFileWriter<LabelImageType>::New();
  // maskWriter->SetInput( thresholder->GetOutput() ); 
  // maskWriter->SetFileName( fmImageFilename );
  // maskWriter->Update();
  // maskWriter = 0;

  return thresholder->GetOutput();
}


//! Use GAC algorithms for segmentation.
LabelImageType::Pointer
PQCT_Analyzer::
ApplyGeodesicActiveContoursToLabelImage(LabelImageType::Pointer roiVolume,
					FloatImageType::Pointer speedImage,
					unsigned int label) {

  // Calculate distance map from initial ROI.
  // Distance map filter type definition.
  FloatImageType::Pointer ROIDistanceOutput;
  typedef itk::SignedMaurerDistanceMapImageFilter<LabelImageType, 
    FloatImageType> 
    LabelDistanceTransformType;
  LabelDistanceTransformType::Pointer ROIDistFilter = 
    LabelDistanceTransformType::New();
  ROIDistFilter->SetInput( roiVolume );
  ROIDistFilter->SetUseImageSpacing( true );
  ROIDistFilter->SetSquaredDistance( true );
  // ROIDistFilter->SetInsideIsPositive( true );
  ROIDistFilter->Update();
  ROIDistanceOutput = ROIDistFilter->GetOutput();
  // ROIDistFilter = 0;

  // // Write final level set to file.
  // std::string distancemapFilename = this->m_outputPath +
  //   this->m_SubjectID + "_" + "distancemap.nii";
  // floatWriter = 
  //   itk::ImageFileWriter<FloatImageType>::New();
  // floatWriter->SetInput( ROIDistanceOutput ); 
  // floatWriter->SetFileName( distancemapFilename );
  // floatWriter->Update();
  // floatWriter = 0; 

  //! GAC level sets.
  typedef itk::GeodesicActiveContourLevelSetImageFilter< FloatImageType, 
    FloatImageType > 
    GeodesicActiveContourFilterType;

  GeodesicActiveContourFilterType::Pointer geodesicActiveContours = 
    GeodesicActiveContourFilterType::New();

  geodesicActiveContours->SetMaximumRMSError( this->m_levelsetMaximumRMSError );
  geodesicActiveContours->SetNumberOfIterations( (int) this->m_levelsetMaximumIterations );
  geodesicActiveContours->SetPropagationScaling(  this->m_levelsetPropagationScalingFactor );
  geodesicActiveContours->SetCurvatureScaling( this->m_levelsetCurvatureScalingFactor ); 
  geodesicActiveContours->SetAdvectionScaling( this->m_levelsetAdvectionScalingFactor );
  geodesicActiveContours->SetIsoSurfaceValue( 0.0 );
  geodesicActiveContours->SetInput( ROIDistanceOutput );
  geodesicActiveContours->SetFeatureImage( speedImage );  // may use other speed volume modified by the distance.
  geodesicActiveContours->Update();

  std::cout << "Geodesic active contours." 
	    << std::endl
	    << "Propagation factor = "
	    << this->m_levelsetPropagationScalingFactor
	    << ", Curvature factor = "
	    << this->m_levelsetCurvatureScalingFactor
	    << ", Advection factor = "
	    << this->m_levelsetAdvectionScalingFactor
	    << std::endl;

  FloatImageType::Pointer levelSetOutputImage = 
    geodesicActiveContours->GetOutput();

  // Print out some useful information.
  std::cout << "Level set segmentation completed." << std::endl;
  std::cout << "Max. no. iterations: " << geodesicActiveContours->GetNumberOfIterations() << "," <<
    "Max. RMS error: " << geodesicActiveContours->GetMaximumRMSError() << std::endl;
  std::cout << "# of iterations: " << geodesicActiveContours->GetElapsedIterations() << ","
	    << "RMS change: " << geodesicActiveContours->GetRMSChange() << std::endl;

  // Apply thresholding to the level sets
  // to get the segmentation result.
  typedef itk::BinaryThresholdImageFilter<FloatImageType, LabelImageType> 
    ThresholdingFilterType;
  ThresholdingFilterType::Pointer thresholder = ThresholdingFilterType::New();
  thresholder->SetInput( levelSetOutputImage );
  thresholder->SetOutsideValue(  BACKGROUND  );
  thresholder->SetInsideValue( label );
  thresholder->SetLowerThreshold( -1000.0 );
  thresholder->SetUpperThreshold(     0.0 );
  thresholder->Update();
  
  std::cout << "Level set-based segmentation, done." 
	    << std::endl;

  return thresholder->GetOutput();

}


//! Use GAC to segment bone at 4%.
LabelImageType::Pointer PQCT_Analyzer::
SegmentbyLevelSets( LabelImageType::IndexType medianIdx,
		    unsigned int label) {

  //! Apply denoising.
  FloatImageType::Pointer smoothedImage = 
    this->SmoothInputVolume( this->m_PQCTImage, 
			     DIFFUSION );


  //! Image gradient magnitude.
  typedef   itk::GradientMagnitudeRecursiveGaussianImageFilter<FloatImageType, 
    FloatImageType >  GradientFilterType;

  FloatImageType::Pointer gradientImage = 0; 
  GradientFilterType::Pointer  gradientMagnitude = GradientFilterType::New();
  gradientMagnitude->SetSigma( this->m_gradientSigma ); // 0.0625
  gradientMagnitude->SetInput( smoothedImage ); // originally: this->m_PQCTImage
  gradientMagnitude->Update();
  gradientImage = gradientMagnitude->GetOutput();

  // // Write gradient image to file.
  // std::string gradientImageFilename = this->m_outputPath +
  //   this->m_SubjectID + "_" + "gradientmag.nii";
  // itk::ImageFileWriter<FloatImageType>::Pointer floatWriter = 
  //   itk::ImageFileWriter<FloatImageType>::New();
  // floatWriter->SetInput( gradientImage ); 
  // floatWriter->SetFileName( gradientImageFilename );
  // floatWriter->Update();
  // floatWriter = 0; 

  //! Then apply sigmoid filter to produce edge potential image.
  // std::cout << "Sigmoid Filter-- Alpha = "
  // 	    << this->m_sigmoidAlpha
  // 	    << ", Beta = "
  // 	    << this->m_sigmoidBeta
  // 	    << std::endl;

  typedef   itk::SigmoidImageFilter<FloatImageType, FloatImageType >  SigmoidFilterType;
  SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();
  sigmoid->SetAlpha( this->m_sigmoidAlpha );   
  sigmoid->SetBeta( this->m_sigmoidBeta  );  
  sigmoid->SetOutputMinimum(  0.0  );
  sigmoid->SetOutputMaximum(  1.0  );
  sigmoid->SetInput( gradientImage ); 
  sigmoid->Update();


  //! Fast marching.
  // Use fast marching to initialize the segmentation process.
  LabelImageType::Pointer roiVolume = 
    this->InitializeROIbyFastMarching( sigmoid->GetOutput(),
				       medianIdx,
				       this->m_fastmarchingStoppingTime );
  std::cout << "FM-based ROI generation." << std::endl;


  //! GAC segmentation.
  // this->m_TissueLabelImage = 
  return this->ApplyGeodesicActiveContoursToLabelImage( roiVolume,
							sigmoid->GetOutput(),
							(unsigned int) label );
}


//! Use GAC to segment SUB fat from IM fat.
LabelImageType::Pointer PQCT_Analyzer::
SegmentbyLevelSets( LabelImageType::Pointer roiVolume,
		    unsigned int label) {
  //! Apply denoising.
  FloatImageType::Pointer smoothedImage = 
    this->SmoothInputVolume( this->m_PQCTImage, 
			     DIFFUSION );


  //! Image gradient magnitude.
  typedef   itk::GradientMagnitudeRecursiveGaussianImageFilter<FloatImageType, 
    FloatImageType >  GradientFilterType;

  FloatImageType::Pointer gradientImage = 0; 
  GradientFilterType::Pointer  gradientMagnitude = GradientFilterType::New();
  gradientMagnitude->SetSigma( this->m_gradientSigma ); // 0.0625
  gradientMagnitude->SetInput( smoothedImage ); // originally: this->m_PQCTImage
  gradientMagnitude->Update();
  gradientImage = gradientMagnitude->GetOutput();

  // // Write gradient image to file.
  // std::string gradientImageFilename = this->m_outputPath +
  //   this->m_SubjectID + "_" + "gradientmag.nii";
  // itk::ImageFileWriter<FloatImageType>::Pointer floatWriter = 
  //   itk::ImageFileWriter<FloatImageType>::New();
  // floatWriter->SetInput( gradientImage ); 
  // floatWriter->SetFileName( gradientImageFilename );
  // floatWriter->Update();
  // floatWriter = 0; 

  //! Then apply sigmoid filter to produce edge potential image.
  // std::cout << "Sigmoid Filter-- Alpha = "
  // 	    << this->m_sigmoidAlpha
  // 	    << ", Beta = "
  // 	    << this->m_sigmoidBeta
  // 	    << std::endl;

  typedef   itk::SigmoidImageFilter<FloatImageType, FloatImageType >  SigmoidFilterType;
  SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();
  sigmoid->SetAlpha( this->m_sigmoidAlpha );   
  sigmoid->SetBeta( this->m_sigmoidBeta  );  
  sigmoid->SetOutputMinimum(  0.0  );
  sigmoid->SetOutputMaximum(  1.0  );
  sigmoid->SetInput( gradientImage ); 
  sigmoid->Update();


  //! GAC segmentation.
  // this->m_TissueLabelImage = 
  return  this->ApplyGeodesicActiveContoursToLabelImage( roiVolume,
							 sigmoid->GetOutput(),
							 (unsigned int) label );
}


//! Overlay images for visualization.
LabelImageType::Pointer PQCT_Analyzer::OverlayLabelImages(LabelImageType::Pointer image1,
							  LabelImageType::Pointer image2)
{
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetBufferedRegion());
  LabelImageType::IndexType idx;

  LabelImageType::Pointer outputlabelImage = LabelImageType::New();
  outputlabelImage->CopyInformation( this->m_TissueLabelImage );
  outputlabelImage->SetRegions( this->m_TissueLabelImage->GetLargestPossibleRegion() );
  outputlabelImage->Allocate();
  outputlabelImage->FillBuffer( BACKGROUND );

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    idx = itImage.GetIndex();
    if(image1->GetPixel(idx) > image2->GetPixel(idx))
      outputlabelImage->SetPixel(idx, image1->GetPixel(idx));
    else
      outputlabelImage->SetPixel(idx, image2->GetPixel(idx));
  }
  
  return outputlabelImage;
}


//! Map classes of tissues according to our convention.
void PQCT_Analyzer::MapTissueClassesPostKMeans(){

  //! Map class labels according to anatomical site.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_KmeansLabelImage, 
				 this->m_KmeansLabelImage->GetBufferedRegion());
  LabelImageType::IndexType idx;

  switch(this->m_WorkflowID) {
  case PQCT_FOUR_PCT_TIBIA://! 4%
    break;
  case PQCT_THIRTYEIGHT_PCT_TIBIA: case PQCT_SIXTYSIX_PCT_TIBIA: case CT_MID_THIGH: 
    //! 38% and 66% tibia and mid thigh ct.
    for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
      idx = itImage.GetIndex();
      if( itImage.Get() == TRAB_BONE )
	itImage.Set( CORT_BONE ); 
    }
    break;
  default:
    std::cerr << "Unacceptable workflow number." 
	      << std::endl;
    exit(1);
    break;
  }
}


//! Set HU prior intensities according to workflow.
void PQCT_Analyzer::SetTissueClassesNoAir() {
  //! Pick classes according to anatomical site.
  switch(this->m_WorkflowID) {
  case PQCT_FOUR_PCT_TIBIA://! 4%
    //! Materials are identified according to HU.
    this->m_TissueClassesVectorNoAir.push_back ( (float) -22.0 ); // FAT
    this->m_TissueClassesVectorNoAir.push_back ( (float) 72.0 ); // MUSCLE
    this->m_TissueClassesVectorNoAir.push_back ( (float) 200.0 ); // TRAB. BONE
    this->m_TissueClassesVectorNoAir.push_back ( (float) 500.0 ); // CORT. BONE
    this->m_TissueClassesVectorNoAir.push_back ( (float) 750.0 ); // HYPER CORT. BONE
    break;
  case PQCT_THIRTYEIGHT_PCT_TIBIA: case PQCT_SIXTYSIX_PCT_TIBIA://! 38% and 66%
    //! Materials are identified according to HU.
    this->m_TissueClassesVectorNoAir.push_back ( (float) -22.0 ); // FAT
    this->m_TissueClassesVectorNoAir.push_back ( (float) 72.0 ); // MUSCLE
    this->m_TissueClassesVectorNoAir.push_back ( (float) 514.0 ); // TRAB. TIBIA
    this->m_TissueClassesVectorNoAir.push_back ( (float) 993.0 ); // CORT. TIBIA
    break;
  case CT_MID_THIGH://! Mid thigh region in CT.
    //! Materials are identified according to HU.
    this->m_TissueClassesVectorNoAir.push_back ( (float) -20.0 ); // FAT
    this->m_TissueClassesVectorNoAir.push_back ( (float) 50.0 ); // MUSCLE
    this->m_TissueClassesVectorNoAir.push_back ( (float) 700 ); // FEMUR 1
    this->m_TissueClassesVectorNoAir.push_back ( (float) 1200.0 ); // FEMUR 2
    break;
  default:
    std::cerr << "Unacceptable workflow number." 
	      << std::endl;
    exit(1);
    break;
  }

}


//! Set HU prior intensities according to workflow.
void PQCT_Analyzer::SetTissueClasses() {
  //! Pick classes according to anatomical site.
  switch(this->m_WorkflowID) {
  case PQCT_FOUR_PCT_TIBIA://! 4%
    //! Materials are identified according to HU.
    this->m_TissueClassesVector.push_back ( (float) -400.0 ); // AIR
    this->m_TissueClassesVector.push_back ( (float) -22.0 ); // FAT
    this->m_TissueClassesVector.push_back ( (float) 72.0 ); // MUSCLE
    this->m_TissueClassesVector.push_back ( (float) 200.0 ); // TRAB. BONE 180.0
    this->m_TissueClassesVector.push_back ( (float) 500.0 ); // CORT. BONE 250.0 
    this->m_TissueClassesVector.push_back ( (float) 750.0 ); // HYPER CORT. BONE
    break;
  case PQCT_THIRTYEIGHT_PCT_TIBIA: case PQCT_SIXTYSIX_PCT_TIBIA://! 38% and 66%
    //! Materials are identified according to HU.
    this->m_TissueClassesVector.push_back ( (float) -400.0 ); // AIR
    this->m_TissueClassesVector.push_back ( (float) -22.0 ); // FAT
    this->m_TissueClassesVector.push_back ( (float) 72.0 ); // MUSCLE
    this->m_TissueClassesVector.push_back ( (float) 514.0 ); // TRAB. TIBIA
    this->m_TissueClassesVector.push_back ( (float) 993.0 ); // CORT. TIBIA
    break;
  case CT_MID_THIGH://! Mid thigh region in CT.
    //! Materials are identified according to HU.
    this->m_TissueClassesVector.push_back ( (float) -940.0 ); // AIR
    this->m_TissueClassesVector.push_back ( (float) -20.0 ); // FAT
    this->m_TissueClassesVector.push_back ( (float) 50.0 ); // MUSCLE
    this->m_TissueClassesVector.push_back ( (float) 700 ); // FEMUR 1
    this->m_TissueClassesVector.push_back ( (float) 1200.0 ); // FEMUR 2
    break;
  default:
    std::cerr << "Unacceptable workflow number." 
	      << std::endl;
    exit(1);
    break;
  }

}


//! Helper function that applies K-Means on the input samples
//! at different anatomical sites.
void PQCT_Analyzer::ApplyKMeans() {

  //! Apply denoising.
  FloatImageType::Pointer smoothedImage = 
    this->SmoothInputVolume( this->m_PQCTImage, 
			     MEDIAN );  // previously: DIFFUSION


  //! 1. k-means clustering into 4 groups {bone,fat,muscle,background}.
  typedef itk::ScalarImageKmeansImageFilter<FloatImageType> 
    ScalarImageKmeansImageFilterType;
  ScalarImageKmeansImageFilterType::Pointer scalarImageKmeansImageFilter = 
    ScalarImageKmeansImageFilterType::New();
  // scalarImageKmeansImageFilter->SetInput( this->m_PQCTImage );
  scalarImageKmeansImageFilter->SetInput( smoothedImage );
  scalarImageKmeansImageFilter->SetDebug( true );

  //! Use prior knowledge to initialize.
  this->SetTissueClasses();
  std::vector<float>::iterator it;
  for(it=this->m_TissueClassesVector.begin();it<this->m_TissueClassesVector.end();it++)
    scalarImageKmeansImageFilter->AddClassWithInitialMean( *it );
  scalarImageKmeansImageFilter->Update();
  this->m_KmeansLabelImage = scalarImageKmeansImageFilter->GetOutput();

  std::cout << "K-means clustering, done" << std::endl;
  std::cout << "Final means are: " << std::endl;
  std::cout << scalarImageKmeansImageFilter->GetFinalMeans() << std::endl;

  //! Map cluster numbers to tissue labels according to our convention.
  this->MapTissueClassesPostKMeans();

  //! In debug mode, write label image to file.
  itk::ImageFileWriter<LabelImageType>::Pointer labelWriter = 
    itk::ImageFileWriter<LabelImageType>::New();
  labelWriter->SetInput( this->m_KmeansLabelImage ); 
  labelWriter->SetFileName( this->m_outputPath + "Kmeans_output.nii" );
  labelWriter->Update();
  labelWriter = 0;

  //! Use duplicator to create the tissue label image.
  typedef itk::ImageDuplicator< LabelImageType > LabelDuplicatorType;
  LabelDuplicatorType::Pointer labelDuplicator = LabelDuplicatorType::New();
  labelDuplicator->SetInputImage( scalarImageKmeansImageFilter->GetOutput()  );
  labelDuplicator->Update();
  this->m_TissueLabelImage = labelDuplicator->GetOutput();
}


//! Identify bone marrow and add to label image.
void PQCT_Analyzer::IdentifyBoneMarrow() {

  //! Apply thresholding to create a bone mask.
  typedef itk::BinaryThresholdImageFilter<LabelImageType, 
    LabelImageType> 
    SelectBoneRegionFilterType;
  SelectBoneRegionFilterType::Pointer boneRegionThresholdFilter = 
    SelectBoneRegionFilterType::New();
  boneRegionThresholdFilter->SetInput( this->m_KmeansLabelImage );
  boneRegionThresholdFilter->SetInsideValue( 1 );
  boneRegionThresholdFilter->SetOutsideValue( 0 );
  boneRegionThresholdFilter->SetLowerThreshold( CORT_BONE );
  boneRegionThresholdFilter->SetUpperThreshold( CORT_BONE );  // number of regions we want detected.
  boneRegionThresholdFilter->Update();

  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage2(this->m_TissueLabelImage, 
				  this->m_TissueLabelImage->GetBufferedRegion());
  LabelImageType::IndexType idx;

  //! Label any fat inside the tibia and fibula
  //! by hole filling.
  typedef itk::GrayscaleFillholeImageFilter<LabelImageType,LabelImageType> 
    BinaryHoleFillingFilterType;

  BinaryHoleFillingFilterType::Pointer binaryHoleFillingFilter =
    BinaryHoleFillingFilterType::New();
  binaryHoleFillingFilter->SetInput( boneRegionThresholdFilter->GetOutput() );
  binaryHoleFillingFilter->Update();
   
  // itk::ImageFileWriter<LabelImageType>::Pointer labelWriter = 
  //   itk::ImageFileWriter<LabelImageType>::New();
  // labelWriter->SetInput( binaryHoleFillingFilter->GetOutput() ); 
  //   labelWriter->SetFileName( this->m_outputPath + "HoleFilling_output.nii" );
  // labelWriter->Update();
  // labelWriter = 0;

  for(itImage2.GoToBegin();!itImage2.IsAtEnd();++itImage2) {
    idx = itImage2.GetIndex();
    if( boneRegionThresholdFilter->GetOutput()->GetPixel(idx) == 0 &&
	binaryHoleFillingFilter->GetOutput()->GetPixel(idx) != 0 )
      itImage2.Set( BONE_INT );
  }


}


//! Identify tibia and fibula (remove fibula).
void PQCT_Analyzer::IdentifyTibiaAndFibula() {

  //! Apply thresholding to create a bone mask.
  typedef itk::BinaryThresholdImageFilter<LabelImageType, 
    LabelImageType> 
    SelectBoneRegionFilterType;
  SelectBoneRegionFilterType::Pointer boneRegionThresholdFilter = 
    SelectBoneRegionFilterType::New();
  boneRegionThresholdFilter->SetInput( this->m_TissueLabelImage );
  boneRegionThresholdFilter->SetInsideValue( 1 );
  boneRegionThresholdFilter->SetOutsideValue( 0 );
  boneRegionThresholdFilter->SetLowerThreshold( CORT_BONE );
  boneRegionThresholdFilter->SetUpperThreshold( BONE_INT );  // number of regions we want detected.
  boneRegionThresholdFilter->Update();

  //! Relabel and compute areas.
  //! The larger bone is tibia and the smaller is fibula.
  //! Apply connected component labeling on bone.
  typedef itk::ConnectedComponentImageFilter<LabelImageType, 
    LabelImageType, 
    LabelImageType> 
    ConnectedComponentLabelFilterType;
  ConnectedComponentLabelFilterType::Pointer labelMaskFilter2 = 
    ConnectedComponentLabelFilterType::New();
  labelMaskFilter2->SetInput( boneRegionThresholdFilter->GetOutput() );
  labelMaskFilter2->SetMaskImage( boneRegionThresholdFilter->GetOutput() );
  labelMaskFilter2->SetFullyConnected( true );
  labelMaskFilter2->Update();

  // Rank components wrt to size and relabel.
  typedef itk::RelabelComponentImageFilter<LabelImageType, 
    LabelImageType> 
    RelabelFilterType;
  RelabelFilterType::Pointer  sortLabelsImageFilter2 = 
    RelabelFilterType::New();
  sortLabelsImageFilter2->SetInput( labelMaskFilter2->GetOutput() );
  sortLabelsImageFilter2->Update();

  // // Write image to check intermediate results.
  // itk::ImageFileWriter<LabelImageType>::Pointer labelWriter = 
  //   itk::ImageFileWriter<LabelImageType>::New();
  // labelWriter->SetInput( sortLabelsImageFilter2->GetOutput() ); 
  //   labelWriter->SetFileName( this->m_outputPath + "ConnectedComponents.nii" );
  // labelWriter->Update();
  // labelWriter = 0;

  //! Update tissue labels.
  //! Label tibia and fibula pixels.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage2(this->m_TissueLabelImage, 
				  this->m_TissueLabelImage->GetBufferedRegion());
  LabelImageType::IndexType idx;
  for(itImage2.GoToBegin();!itImage2.IsAtEnd();++itImage2) {
    idx = itImage2.GetIndex();
    if( sortLabelsImageFilter2->GetOutput()->GetPixel(idx) > 1 )
      itImage2.Set( AIR );  // previously: FIBULA.
  }
 
}


//! Log some header info useful for the study.
void PQCT_Analyzer::LogHeaderInfo() {
  // Set floating point precision.
  std::cout.setf(std::ios::fixed, std::ios::floatfield);
  std::cout.precision(FLOAT_PRECISION);
  this->m_TissueShapeEntries.valueString.setf(std::ios::fixed, std::ios::floatfield);
  this->m_TissueShapeEntries.valueString.precision(FLOAT_PRECISION);

  //! Add subject ID.
  this->m_TissueShapeEntries.headerString.width(STRING_LENGTH_HEADER_TAGS);
  this->m_TissueShapeEntries.valueString.width(STRING_LENGTH_HEADER_TAGS);    
  this->m_TissueShapeEntries.headerString << "Subject_ID";
  this->m_TissueShapeEntries.valueString  << this->m_SubjectID;

  //! Add subject number.
  this->m_TissueShapeEntries.headerString.width(STRING_LENGTH_HEADER_TAGS);
  this->m_TissueShapeEntries.valueString.width(STRING_LENGTH_HEADER_TAGS);    
  this->m_TissueShapeEntries.headerString << "Subject_#";
  this->m_TissueShapeEntries.valueString  << this->m_PatientInformation.PatientNumber;

  //! Subject name.
  this->m_TissueShapeEntries.headerString.width(STRING_LENGTH);
  this->m_TissueShapeEntries.valueString.width(STRING_LENGTH);    
  this->m_TissueShapeEntries.headerString << "Subject_Name";
  this->m_TissueShapeEntries.valueString  << this->m_PatientInformation.PatientName;

  // //! Subject gender.
  // this->m_TissueShapeEntries.headerString.width(STRING_LENGTH_HEADER_TAGS);
  // this->m_TissueShapeEntries.valueString.width(STRING_LENGTH_HEADER_TAGS);    
  // this->m_TissueShapeEntries.headerString << "Gender";
  // this->m_TissueShapeEntries.valueString  << this->m_PatientInformation.PatientGender;

  // //! Subject measurement number.
  // this->m_TissueShapeEntries.headerString.width(STRING_LENGTH_HEADER_TAGS);
  // this->m_TissueShapeEntries.valueString.width(STRING_LENGTH_HEADER_TAGS);    
  // this->m_TissueShapeEntries.headerString << "Measurement #";
  // this->m_TissueShapeEntries.valueString  <<  this->m_PatientInformation.PatientMeasurementNumber;

  //! Subject birthdate.
  this->m_TissueShapeEntries.headerString.width(STRING_LENGTH_HEADER_TAGS);
  this->m_TissueShapeEntries.valueString.width(STRING_LENGTH_HEADER_TAGS);    
  this->m_TissueShapeEntries.headerString << "Birthdate";
  this->m_TissueShapeEntries.valueString  <<  this->m_PatientInformation.PatientBirthDate;
  
  //! Image origin.
  this->m_TissueShapeEntries.headerString.width(STRING_LENGTH_HEADER_TAGS);
  this->m_TissueShapeEntries.valueString.width(STRING_LENGTH_HEADER_TAGS);    
  this->m_TissueShapeEntries.headerString << "Image_Origin";
  this->m_TissueShapeEntries.valueString  <<  this->m_DetectorInformation.SliceOrigin;
  
  //! Image Scan date.
  this->m_TissueShapeEntries.headerString.width(STRING_LENGTH_HEADER_TAGS);
  this->m_TissueShapeEntries.valueString.width(STRING_LENGTH_HEADER_TAGS);    
  this->m_TissueShapeEntries.headerString << "Scan_Date";
  this->m_TissueShapeEntries.valueString  <<  this->m_DetectorInformation.ScanDate;

  //! Anatomical site.
  this->m_TissueShapeEntries.headerString.width(STRING_LENGTH_HEADER_TAGS);
  this->m_TissueShapeEntries.valueString.width(STRING_LENGTH_HEADER_TAGS);    
  this->m_TissueShapeEntries.headerString << "Tibia_Site";
  this->m_TissueShapeEntries.valueString  <<  AnatomicalSite[this->m_WorkflowID];

}


//! Compute tissue areas, centroids, etc. 
void  PQCT_Analyzer::ComputeTissueShapeAttributes(LabelImageType::Pointer labelImage) {

  // // Rank components wrt to size and relabel.
  // typedef itk::RelabelComponentImageFilter<LabelImageType, 
  //   LabelImageType> 
  //   RelabelFilterType;
  // RelabelFilterType::Pointer  sortLabelsImageFilter = 
  //   RelabelFilterType::New();
  // sortLabelsImageFilter->SetInput( this->m_TissueLabelImage );
  // sortLabelsImageFilter->Update();
  
  //! Use statistical label map objects for each tissue type.
  typedef unsigned long LabelType;
  typedef itk::ShapeLabelObject< LabelType, pixelDimensions > LabelObjectType;
  typedef itk::LabelMap< LabelObjectType > LabelMapType;

  //! Shape attributes.
  //! Convert label image to shape label map.
  typedef itk::LabelImageToShapeLabelMapFilter<LabelImageType,LabelMapType> 
    LabelImageToShapeLabelMapFilterType;
  LabelImageToShapeLabelMapFilterType::Pointer labelImageToShapeLabelMapFilter = 
    LabelImageToShapeLabelMapFilterType::New();
  labelImageToShapeLabelMapFilter->SetInput( labelImage );
  labelImageToShapeLabelMapFilter->SetBackgroundValue( AIR );
  labelImageToShapeLabelMapFilter->Update();

  //! Dsiplay computed shape attributes.
  // Get label map.
  LabelMapType::Pointer labelMap = labelImageToShapeLabelMapFilter->GetOutput();
  // std::cout << "Number of tissue types:" << labelMap->GetNumberOfLabelObjects() << std::endl;
  // this->m_textFile << "Number of tissue types:" << labelMap->GetNumberOfLabelObjects() << std::endl;

  // Set up iterator and label object container.
  // LabelMapType::LabelObjectContainerType::const_iterator it;
  // const LabelMapType::LabelObjectContainerType & labelObjectContainer = 
  //  labelMap->GetLabelObjectContainer();

  // Pass quantification parameter names and values.

  // Set floating point precision.
  std::cout.setf(std::ios::fixed, std::ios::floatfield);
  std::cout.precision(FLOAT_PRECISION);
  this->m_TissueShapeEntries.valueString.setf(std::ios::fixed, std::ios::floatfield);
  this->m_TissueShapeEntries.valueString.precision(FLOAT_PRECISION);
  

  // Display headers in standard output and write to text file.
  std::cout << "Tissue type\t\tArea (mm^2)\tCentroid coordinates" << std::endl;

  // for( it = labelObjectContainer.begin(); it != labelObjectContainer.end(); it++ )
  std::stringstream tempStringstream;
  for(unsigned int i = 0; i < labelMap->GetNumberOfLabelObjects(); i++)
    {
      // const LabelType & label = it->first;
      // LabelObjectType * labelObject = it->second;
      LabelObjectType * labelObject = labelMap->GetNthLabelObject(i);
      const LabelType & label = labelObject->GetLabel();

      std::cout << label << " [" << TissueTypeString[label] << "]" << "\t\t" 
		<< labelObject->GetPhysicalSize() << "\t\t" 
		<< labelObject->GetCentroid() << std::endl;

      this->m_TissueShapeEntries.headerString.width(STRING_LENGTH);
      this->m_TissueShapeEntries.valueString.width(STRING_LENGTH);    
      tempStringstream.str("");
      tempStringstream <<  label << "-" << TissueTypeString[label] << "[Area(mm^2)]";
      this->m_TissueShapeEntries.headerString << tempStringstream.str();
      this->m_TissueShapeEntries.valueString << labelObject->GetPhysicalSize();

      this->m_TissueShapeEntries.headerString.width(STRING_LENGTH);
      this->m_TissueShapeEntries.valueString.width(STRING_LENGTH);    
      tempStringstream.str("");
      tempStringstream <<  label << "-" << TissueTypeString[label] << "[Princ.Mom.1]";
      this->m_TissueShapeEntries.headerString << tempStringstream.str();
      this->m_TissueShapeEntries.valueString << labelObject->GetPrincipalMoments()[0];

      this->m_TissueShapeEntries.headerString.width(STRING_LENGTH);
      this->m_TissueShapeEntries.valueString.width(STRING_LENGTH);    
      tempStringstream.str("");
      tempStringstream <<  label << "-" << TissueTypeString[label] << "[Princ.Mom.2]";
      this->m_TissueShapeEntries.headerString << tempStringstream.str();
      this->m_TissueShapeEntries.valueString << labelObject->GetPrincipalMoments()[1];

      this->m_TissueShapeEntries.headerString.width(STRING_LENGTH);
      this->m_TissueShapeEntries.valueString.width(STRING_LENGTH);    
      tempStringstream.str("");
      tempStringstream <<  label << "-" << TissueTypeString[label] << "[Eq.Radius]";
      this->m_TissueShapeEntries.headerString << tempStringstream.str();
      this->m_TissueShapeEntries.valueString << labelObject->GetEquivalentSphericalRadius();
    }

  // Add end-of-line character.
  // this->m_textFile << std::endl;

}


//! Overloaded version.
void  PQCT_Analyzer::ComputeTissueShapeAttributes() {
  this->ComputeTissueShapeAttributes(this->m_TissueLabelImage);
}


//! Compute tissue means, std. devs, etc 
void  PQCT_Analyzer::ComputeTissueIntensityAttributes(LabelImageType::Pointer labelImage) {

  //! Use statistical label map objects for each tissue type.
  typedef unsigned long LabelType;
  const int dim = 2;
  typedef itk::StatisticsLabelObject< LabelType, dim > LabelObjectType;
  typedef itk::LabelMap< LabelObjectType > LabelMapType;

  // Intensity attributes.
  typedef itk::LabelImageToStatisticsLabelMapFilter<LabelImageType, PQCTImageType, LabelMapType>
    LabelImageToStatisticsLabelMapFilterType;
  LabelImageToStatisticsLabelMapFilterType::Pointer labelImageToStatisticsLabelMapFilter = 
    LabelImageToStatisticsLabelMapFilterType::New();
  labelImageToStatisticsLabelMapFilter->SetInput( labelImage );
  labelImageToStatisticsLabelMapFilter->SetFeatureImage( this->m_PQCTImage );
  labelImageToStatisticsLabelMapFilter->SetBackgroundValue( AIR );
  labelImageToStatisticsLabelMapFilter->Update();

  //! Display computed intensity attributes.

  // Get label map.
  LabelMapType::Pointer labelMap = labelImageToStatisticsLabelMapFilter->GetOutput();

  // Set up iterator and label object container.
  // LabelMapType::LabelObjectContainerType::const_iterator it;
  // const LabelMapType::LabelObjectContainerType & labelObjectContainer = 
  //   labelMap->GetLabelObjectContainer();
 
  // Pass quantification parameter names and values.
  // Display headers in standard output and write to text file.
  std::cout << "Tissue type\t\tDensity mean\tDensity std.dev." << std::endl;
  // this->m_textFile << "Subject ID\tTissue type\tDensity mean\tDensity std. dev." << std::endl;
  // this->m_textFile << this->m_SubjectID << "\t";

  // Set floating point precision.
  std::cout.setf(std::ios::fixed, std::ios::floatfield);
  std::cout.precision(FLOAT_PRECISION);
  this->m_TissueIntensityEntries.valueString.setf(std::ios::fixed, std::ios::floatfield);
  this->m_TissueIntensityEntries.valueString.precision(FLOAT_PRECISION);
  this->m_TissueIntensityEntries.headerString.width(STRING_LENGTH);
  this->m_TissueIntensityEntries.valueString.width(STRING_LENGTH);    

  // for( it = labelObjectContainer.begin(); it != labelObjectContainer.end(); it++ )
  std::stringstream tempStringstream;
  for(unsigned int i = 0; i < labelMap->GetNumberOfLabelObjects(); i++)
    {
      // const LabelType & label = it->first;
      // LabelObjectType * labelObject = it->second;
      LabelObjectType * labelObject = labelMap->GetNthLabelObject(i);
      const LabelType & label = labelObject->GetLabel();

      std::cout << label << " [" << TissueTypeString[label] << "]" << "\t\t" 
  		<< labelObject->GetMean() << "\t\t" 
		<< labelObject->GetStandardDeviation() << std::endl;

      tempStringstream.str("");
      tempStringstream <<  label << "-" << TissueTypeString[label] << "[Den.M.]";
      this->m_TissueIntensityEntries.headerString.width(STRING_LENGTH);
      this->m_TissueIntensityEntries.headerString << tempStringstream.str();

      tempStringstream.str("");
      tempStringstream <<  label << "-" << TissueTypeString[label] << "[Den.SD.]";
      this->m_TissueIntensityEntries.headerString.width(STRING_LENGTH);
      this->m_TissueIntensityEntries.headerString << tempStringstream.str();

      this->m_TissueIntensityEntries.valueString.width(STRING_LENGTH); 
      this->m_TissueIntensityEntries.valueString << labelObject->GetMean();
      this->m_TissueIntensityEntries.valueString.width(STRING_LENGTH); 
      this->m_TissueIntensityEntries.valueString << labelObject->GetStandardDeviation();

    }

  // // Add end-of-line character.
  // this->m_textFile << std::endl;

}


//! Overloaded version.
void  PQCT_Analyzer::ComputeTissueIntensityAttributes() {
  this->ComputeTissueIntensityAttributes(this->m_TissueLabelImage);
}


