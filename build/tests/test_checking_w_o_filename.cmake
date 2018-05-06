#test_checking_w_o_filename.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test
#
#new_test "test checking w/o filename: "
#$rhash -p '%c\n%m\n%e\n%h\n%g\n%t\n%a\n' test1K.data > test1K.data.hash
#TEST_RESULT=$( $rhash -vc test1K.data.hash 2>&1 | grep -i -e warn -e err )
#TEST_EXPECTED=""
#check "$TEST_RESULT" "$TEST_EXPECTED"
include(CMakeTestMacros.cmake)
SET(TEST_FILE "test1K.data.hash")
get_filename_component(_FNAME "${TEST_DATA_FILE}" NAME)
execute_process(
COMMAND ${RHASH} -p "%c\n%m\n%e\n%h\n%g\n%t\n%a\n" "${_FNAME}"
   OUTPUT_VARIABLE TEST_RESULT
   RESULT_VARIABLE TEST_ERR_VAR
   ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()

#rhash -vc
FILE(WRITE "${TMPDIR}/${TEST_FILE}" "${TEST_RESULT}")
execute_process(COMMAND ${RHASH} -vc "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT
   RESULT_VARIABLE TEST_ERR_VAR
   ERROR_VARIABLE TEST_ERROR_RESULT)
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
