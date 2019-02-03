use strict;
use warnings;
use Cwd;
use ExtUtils::MakeMaker;
use File::Copy;

my $libs  = '';
my $inc   = '';
my $obj   = '';
my $clean = '';
my $inc_dir = undef;
my $lib_dir = undef;
my $rh_builtin_dir = 'librhash';
my $rh_local_dir   = getcwd() . '/../../librhash';
my $rh_type = $ENV{'LIBRHASH'} || 'auto';
my $custom_inc = $ENV{'LIBRHASH_INC'} || '';
my $custom_ld  = $ENV{'LIBRHASH_LD'}  || '';

sub has_librhash {
	return (-f $_[0] . '/rhash.h');
}

if ($rh_type eq 'auto')
{
	$rh_type = ($custom_ld =~ /-L/ ? 'custom' :
		has_librhash($rh_builtin_dir) ? 'builtin' :
		has_librhash($rh_local_dir) ? 'local' : 'system' );
	print "Selected librhash type: $rh_type\n";
}
if ($rh_type ne 'custom')
{
	$inc_dir = ($rh_type eq 'builtin' ? $rh_builtin_dir :
		$rh_type eq 'local' ? $rh_local_dir :
		$rh_type eq 'system' ? '' : die("Unknown type LIBRHASH='$rh_type'"));
	$lib_dir = $inc_dir if $rh_type ne 'builtin';
	!$inc_dir || -d $inc_dir || die "Not a directory: '$inc_dir'";
	!$inc_dir || has_librhash($inc_dir) || die "No librhash headers at: '$inc_dir'";
	$inc  = "-I$inc_dir" if $inc_dir;
	$libs = "-L$lib_dir" if $lib_dir;
	$libs .= ' -lrhash'  if $rh_type ne 'builtin';
} else {
	# set custom compile and linking flags
	$inc = $custom_inc;
	$libs = $custom_ld;
}

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

if($rh_type eq 'builtin') {
	# using sources of the builtin librhash
	my @c_files = copy_c_files($rh_builtin_dir);
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
                bugtracker  => 'https://github.com/rhash/RHash/issues',
                license     => 'https://github.com/rhash/RHash/blob/master/COPYING',
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
