#test_ignoring_of_log_files.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test

#new_test "test ignoring of log files: "
#touch test_file1.out test_file2.out
#TEST_RESULT=$( $rhash -C --simple test_file1.out test_file2.out -o test_file1.out -l test_file2.out 2>/dev/null )
#check "$TEST_RESULT" "" .
#TEST_RESULT=$( $rhash -c test_file1.out test_file2.out -o test_file1.out -l test_file2.out 2>/dev/null )
#check "$TEST_RESULT" ""
#rm test_file1.out test_file2.out


include(CMakeTestMacros.cmake)
SET(TEST_STR "")
SET(TEST_FILE_1 "test_file1.out")
SET(TEST_FILE_2 "test_file1.out")
SET(TEST_EXPECTED "")

FILE(WRITE "${TMPDIR}/${TEST_FILE_1}" "")
FILE(WRITE "${TMPDIR}/${TEST_FILE_2}" "")

execute_process(COMMAND ${RHASH} --simple "${TMPDIR}/${TEST_FILE_1}" "${TMPDIR}/${TEST_FILE_2}" -o "${TMPDIR}/${TEST_FILE_1}" -l "${TMPDIR}/${TEST_FILE_2}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun()
check_test_res_equality()
#file(WRITE "${TMPDIR}/test1K.data.hash" "${TEST_OUTPUT}")
execute_process(COMMAND ${RHASH} -c "${TMPDIR}/${TEST_FILE_1}" "${TMPDIR}/${TEST_FILE_2}" -o "${TMPDIR}/${TEST_FILE_1}" -l "${TMPDIR}/${TEST_FILE_2}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun()
check_test_res_equality()

FILE(REMOVE "${TMPDIR}/${TEST_FILE_1}")
FILE(REMOVE "${TMPDIR}/${TEST_FILE_2}")
