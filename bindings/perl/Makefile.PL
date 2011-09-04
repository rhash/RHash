use strict;
use warnings;

use ExtUtils::MakeMaker;

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    NAME         => 'Rhash',
    AUTHOR        => 'Aleksey Kravchenko',
    VERSION_FROM => 'Rhash.pm', # finds $VERSION
    LIBS         => ['-lrhash'],
    DEFINE       => '',     # e.g., '-DHAVE_SOMETHING'
    INC          => '',     # e.g., '-I/usr/include/other'
);

