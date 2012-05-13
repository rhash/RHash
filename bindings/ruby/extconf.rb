require 'mkmf'

if ENV['LIBRHASH_INC']
    $CFLAGS  += ENV['LIBRHASH_INC']
else
    have_header('rhash.h')
end

$LDFLAGS += ' ' + ENV['LIBRHASH_LD'] if ENV['LIBRHASH_LD']
$LDFLAGS += ' -lrhash'

dir_config('rhash')
create_makefile('rhash')
