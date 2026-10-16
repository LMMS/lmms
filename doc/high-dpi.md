# High-DPI: Implementierung und Abnahme

Stand: 16. Oktober 2026. Dieses Protokoll ersetzt die unbelegten Abschlussbehauptungen aus Commit `02147dcae`. Offscreen-Tests sind kein Nachweis für echte Per-Monitor-Wechsel.

## Ressourcenvertrag

- Qt zeichnet in logischen Koordinaten. Systemskalierung bleibt Standard; ein eigener Oberflächenzoom ist nicht enthalten. `PassThrough` wird vor der GUI-Anwendung gesetzt. Der CLI-Pfad benötigt weder Screen noch Pixmap.
- `Dpi::rasterSizeFor` verwendet Deckenrundung. Nichtpositive/nicht endliche Zielgrößen und nicht darstellbare Rasterabmessungen ergeben einen leeren Puffer. Nichtpositive oder nicht endliche DPRs fallen auf 1 zurück.
- `embed::PixmapResource` behält Quelle und logische Zielgröße neben der Pixmap. `atDpr()` lädt eine passende Variante. Gerundete Rastermaße dürfen nicht zurück ins Layout fließen; beim Zeichnen ein explizites Zielrechteck verwenden.
- SVG, PNG und XPM verwenden dieselbe Größenbedeutung. Ohne Größenangabe gilt die natürliche logische Größe; ein ausdrücklich benanntes `@2x`-PNG hat die halbe Dateigröße als logisches Maß. Ein einzelnes Zielmaß erhält das Seitenverhältnis.
- Zuerst wird die Quelle innerhalb der Theme-Suchreihenfolge aufgelöst. Erst danach werden SVG- oder `@2x`-Varianten im selben Verzeichnis gesucht. Ein höher aufgelöstes Default-Asset übergeht keine Grafik des ausgewählten Themes.
- Cache-Schlüssel enthalten aufgelöste Quelle, XPM-Identität, Zielmaße und vollständigen DPR. Natürliche und identische explizite Zielgrößen teilen das Raster. Pixel bleiben durch `QPixmapCache` begrenzt.
- Eine begrenzte Metadatenablage unterstützt alte Pixmap-Signaturen. Lang lebende Verbraucher behalten einen eigenen `PixmapResource`, damit Metadatenverdrängung ihren Reload nicht verhindert. Beliebig transformierte oder selbst erzeugte Pixmaps besitzen keine rekonstruierbare Quellenbeschreibung; deren Besitzer bleibt verantwortlich.
- Alte kontextfreie Aufrufe liefern eine vorläufige 1x-Pixmap. `embed::getIcon()` und `PixmapLoader::icon()` liefern für Menüs, Toolbars und Fenster dateibasierte QIcons, sodass Qt die aktuelle Auflösung anfordern kann. PixmapButton, Fader, Draggable, Timeline und Analyzer behalten Ressourcenbeschreibungen.

## Dynamische Aktualisierung und Geometrie

`MainApplication` erfasst später angezeigte Top-Level-Widgets. `DpiContext` gehört zum Fenster, verwendet `QPointer`, verwaltet Screen-Verbindungen und fasst Änderungen in einem Eventloop-Durchlauf zusammen. Ein nach Attach nicht mehr benötigter Kontext wird entfernt.

`Dpi::watch()` registriert einen Verbraucher mit lebenszeitgebundenem Callback. Show, Reparent, Handle-, Font-, Palette-, Style- und Enabled-Änderungen sowie Kontextbenachrichtigungen erneuern Ressourcen oder invalidieren Caches. Attach/Detach bindet registrierte Nachfahren neu. Es gibt weder globales DPR-Polling noch ein globales Leeren des Pixmap-Caches bei jedem Monitorwechsel.

Knob-, Clip-, Track-, LCD-, CPU-, AFP-, SlicerT-, EQ- und Compressor-Raster berücksichtigen den DPR. Die SampleClip-Viewportbegrenzung bleibt bestehen. Compressor-Puffer passen zur sichtbaren Fläche, statt fünf Desktopflächen hochauflösend vorzuhalten. Sample-/FFT-Daten, musikalischer Zoom und Projektmodelle bleiben DPI-unabhängig. PianoRoll und AutomationEditor zeichnen ihre Editorflächen direkt in logischen Widgetkoordinaten; ihre Eingabekoordinaten werden nicht skaliert.

Geometrie verwendet die bestehenden XML-Attribute. Ein gemeinsamer Helfer berücksichtigt einzelne Arbeitsflächen, Lücken, negative Monitorpositionen und Mindestgrößen. Bei abgelösten Fenstern werden das sichtbare Widget und native Rahmenmaße verwendet; Wayland erhält nur Größenanpassungen. Einstellungsaktionen liegen außerhalb des scrollbar gemachten Dialoginhalts.

