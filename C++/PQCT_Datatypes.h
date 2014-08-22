/*===========================================================================

Program:   Bone, muscle and fat quantification from PQCT data.
Module:    $RCSfile: PQCT_DataTypes.h,v $
Language:  C++
Date:      $Date: 2012/07/03 13:48:00 $
Version:   $Revision: 0.2 $
Author:    S. K. Makrogiannis
3T MRI Facility National Institute on Aging/National Institutes of Health.

=============================================================================*/

#ifndef __PQCT_DataTypes_h__
#define __PQCT_DataTypes_h__

#include <cmath>
#include <itkImage.h>
#include <fstream>
#include <sstream>
#include <itkGDCMImageIO.h>

#define FLOAT_PRECISION 3
#define STRING_LENGTH 26
#define STRING_LENGTH_HEADER_TAGS 16

#ifndef MY_ROUND
#define MY_ROUND(X) ( ( X - floor(X) ) >= 0.5? ceil(X):floor(X) )
#endif 

#ifndef MY_SQUARE
#define MY_SQUARE(X)  (X * X)
#endif

#ifdef _WIN32
static const std::string PathSeparator = "\\";
#else
static const std::string PathSeparator = "/";
#endif

//! Enumeration of different workflows.
typedef enum{PQCT_FOUR_PCT_TIBIA=0, 
	     PQCT_THIRTYEIGHT_PCT_TIBIA, 
	     PQCT_SIXTYSIX_PCT_TIBIA,
	     CT_MID_THIGH,
	     PQCT_ANONYMIZE,
} PQCTWorkflow;

//! String array of different workflows.
static const std::string AnatomicalSite[] = { 
  "4_PCT",
  "38_PCT", 
  "66_PCT",
  "MID_THIGH",
  "UNUSED",
};

//! Enumeration of subcutaneous/inter-muscular fat separation method.
typedef enum{CONNECTED_COMPONENTS=1,
	     GAC} SAT_IMFAT_SEPARATION_ALGORITHM;

//! Enumeration of tissue types.
typedef enum{AIR=0, 
	     FAT, 
	     MUSCLE,
	     TRAB_BONE,
	     CORT_BONE, 
	     H_CORT_BONE,
	     BONE_INT,
	     SUB_FAT, 
	     IM_FAT, 
	     BONE_4PCT,
	     BONE_4PCT_50PCT,
	     BONE_4PCT_10PCT,
	     TOT_AREA,
} TISSUE_TYPES;


//! Tissue model used in quantification algorithm.
static const std::string TissueTypeString[] = { 
  "AIR",
  "FAT", 
  "MUSCLE", 
  "TRAB_BO",
  "COR_BO",
  "H_CORT_BONE",
  "BO_INT",
  "SUB_FA", 
  "IM_FA", 
  "BO_4%",
  "BO_4%50%",
  "BO_4%10%",
  "TOT_AR"};

//! Struct of variable names and values used in quantification.
typedef struct t_TableEntries
{
  std::stringstream headerString;
  std::stringstream valueString;
} 
  TableEntries;

//! Enumeration of smoothing approaches.
typedef enum {GAUSSIAN=0,
	      DIFFUSION,
	      MEDIAN}
  SmoothingMethod;

//! PQCT image info.
static const int pixelDimensions = 2; 
static const int headerLength = 1609;
static const float pixelSpacing66PCT = 0.8F;
static const float pixelSpacing4PCT = 0.5F;
static const float slope = 1724.0F; // My calibration: 1491.9F; AU: 1550.0F,1724.0F; HU: 0.716F
static const float intercept = -322.0F; // My calibration: -265.06F; AU: -355.0F,-322.0F; HU: -0.693F

//! Length in bytes of Borland Pascal data types.
#define LONGINT 4
#define INTEGER 2
#define SHORTINT 1
#define BYTE 1
#define BOOLEAN 1
#define CHAR 1
#define DOUBLE 8
#define SINGLE 4
#define WORD 2
#define MAXNUMDET 32

