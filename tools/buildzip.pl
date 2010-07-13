#!/usr/bin/perl
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

use strict;

use File::Copy; # For move() and copy()
use File::Find; # For find()
use File::Path qw(mkpath rmtree); # For rmtree()
use Cwd 'abs_path';
use Getopt::Long qw(:config pass_through);	# pass_through so not confused by -DTYPE_STUFF

my $ROOT="..";

my $ziptool="zip -r9";
my $output="rockbox.zip";
my $verbose;
my $install;
my $exe;
my $target;
my $modelname;
my $incfonts;
my $target_id; # passed in, not currently used
my $rbdir=".rockbox"; # can be changed for special builds
my $app;


sub glob_mkdir {
    my ($dir) = @_;
    mkpath ($dir, $verbose, 0777);
    return 1;
}

sub glob_install {
    my ($_src, $dest, $opts) = @_;

    unless ($opts) {
        $opts = "-m 0664";
    }

    foreach my $src (glob($_src)) {
        unless ( -d $src || !(-e $src)) {
            system("install -vc $opts \"$src\" \"$dest\"");
            print "install $opts \"$src\" -> \"$dest\"\n" if $verbose;
        }
    }
    return 1;
}

sub glob_copy {
    my ($pattern, $destination) = @_;
    print "glob_copy: $pattern -> $destination\n" if $verbose;
    foreach my $path (glob($pattern)) {
        copy($path, $destination);
    }
}

sub glob_move {
    my ($pattern, $destination) = @_;
    print "glob_move: $pattern -> $destination\n" if $verbose;
    foreach my $path (glob($pattern)) {
        move($path, $destination);
    }
}

sub glob_unlink {
    my ($pattern) = @_;
    print "glob_unlink: $pattern\n" if $verbose;
    foreach my $path (glob($pattern)) {
        unlink($path);
    }
}

sub find_copyfile {
    my ($pattern, $destination) = @_;
    print "find_copyfile: $pattern -> $destination\n" if $verbose;
    return sub {
        my $path = $_;
        if ($path =~ $pattern && filesize($path) > 0 && !($path =~ /$rbdir/)) {
            copy($path, $destination);
            print "cp $path $destination\n" if $verbose;
            chmod(0755, $destination.'/'.$path);
        }
    }
}

sub find_installfile {
    my ($pattern, $destination) = @_;
    print "find_installfile: $pattern -> $destination\n" if $verbose;
    return sub {
        my $path = $_;
        if ($path =~ $pattern) {
        print "FIND_INSTALLFILE: $path\n";
            glob_install($path, $destination);
        }
    }
}


