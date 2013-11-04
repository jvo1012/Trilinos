coarse: max size = 100
coarse: type = none
verbosity = test
max levels = 10   [default]
debug: graph level = -1   [default]
number of equations = 1   [default]
smoother: pre or post = both   [default]
aggregation: type = uncoupled   [default]
multigrid algorithm = sa   [default]
repartition: enable = 0   [default]

Level 0
 Setup Smoother (MueLu::IfpackSmoother{type = point relaxation stand-alone})
 relaxation: sweeps = 1
 relaxation: damping factor = 1
 relaxation: type = symmetric Gauss-Seidel
 relaxation: min diagonal value = 0   [default]
 relaxation: zero starting solution = 1   [default]
 relaxation: backward mode = 0   [default]
 relaxation: use l1 = 0   [default]
 relaxation: l1 eta = 1.5   [default]
 relaxation: number of local smoothing indices = 9999   [default]
 relaxation: local smoothing indices = 0   [default]
 
Level 1
 Prolongator smoothing (MueLu::SaPFactory)
  Build (MueLu::TentativePFactory)
   Build (MueLu::UncoupledAggregationFactory)
    Build (MueLu::CoalesceDropFactory)
    lightweight wrap = 1
    Dirichlet detection threshold = 0
    aggregation threshold = 0
    algorithm = original
    disable Dirichlet detection = 0   [unused]
    
   Ordering = 0   [default]
   MaxNeighAlreadySelected = 0   [default]
   MinNodesPerAggregate = 2   [default]
   UseOnePtAggregationAlgorithm = 1   [default]
   UseSmallAggregatesAggregationAlgorithm = 0   [default]
   UseUncoupledAggregationAlgorithm = 1   [default]
   UseMaxLinkAggregationAlgorithm = 1   [default]
   UseIsolatedNodeAggregationAlgorithm = 1   [default]
   UseEmergencyAggregationAlgorithm = 1   [default]
   OnePt aggregate map name =    [default]
   SmallAgg aggregate map name =    [default]
   
   Build (MueLu::AmalgamationFactory)
   [empty list]
   
   Nullspace factory (MueLu::NullspaceFactory)
   [empty list]
   
   Build (MueLu::CoarseMapFactory)
   [empty list]
   
  [empty list]
  
 Damping factor = 1.33333
 
 Transpose P (MueLu::TransPFactory)
 [empty list]
 
 Computing Ac (MueLu::RAPFactory)
 Keep AP Pattern = 0   [default]
 Keep RAP Pattern = 0   [default]
 
 Setup Smoother (MueLu::IfpackSmoother{type = point relaxation stand-alone})
 relaxation: sweeps = 1
 relaxation: damping factor = 1
 relaxation: type = symmetric Gauss-Seidel
 relaxation: min diagonal value = 0   [default]
 relaxation: zero starting solution = 1   [default]
 relaxation: backward mode = 0   [default]
 relaxation: use l1 = 0   [default]
 relaxation: l1 eta = 1.5   [default]
 relaxation: number of local smoothing indices = 3333   [default]
 relaxation: local smoothing indices = 0   [default]
 
