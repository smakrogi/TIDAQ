/*===========================================================================

  Program:   Bone, muscle and fat quantification from PQCT data.
  Module:    $RCSfile: PQCT_Analysis_Four_PCT.cxx,v $
  Language:  C++
  Date:      $Date: 2012/08/06 10:01:00 $
  Version:   $Revision: 0.1 $
  Author:    S. K. Makrogiannis
  3T MRI Facility National Institute on Aging/National Institutes of Health.

  =============================================================================*/

#include <itkSignedMaurerDistanceMapImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkGrayscaleFillholeImageFilter.h>
#include <itkImageFileWriter.h>

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
#include <ctime>

#include "PQCT_Datatypes.h"
#include "PQCT_Analysis.h"


//! Select a fraction of the area around centroid.
LabelImageType::Pointer
PQCT_Analyzer::
SelectAreaFraction( int label ) {

  //! Compute signed distance map on mask.
  typedef itk::SignedMaurerDistanceMapImageFilter<LabelImageType, 
    FloatImageType> 
    LabelDistanceTransformType;
  LabelDistanceTransformType::Pointer ROIDistFilter2 = 
    LabelDistanceTransformType::New();
  ROIDistFilter2->SetInput( this->m_TissueLabelImage );
  ROIDistFilter2->SetUseImageSpacing( true );
  ROIDistFilter2->SetSquaredDistance( true );
  // ROIDistFilter2->SetInsideIsPositive( true );
  ROIDistFilter2->Update();


  // //! Find minimum value.
  // typedef itk::MinimumMaximumImageCalculator<FloatImageType>
  //   MinimumMaximumFilterType;
  // MinimumMaximumFilterType::Pointer minmaxFilter = 
  //   MinimumMaximumFilterType::New();
  // minmaxFilter->SetImage(ROIDistFilter2->GetOutput());
  // minmaxFilter->Compute();
  // FloatImageType::PixelType distancemapMinimum = minmaxFilter->GetMinimum();

  //! Build vectors with the distance map values and the indices. Store region size.
  std::vector<float> distanceValues;
  std::vector<int> xCoordinates,yCoordinates;
  
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetBufferedRegion());
  LabelImageType::IndexType idx;

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    idx = itImage.GetIndex();
    if( itImage.Get() == BONE_4PCT ) {
      xCoordinates.push_back( itImage.GetIndex()[0] );
      yCoordinates.push_back( itImage.GetIndex()[1] );
      distanceValues.push_back( ROIDistFilter2->GetOutput()->GetPixel(idx) );
    }
  }

  //! Sort vector of distance values. 
  std::vector<size_t> indices;
  for (unsigned i = 0; i < xCoordinates.size(); i++)
    indices.push_back(i);
  // b = [0, 1, 2]
  std::sort(indices.begin(), indices.end(), index_cmp<std::vector<float>&>(distanceValues));
  // b = [0, 2, 1]

  float desiredFraction = 0;
  if ( label == BONE_4PCT_50PCT )
    desiredFraction = 0.5;
  else if( label == BONE_4PCT_10PCT )
    desiredFraction = 0.1;
  else
    std::cerr << "Unexpected fraction level" 
	      << std::endl;
    
  //! Select fraction of indices and label them.
  int fractionThreshold = (int) ( (float)xCoordinates.size() * desiredFraction );

  LabelImageType::Pointer outputlabelImage = LabelImageType::New();
  outputlabelImage->CopyInformation( this->m_TissueLabelImage );
  outputlabelImage->SetRegions( this->m_TissueLabelImage->GetLargestPossibleRegion() );
  outputlabelImage->Allocate();
  outputlabelImage->FillBuffer( BACKGROUND );

  for(unsigned i = 0; i < fractionThreshold; i++) {
    idx[0] = xCoordinates[indices[i]];
    idx[1] = yCoordinates[indices[i]];
    outputlabelImage->SetPixel(idx, label);
  }
  
  return outputlabelImage;
  
}


