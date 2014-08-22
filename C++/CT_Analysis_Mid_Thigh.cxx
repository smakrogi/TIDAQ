/*===========================================================================

  Program:   Bone, muscle and fat quantification from PQCT data.
  Module:    $RCSfile: CT_Analysis_MidThigh.cxx,v $
  Language:  C++
  Date:      $Date: 2012/07/10 14:29:00 $
  Version:   $Revision: 0.1 $
  Author:    S. K. Makrogiannis
  3T MRI Facility National Institute on Aging/National Institutes of Health.

  =============================================================================*/

#include <itkBinaryThresholdImageFilter.h>
#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryMorphologicalClosingImageFilter.h>
// #include <itkGrayscaleFillholeImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkShapeLabelObject.h>
#include <itkLabelMap.h>
#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkLabelImageToStatisticsLabelMapFilter.h>
#include <itkExtractImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageFileWriter.h>
#include <vector>
#include <algorithm>

#include <itkDecisionRule.h>
#include <itkVector.h>
#include <itkListSample.h>
#include <itkKdTree.h>
#include <itkWeightedCentroidKdTreeGenerator.h>
#include <itkKdTreeBasedKmeansEstimator.h>
#include <itkMinimumDecisionRule.h>
#include <itkEuclideanDistanceMetric.h>
#include <itkDistanceToCentroidMembershipFunction.h>
#include <itkSampleClassifierFilter.h>


#include "PQCT_Datatypes.h"
#include "PQCT_Analysis.h"


//! Dilate subcutaneous fat towards muscle to accomodate error caused by
//! partial volume effect.
void PQCT_Analyzer::DilateSubcutaneousFatForPVECorrection(){

  //! Mask sub. fat region.
  typedef itk::BinaryThresholdImageFilter<LabelImageType, 
    LabelImageType> 
    SelectSubcutaneousFatFilterType;
  SelectSubcutaneousFatFilterType::Pointer selectsubcutaneousfatFilter = 
    SelectSubcutaneousFatFilterType::New();
  selectsubcutaneousfatFilter->SetInput( this->m_TissueLabelImage );
  selectsubcutaneousfatFilter->SetInsideValue( FOREGROUND );
  selectsubcutaneousfatFilter->SetOutsideValue( BACKGROUND );
  selectsubcutaneousfatFilter->SetLowerThreshold( SUB_FAT ); 
  selectsubcutaneousfatFilter->SetUpperThreshold( SUB_FAT ); 
  selectsubcutaneousfatFilter->Update();


  //! Generate structuring element.
  float structureElementRadius = 2.0F;
  typedef itk::BinaryBallStructuringElement<LabelPixelType, LabelImageType::ImageDimension> 
    StructuringElementType;
  StructuringElementType structuringElement;
  structuringElement.SetRadius( structureElementRadius );
  structuringElement.CreateStructuringElement();


  //! Dilate sub. fat mask.
  typedef itk::BinaryDilateImageFilter<LabelImageType,
    LabelImageType,
    StructuringElementType>
    BinaryDilationFilterType;
  BinaryDilationFilterType::Pointer binaryDilationFilter = 
    BinaryDilationFilterType::New();
  binaryDilationFilter->SetKernel( structuringElement );
  binaryDilationFilter->SetInput( selectsubcutaneousfatFilter->GetOutput() );
  binaryDilationFilter->SetForegroundValue( FOREGROUND );
  binaryDilationFilter->Update();
  

  //! Update label map while preserving the air voxels.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetBufferedRegion());
  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    LabelImageType::IndexType idx = itImage.GetIndex();
    if( binaryDilationFilter->GetOutput()->GetPixel(idx) == FOREGROUND &&
	itImage.Get() == IM_FAT )
      itImage.Set( SUB_FAT );
  }


}


