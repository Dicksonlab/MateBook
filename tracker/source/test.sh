#!/bin/sh
set -u
set -e

map_path()
{
	paramIndex=$1

	PARAMARRAY[${paramIndex}]=$(sed \
		-e "s:^/Volumes/DIK.screen.scratch:/projects/DIK.screen/scratch:" \
		-e "s:^\\\\\\\\wright\\\\DIK.screen.scratch:/projects/DIK.screen/scratch:I" \
		-e "s:^//wright/DIK.screen.scratch:/projects/DIK.screen/scratch:I" \
		-e "s/\\\\/\\//g" \
	<<< ${PARAMARRAY[${paramIndex}]})

        PARAMARRAY[${paramIndex}]=$(sed \
		-e "s:^/Volumes/dikarchive:/projects/dikarchive:" \
		-e "s:^\\\\\\\\manray\\\\dikarchive:/projects/dikarchive:I" \
		-e "s:^//manray/dikarchive:/projects/dikarchive:I" \
		-e "s/\\\\/\\//g" \
	<<< ${PARAMARRAY[${paramIndex}]})
}

PATHS=(1 2 3);
ALLPARAMS="$@"
echo $ALLPARAMS
PARAMARRAY=("${@}")
for (( i = 0; i < ${#PATHS[@]}; i++ )); do
#	echo ${i}
#	echo ${PATHS[${i}]}
#	echo ${PARAMARRAY[${PATHS[${i}]}]}
	map_path ${PATHS[${i}]};
done

ALLPARAMSNEW="${PARAMARRAY[@]}"
echo $ALLPARAMSNEW
