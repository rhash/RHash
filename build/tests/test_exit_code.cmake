#test_exit_code.cmake.cmake
#
#Parameters:
#
#  RHASH = rhash executable
#  TDATA = test data file test1K.data
#
#do something like this:
#
#====
#new_test "test exit code:             "
#rm -f none-existent.file
#test -f none-existent.file && print_failed .
#$rhash -H none-existent.file 2>/dev/null
#check "$?" "2" .
#$rhash -c none-existent.file 2>/dev/null
#check "$?" "2" .
#$rhash -H test1K.data >/dev/null
#check "$?" "0"

#if [ $fail_cnt -gt 0 ]; then
#  echo "Failed $fail_cnt checks"
#  exit 1 # some tests failed
#fi
#===

message("RHASH - ${RHASH}")
message("TDATA - ${TDATA}")

file(REMOVE none-existent.file)
execute_process(COMMAND ${RHASH} -H none-existent.file
  RESULT_VARIABLE RES_VAR
  OUTPUT_VARIABLE STD_OUT
  ERROR_VARIABLE ERR_OUT) 
if ((NOT "${RES_VAR}" STREQUAL "2") AND (NOT "${RES_VAR}" STREQUAL "No such file or directory")
AND (NOT "${RES_VAR}" STREQUAL "The system cannot find the file specified"))
  message(FATAL_ERROR "Result code: ${RES_VAR}\rstdout: ${STD_OUT}\rstderr: ${ERR_OUT}")
endif()

execute_process(COMMAND ${RHASH} -c none-existent.file
  RESULT_VARIABLE RES_VAR
  OUTPUT_VARIABLE STD_OUT
  ERROR_VARIABLE ERR_OUT) 
if ((NOT "${RES_VAR}" STREQUAL "2") AND (NOT "${RES_VAR}" STREQUAL "No such file or directory")
AND (NOT "${RES_VAR}" STREQUAL "The system cannot find the file specified"))
  message(FATAL_ERROR "Result code: ${RES_VAR}\rstdout: ${STD_OUT}\rstderr: ${ERR_OUT}")
endif()

execute_process(COMMAND ${RHASH} -H ${TDATA}
  RESULT_VARIABLE RES_VAR
  OUTPUT_VARIABLE STD_OUT
  ERROR_VARIABLE ERR_OUT) 
if ((NOT "${RES_VAR}" STREQUAL "0") AND (NOT "${RES_VAR}" STREQUAL ""))
  message(FATAL_ERROR "Result code: ${RES_VAR}\rstdout: ${STD_OUT}\rstderr: ${ERR_OUT}")
endif((NOT "${RES_VAR}" STREQUAL "0") AND (NOT "${RES_VAR}" STREQUAL ""))
