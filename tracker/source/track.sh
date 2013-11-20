#$ -S /bin/sh
#$ -q dickson.q
#$ -q nonofficehours.q
#$ -P MateBook
#$ -cwd
set -u
set -e

umask 007

hostname
date
pwd
echo "$0 got MB_VERSION $MB_VERSION"
echo "$0 got MB_VIDEO $MB_VIDEO"
echo "$0 got MB_OUTDIR $MB_OUTDIR"
echo "$0 got MB_SETTINGS $MB_SETTINGS"
echo "$0 got MB_PREPROCESS $MB_PREPROCESS"
echo "$0 got MB_TRACK $MB_TRACK"
echo "$0 got MB_POSTPROCESS $MB_POSTPROCESS"
echo "$0 got MB_BEGIN $MB_BEGIN"
echo "$0 got MB_END $MB_END"
echo "$0 got MB_PATH $MB_PATH"

cd "$MB_OUTDIR"
md5sum "$MB_VIDEO" >video.md5
LD_LIBRARY_PATH=${MB_PATH}/usr/lib ${MB_PATH}/usr/bin/tracker/$MB_VERSION/tracker --in "$MB_VIDEO" --out . --settings "$MB_SETTINGS" --preprocess "$MB_PREPROCESS" --track "$MB_TRACK" --postprocess "$MB_POSTPROCESS" --begin "$MB_BEGIN" --end "$MB_END"
