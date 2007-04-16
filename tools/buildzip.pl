#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

$ROOT="..";

my $ziptool="zip";
my $output="rockbox.zip";
my $verbose;
my $exe;
my $target;
my $archos;
my $incfonts;

while(1) {
    if($ARGV[0] eq "-r") {
        $ROOT=$ARGV[1];
        shift @ARGV;
        shift @ARGV;    
    }

    elsif($ARGV[0] eq "-z") {
        $ziptool=$ARGV[1];
        shift @ARGV;
        shift @ARGV;    
    }

    elsif($ARGV[0] eq "-t") {
        # The target name as used in ARCHOS in the root makefile
        $archos=$ARGV[1];
        shift @ARGV;
        shift @ARGV;    
    }
    elsif($ARGV[0] eq "-o") {
        $output=$ARGV[1];
        shift @ARGV;
        shift @ARGV;    
    }
    elsif($ARGV[0] eq "-f") {
        $incfonts=$ARGV[1]; # 0 - no fonts, 1 - fonts only 2 - fonts and package
        shift @ARGV;
        shift @ARGV;    
    }

    elsif($ARGV[0] eq "-v") {
        $verbose =1;
        shift @ARGV;
    }
    else {
        $target = $ARGV[0];
        $exe = $ARGV[1];
        last;
    }
}


my $firmdir="$ROOT/firmware";
my $appsdir="$ROOT/apps";
my $viewer_bmpdir="$ROOT/apps/plugins/bitmaps/viewer_defaults";

my $cppdef = $target;

sub gettargetinfo {
    open(GCC, ">gcctemp");
    # Get the LCD screen depth and graphical status
    print GCC <<STOP
\#include "config.h"
#ifdef HAVE_LCD_BITMAP
Bitmap: yes
Depth: LCD_DEPTH
Icon Width: CONFIG_DEFAULT_ICON_WIDTH
Icon Height: CONFIG_DEFAULT_ICON_HEIGHT
#endif
Codec: CONFIG_CODEC
#ifdef HAVE_REMOTE_LCD
Remote Depth: LCD_REMOTE_DEPTH
Remote Icon Width: CONFIG_REMOTE_DEFAULT_ICON_WIDTH
Remote Icon Height: CONFIG_REMOTE_DEFAULT_ICON_HEIGHT
#else
Remote Depth: 0
#endif
STOP
;
    close(GCC);

    my $c="cat gcctemp | gcc $cppdef -I. -I$firmdir/export -E -P -";

    # print STDERR "CMD $c\n";

    open(TARGET, "$c|");

    my ($bitmap, $depth, $swcodec, $icon_h, $icon_w);
    my ($remote_depth, $remote_icon_h, $remote_icon_w);
    $icon_count = 1;
    while(<TARGET>) {
        # print STDERR "DATA: $_";
        if($_ =~ /^Bitmap: (.*)/) {
            $bitmap = $1;
        }
        elsif($_ =~ /^Depth: (\d*)/) {
            $depth = $1;
        }
        elsif($_ =~ /^Icon Width: (\d*)/) {
            $icon_w = $1;
        }
        elsif($_ =~ /^Icon Height: (\d*)/) {
            $icon_h = $1;
        }
        elsif($_ =~ /^Codec: (\d*)/) {
            # SWCODEC is 1, the others are HWCODEC
            $swcodec = ($1 == 1);
        }
        elsif($_ =~ /^Remote Depth: (\d*)/) {
            $remote_depth = $1;
        }
        elsif($_ =~ /^Remote Icon Width: (\d*)/) {
            $remote_icon_w = $1;
        }
        elsif($_ =~ /^Remote Icon Height: (\d*)/) {
            $remote_icon_h = $1;
        }
    }
    close(TARGET);
    unlink("gcctemp");

    return ($bitmap, $depth, $icon_w, $icon_h,
            $swcodec, $remote_depth, $remote_icon_w, $remote_icon_h);
}

sub filesize {
    my ($filename)=@_;
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks)
        = stat($filename);
    return $size;
}

