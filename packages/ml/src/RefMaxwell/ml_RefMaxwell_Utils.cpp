#include "ml_config.h"
#if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_TEUCHOS)
#include "ml_RefMaxwell_Utils.h"
#include "ml_epetra.h"
#include "ml_epetra_utils.h"
#include "ml_mat_formats.h"
#include <fstream>

// ================================================ ====== ==== ==== == =
int ML_Epetra::CSR_getrow_ones(ML_Operator *data, int N_requested_rows, int requested_rows[],
   int allocated_space, int columns[], double values[], int row_lengths[])
{
   /* register (C++11 deprecates this storage class specifier) */ int    *bindx, j;
   int     *rowptr,  row, itemp;
   struct ML_CSR_MSRdata *input_matrix;
   ML_Operator *mat_in;

   row            = *requested_rows;
   mat_in = (ML_Operator *) data;
   input_matrix = (struct ML_CSR_MSRdata *) ML_Get_MyGetrowData(mat_in);
   rowptr = input_matrix->rowptr;
   itemp = rowptr[row];
   *row_lengths = rowptr[row+1] - itemp;


   if (*row_lengths > allocated_space) {
    ML_avoid_unused_param( (void *) &N_requested_rows);
    return(0);
  }

   bindx  = &(input_matrix->columns[itemp]);
   for (j = 0 ; j < *row_lengths; j++) {
      *columns++ = *bindx++;
   }
   for (j = 0 ; j < *row_lengths; j++) {
      *values++  = 1;
   }
   return(1);
}

// ================================================ ====== ==== ==== == =
void ML_Epetra::IVOUT(const Epetra_IntVector & A, const char *of){
  std::ofstream os(of);
  int i;
  int NumProc=A.Map().Comm().NumProc();
  int MyPID  =A.Map().Comm().MyPID();

  for (int iproc=0; iproc < NumProc; iproc++) {
    if (MyPID==iproc) {
      int MyLength = A.MyLength();
      for (i=0; i<MyLength; i++) {
          os.width(20);
          os << A[i]<<std::endl;
      }
      os << std::flush;
    }/*end if*/
    // Do a few global ops to give I/O a chance to complete
    A.Map().Comm().Barrier();
    A.Map().Comm().Barrier();
    A.Map().Comm().Barrier();
  }/*end for*/
}/*end IVOUT*/

