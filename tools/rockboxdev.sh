#!/bin/sh

# Abort execution as soon as an error is encountered
# That way the script do not let the user think the process completed correctly
# and leave the opportunity to fix the problem and restart compilation where
# it stopped
set -e

# this is where this script will store downloaded files and check for already
# downloaded files
dlwhere="${RBDEV_DOWNLOAD:-/tmp/rbdev-dl}"

# will append the target string to the prefix dir mentioned here
# Note that the user running this script must be able to do make install in
# this given prefix directory. Also make sure that this given root dir
# exists.
prefix="${RBDEV_PREFIX:-/usr/local}"

# This directory is used to extract all files and to build everything in. It
# must not exist before this script is invoked (as a security measure).
builddir="${RBDEV_BUILD:-/tmp/rbdev-build}"

# This script needs to use GNU Make. On Linux systems, GNU Make is invoked
# by running the "make" command, on most BSD systems, GNU Make is invoked
# by running the "gmake" command. Set the "make" variable accordingly.
if [ -f "`which gmake 2>/dev/null`" ]; then
    make="gmake"
else
    make="make"
fi

if [ -z $GNU_MIRROR ] ; then
    GNU_MIRROR=http://mirrors.kernel.org/gnu
fi

# These are the tools this script requires and depends upon.
reqtools="gcc bzip2 make patch makeinfo"

##############################################################################
# Functions:

findtool(){
  file="$1"

  IFS=":"
  for path in $PATH
  do
    # echo "Checks for $file in $path" >&2
    if test -f "$path/$file"; then
      echo "$path/$file"
      return
    fi
  done
}

input() {
    read response
    echo $response
}

#$1 file
#$2 URL"root
getfile() {
  tool=`findtool curl`
  if test -z "$tool"; then
    tool=`findtool wget`
    if test -n "$tool"; then
      # wget download
      echo "ROCKBOXDEV: Downloading $2/$1 using wget"
      $tool -O $dlwhere/$1 $2/$1
    fi
  else
     # curl download
      echo "ROCKBOXDEV: Downloading $2/$1 using curl"
     $tool -Lo $dlwhere/$1 $2/$1
  fi

  if [ $? -ne 0 ] ; then
      echo "ROCKBOXDEV: couldn't download the file!"
      echo "ROCKBOXDEV: check your internet connection"
      exit
  fi

  if test -z "$tool"; then 
    echo "ROCKBOXDEV: No downloader tool found!"
    echo "ROCKBOXDEV: Please install curl or wget and re-run the script"
    exit
  fi
}


build() {
    toolname="$1"
    target="$2"
    version="$3"
    patch="$4"
    configure_params="$5"
    needs_gmp="$6"

    patch_url="http://www.rockbox.org/gcc"

    case $toolname in
        gcc)
            file="gcc-core-$version.tar.bz2"
            url="$GNU_MIRROR/gcc/gcc-$version"
            ;;

        binutils)
            file="binutils-$version.tar.bz2"
            url="$GNU_MIRROR/binutils"
            ;;

        *)
            echo "ROCKBOXDEV: Bad toolname $toolname"
            exit
            ;;
    esac
    
    # create build directory
    if test -d $builddir; then
        if test ! -w $builddir; then
            echo "ROCKBOXDEV: No write permission for $builddir"
            exit
        fi
    else
        mkdir -p $builddir
    fi

    # download source tarball
    if test ! -f "$dlwhere/$file"; then
        getfile "$file" "$url"
    fi

    # download patch
    if test -n "$patch"; then
        if test ! -f "$dlwhere/$patch"; then
            getfile "$patch" "$patch_url"
        fi
    fi

    cd $builddir

    echo "ROCKBOXDEV: extracting $file"
    tar xjf $dlwhere/$file

    # do we have a patch?
    if test -n "$patch"; then
        echo "ROCKBOXDEV: applying patch $patch"

        # apply the patch
        (cd $builddir/$toolname-$version && patch -p1 < "$dlwhere/$patch")

        # check if the patch applied cleanly
        if [ $? -gt 0 ]; then
            echo "ROCKBOXDEV: failed to apply patch $patch"
            exit
        fi
    fi

    # kludge to avoid having to install GMP and MPFR for new gcc
    if test -n "$needs_gmp"; then
        if test ! -d gmp; then
            echo "ROCKBOXDEV: Getting GMP"
            getfile "gmp-5.0.1.tar.bz2" "$GNU_MIRROR/gmp"
            tar xjf $dlwhere/gmp-5.0.1.tar.bz2
            ln -s gmp-5.0.1.tar.bz2 gmp
        fi

        if test ! -d mpfr; then
            echo "ROCKBOXDEV: Getting MPFR"
            getfile "mpfr-2.4.2.tar.bz2" "$GNU_MIRROR/mpfr"
            tar xjf $dlwhere/mpfr-2.4.2.tar.bz2
            ln -s mpfr-2.4.2 mpfr
        fi
    fi

    echo "ROCKBOXDEV: mkdir build-$toolname"
    mkdir build-$toolname

    echo "ROCKBOXDEV: cd build-$toolname"
    cd build-$toolname

    echo "ROCKBOXDEV: $toolname/configure"
    ../$toolname-$version/configure --target=$target --prefix=$prefix --enable-languages=c --disable-libssp --disable-docs $configure_params

    echo "ROCKBOXDEV: $toolname/make"
    $make -j8

    echo "ROCKBOXDEV: $toolname/make install"
    $make install

    rm -rf $builddir
}

