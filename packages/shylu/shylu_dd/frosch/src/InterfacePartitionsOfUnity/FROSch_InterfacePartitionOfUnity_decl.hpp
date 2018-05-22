//@HEADER
// ************************************************************************
//
//               ShyLU: Hybrid preconditioner package
//                 Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Alexander Heinlein (alexander.heinlein@uni-koeln.de)
//
// ************************************************************************
//@HEADER

#ifndef _FROSCH_INTERFACEPARTITIONOFUNITY_DECL_HPP
#define _FROSCH_INTERFACEPARTITIONOFUNITY_DECL_HPP

#define FROSCH_ASSERT(A,S) if(!(A)) { std::cerr<<"Assertion failed. "<<S<<std::endl; std::cout.flush(); throw std::out_of_range("Assertion.");};

//#define INTERFACE_OUTPUT

#include <FROSch_DDInterface_def.hpp>

// TODO
// -> "Parent" -> "Anchestor"

namespace FROSch {
    
    template <class SC = Xpetra::Operator<>::scalar_type,
    class LO = typename Xpetra::Operator<SC>::local_ordinal_type,
    class GO = typename Xpetra::Operator<SC, LO>::global_ordinal_type,
    class NO = typename Xpetra::Operator<SC, LO, GO>::node_type>
    class InterfacePartitionOfUnity {
        
    public:
        
        typedef Teuchos::RCP<const Teuchos::Comm<int> > CommPtr;

        typedef Xpetra::Map<LO,GO,NO> Map;
        typedef Teuchos::RCP<Map> MapPtr;
        typedef Teuchos::ArrayRCP<MapPtr> MapPtrVecPtr;
        
        typedef Xpetra::Matrix<SC,LO,GO,NO> CrsMatrix;
        typedef Teuchos::RCP<CrsMatrix> CrsMatrixPtr;
        
        typedef Xpetra::MultiVector<SC,LO,GO,NO> MultiVector;
        typedef Teuchos::RCP<const MultiVector> ConstMultiVectorPtr;
        typedef Teuchos::RCP<MultiVector> MultiVectorPtr;
        typedef Teuchos::ArrayRCP<MultiVectorPtr> MultiVectorPtrVecPtr;
        typedef Teuchos::ArrayRCP<ConstMultiVectorPtr> ConstMultiVectorPtrVecPtr;

        typedef Teuchos::RCP<Teuchos::ParameterList> ParameterListPtr;
        
        typedef Teuchos::RCP<DDInterface<SC,LO,GO,NO> > DDInterfacePtr;
        
        typedef Teuchos::RCP<EntitySet<SC,LO,GO,NO> > EntitySetPtr;
        
        typedef unsigned UN;
        
        typedef Teuchos::Array<GO> GOVec;
        typedef Teuchos::ArrayView<GO> GOVecView;
        
        typedef Teuchos::ArrayRCP<SC> SCVecPtr;
        typedef Teuchos::ArrayRCP<SCVecPtr> SCVecPtr2D;

        
        InterfacePartitionOfUnity(CommPtr mpiComm,
                                  CommPtr serialComm,
                                  UN dimension,
                                  UN dofsPerNode,
                                  MapPtr nodesMap,
                                  MapPtrVecPtr dofsMaps,
                                  ParameterListPtr parameterList);
        
        virtual ~InterfacePartitionOfUnity();
        
        virtual int removeDirichletNodes(GOVecView myGlobalDirichletBoundaryDofs = GOVecView()) = 0;
        
        virtual int sortInterface(CrsMatrixPtr Matrix,
                                  SCVecPtr2D localNodeList = SCVecPtr2D()) = 0;
        
        virtual int computePartitionOfUnity() = 0;
        
        ConstMultiVectorPtrVecPtr getLocalPartitionOfUnity() const;
        
        DDInterfacePtr getDDInterface() const;

        
    protected:
        
        CommPtr MpiComm_;
        CommPtr SerialComm_;
        
        DDInterfacePtr DDInterface_;
        
        EntitySetPtr Interface_;
        
        ParameterListPtr ParameterList_;
        
        ConstMultiVectorPtrVecPtr LocalPartitionOfUnity_;
        
        bool Verbose_;
    };
    
}

#endif