//! Algorithm Parameters.
#define SMOOTHINGSIGMA 0.5
#define MEDIANFILTERRADIUS 2
#define DIFFUSIONFILTERITERATIONS 20
#define DIFFUSIONFILTERTIMESTEP 0.0325
#define DIFFUSIONFILTERCONDUCTANCEPARAMETER 2.0
#define LEVELSETSIGMOIDBETA 55 
#define SIGMOIDBETAALPHARATIO 4.5
#define FASTMARCHINGSTOPPINGTIME 10.0
#define LEVELSETPROPAGATIONSCALINGFACTOR 1.0  // 1.0
#define LEVELSETCURVATURESCALINGFACTOR 0.2  // 0.2
#define LEVELSETADVECTIONSCALINGFACTOR 1.5  // 1.5
#define LEVELSETMAXITERATIONS 200
#define LEVELSETMAXRMSERROR 0.015
#define BACKGROUND 0
#define FOREGROUND 255
#define CTLEGTHRESHOLD -200
#define SIGNEDSHORTMAX 32767
#define PADDINGLENGTH 2
#define LEGPHYSICALSIZETHRESHOLD 500

//! ITK Data type definitions used in the application.
typedef short PQCTPixelType;
typedef float FloatPixelType;
typedef unsigned char LabelPixelType;
typedef itk::Image<PQCTPixelType, pixelDimensions> PQCTImageType;
typedef itk::Image<FloatPixelType, pixelDimensions> FloatImageType;
typedef itk::Image<LabelPixelType, pixelDimensions> LabelImageType;

//! Standard file extensions.
static const std::string labelImageFileExtension = ".Labels.nii";
static const std::string quantificationFileExtension = ".Quantification.txt";
static const std::string inputImageFileExtension = ".Input.nii";
static const std::string anonymizedImageFileExtension = ".Anon";
static const std::string anonymizedImageFilePrefix = "Anon_";

//! Segmentation parameter keys.
static const std::string parameterIDs[] = { "AUtoDensitySlope",
					    "AUtoDensityIntercept",
					    "SmoothingSigma",
					    "MedianFilterRadius",
					    "LevelSetSigmoidBeta",
					    "SigmoidBetaAlphaRatio",
					    "FastMarchingStoppingTime",
					    "LevelSetPropagationScalingFactor",
					    "LevelSetCurvatureScalingFactor",
					    "LevelSetAdvectionScalingFactor",
					    "LevelsetMaximumIterations",
					    "LevelsetMaximumRMSError",
					    "SAT_IMFAT_SeparationAlgorithm",
					    "CT_LegThreshold"};

//! Segmentation parameter values.
static const float parameterValues[] = { 1724.0,
					 -322.0,
					 0.5,
					 2,
					 55,
					 4.5,
					 10.0,
					 0.5,
					 0.1,
					 1.5,
					 250,
					 0.0015,
					 1,
					 -200 };


/* //! Function that re-orients input image. */
/* template <class ImageType> */
/* itk::SmartPointer<ImageType> */
/* ReorientImage( itk::SmartPointer<ImageType> inputImage, */
/*                itk::SpatialOrientation::ValidCoordinateOrientationFlags desiredOrientation ) */
/* { */
/*   // now orient and cast to output orientation and type */
/*   typedef itk::OrientImageFilter< ImageType, ImageType > OrientingFilterType; */
/*   typename OrientingFilterType::Pointer orientingFilter = OrientingFilterType::New();   */
/*   orientingFilter->SetInput( inputImage ); */
/*   // get input direction from image */
/*   orientingFilter->UseImageDirectionOn(); */
/*   orientingFilter->SetDesiredCoordinateOrientation( desiredOrientation );   */
/*   orientingFilter->Update(); */
/*   typename ImageType::Pointer output = orientingFilter->GetOutput(); */
/*   orientingFilter = 0; */

/*   return output; */
/* } */

#endif
