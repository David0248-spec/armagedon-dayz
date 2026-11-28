#!/usr/bin/env python3
"""ArmaGedonBase: budowanie bazy (konstrukcje + meble) w paczce Arma-Gedon.

Bierze paczki zrodlowe (uzyte za zgoda administracji zrodlowej paczki), przenosi je pod
nasze nazwy i zapisuje jako armagedonbase_*.pbo:
  - klasy, prefiksy, sciezki, defines i klucze tekstow -> AG_ / AGA_
  - autorzy w configach -> ARMA-GEDON
  - zbinaryzowane configi -> tekstowe config.cpp
  - modele/tekstury (binarne): tylko sciezki, zamiany tej samej dlugosci
  - nazwy selekcji z modeli (strefy, animacje, punkty) zostaja bez zmian, zeby model
    i config dalej do siebie pasowaly
  - poprawki: warsztat bez zaleznosci od zewnetrznego systemu craftingu, zlom -> blacha

Uzycie: agb_build.py <katalog_Addons_zrodla> <katalog_wyjscia>
"""
import hashlib
import os
import re
import struct
import sys

# zrodlo -> (plik wyjsciowy, prefiks)
PACKS = [
    ('tR_Dependence', 'armagedonbase_core.pbo', 'AG_Dependence'),
    ('tR_Fortification', 'armagedonbase_fort.pbo', 'AG_Fortification'),
    ('tR_Modules', 'armagedonbase_modules.pbo', 'AG_Modules'),
    ('tR_Stockade', 'armagedonbase_stockade.pbo', 'AG_Stockade'),
    ('tR_TreeHouse', 'armagedonbase_treehouse.pbo', 'AG_TreeHouse'),
    ('tR_Custom', 'armagedonbase_balance.pbo', 'AG_Custom'),
    ('Salvation_tRTreeHouseFix', 'armagedonbase_treefix.pbo', 'ArmaGedon_AGTreeHouseFix'),
    ('JLKStorage', 'armagedonbase_storage.pbo', 'AGAStorage'),
    ('JLK_Autoclose', 'armagedonbase_autoclose.pbo', 'AGA_Autoclose'),
    ('LoadedStash', 'armagedonbase_stash.pbo', 'AGA_Stashes'),
    ('barrel_stash', 'armagedonbase_barrels.pbo', 'AGA_BarStash'),
]

DROP = re.compile(r'(?i)(^extras[\\/]|\.md$|\.txt$|\.meta$)')
TEXT_EXT = ('.c', '.cpp', '.h', '.hpp', '.csv', '.xml', '.json', '.layout', '.imageset', '.txt', '.lst', '$pboprefix$')
SKIP_EXT = ('.bisign', '.zip')


# ---------------------------------------------------------------- PBO
def lzss(src, n):
    out = bytearray(); i = 0
    while len(out) < n and i < len(src):
        f = src[i]; i += 1
        for b in range(8):
            if len(out) >= n: break
            if f & (1 << b):
                out.append(src[i]); i += 1
            else:
                r = src[i] | (src[i + 1] << 8); i += 2
                rpos = len(out) - ((r & 0xff) | ((r & 0xf000) >> 4)); rl = ((r & 0x0f00) >> 8) + 3
                for k in range(rl):
                    p = rpos + k; out.append(out[p] if p >= 0 else 0x20)
    return bytes(out)


def read_pbo(path):
    b = open(path, 'rb').read(); i = 0; ents = []; props = []

    def cstr():
        nonlocal i
        j = b.index(b'\0', i); s = b[i:j]; i = j + 1; return s
    while True:
        name = cstr(); mt, osz, off, ts, dsz = struct.unpack_from('<5I', b, i); i += 20
        if name == b'' and mt == 0x56657273:
            while True:
                k = cstr()
                if k == b'': break
                props.append((k, cstr()))
            continue
        if name == b'': break
        ents.append([name, mt, osz, ts, dsz])
    pos = i; out = []
    for name, mt, osz, ts, dsz in ents:
        d = b[pos:pos + dsz]; pos += dsz
        if mt == 0x43707273 and osz:
            d = lzss(d, osz)
        out.append([name.decode('latin1'), ts, d])
    return props, out


