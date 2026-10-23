# DRIFT 0.8 — FM-1 Ambient Synth

Eine Spur, acht Stimmen, 21 Factory-Patches, feste generative Patterns und drei
Sample-Slots. Firmware und Webplayer verwenden denselben C-Code für Audio und Display.

## Starten / installieren

Webplayer: Start audio → PLAY. Für eine lokale Kopie im entpackten Hauptordner
`python3 -m http.server 8000` starten; dann `http://localhost:8000/webplayer/` öffnen.
AudioWorklet und Web MIDI benötigen HTTPS oder localhost, nicht file://.

Unter **Install firmware** kann das mitgelieferte DRIFT-0.8-Paket per USB-MIDI auf
FM-1 installiert werden. Chrome oder Edge verwenden, Daten vorher sichern, andere
MIDI-Apps und den Sample-Uploader schließen. Der Installer prüft SHA-256, Dateigröße
und Gerätemodell, führt Loader- und Firmware-Transfer aus und prüft die zurückkehrende
Gerätekennung. USB und Strom währenddessen verbunden lassen. Bei unterbrochenem
Loader-Transfer erneut verbinden und Install drücken; der Installer unterstützt Resume.
Ein Recovery-Link führt zum Felucca-Installer mit Rückkehr zur offiziellen Firmware.

**Entwicklungsstand:** Hardware-Firmware und WebAssembly erfolgreich gebaut.
Native Firmware-, Audio-, Sample- und simulierte OTA-Protokolltests durchgeführt.
Kein physischer FM-1 wurde hier geflasht; USB-Installation und Sample-Upload sind
noch nicht am Gerät getestet. Die vollständige Browser-Bedienung wurde nicht
in einem echten Browser automatisiert geprüft. Vor Einsatz am Gerät zuerst den
Webplayer ausprobieren. Backup-/Recovery-Hinweise auch in BUILDING.md.

## Bedienung

| Taste / Regler | Funktion |
| --- | --- |
| HOME | PATCHES: Patchliste, Favoriten zusätzlich oben. Erneut drücken: Löschen bestätigen. |
| PATCHES | Patch wählen. |
| REC | Favoriten-Herz setzen / entfernen. |
| SELECT | BPM / Tempo, auf allen normalen Seiten. |
| ALGORITHM | Low-Pass-Cutoff, mit grafischer Filterkurve. |
| GLO | OSCILLATOR 1: Wave, Detune, Mix, Level. Erneut drücken: OSCILLATOR 2 mit Wave (OFF zuerst), Tune (±24 Halbtöne), Level. |
| EDIT | FILTER: Cutoff, Resonance, Envelope, Drive, mit Balken. |
| ENV | ENVELOPE: ADSR mit Hüllkurvenzeichnung. |
| LFO | MODULATION: Rate, Filter, Target, Depth. Referenzwelle, Filter- und Zielkurve mit Phasenmarker. |
| FX | EFFECTS / SPACE / FX TYPES; drei Seiten, Werte und vertikale Balken. |
| SCL | SCALE: Root, Scale, Min, Max. Grundton und ausgeschriebene Skala groß. Erneut drücken: LED und Scale only. |
| ARP | SEQUENCE 1: Bars, Notes, Duration, Probability. |
| SEQ | SEQUENCE 2: Direction, Interval, Chaos, Time. |
| KNOB 1–4 | Die vier nebeneinander angezeigten Werte. |
| PLAY | Loop starten / stoppen. |
| SAVE | Gestoppt: Patch speichern. Bei gleichem Namen Überschreiben bestätigen; ein neuer Name belegt einen freien Benutzerplatz. |
| OCT− / OCT+ | Oktave der live gespielten Klaviatur, ±3. LED: ±1 schwach, ±2 mittel, ±3 hell; OCT 0 beide aus. Anzeige unten rechts. |
| MASTER | Ausgangslautstärke. |

SELECT und ALGORITHM zeigen ihren Wert groß. 500 ms nach dem letzten Drehimpuls
kehrt das Display zur bisherigen Seite zurück. SELECT regelt 40–240 BPM; ALGORITHM
setzt den Filter explizit auf Low-Pass. Reverb bleibt unter FX einstellbar.
Die Filterkurve zeigt Cutoff und Resonanz schematisch, keine gemessene FFT.

Die Patchliste zeigt sechs Zeilen. Die größeren unteren Grafiken nutzen die Fläche
bis zur Fußzeile. GLO zeigt eine schematische Oszillatorwelle einschließlich Mix,
Detune und Level; SAMPLE 1–3 zeigen die dekodierte Sample-Peakhüllkurve mit Pegel.
Die Oszillatorgrafik ist eine statische Vorschau, kein Live-Oszilloskop.

