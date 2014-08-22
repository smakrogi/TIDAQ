/*===========================================================================

Program:   Bone, muscle and fat quantification from PQCT data.
Module:    $RCSfile: PQCT_AnalysisWrapper.h,v $
Language:  C++
Date:      $Date: 2011/08/10 14:17:00 $
Version:   $Revision: 0.1 $
Author:    S. K. Makrogiannis
3T MRI Facility National Institute on Aging/National Institutes of Health.

=============================================================================*/

#ifndef __PQCT_AnalysisWrapperITK_h__
#define __PQCT_AnalysisWrapperITK_h__

/* Cmake will define MyLibrary_EXPORTS on Windows when it
configures to build a shared library. If you are going to use
another build system on windows or create the visual studio
projects by hand you need to define MyLibrary_EXPORTS when
building a DLL on windows.
*/
// We are using the Visual Studio Compiler and building Shared libraries

#if defined (_WIN32) 
  #if defined(PQCT_Analysis_EXPORTS)
    #define  MYLIB_EXPORT __declspec(dllexport)
  #else
    #define  MYLIB_EXPORT __declspec(dllimport)
  #endif /* PQCT_Analysis_EXPORTS */
#else /* defined (_WIN32) */
 #define MYLIB_EXPORT
#endif

MYLIB_EXPORT void PQCT_AnalysisWrapperITK(std::string pqctImage,
					  unsigned int workflowID,
					  std::string parameterFilename);

#endif
