//@HEADER
// ************************************************************************
// 
//               ML: A Multilevel Preconditioner Package
//                 Copyright (2002) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ************************************************************************
//@HEADER

#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif

// The following header file contains macro definitions for ML. In particular, HAVE_ML_EPETRA,
// HAVE_ML_TEUCHOS, HAVE_ML_TRIUTILS are defines in this file.
#include "ml_config.h"

// the following code cannot be compiled without these Trilinos
// packages. Note that triutils is required in the examples only (to
// generate the linear system), not by the ML library
#if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_TEUCHOS) && defined(HAVE_ML_TRIUTILS) && defined(HAVE_ML_AZTECOO)

#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"
#include "Epetra_IntVector.h"
#include "Epetra_SerialDenseVector.h"
#include "Epetra_Vector.h"
#include "Epetra_VbrMatrix.h"
#include "Epetra_LinearProblem.h"
#include "AztecOO.h"
#include "Trilinos_Util_CrsMatrixGallery.h"

#include "ml_include.h"
#include "ml_MultiLevelPreconditioner.h"

using namespace Teuchos;
using namespace Trilinos_Util;

// *) inspired from ml_example_MultiLevelPreconditioner_viz.cpp (in the
//    examples subdirectory)
// *) define VIZ to visualize the aggregates (does not work for
//    all the aggregation schemes)
//
// Marzio Sala, SNL 9214, Nov-04

int main(int argc, char *argv[])
{
  
#ifdef EPETRA_MPI
  MPI_Init(&argc,&argv);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif

  int NumNodes = 65536;
  int NumPDEEqns = 1;

  VbrMatrixGallery Gallery("laplace_2d", Comm);
  Gallery.Set("problem_size", NumNodes);

  // retrive pointers for linear system matrix and linear problem
  Epetra_RowMatrix * A = Gallery.GetVbrMatrix(NumPDEEqns);
  Epetra_LinearProblem * Problem = Gallery.GetVbrLinearProblem();

  // Construct a solver object for this problem
  AztecOO solver(*Problem);

  double * x_coord = 0;
  double * y_coord = 0;
  double * z_coord = 0; // the problem is 2D, here z_coord will be NULL
  
  Gallery.GetCartesianCoordinates(x_coord, y_coord, z_coord);

  ParameterList MLList;
  MLList.set("output",8);
  MLList.set("max levels",10);
  MLList.set("increasing or decreasing","increasing");
  MLList.set("smoother: type", "symmetric Gauss-Seidel");

  // *) if a low number, it will use all the available processes
  // *) if a big number, it will use only processor 0 on the next level
  MLList.set("aggregation: next-level aggregates per process", 1);

  MLList.set("aggregation: type (level 0)", "Zoltan");
  MLList.set("aggregation: type (level 1)", "METIS");
  MLList.set("aggregation: type (level 2)", "Zoltan");

  int NumMyElements = A->RowMatrixRowMap().NumMyElements() / NumPDEEqns;
  vector<double> zz_coord(NumMyElements * NumPDEEqns * 2);
  for (int i = 0 ; i < NumMyElements ; ++i) {
    zz_coord[i] = x_coord[i];
    zz_coord[i + NumMyElements] = y_coord[i];
  }

  MLList.set("aggregation: zoltan coordinates", &zz_coord[0]);
  MLList.set("aggregation: zoltan dimensions", 2);

  // specify the reduction with respect to the previous level
  // (very small values can break the code)
  int ratio = 16;
  MLList.set("aggregation: global aggregates (level 0)", 
             NumNodes / ratio);
  MLList.set("aggregation: global aggregates (level 1)", 
             NumNodes / (ratio * ratio));
  MLList.set("aggregation: global aggregates (level 2)", 
             NumNodes / (ratio * ratio * ratio));

#ifdef VIZ
  MLList.set("viz: enable", true);
  MLList.set("viz: x-coordinates", x_coord);
  MLList.set("viz: y-coordinates", y_coord);
#endif

  // create the preconditioner object and compute hierarchy
  // See comments in "ml_example_epetra_preconditioner.cpp"

  ML_Epetra::MultiLevelPreconditioner * MLPrec = 
    new ML_Epetra::MultiLevelPreconditioner(*A, MLList, true);

#ifdef VIZ  
  MLPrec->VisualizeAggregates();
#endif

  solver.SetPrecOperator(MLPrec);
  solver.SetAztecOption(AZ_solver, AZ_cg_condnum);
  solver.SetAztecOption(AZ_output, 32);

  // solve with 500 iterations and 1e-12 tolerance  
  solver.Iterate(500, 1e-12);

  delete MLPrec;
  
  // compute the real residual

  double residual, diff;
  Gallery.ComputeResidualVbr(&residual);
  Gallery.ComputeDiffBetweenStartingAndExactSolutionsVbr(&diff);
  
  if( Comm.MyPID()==0 ) {
    cout << "||b-Ax||_2 = " << residual << endl;
    cout << "||x_exact - x||_2 = " << diff << endl;
  }

  // delete memory for coordinates
  if( x_coord ) delete [] x_coord;
  if( y_coord ) delete [] y_coord;
  if( z_coord ) delete [] z_coord;
  
  if (residual > 1e-5) {
    cout << "TEST FAILED!!!!" << endl;
    exit(EXIT_FAILURE);
  }

#ifdef EPETRA_MPI
  MPI_Finalize() ;
#endif

  cout << "TEST PASSED" << endl;
  exit(EXIT_SUCCESS);
  
}

#else

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  puts("Please configure ML with --enable-epetra --enable-teuchos");
  puts("--enable-aztecoo --enable-triutils");
  
  return 0;
}

#endif /* #if defined(ML_WITH_EPETRA) && defined(HAVE_ML_TEUCHOS) && defined(HAVE_ML_TRIUTILS) */

