## Entwurf-und-Fertigung-von-Flachbaugruppen-SCARA
 
Dieses Repository ist Teil des Modules **IE-M13-3054** der Studierenden **Dejan Dimitrijević**, **Alexander Klose** und **Jakob Cap** an der [**Hochschule für Wirtschaft und Recht (HWR) Berlin**](https://www.hwr-berlin.de/).

## Inhaltsverzeichnis
* [Grundlegende Informationen](#grundlegende-informationen)
* [Repository-Struktur](#repository-struktur)
* [Idee hinter dem Projekt](#idee-hinter-dem-projekt)
* [Schaltplanentwicklung](#schaltplanentwicklung)
* [Prototypenentwicklung](#prototypenentwicklung)
* [Finales PCB-Design](#finales-pcb-design)
 
## Grundlegende Informationen
 
Dieses GitHub-Repository befasst sich mit der Entwicklung und Fertigung eines PCBs für einen SCARA-Roboter, welcher im Rahmen des Moduls **IE-M26-6045** von den Studierenden **Jakob Cap** und **David Kowalczyk** entwickelt wurde. Der SCARA-Roboter DJ01 ist im Nachfolgenden Bild zu sehen.

<p align="center">
<img src="images/SCARA - DJ01 Baugruppe Als einzelne Komponenten v3.png" alt="SCARA-Roboter DJ01" height="600" width="auto">
</p>

## Repository-Struktur
 
```
├── sw/
│   └── Code/              	   # Programmcode des SCARA-Roboters DJ01
├── proj/
│   ├── Easy EDA/              # Easy EDA Projekt der PCBs inklusive Schaltplan und PCB Layout
│       ├── Prototyp/          # Easy EDA Projekt des ersten Prototyps
│       └── Finales Design/    # Easy EDA Projekt des finales Designs
│   ├── Schaltplan/            # Schaltpläne der PCBs
│       ├── Prototyp/          # Schaltplan des ersten Prototyps
│       └── Finales Design/    # Schaltplan des finales Designs
│   └── Export/                # Exportierte Dateien (Gerber, BOM, Pick-and-Place, etc.) für die Fertigung des PCBs
│       ├── Prototyp/          # Exportierte Dateien des ersten Prototyps
│       └── Finales Design/    # Exportierte Dateien des finales Designs
├── docs/
│   ├── 3D-Design/             # 3D Dateien und technische Zeichnungen
│   ├── Datenblätter/          # Datenblätter der Komponenten
│   ├── Pinouts/               # Pinouts der Komponenten
│   ├── Lastenheft/            # Lastenheft des SCARA-Roboters DJ01
│   ├── Bestandsschaltplan/    # Bestandsschaltplan des SCARA-Roboters DJ01
│   ├── Bauanleitung/          # Bauanleitung des SCARA-Roboters DJ01
│   └── Ablaufdiagramm/        # Ablaufdiagramm des Programmcodes
├── Abgabe Konstruktionslehre/ # Ausarbeitung für das Modul Konstruktionslehre
├── Images/                    # Bilder für die README.md
└── README.md
```
## Idee hinter dem Projekt

Die Idee hinter dem Projekt ist es, ein PCB für den SCARA-Roboter DJ01 zu entwickeln, das die Aktuell verwendete Steuerungseinheit ersetzt. Aktuell erfolgt die Ansteuerung der NEMA 11 und NEMA 17 Schrittmotoren sowie des Servomotors über ein ESPDuino-Board. Dieses ESPDuino-Board ist mit einem CNC-Schihield V3 verbunden, auf welchem vier [`TMC2209`](docs/Datenblaetter/tmc2209_datasheet_rev1.09.pdf) Schrittmotortreiber eingesteckt sind. Zudem werden über das CNC-Shield der Servomotor des Greifers angesteuert sowie die vier Endlagenschalter ausgelesen. Eine schematische Darstellung des aktuellen Aufbaus ist in der nachfolgenden Abbildung zu shehen.

<p align="center">
<img src="images/SCARA_Symbolschaltplan_Steckplatine.png" alt="Schematischer Schaltplan des SCARA-Roboters DJ01" height="600" width="auto">
</p>

Da das CNC-Schield keine Anschlüsse für die DIAG-Pins der TMC2209 Schrittmotortreiber bietet, ist es nicht möglich, die Fehlerzustände der Schrittmotortreiber auszulesen und ein Sensorloses Homing der Motoren zu implementieren. Zudem muss das CNC-Schield und der ESPDuino über zwei separate Spannungsquellen versorgt werden, was den Aufbau unnötig verkompliziert.

Ziel dieses Projektes ist es daher, ein PCB zu entwickeln, welches in der Lage ist die vier TMC2209 Schrittmotortreiber sowie den Servomotor anzusteuern und über die Verwendung der DIAG-Pins ein Sensorloses Homing der Motoren zu ermöglichen. Zeitgleich soll die Stromversorgung des gesamten Roboters nur noch über ein einzelnes 24 V DC-Netzteil erfolgen. Die genaue Auflistung der funktionalen und nicht funktionalen Anforderungen kann dem [`Lastenheft`](docs/Lastenheft/SCARA_PCB_Lastenheft.pdf) entnommen werden.

## Prototypenentwicklung

Aufgrundlage der im Abschnitt [`Idee hinter dem Projekt`](#idee-hinter-dem-projekt) beschriebenen Anforderungen wurde ein erster Prototyp des PCBs entwickelt. Dieser erste Prototyp wurde in der Software **EasyEDA** entworfen und anschließend über den Hersteller **JLCPCB** gefertigt. Die entwicklung dieses Prototyps ist in den nachfolgenden Abschnitten beschrieben.

### Schaltplanentwicklung

Die Entwicklung des Schaltplans erfolgte auf Grundlage der im Abschnitt [`Idee hinter dem Projekt`](#idee-hinter-dem-projekt) beschriebenen Anforderungen. Ziel war es, die vier bestehenden TMC2209 Schrittmotortreiber über Steckplätze auf dem PCB zu befestigen. Die Motorkabel der Schrittmotoren und des Servomotors sollten über Steckverbinder auf dem PCB angeschlossen werden. Aufgrund seiner großen Abmessungen sollte der aktuell verwendete ESPDuino durch ein kleineres ESP32-DevkitC 

### PCB-Layout

### Bestückung und Fertigung

### Erkentnisse und Fehlerbehebung

## Finales PCB-Design

### Änderungen des Schaltplans

### Änderungen des PCB-Layouts

### Bestückung und Fertigung

