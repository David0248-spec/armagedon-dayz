#!/usr/bin/env python3
"""Buduje ArmaGedonCore.pbo ze źródeł w mods/ArmaGedonCore (PBO bez kompresji, nagłówek z prefixem).

Użycie:  python3 tools/build_pbo.py [katalog_zrodel] [plik_wyjsciowy] [prefix]
Domyślnie: mods/ArmaGedonCore -> dist/@ArmaGedonCore/addons/ArmaGedonCore.pbo, prefix ArmaGedonCore

Kolejność plików jest stała (posortowana), a czas plików wyzerowany, więc te same źródła
dają zawsze ten sam .pbo (łatwo sprawdzić w git, czy paczka jest aktualna).
"""
import hashlib, os, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'mods', 'ArmaGedonCore')
out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'dist', '@ArmaGedonCore', 'addons', 'ArmaGedonCore.pbo')
prefix = sys.argv[3] if len(sys.argv) > 3 else 'ArmaGedonCore'

# pliki pomijane (nie trafiają do paczki)
SKIP_EXT = ('.bak', '.orig', '.tmp', '.swp')
SKIP_NAMES = ('Thumbs.db', '.DS_Store', 'desktop.ini')
# kolejność jak w dotychczasowej paczce: najpierw config.cpp w katalogu głównym, potem reszta alfabetycznie
FIRST = ('config.cpp',)


def collect(base):
    files = []
    for d, dirs, names in os.walk(base):
        dirs[:] = sorted(x for x in dirs if not x.startswith('.'))
        for n in names:
            if n.startswith('.') or n in SKIP_NAMES or n.lower().endswith(SKIP_EXT):
                continue
            full = os.path.join(d, n)
            rel = os.path.relpath(full, base).replace('/', '\\')
            files.append((rel, full))
    return files


def order_key(rel):
    # kolejność jak w dotychczasowej paczce: config.cpp, Scripts, Balance, GUI, Names, Data, reszta
    low = rel.lower()
    if rel in FIRST:
        return (0, '')
    for i, d in enumerate(('scripts\\', 'balance\\', 'gui\\', 'names\\', 'data\\')):
        if low.startswith(d):
            return (1 + i, low)
    return (9, low)


def build(files):
    files = sorted(files, key=lambda f: order_key(f[0]))
    data = [open(full, 'rb').read() for _, full in files]
    hdr = bytearray()
    # nagłówek wersji: pusta nazwa + 'Vers' + 4x0, potem para prefix/wartosc i pusty string
    hdr += b'\0' + struct.pack('<5I', 0x56657273, 0, 0, 0, 0)
    hdr += b'prefix\0' + prefix.encode('latin1') + b'\0' + b'\0'
    for (rel, _), d in zip(files, data):
        hdr += rel.encode('latin1') + b'\0' + struct.pack('<5I', 0, len(d), 0, 0, len(d))
    hdr += b'\0' + struct.pack('<5I', 0, 0, 0, 0, 0)
    body = bytes(hdr) + b''.join(data)
    # stopka: bajt 0 + SHA1 całości
    return body + b'\0' + hashlib.sha1(body).digest(), files


if __name__ == '__main__':
    files = collect(src)
    if not files:
        sys.exit('BRAK plikow w %s' % src)
    blob, files = build(files)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    open(out, 'wb').write(blob)
    print('PBO: %s (%d plikow, %d B)' % (out, len(files), len(blob)))
