/*===========================================================================

Program:   Bone, muscle and fat quantification from PQCT data.
Module:    $RCSfile: PQCT_Analysis.h,v $
Language:  C++
Date:      $Date: 2011/08/10 14:17:00 $
Version:   $Revision: 0.1 $
Author:    S. K. Makrogiannis
3T MRI Facility National Institute on Aging/National Institutes of Health.

=============================================================================*/

#ifndef __PQCT_Analysis_h__
#define __PQCT_Analysis_h__

#include "PQCT_Datatypes.h"


//! Used for storing indices.
template<class T> struct index_cmp {
index_cmp(const T arr) : arr(arr) {}
bool operator()(const size_t a, const size_t b) const
{ return arr[a] < arr[b]; }
const T arr;
};


//! Class implementing PQCT analysis.
class PQCT_Analyzer {

 public:

  PQCT_Analyzer(){
    /* this->m_gradientSigma = SMOOTHINGSIGMA; */
    /* this->m_medianFilterKernelLength = MEDIANFILTERRADIUS; */
    /* this->m_sigmoidBeta = LEVELSETSIGMOIDBETA; */
    /* this->m_sigmoidAlpha = (-1) * (this->m_sigmoidBeta / SIGMOIDBETAALPHARATIO ); */
    /* this->m_fastmarchingStoppingTime = (double) FASTMARCHINGSTOPPINGTIME; */
    /* this->m_levelsetMaximumRMSError = LEVELSETMAXRMSERROR; */
    /* this->m_levelsetMaximumIterations = LEVELSETMAXITERATIONS; */
    /* this->m_levelsetPropagationScalingFactor = LEVELSETPROPAGATIONSCALINGFACTOR; */
    /* this->m_levelsetCurvatureScalingFactor = LEVELSETCURVATURESCALINGFACTOR; */
    /* this->m_levelsetAdvectionScalingFactor = LEVELSETADVECTIONSCALINGFACTOR; */

    //! Initialize parameter file;
    this->SetParameterFilename("./PQCT_Analysis_Params.txt");
    //! Set default output path.
    this->SetOutputPath("./");
    //! Set algorithm parameters.
    this->SetParameters();
  };

  ~PQCT_Analyzer(){};

  void SetPQCTImageFilename(std::string PQCTImageFilename) {
    this->m_PQCTImageFilename = PQCTImageFilename;
  };
  void SetWorkflowID(unsigned short workflowID) {
    this->m_WorkflowID = workflowID;
  };
  
  void SetParameterFilename(std::string parameterFilename) {
    this->m_parameterFilename = parameterFilename;
  };
  
  void SetOutputPath(std::string outputPath) {
    this->m_outputPath = outputPath;
  };
  void SetLabelFilename(std::string labelImageFilename){
    this->m_LabelImageFilename = labelImageFilename;
  };
  void SetQuantificationFilename(std::string quantificationFilename){
    this->m_QuantificationFilename = quantificationFilename;
  };

  void Execute();

 protected:
  void SetParameters();
  int 
    ParseSegmentationParameterFile( std::string paramsFilename, 
                                    bool debugFlag );
  void CopyParameterValuesToClassVariables();

  void ReadpQCTImageHeader();
  void ReadPQCTImage();
  void CalibrateImage();
  void AnonymizePQCTImage();
  int ReadDicomCTImage();
  std::string 
    RetrieveDicomTagValue(std::string tagkey, 
			  itk::GDCMImageIO::Pointer gdcmImageIO);
  void Analyze4PCT();
  void Analyze38PCT();
  void Analyze66PCT();
  void AnalyzeCTMidThigh();

  FloatImageType::Pointer SmoothInputVolume( PQCTImageType::Pointer inputVolume,
					     int denoisingMethod );
  LabelImageType::Pointer 
    ForegroundBackgroundSegmentationByFastMarching();
  LabelImageType::Pointer 
    ForegroundBackgroundSegmentationAfterKMeans();

  LabelImageType::Pointer
    InitializeROIbyFastMarching(FloatImageType::Pointer speedImage,
				LabelImageType::IndexType medianIdx,
				float stoppingTime);
  LabelImageType::Pointer
    ApplyGeodesicActiveContoursToLabelImage(LabelImageType::Pointer roiVolume,
					    FloatImageType::Pointer speedImage,
					    unsigned int label);
  LabelImageType::Pointer SegmentbyLevelSets( LabelImageType::IndexType medianIdx,
					      unsigned int label );
  LabelImageType::Pointer SegmentbyLevelSets( LabelImageType::Pointer roiVolume,
					      unsigned int label );
  LabelImageType::Pointer 
    SelectAreaFraction( int label );
  LabelImageType::Pointer OverlayLabelImages(LabelImageType::Pointer image1,
					     LabelImageType::Pointer image2);
  LabelImageType::Pointer CreateOutputLabelImage();
  void CopyFinalLabelsinOriginalSpace(LabelImageType::Pointer labelImageInOriginalSpace);

