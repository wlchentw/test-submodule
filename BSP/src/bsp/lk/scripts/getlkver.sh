#!/bin/sh


work_dir="$(pwd)"
echo "$0 start workdir=\"${work_dir}\" ---" 1>&2

local_rev_state=""

build_date_time="$(date +%m%d%H%M%S)"

if [ ! -e ".git" ];then
	echo "no git repository !! please check !" 1>&2 
	local_rev_state="?"
elif [ "$(git status -s |grep ".[MDTAC][[:blank:]]")" ];then
	local_rev_state="M"
else
	local_rev_state="B"
fi
lk_rev_string="$(git log -1 --pretty=format:%h-%cI)"
#pwd
cd "../dramk_8512" 
if [ $? = 0 ];then
	dram_rev_string="$(git log -1 --pretty=format:%h-%cI)"
	dram_rev="$(echo "${dram_rev_string}"|awk -F- '{print $1}')"
	if [ ! -e ".git" ];then
		echo "no dram git repository !! please check !" 1>&2 
		local_rev_state="?"
	elif [ "$(git status -s |grep ".[MDTAC][[:blank:]]")" ];then
		local_rev_state="M"
	fi
	cd "${work_dir}"
fi

rev_string="${lk_rev_string}"
rev_and_date="$(echo "${rev_string}"|sed -e "s/T.*$//")"
rev_times="$(echo "${rev_string}"|sed -e "s/^.*T//"|sed -e s/+.*$//)"
rev="$(echo "${rev_and_date}"|awk -F- '{print $1}')"
rev_year="$(echo "${rev_and_date}"|awk -F- '{print $2}')"
rev_month="$(echo "${rev_and_date}"|awk -F- '{print $3}')"
rev_day="$(echo "${rev_and_date}"|awk -F- '{print $4}')"
rev_hour="$(echo "${rev_times}"|awk -F: '{print $1}')"
rev_min="$(echo "${rev_times}"|awk -F: '{print $2}')"
rev_sec="$(echo "${rev_times}"|awk -F: '{print $3}')"
ver="\"${rev}_${dram_rev}-${rev_year}${rev_month}${rev_day}T${rev_hour}${rev_min}${rev_sec}-${local_rev_state}${build_date_time}\""
echo "ver=\"${ver}\"" 1>&2
echo "#define LK_VERSION $ver" > "include/version.h"

