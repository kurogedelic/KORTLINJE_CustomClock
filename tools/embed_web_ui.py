#!/usr/bin/env python3
"""Copy the standalone index.html into the ESP32 firmware header."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
html = (root / 'web' / 'index.html').read_text(encoding='utf-8')
if ')KORTLINJE"' in html:
    raise SystemExit('raw-string delimiter occurs in index.html')
header = root / 'firmware' / 'KORTLINJE_C3' / 'WebUi.h'
header.write_text(
    '#pragma once\n#include <Arduino.h>\n\n'
    '// Embedded copy of ../../web/index.html. Keep both files identical.\n'
    'static const char KORTLINJE_WEB_UI[] PROGMEM = R"KORTLINJE(\n'
    + html + ')KORTLINJE";\n', encoding='utf-8')
print(header.relative_to(root))