def write_pbo(path, props, ents):
    h = bytearray()
    h += b'\0' + struct.pack('<5I', 0x56657273, 0, 0, 0, 0)
    for k, v in props:
        h += k + b'\0' + v + b'\0'
    h += b'\0'
    for name, ts, d in ents:
        h += name.encode('latin1') + b'\0' + struct.pack('<5I', 0, len(d), 0, ts, len(d))
    h += b'\0' + struct.pack('<5I', 0, 0, 0, 0, 0)
    for name, ts, d in ents:
        h += d
    h += b'\0' + hashlib.sha1(bytes(h)).digest()
    open(path, 'wb').write(bytes(h))


# ---------------------------------------------------------------- config.bin -> tekst
def derap(b):
    def ci(i):
        v = 0; s = 0
        while True:
            c = b[i]; i += 1; v |= (c & 0x7f) << s; s += 7
            if not c & 0x80: return v, i

    def az(i):
        j = b.index(b'\0', i); return b[i:j].decode('latin1'), j + 1

    def q(s):
        return '"' + s.replace('"', '""') + '"'

    def fl(f):
        for fmt in ('%g', '%.7g', '%.9g'):
            r = fmt % f
            if struct.unpack('<f', struct.pack('<f', float(r)))[0] == f:
                return r
        return '%.9g' % f

    def arr(i):
        n, i = ci(i); out = []
        for _ in range(n):
            t = b[i]; i += 1
            if t == 0: v, i = az(i); out.append(q(v))
            elif t == 1: out.append(fl(struct.unpack_from('<f', b, i)[0])); i += 4
            elif t == 2: out.append(str(struct.unpack_from('<i', b, i)[0])); i += 4
            elif t == 3: v, i = arr(i); out.append(v)
            elif t == 4: v, i = az(i); out.append(v)
            else: raise ValueError('typ tablicy %d' % t)
        return '{' + ','.join(out) + '}', i

    def body(off, ind, out):
        parent, i = az(off); n, i = ci(i)
        for _ in range(n):
            t = b[i]; i += 1
            if t == 0:
                name, i = az(i); o = struct.unpack_from('<I', b, i)[0]; i += 4
                p = az(o)[0]
                out.append('%sclass %s%s' % (ind, name, (': ' + p) if p else ''))
                out.append(ind + '{')
                body(o, ind + '\t', out)
                out.append(ind + '};')
            elif t == 1:
                st = b[i]; i += 1; name, i = az(i)
                if st == 0: v, i = az(i); v = q(v)
                elif st == 1: v = fl(struct.unpack_from('<f', b, i)[0]); i += 4
                elif st == 2: v = str(struct.unpack_from('<i', b, i)[0]); i += 4
                else: v, i = az(i)
                out.append('%s%s = %s;' % (ind, name, v))
            elif t in (2, 5):
                if t == 5: i += 4
                name, i = az(i); v, i = arr(i)
                out.append('%s%s[] %s= %s;' % (ind, name, '+' if t == 5 else '', v))
            elif t == 3:
                name, i = az(i); out.append('%sclass %s;' % (ind, name))
            elif t == 4:
                name, i = az(i); out.append('%sdelete %s;' % (ind, name))
            else:
                raise ValueError('typ wpisu %d' % t)
    if b[:4] != b'\0raP':
        raise ValueError('to nie jest config.bin')
    out = []
    body(16, '', out)
    return '\n'.join(out) + '\n'


# ---------------------------------------------------------------- nazwy
def case_like(src, dst):
    """dst w wielkosci liter src: male -> male, inaczej wielkie (Xyz -> Xyz)."""
    if src.islower():
        return dst.lower()
    if src[:1].isupper() and src[1:].islower():
        return dst[:1].upper() + dst[1:].lower()
    return dst.upper()


