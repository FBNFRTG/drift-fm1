#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""After build.sh and web/emu/build.sh: assemble the self-hosted DRIFT release."""
from pathlib import Path
import hashlib,html,json,shutil,zipfile
SRC=Path(__file__).resolve().parents[1]
OUT=SRC/'build'/'drift-0.8-website'
SITE=OUT/'upload'
VERSION='0.8';PRODUCT='FM-1_983';BASE='https://www.fbnfrtg.de/tools/fm1/drift/'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def zip_tree(path,root,prefix='',skip=()):
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED) as z:
  for p in sorted(root.rglob('*')):
   if p.is_file() and not any(x in p.relative_to(root).parts for x in skip):z.write(p,prefix+str(p.relative_to(root)))
if OUT.exists():shutil.rmtree(OUT)
SITE.mkdir(parents=True)
shutil.copytree(SRC/'web/site',SITE,dirs_exist_ok=True)
shutil.copytree(SRC/'build/emu',SITE/'webplayer',dirs_exist_ok=True)
# Remove any firmware package inherited from a previous build.
for p in (SITE/'webplayer/install').glob('*.fwsc'):p.unlink()
DL=SITE/'downloads';DL.mkdir()
fw=DL/f'drift-fm1-{VERSION}.fwsc';shutil.copy2(SRC/'build/felucca.fwsc',fw)
assert b'FELUCCA-LOADER-1' in fw.read_bytes()
shutil.copy2(fw,SITE/'webplayer/install'/fw.name)
m={'version':VERSION,'product':PRODUCT,'bytes':fw.stat().st_size,'sha256':sha(fw)}
(SITE/'webplayer/install/manifest.json').write_text(json.dumps(m,indent=2)+'\n')
for p in SITE.glob('*.html'):p.write_text(p.read_text().replace('@FW_BYTES@',str(m['bytes'])).replace('@FW_SHA@',m['sha256']))
DOC=SITE/'docs';DOC.mkdir()
for name in ['README.md','README-DRIFT.md','PUBLISHING-DE.md','BUILDING.md','CHANGES-DRIFT.md','LICENSING.md','CREDITS.md']:shutil.copy2(SRC/name,DOC/name)
LIC=SITE/'licenses';shutil.copytree(SRC/'LICENSES',LIC,dirs_exist_ok=True);shutil.copy2(SRC/'LICENSE',LIC/'GPL-3.0.txt')
(LIC/'index.html').write_text('<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>DRIFT — Licence texts</title><link rel="stylesheet" href="../assets/style.css"><main class="wrap prose"><div class="subhero"><a href="../licenses.html">← Licences & credits</a><h1>Licence texts.</h1></div><ul>'+''.join('<li><a href="'+p.name+'">'+html.escape(p.name)+'</a></li>' for p in sorted(LIC.glob('*')) if p.is_file() and p.name!='index.html')+'</ul></main></html>')
lp=LIC/'index.html'
lp.write_text(lp.read_text().replace('</main>', '<p><a href="https://www.fbnfrtg.de/impressum/">Impressum / Legal notice</a></p></main>'))
sourcezip=DL/f'drift-{VERSION}-source.zip'
# Include the complete checked-in source, engines and sample assets; exclude build output.
with zipfile.ZipFile(sourcezip,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(SRC.rglob('*')):
  rel=p.relative_to(SRC)
  if p.is_file() and not any(x in rel.parts for x in ('build','.git','__pycache__','.DS_Store')) and not p.name.endswith(('.log','.f32','.pyc')) and not p.name.startswith('drift-tests-'):z.write(p,f'drift-{VERSION}-source/'+str(rel))
manifest={'name':'DRIFT','version':VERSION,'date':'2026-10-23','author':'Fabian Freitag','device':'M-VAVE FM-1','identity':PRODUCT,'loader_identity':'ota-'+PRODUCT,'license':'GPL-3.0-only','homepage':BASE,'play':BASE+'webplayer/','installer':BASE+'webplayer/install/','firmware':{'url':BASE+'downloads/'+fw.name,'bytes':m['bytes'],'sha256':m['sha256']},'source':{'url':BASE+'downloads/'+sourcezip.name,'bytes':sourcezip.stat().st_size,'sha256':sha(sourcezip)},'image':BASE+'assets/drift-sequence.png','hardware_test_status':'pending for exact release 0.8','software_test_status':'passed for unchanged 0.8 binaries; see validation logs in complete bundle','compatibility':'DRIFT 0.7 patch and sample layout retained. Other firmware data is not guaranteed compatible.','hub_status':'not submitted; no approval claimed'}
(SITE/'release.json').write_text(json.dumps(manifest,indent=2)+'\n')
(SITE/'HUB-INFO.txt').write_text(f'''DRIFT — self-hosted release information
Author: Fabian Freitag
Version: {VERSION} (open beta), 23 October 2026
Device: M-VAVE FM-1
Identity: {PRODUCT}
Loader: ota-{PRODUCT}, Felucca loader included
USB port names: Felucca / Felucca Update
License: GPL-3.0-only, upstream notices preserved

DRIFT is an ambient instrument for live sessions, with classic synth sounds, pads, plucks and a repeating generative Euclidean sequencer. It offers two oscillators, ADSR, scales, an assignable LFO, three user sample slots and multiple delay/reverb types.

Project: {BASE}
Release metadata: {BASE}release.json
Firmware: {BASE}downloads/{fw.name}
SHA-256: {m['sha256']}
Complete corresponding source: {BASE}downloads/{sourcezip.name}
Browser player: {BASE}webplayer/
Installer: {BASE}webplayer/install/
Sample uploader: {BASE}webplayer/samples/
Manual: {BASE}manual.html
Screen 240x240: {BASE}assets/drift-sequence.png
Wide image: {BASE}assets/drift-cover.png
License and credits: {BASE}licenses.html

Repository: https://github.com/FBNFRTG/drift-fm1
Releases: https://github.com/FBNFRTG/drift-fm1/releases
Attach the firmware .fwsc to a GitHub release after hardware tests.

FM-1_983 was not present in the Hub catalogue checked on 23 October 2026; it is assigned here to DRIFT, not centrally reserved.

Test status: software build, synthesis, persistence and simulated update/loader tests pass. Physical installation from V15 and another firmware, outgoing updates and recovery for this exact package are still pending. No successful hardware test or Hub approval is claimed. Complete these hardware checks before submission.

Data: no patch/sample layout changes from DRIFT 0.7. Cross-firmware data compatibility is not guaranteed.
''')
# A release bundle contains source, binaries, usable web tools, manual and validation.
complete=DL/f'drift-{VERSION}-complete.zip'
with zipfile.ZipFile(complete,'w',zipfile.ZIP_DEFLATED) as z:
 z.write(fw,'firmware/'+fw.name);z.write(SRC/'build/felucca.bin','firmware/drift-fm1-0.8.bin')
 with zipfile.ZipFile(sourcezip) as s:
  for name in s.namelist():z.writestr('source/'+name.split('/',1)[1],s.read(name))
 for p in sorted(SITE.rglob('*')):
  if p.is_file() and 'downloads' not in p.relative_to(SITE).parts:
   target='website/'+str(p.relative_to(SITE))
   if p.suffix=='.html':z.writestr(target,p.read_text().replace('downloads/'+complete.name,BASE+'downloads/'+complete.name))
   else:z.write(p,target)
 for p in (fw,sourcezip):z.write(p,'website/downloads/'+p.name)
 z.writestr('website/downloads/SHA256SUMS.txt',''.join(f'{sha(p)}  {p.name}\n' for p in (fw,sourcezip)))
 for p in sorted(SRC.glob('drift-*')):
  if p.is_file() and p.suffix in ('.txt','.log'):z.write(p,'validation/'+p.name)
 z.writestr('README.txt','DRIFT 0.8. Firmware: firmware/drift-fm1-0.8.fwsc. Complete buildable source: source/. The full uploadable project website is supplied separately as drift-0.8-website.zip. Browser tools in website/webplayer/ require an HTTP server or HTTPS hosting; open website/manual.html for the manual. See source/BUILDING.md and source/README-DRIFT.md.\n')
(DL/'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.name}\n' for p in (fw,sourcezip,complete)))
# Self-hosted Apache support, scoped to this folder. No third-party traffic required.
(SITE/'.htaccess').write_text('''DirectoryIndex index.html
<IfModule mod_mime.c>
 AddType application/wasm .wasm
 AddType application/octet-stream .fwsc
</IfModule>
<IfModule mod_headers.c>
 <FilesMatch "\\.(html|json|js|wasm|fwsc)$">
  Header set Cache-Control "no-cache"
 </FilesMatch>
</IfModule>
''')
(OUT/'START-HERE-DE.txt').write_text('''DRIFT 0.8 — Website hochladen

1. Den INHALT des Ordners upload in /tools/fm1/drift/ auf deinem Webspace kopieren.
   Nicht den Ordner upload selbst hochladen. index.html muss direkt unter /tools/fm1/drift/ liegen.
2. Vorhandene gleichnamige Dateien überschreiben. Den gesamten Ordner webplayer ebenfalls ersetzen,
   damit Firmware, Wasm, Installer und Manifest zusammenpassen. Nicht nur index.html austauschen.
3. https://www.fbnfrtg.de/tools/fm1/drift/ öffnen, ggf. Browsercache leeren.
   Der bisherige Link /tools/fm1/drift/webplayer/ bleibt erhalten.
4. Installer: /tools/fm1/drift/webplayer/install/
   Download: /tools/fm1/drift/downloads/drift-fm1-0.8.fwsc
   Identität: FM-1_983. Loader: ota-FM-1_983. Versionsnummer: 0.8.

Die Website benötigt keine Datenbank, kein npm und kein PHP. HTTPS ist für Web-MIDI notwendig.
Die optionale .htaccess legt index.html als Startdatei und den WASM-MIME-Typ fest.
Falls dein Hoster .htaccess-Anweisungen ablehnt (HTTP 500), .htaccess entfernen und
application/wasm als MIME-Typ für .wasm im Hosting-Panel setzen lassen.

Vor der Hub-Einreichung die neue 0.8-Datei auf dem realen FM-1 testen:
Installation aus V15, Installation aus anderer Firmware, USB-Identität FM-1_983,
Wechsel von DRIFT zu anderer Firmware, Wiederaufnahme/Recovery. Vorher Daten sichern.
Die Website kennzeichnet diese Hardwaretests ehrlich als ausstehend.
Wenn sie bestanden sind, Teststatus in web/site/release.html und tools/package_drift.py
im Quellcode aktualisieren und das Website-Paket neu erzeugen; alternativ die Texte
in upload/release.html, upload/index.html, upload/release.json und upload/HUB-INFO.txt anpassen.

Für den Firmware Hub: Angaben in HUB-INFO.txt nach den Tests an zven@thefirmwarehub.com schicken.
Keine Nachricht wurde automatisch gesendet. Das GitHub-Repository ist https://github.com/FBNFRTG/drift-fm1 . Die .fwsc muss als Release-Anhang veröffentlicht werden.
Die Kennung war am 23.10.2026 im Hub-Katalog nicht belegt; keine zentrale Reservierung.

Speicherlayout unverändert gegenüber DRIFT 0.7. Andere Firmwares können Nutzerdaten
anders speichern oder überschreiben. Der Identitätswechsel allein verschiebt keine Patches oder Samples.

Dateien aus früheren Releases dürfen als Archiv erhalten bleiben. Neue Installationen
verwenden ausschließlich das neue Manifest und die zugehörige Firmware.
''')
zip_tree(SRC/'build/drift-0.8-website.zip',OUT)
print(SRC/'build/drift-0.8-website.zip')
print(json.dumps(m))
