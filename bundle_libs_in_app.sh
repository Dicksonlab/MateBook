#!/bin/bash

# $1 = .app/

#set -e

mkdir -p $1/Contents/Frameworks
libs=($(otool -L $1/Contents/MacOS/* | awk '{print $1}' | grep -E '^(/Users|[^/])' | grep -v $1))
execs=($(otool -L $1/Contents/MacOS/* | grep -v 'not an object file' | grep $1))
for i in ${libs[*]} ; do
	cp -RL ${LIB_DIR}/$(basename $i) $1/Contents/Frameworks
  chmod -R u+w $1/Contents/Frameworks/$(basename $i)

  install_name_tool -id @executable_path/../Frameworks/$(basename $i) \
      $1/Contents/Frameworks/$(basename $i)
  for j in ${execs[*]} ; do
    install_name_tool -change $i @executable_path/../Frameworks/$(basename $i) ${j%?}
  done
done

flag=true
while $flag ; do
  flag=false
  for i in ${libs[*]}; do
    libs2=($(otool -L $1/Contents/Frameworks/$(basename $i) | awk '{print $1}' | \
        grep -E '^(/Users|[^/])' | grep -v $1 | grep -v executable_path))
    for j in ${libs2[*]}; do
      if [[ ! -f $1/Contents/Frameworks/$(basename $j) ]] ; then
        flag=true
        cp -RL ${LIB_DIR}/$(basename $j) $1/Contents/Frameworks
        chmod -R u+w $1/Contents/Frameworks/$(basename $j)
      fi
      install_name_tool -change $j @executable_path/../Frameworks/$(basename $j) \
          $1/Contents/Frameworks/$(basename $i)
    done
  done
done
