#!/bin/bash
# args: TOKEN, DOWNLOAD_LINK, DEPLOY_VERSION
#

echo -e "Triggering k8s tests..."
for arg in "$@"
do
	#echo ${arg}
	PARAM=`echo ${arg} | awk -F= '{print $1}'`
	VALUE=`echo ${arg} | awk -F= '{print $2}'`
	export ${PARAM}=${VALUE}
done

LINK="https://jenkins.mios.com/buildByToken/buildWithParameters?"
LINK+="job=qa-ota-update&"
LINK+="token=${TOKEN}&"
LINK+="UUID=36804aa0-92a4-11e9-91cd-273af3fa6fc6&"
LINK+="SIGNATURE=1000&"
LINK+="TARGET_URLS=\{\"firmware\":\"${DOWNLOAD_LINK}.bin\",\"checksum\":\"${DOWNLOAD_LINK}.sig\"\}&"
LINK+="TARGET_VERSION=${DEPLOY_VERSION}&"
LINK+="ENVIRONMENT=qa&"
LINK+="CHECK_INFO=false"
curl ${LINK}
echo "executed curl '${LINK}'"