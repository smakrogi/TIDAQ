
/*===========================================================================

Program:   Bone, muscle and fat quantification from PQCT data.
Module:    $RCSfile: PQCT_AnalysisWrapper.cxx,v $
Language:  C++
Date:      $Date: 2011/08/10 14:17:00 $
Version:   $Revision: 0.1 $
Author:    S. K. Makrogiannis
3T MRI Facility National Institute on Aging/National Institutes of Health.

=============================================================================*/

#include <string>
#include <iostream>
// #include <jni.h>
#include "PQCT_Analysis.h"
#include "PQCTAnalysisJNI.h"
#include "PQCT_AnalysisWrapperITK.h"


//! This wrapper is expected to have interfaces to JNI and to regular C++ datatypes.
//! This function uses as input the C++ datatypes.
MYLIB_EXPORT void PQCT_AnalysisWrapperITK( std::string pqctimageFilename,
					   unsigned int workflowID,
					   std::string parameterFilename)
{
  //! Instantiate the pqct analysis class.
  PQCT_Analyzer* ITK_Analyzer = new PQCT_Analyzer();

  ITK_Analyzer->SetPQCTImageFilename( pqctimageFilename );
  ITK_Analyzer->SetWorkflowID( (short)workflowID );
  //  ITK_Analyzer->SetLabelFilename( outputlabelimageFilename );
  //  ITK_Analyzer->SetQuantificationFilename( quantificationFilename );
  ITK_Analyzer->SetParameterFilename(parameterFilename);
  ITK_Analyzer->Execute();

  delete ITK_Analyzer;
}


//! Use definition produced by JNI for binding.
JNIEXPORT void JNICALL Java_PQCTAnalysisJNI_execute
  (JNIEnv *env, jobject jobj, jstring name, jshort shortNum, jstring name2, jstring name3){
  jboolean iscopy;
  const char *mfile = env->GetStringUTFChars(name, &iscopy);
  const char *mfile2 = env->GetStringUTFChars(name2, &iscopy);
  const char *mfile3 = env->GetStringUTFChars(name3, &iscopy);
  std::string pqctimageFilename = mfile;
  std::string parameterFilename = mfile2;
  std::string outputPath = mfile3;
  short workflowID = (short) shortNum;

  //! Instantiate the pqct analysis class.
  PQCT_Analyzer* ITK_Analyzer = new PQCT_Analyzer();

  ITK_Analyzer->SetPQCTImageFilename( pqctimageFilename );
  ITK_Analyzer->SetParameterFilename(parameterFilename);
  ITK_Analyzer->SetOutputPath( outputPath );
  ITK_Analyzer->SetWorkflowID( workflowID );
  //  ITK_Analyzer->SetLabelFilename( outputlabelimageFilename );
  //  ITK_Analyzer->SetQuantificationFilename( quantificationFilename );
  ITK_Analyzer->Execute();

  delete ITK_Analyzer;
}
