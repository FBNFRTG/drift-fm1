# DRIFT veröffentlichen und Dateien ersetzen

Das Liefer-ZIP enthält genau zwei Ordner:

- github: Den Inhalt direkt ins Hauptverzeichnis von FBNFRTG/drift-fm1 übernehmen. Nicht als Unterordner github hochladen.
- web: Den Inhalt nach /tools/fm1/drift/ auf dem Webspace hochladen. Nicht als Unterordner web hochladen.

## GitHub ersetzen — empfohlen mit GitHub Desktop

1. GitHub Desktop installieren, anmelden, File → Clone Repository → FBNFRTG/drift-fm1 wählen.
2. Repository → Show in Finder. Prüfen, dass dies die lokale Kopie von drift-fm1 ist.
3. Alle bisherigen Projektdateien und Ordner aus dieser lokalen Kopie entfernen. Den versteckten Ordner .git unbedingt behalten. Das Repository selbst nicht löschen.
4. Den gesamten Inhalt von github aus diesem ZIP in die lokale Kopie legen; auch .gitignore übernehmen (Finder: Cmd+Shift+Punkt zeigt versteckte Dateien).
5. In GitHub Desktop die Änderungen kontrollieren: neue README, neue web-Quelldateien, gelöschte Altdateien. Summary: Update DRIFT documentation and clean release layout.
6. Commit to main, dann Push origin. Die bisherigen Versionen bleiben in der Git-Historie.

Nur Dateien hochzuladen überschreibt gleichnamige Dateien, entfernt aber keine alten. Im Browser kann man einzelne Dateien oder Ordner über das Menü … löschen und committen; für diesen vollständigen Austausch ist Desktop übersichtlicher. Kein Force Push und keine Löschung des GitHub-Repositories nötig.

## Website ersetzen

Vorher den bisherigen Ordner /tools/fm1/drift/ als Sicherung herunterladen. Danach ausschließlich dessen bisherige Projektdateien entfernen und den Inhalt von web hochladen. Keine anderen Bereiche von fbnfrtg.de löschen. Versteckte .htaccess mit übertragen. /webplayer/ bleibt unter derselben Adresse. HTTPS verwenden.

## GitHub Release

Nach den Hardwaretests einen Release v0.9.1 mit Titel DRIFT 0.9.1 erstellen (falls er schon existiert, den passenden Release bearbeiten). Als Assets web/downloads/drift-fm1-0.9.1.fwsc und web/downloads/drift-0.9.1-source.zip anhängen. Die ZIP-Datei ersetzt nicht den lesbaren Quellcode im Repository. Ein bestehender unveränderlicher Release lässt sich gegebenenfalls nicht ändern; dann eine neue Version erstellen und alle Versionsangaben konsistent aktualisieren.

Firmware und Webplayer wurden für 0.9.1 neu gebaut. Vor Veröffentlichung auf dem echten FM-1 testen. Identity: FM-1_983, Loader: ota-FM-1_983. Keine neuen Hardwaretests werden behauptet.

## Aufgeräumt

Entfernt: alte Felucca-Startseite, Felucca-Editor und Backup-Weboberfläche, deren Webtests, alte Bedienfotos und der nicht mehr passende alte Test-Allrounder. Der neue tests/run_tests.sh startet die DRIFT-spezifischen Tests. Firmware, eingebundene Engines, Sample- und Font-Assets, Build-Werkzeuge und rechtliche Hinweise bleiben erhalten; interne Felucca-Dateinamen sind Abhängigkeiten, kein Branding der Projektseite.

## Hub

Repository, Release-Link, Identity und Angaben aus web/HUB-INFO.txt nach realen Hardwaretests an den Hub-Betreiber senden. Teststatus in den Projekttexten erst nach tatsächlich bestandenen Tests aktualisieren.