sub buildlangs {
    my ($outputlang)=@_;
    my $dir = "$ROOT/apps/lang";
    opendir(DIR, $dir);
    my @files = grep { /\.lang$/ } readdir(DIR);
    closedir(DIR);

    for(@files) {
        my $output = $_;
        $output =~ s/(.*)\.lang/$1.lng/;
        print "$ROOT/tools/genlang -e=$dir/english.lang -t=$archos -b=$outputlang/$output $dir/$_\n" if($verbose);
        system ("$ROOT/tools/genlang -e=$dir/english.lang -t=$archos -b=$outputlang/$output $dir/$_ >/dev/null 2>&1");
    }
}

sub buildzip {
    my ($zip, $image, $fonts)=@_;

    my ($bitmap, $depth, $icon_w, $icon_h, $swcodec,
        $remote_depth, $remote_icon_w, $remote_icon_h) = &gettargetinfo();

    # print "Bitmap: $bitmap\nDepth: $depth\nSwcodec: $swcodec\n";

    # remove old traces
    `rm -rf .rockbox`;

    mkdir ".rockbox", 0777;

    if(!$bitmap) {
        # always disable fonts on non-bitmap targets
        $fonts = 0;
    }

    if($fonts) {
        mkdir ".rockbox/fonts", 0777;

        opendir(DIR, "$ROOT/fonts") || die "can't open dir fonts";
        my @fonts = grep { /\.bdf$/ && -f "$ROOT/fonts/$_" } readdir(DIR);
        closedir DIR;

        for(@fonts) {
            my $f = $_;

            print "FONT: $f\n" if($verbose);
            my $o = $f;
            $o =~ s/\.bdf/\.fnt/;
            my $cmd ="$ROOT/tools/convbdf -f -o \".rockbox/fonts/$o\" \"$ROOT/fonts/$f\" >/dev/null 2>&1";
            print "CMD: $cmd\n" if($verbose);
            `$cmd`;
            
        }
        if($fonts < 2) {
          # fonts-only package, return
          return;
        }
    }

    mkdir ".rockbox/langs", 0777;
    mkdir ".rockbox/rocks", 0777;

    if($swcodec) {
        mkdir ".rockbox/eqs", 0777;

        `cp $ROOT/apps/eqs/*.cfg .rockbox/eqs/`; # equalizer presets
    }

    mkdir ".rockbox/wps", 0777;
    mkdir ".rockbox/themes", 0777;
    mkdir ".rockbox/codepages", 0777;

    if($bitmap) {
        system("$ROOT/tools/codepages");
    }
    else {
        system("$ROOT/tools/codepages -m");
    }
    $c = 'find . -name "*.cp" ! -empty -exec mv {} .rockbox/codepages/ \; >/dev/null 2>&1';
    `$c`;

    if($bitmap) {
        mkdir ".rockbox/codecs", 0777;
        if($depth > 1) {
            mkdir ".rockbox/backdrops", 0777;
        }

        my $c = 'find apps -name "*.codec" ! -empty -exec cp {} .rockbox/codecs/ \; 2>/dev/null';
        `$c`;

        my @call = `find .rockbox/codecs -type f 2>/dev/null`;
        if(!$call[0]) {
            # no codec was copied, remove directory again
            rmdir(".rockbox/codecs");

        }
    }

    $c= 'find apps "(" -name "*.rock" -o -name "*.ovl" ")" ! -empty -exec cp {} .rockbox/rocks/ \;';
    print `$c`;

    open VIEWERS, "$ROOT/apps/plugins/viewers.config" or
        die "can't open viewers.config";
    @viewers = <VIEWERS>;
    close VIEWERS;

    open VIEWERS, ">.rockbox/viewers.config" or
        die "can't create .rockbox/viewers.config";
    mkdir ".rockbox/viewers", 0777;
    foreach my $line (@viewers) {
        if ($line =~ /([^,]*),([^,]*),/) {
            my ($ext, $plugin)=($1, $2);
            my $r = "${plugin}.rock";
            my $oname;

            my $dir = $r;
            my $name;

            # strip off the last slash and file name part
            $dir =~ s/(.*)\/(.*)/$1/;
            # store the file name part
            $name = $2;

            # get .ovl name (file part only)
            $oname = $name;
            $oname =~ s/\.rock$/.ovl/;

            # print STDERR "$ext $plugin $dir $name $r\n";

            if(-e ".rockbox/rocks/$name") {
                if($dir ne "rocks") {
                    # target is not 'rocks' but the plugins are always in that
                    # dir at first!
                    `mv .rockbox/rocks/$name .rockbox/$r`;
                }
                print VIEWERS $line;
            }
            elsif(-e ".rockbox/$r") {
                # in case the same plugin works for multiple extensions, it
                # was already moved to the viewers dir
                print VIEWERS $line;
            }

            if(-e ".rockbox/rocks/$oname") {
                # if there's an "overlay" file for the .rock, move that as
                # well
                `mv .rockbox/rocks/$oname .rockbox/$dir`;
            }
        }
    }
    close VIEWERS;
    
    if ($bitmap) {
        mkdir ".rockbox/icons", 0777;
        `cp $viewer_bmpdir/viewers.${icon_w}x${icon_h}x$depth.bmp .rockbox/icons/viewers.bmp`;
        if ($remote_depth) {
            `cp $viewer_bmpdir/remote_viewers.${remote_icon_w}x${remote_icon_h}x$remote_depth.bmp .rockbox/icons/remote_viewers.bmp`;
        }
    }
    
    `cp $ROOT/apps/tagnavi.config .rockbox/`;
      
    if($bitmap) {
        `cp $ROOT/apps/plugins/sokoban.levels .rockbox/rocks/`; # sokoban levels
        `cp $ROOT/apps/plugins/snake2.levels .rockbox/rocks/`; # snake2 levels
    }

    if($image) {
        # image is blank when this is a simulator
        if( filesize("rockbox.ucl") > 1000 ) {
            `cp rockbox.ucl .rockbox/`;  # UCL for flashing
        }
        if( filesize("rombox.ucl") > 1000) {
            `cp rombox.ucl .rockbox/`;  # UCL for flashing
        }
        
        # Check for rombox.target
        if ($image=~/(.*)\.(\w+)$/)
        {
            my $romfile = "rombox.$2";
            if (filesize($romfile) > 1000)
            {
                `cp $romfile .rockbox/`;
            }
        }
    }

    mkdir ".rockbox/docs", 0777;
    for(("COPYING",
         "LICENSES",
        )) {
        `cp $ROOT/docs/$_ .rockbox/docs/$_.txt`;
    }

    # Now do the WPS dance
    if(-d "$ROOT/wps") {
        system("perl $ROOT/wps/wpsbuild.pl -r $ROOT $ROOT/wps/WPSLIST $target");
    }
    else {
        print STDERR "No wps module present, can't do the WPS magic!\n";
    }

    # now copy the file made for reading on the unit:
    #if($notplayer) {
    #    `cp $webroot/docs/Help-JBR.txt .rockbox/docs/`;
    #}
    #else {
    #    `cp $webroot/docs/Help-Stu.txt .rockbox/docs/`;
    #}

    buildlangs(".rockbox/langs");

}

