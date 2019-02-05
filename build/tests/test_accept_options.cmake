#test_wrong_sums_detection.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test

#new_test "test *accept options:       "
#rm -rf test_dir/
#mkdir test_dir 2>/dev/null && touch test_dir/file.txt test_dir/file.bin
#JPM Note - this fails to work.  MSYS seems to handle this correctly (at least here.
#SLASH="/"
#if [ -n "$MSYSTEM" ]; then
# case "$MSYSTEM" in
#   MINGW32|MINGW64)
#    SLASH=//;
#   ;;
#   *)
#    SLASH="/";
# esac
#else
#  SLASH="/";
#TEST_RESULT=$( $rhash -rC --simple --accept=.bin --path-separator=$SLASH test_dir )
#check "$TEST_RESULT" "00000000  test_dir/file.bin" .
#TEST_RESULT=$( $rhash -rC --simple --accept=.txt --path-separator=\\ test_dir )
#check "$TEST_RESULT" "00000000  test_dir\\file.txt" .
## test --crc-accept and also --path-separator options
## note: path-separator doesn't affect the following '( Verifying <filepath> )' message
#TEST_RESULT=$( $rhash -rc --crc-accept=.bin test_dir 2>/dev/null | sed -n '/Verifying/s/-//gp' )
#match "$TEST_RESULT" "( Verifying test_dir.file\\.bin )"
#rm -rf test_dir/
#
#Note that for this test, we will ignore the concerns about MSYS's shell since we are NOT using
#a shell to pass commands. That's a virtue with this rewrite because you should be able to run
#the tests irregardless of the shell itself (hopefully, this makes things more cross-platform).

include(CMakeTestMacros.cmake)
SET(TEST_STR "a")
SET(TEST_FILE "test-empty.file")

IF(EXISTS "${TMPDIR}/test_dir")
  FILE(REMOVE_RECURSE "${TMPDIR}/test_dir")
ENDIF(EXISTS "${TMPDIR}/test_dir")
file(MAKE_DIRECTORY "${TMPDIR}/test_dir")
file(WRITE "${TMPDIR}/test_dir/file.bin" "")
file(WRITE "${TMPDIR}/test_dir/file.txt" "")

#Note that cmake itself can not do a stdin test
#and I don't want to use a shell for it since
#shell samantics are so different.

Set(TEST_EXPECTED "00000000  ${TMPDIR}/test_dir/file.bin\n")
execute_process(COMMAND ${RHASH} -rC --simple --accept=.bin --path-separator=/ "${TMPDIR}/test_dir"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()
check_test_res_equality()

Set(TEST_EXPECTED "00000000  ${TMPDIR}/test_dir/file.txt\n")
string(REPLACE "/" "\\" TEST_EXPECTED "${TEST_EXPECTED}")
execute_process(COMMAND ${RHASH} -rC --simple --accept=.txt --path-separator=\\ "${TMPDIR}/test_dir"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun()
check_test_res_equality()

## test --crc-accept and also --path-separator options
## note: path-separator doesn't affect the following '( Verifying <filepath> )' message
#TEST_RESULT=$( $rhash -rc --crc-accept=.bin test_dir 2>/dev/null | sed -n '/Verifying/s/-//gp' )
#match "$TEST_RESULT" "( Verifying test_dir.file\\.bin )"
#rm -rf test_dir/

set(TEST_EXPECTED "Verifying ${TMPDIR}/test_dir/file.bin")
execute_process(COMMAND ${RHASH} -rc --crc-accept=.bin "${TMPDIR}/test_dir/file.bin"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun()
#JPM - Regex in cmake sometimes requires \\\ instead of \ due to how strings parsing works.
string(REGEX REPLACE "\\\n" "" TEST_RESULT "${TEST_RESULT}")
string(REGEX REPLACE "-.*\\\( " "" TEST_RESULT "${TEST_RESULT}")
string(REGEX REPLACE " \\\)-.*" "" TEST_RESULT "${TEST_RESULT}")
check_test_res_equality()

FILE(REMOVE "${TMPDIR}/${TEST_FILE}")

  FILE(REMOVE_RECURSE "${TMPDIR}/test_dir")