S_KEEP = set()   # nazwy z modeli (selekcje, animacje, punkty) - bez zmian


def keep_token(tok):
    lo = tok.lower()
    if lo in S_KEEP:
        return True
    if lo.endswith('_') and any(s.startswith(lo) for s in S_KEEP):
        return True
    return False


P_COMP = re.compile(r'(?<![A-Za-z0-9_.\-])((?:[tT][rR]_|[Jj][Ll][Kk]|[Ll][Oo][Aa][Dd][Ee][Dd][Ss][Tt][Aa][Ss][Hh]|[Bb][Aa][Rr][Rr][Ee][Ll]_[Ss][Tt][Aa][Ss][Hh])[A-Za-z0-9_ .\-]*)(?=[\\/])'
                    r'|(?<=[\\/])((?:[tT][rR]_|[Jj][Ll][Kk])[A-Za-z0-9_ .\-]*)')


def ren_component(c):
    lo = c.lower()
    if lo.startswith('loadedstash'):
        return case_like(c[:11], 'aga_stashes') + c[11:]
    if lo.startswith('barrel_stash'):
        return case_like(c[:12], 'aga_barstash') + c[12:]
    if lo.startswith('tr_'):
        return case_like(c[:2], 'ag') + c[2:]
    if lo.startswith('jlk'):
        return case_like(c[:3], 'aga') + c[3:]
    return c


def ren_paths(s):
    """Sciezki: katalogi i pliki zaczynajace sie od nazw zrodlowych (dlugosc bez zmian)."""
    def one(m):
        c = m.group(1) or m.group(2)
        return m.group(0).replace(c, ren_component(c), 1)
    return P_COMP.sub(one, s)


P_TR = re.compile(r'(?<![A-Za-z0-9])([tT][rR])(_[A-Za-z0-9_]*)')
P_TRCAP = re.compile(r'(?<![A-Za-z0-9])tR(?=[A-Z])')
P_JLK = re.compile(r'(?<![A-Za-z0-9])([Jj][Ll][Kk])([A-Za-z0-9_]*)')
P_PROXY = re.compile(r'(Proxy)((?:[tT][rR]_|[Jj][Ll][Kk])[A-Za-z0-9_]*)')


PROXY_PATHED = set()   # proxy wskazywane sciezka w modelach (plik modelu dostaje nowa nazwe)


def proxy_sub(m):
    tok = m.group(2)
    if tok.lower() not in PROXY_PATHED and keep_token(tok):
        return m.group(0)
    if tok.lower().startswith('tr'):
        return m.group(1) + case_like(tok[:2], 'ag') + tok[2:]
    return m.group(1) + case_like(tok[:3], 'aga') + tok[3:]
P_SALVATION = re.compile(r'(?i)salvation')
P_SALV = re.compile(r'(?<![A-Za-z0-9])([Ss]alv)(?=[A-Z_])')
P_LOADED = re.compile(r'LoadedStash')
P_BARREL = re.compile(r'(?<![A-Za-z0-9_])barrel_stash(?![A-Za-z0-9_])')


def ren_text(s):
    s = re.sub(r'(?im)^(\s*(?:author|credits)\s*=\s*)"[^"]*"', r'\1"ARMA-GEDON"', s)
    s = re.sub(r'(?im)^(\s*authorID\s*=\s*)"[^"]*"', r'\1"0"', s)
    s = ren_paths(s)
    s = P_PROXY.sub(proxy_sub, s)
    s = P_TR.sub(lambda m: m.group(0) if keep_token(m.group(0)) else case_like(m.group(1), 'ag') + m.group(2), s)
    s = P_TRCAP.sub('AG', s)
    s = P_JLK.sub(lambda m: m.group(0) if keep_token(m.group(0)) else case_like(m.group(1), 'aga') + m.group(2), s)
    s = P_SALVATION.sub(lambda m: case_like(m.group(0), 'armagedon'), s)
    s = P_SALV.sub(lambda m: case_like(m.group(1), 'arma'), s)
    s = P_LOADED.sub('AGA_Stashes', s)
    s = P_BARREL.sub('AGA_BarStash', s)
    s = s.replace('Tierro', 'ArmaGedon').replace('MightyMoitzbert', 'ARMA-GEDON').replace('Mighty Moitzbert', 'ARMA-GEDON')
    return s