//! Separate IMFAT from Muscle in CT using clustering.
void PQCT_Analyzer::SeparateIMFATFromMuscle() {

  //! Create vector of samples.
  typedef itk::Vector<PQCTPixelType, 1> MeasurementVectorType;
  typedef itk::Statistics::ListSample< MeasurementVectorType > SampleType;

  SampleType::Pointer samples = SampleType::New();
  MeasurementVectorType mv;

  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetLargestPossibleRegion());
  typedef LabelImageType::IndexType LabelImageIndexType;
  std::vector<LabelImageIndexType> sampleIndices;


  //! Samples are pixel intensities corresponding to 
  //! previously detected muscle and imfat pixels.
  //! For each pixel of muscle and imat
  //! add pixel index and intensity to corresponding vectors.
  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {

    if ( itImage.Get() == MUSCLE || itImage.Get() == IM_FAT) {
      sampleIndices.push_back(itImage.GetIndex());  
      samples->PushBack(this->m_PQCTImage->GetPixel(itImage.GetIndex()));
    }
  }
  
  
  //! Create a kd-tree.
  typedef itk::Statistics::WeightedCentroidKdTreeGenerator< SampleType >
    TreeGeneratorType;
  TreeGeneratorType::Pointer treeGenerator = TreeGeneratorType::New();
  
  treeGenerator->SetSample( samples );
  treeGenerator->SetBucketSize( 16 );
  treeGenerator->Update();
 
  //! Create and run kmeans estimator.
  typedef TreeGeneratorType::KdTreeType TreeType;
  typedef itk::Statistics::KdTreeBasedKmeansEstimator<TreeType> EstimatorType;
  EstimatorType::Pointer estimator = EstimatorType::New();
 
  EstimatorType::ParametersType initialMeans(2);
  initialMeans[0] = -20.0; // Cluster 1, mean[0]
  initialMeans[1] = 50.0; // Cluster 2, mean[0]

  estimator->SetParameters( initialMeans );
  estimator->SetKdTree( treeGenerator->GetOutput() );
  estimator->SetMaximumIteration( 200 );
  estimator->SetCentroidPositionChangesThreshold(0.0);
  estimator->StartOptimization();

  EstimatorType::ParametersType estimatedMeans = estimator->GetParameters();
 
  for ( unsigned int i = 0 ; i < 2 ; i++ )
    {
    std::cout << "cluster[" << i << "] " << std::endl;
    std::cout << "    estimated mean : " << estimatedMeans[i] << std::endl;
    }

  //! Create classifier.

  //! Membership function is distance to cluster's centroid.
  typedef itk::Statistics::DistanceToCentroidMembershipFunction< MeasurementVectorType >
    MembershipFunctionType;
  typedef MembershipFunctionType::Pointer                      MembershipFunctionPointer;

  //! Decision rule is minimum distance.
  typedef itk::Statistics::MinimumDecisionRule DecisionRuleType;
  DecisionRuleType::Pointer decisionRule = DecisionRuleType::New();

  //! Configure classifier.
  typedef itk::Statistics::SampleClassifierFilter< SampleType > ClassifierType;
  ClassifierType::Pointer classifier = ClassifierType::New();
 
  classifier->SetDecisionRule(decisionRule);
  classifier->SetInput( samples );
  classifier->SetNumberOfClasses( 2 );
 
  typedef ClassifierType::ClassLabelVectorObjectType               ClassLabelVectorObjectType;
  typedef ClassifierType::ClassLabelVectorType                     ClassLabelVectorType;
  typedef ClassifierType::MembershipFunctionVectorObjectType       MembershipFunctionVectorObjectType;
  typedef ClassifierType::MembershipFunctionVectorType             MembershipFunctionVectorType;
 
  ClassLabelVectorObjectType::Pointer  classLabelsObject = ClassLabelVectorObjectType::New();
  classifier->SetClassLabels( classLabelsObject );
 
  ClassLabelVectorType &  classLabelsVector = classLabelsObject->Get();
  classLabelsVector.push_back( IM_FAT );
  classLabelsVector.push_back( MUSCLE );

  MembershipFunctionVectorObjectType::Pointer membershipFunctionsObject =
    MembershipFunctionVectorObjectType::New();
  classifier->SetMembershipFunctions( membershipFunctionsObject );
 
  MembershipFunctionVectorType &  membershipFunctionsVector = membershipFunctionsObject->Get();
 
  MembershipFunctionType::CentroidType origin( samples->GetMeasurementVectorSize() );
  int index = 0;
  for ( unsigned int i = 0 ; i < 2 ; i++ )
    {
    MembershipFunctionPointer membershipFunction = MembershipFunctionType::New();
    for ( unsigned int j = 0 ; j < samples->GetMeasurementVectorSize(); j++ )
      {
      origin[j] = estimatedMeans[index++];
      }
    membershipFunction->SetCentroid( origin );
    membershipFunctionsVector.push_back( membershipFunction.GetPointer() );
    }

  //! Classify samples and set pixel values in label map.

  classifier->Update();
 
  const ClassifierType::MembershipSampleType* membershipSample = classifier->GetOutput();
  ClassifierType::MembershipSampleType::ConstIterator iter = membershipSample->Begin();
  unsigned int count;

  while ( iter != membershipSample->End() )
    {
    // std::cout << "measurement vector = " << iter.GetMeasurementVector()
    //           << "class label = " << iter.GetClassLabel()
    //           << std::endl;
    
    this->m_TissueLabelImage->SetPixel(sampleIndices[count], iter.GetClassLabel());
    ++count;
    ++iter;
    }

}


