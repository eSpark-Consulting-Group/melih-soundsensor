#!/bin/bash
# args: LOCAL_WEB_SERVER_DIR
#

echo -e "Trying local_updater script..."
for arg in "$@"
do
	#echo ${arg}
	PARAM=`echo ${arg} | awk -F= '{print $1}'`
	VALUE=`echo ${arg} | awk -F= '{print $2}'`
	echo -e "export ${PARAM}=${VALUE}"
	export ${PARAM}=${VALUE}
done

DEPLOY_LOCAL_PORT=80
DEPLOY_LOCAL_WEB_SERVER_EXTENSION=http.server
IPADR=$(ifconfig | grep -v '0.0.0.0' | grep -Eo 'inet (addr:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*' | grep -v '127.0.0.1' | awk '{print $1}')

echo "IP address: ${IPADR}" 

echo "Run web server for ${LOCAL_WEB_SERVER_DIR} folder on ${DEPLOY_LOCAL_PORT} port."
if (( $DEPLOY_LOCAL_PORT > 1024 )); then

	pkill -f ${DEPLOY_LOCAL_WEB_SERVER_EXTENSION}
	cd ${LOCAL_WEB_SERVER_DIR} && python3 -m ${DEPLOY_LOCAL_WEB_SERVER_EXTENSION} ${DEPLOY_LOCAL_PORT} > /dev/null &

else

	echo "For running web server on ${DEPLOY_LOCAL_PORT} port need to root privileges!"
	sudo pkill -f ${DEPLOY_LOCAL_WEB_SERVER_EXTENSION}
	cd ${LOCAL_WEB_SERVER_DIR} && sudo python3 -m ${DEPLOY_LOCAL_WEB_SERVER_EXTENSION} ${DEPLOY_LOCAL_PORT} > /dev/null &

fi
