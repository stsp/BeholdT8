#!/usr/bin/perl
use strict;
use File::Find;
use File::Path;
use File::Copy;
use Fcntl ':mode';
use Getopt::Long;
use Digest::SHA1;

#######################################
# FIXME: need to handle applied patches
#######################################

my $silent = 0;
my $debug = 0;
my $recheck = 0;
GetOptions( "--debug" => \$debug,
	    "--silent" => \$silent,
	    "--recheck" => \$recheck,
	  );

my $dir = shift;

my $ctlfile = ".linked_dir";

my %dirs;
my %files;

#########################################
# Control info stored at the control file
my $path;
my %fhash;
#########################################

sub read_ctlfile()
{
	my $line;

	open IN, $ctlfile or return;
	while (<IN>) {
		next if (m/^\s*\#/);
		next if (m/^\n$/);
		if (m/^path:\s*([^\s]+)/) {
			$path = $1;
		} elsif (m/^hash\:\s*([^\s]+)\s*=\s*([^\s]+)/) {
			$fhash{$1} = $2;
		} else {
			printf("Parse error on this line of $ctlfile:\n\t$_");
			die;
		}
	}
	close IN;
}

sub write_ctlfile()
{
	open OUT, ">$ctlfile" or print "Error: Can't write to $ctlfile\n";
	print OUT "path: $path\n";
	foreach my $file (keys %fhash) {
		printf OUT "hash: %s=%s\n", $file, $fhash{$file};
	}
	close OUT;
}

sub add_dirs($)
{
	my $data = shift;
	my @dirs = split(' ', $data);

	foreach my $val (@dirs) {
		$dirs{$val} = 1;
	}
}

sub add_files($)
{
	my $data = shift;
	my @dirs = split(' ', $data);

	foreach my $val (@dirs) {
		$files{$val} = 1;
	}
}

sub get_file_dir_names()
{
	open IN, "Makefile" or die "Couldn't open Makefile";
	while (<IN>) {
		if (m/^\s*TARDIR\s*[\+\:]*=\s*([A-Za-z_].*)/) {
			add_dirs($1);
		} elsif (m/^\s*TARFILES\s*[\+\:]*=\s*([A-Za-z_].*)/) {
			add_files($1);
		}
	}
	close IN;
}


sub hash_calc($)
{
	my $file = shift;

	my $ctx = Digest::SHA1->new;

	my $rc = open IN, $file;
	if (!$rc) {
		print "Couldn't open file $file\n" if ($debug || $recheck);
		return 0;
	}
	$ctx->addfile(*IN);
	my $digest = $ctx->hexdigest;
	close IN;

	return $digest;
}


sub sync_files($)
{
	my $file = shift;
	my $path = $file;
	my $filehash;

	$path =~ s,/[^/]+$,,;


	$filehash = hash_calc("$dir/$file");

	if ($recheck) {
		$fhash{$file} = hash_calc("$file");
	}

	if (!exists($fhash{$file}) || ($filehash ne $fhash{$file})) {
		printf "Re-syncying file $file (orig = %s, copy = %s)\n", $filehash, $fhash{$file} if ($recheck);
		print "install -D $dir/$file $file\n" if ($debug);

		$fhash{$file} = $filehash;
		mkpath($path);
		copy("$dir/$file", $file);
	} else {
		print "Skipping file $file, as is already synchronized\n" if ($debug);
	}
}

sub parse_dir()
{
	my $file = $File::Find::name;
	my $mode = (stat($file))[2];

	return if ($mode & S_IFDIR);

	$file =~ s,^($dir/),,;

	return if ($file =~ /^\./);
	return if ($file =~ /\.mod\.c/);

	if ($file =~ /Makefile$/ || $file =~ /Kconfig$/ || $file =~ /\.[ch]$/ ) {
		sync_files $file;
		return;
	}

	printf "Skipping bogus file $file\n" if ($debug);
}

sub sync_dirs($)
{
	my $subdir = shift;

	print "sync dir: $subdir\n" if (!$silent);

	find({wanted => \&parse_dir, no_chdir => 1}, "$dir/$subdir");
}

sub sync_all()
{
	foreach my $val (keys %files) {
		print "sync file: $val\n" if (!$silent);
		sync_files($val);
	}
	foreach my $val (keys %dirs) {
		sync_dirs($val);
	}
}

# Main

if (!$dir) {
	read_ctlfile();
	die "Please provide a directory to use" if !($path);
	$dir = $path;

	printf "Syncing with dir $dir\n";
} else {
	read_ctlfile();
}

if ($path ne $dir) {
	$path = $dir;
	%fhash = ();
}

get_file_dir_names();
sync_all();
write_ctlfile();

unlink ".patches_applied" if ($recheck);
