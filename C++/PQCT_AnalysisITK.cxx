/*===========================================================================

Program:   Bone, muscle and fat quantification from PQCT data.
Module:    $RCSfile: PQCT_AnalysisITK.cxx,v $
Language:  C++
Date:      $Date: 2011/08/10 14:17:00 $
Version:   $Revision: 0.1 $
Author:    S. K. Makrogiannis
3T MRI Facility National Institute on Aging/National Institutes of Health.

=============================================================================*/

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#ifdef __BORLANDC__
#define ITK_LEAN_AND_MEAN
#endif

#include <cstdlib>
#include <string>
#include <iostream>
#include "PQCT_AnalysisWrapperITK.h"

/*! \mainpage PQCT analysis index page.
 *  \section intro_sec Introduction
 *  \section installation_sec Installation
 *  \section organization_sec Software organization
 *  \subsection C++
 *  \subsection JNI
 *  \subsection MIPAV plugin.
 */


//! Main standalone routine that executes the pipeline.

int
main( int argc, char ** argv )
{
  // itk::MultiThreader::SetGlobalMaximumNumberOfThreads(1);


  //! Arguments are: input image filename , workflow integer number, output label image filename, 
  //! output text filename .

  if (argc < 4) {
    std::cerr << "Usage: " 
              << argv[0] 
              << " <pqct image> <workflow {0,1,2,3,4} (4%, 38%, 66%, MID THIGH CT, Anonymize pQCT)> <parameter filename>"
              << std::endl; 
    return EXIT_FAILURE;
  }

  
  std::string pqctImageFilename = (std::string) argv[1];
  unsigned int workflowID = (unsigned int) atoi(argv[2]);
  std::string parameterFilename = (std::string) argv[3];
  // std::string outputLabelImageFilename = (std::string) argv[3];
  // std::string quantificationFilename = (std::string) argv[4];

 //! Call intermediate wrapper that passes the arguments and 
  //! invokes image processing pipeline.

  PQCT_AnalysisWrapperITK( pqctImageFilename,
			   workflowID,
			   parameterFilename );
  
  return EXIT_SUCCESS;
}
