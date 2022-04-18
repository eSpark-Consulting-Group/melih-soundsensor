#!/usr/bin/env python3

import os, struct, argparse

BLOCK_ALIGNMENT = 4

def iter_files_recursively(path):
    for root, dirs, files in os.walk(path):
        for name in files:
            with open(os.path.join(root, name), "rb") as f:
                yield f

def align_block(content_len):
    return bytes(BLOCK_ALIGNMENT - ((content_len % BLOCK_ALIGNMENT) or BLOCK_ALIGNMENT))

def write_aligned(dest_file, binary_data):
    dest_file.write(align_block(dest_file.write(binary_data))) 

def serialize_file(dest_file, source_file, path_base):
    rel_path = os.path.relpath(source_file.name, path_base)
    dest_file.write(struct.pack("<ii", len(rel_path), os.path.getsize(source_file.name)))
    write_aligned(dest_file, rel_path.encode("ascii"))
    write_aligned(dest_file, source_file.read())

def compress_dir(source_dir_path, output_file_path):
    with open(output_file_path, "wb") as out_file:
        for input_file in iter_files_recursively(source_dir_path):
            serialize_file(out_file, input_file, source_dir_path)

if __name__ == "__main__":
    args_parser = argparse.ArgumentParser(description = "")
    args_parser.add_argument('--source_dir_path', help="Path to a directory to be compressed", type=str, required = True)
    args_parser.add_argument('--output_file_path', help="Path to the output file", type=str, required = True)
    args = args_parser.parse_args()
    exit(compress_dir(args.source_dir_path, args.output_file_path))
