#!/bin/bash

#################################
#################################
#####                       #####
#####  Escape Build Script  #####
#####                       #####
#################################
#################################

function usage {
    echo "Usage: build.sh <target> <platform>"
    echo "    Targets: release debug"
    echo "    Platforms: win32 win64"
}

TARGET=$1
PLATFORM=$2

if [[ -z "$PLATFORM" ]]
then
    PLATFORM="win64"
fi

if [[ -z "$TARGET" ]]
then
    TARGET="release"
fi

###########################################################
### Shared Flags - Applies to all targets and platforms ###
###########################################################

S_CXXFLAGS="-std=c++1y -Wall -DGLM_FORCE_RADIANS -DGLEW_STATIC"
S_LDFLAGS="-static"
S_LDLIBS="-lglfw3 -lglew32 -lyaml-cpp -lpng -lz -lopengl32 -lgdi32"

####################################################
### Target Flags - Flags specific to each target ###
####################################################

T_CXXFLAGS=""
T_LDFLAGS=""
T_LDLIBS=""

case $TARGET in
    release)
        T_CXXFLAGS="-Ofast"
        ;;
    debug)
        T_CXXFLAGS="-Og -g"
        ;;
    profile)
        T_CXXFLAGS="-O2 -pg"
        T_LDFLAGS="-pg"
        ;;
    *)
        echo "Invalid target."
        usage
        exit 1
        ;;
esac

########################################################
### Platform Flags - Flags specific to each platform ###
########################################################

P_CXXFLAGS=""
P_LDFLAGS=""
P_LDLIBS=""

case $PLATFORM in
    win32)
        P_CXXFLAGS="-m32"
        P_LDFLAGS="-m32"
        ;;
    win64)
        ;;
    *)
        echo "Invalid platform."
        usage
        exit 2
        ;;
esac

####################################
### Export variables for Respite ###
####################################

export CXX="g++"
export CXXFLAGS="$CXXFLAGS $S_CXXFLAGS $T_CXXFLAGS $P_CXXFLAGS"
export LDFLAGS="$LDFLAGS $S_LDFLAGS $T_LDFLAGS $P_LDFLAGS"
export LDLIBS="$LDLIBS $S_LDLIBS $T_LDLIBS $P_LDLIBS"

############################################################
### Respite Cache - Used to create multiple build caches ###
############################################################

RESPITE_CACHE=".respite-$TARGET-$PLATFORM"

mkdir -p $RESPITE_CACHE
cmd //c "mklink /J .respite $RESPITE_CACHE"

######################################
### Check for existing executables ###
######################################

EXE="escape-$TARGET-$PLATFORM.exe"

if [[ -f $EXE ]]
then
    mv $EXE a.respite
fi

###################################
### Respite - Build the project ###
###################################

if respite 2> error.log
then
    mv a.respite $EXE
else
    cat error.log
    echo "BUILD FAILED"
fi

cmd //c "rd .respite"
