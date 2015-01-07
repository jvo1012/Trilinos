#include <Kokkos_Core.hpp>

#include "util.hpp"
#include "graph_helper_scotch.hpp"

#include "crs_matrix_base.hpp"
#include "crs_matrix_view.hpp"
#include "crs_row_view.hpp"

#include "ichol.hpp"

using namespace std;

typedef double value_type;
typedef int    ordinal_type;
typedef int    size_type;

typedef Kokkos::OpenMP space_type; 

typedef Example::CrsMatrixBase<value_type,ordinal_type,size_type,space_type> CrsMatrixBase;
typedef Example::CrsMatrixView<CrsMatrixBase> CrsMatrixView;

typedef Example::GraphHelper_Scotch<CrsMatrixBase> GraphHelper;

typedef Example::Uplo Uplo;
typedef Example::AlgoIChol AlgoIChol;

int main (int argc, char *argv[]) {
  if (argc < 2) {
    cout << "Usage: " << argv[0] << " filename" << endl;
    return -1;
  }
  
  Kokkos::initialize();
  cout << "Default execution space initialized = "
       << typeid(Kokkos::DefaultExecutionSpace).name()
       << endl;

  CrsMatrixBase AA("AA");

  ifstream in;
  in.open(argv[1]);
  if (!in.good()) {
    cout << "Error in open the file: " << argv[1] << endl;
    return -1;
  }
  AA.importMatrixMarket(in);
  cout << AA << endl;

  GraphHelper S(AA);
  S.computeOrdering();
  
  CrsMatrixBase PA("Permuted AA");
  PA.copy(S.PermVector(), S.InvPermVector(), AA);
  
  CrsMatrixBase UU("UU");
  UU.copy(Uplo::Upper, PA);
  
  cout << UU << endl;
  
  CrsMatrixView U(UU);
    
  int r_val = 0;
  
  // r_val = Example::IChol<Uplo::Upper,AlgoIChol::RightUnblocked>::invoke(U);
  Example::IChol<Uplo::Upper,AlgoIChol::RightUnblocked>::TaskFunctor<CrsMatrixView>(U).apply(r_val);
  //Example::IChol<Uplo::Upper,AlgoIChol::RightUnblockedOpt1>::TaskFunctor<CrsMatrixView>(U).apply(r_val);
  
  if (r_val != 0)  {
    cout << " Error = " << r_val << endl;
    return r_val;
  }
  cout << UU << endl;

  Kokkos::finalize();

  return 0;
}
