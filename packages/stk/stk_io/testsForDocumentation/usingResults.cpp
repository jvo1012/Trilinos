#include <gtest/gtest.h>
#include <string>
#include <mpi.h>
#include <stk_io/StkMeshIoBroker.hpp>
#include <stk_io/IossBridge.hpp>
#include <Ioss_SubSystem.h>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/Types.hpp>
#include <fieldNameTestUtils.hpp>
#include <restartTestUtils.hpp>

namespace {

TEST(StkMeshIoBrokerHowTo, writeResultsWithMultistateField)
{
    std::string resultsFilename = "output.results";
    MPI_Comm communicator = MPI_COMM_WORLD;
    const std::string fieldName = "disp";
    const double stateNp1Value = 1.0;
    const double stateNValue = 2.0;
    const double stateNm1Value = 3.0;
    double time = 0.0;

    const std::string np1Name = fieldName+"NP1";
    const std::string nName   = fieldName+"N";
    const std::string nm1Name = fieldName+"Nm1";
    {
        stk::io::StkMeshIoBroker stkIo(communicator);

        stk::mesh::MetaData &stkMeshMetaData = generateMetaData(stkIo);
	// Declare a multi-state field
        stk::mesh::FieldBase *triStateField =
	  declareNodalField(stkMeshMetaData, fieldName, 3);

        stkIo.populate_bulk_data();

        putDataOnTriStateField(stkIo.bulk_data(), triStateField,
                stateNp1Value, stateNValue, stateNm1Value);

        size_t fileHandle = stkIo.create_output_mesh(resultsFilename);

	// Output each state of the multi-state field individually
	stkIo.add_results_field_with_alternate_name(fileHandle,
	        *triStateField->field_state(stk::mesh::StateNP1), np1Name);

	stkIo.add_results_field_with_alternate_name(fileHandle,
	        *triStateField->field_state(stk::mesh::StateN), nName);

	stkIo.add_results_field_with_alternate_name(fileHandle,
	        *triStateField->field_state(stk::mesh::StateNM1), nm1Name);
	
	stkIo.begin_output_step(time, fileHandle);
	stkIo.process_output_request(fileHandle);
        stkIo.end_output_step(fileHandle);
    }

    testMultistateFieldWroteCorrectly(resultsFilename, time,
            np1Name, nName, nm1Name,
            stateNp1Value, stateNValue, stateNm1Value);
    unlink(resultsFilename.c_str());
}

}