//! Re-cluster leg area pixels.
void PQCT_Analyzer::Separate_Four_PCT_Tissues() {

  //! Apply denoising.
  FloatImageType::Pointer smoothedImage = 
    this->SmoothInputVolume( this->m_PQCTImage, 
			     DIFFUSION );

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
  //! For each no-background pixel
  //! add pixel index and intensity to corresponding vectors.
  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    if ( itImage.Get() != AIR) {
      sampleIndices.push_back(itImage.GetIndex());  
      samples->PushBack(smoothedImage->GetPixel(itImage.GetIndex())); // previously: this->m_PQCTImage
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
 
  this->SetTissueClassesNoAir();
  EstimatorType::ParametersType initialMeans(this->m_TissueClassesVectorNoAir.size());
  for(int i=0; i<this->m_TissueClassesVectorNoAir.size(); i++)
    initialMeans[i] = static_cast<double>(this->m_TissueClassesVectorNoAir[i]);

  estimator->SetParameters( initialMeans );
  estimator->SetKdTree( treeGenerator->GetOutput() );
  estimator->SetMaximumIteration( 200 );
  estimator->SetCentroidPositionChangesThreshold(0.0);
  estimator->StartOptimization();

  EstimatorType::ParametersType estimatedMeans = estimator->GetParameters();

  std::cout << "[";
  for ( unsigned int i = 0 ; i < this->m_TissueClassesVectorNoAir.size() ; i++ )
    {
    std::cout << estimatedMeans[i] << ", ";
    }
  std::cout << "]" << std::endl;

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
  classifier->SetNumberOfClasses( this->m_TissueClassesVectorNoAir.size() );
 
  typedef ClassifierType::ClassLabelVectorObjectType               ClassLabelVectorObjectType;
  typedef ClassifierType::ClassLabelVectorType                     ClassLabelVectorType;
  typedef ClassifierType::MembershipFunctionVectorObjectType       MembershipFunctionVectorObjectType;
  typedef ClassifierType::MembershipFunctionVectorType             MembershipFunctionVectorType;
 
  ClassLabelVectorObjectType::Pointer  classLabelsObject = ClassLabelVectorObjectType::New();
  classifier->SetClassLabels( classLabelsObject );
 
  ClassLabelVectorType &  classLabelsVector = classLabelsObject->Get();
  classLabelsVector.push_back( FAT );
  classLabelsVector.push_back( MUSCLE );
  classLabelsVector.push_back( TRAB_BONE );
  classLabelsVector.push_back( CORT_BONE );
  classLabelsVector.push_back( H_CORT_BONE );

  MembershipFunctionVectorObjectType::Pointer membershipFunctionsObject =
    MembershipFunctionVectorObjectType::New();
  classifier->SetMembershipFunctions( membershipFunctionsObject );
 
  MembershipFunctionVectorType &  membershipFunctionsVector = membershipFunctionsObject->Get();
 
  MembershipFunctionType::CentroidType origin( samples->GetMeasurementVectorSize() );
  int index = 0;
  for ( unsigned int i = 0 ; i < this->m_TissueClassesVectorNoAir.size() ; i++ )
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


//! Analysis at 4% tibia.
void PQCT_Analyzer::Analyze4PCT(){

  //! Open output csv file for writing.
  std::string prefix = this->m_outputPath + this->m_SubjectID + "_4pct";
  std::string tempFilename = prefix + quantificationFileExtension;
  this->m_textFile.open(tempFilename.c_str());
  if (this->m_textFile.fail()) {
    std::cerr << "unable to open file for writing" << std::endl;
    return;
  }

  //! Start clock.
  std::clock_t begin = std::clock();

  //! Foreground background segmentation.
  std::cout << "--------Quantification at 4% Tibia--------" << std::endl;

  //! Segment tissue types using K-means clustering.
  this->ApplyKMeans();


  // //! Re-cluster all leg tissues (no air).
  // this->Separate_Four_PCT_Tissues();


  //! Pick trabecular-cortical bone class and
  //! apply threshold to mask out other connected components.
  typedef itk::BinaryThresholdImageFilter<LabelImageType, 
    LabelImageType> 
    SelectBoneFilterType;
  SelectBoneFilterType::Pointer boneThresholdFilter = 
    SelectBoneFilterType::New();
  boneThresholdFilter->SetInput( this->m_KmeansLabelImage );
  boneThresholdFilter->SetInsideValue( 1 );
  boneThresholdFilter->SetOutsideValue( 0 );
  boneThresholdFilter->SetLowerThreshold( TRAB_BONE );
  boneThresholdFilter->SetUpperThreshold( H_CORT_BONE );  // number of regions we want detected.
  boneThresholdFilter->Update();

  //! Apply connected component labeling on bone.
  typedef itk::ConnectedComponentImageFilter<LabelImageType, 
    LabelImageType, 
    LabelImageType> 
    ConnectedComponentLabelFilterType;
  ConnectedComponentLabelFilterType::Pointer labelMaskFilter = 
    ConnectedComponentLabelFilterType::New();
  labelMaskFilter->SetInput( boneThresholdFilter->GetOutput() );
  labelMaskFilter->SetMaskImage( boneThresholdFilter->GetOutput() );
  labelMaskFilter->SetFullyConnected( true );
  labelMaskFilter->Update();

  //! Rank components wrt to size and relabel.
  typedef itk::RelabelComponentImageFilter<LabelImageType, 
    LabelImageType> 
    RelabelFilterType;
  RelabelFilterType::Pointer  sortLabelsImageFilter = 
    RelabelFilterType::New();
  sortLabelsImageFilter->SetInput( labelMaskFilter->GetOutput() );
  sortLabelsImageFilter->Update();

  //! Pick the largest component.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetBufferedRegion());

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    LabelImageType::IndexType idx = itImage.GetIndex();
    if( sortLabelsImageFilter->GetOutput()->GetPixel(idx) == 1 )
      itImage.Set( BONE_4PCT );
    else 
      itImage.Set( AIR );
  }

  //! Hole filling.
  typedef itk::GrayscaleFillholeImageFilter<LabelImageType,LabelImageType> 
    BinaryHoleFillingFilterType;

  BinaryHoleFillingFilterType::Pointer binaryHoleFillingFilter =
    BinaryHoleFillingFilterType::New();
  binaryHoleFillingFilter->SetInput( this->m_TissueLabelImage );
  // binaryHoleFillingFilter->InPlaceOn(); 
  binaryHoleFillingFilter->Update();

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    LabelImageType::IndexType idx = itImage.GetIndex();
    if( binaryHoleFillingFilter->GetOutput()->GetPixel(idx) == BONE_4PCT )
      itImage.Set( BONE_4PCT );
    else 
      itImage.Set( AIR );
  }

  //! Initial ROI around the median point.
  //! Compute median point of ROI.
  std::vector<int> xCoordinates,yCoordinates;
  LabelImageType::IndexType medianIdx;

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    if( itImage.Get() == BONE_4PCT ) {
      xCoordinates.push_back( itImage.GetIndex()[0] );
      yCoordinates.push_back( itImage.GetIndex()[1] );
    }
  }
  size_t size = xCoordinates.size();
  sort(xCoordinates.begin(), xCoordinates.end());
  sort(yCoordinates.begin(), yCoordinates.end());
  if (size  % 2 == 0)
    {
      medianIdx[0] = (xCoordinates[size / 2 - 1] + xCoordinates[size / 2]) / 2;
      medianIdx[1] = (yCoordinates[size / 2 - 1] + yCoordinates[size / 2]) / 2;
    }
  else 
    {
      medianIdx[0] = xCoordinates[size / 2];
      medianIdx[1] = yCoordinates[size / 2];
    }

  //! Segmentation by level sets.
  this->m_TissueLabelImage = 
    this->SegmentbyLevelSets( medianIdx, BONE_4PCT );

  //! Compute centroid and density over whole detected bone.
  this->LogHeaderInfo();
  this->ComputeTissueShapeAttributes();
  this->ComputeTissueIntensityAttributes();

  //! Label 50% trab. bone area.
  LabelImageType::Pointer outputlabelImage = 
    this->SelectAreaFraction( BONE_4PCT_50PCT );

  //! Compute centroid and density over 50% trabecular bone.
  //! radius using itk iterator.
  this->ComputeTissueShapeAttributes( outputlabelImage );
  this->ComputeTissueIntensityAttributes( outputlabelImage );

   //! Label 10% trab. bone area.
  LabelImageType::Pointer outputlabelImage2 = 
    this->SelectAreaFraction( BONE_4PCT_10PCT );

  //! Compute centroid and density over 10% trabecular bone.
  //! radius using itk iterator.
  this->ComputeTissueShapeAttributes( outputlabelImage2 );
  this->ComputeTissueIntensityAttributes( outputlabelImage2 );

  //! Foreground/background segmentation and computation of region attributes.
  LabelImageType::Pointer totalLegCrosssectionImage = 
    // this->ForegroundBackgroundSegmentationByFastMarching();
    this->ForegroundBackgroundSegmentationAfterKMeans();

  //! Compute centroid and density over total leg.
  this->ComputeTissueShapeAttributes( totalLegCrosssectionImage );
  this->ComputeTissueIntensityAttributes( totalLegCrosssectionImage );

 //! Stop the clock.
  std::clock_t end = std::clock();
  double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

  //! Pass Elapsed_Time to stringstream.
  std::stringstream tempStringstream;
  tempStringstream.str("");
  tempStringstream <<  "Elapsed_Time";
  this->m_TissueIntensityEntries.headerString.width(STRING_LENGTH);
  this->m_TissueIntensityEntries.headerString << tempStringstream.str();

  this->m_TissueIntensityEntries.valueString.width(STRING_LENGTH); 
  this->m_TissueIntensityEntries.valueString << elapsed_secs;

  // Write results to text file.
  this->WriteToTextFile();

  //! Create label map that shows regions.
  LabelImageType::Pointer outputlabelImage3 = 
    this->OverlayLabelImages(this->m_TissueLabelImage,
  			     outputlabelImage);

  LabelImageType::Pointer outputlabelImage4 = 
    this->OverlayLabelImages(outputlabelImage2,
  			     outputlabelImage3);

  //! Save output image to file.
  itk::ImageFileWriter<LabelImageType>::Pointer labelWriter2 = 
    itk::ImageFileWriter<LabelImageType>::New();
  labelWriter2->SetInput( outputlabelImage4 ); 
  labelWriter2->SetFileName( prefix + labelImageFileExtension );
  labelWriter2->Update();
  labelWriter2 = 0;

  // Close output stream before exiting.
  this->m_textFile.close();
}
