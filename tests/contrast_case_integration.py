"""Native schema, independent replay, and hostile mutations; stdlib only."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import contrast_judge as judge
from run_contrast_experiment import case_text

case = json.loads((Path(__file__).parent/'data/contrast/development-case.json').read_text())
with tempfile.TemporaryDirectory() as temporary:
    root=Path(temporary)
    (root/'case.txt').write_text(case_text(case))
    subprocess.run([sys.argv[1],str(root/'case.txt'),str(root/'raw.json')],check=True,timeout=15)
    raw=json.loads((root/'raw.json').read_text())
    judge.judge(case,raw)
    for change in ('learned','missing_contact','remote_contact','negative_cpu','wrong_energy','teacher_cpu'):
        modified=copy.deepcopy(raw)
        b=modified['A']['social'][1]
        if change=='learned':
            for e in b['received']: e['learned']=e['measurement']['tick']+1
        elif change=='missing_contact': del b['contact_tick']
        elif change=='remote_contact': b['contact_positions'][0]=-100
        elif change=='negative_cpu': b['cpu_ms']=-1
        elif change=='wrong_energy': b['energy']+=1
        else: modified['A']['cpu_ms']=-1
        try:
            judge.judge(case,modified)
        except ValueError:
            continue
        raise AssertionError('mutation accepted: '+change)
print('PASS strict native case, physical contact proof, costs, six hostile mutations')
