IF ("$ENV{ATDM_CONFIG_BUILD_TYPE}" STREQUAL "DEBUG")
	ATDM_SET_ENABLE(PanzerAdaptersSTK_CurlLaplacianExample-ConvTest-Quad-Order-4_DISABLE ON)
	ATDM_SET_ENABLE(PanzerAdaptersSTK_MixedPoissonExample-ConvTest-Hex-Order-3_DISABLE ON)
ENDIF()

ATDM_SET_ENABLE(MueLu_ENABLE_Kokkos_Refactor_Use_By_Default ON)
ATDM_SET_ENABLE(TPL_ENABLE_gtest OFF)
ATDM_SET_ENABLE(Trilinos_ENABLE_Gtest OFF)
INCLUDE("${CMAKE_CURRENT_LIST_DIR}/EMPIRETrilinosPackagesList.cmake")
FOREACH(TRIBITS_PACKAGE ${EMPIRE_Trilinos_Packages})
  SET(${PROJECT_NAME}_ENABLE_${TRIBITS_PACKAGE} ON)
ENDFOREACH()
