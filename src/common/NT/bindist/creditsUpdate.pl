#!/usr/bin/perl
# 
# ./src/common/NT/bindist/makeCreditsinc.pl
#
# Build the Windows' credit text from the CREDITS file
# The output of this file is C++ source and gets compiled into the client.
#
# $Log: creditsUpdate.pl,v $
# Revision 1.1  2003/04/10 12:24:31  kps
# 4.5.4X rc10
#
# Revision 5.1  2002/06/15 18:16:31  dik
# Windows dynamically generates credits.inc.h to display in the scrolling
# credits window.
#
# Revision 1.2  2002/06/13 23:27:17  dick
# Anything ending in : is a Header line
#
# Revision 1.1  2002/06/03 17:42:31  dick
# Windows generates source file credits.inc.h from CREDITS to be included
# in the About box.
#

$debug = 0;

$CREDITS="../../../../doc/CREDITS";
#    Jarno van der Kolk      Bugfix, Windows recording,

$CREDITSINCFILE="credits.inc.h";
$CREDITSINC="../../../client/NT/$CREDITSINCFILE";
#credits += "#r001#c005Jarno van der Kolk#c025Bugfix, Windows recording,";


open (INPUT, "<$CREDITS") || die "Cant open $CREDITS";
open (OUTPUT, ">$CREDITSINC") || die "Cant open output $CREDITSINC\n";

select OUTPUT;

#use POSIX qw(strftime);
#$now_string = strftime "%a %b %d %Y %H:%M:%S", gmtime;
print "// $CREDITSINCFILE\n";
print "// Generated by $0\n\n";
#print "// $now_string\n\n";

print qq~credits = "";\n~;

$header=0;
$title=1;

$mode=$header;
$blanklines = 0;
$linecount = 0;
$sourcecount = 0;
while ($line = <INPUT>)
{
	$linecount++;
	$sourcecount++;
	#chomp $line;
	chop $line;
	chop $line;
	$blankline = 0;
	if (!$line =~ /\w/)		# contains a non-space
	{
		$blanklines++;
		next;
	}
		
	$r = sprintf("%03d",$blanklines+1);
	if ($line =~ /^+\S/)	# starts with a non-space
	{						# intro text or Release #
		print qq~credits += "#r$r$line";\n~;
		$blanklines = 0;
		# squash the blank line after 'Release'
		#if ($line =~ /^Release/ || $line =~ /^Version/ || $line =~ /Pre /)
		if ($line =~ /\:$/)
		{
			$linecount--;
			$blanklines = -1;
		}
	}
	else					
	{						# it must be a credit
		$human = substr($line, 4, 24);
		$task  = substr($line, 28, 999);
		print STDERR "$line\n" if ($debug);
		print STDERR "human=<$human>\n" if ($debug);
		$human = $` if ($human =~ /\s+$/);	# trim trailing spaces
		print STDERR "human=<$human>\n" if ($debug);
		#$human= "" if (!$human =~ /\w/);
		$task = "" if (!$task  =~ /\w/);
		print STDERR "human=<$human>\n" if ($debug);
		print STDERR "task=<$task>\n" if ($debug);
		if ( !length($human) && !length($task))
		{
			print STDERR "Error, no human or task on line $linecount\n";
			die;
		}
		print qq ~credits += "#r$r~;
		print qq ~#c005$human~ if (length($human));
		print qq ~#c025$task~  if (length($task));
		print qq ~";\n~;
		$blanklines = 0;
	}
	#print $line;
}
print qq ~credits += "#r000";\n~;		# flush the last line when building the bitmap
$linecount++;

print STDERR "linecount=$linecount\n" if ($debug);
print "lineCount = $linecount;\n";