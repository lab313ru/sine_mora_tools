import os.path
import struct
import subprocess
from base64 import b64decode
import lz4.block
import sys
from subprocess import Popen
import re


STR = re.compile(rb'[\'"](.+?)[\'"]')


filekey_base64 = b'McLaXCI3KUE='
filekey = b'\x00' * 8
cryptkey = 0x5F148D4B
randseed = 0


TAG = 0x73476365
HDR_FMT = '<I 8s'
HDR_SIZE = struct.calcsize(HDR_FMT)


def sdbm(h, buf):
    for c in buf:
        h = (0x1003F * (h + c)) & 0xFFFFFFFF
    return h


def inthash(seed):
    seed = ((seed ^ 61) ^ (seed >> 16)) & 0xFFFFFFFF
    seed = (seed * 9) & 0xFFFFFFFF
    seed = (seed ^ (seed >> 4)) & 0xFFFFFFFF
    seed = (seed * 0x27d4eb2d) & 0xFFFFFFFF
    seed = (seed ^ (seed >> 15)) & 0xFFFFFFFF
    return seed


def next_key():
    global randseed
    res = inthash(randseed)
    randseed = res
    return res



def raw_encdec(fk, key, add):
    global randseed

    if key == 0:
        key = cryptkey

    randseed = key

    fk = bytearray(fk)
    lf = len(fk)
    count = ((lf - 1) >> 2) + 1

    if count*4 > lf:
        fk += b'\x00' * (lf - count*4)

    for i in range(count):
        nk = next_key()
        fk_dw = struct.unpack_from('<I', fk, i * 4)[0]

        if add:
            struct.pack_into('<I', fk, i * 4, (fk_dw + nk) & 0xFFFFFFFF)
        else:
            struct.pack_into('<I', fk, i * 4, (fk_dw - nk) & 0xFFFFFFFF)

    return bytes(fk)


def get_key(path):
    global filekey

    if filekey == b'\x00' * 8:
        v8 = b64decode(filekey_base64)
        v7 = raw_encdec(v8, cryptkey, False)

        if len(v7) > 7:
            filekey = v7

    fd, fn = os.path.split(path)
    fn = fn.lower()
    hs = sdbm(0, fn.encode())

    return hs


def file_encdec(buf, fkey, key, start):
    buf = bytearray(buf)
    size = len(buf)

    fkey1 = struct.unpack_from('<I', fkey, 4)[0]
    fkey0 = struct.unpack_from('<I', fkey, 0)[0]
    h1 = inthash(key)
    x0 = (fkey1 + h1) & 0xFFFFFFFF

    tail = start % 16

    x1 = ((start + (0x0F if start < 0 else 0)) & 0xFFFFFFF0) >> 2
    x2 = (inthash((key + 0x544836) & 0xFFFFFFFF) + fkey0) & 0xFFFFFFFF

    x5 = x6 = 0

    index = -tail
    while index < size:
        x3 = index

        if (x1 & 3) == 0:
            x4 = inthash(x1 >> 2)
            x5 = x4 ^ x0
            x6 = x4 ^ x2


        if x1 & 1:
            x10 = x9 = x5 = inthash(x5)
        else:
            x10 = x9 = x6 = inthash(x6)

        x1 += 1

        if x3 >= -3:
            if 0 <= x3 < size - 3:
                xx = struct.unpack_from('<I', buf, index)[0]
                struct.pack_into('<I', buf, index, xx ^ x9)
            else:
                x11 = max(tail, 0)
                x12 = tail + size

                if x12 >= 4:
                    x12 = 4

                x13 = 8 * x11

                while x11 < x12:
                    x14 = x10 >> x13
                    x13 += 8

                    xx = struct.unpack_from('B', buf, x11 + index)[0]
                    struct.pack_into('B', buf, index + x11, (xx ^ x14) & 0xFF)
                    x11 += 1

        tail -= 4
        index += 4

    return bytes(buf)