##############################################################################
# Code:

# Verify required tools
for t in $reqtools; do
    tool=`findtool $t`
    if test -z "$tool"; then
        echo "ROCKBOXDEV: \"$t\" is required for this script to work."
        echo "ROCKBOXDEV: Please install \"$t\" and re-run the script."
        exit
    fi
done

echo "Download directory : $dlwhere (set RBDEV_DOWNLOAD to change)"
echo "Install prefix     : $prefix  (set RBDEV_PREFIX to change)"
echo "Build dir          : $builddir (set RBDEV_BUILD to change)"
echo ""

# Verify download directory
if test -d "$dlwhere"; then
  if ! test -w "$dlwhere"; then
    echo "ROCKBOXDEV: No write permission for $dlwhere"
    exit
  fi
else
  mkdir $dlwhere
  if test $? -ne 0; then
    echo "ROCKBOXDEV: Failed creating directory $dlwhere"
    exit
  fi
fi

# Verify the prefix dir
if test ! -d $prefix; then
  mkdir -p $prefix
  if test $? -ne 0; then
      echo "ROCKBOXDEV: Failed creating directory $prefix"
      exit
  fi
fi
if test ! -w $prefix; then
  echo "ROCKBOXDEV: No write permission for $prefix"
  exit
fi

echo "Select target arch:"
echo "s   - sh       (Archos models)"
echo "m   - m68k     (iriver h1x0/h3x0, ifp7x0 and iaudio)"
echo "a   - arm      (ipods, iriver H10, Sansa, etc)"
echo "e   - arm-eabi (same as above, new gcc toolchain)"
echo "i   - mips     (Jz4740 and ATJ-based players)"
echo "separate multiple targets with spaces"
echo "(Example: \"s m a\" will build sh, m86k and arm)"
echo ""

selarch=`input`
system=`uname -s`

# add target dir to path to ensure the new binutils are used in gcc build
PATH="$prefix/bin:${PATH}"

for arch in $selarch
do
    echo ""
    case $arch in
        [Ss])
            build "binutils" "sh-elf" "2.16.1"
            build "gcc" "sh-elf" "4.0.3" "gcc-4.0.3-rockbox-1.diff"
            ;;

        [Ii])
            build "binutils" "mipsel-elf" "2.17" "" "--disable-werror"
            patch=""
            if [ "$system" = "Interix" ]; then
                patch="gcc-4.1.2-interix.diff"
            fi
            build "gcc" "mipsel-elf" "4.1.2" "$patch"
            ;;

        [Mm])
            build "binutils" "m68k-elf" "2.16.1"
            patch=""
            case $system in
                CYGWIN* | Darwin | FreeBSD | Interix | SunOS)
                    patch="gcc-3.4.6.patch"
                    ;;
                Linux)
                    machine=`uname -m`
                    if [ "$machine" = "x86_64" ]; then
                        patch="gcc-3.4.6-amd64.patch"
                    fi
                    ;;
            esac
            build "gcc" "m68k-elf" "3.4.6" "$patch"
            ;;

        [Aa])
            build "binutils" "arm-elf" "2.16.1"
            build "gcc" "arm-elf" "4.0.3" "rockbox-multilibs-arm-elf-gcc-4.0.3_3.diff"
            ;;

        [Ee])
            build "binutils" "arm-elf-eabi" "2.20.1"
            build "gcc" "arm-elf-eabi" "4.4.4" "rockbox-multilibs-noexceptions-arm-elf-eabi-gcc-4.4.2_1.diff" "" "needs_gmp"
            ;;

        *)
            echo "ROCKBOXDEV: Unsupported architecture option: $arch"
            exit
            ;;
    esac
done

echo ""
echo "ROCKBOXDEV: Done!"
echo ""
echo "ROCKBOXDEV: Make sure your PATH includes $prefix/bin"
echo ""
