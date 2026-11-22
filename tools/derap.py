import sys, struct
def ci(b, i):
    v = 0; s = 0
    while True:
        c = b[i]; i += 1; v |= (c & 0x7f) << s; s += 7
        if not c & 0x80: return v, i
def az(b, i):
    j = b.index(b'\0', i); return b[i:j].decode('latin1'), j + 1
def arr(b, i):
    n, i = ci(b, i); out = []
    for _ in range(n):
        t = b[i]; i += 1
        if t == 0: v, i = az(b, i); out.append('"%s"' % v)
        elif t == 1: out.append('%g' % struct.unpack_from('<f', b, i)[0]); i += 4
        elif t == 2: out.append(str(struct.unpack_from('<i', b, i)[0])); i += 4
        elif t == 3: v, i = arr(b, i); out.append(v)
        elif t == 4: v, i = az(b, i); out.append(v)
    return '{' + ','.join(out) + '}', i
def body(b, off, ind, out):
    parent, i = az(b, off); n, i = ci(b, i)
    for _ in range(n):
        t = b[i]; i += 1
        if t == 0:
            name, i = az(b, i); o = struct.unpack_from('<I', b, i)[0]; i += 4
            p = az(b, o)[0]
            out.append('%sclass %s%s {' % (ind, name, (': ' + p) if p else '')); body(b, o, ind + '  ', out); out.append(ind + '};')
        elif t == 1:
            st = b[i]; i += 1; name, i = az(b, i)
            if st == 0: v, i = az(b, i); v = '"%s"' % v
            elif st == 1: v = '%g' % struct.unpack_from('<f', b, i)[0]; i += 4
            elif st == 2: v = str(struct.unpack_from('<i', b, i)[0]); i += 4
            else: v, i = az(b, i)
            out.append('%s%s = %s;' % (ind, name, v))
        elif t in (2, 5):
            if t == 5: i += 4
            name, i = az(b, i); v, i = arr(b, i)
            out.append('%s%s[] %s= %s;' % (ind, name, '+' if t == 5 else '', v))
        elif t == 3: name, i = az(b, i); out.append('%sclass %s;' % (ind, name))
        elif t == 4: name, i = az(b, i); out.append('%sdelete %s;' % (ind, name))
b = open(sys.argv[1], 'rb').read(); out = []
body(b, 16, '', out); print('\n'.join(out))