// ================================================ ====== ==== ==== == =
// Copied from ml_agg_genP.c
static int ML_Aux_Getrow(ML_Operator *data, int N_requested_rows, int requested_rows[],
                         int allocated_space, int columns[], double values[],
                         int row_lengths[])
{
  int ierr;
  int i, j, count, mod;
  int BlockCol, BlockRow, RowMod;
  double DiagValue = 0.0;
  int DiagID;
  int* Filter;

  ierr = (*(data->aux_data->aux_func_ptr))(data, N_requested_rows, requested_rows,
                                      allocated_space, columns, values, row_lengths);
  if (ierr == 0)
    return(0);

  if (N_requested_rows != 1) {
    fprintf(stderr, "ML_Aux_Getrow() works only if N_requested_rows == 1\n"
            "(file %s, line %d)\n",
            __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  /* new part */
  mod       = data->invec_leng / data->aux_data->filter_size;
  BlockRow  = requested_rows[0] / mod;
  RowMod    = requested_rows[0] % mod;
  Filter    = data->aux_data->filter[BlockRow];
  count     = 0;
  DiagID    = -1;
  DiagValue = 0.0;

  for (i = 0 ; i < row_lengths[0] ; ++i)
  {
    BlockCol = columns[i] / mod;

    if (BlockCol ==  BlockRow)
    {
      columns[count] = columns[i];
      values[count]  = values[i];
      if (requested_rows[0] == columns[i]) DiagID = count;
      ++count;
      goto after;
    }

    /* different block col, same equation */
    for (j = 0 ; j < Filter[0] ; ++j)
    {
      /* look for elements to discard */
      if (Filter[j + 1] == BlockCol)
      {
        /* We modify the matrix diagonal so that the constant vector */
        /* is still in the null space (assuming that the constant    */
        /* was in the original matrix null space). For block pde     */
        /* systems we need to check that both the column and the row */
        /* correspond to the same DOF within the node.               */
        /* Note: This code will not preserve the null space if it is */
        /* not given by piecewise constants.                         */

        if (columns[i] % mod == RowMod) {
          DiagValue += values[i];
        }
        goto after;
      }
    }
    columns[count] = columns[i];
    values[count]  = values[i];
    ++count;
after:
    continue;
  }

  if (DiagID == -1)
  {
    fprintf(stderr, "Diagonal not defined for row %d\n", requested_rows[0]);
  }

  values[DiagID] += DiagValue;
  row_lengths[0] = count;

  return(ierr);
}


// ================================================ ====== ==== ==== == =
int RefMaxwell_SetupCoordinates(ML_Operator* A, Teuchos::ParameterList &List_, double *&coordx, double *&coordy, double *&coordz)
// Coppied From int ML_Epetra::MultiLevelPreconditioner::SetupCoordinates()
{
  double* in_x_coord = 0;
  double* in_y_coord = 0;
  double* in_z_coord = 0;
  int NumPDEEqns_ =1;  // Always use 1 because A is a nodal matrix.

  // For node coordinates
  in_x_coord = List_.get("x-coordinates", (double *)0);
  in_y_coord = List_.get("y-coordinates", (double *)0);
  in_z_coord = List_.get("z-coordinates", (double *)0);

  if (!(in_x_coord == 0 && in_y_coord == 0 && in_z_coord == 0))
    {
      ML_Operator* AAA = A;

      int n = AAA->invec_leng, Nghost = 0;

      if (AAA->getrow->pre_comm)
        {
          if (AAA->getrow->pre_comm->total_rcv_length <= 0)
            ML_CommInfoOP_Compute_TotalRcvLength(AAA->getrow->pre_comm);
          Nghost = AAA->getrow->pre_comm->total_rcv_length;
        }

      std::vector<double> tmp(Nghost + n);
      for (int i = 0 ; i < Nghost + n ; ++i)
        tmp[i] = 0.0;

      n /= NumPDEEqns_;
      Nghost /= NumPDEEqns_;

      if (in_x_coord)
        {
          double* x_coord = (double *) ML_allocate(sizeof(double) * (Nghost+n));

          for (int i = 0 ; i < n ; ++i)
            tmp[i * NumPDEEqns_] = in_x_coord[i];

          ML_exchange_bdry(&tmp[0],AAA->getrow->pre_comm, NumPDEEqns_ * n,
                           AAA->comm, ML_OVERWRITE,NULL);

          for (int i = 0 ; i < n + Nghost ; ++i)
            x_coord[i] = tmp[i * NumPDEEqns_];

          coordx = x_coord;
        }

      if (in_y_coord)
        {
          double* y_coord = (double *) ML_allocate(sizeof(double) * (Nghost+n));

          for (int i = 0 ; i < n ; ++i)
            tmp[i * NumPDEEqns_] = in_y_coord[i];

          ML_exchange_bdry(&tmp[0],AAA->getrow->pre_comm, NumPDEEqns_ * n,
                           AAA->comm, ML_OVERWRITE,NULL);

          for (int i = 0 ; i < n + Nghost ; ++i)
            y_coord[i] = tmp[i * NumPDEEqns_];

          coordy = y_coord;
        }

      if (in_z_coord)
        {
          double* z_coord = (double *) ML_allocate(sizeof(double) * (Nghost+n));

          for (int i = 0 ; i < n ; ++i)
            tmp[i * NumPDEEqns_] = in_z_coord[i];

          ML_exchange_bdry(&tmp[0],AAA->getrow->pre_comm, NumPDEEqns_ * n,
                           AAA->comm, ML_OVERWRITE,NULL);

          for (int i = 0 ; i < n + Nghost ; ++i)
            z_coord[i] = tmp[i * NumPDEEqns_];

          coordz = z_coord;
        }

    } // if (!(in_x_coord == 0 && in_y_coord == 0 && in_z_coord == 0))

  return(0);
 }

// ================================================ ====== ==== ==== == =
int RefMaxwell_SetupMaterial(ML_Operator* A, Teuchos::ParameterList &List_, double *&material)
// Modified From int ML_Epetra::MultiLevelPreconditioner::SetupCoordinates()
{
  int NumPDEEqns_ =1;  // Always use 1 because A is a nodal matrix.

  // For node coordinates
  double * in_coord = List_.get("material coordinates", (double *)0);

  if (in_coord != 0)  {
      ML_Operator* AAA = A;

      int n = AAA->invec_leng, Nghost = 0;

      if (AAA->getrow->pre_comm)
        {
          if (AAA->getrow->pre_comm->total_rcv_length <= 0)
            ML_CommInfoOP_Compute_TotalRcvLength(AAA->getrow->pre_comm);
          Nghost = AAA->getrow->pre_comm->total_rcv_length;
        }

      std::vector<double> tmp(Nghost + n);
      for (int i = 0 ; i < Nghost + n ; ++i)
        tmp[i] = 0.0;

      n /= NumPDEEqns_;
      Nghost /= NumPDEEqns_;

      if (in_coord)
        {
          double* x_coord = (double *) ML_allocate(sizeof(double) * (Nghost+n));

          for (int i = 0 ; i < n ; ++i)
            tmp[i * NumPDEEqns_] = in_coord[i];

          ML_exchange_bdry(&tmp[0],AAA->getrow->pre_comm, NumPDEEqns_ * n,
                           AAA->comm, ML_OVERWRITE,NULL);

          for (int i = 0 ; i < n + Nghost ; ++i)
            x_coord[i] = tmp[i * NumPDEEqns_];

          material = x_coord;
        }


    }

  return(0);
 }

// ================================================ ====== ==== ==== == =
// Modified from ml_agg_genP.c
static void ML_Init_Material(ML_Operator* A, Teuchos::ParameterList &List) {
  int i, j, n, count, num_PDEs, BlockRow, BlockCol;
  double threshold;
  int* columns;
  double* values;
  int allocated, entries = 0;
  int** filter;
  //  int Nghost;


  // Boundary exchange the material vector
  double *material=0;
  RefMaxwell_SetupMaterial(A,List,material);
  TEUCHOS_TEST_FOR_EXCEPTION(material == 0, std::logic_error,"ML_Init_Material: material vector not found");

  num_PDEs = A->num_PDEs;
  threshold = A->aux_data->threshold;

  ML_Operator_AmalgamateAndDropWeak(A, num_PDEs, 0.0);
  n = A->invec_leng;
  //  Nghost = ML_CommInfoOP_Compute_TotalRcvLength(A->getrow->pre_comm);

  filter = (int**) ML_allocate(sizeof(int*) * n);

  allocated = 128;
  columns = (int *)    ML_allocate(allocated * sizeof(int));
  values  = (double *) ML_allocate(allocated * sizeof(double));


  for (i = 0 ; i < n ; ++i) {
    BlockRow = i;

    ML_get_matrix_row(A,1,&i,&allocated,&columns,&values, &entries,0);
    count = 0;
    for (j = 0 ; j < entries ; ++j) {
      BlockCol = columns[j];
      if (  (i != columns[j]) &&
            (std::abs(log10(material[BlockRow]) - log10(material[BlockCol])) > threshold) ) {
        columns[count++] = columns[j];
      }
    }
    
    /* insert the rows */
    filter[BlockRow] = (int*) ML_allocate(sizeof(int) * (count + 1));
    filter[BlockRow][0] = count;

    for (j = 0 ; j < count ; ++j)
      filter[BlockRow][j + 1] = columns[j];
  }

  ML_free(columns);
  ML_free(values);

  ML_Operator_UnAmalgamateAndDropWeak(A, num_PDEs, 0.0);

  A->aux_data->aux_func_ptr  = A->getrow->func_ptr;
  A->getrow->func_ptr = ML_Aux_Getrow;
  A->aux_data->filter = filter;
  A->aux_data->filter_size = n;

  // Cleanup
  ML_free(material);

}

// ================================================ ====== ==== ==== == =
// Copied from ml_agg_genP.c
static void ML_Init_Aux(ML_Operator* A, Teuchos::ParameterList &List) {
  int i, j, n, count, num_PDEs, BlockRow, BlockCol;
  double threshold;
  int* columns;
  double* values;
  int allocated, entries = 0;
  int N_dimensions;
  int DiagID;
  double DiagValue;
  int** filter;
  double dist;
  double *LaplacianDiag;
  int     Nghost;


  // Boundary exchange the material vector
  double *x_coord=0, *y_coord=0, *z_coord=0;
  RefMaxwell_SetupCoordinates(A,List,x_coord,y_coord,z_coord);
  int dim=(x_coord!=0) + (y_coord!=0) + (z_coord!=0);

  /* Sanity Checks */
  if(dim == 0 || ((!x_coord && (y_coord || z_coord)) || (x_coord && !y_coord && z_coord))){
    std::cerr<<"Error: Coordinates not defined.  This is necessary for aux aggregation (found "<<dim<<" coordinates).\n";
    exit(-1);
  }

  num_PDEs = A->num_PDEs;
  N_dimensions = dim;
  threshold = A->aux_data->threshold;

  ML_Operator_AmalgamateAndDropWeak(A, num_PDEs, 0.0);
  n = A->invec_leng;
  Nghost = ML_CommInfoOP_Compute_TotalRcvLength(A->getrow->pre_comm);

  LaplacianDiag = (double *) ML_allocate((A->getrow->Nrows+Nghost+1)*
                                         sizeof(double));

  filter = (int**) ML_allocate(sizeof(int*) * n);

  allocated = 128;
  columns = (int *)    ML_allocate(allocated * sizeof(int));
  values  = (double *) ML_allocate(allocated * sizeof(double));

  for (i = 0 ; i < n ; ++i) {
    BlockRow = i;
    DiagID = -1;
    DiagValue = 0.0;

    ML_get_matrix_row(A,1,&i,&allocated,&columns,&values, &entries,0);

    for (j = 0; j < entries; j++) {
      BlockCol = columns[j];
      if (BlockRow != BlockCol) {
        dist = 0.0;

        switch (N_dimensions) {
        case 3:
          dist += (z_coord[BlockRow] - z_coord[BlockCol]) * (z_coord[BlockRow] - z_coord[BlockCol]);
        case 2:
          dist += (y_coord[BlockRow] - y_coord[BlockCol]) * (y_coord[BlockRow] - y_coord[BlockCol]);
        case 1:
          dist += (x_coord[BlockRow] - x_coord[BlockCol]) * (x_coord[BlockRow] - x_coord[BlockCol]);
        }

        if (dist == 0.0) {
          printf("node %d = %e ", i, x_coord[BlockRow]);
          if (N_dimensions > 1) printf(" %e ", y_coord[BlockRow]);
          if (N_dimensions > 2) printf(" %e ", z_coord[BlockRow]);
          printf("\n");
          printf("node %d = %e ", j, x_coord[BlockCol]);
          if (N_dimensions > 1) printf(" %e ", y_coord[BlockCol]);
          if (N_dimensions > 2) printf(" %e ", z_coord[BlockCol]);
          printf("\n");
          printf("Operator has inlen = %d and outlen = %d\n",
                 A->invec_leng, A->outvec_leng);
        }

        dist = 1.0 / dist;
        DiagValue += dist;
      }
      else if (columns[j] == i) {
        DiagID = j;
      }
    }

    if (DiagID == -1) {
      fprintf(stderr, "ERROR: matrix has no diagonal!\n"
              "ERROR: (file %s, line %d)\n",
              __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
    LaplacianDiag[BlockRow] = DiagValue;
  }
  if ( A->getrow->pre_comm != NULL )
     ML_exchange_bdry(LaplacianDiag,A->getrow->pre_comm,A->getrow->Nrows,
                      A->comm, ML_OVERWRITE,NULL);


  for (i = 0 ; i < n ; ++i) {
    BlockRow = i;

    ML_get_matrix_row(A,1,&i,&allocated,&columns,&values, &entries,0);

    for (j = 0; j < entries; j++) {
      BlockCol = columns[j];
      if (BlockRow != BlockCol) {
        dist = 0.0;
        switch (N_dimensions) {
        case 3:
          dist += (z_coord[BlockRow] - z_coord[BlockCol]) * (z_coord[BlockRow] - z_coord[BlockCol]);
        case 2:
          dist += (y_coord[BlockRow] - y_coord[BlockCol]) * (y_coord[BlockRow] - y_coord[BlockCol]);
        case 1:
          dist += (x_coord[BlockRow] - x_coord[BlockCol]) * (x_coord[BlockRow] - x_coord[BlockCol]);
        }

        dist = 1.0 / dist;
        values[j] = dist;
      }
    }

    count = 0;
    for (j = 0 ; j < entries ; ++j) {
      if (  (i != columns[j]) &&
            (values[j]*values[j] <
       LaplacianDiag[BlockRow]*LaplacianDiag[columns[j]]*threshold*threshold)){
        columns[count++] = columns[j];
      }
    }

    /* insert the rows */
    filter[BlockRow] = (int*) ML_allocate(sizeof(int) * (count + 1));
    filter[BlockRow][0] = count;

    for (j = 0 ; j < count ; ++j)
      filter[BlockRow][j + 1] = columns[j];

  }

  ML_free(columns);
  ML_free(values);

  ML_free(LaplacianDiag);

  ML_Operator_UnAmalgamateAndDropWeak(A, num_PDEs, 0.0);

  A->aux_data->aux_func_ptr  = A->getrow->func_ptr;
  A->getrow->func_ptr = ML_Aux_Getrow;
  A->aux_data->filter = filter;
  A->aux_data->filter_size = n;

  // Cleanup
  ML_free(x_coord);
  ML_free(y_coord);
  ML_free(z_coord);

}

// ================================================ ====== ==== ==== == =
// Copied from ml_agg_genP.c
static void ML_Finalize_Aux(ML_Operator *A)
{
  int i;
  A->getrow->func_ptr = A->aux_data->aux_func_ptr;
  A->aux_data->aux_func_ptr = 0;
  for (i = 0 ; i < A->aux_data->filter_size ; ++i)
    ML_free((A->aux_data->filter[i]));
  ML_free(A->aux_data->filter);
}

// ================================================ ====== ==== ==== == =
static void ML_Smooth_Prolongator(ML_Operator *Amat, ML_Aggregate * ag, int numSmSweeps, ML_Operator *Pmatrix, ML_Operator *Pfinal) {
  ML_Comm *comm = Amat->comm;
  ML_Krylov   *kdata;
  struct ML_AGG_Matrix_Context widget;
  double *dampingFactors; /* coefficients of prolongator smoother */
  int level = 0;
  ML_Operator *tmpmat1=NULL,*tmpmat2=NULL, *AGGsmoother=NULL, *Ptemp=NULL;
  struct MLSthing *mls_widget = NULL;
  int Nfine    = Amat->outvec_leng;
  int ii;
  double max_eigen = 0;
  ML_Operator *blockMat = NULL;

  if ( ag->smoothP_damping_factor == 0.0 || numSmSweeps == 0 ) 
    return;

  widget.Adiag = NULL;
  widget.near_bdry = NULL;

  if (Amat->num_PDEs < ag->num_PDE_eqns) Amat->num_PDEs = ag->num_PDE_eqns;
  if (ag->block_scaled_SA == 1) {
    /*
      Create block scaled and compute its eigenvalues
      a) if the user has requested it, save this Amat into
      the aggregate data structure.
    */
    mls_widget = ML_Smoother_Create_MLS();
    ML_Gen_BlockScaledMatrix_with_Eigenvalues(Amat, -1, NULL,
                                              &blockMat, mls_widget);
    max_eigen = blockMat->lambda_max;
  }
  else    
    max_eigen = Amat->lambda_max;


  if ( comm->ML_mypid == 0 && ML_Get_PrintLevel() > 5)
    printf("Calculating eigenvalue estimate using ");    
  switch( Amat->spectral_radius_scheme ) {      
  case ML_USE_CG:  /* compute it using CG */
    if ( comm->ML_mypid == 0 && ML_Get_PrintLevel() > 5)
      printf("CG method\n");
    
    kdata = ML_Krylov_Create( comm );
    ML_Krylov_Set_PrintFreq( kdata, 0 );
    ML_Krylov_Set_MaxIterations(kdata, Amat->spectral_radius_max_iters);
    ML_Krylov_Set_ComputeEigenvalues( kdata );
    ML_Krylov_Set_Amatrix(kdata, Amat);
    ML_Krylov_Solve(kdata, Nfine, NULL, NULL);
    /* This is a bit screwy in that max_eigen corresponds to Dinv A. */
    /* This is good for Cheby smoothers but bad for smoothed agg     */
    /* if we actually filter A (significantly) as we should compute  */
    /* the eigenvalue of Dinv Afilt.                                 */
    max_eigen = ML_Krylov_Get_MaxEigenvalue(kdata);
    
    Amat->lambda_max = max_eigen;
    Amat->lambda_min = kdata->ML_eigen_min;
    ML_Krylov_Destroy( &kdata );
    if ( max_eigen <= 0.0 ) {
      printf("Gen_Prolongator warning : max eigen <= 0.0 \n");
      max_eigen = 1.0;
    }      
    break;
    
  case ML_USE_ANASAZI: /* Use Anasazi */
#if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_ANASAxI) && defined(HAVE_ML_TEUCHOS)
    ML_Anasazi_Get_SpectralNorm_Anasazi(Amat, 0, 10, 1e-5,
                                        ML_FALSE, ML_TRUE, &max_eigen);
    if ( comm->ML_mypid == 0 && ML_Get_PrintLevel() > 5)
      printf("Anasazi\n");
#else
    fprintf(stderr,
            "--enable-epetra --enable-anasazi --enable-teuchos required\n"
            "(file %s, line %d)\n",
            __FILE__,
            __LINE__);
    exit(EXIT_FAILURE);
#endif
    Amat->lambda_max = max_eigen;
    Amat->lambda_min = -12345.6789;
    if ( max_eigen <= 0.0 ) {
      printf("Gen_Prolongator warning : max eigen <= 0.0 \n");
      max_eigen = 1.0;
    }      
    break;
    
  case ML_USE_POWER: /* use ML's power method */
    if ( comm->ML_mypid == 0 && ML_Get_PrintLevel() > 5)
      printf("power method\n");
    kdata = ML_Krylov_Create( comm );
    ML_Krylov_Set_PrintFreq( kdata, 0 );
    ML_Krylov_Set_MaxIterations(kdata, Amat->spectral_radius_max_iters);
    ML_Krylov_Set_ComputeNonSymEigenvalues( kdata );
    ML_Krylov_Set_Amatrix(kdata, Amat);
    ML_Krylov_Solve(kdata, Nfine, NULL, NULL);
    max_eigen = ML_Krylov_Get_MaxEigenvalue(kdata);
    Amat->lambda_max = max_eigen;
    Amat->lambda_min = kdata->ML_eigen_min;
    ML_Krylov_Destroy( &kdata );
    if ( max_eigen <= 0.0 ) {
      printf("Gen_Prolongator warning : max eigen <= 0.0 \n");
      max_eigen = 1.0;
    }
    
    break;
    
  default: /* using matrix max norm */
    if ( comm->ML_mypid == 0 && ML_Get_PrintLevel() > 5)
      printf("matrix max norm\n");
    max_eigen = ML_Operator_MaxNorm(Amat, ML_TRUE);
    break;
    
  } /* switch( Amat->spectral_radius_scheme ) */    
  
  widget.omega  = ag->smoothP_damping_factor / max_eigen;
  if ( comm->ML_mypid == 0 && 7 < ML_Get_PrintLevel())
    printf("Gen_Prolongator (level %d) : Max eigenvalue = %2.4e\n",
           ag->cur_level, max_eigen);

  /******* Smoothing *******/
  dampingFactors = (double *) ML_allocate( sizeof(double) * numSmSweeps );
  if (numSmSweeps == 1)
    dampingFactors[0] = ag->smoothP_damping_factor;
  else
    /* Calculate the proper Chebyshev polynomial coefficients. */
    ML_AGG_Calculate_Smoothing_Factors(numSmSweeps, dampingFactors);
    
  if ( comm->ML_mypid == 0 && 5 < ML_Get_PrintLevel() ) {
    printf("\n");
    for (ii=0; ii<numSmSweeps; ii++)
      {
        if (ag->minimizing_energy == -1)
          {
            printf("\nProlongator smoother (level %d) : damping factor #%d = %2.4e\nProlongator smoother (level %d) : ( = %2.4e / %2.4e)\nNon-smoothed restriction is used.\n",
                   level, ii+1, dampingFactors[ii]/ max_eigen, level,
                   dampingFactors[ii], max_eigen );
          }
        else
          {
            printf("Prolongator/Restriction smoother (level %d) : damping factor #%d = %2.4e\nProlongator/Restriction smoother (level %d) : ( = %2.4e / %2.4e)\n",
                   level, ii+1, dampingFactors[ii]/ max_eigen, level,
                   dampingFactors[ii], max_eigen );
          }
        printf("\n");
      }
  }
  
  /* Create the prolongator smoother operator, I-omega*inv(D)*A. */
  AGGsmoother = ML_Operator_Create(comm);
  //    ML_Operator_Set_Label(AGGsmoother,"Prolongator smoother");
  widget.drop_tol = ag->drop_tol_for_smoothing;
  widget.Amat   = Amat;
  widget.aggr_info = ag->aggr_info[level];
  ML_Operator_Set_ApplyFuncData(AGGsmoother, widget.Amat->invec_leng,
                                widget.Amat->outvec_leng, &widget,
                                widget.Amat->matvec->Nrows, NULL, 0);
  ML_Operator_Set_Getrow(AGGsmoother,
                         widget.Amat->getrow->Nrows,
#ifdef USE_MOREACCURATE
                         ML_AGG_JacobiMoreAccurate_Getrows
#else
                           ML_AGG_JacobiSmoother_Getrows
#endif
                         );
  ML_CommInfoOP_Clone(&(AGGsmoother->getrow->pre_comm),
                      widget.Amat->getrow->pre_comm);
  tmpmat2 = Pmatrix;
  for (ii=0; ii < numSmSweeps; ii++) {
    /* Set the appropriate prolongator smoother damping factor. */
    widget.omega  = dampingFactors[ii] / max_eigen;
    
    if (ii < numSmSweeps-1) {
      tmpmat1 = tmpmat2;
      tmpmat2 = ML_Operator_Create(Amat->comm);
    }
    else {
      tmpmat1 = tmpmat2;
      tmpmat2 = Pfinal;
    }
    
    if (ag->block_scaled_SA == 1) {
      /* Computed the following:
       *    a) turn off the usual 2 mat mult.
       *    b) Ptemp = A*P
       *    c) Ptemp = Dinv*Ptemp;
       *    d) do an ML_Operator_Add() with the original P.
       */
      Ptemp = ML_Operator_Create(Amat->comm);
      ML_2matmult(Amat, tmpmat1, Ptemp, ML_CSR_MATRIX );
      ML_AGG_DinvP(Ptemp, mls_widget, Amat->num_PDEs, ROW_SCALE_WITH_D);
      
      ML_Operator_Add(tmpmat1, Ptemp, tmpmat2, ML_CSR_MATRIX,
                      -dampingFactors[ii] / max_eigen);
      ML_Operator_Destroy(&Ptemp);
    }
    else
      ML_2matmult(AGGsmoother, tmpmat1, tmpmat2, ML_CSR_MATRIX );
    
    /*Free intermediate matrix I-omega*inv(D)*A, as long as it's not Ptent.*/
    if (tmpmat1 && tmpmat1 != Pmatrix) ML_Operator_Destroy(&tmpmat1);
    
  } /* for (ii=0; ii < numSmSweeps; ii++) */
  ML_Operator_Destroy(&AGGsmoother);
  ML_free(dampingFactors);
  if (mls_widget != NULL) ML_Smoother_Destroy_MLS(mls_widget);

}/*end ML_Smooth_Prolongator */



// ================================================ ====== ==== ==== == =
int ML_Epetra::RefMaxwell_Aggregate_Nodes(const Epetra_CrsMatrix & A, Teuchos::ParameterList & List, ML_Comm * ml_comm, std::string PrintMsg,
                                          ML_Aggregate_Struct *& MLAggr,ML_Operator *&P, int &NumAggregates){

  /* Output level */
  bool verbose, very_verbose;
  int OutputLevel = List.get("ML output", -47);
  if(OutputLevel == -47) OutputLevel = List.get("output", 1);
  if(OutputLevel>=15) very_verbose=verbose=true;
  if(OutputLevel > 5) {very_verbose=false;verbose=true;}
  else very_verbose=verbose=false;

  /* Wrap A in a ML_Operator */
  ML_Operator* A_ML = ML_Operator_Create(ml_comm);
  ML_Operator_WrapEpetraCrsMatrix(const_cast<Epetra_CrsMatrix*>(&A),A_ML);

  /* Pull Teuchos Options */
  std::string CoarsenType  = List.get("aggregation: type", "Uncoupled");
  double Threshold         = List.get("aggregation: threshold", 0.0);
  double RowSum_Threshold  = List.get("aggregation: rowsum threshold", -1.0);
  double DampingFactor     = List.get("aggregation: damping factor", 0.0);
  int PSmSweeps            = List.get("aggregation: smoothing sweeps", 1);
  std::string EigType      = List.get("eigen-analysis: type","cg");
  int NumEigenIts          = List.get("eigen-analysis: iterations",10);

  int NodesPerAggr    = List.get("aggregation: nodes per aggregate",
                                  ML_Aggregate_Get_OptimalNumberOfNodesPerAggregate());
  bool UseAux         = List.get("aggregation: aux: enable",false);
  double AuxThreshold = List.get("aggregation: aux: threshold",0.0);
  int  MaxAuxLevels   = List.get("aggregation: aux: max levels",10);

  bool UseMaterial    = List.get("aggregation: material: enable",false);
  double MatThreshold = List.get("aggregation: material: threshold",0.0);
  int  MaxMatLevels   = List.get("aggregation: material: max levels",10);
  ML_Operator * smooP;
  int doQR            = (int) List.get("aggregation: do qr",false);

  // FIXME:  We need to allow this later
  TEUCHOS_TEST_FOR_EXCEPTION(UseAux && UseMaterial, std::logic_error,"RefMaxwell_Aggregate_Nodes: Cannot use material and aux aggregation at the same time");


  ML_Aggregate_Create(&MLAggr);
  ML_Aggregate_Set_MaxLevels(MLAggr, 2);
  ML_Aggregate_Set_StartLevel(MLAggr, 0);
  ML_Aggregate_Set_Threshold(MLAggr, Threshold);
  ML_Aggregate_Set_RowSum_Threshold(MLAggr, RowSum_Threshold);
  ML_Aggregate_Set_MaxCoarseSize(MLAggr,1);
  MLAggr->cur_level = 0;
  ML_Aggregate_Set_Reuse(MLAggr);
  ML_Aggregate_Set_Do_QR(MLAggr,doQR);

  ML_Aggregate_Set_DampingFactor(MLAggr,DampingFactor);  
  ML_Aggregate_Set_DampingSweeps(MLAggr,PSmSweeps,0);
  if( EigType == "cg" )                ML_Operator_Set_SpectralNormScheme_Calc(A_ML);
  else if( EigType == "Anorm" )        ML_Operator_Set_SpectralNormScheme_Anorm(A_ML);
  else if( EigType == "Anasazi" )      ML_Operator_Set_SpectralNormScheme_Anasazi(A_ML);
  else if( EigType == "power-method" ) ML_Operator_Set_SpectralNormScheme_PowerMethod(A_ML);
  else {
    if(!A.Comm().MyPID()) printf("%s Unsupported (1,1) block eigenvalue type(%s), resetting to cg\n",PrintMsg.c_str(),EigType.c_str());
    ML_Operator_Set_SpectralNormScheme_Calc(A_ML);
  }
  ML_Operator_Set_SpectralNorm_Iterations(A_ML, NumEigenIts);

  MLAggr->keep_agg_information = 1;
  P = ML_Operator_Create(ml_comm);

  /* Process Teuchos Options */
  if (CoarsenType == "Uncoupled")
    ML_Aggregate_Set_CoarsenScheme_Uncoupled(MLAggr);
  else if (CoarsenType == "Uncoupled-MIS"){
    ML_Aggregate_Set_CoarsenScheme_UncoupledMIS(MLAggr);
  }
  else if (CoarsenType == "METIS"){
    ML_Aggregate_Set_CoarsenScheme_METIS(MLAggr);
    ML_Aggregate_Set_NodesPerAggr(0, MLAggr, 0, NodesPerAggr);
  }/*end if*/
  else {
    if(!A.Comm().MyPID()) printf("%s Unsupported (1,1) block aggregation type(%s), resetting to uncoupled-mis\n",PrintMsg.c_str(),CoarsenType.c_str());
    ML_Aggregate_Set_CoarsenScheme_UncoupledMIS(MLAggr);
  }

  /* Setup Aux Data (aux) */
  if(UseAux) {
    A_ML->aux_data->enable=1;
    A_ML->aux_data->threshold=AuxThreshold;
    A_ML->aux_data->max_level=MaxAuxLevels;
    ML_Init_Aux(A_ML,List);
    if(verbose && !A.Comm().MyPID()) {
      printf("%s Using auxiliary matrix\n",PrintMsg.c_str());
      printf("%s aux threshold = %e\n",PrintMsg.c_str(),A_ML->aux_data->threshold);
    }
  }

  /* Setup Aux Data (material) */
  if(UseMaterial) {
    A_ML->aux_data->enable=1;
    A_ML->aux_data->threshold=MatThreshold;
    A_ML->aux_data->max_level=MaxMatLevels;
    ML_Init_Material(A_ML,List);
    if(verbose && !A.Comm().MyPID()) {
      printf("%s Using material matrix\n",PrintMsg.c_str());
      printf("%s material threshold = %e\n",PrintMsg.c_str(),A_ML->aux_data->threshold);
    }
  }

  /* Aggregate Nodes */
  int printlevel=ML_Get_PrintLevel();
  if(verbose) ML_Set_PrintLevel(10);
  NumAggregates = ML_Aggregate_Coarsen(MLAggr,A_ML, &P, ml_comm);

  /* Do prolongator smoothing, if requested */
  if(PSmSweeps && DampingFactor != 0.0) {
    if(verbose && !A.Comm().MyPID()) printf("%s Smoothing Prolongator w/ Damping Factor %e\n",PrintMsg.c_str(),DampingFactor);
    smooP = ML_Operator_Create(ml_comm);
    ML_Smooth_Prolongator(A_ML,MLAggr,PSmSweeps,P,smooP);
    ML_Operator_Destroy(&P);
    P = smooP;
  }

 if(verbose) ML_Set_PrintLevel(printlevel);
  if(very_verbose) printf("[%d] %s %d aggregates created invec_leng=%d\n",A.Comm().MyPID(),PrintMsg.c_str(),NumAggregates,P->invec_leng);

  if(verbose){
    int globalAggs=0;
    A.Comm().SumAll(&NumAggregates,&globalAggs,1);
    if(!A.Comm().MyPID()) {
      printf("%s Aggregation threshold = %e\n",PrintMsg.c_str(),Threshold);
      printf("%s Global aggregates     = %d\n",PrintMsg.c_str(),globalAggs);
    }
  }

  /* Cleanup */
  ML_qr_fix_Destroy();
  if(UseAux)      ML_Finalize_Aux(A_ML);
  if(UseMaterial) ML_Finalize_Aux(A_ML);
  ML_Operator_Destroy(&A_ML);

  return 0;
}

// ================================================ ====== ==== ==== == =
// Build the edge nullspace
Epetra_MultiVector* ML_Epetra::Build_Edge_Nullspace(const Epetra_CrsMatrix & D0_Clean_Matrix,  const Teuchos::ArrayRCP<int> BCedges, Teuchos::ParameterList & List_,bool verbose_)
{
  Epetra_MultiVector *nullspace;
  double ** d_coords;
  const Epetra_Map & NodeMap = D0_Clean_Matrix.DomainMap();
  const Epetra_Map & EdgeMap = D0_Clean_Matrix.RangeMap();
  const Epetra_Comm & Comm   = D0_Clean_Matrix.Comm();

  /* Check the List - Do we have a nullspace pre-provided? */
  std::string nulltype=List_.get("null space: type","default vectors");
  double* nullvecs=List_.get("null space: vectors",(double*)0);
  int dim=List_.get("null space: dimension",0);
  if (nulltype=="pre-computed" && nullvecs && (dim==2 || dim==3)){
    /* Build a multivector out of it */
    if(verbose_ && !Comm.MyPID()) printf("Using pre-computed nullspace\n");
    int Ne=EdgeMap.NumMyElements();
    d_coords=new double*[dim];
    d_coords[0]=nullvecs;
    d_coords[1]=&nullvecs[Ne];
    if(dim==3) d_coords[2]=&nullvecs[2*Ne];
    nullspace=new Epetra_MultiVector(View,EdgeMap,d_coords,dim);
  }
  else{
    if(verbose_ && !Comm.MyPID()) printf("Building nullspace from scratch\n");
    /* Pull the (nodal) coordinates from Teuchos */
    double * xcoord=List_.get("x-coordinates",(double*)0);
    double * ycoord=List_.get("y-coordinates",(double*)0);
    double * zcoord=List_.get("z-coordinates",(double*)0);
    dim=(xcoord!=0) + (ycoord!=0) + (zcoord!=0);

    /* Sanity Checks */
    TEUCHOS_TEST_FOR_EXCEPT_MSG(
      dim == 0 || ((!xcoord && (ycoord || zcoord)) || (xcoord && !ycoord && zcoord)),
      "Error: Coordinates not defined and no nullspace is provided.  One of these are *necessary* for the EdgeMatrixFreePreconditioner (found "<<dim<<" coordinates).\n"
      );

#ifdef OLD
    /* Normalize */
    double d1 = sqrt(ML_gdot(NodeMap.NumMyElements(), xcoord, xcoord, ml_comm_));
    for (int i = 0; i < NodeMap.NumMyElements(); i++) xcoord[i] /= d1;
    d1 = sqrt(ML_gdot(NodeMap.NumMyElements(), ycoord, ycoord, ml_comm_));
    for (int i = 0; i < NodeMap.NumMyElements(); i++) ycoord[i] /= d1;
    if (dim==3) {
      d1 = sqrt(ML_gdot(NodeMap.NumMyElements(), zcoord, zcoord, ml_comm_));
      for (int i = 0; i < NodeMap.NumMyElements(); i++) zcoord[i] /= d1;
    }
#endif

    /* Build the MultiVector */
    d_coords=new double* [dim];
    d_coords[0]=xcoord; d_coords[1]=ycoord;
    if(dim==3) d_coords[2]=zcoord;
    Epetra_MultiVector n_coords(View,NodeMap,d_coords,dim);

    // CMS: We're removing the normalization here.

    /* Build the Nullspace */
    nullspace=new Epetra_MultiVector(EdgeMap,dim,true);
    D0_Clean_Matrix.Multiply(false,n_coords,*nullspace);
  }

  /* Nuke the BC edges */
  for(int j=0;j<dim;j++)
    for(int i=0;i<BCedges.size();i++)
      (*nullspace)[j][BCedges[i]]=0;

  /* Cleanup */
  delete [] d_coords ;
  return nullspace;
}/*end BuildNullspace*/



#endif
