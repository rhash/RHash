use strict;
use warnings;
use ExtUtils::MakeMaker;
use File::Copy;

my $libs  = '';
my $inc   = '';
my $obj   = '';
my $clean = '';
my $local_librhash_dir = 'librhash';
my $has_builtin_librhash = (-f $local_librhash_dir . '/rhash.h');

# set the location of LibRHash headers and the linking flags
$inc = $ENV{'LIBRHASH_INC'} if defined($ENV{'LIBRHASH_INC'});
if(defined($ENV{'LIBRHASH_LD'}) && $ENV{'LIBRHASH_LD'} =~ /-L/) {
	$libs = $ENV{'LIBRHASH_LD'} . ' ' . $libs;
}

# use a system-wide librhash
$libs = '-lrhash' if (defined($ENV{'USE_SYSTEM_LIBRHASH'}) || ($libs eq '' && !$has_builtin_librhash));

# copy and rename *.c files by prepending underscore '_'
sub copy_c_files($) {
	my $from_dir = $_[0];
	my @result = ();
	(opendir my($dh), $from_dir) or die "Can't open $from_dir: $!";
	my @files = grep { /(?<!\Atest_hashes)\.c$/ } readdir $dh;
	closedir $dh;
	for (@files) {
		my ($from, $to) = ("$from_dir/$_", "_$_");
		push @result, $to;

		my ($df, $dt) = ((stat($from))[9], (stat($to))[9]);
		next if(defined($dt) && defined($df) && $dt >= $df);
		#print "copy $from -> $to\n";
		copy($from, $to)
			or die "Can't copy $from to $to: $!";
	}
	return @result;
}

if($inc eq '' && $libs eq '' && $has_builtin_librhash) {
	# use the local version of librhash
	print "Using builtin LibRHash\n";
	$inc = '-I' . $local_librhash_dir;
	my @c_files = copy_c_files($local_librhash_dir);
	$clean = join(' ', @c_files);
	$obj = join(' ', map { s/\.c$/\$(OBJ_EXT)/; $_ } @c_files) . ' ';
}

# make setting optional MakeMaker parameters more readable
sub OPTIONAL {
	return () unless $ExtUtils::MakeMaker::VERSION ge shift;
	return @_;
}

# see ExtUtils::MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written
WriteMakefile(
    NAME         => 'Crypt::Rhash',
    ABSTRACT     => 'Library for computing hash sums and magnet links',
    AUTHOR       => 'Aleksey Kravchenko',
    VERSION_FROM => 'Rhash.pm', # finds $VERSION
    OPTIONAL( '6.31',
        LICENSE => 'unrestricted',
    ),
    OPTIONAL( '6.46',
        # Use META_ADD instead of META_MERGE so that we can remove
        # any build-time dependencies that MakeMaker will put into
        # the requires field.
        META_ADD => {
            resources => {
                homepage    => 'http://rhash.sf.net/',
                license     => 'http://rhash.anz.ru/license.php',
                bugtracker  => 'https://sourceforge.net/p/rhash/bugs/',
                repository  => 'https://github.com/rhash/RHash',
            },
        },
    ),

    LIBS         => [ $libs ],
    DEFINE       => '',       # e.g., '-DHAVE_SOMETHING'
    INC          => $inc,     # e.g., '-I/usr/include/other'
    OBJECT       => $obj . 'Rhash$(OBJ_EXT)',
    clean        => {
        FILES => $clean,
    },
);
