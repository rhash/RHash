#test_ignoring_of_log_files.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test


#new_test "test exit code:             "
#rm -f none-existent.file
#test -f none-existent.file && print_failed .
#$rhash -H none-existent.file 2>/dev/null
#check "$?" "2" .
#$rhash -c none-existent.file 2>/dev/null
#check "$?" "2" .
#$rhash -H test1K.data >/dev/null
#check "$?" "0"

include(CMakeTestMacros.cmake)
SET(TEST_STR "")
SET(TEST_FILE "none-existent.file")
SET(TEST_EXPECTED "")

FILE(REMOVE "${TMPDIR}/${TEST_FILE}")

execute_process(COMMAND ${RHASH} -H "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun_exitcod_2()

execute_process(COMMAND -c "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun_exitcod_2()

execute_process(COMMAND -c "${TMPDIR}/${TEST_DATA_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun()

