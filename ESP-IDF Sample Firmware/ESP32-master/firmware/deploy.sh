#!/bin/bash
# args: APP_BIN, DEPLOY_LOCAL, DEPLOY_DIR, DEPLOY_VERSION, DEPLOY_BOARD_VERSION, DEPLOY_SERVER
#

echo -e "Trying deploy script..."
for arg in "$@"
do
	#echo ${arg}
	PARAM=`echo ${arg} | awk -F= '{print $1}'`
	VALUE=`echo ${arg} | awk -F= '{print $2}'`
	echo -e "export ${PARAM}=${VALUE}"
	export ${PARAM}=${VALUE}
done

DEPLOY_USER=ubuntu
SFTP_USER=integrator
DEPLOY_PORT=8822
DEPLOY_FIRMWARE_EXTENSION=bin
DEBUG_FIRMWARE_EXTENSION=elf
SIGN_FIRMWARE_EXTENSION=sig
APP_ELF=${APP_BIN/${DEPLOY_FIRMWARE_EXTENSION}/${DEBUG_FIRMWARE_EXTENSION}}
DEPLOY_ENV=$(<${DEPLOY_DIR}/../../deploy_env)
DEPLOY_FIRMWARE_CHECKSUM_EXTENSION=${DEPLOY_FIRMWARE_EXTENSION}.sha256
BOARD_DEPLOY=false
case $DEPLOY_BOARD_VERSION in
	rev1)
		export DEPLOY_FIRMWARE_PREFIX=a
		export BOARD_DEPLOY=true
		;;
	rev2)
		export DEPLOY_FIRMWARE_PREFIX=a2
		export BOARD_DEPLOY=true
		;;
        rev3)
		export DEPLOY_FIRMWARE_PREFIX=a3
		export BOARD_DEPLOY=true
		;;
	uart)
		export DEPLOY_FIRMWARE_PREFIX=u
		export BOARD_DEPLOY=true
		;;
	plug_hub)
		export DEPLOY_FIRMWARE_PREFIX=p
		export BOARD_DEPLOY=true
		;;
	plug_2hub)
		export DEPLOY_FIRMWARE_PREFIX=p2
		export BOARD_DEPLOY=true
		;;
	plug_3hub)
		export DEPLOY_FIRMWARE_PREFIX=p3
		export BOARD_DEPLOY=true
		;;
	water_valve)
		export DEPLOY_FIRMWARE_PREFIX=w
		export BOARD_DEPLOY=true
		;;
esac
DEPLOY_FIRMWARE_FILE=${DEPLOY_FIRMWARE_PREFIX}-${DEPLOY_BOARD_VERSION}-${DEPLOY_ENV}-${DEPLOY_VERSION}
DEPLOY_FIRMWARE_FILE_NAME=${DEPLOY_FIRMWARE_FILE}.${DEPLOY_FIRMWARE_EXTENSION}
DEBUG_FIRMWARE_FILE_NAME=${DEPLOY_FIRMWARE_FILE}.${DEBUG_FIRMWARE_EXTENSION}
DEPLOY_FIRMWARE_CHECKSUM_FILE_NAME=${DEPLOY_FIRMWARE_FILE}.${DEPLOY_FIRMWARE_CHECKSUM_EXTENSION}
SIGNED_FIRMWARE_FILE_NAME=${DEPLOY_FIRMWARE_FILE}.${SIGN_FIRMWARE_EXTENSION}
SIGN_DOMAIN="https://sf-2019.mios.com"
SIGN_PORT=9980
SIGN_KEY=$(</var/jenkins_home/ssh/sign_api_key)
OTA_SFTP_KEY_FILE=/var/jenkins_home/ssh/ota_sftp_key
OTA_SERVER="dl-fmt.mios.com"
OTA_PATH="upload/a32/"
DL_SERVER="dl.mios.com"

