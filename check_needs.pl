#!/usr/bin/perl

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
	print "please install \"$prog\", otherwise, build won't work.";
	print " This program is generally found at \"$pkgname\" package." if ($pkgname);
	print "\n";

	die "need $prog";
}

need_program "git";
need_program "make";
need_program "gcc";
need_program "patch";
need_program "lsdiff", "patchutils";
