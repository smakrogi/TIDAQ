Project(PQCT_Apps)

cmake_minimum_required(VERSION 2.6)

# Version number.
SET( PQCT_Apps_VERSION_MAJOR 1 )
SET( PQCT_Apps_VERSION_MINOR 4 )

SET (LIBRARY_OUTPUT_PATH ${PQCT_Apps_BINARY_DIR}/bin CACHE PATH "Single output directory
 for building all libraries." FORCE)
SET (EXECUTABLE_OUTPUT_PATH ${PQCT_Apps_BINARY_DIR}/bin CACHE PATH "Single output direct
ory for building all executables." FORCE)
SET (BUILD_SHARED_LIBS TRUE CACHE BOOL "build shared libs" )

FIND_PACKAGE(ITK REQUIRED)
IF (ITK_FOUND)
  INCLUDE(${ITK_USE_FILE})   
ELSE (ITK_FOUND)
  MESSAGE(ERROR "Cannot build without a ITK build tree.")
ENDIF (ITK_FOUND)

# FIND_PACKAGE(JAVA)
# IF(JAVA_INCLUDE_PATH)
#   INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH})
# ENDIF(JAVA_INCLUDE_PATH)

FIND_PACKAGE(JNI)
IF(JNI_INCLUDE_DIRS)
 INCLUDE_DIRECTORIES(${JNI_INCLUDE_DIRS})
ENDIF(JNI_INCLUDE_DIRS)

# SET ( JNI_INCLUDE_DIRS2 
#       /usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include
#       /usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/include/linux)
# INCLUDE_DIRECTORIES(${JNI_INCLUDE_DIRS2})

# SET ( JNI_LIBRARIES2
#      /usr/lib/jvm/java-1.6.0-openjdk-1.6.0.0.x86_64/lib )

INCLUDE_DIRECTORIES(
   ${PQCT_Apps_SOURCE_DIR}
   ${PQCT_Apps_BINARY_DIR} 
)

SET(ITK_LIBS 
    ${ITK_LIBRARIES}	
    # ITKAlgorithms
    # ITKBasicFilters
    # ITKCommon
    # ITKIO
    )

SET (LIB_TYPE STATIC)
IF (BUILD_SHARED_LIBS)
  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
  SET (LIB_TYPE SHARED)
ENDIF (BUILD_SHARED_LIBS)

ADD_LIBRARY(PQCT_Analysis
   ${LIB_TYPE}
   PQCT_Analysis_File_IO.cxx
   PQCT_Analysis_Four_PCT.cxx
   PQCT_Analysis_ThirtyEight_PCT.cxx
   PQCT_Analysis_SixtySix_PCT.cxx
   CT_Analysis_Mid_Thigh.cxx
   PQCT_Analysis.cxx
   PQCT_AnalysisWrapper.cxx)

TARGET_LINK_LIBRARIES ( PQCT_Analysis ${ITK_LIBS} )

ADD_EXECUTABLE( PQCT_AnalysisITK PQCT_AnalysisITK.cxx )
TARGET_LINK_LIBRARIES( PQCT_AnalysisITK PQCT_Analysis ${ITK_LIBS})

