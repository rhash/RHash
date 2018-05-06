#test_handling_empty_files.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test

#new_test "test handling empty files:  "
#printf "" > test-empty.file
#TEST_RESULT=$( $rhash -p "%m" test-empty.file )
#check "$TEST_RESULT" "d41d8cd98f00b204e9800998ecf8427e" .
## now test processing of empty stdin
#TEST_RESULT=$( printf "" | $rhash -p "%m" - )
#check "$TEST_RESULT" "d41d8cd98f00b204e9800998ecf8427e" .
## test verification of empty file
#TEST_RESULT=$( $rhash -c test-empty.file | grep "^[^-]" )
#check "$TEST_RESULT" "Everything OK"
#rm test-empty.file

set(TEST_EXPECTED "d41d8cd98f00b204e9800998ecf8427e")
include(CMakeTestMacros.cmake)
SET(TEST_STR "test_string1")
SET(TEST_FILE "test-empty.file")

FILE(WRITE "${TMPDIR}/${TEST_FILE}" "")

#Note that cmake itself can not do a stdin test
#and I don't want to use a shell for it since
#shell samantics are so different.
execute_process(COMMAND ${RHASH} -p "%m" "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()
check_test_res_equality()

execute_process(
COMMAND ${RHASH}  -c "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()

string(REGEX MATCH "^[^-]" _POS_OK ${TEST_RESULT})
check_POS_OK()

FILE(REMOVE "${TMPDIR}/${TEST_FILE}")
