#!/bin/bash

PATCHES_PREFIX=patches/

ORIGINAL=${1:-ffmpeg-orig}
MODIFIED=${2:-ffmpeg}
PATCH=${3:-new.patch}

mkdir -p $PATCHES_PREFIX
echo "Generating $PATCH from $ORIGINAL vs $MODIFIED..."
diff -ruN $ORIGINAL $MODIFIED >$PATCHES_PREFIX$PATCH || exit
echo "Done!"