Die Seite steht oben links, Seitenpaare zeigen mittig 1/2 bzw. 2/2; FX zeigt 1/3, 2/3, 3/3. BPM bleibt oben rechts.
HOME-Knobs behalten Cutoff, Resonanz, Attack, Release. Im SAVE-Dialog gelten dessen
Namensfunktionen: Knob 1 Cursor, Knob 2 Zeichen, SAVE/OCT+ speichern, HOME/OCT− abbrechen.

## Sequencer

**Ein Loop bleibt identisch, bis SCL oder SEQUENCE verändert wird.** Wahrscheinlichkeit
wird einmal beim Erzeugen angewendet, nicht bei jedem Durchlauf. Stop/Start würfelt
nicht neu. Klangmodulation und Effektausklang entwickeln sich weiter.

Parameteränderungen regenerieren das Pattern am laufenden Abspielkopf. Dieser wird
nicht auf null gesetzt. Alte gehaltene Sequencer-Noten werden freigegeben; neue
Ereignisse folgen ab der aktuellen Position. Wird die Loop-Länge verkürzt, wird die
Position in den neuen Loop umgerechnet (Modulo), ohne Transport-Neustart.

### SEQUENCE 1 — ARP

- BARS: 1–16 Takte in 4/4; Standard 4.
- NOTES: 1–32 Ereignisse, maximal 16 je eingestelltem Takt.
- DUR /16: 1–64 Sechzehntel pro Note.
- PROB: 0–100 %, Standard 100. Pausen wiederholen sich exakt.

Die Grundverteilung ist euklidisch auf einem Sechzehntelraster. Falls gleiche Abstände
nicht möglich sind, unterscheiden sie sich maximal um einen Rasterschritt.

### SEQUENCE 2 — SEQ

| Knob | Werte / Wirkung |
| --- | --- |
| 1 DIR | 0 ROOT; 1–7 UP 1–7; 8–14 DN 1–7; 15–21 UD 1–7. Anzahl der Skalenstufen in aufsteigenden, absteigenden oder wechselnden Läufen. |
| 2 INTERVAL | 0 Grundabstand; 1 Sekunde, 2 Terz, 3 Quarte, 4 Quinte, 5 Sexte, 6 Septime, 7 Oktave. Intervalle folgen der Skala. |
| 3 CHAOS | 0–100. Deterministische Durchmischung der Notenfolge; 100 mischt die ganze Folge. |
| 4 TIME | 0–100. Zunächst Swing, dann unregelmäßige Verschiebungen, oben starke zeitliche Cluster. |

Bei allen vier Nullwerten spielt ausschließlich die Rootnote, in der zulässigen
Oktave möglichst nahe C4. Enthält Min/Max keine Rootnote, bleibt das Pattern stumm.
DIR 0 mit INTERVAL > 0 erzeugt einen kurzen aufsteigenden Intervalllauf. CHAOS allein
kann identische Rootnoten nicht klanglich unterscheiden; erst DIR oder INTERVAL
liefert verschiedene Tonhöhen. Alle Noten bleiben innerhalb von Skala und Min/Max;
an Grenzen werden Läufe begrenzt. Extreme TIME-Werte dürfen Ereignisse bündeln.

## Drei Samples

**SAMPLE 1–3**, je 80 KiB, höchstens 162816 Frames = ca. **7,384 Sekunden**, mono,
22.050 Hz, intern 4-Bit-IMA-ADPCM. Der englische Uploader konvertiert unterstützte
Audiodateien lokal; WAV empfohlen. Stereo wird gemittelt. Zu lange Dateien werden
abgelehnt. Rootnote gibt die Originaltonhöhe an (60 = C4); Loop wiederholt die Datei.

Ziel Webplayer: speichern, Player neu laden, GLO → WAVE → SAMPLE 1–3.
Ziel FM-1: DRIFT 0.8 installieren, MIDI-Zugriff erlauben, Ein-/Ausgang wählen,
Check device, dann Save/Replace. Während der Übertragung stoppt die Wiedergabe.
Ein abgebrochener Upload kann den ausgewählten Slot unvollständig lassen; erneut laden.

**Migration von 0.2:** Drei größere Slots ersetzen sechs kleinere. Vorher sichern und
Samples neu laden. Alte Sampleheader werden absichtlich nicht interpretiert. Die
Patchbank liegt außerhalb dieses Bereichs. Vorhandene SEQUENCE-2-Patchwerte haben
jetzt eine neue Bedeutung; diese Einstellungen nach dem Update kontrollieren.

