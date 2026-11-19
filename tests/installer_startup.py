"""Exercise only the language branch of the compiled NSIS x64 .onInit callback."""
from struct import unpack_from


def startup(program, header, saved, system, reply, silent=False, dialog_failure=None):
    # NSIS 3.11 amd64 exehead 'header': first callback index after blocks and UI settings.
    pc = unpack_from('<i', header, 140)[0]
    registers = {24: str(system)}  # NSIS $LANGUAGE
    stack, calls = [], []

    def value(offset):
        text = program.string(offset)
        i = text.find('\x03')
        while i >= 0:
            packed = ord(text[i+1])
            key = (packed & 127) | ((packed >> 8 & 127) << 7)
            text = text[:i] + registers.get(key, '') + text[i+2:]
            i = text.find('\x03')
        return text

    for _ in range(160):
        op, a, b, c, d, _, _ = program.code[pc]
        if op == 52:  # ReadRegStr
            assert program.string(d) == 'InstallerLanguage'
            registers[a] = saved
        elif op == 26:  # StrCmp
            target = c if value(a) == value(b) else d
            if target:
                pc = target-1
                continue
        elif op == 25:  # StrCpy
            registers[a] = value(b)
        elif op == 2:   # Goto
            pc = a-1
            continue
        elif op == 14:  # IfSilent
            assert c == 8
            target = a if silent else b
            if target:
                pc = target-1
                continue
        elif op == 31:  # Push / Pop
            if b:
                registers[a] = stack.pop() if stack else ''
            else:
                stack.append(value(a))
        elif op == 44:  # Plugin call. Model dialog replies and OS setup prerequisites.
            plugin = program.string(b)
            if plugin == 'Choose':
                calls.append(plugin)
                assert stack[-1] == 'preferred-' + registers[24]
                if dialog_failure != 'load':
                    stack.pop()
                    stack.append('error' if dialog_failure == 'open' else reply)
            elif plugin == 'LangDialog':
                calls.append(plugin)
                assert {'Português','English','2070','1033'} <= set(stack)
                stack.clear()
                stack.append(reply)
            elif plugin == 'Call':
                assert any('CreateMutexW' in s for s in stack)
                return registers[24], calls
            else:
                raise AssertionError(plugin)
        elif op == 3:  # Abort: no compatibility check, file write or launch could run.
            return None, calls
        elif op == 22:  # Bilingual error box if neither dialog produced a language.
            pass
        elif op not in (5,13,20):  # InitPluginsDir, flags, extract bundled DLL to the plugin directory.
            raise AssertionError((pc,op))
        pc += 1
    raise AssertionError('language initialization did not terminate')
