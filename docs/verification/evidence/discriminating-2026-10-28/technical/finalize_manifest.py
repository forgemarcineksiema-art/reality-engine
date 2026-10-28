"""Finalize preserved rebuild and verification records without changing raw runs."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile
root=Path('docs/verification/evidence/discriminating-2026-10-28')
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
rebuild=Path('build/contrast-rebuild/RelWithDebInfo/reality_contrast.exe')
with zipfile.ZipFile(root/'rebuilt-program.zip','x',zipfile.ZIP_DEFLATED) as out:
    out.write(rebuild,'reality_contrast.exe')
    for name in ('CMakeCache.txt','CMakeFiles/4.4.4/CMakeCXXCompiler.cmake'):
        out.write(Path('build/contrast-rebuild')/name,Path(name).name)
    for name in ('configure','pulse','work','contact'):
        out.write(Path('build/contrast-continuation-final')/(name+'.rebuilt'),name+'.rebuilt')
technical=root/'technical'
for p in Path('build/discrimination-final').iterdir():
    if p.is_file(): shutil.copyfile(p,technical/p.name)
files=[dict(path=p.relative_to(root).as_posix(),bytes=p.stat().st_size,sha256=sha(p))
       for p in sorted(root.rglob('*')) if p.is_file() and p.name!='manifest.json']
(root/'manifest.json').write_text(json.dumps(dict(algorithm='SHA-256',files=files),indent=2)+'\n')
print('manifest',len(files),'files',sum(p['bytes'] for p in files),'bytes')
