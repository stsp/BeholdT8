#!/usr/bin/perl
use strict;


sub patch_file($$$$)
{
	my $filename = shift;
	my $function = shift;
	my $after_line = shift;
	my $media_build_version = shift;
	my $patched;
	my $warning = "WARNING: You are using an experimental version of the media stack.";

	open IN, "$filename" or die "can't open $filename";
	my $is_function;
	my $file;
	my $org_file;
	while (<IN>) {
		$org_file .= $_;
		next if (m/($warning)/);
		$file .= $_;
		if (m/($function)/) {
			$is_function = 1;
			next;
		};
		next if (!$is_function);
		if (/\}/) {
			$is_function--;
			next;
		};
		if (m/\{/) {
			$is_function++;
			next;
		};
		if ($is_function && m/($after_line)/) {
			$file .= "\tprintk(KERN_ERR \"$warning\\n" .
				 "\\tAs the driver is backported to an older kernel, it doesn't offer\\n" .
				 "\\tenough quality for its usage in production.\\n" .
				 "\\tUse it with care.\\n$media_build_version\\n\");\n";
			$is_function = 0;
			$patched++;
			next;
		};
	}
	close IN;
	if ($org_file ne $file) {
		open OUT, ">$filename.new" or die "Can't open $filename.new";
		print OUT $file;
		close OUT;

		rename "$filename", "$filename~" or die "Can't rename $filename to $filename~";
		rename "$filename.new", "$filename" or die "Can't rename $filename.new to $filename";
		if ($patched) {
			print "Patched $filename\n";
		} else {
			die "$filename was not patched.\n";
		}
	} else {
			print "$filename was already patched\n";
	}
}

#
# Main
#

# Prepare patches message
open IN, "git_log" or die "can't open git_log";
my $logs;
$logs.=$_ while (<IN>);
close IN;
$logs =~ s/\s+$//;
$logs =~ s,\n,\\n\\t,g;
$logs =~ s,\",\\\",g;
$logs = "Latest git patches (needed if you report a bug to linux-media\@vger.kernel.org):\\n\\t$logs";

# Patch dvbdev
patch_file "drivers/media/dvb-core/dvbdev.c", "__init init_dvbdev", "MKDEV", $logs;
# Patch v4l2-dev
patch_file "drivers/media/v4l2-core/v4l2-dev.c", "__init videodev_init", "printk", $logs;
# Patch rc core
patch_file "drivers/media/rc/rc-main.c", "__init rc_core_init", "rc_map_register", $logs;
