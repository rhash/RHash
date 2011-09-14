require 'mkmf'

if ENV['LIBRHASH_INC'] and ENV['LIBRHASH_LD']
    $CFLAGS  += ENV['LIBRHASH_INC']
    $LDFLAGS += ' ' + ENV['LIBRHASH_LD']
else
    have_header('rhash/rhash.h')
end
$LDFLAGS += ' -lrhash'

dir_config('rhash')
create_makefile('rhash')
