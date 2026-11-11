#!/usr/bin/env python3
"""Відтворювана інвентаризація; не замінює компіляцію Enfusion."""
import pathlib,zipfile,hashlib,json,re,collections
ROOT=pathlib.Path(__file__).resolve().parents[1]
def digest(data): return hashlib.sha256(data).hexdigest()
archives=[]; indexes={}; egc=[]
for p in sorted(pathlib.Path('/tmp/codex-remote-attachments').rglob('*.zip')):
 with zipfile.ZipFile(p) as z:
  entries=[]; classes=[]
  for i in z.infolist():
   if i.is_dir(): continue
   data=z.read(i); entries.append({'path':i.filename,'bytes':len(data),'sha256':digest(data)})
   local=ROOT/'research'/p.stem/i.filename
   if not local.is_file() or digest(local.read_bytes())!=digest(data): raise RuntimeError(f'Changed: {local}')
   if i.filename.endswith('.c'):
    s=data.decode('utf-8-sig',errors='replace')
    # Евристичний індекс: коментарі видаляємо перед пошуком визначень.
    clean=re.sub(r'/\*.*?\*/|//[^\n]*','',s,flags=re.S)
    defs=re.findall(r'\b(modded\s+)?class\s+(\w+)(?:\s*:\s*(\w+))?',clean)
    classes.extend({'file':i.filename,'name':n,'base':b,'modded':bool(m)} for m,n,b in defs)
    if 'EGC_scripts' in p.name:
     egc.append({'file':i.filename,'classes':[n for _,n,_ in defs], 'references':sorted(set(re.findall(r'\bEGC_\w+',clean))), 'profile_paths': sorted(set(re.findall(r'\$profile:[^"\n]*',clean)))})
  archives.append({'name':p.name,'sha256':digest(p.read_bytes()),'files':len(entries),'bytes':sum(x['bytes'] for x in entries),'extensions':dict(collections.Counter(pathlib.Path(x['path']).suffix for x in entries)), 'license_candidates':[e['path'] for e in entries if re.search(r'(^|/)(license|copying|notice|readme)',e['path'],re.I)]})
  indexes[p.stem]={'files':entries,'classes':classes}
(ROOT/'docs/archive_manifest.json').write_text(json.dumps({'archives':archives,'indexes':indexes},ensure_ascii=False,indent=2))
(ROOT/'docs/egc_dependencies.json').write_text(json.dumps(egc,ensure_ascii=False,indent=2))
lines=['# Індекс EGC','', 'Повний перелік 84 скриптів; зв’язки за посиланнями — евристика, не граф викликів.','']
for f in egc:
 lines += ['## '+f['file'],'','Класи: '+', '.join(f['classes']),'','Посилання EGC: '+', '.join(f['references']),'']
(ROOT/'docs/EGC_INDEX.md').write_text('\n'.join(lines))
for a in archives: print(a['name'],a['files'],a['extensions'],a['license_candidates'])
print('PASS: all extracted files match archive bytes')
