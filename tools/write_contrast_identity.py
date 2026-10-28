"""Stamp actual selected source bytes into the native experiment at build time."""
import sys
sys.dont_write_bytecode = True
import json
from pathlib import Path
from run_contrast_experiment import ROOT, source_files, hashes

target=Path(sys.argv[1])
identity=json.dumps(hashes(ROOT,source_files()),sort_keys=True,separators=(',',':'))
contents='#pragma once\ninline constexpr const char* contrast_source_hashes = R"IDENTITY('+identity+')IDENTITY";\n'
if not target.exists() or target.read_text(encoding='utf-8')!=contents:
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(contents,encoding='utf-8')