my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
 localtime(time);

$mon+=1;
$year+=1900;

#$date=sprintf("%04d%02d%02d", $year,$mon, $mday);
#$shortdate=sprintf("%02d%02d%02d", $year%100,$mon, $mday);

# made once for all targets
sub runone {
    my ($target, $fonts)=@_;

    # build a full install .rockbox directory
    buildzip($output, $target, $fonts);

    # create a zip file from the .rockbox dfir

    `rm -f $output`;
    if($verbose) {
      print "find .rockbox | xargs $ziptool $output >/dev/null\n";
    }
    `find .rockbox | xargs $ziptool $output >/dev/null`;

    if($target && ($fonts != 1)) {
        # On some targets, rockbox.* is inside .rockbox
        if($target !~ /(mod|ajz|wma)\z/i) {
            `cp $target .rockbox/$target`;
            $target = ".rockbox/".$target;
        }
        
        if($verbose) {
            print "$ziptool $output $target\n";
        }
        `$ziptool $output $target`;
    }

    # remove the .rockbox afterwards
    `rm -rf .rockbox`;
};

if(!$exe) {
    # not specified, guess!
    if($target =~ /(recorder|ondio)/i) {
        $exe = "ajbrec.ajz";
    }
    elsif($target =~ /iriver/i) {
        $exe = "rockbox.iriver";
    }
    else {
        $exe = "archos.mod";
    }
}
elsif($exe =~ /rockboxui/) {
    # simulator, exclude the exe file
    $exe = "";
}

runone($exe, $incfonts);


