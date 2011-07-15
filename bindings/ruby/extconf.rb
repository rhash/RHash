require 'mkmf'

have_header('rhash/rhash.h')
$LDFLAGS += ' -lrhash'

dir_config('rhash')
create_makefile('rhash')
