/*===========================================================================

Program:   Bone, muscle and fat quantification from PQCT data.
Module:    $RCSfile: PQCT_AnalysisWrapper.h,v $
Language:  C++
Date:      $Date: 2011/08/10 14:17:00 $
Version:   $Revision: 0.1 $
Author:    S. K. Makrogiannis
3T MRI Facility National Institute on Aging/National Institutes of Health.

=============================================================================*/

#ifndef __PQCT_AnalysisWrapper_h__
#define __PQCT_AnalysisWrapper_h__

void PQCT_AnalysisWrapperITK(std::string pqctImage,
			     unsigned int workflowID,
			     std::string parameterFilename);

// void PQCT_AnalysisWrapperJNI();

#endif
