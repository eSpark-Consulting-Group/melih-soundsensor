#!/usr/bin/env python3

import argparse, datetime, time, os, subprocess, pathlib
from enum import Enum
from shutil import copyfile, rmtree
from version import writeVersionInfo

configFileTemplate = '''
set(MAJOR {ver_major})
set(MINOR {ver_minor})
set(BUILD {ver_build})

set(BOARD {target_board})
'''

finishMessageTemplate = "\tBoard: {board}({config_path}), stored at {out_dir}"

def touch(fname):
	if os.path.exists(fname):
		os.utime(fname, None)
	else:
		open(fname, 'a').close()

class Board(Enum):
	devboard = 'devboard'
	rev1 = 'rev1'
	rev2 = 'rev2'
	rev3 = 'rev3'
	uart = 'uart'
	oldrev = 'old_rev'
	valve = 'valve'
	plug = 'plug'
	plug2 = 'plug2'
	plug3 = 'plug3'

	def __str__(self):
		return self.value

	def getSdkConfigRelPath(self):
		enumToPaths = {
			Board.devboard: 'configs/devboard',
			Board.rev1: 'configs/rev_1',
			Board.rev2: 'configs/rev_2',
            Board.rev3: 'configs/rev_3',
			Board.oldrev: 'configs/old_rev',
                        Board.uart: 'configs/uart',
			Board.valve: 'configs/valve',
			Board.plug: 'configs/plug',
			Board.plug2: 'configs/plug_v2',
			Board.plug3: 'configs/plug_v3',
		}
		return enumToPaths.get(self)

	def getSdkConfigFullPath(self, projectDir):
		return projectDir + '/' + self.getSdkConfigRelPath()

	def getPrefix(self):
		enumToPrefix = {
			Board.rev1: 'a-rev1',
			Board.rev2: 'a2-rev2',
            Board.rev3: 'a3-rev3',
			Board.plug: 'p-plug_hub',
            Board.uart: 'u-uart',
			Board.valve: 'w-water_valve',
			Board.plug2: 'p2-plug_2hub',
			Board.plug3: 'p3-plug_3hub',
		}
		return enumToPrefix.get(self)

def main():
	arg_pars = argparse.ArgumentParser(description = "Generating version information.")
	arg_pars.add_argument("--major", help="Major version", type=int, default=0, required = False)
	arg_pars.add_argument("--minor", help="Minor version", type=int, default=0, required = False)
	arg_pars.add_argument("--build", help="Build version", type=int, default=0, required = False)
	arg_pars.add_argument("--board", help="Board type", type=Board, choices=list(Board), default=Board.devboard, required = False)
	arg_pars.add_argument("--wifi-ssid", help="WiFi network name(Only for debugging)", type=str, required = False)
	arg_pars.add_argument("--wifi-psw", help="WiFi network password(Only for debugging)", type=str, required = False)
	arg_pars.add_argument("--initialbranch", help="Branch name(needed for Jenkins build)", type=str, required = False)
	arg_pars.add_argument("--config-dir", help="Config output directory", type=str, required = True)
	args = arg_pars.parse_args()

	if args.build == 0:
		args.major = 0
		args.minor = 0

	if os.path.isdir(args.config_dir):
		rmtree(args.config_dir)
	os.makedirs(args.config_dir)

	if os.path.isfile(args.config_dir + "/sdkconfig"):
		os.remove(args.config_dir + "/sdkconfig")
	if os.path.isfile(args.config_dir + "/sdkconfig.old"):
		os.remove(args.out_dconfig_dirir + "/sdkconfig.old")
	
	sdkConfigDefaultsPath = args.config_dir + "/sdkconfig.defaults"
	if os.path.lexists(sdkConfigDefaultsPath):
		os.remove(sdkConfigDefaultsPath)

	prefix = args.board.getPrefix()
	localtime = time.localtime()
	args.major, args.minor, args.build = writeVersionInfo(args.major, args.minor, args.build, args.board, args.wifi_ssid, args.wifi_psw, args.initialbranch, args.config_dir + "/version", localtime, prefix)

	fullBuildConfigFilePath = args.config_dir + "/config.cmake"
	versionFileHandle = open(fullBuildConfigFilePath, "w")
	versionFileHandle.writelines(configFileTemplate.format(ver_major=args.major, ver_minor=args.minor, ver_build=args.build, target_board=str(args.board).upper()))
	versionFileHandle.close()

	fullSourceSdkConfigFilePath = args.board.getSdkConfigFullPath(os.path.dirname(os.path.realpath(__file__)))
	# Only for Windows
	if os.name == 'nt':
		copyfile(fullSourceSdkConfigFilePath, sdkConfigDefaultsPath)
	else:
		os.symlink(os.path.relpath(fullSourceSdkConfigFilePath, os.path.dirname(sdkConfigDefaultsPath)), sdkConfigDefaultsPath)

	print(finishMessageTemplate.format(board=args.board, config_path=args.board.getSdkConfigRelPath(), out_dir=args.config_dir))

if __name__ == '__main__':
	exit(main())