## Quellcode und Tests

Vollständiger geänderter Quellcode einschließlich Engines liegt im Repository.
Toolchain/SDK nach BUILDING.md bereitstellen, dann im Repository-Hauptordner:

```sh
JIELI_TOOLCHAIN=/pfad/toolchain AC79_SDK=/pfad/sdk sh build.sh
# Emscripten aktivieren
EMU_PREVIEW=0 sh web/emu/build.sh
node web/samples/test.mjs
cc -O2 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src -o build/drift-test tests/drift_test.c -lm
build/drift-test
node web/emu/drift_test.mjs
node web/install/test.mjs
```

Das Installer-Manifest und seine Firmwaredatei werden beim Verpacken aus dem
geprüften Target-Build erzeugt. Testprotokolle im validation/-Ordner.

GPL-3.0-only; mitgelieferte Engine-, Schrift- und SDK-Lizenzen beachten.
Felucca: Leo Kuroshita / Hügelton Instruments. Browser-Port nach charlesvestal/X0X.
Die Pattern-Algorithmen sind eigene Implementierungen, keine übernommenen Arturia-
oder Harmony-Bloom-Algorithmen. Keine Verbindung zu den Geräteherstellern.

## Ergänzung: WARM SUB

Warmes monophones Subbass-Patch mit dominantem Suboszillator eine Oktave unter
dem Hauptoszillator, ohne Detune, tiefem Low-Pass und sanfter Sättigung. Wenig Hall,
kein Chorus/Delay; kurze Attack und kontrollierter Release. Der Sequencer nutzt
MIDI 24–47. Bestehende Patchnummern bleiben erhalten; WARM SUB steht am Listenende.
BPM bleibt oben rechts über der Trennlinie.

## Neu in 0.4

Standardtempo: 80 BPM. Auf SCL 2 beleuchtet LED alle Tasten der aktuellen Skala; Scale only sperrt skalenfremde Live-Tasten und MIDI-Noten. Beide Optionen werden im Patch gespeichert. GLO 2 hat keinen On/Off-Regler: Die erste WAVE ist OFF, danach folgen SAW, SQUARE, TRI, SINE, PULSE. Knob 2 stimmt ±24 Halbtöne, Knob 3 regelt LEVEL; Knob 4 ist frei. Beide Oszillatoren teilen Filter, ADSR und Effekte. Automatische Pegelanpassung verhindert Übersteuerung beim Mischen. TWIN HORIZON, FIFTH LIGHT und DEEP PAIR nutzen beide Oszillatoren. Vorhandene Patches starten mit Oszillator 2 auf OFF.

Display: vollständiger 4-Bit-Graustufen-Bildpuffer; nur geänderte 8-Zeilen-Streifen werden übertragen. Kein sichtbares Löschen vor dem Neuzeichnen. Der Hardware-Build und Emulator prüfen die Änderung; die tatsächliche Flackerfreiheit muss noch auf dem FM-1 bestätigt werden.

## Neu in 0.5

LFO: Knob 1 RATE, Knob 2 FILTER (unabhängige Intensität), Knob 3 TARGET, Knob 4 DEPTH. Ziele: VOLUME (Tremolo), DELAY (Delay-Send um den FX-Grundwert), O2 FREQ (Tonhöhe von Oszillator 2, bis ±12 Halbtöne), O2 DET (feine Verstimmung von Oszillator 2, bis ±50 Cent). DEPTH 0 bedeutet keine zusätzliche Modulation. Für O2-Ziele muss auf GLO 2 eine WAVE außer OFF gewählt sein. Rate und Phase gelten für Filter und gewähltes Ziel gemeinsam. Die graue Referenzkurve zeigt die volle Welle, die weißen Kurven die Filter- und Zielintensität. Das Ziel wird mit SAVE gespeichert. Ältere Patches behalten ihre Lautstärke-Modulation; vorhandenes altes Pitch-Vibrato wird beim Wechsel des Ziels abgeschaltet.

Die OCT-Tasten zeigen den Betrag der Oktavverschiebung durch drei Helligkeitsstufen. Die Patchliste zentriert Text anhand der tatsächlichen Großbuchstabenhöhe im 24-Pixel-Feld.

## Neu in 0.6 — AIR & SALT

