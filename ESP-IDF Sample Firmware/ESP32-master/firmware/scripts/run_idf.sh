#!/bin/bash
# args: ROOT_DIR, IDF_DIR, MONTTY, CMD
#

for arg in "$@"
do
	#echo ${arg}
	PARAM=`echo ${arg} | awk -F= '{print $1}'`
	VALUE=`echo ${arg} | awk -F= '{print $2}'`
	echo -e "${PARAM}=${VALUE}"
	export ${PARAM}=${VALUE}
done


#Save current directory
cwd=$(pwd)

#Monitor must be started from the directory with root CMakeLists.txt file
cd ${ROOT_DIR}
${IDF_DIR}/tools/idf.py -p ${MONTTY} ${CMD}

#Return to scripts directory
cd "${cwd}"
