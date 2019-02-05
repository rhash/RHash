#test_eDonkey_link.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test

#new_test "test eDonkey link:          "
#TEST_RESULT=$( printf "a" | $rhash -p '%f %L %l\n' - )
#TEST_EXPECTED="(stdin) ed2k://|file|(stdin)|1|BDE52CB31DE33E46245E05FBDBD6FB24|h=Q336IN72UWT7ZYK5DXOLT2XK5I3XMZ5Y|/ ed2k://|file|(stdin)|1|bde52cb31de33e46245e05fbdbd6fb24|h=q336in72uwt7zyk5dxolt2xk5i3xmz5y|/"
#check "$TEST_RESULT" "$TEST_EXPECTED" .
# here we should test checking of ed2k links but it is currently unsupported
#TEST_RESULT=$( $rhash -L test1K.data | $rhash -vc - 2>/dev/null | grep test1K.data )
#match "$TEST_RESULT" "^test1K.data *OK"
include(CMakeTestMacros.cmake)
set(TEST_EXPECTED "a ed2k://|file|a|1|BDE52CB31DE33E46245E05FBDBD6FB24|h=Q336IN72UWT7ZYK5DXOLT2XK5I3XMZ5Y|/ ed2k://|file|a|1|bde52cb31de33e46245e05fbdbd6fb24|h=q336in72uwt7zyk5dxolt2xk5i3xmz5y|/\n")

set(TEST_STR "a")
SET(TEST_FILE "a")

FILE(WRITE "${TMPDIR}/${TEST_FILE}" "${TEST_STR}")

execute_process(
COMMAND ${RHASH} -p "%f %L %l\n" "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()
FILE(REMOVE "${TMPDIR}/${TEST_FILE}")
check_test_res_equality()

set(TEST_EXPECTED "ed2k://|file|test1K.data|1024|5ae257c47e9be1243ee32aabe408fb6b|h=lmagnhcibvop7pp2rpn2tflbcyhs2g3x|/")
# here we should test checking of ed2k links but it is currently unsupported
#TEST_RESULT=$( $rhash -L test1K.data | $rhash -vc - 2>/dev/null | grep test1K.data )
#match "$TEST_RESULT" "^test1K.data *OK"
execute_process(
COMMAND ${RHASH} -L "${TEST_DATA_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()
check_test_res_equality()

#rhash -vc
FILE(WRITE "${TMPDIR}/${TEST_FILE}" "${TEST_RESULT}")
execute_process(
COMMAND ${RHASH} -vc "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun()

string(REGEX MATCH "test1K.data *OK" _POS_OK ${TEST_RESULT})
check_POS_OK()

FILE(REMOVE "${TMPDIR}/${TEST_FILE}")