//! Remove patient table and select left thigh.
LabelImageType::Pointer PQCT_Analyzer::SelectOneLeg(){

  //! Apply foreground/background thresholding.
  typedef itk::BinaryThresholdImageFilter<PQCTImageType, 
    LabelImageType> 
    ForegroundBackgroundThresholdFilterType;
  ForegroundBackgroundThresholdFilterType::Pointer foregroundBackgroundThresholdFilter = 
    ForegroundBackgroundThresholdFilterType::New();
  foregroundBackgroundThresholdFilter->SetInput( this->m_PQCTImage );
  foregroundBackgroundThresholdFilter->SetInsideValue( FOREGROUND );
  foregroundBackgroundThresholdFilter->SetOutsideValue( AIR );
  foregroundBackgroundThresholdFilter->SetLowerThreshold( this->m_CT_LegThreshold );
  foregroundBackgroundThresholdFilter->SetUpperThreshold( SIGNEDSHORTMAX );  // number of regions we want detected 
  foregroundBackgroundThresholdFilter->Update();

  //! Connected components.
  //! Apply connected component labeling.
  typedef itk::ConnectedComponentImageFilter<LabelImageType, 
    LabelImageType, 
    LabelImageType> 
    ConnectedComponentLabelFilterType;
  ConnectedComponentLabelFilterType::Pointer labelMaskFilter2 = 
    ConnectedComponentLabelFilterType::New();
  labelMaskFilter2->SetInput( foregroundBackgroundThresholdFilter->GetOutput() );
  labelMaskFilter2->SetMaskImage( foregroundBackgroundThresholdFilter->GetOutput() );
  labelMaskFilter2->SetFullyConnected( true );
  labelMaskFilter2->Update();

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
  labelImageToShapeLabelMapFilter->SetInput( labelMaskFilter2->GetOutput() );
  labelImageToShapeLabelMapFilter->SetBackgroundValue( AIR );
  labelImageToShapeLabelMapFilter->Update();

  //! Dsiplay computed shape attributes.
  //! Get label map.
  LabelMapType::Pointer labelMap = labelImageToShapeLabelMapFilter->GetOutput();

  //! Pass quantification parameter names and values.
  //! Compute centroids.

  std::cout << "Tissue label\tArea (mm^2)\tCentroid coordinates" << std::endl;

  std::vector<float> regionCentroids;
  std::vector <LabelImageType::RegionType> regionBoundingBoxes;
  for(unsigned int i = 0; i < labelMap->GetNumberOfLabelObjects(); i++)
    {
      LabelObjectType * labelObject = labelMap->GetNthLabelObject(i);
      const LabelType & label = labelObject->GetLabel();

      std::cout << label << "\t" 
		<< labelObject->GetPhysicalSize() << "\t" 
		<< labelObject->GetCentroid() << std::endl;

      //! Set small regions RL coordinate to 0 to discard from leg selection.
      //! Store information needed in the next steps.
      if(labelObject->GetPhysicalSize() < LEGPHYSICALSIZETHRESHOLD) {
	regionCentroids.push_back( 0 );
	regionBoundingBoxes.push_back( labelObject->GetBoundingBox() );
      }
      else {
	regionCentroids.push_back( labelObject->GetCentroid()[0] );
	regionBoundingBoxes.push_back( labelObject->GetBoundingBox() );
      }
    }


  //! Select left-most object as the one with maximum R coordinate.
  int leftlegIndex = static_cast<int> ( std::max_element(regionCentroids.begin(), 
							 regionCentroids.end()) - regionCentroids.begin() );
  LabelObjectType * labelObject2 = labelMap->GetNthLabelObject(leftlegIndex);
  this->m_leftlegLabel = labelObject2->GetLabel();
  PQCTImageType::RegionType leftlegBoundingBox = regionBoundingBoxes[leftlegIndex];
  PQCTImageType::IndexType newIndex;
  PQCTImageType::SizeType newSize;
  for(int i=0;i<pixelDimensions;i++) {
    newSize[i] = leftlegBoundingBox.GetSize()[i] + 2 * PADDINGLENGTH;
    newIndex[i] =  leftlegBoundingBox.GetIndex()[i] - PADDINGLENGTH;
    }
  leftlegBoundingBox.SetSize(newSize);
  leftlegBoundingBox.SetIndex(newIndex);
  std::cout << "Left leg is object with label: " << this->m_leftlegLabel << std::endl;
  std::cout << "Corresponding bounding box is: " 
	    << leftlegBoundingBox 
	    << std::endl;


  //! Crop around object.
  typedef itk::ExtractImageFilter< PQCTImageType, PQCTImageType > ExtractImageFilterType;
  ExtractImageFilterType::Pointer leftlegExtractor = ExtractImageFilterType::New();
  leftlegExtractor->SetExtractionRegion(leftlegBoundingBox);
  leftlegExtractor->SetInput(this->m_PQCTImage);
  leftlegExtractor->SetDirectionCollapseToIdentity(); // This is required.
  leftlegExtractor->Update();

  this->m_PQCTImage = 0;
  this->m_PQCTImage = leftlegExtractor->GetOutput();


  //! Write image to nifti file.
  itk::ImageFileWriter< PQCTImageType >::Pointer 
    inputWriter = itk::ImageFileWriter< PQCTImageType >::New();
  inputWriter->SetInput( this->m_PQCTImage );
  inputWriter->SetFileName( this->m_outputPath + "OneLeg.nii" );
  // inputWriter->SetFileName( this->m_outputPath + this->m_SubjectID + inputImageFileExtension );
  inputWriter->Update();
  inputWriter = 0;

  return labelMaskFilter2->GetOutput();
}


