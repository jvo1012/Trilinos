//
//  File: BlockArnoldiPetraEx.cpp
//
//  This example computes the specified eigenvalues of the discretized 2D Convection-Diffusion
//  equation using the block Arnoldi method.  This discretized operator is constructed as an
//  Epetra matrix, then passed into the AnasaziPetraMat to be used in the construction of the
//  Krylov decomposition.  The specifics of the block Arnoldi method can be set by the user.

#include "AnasaziPetraInterface.hpp"
#include "AnasaziBlockArnoldi.hpp"
#include "AnasaziConfigDefs.hpp"
#include "Epetra_CrsMatrix.h"

#ifdef EPETRA_MPI
#include "Epetra_MpiComm.h"
#include <mpi.h>
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"

int main(int argc, char *argv[]) {
	int ierr = 0, i, j;

#ifdef EPETRA_MPI

	// Initialize MPI

	MPI_Init(&argc,&argv);

	int size, rank; // Number of MPI processes, My process ID

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#else

	int size = 1; // Serial case (not using MPI)
	int rank = 0;

#endif


#ifdef EPETRA_MPI
	Epetra_MpiComm & Comm = *new Epetra_MpiComm(MPI_COMM_WORLD);
#else
	Epetra_SerialComm & Comm = *new Epetra_SerialComm();
#endif

	int MyPID = Comm.MyPID();
	int NumProc = Comm.NumProc();
	cout << "Processor "<<MyPID<<" of "<< NumProc << " is alive."<<endl;

	bool verbose = (MyPID==0);

	//  Dimension of the matrix
	int NumGlobalElements = 1000;

	// We will use zero based indices
	int IndexBase = 0;

	// Construct a Map that puts approximately the same number of
	// equations on each processor.

	Epetra_Map& Map = *new Epetra_Map(NumGlobalElements, 0, Comm);

	// Get update list and number of local equations from newly created Map.

	int NumMyElements = Map.NumMyElements();

	int * MyGlobalElements = new int[NumMyElements];
    	Map.MyGlobalElements(MyGlobalElements);

	// Create an integer vector NumNz that is used to build the Petra Matrix.
	// NumNz[i] is the Number of OFF-DIAGONAL term for the ith global equation
	// on this processor
	int * NumNz = new int[NumMyElements];

	// We are building a tridiagonal matrix
	// So we need 2 off-diagonal terms (except for the first and last equation)

	for (i=0; i<NumMyElements; i++) {
		if (MyGlobalElements[i]==0 || MyGlobalElements[i] == NumGlobalElements-1)
			NumNz[i] = 2;
		else
			NumNz[i] = 3;
	}

	// Create an Epetra_Matrix

	Epetra_CrsMatrix& A = *new Epetra_CrsMatrix(Copy, Map, NumNz);

	// Diffusion coefficient, can be set by user.
	double rho = 1.0;  

	// Add  rows one-at-a-time
	// Need some vectors to help

	const double one = 1.0;
	double *Values = new double[2];
	double h = one /(NumGlobalElements + one);
	double c = rho*h/ 2.0;
	Values[0] = -one-c; Values[1] = -one+c;
	int *Indices = new int[2];
	double diag = 2.0;
	int NumEntries;
	
	for (i=0; i<NumMyElements; i++)
	{
		if (MyGlobalElements[i]==0)
		{
			Indices[0] = 1;
			NumEntries = 1;
			assert(A.InsertGlobalValues(MyGlobalElements[i], NumEntries, Values+1, Indices)==0);
		}
		else if (MyGlobalElements[i] == NumGlobalElements-1)
		{
			Indices[0] = NumGlobalElements-2;
			NumEntries = 1;
			assert(A.InsertGlobalValues(MyGlobalElements[i], NumEntries, Values, Indices)==0);
		}
		else
		{
			Indices[0] = MyGlobalElements[i]-1;
			Indices[1] = MyGlobalElements[i]+1;
			NumEntries = 2;
			assert(A.InsertGlobalValues(MyGlobalElements[i], NumEntries, Values, Indices)==0);
		}
		// Put in the diagonal entry
		assert(A.InsertGlobalValues(MyGlobalElements[i], 1, &diag, MyGlobalElements+i)==0);
	}

	// Finish up
	assert(A.TransformToLocal()==0);
	A.SetTracebackMode(1); // Shutdown Epetra Warning tracebacks

	//************************************
	// Start the block Arnoldi iteration
	//***********************************
	//
	//  Variables used for the Block Arnoldi Method
	//
	int block = 5;
	int length = 30;
	int nev = 5;
	double tol = 1.0e-8;
	string which="LM";
	int step = 1;
	int restarts = 10;

	// create a PetraAnasaziVec. Note that the decision to make a view or
	// or copy is determined by the petra constructor called by Anasazi::PetraVec.
	// This is possible because I pass in arguements needed by petra.
	Anasazi::PetraVec<double> ivec(Map, block);
	ivec.MvRandom();

	// call the ctor that calls the petra ctor for a matrix
	Anasazi::PetraMat<double> Amat(A);	
	Anasazi::Eigenproblem<double> MyProblem(&Amat, &ivec);

	// initialize the Block Arnoldi solver
	Anasazi::BlockArnoldi<double> MyBlockArnoldi(MyProblem, tol, nev, length, block, 
						which, step, restarts);
	
	// inform the solver that the problem is symmetric
	//MyBlockArnoldi.setSymmetric(true);
	MyBlockArnoldi.setDebugLevel(0);

#ifdef UNIX
	Epetra_Time & timer = *new Epetra_Time(Comm);
#endif

	// iterate a few steps (if you wish)
	//MyBlockArnoldi.iterate(5);

	// solve the problem to the specified tolerances or length
	MyBlockArnoldi.solve();

#ifdef UNIX
	double elapsed_time = timer.ElapsedTime();
	double total_flops = A.Flops();
	double MFLOPs = total_flops/elapsed_time/1000000.0;
#endif

	// obtain results directly
	double* resids = MyBlockArnoldi.getResiduals();
	double* evalr = MyBlockArnoldi.getEvals(); 
	double* evali = MyBlockArnoldi.getiEvals();

	// retrieve eigenvectors
	Anasazi::PetraVec<double> evecr(Map, nev);
	MyBlockArnoldi.getEvecs( evecr );
	Anasazi::PetraVec<double> eveci(Map, nev);
	MyBlockArnoldi.getiEvecs( eveci );

	// output results to screen
	MyBlockArnoldi.currentStatus();

#ifdef UNIX
	if (verbose)
		cout << "\n\nTotal MFLOPs for Arnoldi = " << MFLOPs << " Elapsed Time = "<<  elapsed_time << endl;
#endif


	// Release all objects
	delete [] resids, evalr, evali;
	delete [] NumNz;
	delete [] Values;
	delete [] Indices;
	delete [] MyGlobalElements;

	delete &A;
	delete &Map;
	delete &Comm;

	return 0;
}
