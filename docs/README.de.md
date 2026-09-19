# Cam Outlines

Nativer OBS-Effektfilter für Windows x64. Zeichnet Gesichtslandmarks als Linien
über die vorhandene Kameraquelle. Augen, Augenbrauen, Nase, Mund und Kiefer sind
einzeln auswählbar. Zusätzlich gibt es die vollständige Gesichtskontur, Iris-Ringe
und ein optionales Face Mesh. Farbe, Deckkraft, Linienstärke, Glättung und Trackingrate sind
einstellbar. Kein zusätzlicher Kamerazugriff, Python-Prozess oder Browser nötig.

## Installation

1. OBS schließen.
2. **camoutlines-0.2.1-windows-x64.zip** unter
   [Releases](https://github.com/AlexKrois/obs-camoutlines/releases/tag/v0.2.1) herunterladen.
   Nicht das automatisch von GitHub angebotene „Source code“-ZIP verwenden.
3. Die beiden ZIP-Ordner **obs-plugins** und **data** direkt nach
   `C:\Program Files (x86)\Steam\steamapps\common\OBS Studio\` entpacken.
   Ordner zusammenführen und vorhandene Cam-Outlines-Dateien beim Update ersetzen.
   Die Plugin-DLL liegt anschließend unter
   `C:\Program Files (x86)\Steam\steamapps\common\OBS Studio\obs-plugins\64bit\camoutlines.dll`.
   Die Daten liegen unter `OBS Studio\data\obs-plugins\camoutlines\`.
4. OBS starten. Kameraquelle → **Filter** → **Effektfilter** → **+** →
   **Cam Outlines — Gesichtslandmarks**.
5. Gewünschte Gesichtspartien auswählen. Mit 640 Pixel Tracking-Auflösung,
   30 Tracking-FPS und 2,5 Pixel Linienstärke beginnen.

Bei einer normalen OBS-Installation statt des Steam-Pfads deren Stammordner
verwenden, üblicherweise `C:\Program Files\obs-studio`.
Das ganze ZIP gehört **nicht** nach `obs-plugins\64bit`: Dort liegt nur die
Plugin-DLL. Die mitgelieferten Daten müssen ihre separate Ordnerstruktur behalten.
Das Installations-ZIP enthält nur Laufzeitdateien, Übersetzungen und Lizenzhinweise.
Quellcode und Entwicklerdateien sind separat im Repository verfügbar.

### Neue Optionen in 0.2.0

- **Vollständige Gesichtskontur:** geschlossene Kontur inklusive Stirn. Der Kiefer
  ist darin bereits enthalten; bei beiden aktiven Schaltern werden gemeinsame
  Linien nur einmal gezeichnet.
- **Iris-Ringe:** zwei geglättete elliptische Umrisse aus den Iris-Landmarks.
  Das ist keine kalibrierte Blickrichtungs- oder Pupillengrößenmessung. Bei
  geschlossenen oder verdeckten Augen kann das Modell Positionen schätzen.
- **Face Mesh (Dreiecksnetz):** 1.322 eindeutige Verbindungen der MediaPipe-
  Gesichtstopologie. Eigene Linienstärke und Deckkraft; dieselbe Farbe wie die
  Konturen. Startwerte: 1 Pixel und 35 % Deckkraft. Wird unter den Konturen gezeichnet.

Alle drei neuen Optionen sind standardmäßig ausgeschaltet und einzeln zuschaltbar.
Bestehende Einstellungen bleiben erhalten. Für das Update OBS schließen und das
Installationsskript erneut ausführen; keinen zweiten Filter hinzufügen.

### Installation direkt im OBS-Ordner (auch Steam)

Alternativ im Projekt nach dem Build ausführen, während OBS geschlossen ist:

```powershell
./scripts/install.ps1 -ObsRoot 'C:\Program Files (x86)\Steam\steamapps\common\OBS Studio'
```

Das Skript sichert vorhandene Plugin-Dateien und installiert mit Prüfsummenprüfung.
Je nach Ordnerrechten ist eine PowerShell mit Administratorrechten erforderlich.
Das ZIP und das Skript verwenden dieselbe Ordnerstruktur:

```text
OBS Studio/
  obs-plugins/64bit/camoutlines.dll
  data/obs-plugins/camoutlines/libmediapipe.dll
  data/obs-plugins/camoutlines/face_landmarker.task
  data/obs-plugins/camoutlines/locale/de-DE.ini
  data/obs-plugins/camoutlines/locale/en-US.ini
```

Keinen zusätzlichen `camoutlines`- oder `data`-Unterordner anlegen. Werden `FilterName`, `Help` oder
`Error: Cannot load MediaPipe DLL (Windows error 87)` angezeigt, zuerst diese
Ordnerstruktur prüfen: In diesem Fall findet OBS die Plugin-Daten nicht.

Gebaut gegen die OBS-30.2.3-Schnittstelle; Darstellung auch mit der Steam-Version
OBS 32.2.2 getestet. Das Microsoft Visual C++ x64 Runtime muss vorhanden sein (normalerweise
bereits mit OBS installiert). Dies ist ein erster Prototyp, kein fertiges Release.

## Entwicklung

Benötigt Windows x64, PowerShell 7, Visual Studio mit C++-Desktopentwicklung und
CMake-Komponente sowie eine lokale OBS-Installation.

```powershell
./scripts/prepare.ps1
./scripts/build.ps1
# Bei anderem Installationspfad:
./scripts/build.ps1 -ObsRoot 'D:\OBS'
```

`prepare.ps1` lädt OBS-30.2.3-Header, MediaPipe 0.10.32 (C-Header und Windows-DLL)
und das offizielle Face-Landmarker-Modell v1. Die Wheel-Prüfsumme wird gegen
die PyPI-Metadaten geprüft. Es installiert keine Python-Pakete. `build.ps1`
erzeugt aus der lokalen OBS-DLL die Importbibliothek, baut und testet das Plugin
und erstellt das ZIP unter `dist/`. Es verändert keine OBS-Installation.

## Verhalten und Grenzen

- Verfolgt ein Gesicht mit 478 Landmark-Punkten; gezeichnet werden ausgewählte
  anatomische Linienzüge. Der Kiefer ist die untere Gesichtskontur, ohne Stirn.
- Das Kamerabild behält seinen Bildausschnitt. Glättung reduziert Zittern und
  erhöht zugleich die Verzögerung der Linien.
- CPU-Inferenz läuft in einem eigenen Thread. Es gibt keine anwachsende
  Frame-Warteschlange. Die Übertragung vom GPU-Bild kann trotzdem Renderzeit kosten.
- Linien verschwinden bei fehlendem Gesicht oder wenn das zugrunde liegende
  Kamerabild älter als 350 ms ist. Schnelle Bewegungen können sichtbaren Versatz
  erzeugen; starke Drehung und Verdeckung können die Erkennung unterbrechen.
- Erste Version für SDR-Quellen. HDR-Farbtreue, mehrere Gesichter und manuell
  konfigurierbare Landmark-Verbindungen sind nicht implementiert.
- Analyse erfolgt am Bild an dieser Position der Filterkette. Spiegelung oder
  Skalierung der gesamten OBS-Quelle transformiert Bild und Linien gemeinsam.
- Fehlendes Modell/Runtime: Kamerabild bleibt sichtbar; Details stehen im
  OBS-Protokoll (`[camoutlines]`). Nach Korrektur Filter neu hinzufügen.
- Für einen sauberen visuellen Vergleich zuerst andere Effektfilter abschalten.

## Prüfung in OBS

Mit einer Kamera jedes Kontur-Kästchen separat prüfen, Kopf bewegen, blinzeln,
Mund öffnen und das Bild verlassen. Anschließend Quelle spiegeln, Filter aus/ein
schalten, Quellenauflösung wechseln und die Szene wechseln. Die Linien müssen
korrekt mittransformiert werden und nach Gesichtsverlust verschwinden. OBS-
Statistik auf ausgelassene Rendering-Frames prüfen. Diese interaktive Prüfung
ist zusätzlich zu den automatisierten Tests erforderlich.

## Technik

`src/plugin.cpp`: OBS-Integration, Texturaufnahme, Hintergrundthread, Darstellung.
`src/detector.cpp`: dynamisch geladene native MediaPipe-C-Schnittstelle.
`src/landmarks.hpp`: Konturen und Dreiecksgeometrie für einstellbare Linienstärken.
MediaPipe-DLL und Modell sind im Datenordner des Plugins enthalten.

Inspiration: [obs-face-tracker](https://github.com/norihiro/obs-face-tracker).
Eigenständige Implementierung; keine Abhängigkeit von dessen Installation.
Weitere Quellen und Lizenzangaben: [THIRD_PARTY.md](../THIRD_PARTY.md).

## Verifizierter Stand

Am 19.09.2026 auf Windows x64 gegen OBS 30.2.3 gebaut und auch mit OBS 32.2.2
(Steam) getestet. Automatische Tests für
Geometrie und native Inferenz bestanden. Ein zusätzlicher Test mit der echten
OBS-/Direct3D-11-Pipeline bestand für alle acht Konturschalter, Gesichtsverlust,
erneute Erkennung sowie Filter aus/ein. Das dabei gerenderte Testportrait wurde
visuell geprüft. Weitere Live-Tests mit unterschiedlichen Kameras und Systemen sind erforderlich.
Details und reproduzierbare Testaufrufe stehen in `tests/README.md` im Quellcode.
