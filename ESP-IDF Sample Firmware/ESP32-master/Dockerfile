FROM ubuntu:20.10

ENV DEBIAN_FRONTEND noninteractive

RUN dpkg --add-architecture i386 && dpkg --add-architecture amd64 && \
    apt-get update && apt-get upgrade -y && \
    apt-get install -y mc nano git wget flex bison gperf curl \
                       python3 python3-pip python3-setuptools \
                       cmake ninja-build ccache libffi-dev libssl-dev dfu-util pigz\
                       gcc-multilib libreadline-dev libreadline-dev:i386 && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3 10 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

ENV IDF_PATH "/opt/esp-idf"
RUN git clone https://github.com/espressif/esp-idf.git -b release/v4.2 --recursive ${IDF_PATH}

RUN python -m  pip install --upgrade pip virtualenv
ENV IDF_TOOLS_PATH "/opt/espressif"
RUN ${IDF_PATH}/tools/idf_tools.py install
RUN pip install -r $IDF_PATH/requirements.txt 
COPY ./firmware/requirements.txt /requirements.txt
RUN pip install -r /requirements.txt

ENV PATH $IDF_PATH/components/esptool_py/esptool:\
$IDF_PATH/components/espcoredump:\
$IDF_PATH/components/partition_table:\
$IDF_PATH/components/app_update:\
$IDF_TOOLS_PATH/tools/xtensa-esp32-elf/esp-2020r3-8.4.0/xtensa-esp32-elf/bin:\
$IDF_TOOLS_PATH/tools/xtensa-esp32s2-elf/esp-2020r3-8.4.0/xtensa-esp32s2-elf/bin:\
$IDF_TOOLS_PATH/tools/esp32ulp-elf/2.28.51-esp-20191205/esp32ulp-elf-binutils/bin:\
$IDF_TOOLS_PATH/tools/esp32s2ulp-elf/2.28.51-esp-20191205/esp32s2ulp-elf-binutils/bin:\
$IDF_TOOLS_PATH/tools/openocd-esp32/v0.10.0-esp32-20200709/openocd-esp32/bin:\
$IDF_PATH//tools:\
$PATH

CMD ["/bin/bash"]