## Native VST-Grenze

Der Windows-Host bleibt PerMonitorV2. MSVC bindet das Manifest �ber den Linker ein, MinGW �ber eine RC-Ressource; die Auswahl erfolgt durch CMake statt �ber `_MSC_VER` im Ressourcencompiler. Die aktuelle Begleitdatei wird f�r Upgrades weiterhin installiert, damit ein altes `dpiAware=false`-Manifest �berschrieben wird. Der Installer setzt `PreferExternalManifest` nicht mehr global. Die 32-/64-Bit-Hilfsprogramme erhalten ein explizites DPI-unaware-Manifest, das die bisherige Kompatibilität älterer Plugins erhält. Eine hostweite Awareness-Änderung wird nicht auf Fremdplugins übertragen.

Vor Qt- oder Win32-Einbettung werden tatsächliche HWND-Kontexte über dynamisch aufgelöste Windows-APIs verglichen. Fehlende/abweichende Kontexte führen zum separaten Editor. Legacy-Editoren können dadurch unter Windows häufiger separat erscheinen. `SetParent` wird bei inkompatiblen Kontexten nicht aufgerufen, da Windows sonst die Awareness des Hilfsprozesses zurücksetzen kann.

Bei übereinstimmenden per-monitor-Kontexten sind `effEditGetRect` und `audioMasterSizeWindow` native Clientpixel. Nur am Qt-Container wird durch dessen effektiven DPR geteilt. Host-Geometrieänderungen laufen im GUI-Thread; identische Größen werden nicht erneut angewandt. Reine Host-Reaktionen senden keine Größenmeldung zurück.

Der angehängte Protokollbefehl `IdVstUseSeparateUI` schaltet den Hilfsprozess im eigenen GUI-Thread um und stellt den separaten Rahmen in dessen Koordinatenraum wieder her. Bestehende IDs und die zwei Geometriefelder bleiben unverändert. Host und Hilfsprogramme müssen aus demselben Build ausgeliefert werden. VeSTige und VST-Effektdialog berücksichtigen den Laufzeit-Fallback.

Grundlage: [Microsoft SetParent](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setparent), [Mixed-mode DPI](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-improvements-for-desktop-applications). Tatsächliche Kompatibilität historischer Plugin-UIs muss mit Testplugins geprüft werden.

## Theme-Grafiken

[high-dpi-assets.json](high-dpi-assets.json) erfasst alle 283 Default- und 268 Classic-PNGs mit Pixelmaßen und vorhandenen Alternativen. Alte Raster-Skins bleiben ein funktionierender 1x-Fallback; Hochskalieren erzeugt keine neuen Bilddetails. Sie sind ausdrücklich nicht als visuell abgenommen gekennzeichnet.

Positionsmarker beider Themes besitzen zusätzliche Vektorquellen mit unveränderten logischen Maßen und den jeweiligen Theme-Farben. Bei 100 % bleibt die bisherige PNG, bei höherem DPR wird die Vektorquelle verwendet. Der bestehende Default-Fader-SVG wird nach Kontextwechsel neu gerastert. Neue SVGs werden vom vorhandenen Theme-Installationsmuster `*.svg` erfasst.

## Automatisierte Prüfung

Die isolierten Tests kompilieren die echten Produktionsdateien `DpiContext.cpp` und `embed.cpp`, ohne nachgebildete Cache- oder Geometrieimplementierung:

```sh
cmake -S tests/dpi -B build-dpi -DDPI_QT_MAJOR=5 -DCMAKE_PREFIX_PATH=<Qt-Praefix>
cmake --build build-dpi --config Release --parallel
ctest --test-dir build-dpi -C Release --output-on-failure
```

Dasselbe mit Qt 6 wiederholen. Die Matrix umfasst 1/1,25/1,5/1,5625/1,75/2 in getrennten Prozessen. Geprüft werden ungerade Maße, ungültige Größen/Faktoren, Quellen-/Theme-Reihenfolge, SVG/PNG/XPM, 2x-Varianten, Cache-Wiederverwendung, getrennte DPRs, Lebenszyklus/Reparent und Produktions-Geometriehelfer. Der Qt-5-Teststart setzt dieselben High-DPI-Voraussetzungen wie LMMS. Der gemessene effektive DPR wird gegen den angeforderten Faktor geprüft.

Die vollständige LMMS-Suite enthält zusätzlich `DpiWidgetTest` mit echten PixmapButton-, Knob-, LCD-, Graph-, PianoView- und EnvelopeGraph-Instanzen: logische Maße, Klickposition, Sprite-Zustände und Cache-Erneuerung bei unverändertem Modellwert. Die neue CI-Matrix umfasst Windows Qt 5/6, Linux Qt 5 und macOS Qt 6. Ein erfolgreicher Job belegt nur die tatsächlich ausgeführten Tests.

