import itertools
import os.path
import shutil
import struct
import sys
import lz4.block
from lz4.block import _block

PATHS = 'paths.txt'
WORDS = 'words.txt'
BLOCK_FMT = '<IIIII'
BLOCK_SZ = struct.calcsize(BLOCK_FMT)
EXTS = {
    0x5E5AB54D: "dds",
    0x1E1EA7D5: "sph",
    0x06C0B650: "mp3",
    0x5CE18709: "dat",
    0x93FF62E0: "psc",
    0x17B1DCCE: "scb",
    0x1E22F830: "bson",
    0xD7EBE1E0: "loc",
    0x208EF3E0: "sub",
    0x5ED5C411: "dep",
    0x71F3BA83: "hlsl",
    0xF10C041F: "rsb",
    0x1F92D4DE: "ssb",
    0x08F2025B: "mtl",
    0x35B99DDD: "cpp",
    0x483D0203: "unicode"
}


def sdbm(buf, h=0):
    buf = buf.encode()

    h = 0

    for b in buf:
        h = (0x1003F * (h + b)) & 0xFFFFFFFF

    return h


def extract_block(f, paths, not_found):
    block = f.read(BLOCK_SZ)
    b_path, b_fname, b_ext, b_dsize, b_csize = struct.unpack(BLOCK_FMT, block)

    b_cmp = f.read(b_csize)
    try:
        b_dec = lz4.block.decompress(b_cmp, uncompressed_size=b_dsize, return_bytearray=True)
    except _block.LZ4BlockError:
        b_dec = b_cmp

    path = None
    for k, v in paths.items():
        if v == b_path:
            path = k
            break

    if path:
        fname_dir, fname_name = os.path.split(path)

        return {
            'path': fname_dir if fname_name else '',
            'fname': fname_name if fname_name else fname_dir,
            'ext': EXTS[b_ext],
            'dsize': b_dsize,
            'csize': b_csize,
            'dec': b_dec,
            'cmp': b_cmp
        }, f.tell()

    not_found.add(b_path)

    return {
        'path': ('%08X' % b_path),
        'fname': ('%08X' % b_fname),
        'ext': EXTS[b_ext],
        'dsize': b_dsize,
        'csize': b_csize,
        'dec': b_dec,
        'cmp': b_cmp
    }, f.tell()


def load_paths():
    paths = {}

    with open(PATHS) as f:
        for line in f.readlines():
            name = line.rstrip()
            paths[name] = sdbm(name)

    return paths


def load_dirs(dd):
    dirs = {}

    with open(PATHS) as f:
        for line in f.readlines():
            name = line.rstrip()
            fd, fn = os.path.split(name)

            if fn:
                dirs[fd] = sdbm(fd, dd)

    print('dirs loaded')

    return dirs


def load_words(dd):
    words = {}

    with open(WORDS) as f:
        for line in f.readlines():
            name = line.rstrip()
            words[name] = sdbm(name, dd)

    print('words loaded')

    return words


def main(path):
    paths = load_paths()
    size = os.path.getsize(path)

    fd, fn = os.path.split(path)
    fn, fe = os.path.splitext(fn)
    ex_dir = os.path.join(fd, 'extracted_%s' % fn)
    os.makedirs(ex_dir, exist_ok=True)

    not_found = set()
    with open(path, 'rb') as f:
        off = 0

        while off < size:
            b_info, off = extract_block(f, paths, not_found)

            os.makedirs(os.path.join(ex_dir, b_info['path']), exist_ok=True)
            fname = os.path.join(ex_dir, b_info['path'], '%s.%s' % (b_info['fname'], b_info['ext']))

            with open(fname, 'wb') as w:
                w.write(b_info['dec'])

            fname = fname.replace('\\', '/')
            p = fname.find('/')
            print(fname[p+1:])


if __name__ == '__main__':
    main(sys.argv[1])
