use strict;
use warnings;
use ExtUtils::MakeMaker;

my $libs = '-lrhash';
my $inc = '';
$inc = $ENV{'LIBRHASH_INC'} if defined($ENV{'LIBRHASH_INC'});
if(defined($ENV{'LIBRHASH_LD'}) && $ENV{'LIBRHASH_LD'} =~ /-L/) {
    $libs = $ENV{'LIBRHASH_LD'} . ' ' . $libs;
}

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    NAME         => 'Rhash',
    AUTHOR       => 'Aleksey Kravchenko',
    VERSION_FROM => 'Rhash.pm', # finds $VERSION
    LIBS         => [ $libs ],
    DEFINE       => '',       # e.g., '-DHAVE_SOMETHING'
    INC          => $inc,     # e.g., '-I/usr/include/other'
);