def ren_binary(d):
    """Pliki binarne: tylko sciezki i nazwy zrodla, zamiany tej samej dlugosci."""
    s = d.decode('latin1')
    s2 = ren_paths(s)
    s2 = P_SALVATION.sub(lambda m: case_like(m.group(0), 'armagedon'), s2)
    s2 = re.sub(r'(?i)tierro', lambda m: case_like(m.group(0), 'agteam'), s2)
    s2 = s2.replace('Tierro', 'AGTeam').replace('MightyMoitzbert', 'ArmaGedonStudio').replace('Mighty Moitzbert', 'ArmaGedon Studio')
    out = s2.encode('latin1')
    assert len(out) == len(d)
    return out


def collect_keep(src_dir):
    tok = re.compile(rb'(?<![A-Za-z0-9])(?:[tT][rR]_|[Jj][Ll][Kk])[A-Za-z0-9_]*')
    for src, _, _ in PACKS:
        p = os.path.join(src_dir, src + '.pbo')
        if not os.path.exists(p):
            continue
        _, ents = read_pbo(p)
        for name, ts, d in ents:
            if not name.lower().endswith('.p3d'):
                continue
            for m in re.finditer(rb'proxy:([^\x00]*?)\.\d\d\d', d):
                pr = m.group(1).decode('latin1')
                if '\\' in pr or '/' in pr:
                    PROXY_PATHED.add(re.split(r'[\\/]', pr)[-1].lower())
            for m in re.finditer(rb'[\x21-\x7e]{3,}', d):
                st = m.group(0)
                if b'\\' in st or b'/' in st:
                    continue
                for t in tok.findall(st):
                    S_KEEP.add(t.decode('latin1').lower())


# ---------------------------------------------------------------- poprawki (na tekscie zrodlowym)
WORKBENCH_C = '''// warsztat: przechowywanie + naprawa narzedzi (bez zewnetrznego menu craftingu)
class JLK_Storage_Workbench_Base : Container_Base
{
	override bool IsHeavyBehaviour() { return true; }
	override bool IsTwoHandedBehaviour() { return true; }
	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionTogglePlaceObject);
		AddAction(ActionPlaceObject);
	}
}

class JLK_Storage_Workbench_Lvl_1 : JLK_Storage_Workbench_Base {}

class JLK_Storage_Workbench_Lvl_2 : JLK_Storage_Workbench_Base
{
	override bool CanDisplayAttachmentSlot(int slot_id)
	{
		if (slot_id == InventorySlots.GetSlotIdFromString("JLK_Slot_Drill") && !GetInventory().FindAttachment(slot_id))
			return false;
		return super.CanDisplayAttachmentSlot(slot_id);
	}

	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (slotId == InventorySlots.GetSlotIdFromString("JLK_Slot_Drill"))
			return false;
		return super.CanReceiveAttachment(attachment, slotId);
	}
}

class JLK_Storage_Workbench_Lvl_3 : JLK_Storage_Workbench_Base {}
'''

SVWB_COND = re.compile(r'\n[ \t]*SVWB_CraftBench m_bench = SVWB_CraftBench\.Cast\(m_mediumbox\);\s*\n[ \t]*if \(m_bench && m_bench\.m_SVWB_Active\)\s*\n[ \t]*return false;\n')
SVWB_EXEMPT = re.compile(r'(bool IsUpgradeExemptAttachment\(ItemBase box, EntityAI attachment\) \{).*?(\n    \}\n)', re.S)