Level 2
 Prolongator smoothing (MueLu::SaPFactory)
  Build (MueLu::TentativePFactory)
   Build (MueLu::UncoupledAggregationFactory)
    Build (MueLu::CoalesceDropFactory)
    lightweight wrap = 1
    Dirichlet detection threshold = 0
    aggregation threshold = 0
    algorithm = original
    disable Dirichlet detection = 0   [unused]
    
   Ordering = 0   [default]
   MaxNeighAlreadySelected = 0   [default]
   MinNodesPerAggregate = 2   [default]
   UseOnePtAggregationAlgorithm = 1   [default]
   UseSmallAggregatesAggregationAlgorithm = 0   [default]
   UseUncoupledAggregationAlgorithm = 1   [default]
   UseMaxLinkAggregationAlgorithm = 1   [default]
   UseIsolatedNodeAggregationAlgorithm = 1   [default]
   UseEmergencyAggregationAlgorithm = 1   [default]
   OnePt aggregate map name =    [default]
   SmallAgg aggregate map name =    [default]
   
   Build (MueLu::AmalgamationFactory)
   [empty list]
   
   Nullspace factory (MueLu::NullspaceFactory)
   [empty list]
   
   Build (MueLu::CoarseMapFactory)
   [empty list]
   
  [empty list]
  
 Damping factor = 1.33333
 
 Transpose P (MueLu::TransPFactory)
 [empty list]
 
 Computing Ac (MueLu::RAPFactory)
 Keep AP Pattern = 0   [default]
 Keep RAP Pattern = 0   [default]
 
 Setup Smoother (MueLu::IfpackSmoother{type = point relaxation stand-alone})
 relaxation: sweeps = 1
 relaxation: damping factor = 1
 relaxation: type = symmetric Gauss-Seidel
 relaxation: min diagonal value = 0   [default]
 relaxation: zero starting solution = 1   [default]
 relaxation: backward mode = 0   [default]
 relaxation: use l1 = 0   [default]
 relaxation: l1 eta = 1.5   [default]
 relaxation: number of local smoothing indices = 1111   [default]
 relaxation: local smoothing indices = 0   [default]
 
Level 3
 Prolongator smoothing (MueLu::SaPFactory)
  Build (MueLu::TentativePFactory)
   Build (MueLu::UncoupledAggregationFactory)
    Build (MueLu::CoalesceDropFactory)
    lightweight wrap = 1
    Dirichlet detection threshold = 0
    aggregation threshold = 0
    algorithm = original
    disable Dirichlet detection = 0   [unused]
    
   Ordering = 0   [default]
   MaxNeighAlreadySelected = 0   [default]
   MinNodesPerAggregate = 2   [default]
   UseOnePtAggregationAlgorithm = 1   [default]
   UseSmallAggregatesAggregationAlgorithm = 0   [default]
   UseUncoupledAggregationAlgorithm = 1   [default]
   UseMaxLinkAggregationAlgorithm = 1   [default]
   UseIsolatedNodeAggregationAlgorithm = 1   [default]
   UseEmergencyAggregationAlgorithm = 1   [default]
   OnePt aggregate map name =    [default]
   SmallAgg aggregate map name =    [default]
   
   Build (MueLu::AmalgamationFactory)
   [empty list]
   
   Nullspace factory (MueLu::NullspaceFactory)
   [empty list]
   
   Build (MueLu::CoarseMapFactory)
   [empty list]
   
  [empty list]
  
 Damping factor = 1.33333
 
 Transpose P (MueLu::TransPFactory)
 [empty list]
 
 Computing Ac (MueLu::RAPFactory)
 Keep AP Pattern = 0   [default]
 Keep RAP Pattern = 0   [default]
 
 Setup Smoother (MueLu::IfpackSmoother{type = point relaxation stand-alone})
 relaxation: sweeps = 1
 relaxation: damping factor = 1
 relaxation: type = symmetric Gauss-Seidel
 relaxation: min diagonal value = 0   [default]
 relaxation: zero starting solution = 1   [default]
 relaxation: backward mode = 0   [default]
 relaxation: use l1 = 0   [default]
 relaxation: l1 eta = 1.5   [default]
 relaxation: number of local smoothing indices = 371   [default]
 relaxation: local smoothing indices = 0   [default]
 