if [ "${DEPLOY_LOCAL}" == "N" ]; then

	mkdir -p ${DEPLOY_DIR}
	rm -f ${DEPLOY_DIR}/*
	cp ${APP_BIN} ${DEPLOY_DIR}/${DEPLOY_FIRMWARE_FILE_NAME}
	cp ${APP_ELF} ${DEPLOY_DIR}/${DEBUG_FIRMWARE_FILE_NAME}
	#sha256sum ${APP_BIN} | cut -c1-64 | xxd -r -p > ${DEPLOY_DIR}/${DEPLOY_FIRMWARE_CHECKSUM_FILE_NAME}
	#scp -P ${DEPLOY_PORT} ${DEPLOY_DIR}/${DEPLOY_FIRMWARE_FILE_NAME} ${DEPLOY_DIR}/${DEPLOY_FIRMWARE_CHECKSUM_FILE_NAME} ${DEPLOY_USER}@${DEPLOY_SERVER}:/var/www/updates/
	#echo Firmware ${DEPLOY_VERSION} has deployed on ${DEPLOY_SERVER}!
	#echo FileName ${DEPLOY_FIRMWARE_FILE_NAME} !
	if [ "${BOARD_DEPLOY}" == "true" ]; then
		if [ "${DEPLOY_ENV}" == "dev" ] || [ "${DEPLOY_ENV}" == "qa" ] || [ "${DEPLOY_ENV}" == "stg" ] || [ "${DEPLOY_ENV}" == "live" ]; then
			cd ${DEPLOY_DIR}
			curl -F "withlengthheader=1" -F "${SIGN_KEY}" -F "file=@${DEPLOY_FIRMWARE_FILE_NAME}" -P ${SIGN_PORT} ${SIGN_DOMAIN} --output ${SIGNED_FIRMWARE_FILE_NAME}
			printf "put ${DEPLOY_FIRMWARE_FILE_NAME}\nput ${SIGNED_FIRMWARE_FILE_NAME}\nput ${DEBUG_FIRMWARE_FILE_NAME}\nquit">sftp-commands.txt
			sftp -f -i ${OTA_SFTP_KEY_FILE} -P ${DEPLOY_PORT} -b sftp-commands.txt -v ${SFTP_USER}@${OTA_SERVER}:${OTA_PATH}${DEPLOY_ENV}
			if [[ $? != 0 ]]; then
				echo "Transfer failed!"
				exit 1
			else
				printf "Firmware ${DEPLOY_FIRMWARE_FILE_NAME} has been signed and deployed on ${DL_SERVER}/a32/${DEPLOY_ENV}/${DEPLOY_FIRMWARE_FILE_NAME}!">${DEPLOY_DIR}/../../${DEPLOY_FIRMWARE_PREFIX}_otamsg
				printf "${DEPLOY_ENV}-${DEPLOY_VERSION}">${DEPLOY_DIR}/../../buildmsg
				printf "http://${DL_SERVER}/a32/${DEPLOY_ENV}/${DEPLOY_FIRMWARE_FILE}">${DEPLOY_DIR}/../../dwnlink
				printf "${DEPLOY_VERSION}">${DEPLOY_DIR}/../../deployver
			fi
		fi
	fi
else

	mkdir -p ${DEPLOY_DIR}
	rm -f ${DEPLOY_DIR}/${DEPLOY_FIRMWARE_FILE_NAME}
	rm -f ${DEPLOY_DIR}/${DEPLOY_FIRMWARE_CHECKSUM_FILE_NAME}
	cp ${APP_BIN} ${DEPLOY_DIR}/${DEPLOY_FIRMWARE_FILE_NAME}
	sha256sum ${APP_BIN} | cut -c1-64 | xxd -r -p > ${DEPLOY_DIR}/${DEPLOY_FIRMWARE_CHECKSUM_FILE_NAME}
	IPADR=$(ifconfig | grep -v '0.0.0.0' | grep -Eo 'inet (addr:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*' | grep -v '127.0.0.1' | awk '{print $1}')
	echo Firmware ${DEPLOY_VERSION} has deployed in ${DEPLOY_DIR} folder!
	echo "For uploading firmware you can send command to your Hub:"
	echo "{"
	echo "    \"method\": \"updateEzloFirmware\","
	echo "    \"id\": \"$RANDOM\","
	echo "    \"params\": {"
	echo "        \"version\": \"${DEPLOY_VERSION}\","
	echo "        \"source\": \"${IPADR}\""
	echo "    }"
	echo "}"
fi

