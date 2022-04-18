#!/usr/bin/env python3
import argparse, glob, re, subprocess, datetime, time, os
from shutil import which
from os import path
from enum import Enum

versionHeaderFile = "{out_dir}/include/FirmwareVersion.h"
versionFile = "{out_dir}/firmware_version"
gitIsExist = which("git") is not None

versionFileTemplate = '''#pragma once

#define MODEL "ATOM32"

#define FIRMWARE_VER_MAJOR {fw_major}
#define FIRMWARE_VER_MINOR {fw_minor}
#define FIRMWARE_VER_PATCH {fw_patch}
#define FIRMWARE_VERSION "{fw_major}.{fw_minor}.{fw_patch}"
#define KERNEL_VERSION "{kernel}"
#define HARDWARE_ARCH "esp32"
#define HARDWARE_VERSION "{board}"
#define FIRMWARE_PREFIX "{fw_prefix}"

#define BUILD_TIME "{build_time}"
#define BUILD_BUILDER "{build_builder}"
#define BUILD_BRANCH "{build_branch}"
#define BUILD_COMMIT "{build_commit}"
'''

def executeWhoami():
    return subprocess.check_output('echo "$(whoami)@$(hostname)"', shell=True).decode('utf-8').rstrip('\n')

def executeGit(params, default="unknown", execDir=None):
    if gitIsExist:
        return subprocess.check_output("git {0}".format(params), shell=True, cwd=execDir).decode('utf-8').rstrip('\n')
    return default

def writeVersionInfo(major, minor, build, board, wifiSsid, wifiPsw, initBranch, outDir, localtime=time.localtime(), prefix =''):
    revision = executeGit('rev-parse HEAD')
    branch = executeGit('rev-parse --abbrev-ref HEAD')
    firmware = executeGit('describe --first-parent --always')
    # Try parse version from git describe
    if major == 0 and minor == 0 and build == 0:
        a = firmware.split('-')
        if len(a) > 3 and a[2].isdigit():
            build = a[2]
        elif len(a) > 1:
            if a[1].isdigit():
                build = a[1]
        b = a[0].split('.')
        if b[0].isdigit():
            major = b[0]
        if len(b) > 1 and b[1].isdigit():
            minor = b[1]
#   ECMT-115 -- Force jump in ESP32 stg builds by 310 ( build 148 to become 458 for stage branch) -- extra parameter needed since Jenkins runs in detached HEAD
    if initBranch == 'stage':
        build = str(int(build) + 310)
    kernel = executeGit('describe --first-parent --always', execDir=os.path.dirname(os.path.realpath(__file__)) + "/../lib/esp-idf")
    builder = executeWhoami()
    buildTime = time.strftime("%Y-%m-%dT%H:%M:%S%z", localtime)
    fullVersionHeaderFilePath = versionHeaderFile.format(out_dir=outDir)

    if not os.path.exists(outDir+"/include"):
        os.makedirs(outDir+"/include")

    versionFileHandle = open(fullVersionHeaderFilePath, "w")
    versionFileHandle.writelines(versionFileTemplate.format(fw_major=major, fw_minor=minor, fw_patch=build, kernel=kernel, board=board, fw_prefix=prefix, build_time=buildTime, build_builder=builder, build_branch=branch, build_commit=revision))
    if wifiSsid is not None:
        versionFileHandle.writelines( "#define DEFAULT_WIFI_SSID \"{ssid}\"\n".format(ssid=wifiSsid) );
        if wifiPsw is None:
            versionFileHandle.writelines( "#define DEFAULT_WIFI_PASSWORD \"\"\n" );
        else:
            versionFileHandle.writelines( "#define DEFAULT_WIFI_PASSWORD \"{password}\"\n".format(password=wifiPsw) );
    versionFileHandle.close()

    fullVersionFilePath = versionFile.format(out_dir=outDir)
    versionFileHandle = open(fullVersionFilePath, "w")
    versionFileHandle.write("{0}.{1}.{2}".format(major, minor, build))
    versionFileHandle.close()

    return major, minor, build
