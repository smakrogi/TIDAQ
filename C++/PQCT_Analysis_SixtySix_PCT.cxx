/*===========================================================================

  Program:   Bone, muscle and fat quantification from PQCT data.
  Module:    $RCSfile: PQCT_Analysis_SixtySix_PCT.cxx,v $
  Language:  C++
  Date:      $Date: 2012/03/12 10:01:00 $
  Version:   $Revision: 0.1 $
  Author:    S. K. Makrogiannis
  3T MRI Facility National Institute on Aging/National Institutes of Health.

  =============================================================================*/

#include <itkBinaryThresholdImageFilter.h>
#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryMorphologicalClosingImageFilter.h>
// #include <itkBinaryFillholeImageFilter.h>
#include <itkVotingBinaryIterativeHoleFillingImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkImageFileWriter.h>
#include <ctime>

#include "PQCT_Datatypes.h"
#include "PQCT_Analysis.h"


//! Morphological closing to remove holes and gaps from
//! subcutaneous fat region.
void PQCT_Analyzer::CloseSubcutaneousFatRegion() {

  //! Apply threshold to select subcutaneous fat region.
  typedef itk::BinaryThresholdImageFilter<LabelImageType, 
    LabelImageType> 
    SelectSubcutaneousFatRegionFilterType;
  SelectSubcutaneousFatRegionFilterType::Pointer subcutaneousfatRegionThresholdFilter = 
    SelectSubcutaneousFatRegionFilterType::New();
  subcutaneousfatRegionThresholdFilter->SetInput( this->m_TissueLabelImage );
  subcutaneousfatRegionThresholdFilter->SetInsideValue( 1 );
  subcutaneousfatRegionThresholdFilter->SetOutsideValue( 0 );
  subcutaneousfatRegionThresholdFilter->SetLowerThreshold( SUB_FAT );
  subcutaneousfatRegionThresholdFilter->SetUpperThreshold( SUB_FAT );  
  subcutaneousfatRegionThresholdFilter->Update();


  //! Hole filling using iterative binary voting.
  typedef itk::VotingBinaryIterativeHoleFillingImageFilter<LabelImageType>
    VotingBinaryIterativeHoleFillingImageFilterType;
  VotingBinaryIterativeHoleFillingImageFilterType::Pointer votingBinaryIterativeHoleFillingImageFilter = 
    VotingBinaryIterativeHoleFillingImageFilterType::New();

  LabelImageType::SizeType indexRadius;
  indexRadius.Fill( 5 );
  votingBinaryIterativeHoleFillingImageFilter->SetInput( subcutaneousfatRegionThresholdFilter->GetOutput() );
  votingBinaryIterativeHoleFillingImageFilter->SetForegroundValue( 1 );
  votingBinaryIterativeHoleFillingImageFilter->SetBackgroundValue( 0 );
  votingBinaryIterativeHoleFillingImageFilter->SetRadius( indexRadius );
  votingBinaryIterativeHoleFillingImageFilter->SetMajorityThreshold( 5 );
  votingBinaryIterativeHoleFillingImageFilter->SetMaximumNumberOfIterations( 5 );
  votingBinaryIterativeHoleFillingImageFilter->Update();


  //! Update tissue label map.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
  				 this->m_TissueLabelImage->GetBufferedRegion());
  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    LabelImageType::IndexType idx = itImage.GetIndex();
    // if( binaryHoleFillingFilter->GetOutput()->GetPixel(idx) == 1 )
    if( votingBinaryIterativeHoleFillingImageFilter->GetOutput()->GetPixel(idx) == 
    	votingBinaryIterativeHoleFillingImageFilter->GetForegroundValue() )
      itImage.Set( SUB_FAT );
  }


}


