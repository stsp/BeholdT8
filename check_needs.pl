#!/usr/bin/perl

my $need = 0;
sub findprog($)
{
	foreach(split(/:/, $ENV{PATH})) {
		return "$_/$_[0]" if(-x "$_/$_[0]");
	}
}

sub need_program
{
	my $prog = shift;
	my $pkgname = shift;

	return if findprog($prog);
	print "ERROR: please install \"$prog\", otherwise, build won't work.";
	print " This program is generally found at \"$pkgname\" package." if ($pkgname);
	print "\n";

	$need++;
}

sub need_perl_module
{
	my $prog = shift;
	my $pkgname = shift;

	my $err = system("perl -M$prog -e 1 2>/dev/null /dev/null");
	return if ($err == 0);
	print "ERROR: please install \"$prog\", otherwise, build won't work.";
	print " This program is generally found at \"$pkgname\" package." if ($pkgname);
	print "\n";

	$need++;
}

# Check for needed programs/tools
need_program "git";
need_program "make";
need_program "gcc";
need_program "patch";
need_program "lsdiff", "patchutils";

# Check for needed perl modules
need_perl_module "Digest::SHA1", "perl-Digest-SHA1";
need_perl_module "Proc::ProcessTable", "perl-Proc-ProcessTable";

die "Build can't procceed as $need dependency is missing" if ($need == 1);
die "Build can't procceed as $need dependencies are missing" if ($need);
