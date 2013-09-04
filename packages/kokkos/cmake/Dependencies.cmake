SET(SUBPACKAGES_DIRS_CLASSIFICATIONS_OPTREQS
  #SubPackageName       Directory         Class    Req/Opt
  #
  Classic               classic           PS       OPTIONAL
  # New Kokkos subpackages:
  TPL                   TPL               PS       OPTIONAL
  Core                  core              EX       OPTIONAL
  Containers            containers        EX       OPTIONAL
  Example               example           EX       OPTIONAL
  Compat                compat            EX       OPTIONAL
  )

SET(LIB_REQUIRED_DEP_PACKAGES )
SET(LIB_OPTIONAL_DEP_PACKAGES )
SET(TEST_REQUIRED_DEP_PACKAGES )
SET(TEST_OPTIONAL_DEP_PACKAGES )
SET(LIB_REQUIRED_DEP_TPLS )
SET(LIB_OPTIONAL_DEP_TPLS ) 
SET(TEST_REQUIRED_DEP_TPLS )
SET(TEST_OPTIONAL_DEP_TPLS )
