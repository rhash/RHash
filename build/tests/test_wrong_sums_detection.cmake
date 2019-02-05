#test_wrong_sums_detection.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test

#new_test "test wrong sums detection:  "
#printf WRONG | $rhash -p '%c\n%m\n%e\n%h\n%g\n%t\n%a\n%w\n' - > test1K.data.hash
#TEST_RESULT=$( $rhash -vc test1K.data.hash 2>&1 | grep 'OK' )
#check "$TEST_RESULT" ""
#rm test1K.data.hash


include(CMakeTestMacros.cmake)
SET(TEST_STR "a")
SET(TEST_FILE "test-empty.file")

FILE(WRITE "${TMPDIR}/${TEST_FILE}" "WRONG")

#Note that cmake itself can not do a stdin test
#and I don't want to use a shell for it since
#shell samantics are so different.
execute_process(COMMAND ${RHASH}  -p "%c\n%m\n%e\n%h\n%g\n%t\n%a\n%w\n" "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()
file(WRITE "${TMPDIR}/test1K.data.hash" "${TEST_OUTPUT}")
execute_process(COMMAND ${RHASH} -vc "${TMPDIR}/test1K.data.hash"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
#TEST_ERROR_RESULT may not be empty
if(TEST_ERR_VAR GREATER 0)
   report_failed_test()
endif(TEST_ERR_VAR GREATER 0)

string(REGEX MATCH "OK" _POS_OK ${TEST_RESULT})
check_POS_OK()

FILE(REMOVE "${TMPDIR}/${TEST_FILE}")
file(REMOVE "${TMPDIR}/test1K.data.hash")
