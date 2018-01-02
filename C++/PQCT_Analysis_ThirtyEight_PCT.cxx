/*===========================================================================

  Program:   Bone, muscle and fat quantification from PQCT data.
  Module:    $RCSfile: PQCT_Analysis_ThirtyEight_PCT.cxx,v $
  Language:  C++
  Date:      $Date: 2012/03/12 10:01:00 $
  Version:   $Revision: 0.1 $
  Author:    S. K. Makrogiannis
  3T MRI Facility National Institute on Aging/National Institutes of Health.

  =============================================================================*/

#include <itkImageRegionIteratorWithIndex.h>
#include <itkImageFileWriter.h>
#include <ctime>

#include "PQCT_Datatypes.h"
#include "PQCT_Analysis.h"


//! Analyze at 38% tibia.
void PQCT_Analyzer::Analyze38PCT(){

  // Open output csv file for writing.
  std::string prefix = this->m_outputPath + this->m_SubjectID + "_38pct";
  std::string tempFilename = prefix + quantificationFileExtension;
  this->m_textFile.open(tempFilename.c_str());
  if (this->m_textFile.fail()) {
    std::cerr << "unable to open file for writing" << std::endl;
    return;
  }

  //! Start clock.
  std::clock_t begin = std::clock();

  //! Foreground background segmentation.
  std::cout << "--------Quantification at 38% Tibia--------" << std::endl;

  // Segment tissue types.
  this->ApplyKMeans();

  //! Remove identified fat components from interior of tibia and fibula.
  this->IdentifyBoneMarrow();
 
  //! Remove fibula from computation.
  this->IdentifyTibiaAndFibula();
  
  // Remove all other tissues besides cortical tibia and bone marrow.
  typedef itk::ImageRegionIteratorWithIndex<LabelImageType> LabelImageIteratorType;
  LabelImageIteratorType itImage2(this->m_TissueLabelImage, 
				  this->m_TissueLabelImage->GetBufferedRegion());
  LabelImageType::IndexType idx;
  for(itImage2.GoToBegin();!itImage2.IsAtEnd();++itImage2) {
    idx = itImage2.GetIndex();
    if( itImage2.Get() != CORT_BONE && itImage2.Get() != BONE_INT )
      itImage2.Set( AIR ); 
  }

  //! Compute area and average density.
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

  //! Save output image to file.
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

