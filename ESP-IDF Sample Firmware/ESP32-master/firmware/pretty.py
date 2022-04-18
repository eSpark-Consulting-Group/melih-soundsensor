#!/usr/bin/env python3
import argparse, glob, re, subprocess
from os import path

fmt_tool = "clang-format"
required_fmt_version = 6

def checkClangFormatVersion():
	cf_verion = subprocess.check_output("%s --version" % fmt_tool, shell=True)
	version_regex = re.compile(r"clang-format version ([0-9]+)\.")
	major_version_num = int(version_regex.match(cf_verion.decode("utf-8")).group(1))
	return major_version_num >= required_fmt_version

def filesListFromPaths(inputDirs):
	extensions_regex = re.compile(r"(.*\.h$|.*\.cpp$|.*\.c$|.*\.hpp$)")
	files_list = []
	for directory in inputDirs:
		files_and_dirs = glob.glob(directory+'/'+"**", recursive=True )
		files = filter(path.isfile, files_and_dirs)
		files_list.extend( ( filter( extensions_regex.match, files ) ) )
	return files_list

def checkFilesFormatting(inputDirs, excludeDirs):
	nonFormattedFilesList = getFilesMustBeFormatted(inputDirs)
	if excludeDirs:
		excludeList = getFilesMustBeFormatted(excludeDirs)
		for x in excludeList:
			nonFormattedFilesList.remove(x)
	if nonFormattedFilesList:
		print("Must be formatted:")
		print("\n".join(nonFormattedFilesList))
		exit(1)

def getFilesMustBeFormatted(inputDirs):
	files = filesListFromPaths(inputDirs)
	nonFormattedFiles = []
	for file in files:
		out = subprocess.check_output("%s -output-replacements-xml " % fmt_tool+ file, shell=True)
		if("<replacement offset=" in str(out)):
			nonFormattedFiles.append(file)
	return nonFormattedFiles


def formattingFiles(inputDirs, excludeDirs):
	files = getFilesMustBeFormatted(inputDirs)
	if excludeDirs:
		excludeList = getFilesMustBeFormatted(excludeDirs)
		for x in excludeList:
			files.remove(x)
	if files:
		print("This files were formatted:")
		for file in files:
			subprocess.run("clang-format -i %s" % file, shell=True)
			print(path.relpath(file))
	exit(0)

def main():
	if not checkClangFormatVersion():
		exit("clang-format version not supported, expected >= %d" % required_fmt_version)
	arguments_parser = argparse.ArgumentParser(description = "Formatting the source code.")
	arguments_parser.add_argument("--action", help="Desired action.", choices=['check', 'format'], required = True)
	#arguments_parser.add_argument("--format-file", help="Path to format file.", metavar="PATH",default = ".clang-format")
	arguments_parser.add_argument("--paths", nargs='+', metavar="", help = """Paths to directories in which you need to 
		format or check the files formatting. Default currently directory.""", default = '.')
	arguments_parser.add_argument("--exclude", nargs='+', metavar="", help = """Paths to directories in which you need exclude from
		format or check the files formatting.""")
	args = arguments_parser.parse_args()
	if args.action == 'check':
		checkFilesFormatting(args.paths, args.exclude)
	else:
		formattingFiles(args.paths, args.exclude)


if __name__ == '__main__':
	exit(main())