//! Remove skin by morphological erosion.
void PQCT_Analyzer::RemoveSkinByMorphologicalErosion() {

  //! Threshold background.
  typedef itk::BinaryThresholdImageFilter<LabelImageType, 
    LabelImageType> 
    SelectWholeLegFilterType;
  SelectWholeLegFilterType::Pointer selectWholeLegFilter = 
    SelectWholeLegFilterType::New();
  selectWholeLegFilter->SetInput( this->m_TissueLabelImage );
  selectWholeLegFilter->SetInsideValue( 1 );
  selectWholeLegFilter->SetOutsideValue( 0 );
  selectWholeLegFilter->SetLowerThreshold( FAT ); // FAT
  selectWholeLegFilter->SetUpperThreshold( TOT_AREA );  // TOT_AREA
  selectWholeLegFilter->Update();

  //! Generate structuring element.
  float structureElementRadius = 2.0F;
  typedef itk::BinaryBallStructuringElement<LabelPixelType, LabelImageType::ImageDimension> 
    StructuringElementType;
  StructuringElementType structuringElement;
  structuringElement.SetRadius( structureElementRadius );
  structuringElement.CreateStructuringElement();
  // std::cout << "Using a circle str. element with radius "
  // 	    << structureElementRadius
  // 	    << std::endl;
 
  //! Morphological binary erosion to remove skin.
  typedef itk::BinaryErodeImageFilter<LabelImageType,
    LabelImageType,
    StructuringElementType>
    BinaryErosionFilterType;
  BinaryErosionFilterType::Pointer binaryErosionFilter = 
    BinaryErosionFilterType::New();
  binaryErosionFilter->SetKernel( structuringElement );
  binaryErosionFilter->SetInput( selectWholeLegFilter->GetOutput() );
  binaryErosionFilter->SetForegroundValue( 1 );
  binaryErosionFilter->Update();

  //! Update tissue label map.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetBufferedRegion());
  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    LabelImageType::IndexType idx = itImage.GetIndex();
    if( binaryErosionFilter->GetOutput()->GetPixel(idx) == AIR &&
	itImage.Get() == SUB_FAT )
      itImage.Set( AIR );
  }
}


//! Apply fat threshold to remove outliers.
void PQCT_Analyzer::ApplyFatThreshold()
{
  //! Pick trabecular bone class and
  //! apply threshold to mask out other connected components.
  typedef itk::BinaryThresholdImageFilter<PQCTImageType, 
    LabelImageType> 
    SelectFatFilterType;
  SelectFatFilterType::Pointer selectFatFilter = 
    SelectFatFilterType::New();
  selectFatFilter->SetInput( this->m_PQCTImage );
  selectFatFilter->SetInsideValue( 1 );
  selectFatFilter->SetOutsideValue( 0 );
  selectFatFilter->SetLowerThreshold( -179 );
  selectFatFilter->SetUpperThreshold( 0 );
  selectFatFilter->Update();

  //! Remove fat pixels outside this range.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetBufferedRegion());

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    LabelImageType::IndexType idx = itImage.GetIndex();
    if( selectFatFilter->GetOutput()->GetPixel(idx) == 0 && 
	( itImage.Get() == SUB_FAT || itImage.Get() == IM_FAT) )
      itImage.Set( AIR );
  }

}


//! Merge the two types of fat. The separation is done to fill holes in scf first.
void PQCT_Analyzer::MergeSubcutaneousWithInterMuscularFat() {
  
  //! Label fat pixels.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetBufferedRegion());

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    LabelImageType::IndexType idx = itImage.GetIndex();
    if( itImage.Get() == SUB_FAT | itImage.Get() == IM_FAT)
      itImage.Set( FAT ); 
  }

}


