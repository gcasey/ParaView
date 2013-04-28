# CoProcessing test expects the following arguments to be passed to cmake using
# -DFoo=BAR arguments.

# COPROCESSING_TEST_DRIVER -- path to CoProcessingPythonScriptExample
# COPROCESSING_TEST_DIR    -- path to temporary dir
# COPROCESSING_TEST_SCRIPT -- python script to run
# COPROCESSING_IMAGE_TESTER -- path to CoProcessingCompareImagesTester
# COPROCESSING_DATA_DIR     -- path to data dir for baselines

# USE_MPI
# MPIEXEC
# MPIEXEC_NUMPROC_FLAG
# MPIEXEC_NUMPROCS
# MPIEXEC_PREFLAGS
# VTK_MPI_POSTFLAGS

# remove result files generated by  the test
file(REMOVE "${COPROCESSING_TEST_DIR}/CPPressure0.png" )
file(REMOVE "${COPROCESSING_TEST_DIR}/CPGrid0.png" )

if(NOT EXISTS "${COPROCESSING_TEST_DRIVER}")
  message(FATAL_ERROR "'${COPROCESSING_TEST_DRIVER}' does not exist")
endif()

if(NOT EXISTS "${COPROCESSING_IMAGE_TESTER}")
  message(FATAL_ERROR "'${COPROCESSING_IMAGE_TESTER}' does not exist")
endif()

if (USE_MPI)
  message("Executing :
      ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_NUMPROCS} ${MPIEXEC_PREFLAGS}
      \"${COPROCESSING_TEST_DRIVER}\"
      \"${COPROCESSING_TEST_SCRIPT}\"")
  execute_process(COMMAND
      ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_NUMPROCS} ${MPIEXEC_PREFLAGS}
      "${COPROCESSING_TEST_DRIVER}"
      "${COPROCESSING_TEST_SCRIPT}"
    WORKING_DIRECTORY ${COPROCESSING_TEST_DIR}
    RESULT_VARIABLE rv)
else()
  message("Executing : \"${COPROCESSING_TEST_DRIVER}\" \"${COPROCESSING_TEST_SCRIPT}\"")
  execute_process(COMMAND "${COPROCESSING_TEST_DRIVER}" "${COPROCESSING_TEST_SCRIPT}"
    WORKING_DIRECTORY ${COPROCESSING_TEST_DIR}
    RESULT_VARIABLE rv)
endif()

if(NOT rv EQUAL 0)
  message(FATAL_ERROR "Test executable return value was ${rv}")
endif()

if(NOT EXISTS "${COPROCESSING_TEST_DIR}/CPGrid0.png")
  message(FATAL_ERROR "'${COPROCESSING_TEST_DIR}/CPGrid0.png' was not created")
endif()

if(NOT EXISTS "${COPROCESSING_TEST_DIR}/CPPressure0.png")
  message(FATAL_ERROR "'${COPROCESSING_TEST_DIR}/CPPressure0.png' was not created")
endif()

execute_process(COMMAND "${COPROCESSING_IMAGE_TESTER}"
    "${COPROCESSING_TEST_DIR}/CPGrid0.png" 20 -V "${COPROCESSING_DATA_DIR}/Baseline/CPGrid0.png" -T "${COPROCESSING_TEST_DIR}"
    RESULT_VARIABLE failed)
if(failed)
  message(FATAL_ERROR "CPGrid0 image compare failed.")
endif()

execute_process(COMMAND "${COPROCESSING_IMAGE_TESTER}"
    "${COPROCESSING_TEST_DIR}/CPPressure0.png" 20 -V "${COPROCESSING_DATA_DIR}/Baseline/CPPressure0.png" -T "${COPROCESSING_TEST_DIR}"
    RESULT_VARIABLE failed)

if(failed)
  message(FATAL_ERROR "CPPressure0 image compare failed.")
endif()