//! Create a label image with the original size--before any cropping--
//!  and copy label image to it.
LabelImageType::Pointer PQCT_Analyzer::CreateOutputLabelImage() {

  LabelImageType::Pointer outputLabelImage = LabelImageType::New();
  outputLabelImage->CopyInformation( this->m_PQCTImage );
  outputLabelImage->SetRegions( this->m_PQCTImage->GetLargestPossibleRegion() );
  outputLabelImage->Allocate();
  outputLabelImage->FillBuffer( 0 );

  return outputLabelImage;
}


//! Copy labels in original space and write to file.
void PQCT_Analyzer::CopyFinalLabelsinOriginalSpace(LabelImageType::Pointer labelImageInOriginalSpace) {

  itk::ImageRegionIteratorWithIndex<LabelImageType> it(this->m_TissueLabelImage, 
						       this->m_TissueLabelImage->GetLargestPossibleRegion());

  LabelImageType::IndexType indexSource, indexDestination;
  LabelImageType::PointType point;

  //! Use iterator to copy labels to original image space.
  for ( it.GoToBegin(); !it.IsAtEnd(); ++it) {
    indexSource = it.GetIndex();
    this->m_TissueLabelImage->TransformIndexToPhysicalPoint(indexSource, point);
    labelImageInOriginalSpace->TransformPhysicalPointToIndex(point, indexDestination);
    //! Check if inside array boundaries.
    if ( labelImageInOriginalSpace->GetLargestPossibleRegion().IsInside(indexDestination)  )
      labelImageInOriginalSpace->SetPixel(indexDestination, it.Get());
  }

  // return labelImageInOriginalSpace;
}