  void CloseSubcutaneousFatRegion();
  void RemoveSkinByMorphologicalErosion();
  void ApplyFatThreshold();
  void MergeSubcutaneousWithInterMuscularFat();
  void IdentifySubcutaneousAndInterMuscularFat();
  void IdentifySubcutaneousAndInterMuscularFatByGAC();
  void IdentifyTibiaAndFibula();
  LabelImageType::Pointer SelectOneLeg();
  void SeparateIMFATFromMuscle();
  void DilateSubcutaneousFatForPVECorrection();
  void Separate_Four_PCT_Tissues();
  void LogHeaderInfo();
  void ComputeTissueShapeAttributes(LabelImageType::Pointer labelImage);
  void ComputeTissueShapeAttributes();
  void ComputeTissueIntensityAttributes(LabelImageType::Pointer labelImage);
  void ComputeTissueIntensityAttributes();
  void IdentifyBoneMarrow();
  void MapTissueClassesPostKMeans();
  void SetTissueClasses();
  void SetTissueClassesNoAir();
  void ApplyKMeans();
  void WriteToTextFile();

 private:

 // String manipulations to separate the path from filename.
 std::string 
    ExtractDirectory( const std::string& path ) {
      return path.substr( 0, path.find_last_of( PathSeparator ) +1 );
  };
  std::string 
    ExtractFilename( const std::string& path ) {
    return path.substr( path.find_last_of( PathSeparator ) +1 );
  };
  std::string 
    ChangeExtension( const std::string& path, const std::string& ext ) {
  std::string filename = ExtractFilename( path );
  return ExtractDirectory( path ) +filename.substr( 0, filename.find_last_of( '.' ) ) +ext;
  };
  void 
    ExtractSubjectID() {
    std::string filename = this->ExtractFilename( this->m_PQCTImageFilename );
    if( this->m_WorkflowID == CT_MID_THIGH )
      this->m_SubjectID = filename.substr( 0, filename.find_last_of( '.' ));
    else
      this->m_SubjectID = filename;
  }

  // Image header containers.

  typedef struct t_HeaderPrefixType
  {
    int HeaderVersion;
    int HeaderLength;
  } 
  HeaderPrefixType;

  typedef struct t_DetectorInformationType
  {
    int DetRecTypeLength;
    double VoxelSize;
    unsigned short NumberofSlices;
    double SliceOrigin;
    int ScanDate;
  } 
  DetectorInformationType;

  typedef struct t_PatientInformationType
  {
    int PatInfoRecTypeLength;
    unsigned short PatientGender;
    unsigned short PatientEthnicGroup;
    unsigned short PatientMeasurementNumber;
    int PatientNumber;
    int PatientBirthDate;
    std::string PatientName;
    std::string UserID;
    std::string PatientID;
  } 
  PatientInformationType;

  typedef struct t_ImageInformationType
  {
    int PicInfoRecLength;
    unsigned short int PicX0;
    unsigned short PicY0;
    unsigned short MatrixSize[2];
  } 
  ImageInformationType;


  // Other variable-members of the class.
  unsigned short m_WorkflowID, m_SAT_IMFAT_SeparationAlgorithm;
  std::string m_PQCTImageFilename;
  std::string m_LabelImageFilename;
  std::string m_QuantificationFilename;
  std::string m_SubjectID;
  std::vector<float> m_TissueClassesVector, m_TissueClassesVectorNoAir;
  TableEntries m_TissueIntensityEntries, m_TissueShapeEntries;

  // Image and text files.
  PQCTImageType::Pointer m_PQCTImage;
  LabelImageType::Pointer m_KmeansLabelImage;
  LabelImageType::Pointer m_TissueLabelImage;
  std::ofstream m_textFile;

  // Algorithm parameters.
  std::vector<float> m_parameterValues;
  std::string m_parameterFilename, m_outputPath;
  int m_numberofParameters;
  std::vector<std::string> m_parameterIDs;
  int m_plaqueSegmentationParamsIndex;
  float m_AUtoDensitySlope, m_AUtoDensityIntercept;
  int m_medianFilterKernelLength;
  double m_gradientSigma;
  double m_fastmarchingStoppingTime;
  float m_sigmoidAlpha, m_sigmoidBeta;
  float m_levelsetMaximumRMSError;
  int m_levelsetMaximumIterations;
  float m_levelsetPropagationScalingFactor, 
    m_levelsetCurvatureScalingFactor,
    m_levelsetAdvectionScalingFactor;
  int m_CT_LegThreshold;
  unsigned long m_leftlegLabel;
  
  // pQCT ct image header structure.
  HeaderPrefixType m_HeaderPrefix;
  DetectorInformationType m_DetectorInformation;
  PatientInformationType m_PatientInformation;
  ImageInformationType m_ImageInformation;
};

#endif
