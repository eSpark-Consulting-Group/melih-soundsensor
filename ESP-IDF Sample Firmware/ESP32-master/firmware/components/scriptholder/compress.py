#!/usr/bin/env python3

import os, argparse, json, subprocess as sp, pathlib as pt, shutil
from collections import OrderedDict

def get_rev_id( execDir ):
    try:
        data = sp.check_output("git describe --tags", shell=True, cwd=execDir).decode('utf-8').rstrip('\n')
        return data.split('/')[1]
    except:
        return "unknow"

def minify_json(json_file):
    with open(str(json_file), "r") as file:
        content = json.load(file, object_pairs_hook=OrderedDict)

    with open(str(json_file), "w") as file:
        json.dump(content, file, separators=(',', ':'))

def process_dir( in_dir, out_dir, compiler, revId ):
    out_dir = pt.Path(out_dir)
    if out_dir.exists():
        if out_dir.is_dir():
            shutil.rmtree( str( out_dir ) )
        else:
            out_dir.unlink()
    out_dir.mkdir()
    root_out = pt.Path( out_dir ).absolute()

    for root, dirs, files, _ in os.fwalk(in_dir):
        relative_root = pt.Path(root).relative_to(in_dir)
        for file in [pt.Path(name) for name in files]:
            if file.suffix == '.lua':
                command = ' '.join([
                    compiler,
                    '-s',
                    '-o',
                    str(root_out.joinpath(relative_root).joinpath(file)),
                    str(pt.Path(root).joinpath(file))])
                result = sp.run(
                    command, timeout=30, stdout=sp.PIPE, shell=True)
                result.check_returncode()
            else:
                shutil.copy(
                    str(pt.Path(root).joinpath(file)),
                    str(root_out.joinpath(relative_root) ))

                if file.suffix == '.json':
                    minify_json(root_out.joinpath(relative_root).joinpath(file))

        for dirname in dirs:
            os.mkdir(str(root_out.joinpath(relative_root).joinpath(dirname)))

    with open(str(out_dir) + '/config.json', 'r+') as config:
        jsObject = json.load(config)
        version = jsObject['version'].split(".")
        jsObject['version'] = '{0}.{1}.{2}'.format(version[0], version[1], revId)
        config.seek(0)
        json.dump(jsObject, config)
        config.truncate()

def main():
	arguments_parser = argparse.ArgumentParser(description = "")
	arguments_parser.add_argument('--pluginlist', help="Input plugin list", nargs='+', required = True)
	arguments_parser.add_argument('--artype', help="Archive type: 'zip', 'tar.zz', 'molfar.zz'", type=str, required = True)
	arguments_parser.add_argument('--arpath', help="Archive path", type=str, required = True)
	arguments_parser.add_argument('--tmppath', help="temp path", type=str, required = True)
	arguments_parser.add_argument('--luac', help="Lua compiler", type=str, required = True)
	arguments_parser.add_argument('--plugdir', help="Plugins directory", type=str, required = True)
	args = arguments_parser.parse_args()
	
	pluginlist = args.pluginlist
	artype = args.artype
	arpath = args.arpath
	tmppath = args.tmppath
	luac = args.luac

	tmp_dir = pt.Path(tmppath)
	if tmp_dir.exists():
		if tmp_dir.is_dir():
			shutil.rmtree( str( tmp_dir ) )
		else:
			tmp_dir.unlink()
	tmp_dir.mkdir()

	print('Input list: {pluginlist}'.format(pluginlist=pluginlist))
	print('Archive type: {artype}'.format(artype=artype))
	print('Arpath: {arpath}'.format(arpath=arpath))
	print('Tmp ath: {arpath}'.format(arpath=tmppath))
	print('Lua compiler: {arpath}'.format(arpath=luac))

	rev_id = get_rev_id(args.plugdir)
	print('Revision id: {0}'.format(rev_id))

	content = []

	for plugin in pluginlist:
		plugin_name = os.path.basename(plugin)
		compiled_plugin = tmp_dir.joinpath(plugin_name)
		process_dir(plugin, compiled_plugin, luac, rev_id)
		cmd = {
			'zip': 'zip -jrv {plugin}.zip {plugin} && mv {plugin}.zip {arpath}/{name}'.format(plugin=compiled_plugin, arpath=arpath, name=plugin_name),
			'tar.zz': 'flist=$(ls {plugin}) && tar -cf {plugin}.tar -C {plugin} $flist \
				&& pigz -cz {plugin}.tar > {plugin}.tar.zz \
				&& mv {plugin}.tar.zz {arpath}/{name}.tar.zz'.format(plugin=compiled_plugin, arpath=arpath, name=plugin_name),
			'molfar.zz': 'python3 molfar.py --source_dir_path={src_dir} --output_file_path={src_dir}.molfar \
				&& pigz -cz {src_dir}.molfar > {molfar_zz}'.format(src_dir=f"{tmp_dir}/{plugin_name}", molfar_zz=f"{arpath}/{plugin_name}.molfar.zz")
		}
		sp.run(cmd.get(artype), shell=True)
		with open(plugin + '/config.json') as config:
			data = json.load(config)
			version = data['version'].split(".")
			major = version[0]
			minor = version[1]
			plugin_filename = ".".join([os.path.basename(plugin), artype])
			content.append( 'LUASCRIPT( "{name}", {major}, {minor}, "{type}", {filename} )'.format(name=os.path.basename(plugin), major=major, minor=minor, type=artype, filename=plugin_filename.replace(".", "_")) )

	outListFile = 'scriptslist.h'
	f = open(outListFile, 'w')
	f.truncate(0)
	for item in content:
		f.write("%s\n" % item)
	
if __name__ == '__main__':
	exit(main())