sub make_install {
    my ($src, $dest) = @_;

    my $bindir = $dest;
    my $libdir = $dest;
    my $userdir = $dest;

    my @plugins = ( "games", "apps", "demos", "viewers" );
    my @userstuff = ( "backdrops", "codepages", "docs", "fonts", "langs", "themes", "wps", "eqs", "icons" );
    my @files = ();

    if ($app) {
        $bindir .= "/bin";
        $libdir .= "/lib/rockbox";
        $userdir .= "/share/rockbox";
    } else {
        # for non-app builds we expect the prefix to be the dir above .rockbox
        $bindir .= "/.rockbox";
        $libdir = $userdir = $bindir;
    }
    if ($dest =~ /\/dev\/null/) {
        die "ERROR: No PREFIX given\n"
    }

    if ((!$app) && $src && (abs_path($dest) eq abs_path($src))) {
        return 1;
    }

    # binary
    unless ($exe eq "") {
        unless (glob_mkdir($bindir)) {
            return 0;
        }
        glob_install($exe, $bindir, "-m 0775");
    }

    # codecs
    unless (glob_mkdir("$libdir/codecs")) {
        return 0;
    }
    glob_install("$src/codecs/*", "$libdir/codecs");

    # plugins
    unless (glob_mkdir("$libdir/rocks")) {
        return 0;
    }
    foreach my $t (@plugins) {
        unless (glob_mkdir("$libdir/rocks/$t")) {
            return 0;
        }
        glob_install("$src/rocks/$t/*", "$libdir/rocks/$t");
    }

    # rocks/viewers/lua
    unless (glob_mkdir("$libdir/rocks/viewers/lua")) {
        return 0;
    }
    glob_install("$src/rocks/viewers/lua/*", "$libdir/rocks/viewers/lua");

    # all the rest directories
    foreach my $t (@userstuff) {
        unless (glob_mkdir("$userdir/$t")) {
            return 0;
        }
        glob_install("$src/$t/*", "$userdir/$t");
    }

    # wps/ subfolders and bitmaps
    opendir(DIR, $src . "/wps");
    @files = readdir(DIR);
    closedir(DIR);

    foreach my $_dir (@files) {
        my $dir = "wps/" . $_dir;
        if ( -d "$src/$dir" && $_dir !~ /\.\.?/) {
            unless (glob_mkdir("$userdir/$dir")) {
                return 0;
            }
            glob_install("$src/$dir/*", "$userdir/$dir");
        }
    }

    # rest of the files, excluding the binary
    opendir(DIR,$src);
    @files = readdir(DIR);
    closedir(DIR);

    foreach my $file (grep (/[a-zA-Z]+\.(txt|config|ignnore)/,@files)) {
        glob_install("$src/$file", "$userdir/");
    }
    return 1;
}

# Get options
GetOptions ( 'r|root=s'		=> \$ROOT,
	     'z|ziptool=s'	=> \$ziptool,
	     'm|modelname=s' => \$modelname,  # The model name as used in ARCHOS in the root makefile
	     'i|id=s'		=> \$target_id,  # The target id name as used in TARGET_ID in the root makefile
	     'o|output=s'	=> \$output,
	     'f|fonts=s'	=> \$incfonts,   # 0 - no fonts, 1 - fonts only 2 - fonts and package
	     'v|verbose'	=> \$verbose,
	     'install=s'		=> \$install, # install destination
	     'rbdir=s'          => \$rbdir, # If we want to put in a different directory
    );

($target, $exe) = @ARGV;

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
LCD Width: LCD_WIDTH
LCD Height: LCD_HEIGHT
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
#ifdef HAVE_RECORDING
Recording: yes
#endif
STOP
;
    close(GCC);

    my $c="cat gcctemp | gcc $cppdef -I. -I$firmdir/export -E -P -";

    # print STDERR "CMD $c\n";

    open(TARGET, "$c|");

    my ($bitmap, $width, $height, $depth, $swcodec, $icon_h, $icon_w);
    my ($remote_depth, $remote_icon_h, $remote_icon_w);
    my ($recording);
    my $icon_count = 1;
    while(<TARGET>) {
        # print STDERR "DATA: $_";
        if($_ =~ /^Bitmap: (.*)/) {
            $bitmap = $1;
        }
        elsif($_ =~ /^Depth: (\d*)/) {
            $depth = $1;
        }
        elsif($_ =~ /^LCD Width: (\d*)/) {
            $width = $1;
        }
        elsif($_ =~ /^LCD Height: (\d*)/) {
            $height = $1;
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
        if($_ =~ /^Recording: (.*)/) {
            $recording = $1;
        }
    }
    close(TARGET);
    unlink("gcctemp");

    return ($bitmap, $depth, $width, $height, $icon_w, $icon_h, $recording,
            $swcodec, $remote_depth, $remote_icon_w, $remote_icon_h);
}

sub filesize {
    my ($filename)=@_;
    my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,
        $atime,$mtime,$ctime,$blksize,$blocks)
        = stat($filename);
    return $size;
}