def parse_table(buf):
    off = 0
    lb = len(buf)

    tbl = {}
    while off < lb:
        size = struct.unpack_from('<I', buf, off)[0]
        off += 4

        name_len = struct.unpack_from('<I', buf,  off)[0]
        off += 4

        name = buf[off:off+name_len].decode()
        off += name_len

        tbl[name] = size

    return tbl


def read_block(fn, f, size):
    fname_key = get_key(fn)

    hdr = f.read(HDR_SIZE)

    tag, enc = struct.unpack_from(HDR_FMT, hdr)

    if tag == TAG:
        res = file_encdec(enc, filekey, fname_key, 0)

        cmp_size, unp_size = struct.unpack_from('<II', res)

        cmp = f.read(cmp_size)
        assert len(cmp) == cmp_size

        res = file_encdec(cmp, filekey, fname_key, 8)

        if unp_size:
            res = lz4.block.decompress(res, uncompressed_size=unp_size, return_bytearray=True)
    else:
        res = hdr + f.read(size - HDR_SIZE)

    return res


def read_table(fn, f):
    block = read_block(fn, f, HDR_SIZE)
    return parse_table(block)


NOT_FOUND_PATHS = [
    0x02376749,
    0x07F44146,
    0x0AE89E4C,
    0x0B455F4D,
    0x195DAA4E,
    0x30605F88,
    0x48048ACC,
    0x5168C2CB,
    0x52463E5E,
    0x713080FD,
    0x917A1FB5,
    0xA40858CE,
    0xAE57110C,
    0xB631FD83,
    0xE7800A90,
    0xE7890CC7,
    0xE78F0E41,
    0xEE778008,
    0xEF8898CF
]

NOT_FOUND_NAMES = [
    0x4CE45384,
    0x4DE05709,
    0x5209CDC6,
    0x5CA53F3A,
    0x5DEFD6DA,
    0x69C0D1F9,
    0x6D5B90B5,
    0x76DC64B8,
    0x97DA8837,
    0xB37D2105,
    0xB883979C,
    0xCB43CD47,
    0xDC4D7E12
]

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


def main(path):
    fd, fn = os.path.split(path)
    fn_ex, _ = os.path.splitext(fn)
    fn_ex = os.path.join(fd, 'extracted_%s' % fn_ex)

    with open(path, 'rb') as f:
        tbl = read_table(fn, f)

        for name, size in tbl.items():
            nn, ne = os.path.splitext(name)

            # hs = sdbm(0, nn.encode())
            #
            # if hs in NOT_FOUND_PATHS:
            #     print('path found: %s => %08X' % (nn, hs))
            #
            # n_path, n_name = os.path.split(name)
            # nn_name, ne_name = os.path.splitext(n_name)
            #
            # for ext in EXTS:
            #     hs = sdbm(0, ('%s%s' % (nn_name, ext)).encode())
            #
            #     if hs in NOT_FOUND_NAMES:
            #         print('name found: %s => %08X' % (nn, hs))
            #         break

            print('extracting %s from 0x%08X, size=0x%X... ' % (name, f.tell(), size), end='')
            dec = read_block(name, f, size)

            if not name.endswith('.level'):
                print('done!')
                continue

            pp = os.path.join(fn_ex, name)
            xx, _ = os.path.split(pp)

            os.makedirs(xx, exist_ok=True)

            with open(pp, 'wb') as w:
                w.write(dec)

            print('done!')

            if name.endswith('.pvr'):
                nn, ne = os.path.splitext(pp)
                Popen(['PVRTexToolCLI.exe', '-i', pp, '-noout', '-d', '%s.png' % nn], shell = True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            elif name.endswith('.level'):
                px = STR.findall(dec)

                for p in px:
                    try:
                        pn, pe = os.path.split(p.decode())
                    except Exception:
                        continue

                    for ext in EXTS:
                        hs = sdbm(0, ('%s%s' % (pe, ext)).encode())

                        if hs in NOT_FOUND_NAMES:
                            print('name found: %s => %08X' % (nn, hs))
                            continue


if __name__ == '__main__':
    main(sys.argv[1])
