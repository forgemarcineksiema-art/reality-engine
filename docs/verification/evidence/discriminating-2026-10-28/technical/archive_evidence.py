"""Byte-preserving evidence packaging; existing destinations are never overwritten."""
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

root=Path('docs/verification/evidence/discriminating-2026-10-28')
def pack(source,name):
    with zipfile.ZipFile(root/name,'x',zipfile.ZIP_DEFLATED,compresslevel=6) as out:
        for file in sorted(source.rglob('*')):
            if file.is_file(): out.write(file,file.relative_to(source).as_posix())
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
confirm=Path('build/contrast-confirmatory-01')
continuation=Path('build/contrast-continuation-final')
shutil.copyfile(confirm/'reality_contrast.exe',continuation/'reality_contrast.exe')
shutil.copyfile(confirm/'freeze.json',continuation/'freeze.json')
items={name:dict(ticks=ticks,checkpoint=sha(continuation/(name+'.save')),
                 expected=sha(continuation/(name+'.expected')),
                 actual=sha(continuation/(name+'.actual')))
       for name,ticks in [('configure',2),('pulse',1),('work',80),('contact',1)]}
(continuation/'checkpoint-manifest.json').write_text(json.dumps(items,indent=2)+'\n')
pack(Path('build/contrast-development-02'),'development-final.zip')
pack(confirm,'confirmatory.zip')
pack(continuation,'continuations.zip')
for name in ('analysis.json','diagnostics.json','prefix-diagnostics.json','teaching-examples.json',
             'learning-curves.png','learning-curves.svg','costs.csv','apparatus-audit.json','freeze.json'):
    shutil.copyfile(confirm/name,root/name)
technical=root/'technical'
technical.mkdir(exist_ok=True)
for file in Path('build/discrimination-final').iterdir():
    if file.is_file(): shutil.copyfile(file,technical/file.name)
print({p.name:p.stat().st_size for p in root.glob('*.zip')})