//! Identify subcutaneous fat and separate from visceral.
void PQCT_Analyzer::IdentifySubcutaneousAndInterMuscularFat() {

  //! Select fat region and
  //! apply threshold to mask out other connected components.
  typedef itk::BinaryThresholdImageFilter<LabelImageType, 
    LabelImageType> 
    SelectFatRegionFilterType;
  SelectFatRegionFilterType::Pointer fattyRegionThresholdFilter = 
    SelectFatRegionFilterType::New();
  fattyRegionThresholdFilter->SetInput( this->m_KmeansLabelImage );
  fattyRegionThresholdFilter->SetInsideValue( 1 );
  fattyRegionThresholdFilter->SetOutsideValue( 0 );
  fattyRegionThresholdFilter->SetLowerThreshold( FAT );
  fattyRegionThresholdFilter->SetUpperThreshold( FAT );  // number of regions we want detected.
  fattyRegionThresholdFilter->Update();

  //! Apply connected component labeling on fat.
  typedef itk::ConnectedComponentImageFilter<LabelImageType, 
    LabelImageType, 
    LabelImageType> 
    ConnectedComponentLabelFilterType;
  ConnectedComponentLabelFilterType::Pointer labelMaskFilter = 
    ConnectedComponentLabelFilterType::New();
  labelMaskFilter->SetInput( fattyRegionThresholdFilter->GetOutput() );
  labelMaskFilter->SetMaskImage( fattyRegionThresholdFilter->GetOutput() );
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

  //! Pick the largest component as subcutaneous and rest as inter-muscular.
  //! Label subcutaneous and inter-muscular fat pixels.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetBufferedRegion());

  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    LabelImageType::IndexType idx = itImage.GetIndex();
    if( sortLabelsImageFilter->GetOutput()->GetPixel(idx) == 1 )
      itImage.Set( SUB_FAT ); // previously: FAT, SUB_FAT
    else if( sortLabelsImageFilter->GetOutput()->GetPixel(idx) > 1 )
      itImage.Set( IM_FAT );  // previously: FAT, MUSCLE, IM_FAT
  }


  //! Morphological closing to subcutaneous fat region.
  this->CloseSubcutaneousFatRegion();


  //! Erode whole leg area to remove skin.
  this->RemoveSkinByMorphologicalErosion();
  // this->ApplyFatThreshold();


  //! Merge the two types of fat.
  if( this->m_WorkflowID == PQCT_SIXTYSIX_PCT_TIBIA )
    this->MergeSubcutaneousWithInterMuscularFat();
}


//! Identify subcutaneous fat and separate from visceral.
void PQCT_Analyzer::IdentifySubcutaneousAndInterMuscularFatByGAC() {

  //! Threshold background.
  typedef itk::BinaryThresholdImageFilter<LabelImageType, 
    LabelImageType> 
    SelectWholeLegFilterType;
  SelectWholeLegFilterType::Pointer selectWholeLegFilter = 
    SelectWholeLegFilterType::New();
  selectWholeLegFilter->SetInput( this->m_TissueLabelImage );
  selectWholeLegFilter->SetInsideValue( 1 );
  selectWholeLegFilter->SetOutsideValue( 0 );
  selectWholeLegFilter->SetLowerThreshold( FAT ); // FAT
  selectWholeLegFilter->SetUpperThreshold( TOT_AREA );  // TOT_AREA
  selectWholeLegFilter->Update();

  //! Lightly erode foreground mask to generate initial ROI.
  //! Generate structuring element.
  float structureElementRadius = 2.0F;
  typedef itk::BinaryBallStructuringElement<LabelPixelType, LabelImageType::ImageDimension> 
    StructuringElementType;
  StructuringElementType structuringElement;
  structuringElement.SetRadius( structureElementRadius );
  structuringElement.CreateStructuringElement();
  // std::cout << "Using a circle str. element with radius "
  // 	    << structureElementRadius
  // 	    << std::endl;
 
  //! Morphological binary erosion.
  typedef itk::BinaryErodeImageFilter<LabelImageType,
    LabelImageType,
    StructuringElementType>
    BinaryErosionFilterType;
  BinaryErosionFilterType::Pointer binaryErosionFilter = 
    BinaryErosionFilterType::New();
  binaryErosionFilter->SetKernel( structuringElement );
  binaryErosionFilter->SetInput( selectWholeLegFilter->GetOutput() );
  binaryErosionFilter->SetForegroundValue( 1 );
  binaryErosionFilter->Update();

  //! Level set segmentation of non-subcutaneous region.
  LabelImageType::Pointer NonSubcutaneousMask = 
    this->SegmentbyLevelSets( binaryErosionFilter->GetOutput(),
			      FOREGROUND );

  //! Label fat compartments according to separation mask.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage(this->m_TissueLabelImage, 
				 this->m_TissueLabelImage->GetBufferedRegion());
  for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
    LabelImageType::IndexType idx = itImage.GetIndex();
    if( NonSubcutaneousMask->GetPixel(idx) != FOREGROUND && 
	( itImage.Get() == FAT || itImage.Get() == MUSCLE ) )
      itImage.Set( SUB_FAT ); // previously: FAT, SUB_FAT
    else if( NonSubcutaneousMask->GetPixel(idx) == FOREGROUND && itImage.Get() == FAT )
      itImage.Set( IM_FAT );  // previously: FAT, MUSCLE, IM_FAT
  }

  //! Morphological closing to subcutaneous fat region.
  this->CloseSubcutaneousFatRegion();

  //! Erode whole leg area to remove skin.
  this->RemoveSkinByMorphologicalErosion();


  //! Merge the two types of fat.
  if( this->m_WorkflowID == PQCT_SIXTYSIX_PCT_TIBIA )
    this->MergeSubcutaneousWithInterMuscularFat();
}


