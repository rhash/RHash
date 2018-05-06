#test_bsd_format_checking.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test
#
#new_test "test bsd format checking:   "
#TEST_RESULT=$( $rhash --bsd -a test1K.data | $rhash -vc - 2>&1 | grep -i -e warn -e err )
#check "$TEST_RESULT" ""

SET(TEST_FILE "a")
include(CMakeTestMacros.cmake)

execute_process(COMMAND ${RHASH} --bsd -a "${TEST_DATA_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()

#rhash -vc
FILE(WRITE "${TMPDIR}/${TEST_FILE}" "${TEST_RESULT}")
execute_process(COMMAND ${RHASH} -vc "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun()

string(FIND "warn" _POS_MSG "${TEST_ERROR_RESULT}")
if(NOT _POS_MSG)
  string(FIND "err" _POS_MSG  "${TEST_ERROR_RESULT}")
endif(NOT _POS_MSG)

if(_POS_MSG)
  report_failed_test()
else(_POS_MSG)
  message("OK")
endif(_POS_MSG)

FILE(REMOVE "${TMPDIR}/${TEST_FILE}")