## Hier ausgef�hrte Nachweise

| Pr�fung | Ergebnis |
| --- | --- |
| Windows x64, Qt 5.15.2 / MinGW GCC 13.1 | Kern, AFP, SlicerT, EQ, Compressor, Analyzer, VST-Host/Hilfsprozess, VeSTige, VST-Effekt und TripleOscillator gebaut; vollst�ndige registrierte Suite: 43/43 bestanden. |
| Windows x64, Qt 6.8.3 / MSVC 19.51 | Dieselben Ziele gebaut; vollst�ndige registrierte Suite: 43/43 bestanden. |
| Isolierte Produktionscode-Matrix | Je 25/25 Pr�fungen unter MinGW/Qt 5 und MSVC/Qt 6, einschlie�lich nativem Windows-Test. |
| Nativer versteckter Test mit Hostmanifest | Beide Qt-Pfade: tats�chlich PerMonitorV2, 120 DPI / DPR 1,25; 100�32 logische Pixel ergeben exakt 125�40 native Clientpixel. Kein sichtbares Testfenster erforderlich. |
| Bin�rmanifeste | Mit `mt.exe` aus beiden gebauten Host-EXEs und beiden RemoteVstPlugin64-EXEs extrahiert: Host PerMonitorV2, Helper explizit unaware. |
| Portable Qt-6-Ausgabe | Lokal installiert, Qt-Plattform-/SVG-Plugins und acht neue Vektorassets vorhanden; Host-/Helper-Manifeste erneut aus den installierten EXEs gepr�ft. |
| CLI-Audio | Ein Ein-Noten-Projekt mit TripleOscillator ergibt 151040 Frames bei 44,1 kHz, Stereo/16 Bit, Spitzenwert 25951. Qt-5-, Qt-6- und portable Ausgabe sind byteidentisch (SHA-256 `cc48f099d6eeee7a422b2cb97854e0f91f3fdd963741ca12e0a67b601bc6a410`). Qt 5 und portable Ausgabe rendern auch mit absichtlich ung�ltigem QPA-Plugin und Faktor 1,5625: der Core-only-Pfad initialisiert keine GUI. |

Die Audio-Backends und nicht genannten optionalen Plugins waren in diesen lokalen Pr�fbuilds deaktiviert. Die Ergebnisse belegen keine echte Audiowiedergabe und keine komplette Pr�fung aller Fremdplugins. Ein beim Qt-5-Build gefundener Bestandsfehler in `FileRevealer` wurde durch eine ausdr�cklich typisierte `QStringList`-Zuweisung behoben.

## Offene Plattformabnahme

Bis zu einem protokollierten Lauf bleiben offen:

- Windows MinGW/Qt 5.15 und MSVC/Qt 6: 100/125/150/175/200 %, WQHD 2560×1440 bei 125 % ohne Overrides; gebaute und paketierte Manifeste prüfen.
- Zwei Monitore (100+125 %, 125+150/200 %), Start auf Zweitmonitor, unabhängige Fenster, Rückwechsel, Skalierungsänderung am selben Monitor, Entfernen/Wiederanschließen und Primärmonitorwechsel.
- Ruhende Widgets, Editoransichten, Menüs/Tooltips, Drag-Vorschauen, abgelöste Fenster und bestehende Projekte auf Qualität und Eingabeversatz prüfen; beide Themes vergleichen.
- Je ein DPI-fähiger und Legacy-VST-Editor: Qt-/Win32-Einbettung, separater Fallback, Resize, Klickpositionen und Schließen; tatsächliche Host-/Hilfsprozess-Awareness protokollieren.
- ZynAddSubFx/FLTK und Carla separat testen. Deren Toolkit-/Fremdfenster werden von Qt-Widgetkorrekturen nicht automatisch schärfer. LMMS-generierte LV2-/LADSPA-Ansichten mitprüfen.
- Linux X11/Wayland mit benannter Qt-/Compositor-Konfiguration; macOS Retina und externer Bildschirm. Keine pauschale Zusage für jede Fractional-Scaling-Kombination.
- Audiowiedergabe und CLI-Rendering sowie GUI-Zeichenzeit, Raster-Neubauten und Speicher mit demselben Projekt vor/nach der Änderung messen. Keine nicht gemessenen Performancezusagen ableiten.

Die Diagnose nennt Qt-Version, Plattform, bekannte Qt-Skalierungs-Overrides, Screen-Werte, Fenster-DPR-Wechsel und beim VST-Einbettungsversuch tatsächliche HWND-Kontexte.