//! Analyze at 66% Tibia.
void PQCT_Analyzer::Analyze66PCT(){

  // Open output csv file for writing.
  std::string prefix = this->m_outputPath + this->m_SubjectID + "_66pct";
  std::string tempFilename = prefix + quantificationFileExtension;
  this->m_textFile.open(tempFilename.c_str());
  if (this->m_textFile.fail()) {
    std::cerr << "unable to open file for writing" << std::endl;
    return;
  }
  
  //! Start clock.
  std::clock_t begin = std::clock();

  //! Foreground background segmentation.
  std::cout << "--------Quantification at 66% Tibia--------" << std::endl;

  // 1. Segment tissue types.
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


  //! 4. Remove identified fat components from interior of tibia and fibula.
  this->IdentifyBoneMarrow();
 
  // //! 3. Remove fibula from computation.
  this->IdentifyTibiaAndFibula();


  //  //! Hole filling.
  //typedef itk::GrayscaleFillholeImageFilter<LabelImageType,LabelImageType> 
  //  BinaryHoleFillingFilterType;

  //BinaryHoleFillingFilterType::Pointer binaryHoleFillingFilter =
  //  BinaryHoleFillingFilterType::New();
  //binaryHoleFillingFilter->SetInput( this->m_TissueLabelImage );
  //// binaryHoleFillingFilter->InPlaceOn(); 
  //binaryHoleFillingFilter->Update();

  //for(itImage.GoToBegin();!itImage.IsAtEnd();++itImage) {
  //  LabelImageType::IndexType idx = itImage.GetIndex();
  //  if( binaryHoleFillingFilter->GetOutput()->GetPixel(idx) == IM_FAT )
  //    itImage.Set( MUSCLE );
  //  else 
  //    itImage.Set( AIR );
  //}


 // //! 5. Compute: bone, muscle, fat areas, and bone, muscle density.
  // // Display results.
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

  // Save output image to file.
  itk::ImageFileWriter<LabelImageType>::Pointer labelWriter2 = 
    itk::ImageFileWriter<LabelImageType>::New();
  labelWriter2->SetInput( this->m_TissueLabelImage ); 
  labelWriter2->SetFileName( prefix + 
			     labelImageFileExtension );
  labelWriter2->Update();
  labelWriter2 = 0;

  // Close output stream before exiting.
  this->m_textFile.close();

}
