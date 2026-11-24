#!/usr/bin/env python3
"""Reject unsafe Unicode literals, broken encodings and common mojibake in C++."""
from pathlib import Path
import re
import sys
import unicodedata

ROOT = Path(__file__).resolve().parents[1]
LITERAL = re.compile(r'(?P<prefix>u8|u|U|L)?"(?:\\.|[^"\\\n])*"')
# Common UTF-8 decoded as Latin-1/Windows-1252, replacement character, and C1 controls.
BAD = re.compile(r'[\uFFFD\u0080-\u009F]|\u00C3[\u0080-\u00BF]|\u00C2[\u0080-\u00BF]|\u00E2(?:\u20AC|\u0080)|\u00F0\u0178')
ESCAPED = re.compile(r'\\(?:u[0-9a-fA-F]{4}|U[0-9a-fA-F]{8}|x[0-9a-fA-F]{2,}|[2-7][0-7]{2})')
APPROVED = re.compile(r'\bLIBERTY_UTF8\s*\(\s*$')

def inspect(text):
    issues = []
    for m in BAD.finditer(text):
        issues.append((text.count('\n', 0, m.start()) + 1, "possible mojibake / replacement character"))
    for m in LITERAL.finditer(text):
        token = m.group()
        if not (any(ord(c) > 127 for c in token) or ESCAPED.search(token)):
            continue
        if m.group('prefix') != 'u8' or not APPROVED.search(text[max(0, m.start()-100):m.start()]):
            issues.append((text.count('\n', 0, m.start()) + 1, "use LIBERTY_UTF8(u8\"...\") for Unicode text"))
        if unicodedata.normalize('NFC', token) != token:
            issues.append((text.count('\n', 0, m.start()) + 1, "use NFC-normalized Unicode text"))
    return issues

def self_test():
    accent = chr(0xE9)
    broken = chr(0xC3) + chr(0xA9)
    assert inspect('g.drawText("R' + accent + 'glage");')
    assert not inspect('g.drawText(LIBERTY_UTF8(u8"R' + accent + 'glage"));')
    assert inspect('LIBERTY_UTF8(u8"' + broken + '")')
    assert inspect('LIBERTY_UTF8(u8"' + chr(0xFFFD) + '")')
    assert inspect(r'juce::String("\u00e9")')
    assert not inspect('setParam("output", 0.5f);')

def main():
    self_test()
    failed = False
    for f in sorted((ROOT / 'Source').rglob('*')):
        if f.suffix.lower() not in ('.cpp', '.h', '.hpp', '.cc', '.cxx', '.mm'):
            continue
        try:
            text = f.read_bytes().decode('utf-8', errors='strict')
        except UnicodeDecodeError as e:
            print(f'{f.relative_to(ROOT)}: invalid UTF-8: {e}')
            failed = True
            continue
        for line, message in inspect(text):
            print(f'{f.relative_to(ROOT)}:{line}: {message}')
            failed = True
    if failed:
        return 1
    print('Liberty Unicode guard: PASS')
    return 0

if __name__ == '__main__':
    sys.exit(main())
