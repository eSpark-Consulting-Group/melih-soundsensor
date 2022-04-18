import subprocess
import pathlib as pt
from .exception import RuntimeException


def run_shell_command(command, timeout_s):
    shell_command = ' '.join([str(elem) for elem in command])
    result = subprocess.run(
        shell_command, timeout=timeout_s, stdout=subprocess.PIPE, shell=True)
    try:
        result.check_returncode()
    except subprocess.CalledProcessError:
        raise RuntimeException()


def make_filename_in_directory(directory, filename):
    return str(pt.Path(directory).joinpath(pt.Path(filename).name))
