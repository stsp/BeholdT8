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

need_program "git";
need_program "make";
need_program "gcc";
need_program "patch";
need_program "lsdiff", "patchutils";

die "Build can't procceed as $need dependency is missing" if ($need == 1);
die "Build can't procceed as $need dependencies are missing" if ($need);
