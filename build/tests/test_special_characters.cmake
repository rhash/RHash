#test_x_b_B_modifiers.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - binary.data
#TMPDIR - temporary directory for test
#
#new_test "test special characters:    "
#TEST_RESULT=$( echo | $rhash -p '\63\1\277\x0f\x1\t\\ \x34\r' - )
#TEST_EXPECTED=$( printf '\63\1\277\17\1\t\\ 4\r' )
#check "$TEST_RESULT" "$TEST_EXPECTED"

SET(TEST_FILE "bin.data.out")

execute_process(
COMMAND ${RHASH} -p "\\63\\1\\277\\x0f\\x1\\t\\\\ \\x34\\r" ${TEST_DATA_FILE}
   OUTPUT_VARIABLE TEST_RESULT
   RESULT_VARIABLE TEST_ERR_VAR
   ERROR_VARIABLE TEST_ERROR_RESULT
)
#This bit is not optimal but cmake string comparisons
#do not work well and I couldn't find functionality to convert to hex.
file(WRITE "${TMPDIR}/${TEST_FILE}" "${TEST_RESULT}")
file(READ "${TMPDIR}/${TEST_FILE}" TEST_RESULT HEX)
file(REMOVE "${TMPDIR}/${TEST_FILE}")
file(READ "${TEST_DATA_FILE}" TEST_EXPECTED HEX)

if(TEST_ERROR_RESULT)
 message(SEND_ERROR "${TEST_ERROR_RESULT}")
endif(TEST_ERROR_RESULT)

if(TEST_ERR_VAR GREATER 0)
  message(SEND_ERROR "${TEST_ERR_VAR}")
endif(TEST_ERR_VAR GREATER 0)

if("${TEST_EXPECTED}" STREQUAL "${TEST_RESULT}")
  message("OK")
else(_POS_OK)
  message("EXPECTED")
  message("${TEST_EXPECTED}")
  message("GOT")
  message("Variables")
  message("RHASH")
  message("${RHASH}")
  message(TEST_RESULT)
  message("${TEST_RESULT}")
  message("TEST_ERROR_RESULT")
  message("${TEST_ERROR_RESULT}")
  message("TEST_ERR_VAR")
  message("${TEST_ERR_VAR}")
  message(SEND_ERROR "FAILED!!!")
endif("${TEST_EXPECTED}" STREQUAL "${TEST_RESULT}")

