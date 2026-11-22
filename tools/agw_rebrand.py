#!/usr/bin/env python3
# ArmaGedonWeapons: przenosi paczke broni pod nasze nazwy (prefiks AG, klasy AG_*).
# Zamiany sa tej samej dlugosci, wiec struktura plikow (config.bin, p3d) zostaje nienaruszona.
import sys, re, struct, hashlib

SRC_WORD = bytes([75, 97, 114, 109, 97, 75, 114, 101, 119])  # nazwa zrodlowej paczki (9 znakow)
DST_WORD = b'ArmaGedon'
MEDIA = ('.paa', '.ogg', '.wss', '.png', '.jpg', '.edds', '.tga', '.wav', '.ptc', '.lip')

def _case(m, a=b'AG'):
    s = m.group(0)
    return bytes(a[i] if s[i] < 97 else a[i] + 32 for i in range(len(s)))

P_PATH = re.compile(rb'(?<![A-Za-z0-9_])[Kk]{2}(?=[\\/])')                # kk\  KK/
P_FILE = re.compile(rb'(?<=[\\/])[Kk]{2}(?=_)')                             # \kk_xxx
P_IDENT = re.compile(rb'(?<![A-Za-z0-9])KK(?=[A-Z_])')                      # KK_ADAR, KKMag
P_IDENT_L = re.compile(rb'(?<![A-Za-z0-9_])kk(?=_[a-z])')                   # kk_ammo
P_WORD = re.compile(re.escape(SRC_WORD), re.I)

def conv(data, full):
    data = P_PATH.sub(_case, data)
    data = P_FILE.sub(_case, data)
    if full:
        data = P_IDENT.sub(b'AG', data)
        data = P_IDENT_L.sub(b'ag', data)
        data = P_WORD.sub(DST_WORD, data)
    return data

def rebrand(src, dst):
    f = open(src, 'rb'); raw = f.read(); f.close()
    pos = 0; hdr_items = []; ents = []
    def rd():
        nonlocal pos
        e = raw.index(b'\0', pos); s = raw[pos:e]; pos = e + 1; return s
    while True:
        start = pos; n = rd(); h = struct.unpack_from('<5I', raw, pos); pos += 20
        if n == b'' and h[0] == 0x56657273:
            props = []
            while True:
                k = rd()
                if k == b'': break
                props.append((k, rd()))
            hdr_items.append(('props', props)); continue
        if n == b'': hdr_items.append(('end', h)); break
        if h[0] not in (0, 0x43707273) or (h[0] == 0x43707273 and h[1] != h[4] and h[1] != 0):
            pass
        ents.append([n, list(h)])
    data_off = pos; out_data = []; o = data_off
    for e in ents:
        n, h = e; size = h[4]; d = raw[o:o + size]; o += size
        low = n.lower().decode('latin1')
        if not low.endswith(MEDIA) and h[0] == 0:
            d = conv(d, not low.endswith('.p3d'))
        assert len(d) == size
        e[0] = conv(n, True); out_data.append(d)
    b = bytearray()
    for kind, v in hdr_items:
        if kind == 'props':
            b += b'\0' + struct.pack('<5I', 0x56657273, 0, 0, 0, 0)
            for k, val in v:
                if k.lower() == b'prefix': val = conv(val + b'\\', True)[:-1]
                b += k + b'\0' + val + b'\0'
            b += b'\0'
            for n, h in ents: b += n + b'\0' + struct.pack('<5I', *h)
        elif kind == 'end':
            b += b'\0' + struct.pack('<5I', *v)
    for d in out_data: b += d
    b += b'\0' + hashlib.sha1(bytes(b)).digest()
    open(dst, 'wb').write(b)

if __name__ == '__main__':
    rebrand(sys.argv[1], sys.argv[2])
