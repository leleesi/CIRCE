"""Read this build's solid-LZMA, Unicode, x64 NSIS package and Win32 resources."""
from pathlib import Path
from struct import unpack_from, iter_unpack
import lzma
import re
import subprocess
import sys
from installer_startup import startup

project = Path(__file__).resolve().parents[1]
scratch, windres, sevenzip = sys.argv[1:]
scratch = Path(scratch)
signature = b'\xef\xbe\xad\xdeNullsoftInst'
setup = project / 'CIRCE-Setup.exe'


def unpack(binary):
    offset = binary.index(signature) - 4
    header_size = unpack_from('<I', binary, offset + 20)[0]
    payload = binary[offset + 28:]
    prop = payload[0]
    filters = [dict(id=lzma.FILTER_LZMA1, dict_size=unpack_from('<I', payload, 1)[0],
                    lc=prop % 9, lp=prop // 9 % 5, pb=prop // 45)]
    data = lzma.LZMADecompressor(format=lzma.FORMAT_RAW, filters=filters).decompress(payload[5:])
    records = []
    i = 0
    while i < len(data):
        size = unpack_from('<I', data, i)[0]
        assert i + size + 4 <= len(data)
        records.append(data[i+4:i+4+size])
        i += size+4
    assert len(records[0]) == header_size
    return bytearray(binary[:offset]), records


def uninstaller(stub, records):
    # WriteUninstaller applies (size, offset, bytes) icon patches to the setup stub.
    for patches, payload in zip(records, records[1:]):
        if payload[4:20] != signature:
            continue
        i = 0
        while i < len(patches):
            n = unpack_from('<I', patches, i)[0]
            if n == 0:
                assert i+4 == len(patches)
                return bytes(stub) + payload
            offset = unpack_from('<I', patches, i+4)[0]
            assert offset+n <= len(stub) and i+8+n <= len(patches)
            stub[offset:offset+n] = patches[i+8:i+8+n]
            i += 8+n
    raise AssertionError('Missing uninstaller')


class Instructions:
    def __init__(self, header):
        blocks = [unpack_from('<QI', header, 4+i*12) for i in range(6)]
        (entries, count), (strings, _), (tables, langs), (end, _) = blocks[2:6]
        self.code = [unpack_from('<7i', header, entries+i*28) for i in range(count)]
        self.strings = header[strings:tables].decode('utf-16-le')
        self.text = {}
        for i in range(tables, end, (end-tables)//langs):
            lang = unpack_from('<H', header, i)[0]
            values = iter_unpack('<i', header[i+10:i+(end-tables)//langs])
            self.text[lang] = [self.string(n) for n, in values]
        assert self.text.keys() == {1033,2070}

    def string(self, offset):
        return self.strings[offset:].partition('\0')[0] if offset >= 0 else ''


def selected_tag(program, language):
    # Interpret the comparison, assignments and jump linking the wizard choice to the marker.
    code, string = program.code, program.string
    pc = next(i for i, op in enumerate(code) if op[0] == 26 and string(op[2]) == '2070'
              and code[i+1][0] == 25 and string(code[i+1][2]) == 'pt')
    cmp = code[pc]
    jump = cmp[3 if language == 2070 else 4]
    assignment = code[jump-1 if jump else pc+1]
    assert assignment[0] == 25
    return assignment[1], string(assignment[2])


def marker(program, lang):
    var, tag = selected_tag(program, lang)
    op_at = next(i for i, op in enumerate(program.code)
                 if op[0] == 55 and program.string(op[4]).endswith('\\circe.lang'))
    write = program.code[op_at+2]
    assert write[0] == 56 and write[1] == program.code[op_at][1]
    fmt = program.string(write[2])
    assert fmt[0] == '\x03' and len(fmt) == 4 and fmt[2:] == '\r\n'
    packed = ord(fmt[1])
    assert (packed & 127) + ((packed >> 8 & 127) << 7) == var
    return (tag + fmt[2:]).encode('ascii')


def check_icons(binary, expected):
    res = scratch / 'icon-check.res'
    subprocess.run([windres,'-J','coff','-O','res',str(binary),str(res)],check=True)
    data = res.read_bytes()
    images, groups = {}, []
    i = 0
    while i < len(data):
        size, head, marker1, typ, marker2, idx = unpack_from('<IIHHHH',data,i)
        body = data[i+head:i+head+size]
        if marker1==marker2==65535 and typ==3:
            images[idx] = body
        if marker1==marker2==65535 and typ==14:
            groups.append(body)
        i += (head+size+3)&~3
    assert len(groups) == 1
    group = groups[0]
    count = unpack_from('<H',group,4)[0]
    actual = {}
    for i in range(count):
        w,h,_,_,_,_,size,idx = unpack_from('<BBBBHHIH',group,6+14*i)
        assert len(images[idx]) == size
        actual[w or 256, h or 256] = images[idx]
    assert actual == expected, binary


stub, records = unpack(setup.read_bytes())
unbytes = uninstaller(stub, records)
unexe = scratch/'uninstaller.exe'
unexe.write_bytes(unbytes)
subprocess.run([sevenzip,'t',str(unexe)],stdout=subprocess.DEVNULL,check=True)
wizard = Instructions(records[0])
unrecords = unpack(unbytes)[1]
removal = Instructions(unrecords[0])

# Start at the callback address from the binary, including on installs with a previous language.
scenarios = 0
for saved in ('', '2070', '1033', 'broken'):
    for oslang in (1033, 2070):
        for choice in ('2070', '1033', 'cancel'):
            expected = None if choice == 'cancel' else choice
            assert startup(wizard, records[0], saved, oslang, choice) == (expected, ['Choose'])
            scenarios += 1
        expected = saved if saved in ('2070','1033') else str(oslang)
        assert startup(wizard, records[0], saved, oslang, '', silent=True) == (expected, [])
        scenarios += 1
        expected = saved if saved in ('2070','1033') else '1033'
        expected_calls = [] if saved in ('2070','1033') else ['Choose']
        assert startup(removal, unrecords[0], saved, oslang, '1033') == (expected, expected_calls)
        scenarios += 1
for failure in ('load', 'open'):
    for choice in ('2070', '1033', 'cancel', 'error'):
        expected = choice if choice in ('2070','1033') else None
        assert startup(wizard, records[0], '1033', 2070, choice, dialog_failure=failure) == (
            expected, ['Choose', 'LangDialog'])
        scenarios += 1
print(f'OK: {scenarios} percursos do arranque NSIS compilado com respostas simuladas. Uma instalação interativa exige uma escolha.')

for archive, program in ((records, wizard), (unrecords, removal)):
    for file, member in (('installer-header.bmp','modern-header.bmp'), ('installer.bmp','modern-wizard.bmp')):
        assert (project/'assets'/file).read_bytes() in archive and member in program.strings
    assert (project/'bin/CIRCEIdioma.dll').read_bytes() in archive
    assert 'CIRCEIdioma.dll' in program.strings and 'Choose' in program.strings

# Verify the selector's language-neutral dialog, its button IDs and the exact DIB rendered by its static control.
plugin = project/'bin/CIRCEIdioma.dll'
dialog_res = scratch/'selector.res'
subprocess.run([windres,'-J','coff','-O','res',str(plugin),str(dialog_res)],check=True)
data = dialog_res.read_bytes()
i, art, dialog = 0, False, False
while i < len(data):
    size, head, m1, kind, m2, idx = unpack_from('<IIHHHH',data,i)
    body = data[i+head:i+head+size]
    if (m1, m2) == (65535,65535) and (kind,idx) in ((2,201),(5,101)):
        assert unpack_from('<H',data,i+22)[0] == 0  # LANG_NEUTRAL
        if kind == 2:
            assert body == (project/'assets/installer-language.bmp').read_bytes()[14:]
            art = True
        else:
            assert all(label.encode('utf-16le') in body for label in ('&Português','&English','CIRCE · Idioma / Language'))
            dialog = True
    i += (head+size+3)&~3
assert art and dialog
dialog_rc = scratch/'selector.rc'
subprocess.run([windres,'-J','coff','-O','rc',str(plugin),str(dialog_rc)],check=True)
template = dialog_rc.read_text()
assert re.search(r'CONTROL 201, 110, "STATIC", 0x5000004e', template)
assert re.search(r'PUSHBUTTON .*?, 2070,', template) and re.search(r'PUSHBUTTON .*?, 1033,', template)
print('OK: a janela inicial usa a imagem embutida no recurso 201. Imagens de cabeçalho e boas-vindas verificadas na instalação e remoção.')
for lang, tag in [(2070,'pt'),(1033,'en')]:
    assert marker(wizard, lang) == (tag+'\r\n').encode('ascii')
    assert selected_tag(removal,lang)[1] == tag
assert all('InstallerLanguage' in prog.strings for prog in (wizard,removal))
pairs = {}
for key, lang, line in re.findall(r'^LangString (\w+) \$\{LANG_(ENGLISH|PORTUGUESE)\} "(.*)"$',
                                  (project/'instalador/idiomas.nsh').read_text(), re.M):
    pairs.setdefault(key, {})[lang] = line
for key, pair in pairs.items():
    assert any((pair['ENGLISH'],pair['PORTUGUESE']) in zip(program.text[1033],program.text[2070])
               for program in [wizard,removal]), key

ico = (project/'assets/marca.ico').read_bytes()
expected = {}
for i in range(unpack_from('<H', ico, 4)[0]):
    w,h,_,_,_,_,size,offset = unpack_from('<BBBBHHII',ico,6+i*16)
    expected[w or 256,h or 256] = ico[offset:offset+size]
assert {w for w,h in expected if w == h} == {16,20,24,32,40,48,64,96,128,256}
for binary in [project/'bin/CIRCE.exe',setup,unexe]:
    check_icons(binary,expected)
print('OK: textos PT/EN nas tabelas compiladas, escolha de idioma e conteúdo de circe.lang nos dois percursos.')
print('OK: desinstalador reconstruído do pacote e verificado. Dez tamanhos do ícone coincidem na aplicação, na instalação e na remoção.')
