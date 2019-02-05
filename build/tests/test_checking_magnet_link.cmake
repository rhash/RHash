#test_checking_magnet_link.cmake
#
#RHASH - Test executable
#TEST_DATA_FILE - test1K.data
#TMPDIR - temporary directory for test
#
#new_test "test checking magnet link:  "
#TEST_RESULT=$( $rhash --magnet -a test1K.data | $rhash -vc - 2>&1 | grep -i -e warn -e test1K.data )
#TEST_EXPECTED="^test1K.data *OK"
#match "$TEST_RESULT" "$TEST_EXPECTED"

SET(TEST_FILE "a")
set(TEST_EXPECTED "magnet:?xl=1024&dn=test1K.data&xt=urn:crc32:b70b4c26&xt=urn:md4:5ae257c47e9be1243ee32aabe408fb6b&xt=urn:md5:b2ea9f7fcea831a4a63b213f41a8855b&xt=urn:sha1:lmagnhcibvop7pp2rpn2tflbcyhs2g3x&xt=urn:tiger:d25963c1686c96fb8881f6d10c439a7fe853c906e4eb3662&xt=urn:tree:tiger:4oqy25un2xhidqpv5u6bxaz47inucygibk7lfni&xt=urn:btih:gptvwem2uewhkoi5myvtihxav6bah3at&xt=urn:ed2k:5ae257c47e9be1243ee32aabe408fb6b&xt=urn:aich:lmagnhcibvop7pp2rpn2tflbcyhs2g3x&xt=urn:whirlpool:d606b7f44bd288759f8869d880d9d4a2f159d739005e72d00f93b814e8c04e657f40c838e4d6f9030a8c9e0308a4e3b450246250243b2f09e09fa5a24761e26b&xt=urn:ripemd160:29ea7f13cac242905ae2dc1a36d5985815b30356&xt=urn:gost:890bb3ee5dbe4da22d6719a14efd9109b220607e1086c1abbb51eeac2b044cbb&xt=urn:gost-cryptopro:d9c92e33ab144bbb2262a5221739600062831664d16716d03751fba7d952cc06&xt=urn:has160:1a3ff10095b61f4ce0cbde76f615284e52133b99&xt=urn:snefru128:7479ed8c193a23af522f1b8c1a853758&xt=urn:snefru256:4c2f2a13ac7745d117838b0be8ebb39bedd5d44b332ae8c973ac07efb50abac0&xt=urn:sha224:6290817f6001432cd441058d2bb82d88b3f32425ade4c93d56207838&xt=urn:sha256:785b0751fc2c53dc14a4ce3d800e69ef9ce1009eb327ccf458afe09c242c26c9&xt=urn:sha384:55fd17eeb1611f9193f6ac600238ce63aa298c2e332f042b80c8f691f800e4c7505af20c1a86a31f08504587395f081f&xt=urn:sha512:37f652be867f28ed033269cbba201af2112c2b3fd334a89fd2f757938ddee815787cc61d6e24a8a33340d0f7e86ffc058816b88530766ba6e231620a130b566c&xt=urn:edon-r256:069744670fd47d89f59489a45ee0d6b8f597c7c74895914997dedde4c60396f1&xt=urn:edon-r512:cd0f7ecf145c769e462cb3d1cda0a7fb5503c11b0e29e0fe9071c27e07a74f2448686a2e54619dcee8ffcbc1012f6b393faf5e40de01f76f8c75689684c161e2&xt=urn:sha3-224:5b37c09e5b5cf21b0d8097e9479fe6982003b617d41ab2293d77bf22&xt=urn:sha3-256:b6c70631c6ff932b9f380d9cde8750eb9bea393817a9aea410c2119eb7b9b870&xt=urn:sha3-384:bfdb44fcb75b4a02db0487b0c607630283ae792bbef4797bd993009a2fd15cf2425b1a9f82f25f6cdc7cac15be3d572e&xt=urn:sha3-512:b052fd4a09f988bbe4112d9a3eca8ccc517e56da866c1609504c37871146da80731bb681674a2000a41bcb78230b3d9069eb42820293ce23cba294550a1d4d3b\n")     
macro(checkrun)
  if(TEST_ERROR_RESULT)
    message(SEND_ERROR "${TEST_ERROR_RESULT}")
  endif(TEST_ERROR_RESULT)

  if(TEST_ERR_VAR GREATER 0)
    message(SEND_ERROR "${TEST_ERR_VAR}")
  endif(TEST_ERR_VAR GREATER 0)
endmacro(checkrun)

macro(report_failed_test)
  message("EXPECTED")
  message("test1K.data *OK")
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
endmacro(report_failed_test)

macro(check_POS_OK)
  if(_POS_OK)
  else(_POS_OK)
    report_failed_test()
  endif(_POS_OK)
endmacro(check_POS_OK)

execute_process(
COMMAND ${RHASH} --magnet -a "${TEST_DATA_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR ERROR_VARIABLE TEST_ERROR_RESULT)
checkrun()

if("${TEST_RESULT}" STREQUAL "${TEST_EXPECTED}")
else("${TEST_RESULT}" STREQUAL "${TEST_EXPECTED}")
  report_failed_test()
endif("${TEST_RESULT}" STREQUAL "${TEST_EXPECTED}")

#rhash -vc
FILE(WRITE "${TMPDIR}/${TEST_FILE}" "${TEST_RESULT}")
execute_process(
COMMAND ${RHASH} -vc "${TMPDIR}/${TEST_FILE}"
   OUTPUT_VARIABLE TEST_RESULT RESULT_VARIABLE TEST_ERR_VAR)
checkrun()

string(REGEX MATCH "test1K.data *OK" _POS_OK ${TEST_RESULT})
check_POS_OK()

FILE(REMOVE "${TMPDIR}/${TEST_FILE}")
