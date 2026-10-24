# DRIFT 0.9 — Bedienung und Update

Eine Spur, acht Stimmen, zwei Oszillatoren und drei Sample-Slots. Firmware und Webplayer verwenden denselben C-Code für Audio und Display.

## Sequencer

ARP: Takte (1–16), Notenanzahl (1–32), Dauer in Sechzehnteln, Wahrscheinlichkeit. Grundverteilung euklidisch. Der Loop wiederholt sich unverändert; Einstellungen ändern das Pattern bei laufendem Abspielkopf.

SEQ:
- DIR: ROOT, UP 1–7, DN 1–7, UD 1–7, DU 1–7, BOUNCE 1–7, OUT 1–7, IN 1–7, PEDAL 1–7, WALK 1–7. UD/DU wechseln über und unter den Grundton; BOUNCE läuft vorwärts und rückwärts ohne doppelten Endpunkt. OUT/IN bewegen sich nach außen/innen. PEDAL wechselt zwischen Grundton und Lauftönen. WALK ist ein reproduzierbarer Zufallslauf.
- INTERVAL: 1ST, 2ND, 3RD, 4TH, 5TH, 6TH, 7TH, 8VE; danach 3/4, 3/5, 4/5, 2/3, TRIAD, 5/4, 8/3, 3/4/5. Die Brüche sind hier wechselnde Intervalle, keine Taktangaben. TRIAD addiert 2, 2, 3 Skalenstufen. Alle Intervalle folgen der eingestellten Skala. 1ST und 2ND behalten aus Kompatibilitätsgründen denselben Grundschritt; bei DIR ROOT und INTERVAL 1ST bleiben es Grundtöne. Neue Folgen werden an Bereichsgrenzen innerhalb des Bereichs weitergeführt.
- CHAOS: 0 = geordnet. 1–999 = verschiedene feste Durchmischungen. Kein Intensitätsregler mehr. Bei wenigen oder gleichen Noten können mehrere Werte hörbar identisch sein.
- TIME: 0–100, Swing bis zu starken Verschiebungen und Clustern.

Alle vier Werte auf 0 spielen nur den Grundton. Ohne Grundton im gewählten Bereich bleibt die Sequenz stumm.

## FX

| Seite | Knob 1 | Knob 2 | Knob 3 | Knob 4 |
| --- | --- | --- | --- | --- |
| DELAY 1/3 | Delay % | Time | Feedback | Type |
| REVERB 2/3 | Reverb % | Size | Type | Tone |
| MISC 3/3 | Chorus | Damp | Motion | Level |

Delay: Classic, Ping Pong, Reverse. Time geht in zeitlicher Reihenfolge von 1/32 bis 4 Takten, inklusive punktierter und triolischer Werte. D = punktiert, T = Triole. Bei langen Zeiten arbeitet nur der Delay-Puffer mit geringerer Abtastrate: echte längere Wiederholungen, mit dunklerem Klang. Maximal 24 Sekunden bei 40 BPM. Reverse sammelt einen Abschnitt und spielt ihn rückwärts; der Anfang einer kurzen Aufnahme kann daher erst gegen Ende des nächsten Abschnitts hörbar werden. Ping Pong benötigt Feedback für den zweiten und weitere Echos.

Reverb: Room, Spring, Hall, Space, Shimmer, Submarine. Tone färbt den Hall-Eingang; Damp dämpft die Hallfahne. Motion moduliert Hall/Space und steuert bei Shimmer/Submarine zusätzlich den Pitch-Anteil; Room/Spring nutzen ihre eigene feste Bewegung.

## Patches und Update

HOME: Patchliste; REC: Favorit. SAVE: Namen eingeben und speichern. Gleicher Name fragt vor Überschreiben nach. HOME erneut auf der Liste: Löschen, Standard No. Knob 1 wählt Yes/No, HOME bestätigt. Zum Speichern/Löschen Wiedergabe stoppen.

Patches und Samples aus 0.8 bleiben im bisherigen Speicherlayout. CHAOS über 127 belegt freie Bytes im unverändert großen Patchdatensatz. Nichtnull-CHAOS und die Effektfarbe können nach dem Update anders klingen: der frühere Delay-Tone-Wert regelt jetzt Reverb Tone. Alte Firmware bildet neue Optionen nicht korrekt ab. Vor Update Daten sichern.

## Weitere Bedienung

SELECT: BPM (Standard 80). ALGORITHM: Lowpass. GLO: Oszillator 1 / 2 (OFF als erste Wave). ENV: ADSR. EDIT: Filter. LFO: Rate, Filter, Target, Depth. SCL: Root, Scale, Min, Max / Scale LEDs und Scale only. OCT: Live-Oktave. MASTER: Lautstärke.

Drei Sample-Slots: je bis ca. 7,38 Sekunden, mono, 22.050 Hz, intern IMA ADPCM. Der englische Uploader wandelt lokal um; Stereo wird zu Mono.

## Links

- Projekt: https://www.fbnfrtg.de/tools/fm1/drift/
- Webplayer: https://www.fbnfrtg.de/tools/fm1/drift/webplayer/
- Installer: https://www.fbnfrtg.de/tools/fm1/drift/webplayer/install/
- Quellcode: https://github.com/FBNFRTG/drift-fm1
- Impressum: https://www.fbnfrtg.de/impressum/

Build: BUILDING.md. Softwaretests sind enthalten; die neue Firmware 0.9 wurde hier nicht auf einem echten FM-1 getestet. Identity bleibt FM-1_983. Lizenz und Credits: LICENSE, LICENSING.md, CREDITS.md.
