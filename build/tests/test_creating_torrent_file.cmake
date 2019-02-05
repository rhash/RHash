#test_creating_torrent_file.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test

#new_test "test creating torrent file: "
#TEST_RESULT=$( $rhash --btih --torrent --bt-private --bt-piece-length=512 --bt-announce=http://tracker.org/ 'test1K.data' 2>/dev/null )
#check "$TEST_RESULT" "29f7e9ef0f41954225990c513cac954058721dd2  test1K.data"
#rm test1K.data.torrent

include(CMakeTestMacros.cmake)
SET(TEST_STR "")
SET(TEST_EXPECTED "29f7e9ef0f41954225990c513cac954058721dd2  test1K.data\n")

message(STATUS "RHASH           = ${RHASH}")
message(STATUS "TEST_DATA_FILE  = ${TEST_DATA_FILE}")
message(STATUS "TMPDIR          = ${TMPDIR}")


execute_process(COMMAND ${RHASH}  --btih --torrent --bt-private --bt-piece-length=512 --bt-announce=http://tracker.org/ "${TEST_DATA_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun()
check_test_res_equality()

FILE(REMOVE "${TEST_DATA_FILE}.torrent")

