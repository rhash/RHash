#!/bin/sh
# Run RHash tests
# Usage: test_rhash.sh [ --full | --shared ] <PATH-TO-EXECUTABLE>
export LC_ALL=C

# read options
while [ "$#" -gt 0 ]; do
  case $1 in
    --full)
      OPT_FULL=1
      ;;
    --shared)
      OPT_SHARED=1
      ;;
    *)
      [ -x "$1" -a -z "$rhash" -a -x "$1" ] && rhash="$(cd $(dirname $1) && echo $PWD/${1##*/})"
      ;;
  esac
  shift
done
_sdir="$(dirname "$0")"
SCRIPT_DIR="$(cd "$_sdir" && pwd)"
UPPER_DIR="$(cd "$_sdir/.." && pwd)"

# detect binary if not specified by command line
[ -x "$rhash" ] || rhash="$UPPER_DIR/rhash";
[ -x "$rhash" ] || rhash="`which rhash`"
if [ ! -x "$rhash" ]; then
  echo "Fatal: $rhash not found"
  exit 1
fi
echo "Testing $rhash"

win32()
{
  case "$(uname -s)" in
    MINGW*|MSYS*|[cC][yY][gG][wW][iI][nN]*) return 0 ;;
  esac
  return 1
}

# detect shared library
if [ -n "$OPT_SHARED" -a -d "$UPPER_DIR/librhash" ]; then
  D="$UPPER_DIR/librhash"
  N="$D/librhash"
  if [ -r $N.0.dylib ] && ( uname -s | grep -qi "^darwin" || [ ! -r $N.so.0 ] ); then
    export DYLD_LIBRARY_PATH="$D:$DYLD_LIBRARY_PATH"
  elif ls $D/*rhash.dll 2>/dev/null >/dev/null && ( win32 || [ ! -r $N.so.0 ] ); then
    export PATH="$D:$PATH"
  elif [ -r $N.so.0 ]; then
    export LD_LIBRARY_PATH="$D:$LD_LIBRARY_PATH"
  else
    echo "shared library not found at $D"
  fi
fi

# run smoke test: test exit code of a simple command
$rhash --printf "" -m ""
res=$?
if [ $res -ne 0 ]; then
  if [ $res -eq 127 ]; then
    echo "error: could not load dynamic libraries or execute $rhash"
    [ -z "$OPT_SHARED" ] && echo "try running with --shared option"
  elif [ $res -eq 139 ]; then
    echo "error: got segmentation fault by running $rhash"
  else
    echo "error: obtained unexpected exit_code = $res from $rhash"
  fi
  exit 2
fi

# create temp directory
for _tmp in "$TMPDIR" "$TEMPDIR" "/tmp" ; do
  [ -d "$_tmp" ] && break
done
RANDNUM=$RANDOM
[ -z $RANDNUM ] && which jot >/dev/null && RANDNUM=$(jot -r 1 1 32767)
RHASH_TMP="$_tmp/rhash-test-$RANDNUM-$$"
remove_tmpdir()
{
  cd "$SCRIPT_DIR"
  rm -rf "$RHASH_TMP";
}
trap remove_tmpdir EXIT

# prepare test files
mkdir $RHASH_TMP || die "Unable to create tmp dir."
cp "$SCRIPT_DIR/test1K.data" $RHASH_TMP/test1K.data
cd "$RHASH_TMP"

# get the list of supported hash options
HASHOPT="`$rhash --list-hashes|sed 's/ .*$//;/[^23]-/s/-\([0-9R]\)/\1/'|tr A-Z a-z`"

fail_cnt=0
test_num=1
sub_test=0
new_test() {
  printf "%2u. %s" $test_num "$1"
  test_num=$((test_num+1))
  sub_test=0
}

print_failed() {
    st=$( test "$1" = "." -o "$sub_test" -gt 1 && echo " Subtest #$sub_test" )
    echo "Failed$st"
}

# verify obtained value $1 against the expected value $2
check() {
#printf "test '%s' = '%s'", "$1" "$2"
  sub_test=$((sub_test+1))
  if [ "$1" = "$2" ]; then
    test "$3" = "." || echo "Ok"
  else
    print_failed "$3"
    printf "obtained: \"%s\"\n" "$1"
    printf "expected: \"%s\"\n" "$2"
    fail_cnt=$((fail_cnt+1))
    return 1;
  fi
  return 0
}

# match obtained value $1 against given grep-regexp $2
match_line() {
  if echo "$1" | grep -vq "$2"; then
    printf "obtained: \"%s\"\n" "$1"
    printf "regexp:  /%s/\n" "$2"
    fail_cnt=$((fail_cnt+1))
    return 1
  fi
  return 0
}

# match obtained value $1 against given grep-regexp $2
match() {
  sub_test=$((sub_test+1))
  if echo "$1" | grep -vq "$2"; then
    print_failed "$3"
    printf "obtained: \"%s\"\n" "$1"
    printf "regexp:  /%s/\n" "$2"
    fail_cnt=$((fail_cnt+1))
    return 1;
  else
    test "$3" = "." || echo "Ok"
  fi
  return 0
}

new_test "test with a text string:    "
TEST_RESULT=$( $rhash --message "abc" | tail -1 )
TEST_EXPECTED="(message) 352441C2"
check "$TEST_RESULT" "$TEST_EXPECTED"

new_test "test stdin processing:      "
TEST_STR="test_string1"
TEST_RESULT=$( printf "abc" | $rhash -CHMETAGW --sfv - | tail -1 )
TEST_EXPECTED="(stdin) 352441C2 900150983CD24FB0D6963F7D28E17F72 A9993E364706816ABA3E25717850C26C9CD0D89D ASD4UJSEH5M47PDYB46KBTSQTSGDKLBHYXOMUIA A448017AAF21D8525FC10AE87AA6729D VGMT4NSHA2AWVOR6EVYXQUGCNSONBWE5 4E2448A4C6F486BB16B6562C73B4020BF3043E3A731BCE721AE1B303D97E6D4C7181EEBDB6C57E277D0E34957114CBD6C797FC9D95D8B582D225292076D4EEF5 4E2919CF137ED41EC4FB6270C61826CC4FFFB660341E0AF3688CD0626D23B481"
check "$TEST_RESULT" "$TEST_EXPECTED"

new_test "test with 1Kb data file:    "
TEST_RESULT=$( $rhash --printf "%f %C %M %H %E %G %T %A %W\n" test1K.data 2>/dev/null )
TEST_EXPECTED="test1K.data B70B4C26 B2EA9F7FCEA831A4A63B213F41A8855B 5B00669C480D5CFFBDFA8BDBA99561160F2D1B77 5AE257C47E9BE1243EE32AABE408FB6B 7A6682133082A49C37DB7B008394AEB9C184D5FB2A8D2A6251DD4BBA5F6744B4 4OQY25UN2XHIDQPV5U6BXAZ47INUCYGIBK7LFNI LMAGNHCIBVOP7PP2RPN2TFLBCYHS2G3X D606B7F44BD288759F8869D880D9D4A2F159D739005E72D00F93B814E8C04E657F40C838E4D6F9030A8C9E0308A4E3B450246250243B2F09E09FA5A24761E26B"
check "$TEST_RESULT" "$TEST_EXPECTED" .
# test calculation/verification of reversed GOST hashes with 1Kb data file
TEST_RESULT=$( $rhash --simple --gost --gost-cryptopro --gost-reverse test1K.data )
TEST_EXPECTED="test1K.data  bb4c042bacee51bbabc186107e6020b20991fd4ea119672da24dbe5deeb30b89  06cc52d9a7fb5137d01667d1641683620060391722a56222bb4b14ab332ec9d9"
check "$TEST_RESULT" "$TEST_EXPECTED" .
TEST_RESULT=$( $rhash --simple --gost --gost-cryptopro --gost-reverse test1K.data | $rhash -vc - 2>/dev/null | grep test1K.data )
match "$TEST_RESULT" "^test1K.data *OK"

new_test "test handling empty files:  "
EMPTY_FILE="$RHASH_TMP/test-empty.file"
printf "" > "$EMPTY_FILE"
TEST_RESULT=$( $rhash -p "%m" "$EMPTY_FILE" )
check "$TEST_RESULT" "d41d8cd98f00b204e9800998ecf8427e" .
# test processing of empty message
TEST_RESULT=$( $rhash -p "%m" -m "" )
check "$TEST_RESULT" "d41d8cd98f00b204e9800998ecf8427e" .
# test processing of empty stdin
TEST_RESULT=$( printf "" | $rhash -p "%m" - )
check "$TEST_RESULT" "d41d8cd98f00b204e9800998ecf8427e" .
# test verification of empty file
TEST_RESULT=$( $rhash -c "$EMPTY_FILE" | grep "^[^-]" )
check "$TEST_RESULT" "Nothing to verify"
rm "$EMPTY_FILE"

# Test the SFV format using test1K.data from the previous test
new_test "test default format:        "
MATCH_LOG="$RHASH_TMP/match_err.log"
$rhash test1K.data | (
  read l; match_line "$l" "^; Generated by RHash"
  read l; match_line "$l" "^; Written by"
  read l; match_line "$l" "^;\$"
  read l; match_line "$l" "^; *1024  [0-9:\.]\{8\} [0-9-]\{10\} test1K.data\$"
  read l; match_line "$l" "^test1K.data B70B4C26\$"
) > "$MATCH_LOG"
[ ! -s "$MATCH_LOG" ] && echo "Ok" || echo "Failed" && cat "$MATCH_LOG"
rm -f "$MATCH_LOG"

new_test "test %x, %b, %B modifiers:  "
TEST_RESULT=$( $rhash -p '%f %s %xC %bc %bM %Bh %bE %bg %xT %xa %bW\n' -m "a" )
TEST_EXPECTED="(message) 1 E8B7BE43 5c334qy BTAXLOOA6G3KQMODTHRGS5ZGME hvfkN/qlp/zhXR3cuerq6jd2Z7g= XXSSZMY54M7EMJC6AX55XVX3EQ xiyqtg44zbhmfjtr5eytk4rxreqkobntmoyddiolj7ad4aoorxzq 16614B1F68C5C25EAF6136286C9C12932F4F73E87E90A273 86f7e437faa5a7fce15d1ddcb9eaeaea377667b8 RLFCMATZFLWG6ENGOIDFGH5X27YN75MUCMKF42LTYRIADUAIPNBNCG6GIVATV37WHJBDSGRZCRNFSGUSEAGVMAMV4U5UPBME7WXCGGQ"
check "$TEST_RESULT" "$TEST_EXPECTED"

new_test "test %u modifier:           "
mkdir dir1 && printf "a" > "dir1/=@+.txt"
TEST_RESULT=$( $rhash -p '%uf %Uf %up %Up %uxc %uxC %ubc %ubC\n' "dir1/=@+.txt" )
TEST_EXPECTED="%3d%40%2b.txt %3D%40%2B.txt dir1%2f%3d%40%2b.txt dir1%2F%3D%40%2B.txt e8b7be43 E8B7BE43 5c334qy 5C334QY"
check "$TEST_RESULT" "$TEST_EXPECTED" .
TEST_RESULT=$( $rhash -p '%uBc %UBc %Bc %u@c %U@c\n' -m "a" )
TEST_EXPECTED="6Le%2bQw%3d%3d 6Le%2BQw%3D%3D 6Le+Qw== %e8%b7%beC %E8%B7%BEC"
check "$TEST_RESULT" "$TEST_EXPECTED"

new_test "test special characters:    "
TEST_RESULT=$( $rhash -p '\63\1\277\x0f\x1\t\\ \x34\r' -m "" )
TEST_EXPECTED=$( printf '\63\1\277\17\1\t\\ 4\r' )
check "$TEST_RESULT" "$TEST_EXPECTED"

new_test "test file lists:            "
F="$RHASH_TMP/t"
touch ${F}1 ${F}2 ${F}3 ${F}4
( echo ${F}2; echo ${F}3 ) > ${F}l
TEST_RESULT=$($rhash -p '%f ' ${F}1 --file-list ${F}l ${F}4)
check "$TEST_RESULT" "t1 t2 t3 t4 "
rm -f ${F}1 ${F}2 ${F}3 ${F}4 ${F}l

new_test "test eDonkey link:          "
TEST_RESULT=$( $rhash -p '%L %l\n' -m "a" )
TEST_EXPECTED="ed2k://|file|%28message%29|1|BDE52CB31DE33E46245E05FBDBD6FB24|h=Q336IN72UWT7ZYK5DXOLT2XK5I3XMZ5Y|/ ed2k://|file|%28message%29|1|bde52cb31de33e46245e05fbdbd6fb24|h=q336in72uwt7zyk5dxolt2xk5i3xmz5y|/"
check "$TEST_RESULT" "$TEST_EXPECTED" .
# test verification of ed2k links
TEST_RESULT=$( $rhash -L test1K.data | $rhash -vc - 2>/dev/null | grep test1K.data )
match "$TEST_RESULT" "^test1K.data *OK"

if [ -n "$OPT_FULL" ]; then
  new_test "test all hash options:      "
  errors=0
  for opt in $HASHOPT ; do
    TEST_RESULT=$( $rhash --$opt --simple -m "a" )
    match "$TEST_RESULT" "\b[0-9a-z]\{8,128\}\b" . || errors=$((errors+1))
  done
  check $errors 0
fi

new_test "test checking all hashes:   "
TEST_RESULT=$( $rhash --simple -a test1K.data | $rhash -vc - 2>/dev/null | grep test1K.data )
match "$TEST_RESULT" "^test1K.data *OK"

new_test "test checking magnet link:  "
# also test that '--check' verifies files in the current directory
mkdir magnet_dir && $rhash --magnet -a test1K.data > magnet_dir/t.magnet
TEST_RESULT=$( $rhash -vc magnet_dir/t.magnet 2>&1 | grep test1K.data )
TEST_EXPECTED="^test1K.data *OK"
match "$TEST_RESULT" "$TEST_EXPECTED"

new_test "test bsd format checking:   "
TEST_RESULT=$( $rhash --bsd -a test1K.data | $rhash -c --skip-ok - 2>&1 | grep -v '^--' | grep -v '^$' )
check "$TEST_RESULT" "Everything OK"

new_test "test checking w/o filename: "
$rhash -p '%c\n%m\n%e\n%h\n%g\n%t\n%a\n' test1K.data > t.sum
TEST_RESULT=$( $rhash -vc t.sum 2>&1 | grep '^test1K.data' | grep -v ' OK *$' )
check "$TEST_RESULT" ""

new_test "test checking embedded crc: "
printf 'A' > 'test_[D3D99E8B].data' && printf 'A' > 'test_[D3D99E8C].data'
# first verify checking an existing crc32 while '--embed-crc' option is set
TEST_RESULT=$( $rhash -C --simple 'test_[D3D99E8B].data' | $rhash -vc --embed-crc - 2>/dev/null | grep data )
match "$TEST_RESULT" "^test_.*OK" .
TEST_RESULT=$( $rhash -C --simple 'test_[D3D99E8C].data' | $rhash -vc --embed-crc - 2>/dev/null | grep data )
match "$TEST_RESULT" "^test_.*ERROR, embedded CRC32 should be" .
# second verify --check-embedded option
TEST_RESULT=$( $rhash --check-embedded 'test_[D3D99E8B].data' 2>/dev/null | grep data )
match "$TEST_RESULT" "test_.*OK" .
TEST_RESULT=$( $rhash --check-embedded 'test_[D3D99E8C].data' 2>/dev/null | grep data )
match "$TEST_RESULT" "test_.*ERR" .
mv 'test_[D3D99E8B].data' 'test.data'
# test --embed-crc and --embed-crc-delimiter options
TEST_RESULT=$( $rhash --simple --embed-crc --embed-crc-delimiter=_ 'test.data' 2>/dev/null )
check "$TEST_RESULT" "d3d99e8b  test_[D3D99E8B].data"
rm 'test_[D3D99E8B].data' 'test_[D3D99E8C].data'

new_test "test checking recursively:  "
mkdir -p check/a && cp test1K.data check/a/b.data
echo "a/b.data B70B4C26" > check/b.sfv
TEST_RESULT=$( $rhash -Crc check/ | grep b.data )
match "$TEST_RESULT" "^a/b.data *OK" .
echo "B70B4C26" > check/a/b.data.crc32
TEST_RESULT=$( $rhash --crc-accept=.crc32 -Crc check/a | grep "data.*OK" )
match "$TEST_RESULT" "^check/a.b.data *OK" .
# test that hash-files specified explicitly by command line are checked
# in the current directory even with '--recursive' option
echo "test1K.data B70B4C26" > check/t.sfv
TEST_RESULT=$( $rhash -Crc check/t.sfv | grep "data.*OK" )
match "$TEST_RESULT" "^test1K.data *OK"

new_test "test wrong sums detection:  "
$rhash -p '%c\n%m\n%e\n%h\n%g\n%t\n%a\n%w\n' -m WRONG > t.sum
TEST_RESULT=$( $rhash -vc t.sum 2>&1 | grep 'OK' )
check "$TEST_RESULT" ""
rm t.sum

new_test "test *accept options:       "
mkdir -p test_dir/a && touch test_dir/a/file.txt test_dir/a/file.bin
# correctly handle MIGW posix path conversion
echo "$MSYSTEM" | grep -q '^MINGW[36][24]' && SLASH=// || SLASH="/"
# test also --path-separator option
TEST_RESULT=$( $rhash -rC --simple --accept=.bin --path-separator=$SLASH test_dir )
check "$TEST_RESULT" "00000000  test_dir/a/file.bin" .
TEST_RESULT=$( $rhash -rC --simple --accept=.txt --path-separator=\\ test_dir/a )
check "$TEST_RESULT" "00000000  test_dir\\a\\file.txt" .
TEST_RESULT=$( $rhash -rc --crc-accept=.bin test_dir 2>/dev/null | sed -n '/Verifying/s/-//gp' )
match "$TEST_RESULT" "( Verifying test_dir.a.file\\.bin )"

new_test "test ignoring of log files: "
touch t1.out t2.out
TEST_RESULT=$( $rhash -C --simple t1.out t2.out -o t1.out -l t2.out 2>/dev/null )
check "$TEST_RESULT" "" .
TEST_RESULT=$( $rhash -c t1.out t2.out -o t1.out -l t2.out 2>/dev/null )
check "$TEST_RESULT" ""
rm t1.out t2.out

new_test "test creating torrent file: "
TEST_RESULT=$( $rhash --btih --torrent --bt-private --bt-piece-length=512 --bt-announce=http://tracker.org/ test1K.data 2>/dev/null )
check "$TEST_RESULT" "29f7e9ef0f41954225990c513cac954058721dd2  test1K.data"
rm test1K.data.torrent

new_test "test exit code:             "
rm -f none-existent.file
test -f none-existent.file && print_failed .
$rhash -H none-existent.file 2>/dev/null
check "$?" "1" .
$rhash -c none-existent.file 2>/dev/null
check "$?" "1" .
$rhash -H test1K.data >/dev/null
check "$?" "0"
UNWRITABLE_FILE="$RHASH_TMP/test-unwritable.file"
printf "" > "$UNWRITABLE_FILE" && chmod a-w "$UNWRITABLE_FILE"
# check if really unwritable, since superuser still can write
if ! test -w "$UNWRITABLE_FILE" ; then
 $rhash -o "$UNWRITABLE_FILE" -H test1K.data 2>/dev/null
 check "$?" "2" .
fi
rm -f "$UNWRITABLE_FILE"

# check if any test failed
if [ $fail_cnt -gt 0 ]; then
  echo "Failed $fail_cnt checks"
  exit 1 # some tests failed
fi

exit 0 # success
