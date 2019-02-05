#test_checking_magnet_link.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test
#

include(CMakeTestMacros.cmake)

SET(TEST_FILE "a")
SET(TEST_FILE_A "test_[D3D99E8B].data")
SET(TEST_FILE_B "test_[D3D99E8C].data")
file(WRITE "${TMPDIR}/${TEST_FILE_A}" "A")
file(WRITE "${TMPDIR}/${TEST_FILE_B}" "A")

# first verify checking an existing crc32 while '--embed-crc' option is set
#TEST_RESULT=$( $rhash -C --simple 'test_[D3D99E8B].data' | $rhash -vc --embed-crc - 2>/dev/null | grep data )
#match "$TEST_RESULT" "^test_.*OK" .

execute_process(
COMMAND ${RHASH} -C --simple "${TMPDIR}/${TEST_FILE_A}" 
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()

file(WRITE "${TMPDIR}/${TEST_FILE}" "${TEST_RESULT}")

#rhash -vc --embed-crc
execute_process(
COMMAND ${RHASH} -vc --embed-crc "${TMPDIR}/${TEST_FILE}" OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checktestfail()

FILE(REMOVE "${TMPDIR}/${TEST_FILE}")
## second verify --check-embedded option
#TEST_RESULT=$( $rhash --check-embedded 'test_[D3D99E8B].data' 2>/dev/null | grep data )
#match "$TEST_RESULT" "test_.*OK" .
#TEST_RESULT=$( $rhash --check-embedded 'test_[D3D99E8C].data' 2>/dev/null | grep data )
#match "$TEST_RESULT" "test_.*ERR" .
#mv 'test_[D3D99E8B].data' 'test.data'

execute_process(
   COMMAND ${RHASH} --check-embedded "${TMPDIR}/${TEST_FILE_A}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checktestfail()

execute_process(
   COMMAND ${RHASH} --check-embedded "${TMPDIR}/${TEST_FILE_B}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checktestfail_ERR()
file(RENAME "${TMPDIR}/${TEST_FILE_B}" "${TMPDIR}/test.data")

# at last test --embed-crc with --embed-crc-delimiter options
#TEST_RESULT=$( $rhash --simple --embed-crc --embed-crc-delimiter=_ 'test.data' 2>/dev/null )
#check "$TEST_RESULT" "d3d99e8b  test_[D3D99E8B].data"
#rm 'test_[D3D99E8B].data' 'test_[D3D99E8C].data'
execute_process(
   COMMAND ${RHASH}  --simple --embed-crc --embed-crc-delimiter=_ "test.data"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)

string(FIND "d3d99e8b  test_[D3D99E8B].data" _POS_OK "${TEST_RESULT}")
check_POS_OK()
FILE(REMOVE "${TMPDIR}/${TEST_FILE_A}")
FILE(REMOVE "${TMPDIR}/${TEST_FILE_B}")