Level 4
 Prolongator smoothing (MueLu::SaPFactory)
  Build (MueLu::TentativePFactory)
   Build (MueLu::UncoupledAggregationFactory)
    Build (MueLu::CoalesceDropFactory)
    lightweight wrap = 1
    Dirichlet detection threshold = 0
    aggregation threshold = 0
    algorithm = original
    disable Dirichlet detection = 0   [unused]
    
   Ordering = 0   [default]
   MaxNeighAlreadySelected = 0   [default]
   MinNodesPerAggregate = 2   [default]
   UseOnePtAggregationAlgorithm = 1   [default]
   UseSmallAggregatesAggregationAlgorithm = 0   [default]
   UseUncoupledAggregationAlgorithm = 1   [default]
   UseMaxLinkAggregationAlgorithm = 1   [default]
   UseIsolatedNodeAggregationAlgorithm = 1   [default]
   UseEmergencyAggregationAlgorithm = 1   [default]
   OnePt aggregate map name =    [default]
   SmallAgg aggregate map name =    [default]
   
   Build (MueLu::AmalgamationFactory)
   [empty list]
   
   Nullspace factory (MueLu::NullspaceFactory)
   [empty list]
   
   Build (MueLu::CoarseMapFactory)
   [empty list]
   
  [empty list]
  
 Damping factor = 1.33333
 
 Transpose P (MueLu::TransPFactory)
 [empty list]
 
 Computing Ac (MueLu::RAPFactory)
 Keep AP Pattern = 0   [default]
 Keep RAP Pattern = 0   [default]
 
 Setup Smoother (MueLu::IfpackSmoother{type = point relaxation stand-alone})
 relaxation: sweeps = 1
 relaxation: damping factor = 1
 relaxation: type = symmetric Gauss-Seidel
 relaxation: min diagonal value = 0   [default]
 relaxation: zero starting solution = 1   [default]
 relaxation: backward mode = 0   [default]
 relaxation: use l1 = 0   [default]
 relaxation: l1 eta = 1.5   [default]
 relaxation: number of local smoothing indices = 124   [default]
 relaxation: local smoothing indices = 0   [default]
 
Level 5
 Prolongator smoothing (MueLu::SaPFactory)
  Build (MueLu::TentativePFactory)
   Build (MueLu::UncoupledAggregationFactory)
    Build (MueLu::CoalesceDropFactory)
    lightweight wrap = 1
    Dirichlet detection threshold = 0
    aggregation threshold = 0
    algorithm = original
    disable Dirichlet detection = 0   [unused]
    
   Ordering = 0   [default]
   MaxNeighAlreadySelected = 0   [default]
   MinNodesPerAggregate = 2   [default]
   UseOnePtAggregationAlgorithm = 1   [default]
   UseSmallAggregatesAggregationAlgorithm = 0   [default]
   UseUncoupledAggregationAlgorithm = 1   [default]
   UseMaxLinkAggregationAlgorithm = 1   [default]
   UseIsolatedNodeAggregationAlgorithm = 1   [default]
   UseEmergencyAggregationAlgorithm = 1   [default]
   OnePt aggregate map name =    [default]
   SmallAgg aggregate map name =    [default]
   
   Build (MueLu::AmalgamationFactory)
   [empty list]
   
   Nullspace factory (MueLu::NullspaceFactory)
   [empty list]
   
   Build (MueLu::CoarseMapFactory)
   [empty list]
   
  [empty list]
  
 Damping factor = 1.33333
 
 Transpose P (MueLu::TransPFactory)
 [empty list]
 
 Computing Ac (MueLu::RAPFactory)
 Keep AP Pattern = 0   [default]
 Keep RAP Pattern = 0   [default]
 
 
 --------------------------------------------------------------------------------
 ---                            Multigrid Summary                             ---
 --------------------------------------------------------------------------------
 Number of levels    = 6
 Operator complexity = 1.50
 Max Coarse Size     = 100
 Implicit Transpose  = false
 
 matrix rows    nnz  nnz/row procs
 A 0    9999  29995     3.00  1
 A 1    3333   9997     3.00  1
 A 2    1111   3331     3.00  1
 A 3     371   1111     2.99  1
 A 4     124    370     2.98  1
 A 5      42    124     2.95  1
 
 Smoother (level 0) both : MueLu::IfpackSmoother{type = point relaxation stand-alone}
 
 Smoother (level 1) both : MueLu::IfpackSmoother{type = point relaxation stand-alone}
 
 Smoother (level 2) both : MueLu::IfpackSmoother{type = point relaxation stand-alone}
 
 Smoother (level 3) both : MueLu::IfpackSmoother{type = point relaxation stand-alone}
 
 Smoother (level 4) both : MueLu::IfpackSmoother{type = point relaxation stand-alone}
 
 Smoother (level 5) pre  : no smoother
 Smoother (level 5) post : no smoother
 