sub buildzip {
    my ($image, $fonts)=@_;    
    my $libdir = $install;
    my $rbdir = ".rockbox";

    print "buildzip: image=$image fonts=$fonts\n" if $verbose;
    
    my ($bitmap, $depth, $width, $height, $icon_w, $icon_h, $recording,
        $swcodec, $remote_depth, $remote_icon_w, $remote_icon_h) =
      &gettargetinfo();

    # print "Bitmap: $bitmap\nDepth: $depth\nSwcodec: $swcodec\n";

    # remove old traces
    rmtree($rbdir);

    glob_mkdir($rbdir);

    if(!$bitmap) {
        # always disable fonts on non-bitmap targets
        $fonts = 0;
    }
    if($fonts) {
        glob_mkdir("$rbdir/fonts");
        chdir "$rbdir/fonts";
        my $cmd = "$ROOT/tools/convbdf -f $ROOT/fonts/*bdf >/dev/null 2>&1";
        print($cmd."\n") if $verbose;
        system($cmd);
        chdir("../../");

        if($fonts < 2) {
          # fonts-only package, return
          return;
        }
    }

    # create the file so the database does not try indexing a folder
    open(IGNORE, ">$rbdir/database.ignore")  || die "can't open database.ignore";
    close(IGNORE);
    
    glob_mkdir("$rbdir/langs");
    glob_mkdir("$rbdir/rocks");
    glob_mkdir("$rbdir/rocks/games");
    glob_mkdir("$rbdir/rocks/apps");
    glob_mkdir("$rbdir/rocks/demos");
    glob_mkdir("$rbdir/rocks/viewers");

    if ($recording) {
        glob_mkdir("$rbdir/recpresets");
    }

    if($swcodec) {
        glob_mkdir("$rbdir/eqs");

        glob_copy("$ROOT/apps/eqs/*.cfg", "$rbdir/eqs/"); # equalizer presets
    }

    glob_mkdir("$rbdir/wps");
    glob_mkdir("$rbdir/themes");
    if ($bitmap) {
        open(THEME, ">$rbdir/themes/rockbox_default_icons.cfg");
        print THEME <<STOP
# this config file was auto-generated to make it
# easy to reset the icons back to default
iconset: -
# taken from apps/gui/icon.c
viewers iconset: /$rbdir/icons/viewers.bmp
remote iconset: -
# taken from apps/gui/icon.c
remote viewers iconset: /$rbdir/icons/remote_viewers.bmp

STOP
;
        close(THEME);
    }
    
    glob_mkdir("$rbdir/codepages");

    if($bitmap) {
        system("$ROOT/tools/codepages");
    }
    else {
        system("$ROOT/tools/codepages -m");
    }

    glob_move('*.cp', "$rbdir/codepages/");

    if($bitmap && $depth > 1) {
        glob_mkdir("$rbdir/backdrops");
    }

    glob_mkdir("$rbdir/codecs");

    find(find_copyfile(qr/.*\.codec/, abs_path("$rbdir/codecs/")), 'apps/codecs');

    # remove directory again if no codec was copied
    rmdir("$rbdir/codecs");

    find(find_copyfile(qr/\.(rock|ovl|lua)/, abs_path("$rbdir/rocks/")), 'apps/plugins');

    open VIEWERS, "$ROOT/apps/plugins/viewers.config" or
        die "can't open viewers.config";
    my @viewers = <VIEWERS>;
    close VIEWERS;

    open VIEWERS, ">$rbdir/viewers.config" or
        die "can't create $rbdir/viewers.config";

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

            if(-e "$rbdir/rocks/$name") {
                if($dir ne "rocks") {
                    # target is not 'rocks' but the plugins are always in that
                    # dir at first!
                    move("$rbdir/rocks/$name", "$rbdir/rocks/$r");
                }
                print VIEWERS $line;
            }
            elsif(-e "$rbdir/rocks/$r") {
                # in case the same plugin works for multiple extensions, it
                # was already moved to the viewers dir
                print VIEWERS $line;
            }

            if(-e "$rbdir/rocks/$oname") {
                # if there's an "overlay" file for the .rock, move that as
                # well
                move("$rbdir/rocks/$oname", "$rbdir/rocks/$dir");
            }
        }
    }
    close VIEWERS;

    open CATEGORIES, "$ROOT/apps/plugins/CATEGORIES" or
        die "can't open CATEGORIES";
    my @rock_targetdirs = <CATEGORIES>;
    close CATEGORIES;
    foreach my $line (@rock_targetdirs) {
        if ($line =~ /([^,]*),(.*)/) {
            my ($plugin, $dir)=($1, $2);
            move("$rbdir/rocks/${plugin}.rock", "$rbdir/rocks/$dir/${plugin}.rock");
            if(-e "$rbdir/rocks/${plugin}.ovl") {
                # if there's an "overlay" file for the .rock, move that as
                # well
                move("$rbdir/rocks/${plugin}.ovl", "$rbdir/rocks/$dir");
            }
            if(-e "$rbdir/rocks/${plugin}.lua") {
                # if this is a lua script, move it to the appropriate dir
                move("$rbdir/rocks/${plugin}.lua", "$rbdir/rocks/$dir/");
            }
        }
    }

    glob_unlink("$rbdir/rocks/*.lua"); # Clean up unwanted *.lua files (e.g. actions.lua, buttons.lua)

    if ($bitmap) {
        glob_mkdir("$rbdir/icons");
        copy("$viewer_bmpdir/viewers.${icon_w}x${icon_h}x$depth.bmp", "$rbdir/icons/viewers.bmp");
        if ($remote_depth) {
            copy("$viewer_bmpdir/remote_viewers.${remote_icon_w}x${remote_icon_h}x$remote_depth.bmp", "$rbdir/icons/remote_viewers.bmp");
        }
    }

    copy("$ROOT/apps/tagnavi.config", "$rbdir/");
    copy("$ROOT/apps/plugins/disktidy.config", "$rbdir/rocks/apps/");

    if($bitmap) {
        copy("$ROOT/apps/plugins/sokoban.levels", "$rbdir/rocks/games/sokoban.levels"); # sokoban levels
        copy("$ROOT/apps/plugins/snake2.levels", "$rbdir/rocks/games/snake2.levels"); # snake2 levels
        copy("$ROOT/apps/plugins/rockbox-fonts.config", "$rbdir/rocks/viewers/");
    }

    if(-e "$rbdir/rocks/demos/pictureflow.rock") {
        copy("$ROOT/apps/plugins/bitmaps/native/pictureflow_emptyslide.100x100x16.bmp",
             "$rbdir/rocks/demos/pictureflow_emptyslide.bmp");
        my ($pf_logo);
        if ($width < 200) {
            $pf_logo = "pictureflow_logo.100x18x16.bmp";
        } else {
            $pf_logo = "pictureflow_logo.193x34x16.bmp";
        }
        copy("$ROOT/apps/plugins/bitmaps/native/$pf_logo",
             "$rbdir/rocks/demos/pictureflow_splash.bmp");

    }

    if($image) {
        # image is blank when this is a simulator
        if( filesize("rockbox.ucl") > 1000 ) {
            copy("rockbox.ucl", "$rbdir/rockbox.ucl");  # UCL for flashing
        }
        if( filesize("rombox.ucl") > 1000) {
            copy("rombox.ucl", "$rbdir/rombox.ucl");  # UCL for flashing
        }
        
        # Check for rombox.target
        if ($image=~/(.*)\.(\w+)$/)
        {
            my $romfile = "rombox.$2";
            if (filesize($romfile) > 1000)
            {
                copy($romfile, "$rbdir/$romfile");
            }
        }
    }

    glob_mkdir("$rbdir/docs");
    for(("COPYING",
         "LICENSES",
         "KNOWN_ISSUES"
        )) {
        copy("$ROOT/docs/$_", "$rbdir/docs/$_.txt");
    }
    if ($fonts) {
        copy("$ROOT/docs/profontdoc.txt", "$rbdir/docs/profontdoc.txt");
    }
    for(("sample.colours",
         "sample.icons"
        )) {
        copy("$ROOT/docs/$_", "$rbdir/docs/$_");
    }

    # Now do the WPS dance
    if(-d "$ROOT/wps") {
	my $wps_build_cmd="perl $ROOT/wps/wpsbuild.pl ";
	$wps_build_cmd=$wps_build_cmd."-v " if $verbose;
	$wps_build_cmd=$wps_build_cmd." --rbdir=$rbdir -r $ROOT -m $modelname $ROOT/wps/WPSLIST $target";
	print "wpsbuild: $wps_build_cmd\n" if $verbose;
        system("$wps_build_cmd");
	print "wps_build_cmd: done\n" if $verbose;
    }
    else {
        print STDERR "No wps module present, can't do the WPS magic!\n";
    }
    
    # until buildwps.pl is fixed, manually copy the classic_statusbar theme across
    mkdir "$rbdir/wps/classic_statusbar", 0777;
    glob_copy("$ROOT/wps/classic_statusbar/*.bmp", "$rbdir/wps/classic_statusbar");
    if ($swcodec) {
		if ($depth == 16) {
			copy("$ROOT/wps/classic_statusbar.sbs", "$rbdir/wps");
		} elsif ($depth > 1) {
			copy("$ROOT/wps/classic_statusbar.grey.sbs", "$rbdir/wps/classic_statusbar.sbs");
		} else {
			copy("$ROOT/wps/classic_statusbar.mono.sbs", "$rbdir/wps/classic_statusbar.sbs");
		}
    } else {
        copy("$ROOT/wps/classic_statusbar.112x64x1.sbs", "$rbdir/wps/classic_statusbar.sbs");
    }
    system("touch $rbdir/wps/rockbox_none.sbs");
    if ($remote_depth != $depth) {
		copy("$ROOT/wps/classic_statusbar.mono.sbs", "$rbdir/wps/classic_statusbar.rsbs");
	} else {
		copy("$rbdir/wps/classic_statusbar.sbs", "$rbdir/wps/classic_statusbar.rsbs");
	}
    copy("$rbdir/wps/rockbox_none.sbs", "$rbdir/wps/rockbox_none.rsbs");

    # and the info file
    copy("rockbox-info.txt", "$rbdir/rockbox-info.txt");

    # copy the already built lng files
    glob_copy('apps/lang/*lng', "$rbdir/langs/");

    # copy the .lua files
    glob_mkdir("$rbdir/rocks/viewers/lua/");
    glob_copy('apps/plugins/lua/*.lua', "$rbdir/rocks/viewers/lua/");
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

    # in the app the the layout is different (no .rockbox, but bin/lib/share)
    $app = ($modelname eq "application");

    # build a full install .rockbox ($rbdir) directory
    buildzip($target, $fonts);

    unlink($output);

    if($fonts == 1) {
        # Don't include image file in fonts-only package
        undef $target;
    }
    if($target && ($target !~ /(mod|ajz|wma)\z/i)) {
        # On some targets, the image goes into .rockbox.
        copy("$target", "$rbdir/$target");
        undef $target;
    }

    if($install) {
        make_install(".rockbox", $install) or die "MKDIRFAILED\n";
    }
    else {
        unless (".rockbox" eq $rbdir) {
            move(".rockbox", $rbdir);
            print "mv .rockbox $rbdir\n" if $verbose;
        }
        system("$ziptool $output $rbdir $target >/dev/null");
        print "$ziptool $output $rbdir $target >/dev/null\n" if $verbose;
    }
    rmtree(".rockbox");
    print "rm .rockbox\n" if $verbose;
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
elsif(($exe =~ /rockboxui/)) {
    # simulator, exclude the exe file
    $exe = "";
}

runone($exe, $incfonts);


