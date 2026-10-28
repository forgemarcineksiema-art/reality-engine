"""Final integrity check and SHA-256 index for completed evidence."""
import hashlib
import json
from pathlib import Path
import shutil
import zipfile
root=Path('docs/verification/evidence/discriminating-2026-10-28')
technical=root/'technical'
old=technical/'final-docs.log'
if old.exists() and 'ERROR:' in old.read_text(encoding='utf-8'):
    shutil.copyfile(old,technical/'docs-before-manifest.log')
for p in Path('build/discrimination-final').iterdir():
    if p.is_file(): shutil.copyfile(p,technical/p.name)
scratch=Path('.superpowers/sdd/discriminating-implementation')
for p in scratch.iterdir():
    if p.is_file(): shutil.copyfile(p,technical/('plan-'+p.name+'.txt'))
for p in root.glob('*.zip'):
    with zipfile.ZipFile(p) as z:
        assert z.testzip() is None,p
files=[dict(path=p.relative_to(root).as_posix(),bytes=p.stat().st_size,
            sha256=hashlib.sha256(p.read_bytes()).hexdigest())
       for p in sorted(root.rglob('*')) if p.is_file() and p!=root/'manifest.json']
(root/'manifest.json').write_text(json.dumps(dict(algorithm='SHA-256',files=files),indent=2)+'\n')
for entry in files:
    p=root/entry['path']
    assert p.stat().st_size==entry['bytes']
    assert hashlib.sha256(p.read_bytes()).hexdigest()==entry['sha256']
print('PASS',len(files),'evidence files, all SHA-256/sizes; four ZIP CRC checks')
