#!/bin/bash

PATCHES_PREFIX=patches/

ORIGINAL=${1:-ffmpeg}

for PATCH in $PATCHES_PREFIX*.patch; do
	echo "Applying $PATCH at $DIR..."
	patch -u -p1 -d$ORIGINAL < $PATCH || exit
done
echo "Done!"
