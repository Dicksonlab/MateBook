#!/bin/bash
set -u
set -e

MB_VERSION="$2"        ; echo "$0 got MB_VERSION $MB_VERSION"
MB_VIDEO="$4"          ; echo "$0 got MB_VIDEO $MB_VIDEO"
MB_OUTDIR="$6"         ; echo "$0 got MB_OUTDIR $MB_OUTDIR"
MB_SETTINGS="$8"       ; echo "$0 got MB_SETTINGS $MB_SETTINGS"
MB_PREPROCESS="${10}"  ; echo "$0 got MB_PREPROCESS $MB_PREPROCESS"
MB_TRACK="${12}"       ; echo "$0 got MB_TRACK $MB_TRACK"
MB_POSTPROCESS="${14}" ; echo "$0 got MB_POSTPROCESS $MB_POSTPROCESS"
MB_BEGIN="${16}"       ; echo "$0 got MB_BEGIN $MB_BEGIN"
MB_END="${18}"         ; echo "$0 got MB_END $MB_END"

#source /sw/lenny/etc/sge-aragon.bash

#MB_PATH=/projects/DIK.screen/tracker
MB_PATH=/groups/dickson/dicksonlab/MateBook/MateBook

umask 007

if [ -x ${MB_PATH}/usr/bin/tracker/$MB_VERSION/tracker ]; then
	echo "found tracker for version $MB_VERSION"
# translate the video path
  for i in MB_VIDEO MB_OUTDIR MB_SETTINGS
  do
    eval $i=$(sed \
        -e "s:^/Volumes:/groups/dickson:" \
    <<< \"${!i}\")
#    eval $i=$(sed \
#        -e "s:^/Volumes/dikarchive:/projects/dikarchive:I" \
#        -e "s:^\\\\\\\\manray\\\\dikarchive:/projects/dikarchive:I" \
#        -e "s:^//manray/dikarchive:/projects/dikarchive:I" \
#        -e "s/\\\\/\\//g" \
#    <<< ${!i})
#    eval $i=$(sed \
#        -e "s:^/Volumes/gencodys:/projects/gencodys:I" \
#        -e "s:^\\\\\\\\manray\\\\gencodys:/projects/gencodys:I" \
#        -e "s:^//manray/gencodys:/projects/gencodys:I" \
#        -e "s/\\\\/\\//g" \
#    <<< ${!i})
#    eval $i=$(sed \
#        -e "s:^/Volumes/DIK.screen.scratch:/projects/DIK.screen/scratch:I" \
#        -e "s:^\\\\\\\\wright\\\\DIK.screen.scratch:/projects/DIK.screen/scratch:I" \
#        -e "s:^//wright/DIK.screen.scratch:/projects/DIK.screen/scratch:I" \
#        -e "s/\\\\/\\//g" \
#    <<< ${!i})
#    eval $i=$(sed \
#        -e "s:^/Volumes/groups:/groups:I" \
#        -e "s:^\\\\\\\\storage.imp.ac.at\\\\groups:/groups:I" \
#        -e "s:^//storage.imp.ac.at/groups:/groups:I" \
#        -e "s/\\\\/\\//g" \
#    <<< ${!i})
  done
	cd "$MB_OUTDIR"
	echo "  submitting $MB_VIDEO"
	MB_USER="$LOGNAME" \
	MB_VERSION="$MB_VERSION" \
	MB_VIDEO="$MB_VIDEO" \
	MB_OUTDIR="$MB_OUTDIR" \
	MB_SETTINGS="$MB_SETTINGS" \
	MB_PREPROCESS="$MB_PREPROCESS" \
	MB_TRACK="$MB_TRACK"\
	MB_POSTPROCESS="$MB_POSTPROCESS" \
	MB_BEGIN="$MB_BEGIN" \
	MB_END="$MB_END" \
	MB_PATH="$MB_PATH" \
	qsub -N MateBook -cwd -j y -o /dev/null -b y -o output.log -v MB_USER,MB_VERSION,MB_VIDEO,MB_OUTDIR,MB_SETTINGS,MB_PREPROCESS,MB_TRACK,MB_POSTPROCESS,MB_BEGIN,MB_END,MB_PATH ${MB_PATH}/usr/bin/tracker/$MB_VERSION/track.sh
fi