//! Analyze at middle thigh site.
void PQCT_Analyzer::AnalyzeCTMidThigh(){

  //! Open output csv file for writing.
  std::string prefix = this->m_outputPath + this->m_SubjectID + "_MidThigh";
  std::string tempFilename = prefix + quantificationFileExtension;
  this->m_textFile.open(tempFilename.c_str());
  if (this->m_textFile.fail()) {
    std::cerr << "unable to open file for writing" << std::endl;
    return;
  }

  std::cout << "--------Quantification at middle thigh--------" << std::endl;


  //! Instantiate the output label image before any cropping.
  LabelImageType::Pointer outputLabelImage = this->CreateOutputLabelImage();


  //! Remove patient table and select left thigh.
  LabelImageType::Pointer LegOnlyMask = this->SelectOneLeg();


  //! 1. Segment tissue types.
  this->ApplyKMeans();


  //! 2. Separate intermuscular from subcutaneous fat.
  //! Label connected fat components.
  //! Compute fat areas  
  //! select the largest one as subcutaneous fat and
  //! morphological closing to subcutaneous fat.
  switch (this->m_SAT_IMFAT_SeparationAlgorithm) {
    case CONNECTED_COMPONENTS:
      std::cout << "Using post-kmeans connected components for SAT/IMFAT separation."
		<< std::endl;
      this->IdentifySubcutaneousAndInterMuscularFat();
      break;
    case GAC:
      std::cout << "Using GAC for SAT/IMFAT separation."
		<< std::endl;
      this->IdentifySubcutaneousAndInterMuscularFatByGAC();
      break;
    default:
      std::cerr << "Unknown workflow number." 
		<< std::endl;
      break;
    }


  //! 3. Remove identified fat components from interior of femur.
  this->IdentifyBoneMarrow();
 

  //! 4. Re-classify IMFAT and muscle voxels.
  this->SeparateIMFATFromMuscle();


  //! 5. Dilate subcutaneous fat region to address PVE.
  this->DilateSubcutaneousFatForPVECorrection();


  //! 6. Mask results with total thigh mask.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(LegOnlyMask, 
  				 LegOnlyMask->GetLargestPossibleRegion());
  LabelImageType::RegionType labelmapRegion = this->m_TissueLabelImage->GetLargestPossibleRegion();
  LabelImageType::IndexType legonlymaskIdx, labelmapIdx;
  LabelImageType::PointType point;
  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    legonlymaskIdx = itImage.GetIndex();
    LegOnlyMask->TransformIndexToPhysicalPoint(legonlymaskIdx, point);
    this->m_TissueLabelImage->TransformPhysicalPointToIndex(point,labelmapIdx);
    if ( labelmapRegion.IsInside(labelmapIdx) && itImage.Get() != m_leftlegLabel ) {
      this->m_TissueLabelImage->SetPixel(labelmapIdx, BACKGROUND );    
      // std::cout << labelmapIdx << std::endl;
    }
  }


  //! 7. Compute: bone, muscle, fat areas, and bone, muscle density.
  // Display results.
  this->LogHeaderInfo();
  this->ComputeTissueShapeAttributes();
  this->ComputeTissueIntensityAttributes();


  //! Foreground/background segmentation and computation of region attributes.
  LabelImageType::Pointer totalLegCrosssectionImage = 
    // this->ForegroundBackgroundSegmentationByFastMarching();
    this->ForegroundBackgroundSegmentationAfterKMeans();


  //! Compute centroid and density over total leg.
  this->ComputeTissueShapeAttributes( totalLegCrosssectionImage );
  this->ComputeTissueIntensityAttributes( totalLegCrosssectionImage );
  

  //! Write measurements to text file.
  this->WriteToTextFile();

  
  //! Update output label image with the segmentation output.
  this->CopyFinalLabelsinOriginalSpace(outputLabelImage);

  //! Save output image to file.
  itk::ImageFileWriter<LabelImageType>::Pointer labelWriter2 = 
    itk::ImageFileWriter<LabelImageType>::New();
  // labelWriter2->SetInput( this->m_TissueLabelImage ); 
  labelWriter2->SetInput( outputLabelImage ); 
  labelWriter2->SetFileName( prefix + 
			     labelImageFileExtension );
  labelWriter2->Update();
  labelWriter2 = 0;


  //! Close output stream before exiting.
  this->m_textFile.close();

}