def patch(src, name, s):
    s = s.replace('\r\n', '\n')
    low = name.lower().replace('\\', '/')
    if src == 'JLKStorage':
        if low.endswith('furniture/workbench.c'):
            return WORKBENCH_C
        if 'actionupgradeworkbench_' in low:
            s, n1 = SVWB_COND.subn('\n', s)
            s, n2 = SVWB_EXEMPT.subn(r'\1\n        return false;\2', s)
            assert n1 == 1 and n2 == 1, (name, n1, n2)
        if low.endswith('.c'):
            s = s.replace('"Loaded_Scrap_Metal"', '"MetalPlate"')
            assert 'SVWB' not in s, name
        if low == 'config.cpp':
            s, n = re.subn(r',?"SVWB_[A-Za-z0-9_]*"', '', s)
            assert n > 0
            s = s.replace(',"Salvation_Workbench"', '').replace('"Salvation_Workbench",', '')
            assert 'SVWB' not in s and 'Salvation_Workbench' not in s
        if low == 'data/redefine/config.cpp':
            s2 = re.sub(r'\tclass Loaded_Scrap_Metal: Inventory_Base\n\t\{\n\t\tinventorySlot\[\] \+= \{[^}]*\};\n\t\};\n', '', s)
            assert s2 != s
            s = s2.replace('"MetalPlate_Upgrade_x2"};', '"MetalPlate_Upgrade_x2","ScrapMetal_Upgrade_x2","ScrapMetal_Upgrade_x6"};', 1)
            assert 'ScrapMetal_Upgrade_x6"};' in s
            s = re.sub(r',?"SVWB_[A-Za-z0-9_]*"', '', s).replace('{,', '{')
            s = re.sub(r'\tclass Slot_SVWB_\w+\n\t\{.*?\n\t\};\n', '', s, flags=re.S)
            assert 'SVWB' not in s
    if src == 'JLK_Autoclose':
        s = s.replace('JLK AutoClose', 'AutoClose').replace('[JLK]', '[AGA]')
    return s


# ---------------------------------------------------------------- budowanie
def build(src_dir, out_dir):
    collect_keep(src_dir)
    os.makedirs(out_dir, exist_ok=True)
    for src, outname, prefix in PACKS:
        p = os.path.join(src_dir, src + '.pbo')
        if not os.path.exists(p):
            sys.exit('BRAK %s' % p)
        props, ents = read_pbo(p)
        nprops = []
        for k, v in props:
            if k == b'prefix':
                nprops.append((k, prefix.encode()))
            elif k == b'product':
                nprops.append((k, v))
        if not any(k == b'prefix' for k, _ in nprops):
            nprops.append((b'prefix', prefix.encode()))
        out = []
        for name, ts, d in ents:
            low = name.lower()
            if low.endswith(SKIP_EXT) or DROP.search(name):
                continue
            if low in ('$prefix$', '$pboprefix$'):
                out.append([name, ts, prefix.encode()])
                continue
            if low.endswith('config.bin'):
                txt = derap(d)
                name = name[:-3] + 'cpp'
                txt = patch(src, name, txt)
                d = ren_text(txt).encode('latin1')
            elif low.endswith(TEXT_EXT) or low == '$pboprefix$':
                txt = d.decode('latin1')
                if low == '$pboprefix$':
                    txt = prefix
                txt = patch(src, name, txt)
                d = ren_text(txt).encode('latin1')
            else:
                d = ren_binary(d)
            out.append([ren_paths_entry(name), ts, d])
        write_pbo(os.path.join(out_dir, outname), nprops, out)
        print('ArmaGedonBase: %s (%d plikow)' % (outname, len(out)))


def ren_paths_entry(name):
    return ren_paths(name)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    build(sys.argv[1], sys.argv[2])