Neues Factory-Patch am Ende der Liste: AIR & SALT. Klangannäherung auf Basis der vom Nutzer bereitgestellten Stereoaufnahme: gefilterter Sägezahn mit wenig Detune, leiser Dreieck-Oszillator in gleicher Oktave, weicher Attack (149 ms), langes Decay (1,75 s), Release (2,91 s), dezente Filterbewegung, sehr langsame Verstimmung von Oszillator 2, etwas Chorus und Hall. Cutoff liegt nominal bei etwa 1 kHz und öffnet zusätzlich durch die Hüllkurve. D-Dur ist voreingestellt. Für den Charakter der Aufnahme langsam in der mittleren Lage spielen und Noten ausklingen lassen.

Das Patch ist eine Synthese-Annäherung, keine exakte Rekonstruktion oder ein Sample des Originals. Die MP3 wird nicht mitgeliefert.


## Neu in 0.7 — Patches und Effekt-Typen

Alle Klänge heißen jetzt Patches. SAVE öffnet die Namenseingabe. Existiert der Name bereits (Groß-/Kleinschreibung und äußere Leerzeichen werden ignoriert), erscheint „Overwrite PATCHNAME“. KNOB 1 wechselt Yes/No; HOME bestätigt. Standard ist No; damit kehrt man zur Namenseingabe zurück. Ein neuer Name belegt einen freien der 32 Benutzerplätze. Ein vorhandener Benutzer-Patch wird im selben Platz überschrieben, auch bei voller Bank.

Auf der Patchliste ruft erneutes HOME „Delete PATCHNAME“ auf. KNOB 1 wählt Yes/No, HOME bestätigt; Standard ist No. Benutzer-Patches werden samt Favoriten-Markierung gelöscht. Werks-Patches werden dauerhaft ausgeblendet. Beim Überschreiben eines Werks-Patches entsteht eine Benutzer-Version mit dessen Namen und Favoritenstatus; das Werksoriginal wird ausgeblendet. Dafür ist ein freier Benutzerplatz nötig. Zum Speichern und Löschen die Wiedergabe stoppen; PLAY funktioniert auch im Bestätigungsdialog.

FX hat jetzt drei Seiten (1/3, 2/3, 3/3). Seite 3:

| Regler | Funktion |
| --- | --- |
| 1 DELAY | CLASSIC: Mono-Echo; PING PONG: abwechselnd links/rechts; REVERSE: rückwärts abgespielte Audioabschnitte mit kurzen Randblenden. |
| 2 REVERB | ROOM: kurzer Comb-/Allpass-Raum; SPRING: dispersiver Federhall; HALL: dichter Stereo-Hall; SPACE: längerer, stärker bewegter Hall; SHIMMER: Hall mit oktaviertem Signal nach oben und Rückkopplung; SUBMARINE: dunkler Hall mit tiefer Oktave und langsamer Bewegung. |
| 3 TONE | Helligkeit der Delay-Rückkopplung. |
| 4 MOTION | Hall-Modulation; bei Shimmer/Submarine zusätzlich Stärke des oktavierten Signals. Room/Spring bleiben unverändert. |

Die Sendepegel stehen auf FX 1; Hallgröße, Dämpfung, Delay-Raster und Feedback auf FX 2. Classic bietet bis 1,486 s; Ping Pong und Reverse maximal 0,743 s pro Intervall/Audioabschnitt. Längere Raster werden auf diese Grenze begrenzt. Ping Pong benötigt Feedback für die zweite und weitere Wiederholungen. Die Stereo-Wirkung hört man über Kopfhörer oder Stereo-Ausgang. Alle vier neuen Werte werden mit dem Patch gespeichert.

Speicherlayout und Datensatzgröße bleiben unverändert. Vorhandene Benutzer-Patches und Samples aus 0.3–0.6.1 bleiben beim normalen Firmware-Update erhalten. Neue Effektparameter bekommen beim Laden alter Patches kompatible Standardwerte. Die neuen Effekttypen benötigen Firmware 0.7; ein Downgrade kann sie nicht originalgetreu wiedergeben. Hardware-Test auf einem physischen FM-1 steht hier weiterhin aus.


## Neu in 0.8 — öffentliche Projektseite

Eigene Projektkennung FM-1_983 (Firmware und Paket), ota-FM-1_983 im Update-Loader. Die Kennung bleibt bei Release-Builds stabil. Im Hub-Katalog vom 23.10.2026 war diese Nummer nicht vergeben; keine zentrale Reservierung oder Hub-Freigabe wird behauptet. Kein Wechsel des Patch-/Sample-Speicherlayouts. Website: https://www.fbnfrtg.de/tools/fm1/drift/ .
