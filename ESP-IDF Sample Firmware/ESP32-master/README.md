# Atom32 firmware

## Prepare local build environment for Ubuntu 20.10 64b:

Install the required packages:
* sudo dpkg --add-architecture i386
* sudo apt-get update && apt-get upgrade -y
* sudo apt-get install -y git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util pigz gcc-multilib libreadline-dev libreadline-dev:i386
* sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 10
* python -m  pip install --upgrade pip virtualenv
* git clone git@github.com:ezloteam/ESP32.git --recursive
* ESP32/lib/esp-idf/install.sh
* . ESP32/lib/esp-idf/export.sh (don't forget '.' before command line!)
* pip install -r firmware/requirements.txt

## Build localy:

* git clone git@github.com:ezloteam/ESP32.git --recursive
* . ESP32/lib/esp-idf/export.sh (don't forget '.' before command line!)
* cd ESP32/firmware
* idf.py -G "Unix Makefiles" -DCMAKE_CONFIG_BOARD_TYPE=<board_type> set-target esp32 (where <board_type> is rev1, rev2, plug, plug2, plug3, valve etc)
* idf.py build

## Build in docker:

Before next steps call commands "docker --version" or "which docker" to verify that you already have docker. If you haven't docker you shoul'd install it respectively. 

* git clone git@github.com:ezloteam/ESP32.git --recursive
* cd ESP32

To open bash console call: "docker container run -i -t  -v $PWD:/ESP32 -w /ESP32/firmware ezlo/esp32idf:4.2.1". You should use inside container Midnight Commander file manager and nano editor to work with code in place.

To configure and build firmware call for example: "docker container run -i -t -v $PWD:/ESP32 -w /ESP32/firmware ezlo/esp32idf:4.2.1 idf.py -G "Unix Makefiles" -DCMAKE_CONFIG_BOARD_TYPE=plug2 set-target esp32 build"

## Prepare new docker image:

Build new image locally with X.X tag:
* docker image build --tag ezlo/esp32idf:X.X .

Upload new image to Docker Hub:
* docker login
* docker push ezlo/esp32idf:X.X

## Code Style:
https://ezloproject.atlassian.net/wiki/spaces/PM/pages/217940280/Code+style+recommendations


## How to create new qa(stage) build
* git checkout master
* git pull
* git submodule update
* git checkout qa
* git pull
* git checkout -b masteXXXX_to_qa (qaXXXX_to_stage)    #XXXX - last dev(qa) build number, for example dl.mios.com/a32/dev/w-water_valve-dev-0.8.1494.bin! (master1494_to_qa)
* git merge master
* git merge master
* Fix all merge conflicts
* git push --set-upstream origin masterXXXX_to_qa  (qaXXXX_to_stage)
* go to github and create pull request. !!! Select qa(stage) on base branch !!!
* Merge pull request

## How to restore chip
* Read provision info from chip
esptool.py --chip esp32 -b 460800 read_flash 0xFF0000 0x10000 id_n.bin

* Erase chip
esptool.py --chip esp32 -b 460800 erase_flash

* Write new firmware to chip
esptool.py -p /dev/ttyUSB0 -b 460800 --before default_reset --after hard_reset --chip esp32 write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB 0x8000 partition_table/partition-table.bin 0xd000 ota_data_initial.bin 0x1000 bootloader/bootloader.bin 0x10000 atom32.bin

* Write provision info tp chip
esptool.py --chip esp32 -b 460800 write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0xFF0000 id_n.bin

