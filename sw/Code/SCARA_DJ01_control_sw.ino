/*
  Studienprojekt I - Entwicklung und Inbetriebnahme eines SCARA-Roboters 
  Jakob Cap, David Kowalczyk 

  SCARA_DJ01 - Steuerungssofware
  Hardware: ESPduino32 + CNC Shield V3 + TMC2209 V1.3 + mechanische Endschalter + NEMA 17 und 11 Schrittmotoren + MG90S Servo

  FastAccelStepper Library: https://github.com/gin66/FastAccelStepper
  Orientiert an: https://howtomechatronics.com/projects/scara-robot-how-to-build-your-own-arduino-based-robot/
*/
#include <FastAccelStepper.h>
#include <ESP32Servo.h>
#include <BluetoothSerial.h>
#include <math.h>

//Definition des Endschalter-Pins
#define LIM1    13   //Endschalter Arm1-XY-Drehung (Motor 1)
#define LIM2    23   //Endschalter Z-Achse (Motor 2)
#define LIM3     5   //Endschalter Arm2 XY-Drehung (Motor 3)
#define LIM4    34   //Endschalter Greifer-Rotation (Motor 4)

//Definition der Motor-/Treiberpins 
#define Motor1_STEP 26
#define Motor1_DIR 16   //Motor 1 Basis (Rotation Amr1)
#define Motor2_STEP 17  
#define Motor2_DIR 14   //Motor 2 Z-Achse (Translation Z)
#define Motor3_STEP 25  
#define Motor3_DIR 27   //Motor 3 Arm1 (Rotation Arm2)
#define Motor4_STEP 19  
#define Motor4_DIR 18   //Motor4 Arm2 (Rotation Greifer)
#define EN_PIN  12  //Enable-Pin für alle Schrittmotortreiber (LOW-Aktiv)
#define SRV_PIN  2  //PWM-Pin für Greifer-Servo

//Kinematische Konstanten
#define L1     100.0  //Länge des erste Roboterarms in mm
#define L2     110.0  //Länge des zweiten Roboterarms in mm
#define SPD    13.3333  //Schritte pro Grad für alle rotatorischen Achsen (Gleichung und Berechnung s. Studienprojekt)
#define SPM   266.6667   //Schritte pro mm für die translatorische Achse (Gleichung und Berechnung s. Studienprojekt)
#define Z0     85.0 //Referenzhöhe des TCPs nach dem Homing in mm über dem Boden
#define WMAX  155.0 //maximaler Basiswinkel ±° (konstruktiv bedingt)

// Homing-Offsets (Schritte vom Endschalter bis zut absoluten Nullposition)
#define OFF1  -2382
#define OFF2  21750
#define OFF3   1715
#define OFF4    -82

// Demo-Sequenz (Pick-and-Place Ablauf) 
const float DEMO[][55] = {
  {  0, 210,  85, 100},
  {  0, 210,  15, 100},
  {  0, 210,  15, 100},
  {  0, 210,  15, 100},
  {  0, 210,  45, 100},
  {  210, 0,  45, 100},
  {  210, 0,  15, 100},
  {  210, 0,  15, 100},
  {  210, 0,  15, 100},
  {  100, 160,  15, 100},
  {  100, 160,  15, 100},
  {  100, 160,  15, 100},
  {  100, 160,  45, 100},
  {  -120, 120,  45, 100},
  {  -120, 120,  15, 100},
  {  -120, 120,  15, 100},
  {  -120, 120,  15, 100},
  {  -120, 120,  45, 100},
  {  0, 210,  85, 100}, 
  {  100, 160,  85, 100}, 
  {  100, 160,  4,  39},  
  {  100, 160,  45,  39},  
  {  0, 210,  45, 39},  
  {  0, 210,  10, 100},
  {  0, 210,  45, 100},  
  {  -120, 120,  45, 100}, 
  {  -120, 120,  4, 39},
  {  -120, 120,  45, 39}, 
  {  0, 210,  45, 39},  
  {  0, 210,  20, 100},
  {  0, 210,  45, 100},
  {  210, 0,  45, 100}, 
  {  210, 0,  4, 39},
  {  210, 0,  45, 39},
  {  0, 210,  45, 39},  
  {  0, 210,  30, 100},
  {  0, 210,  45, 100},
  {  0, 210,  29, 39},
  {  0, 210,  45, 39},
  {  100, 160,  45, 39}, 
  {  100, 160,  4,  100},  
  {  100, 160,  45, 100},
  {  0, 210,  45, 100},
  {  0, 210,  19, 39},
  {  0, 210,  45, 39},
  {  -120, 120,  45, 39}, 
  {  -120, 120,  4, 100},
  {  -120, 120,  45, 100},
  {  0, 210,  45, 100},
  {  0, 210,  9, 39},
  {  0, 210,  45, 39},
  {  210, 0,  45, 39}, 
  {  210, 0,  4, 100},
  {  210, 0,  45, 100},
  {  0, 210,  85, 100},
};
const int DEMO_N = 55; 

//Objekt-Instanzierung
BluetoothSerial BT;
FastAccelStepperEngine eng = FastAccelStepperEngine();
FastAccelStepper *Motor1, *Motor2, *Motor3, *Motor4;
Servo servoGripper;

//Speicherstruct für eingegeben Sequenzen über Bluetooth oder serielle Schnittstelle
struct Punkt
{ 
  float x; 
  float y; 
  float z; 
  int greiferWinkel; 
};
Punkt sequenz[50];  //Array zum vorporgrammieren, maximal 50 programmierbare Schritte um Stack Overflow zu verhindern
int sequenzAnzahl = 0; //Aktuelle Anzahl gespeicherter Schritte

//Globale Variablen
uint32_t spd = 12500, acc = 7500; //Standard Betriebsgeschwindigkeit und -beschleunigung in Hz (Rechnung s. Studienprojekt)
bool estop = false; //Not-Aus Flag (Deaktiviern des EN Pins - damit stoppen aller Motoren)
bool silent = false;  //Schaltet serielle Textausgaben silent
bool loopStop = false;  //Flag zum Abbrechen von Schleifen

//Homing ISR-Flags (valoatile, da Änderung des Interrupt stattfindet)
volatile bool h1, h2, h3, h4;
//Interrupt Service Routinen werden ausgeführt sobald Endschalter auslöst
void IRAM_ATTR isr1() { h1 = true; }  //Endschalter 1 ausgelöst
void IRAM_ATTR isr2() { h2 = true; }  //"-" 2 "-"
void IRAM_ATTR isr3() { h3 = true; }  //"-" 3 "-"
void IRAM_ATTR isr4() { h4 = true; }  //"-" 4 "-"

//Ausgabefunktion sendet Text an seriellen Monitor
void out(String s) 
{
  Serial.println(s);
  if (BT.connected()) BT.println(s);
}
//Setzt Geschwindigkeit und Beschleunigung für alle 4 Motoren
void setSpeeds() 
{
  Motor1->setSpeedInHz(spd); Motor1->setAcceleration(acc);
  Motor2->setSpeedInHz(spd*1.6); Motor2->setAcceleration(acc*1.6); //Die Z-Achse muss schneller Beschleunigung aufgrund 6mm pitch
  Motor3->setSpeedInHz(spd); Motor3->setAcceleration(acc);
  Motor4->setSpeedInHz(spd); Motor4->setAcceleration(acc);
}
//Sofortiger Stop aller Motoren auf der aktuellen Position
void stopAll() {
  Motor1->forceStopAndNewPosition(Motor1->getCurrentPosition());
  Motor2->forceStopAndNewPosition(Motor2->getCurrentPosition());
  Motor3->forceStopAndNewPosition(Motor3->getCurrentPosition());
  Motor4->forceStopAndNewPosition(Motor4->getCurrentPosition());
}
//Blockiert bis Motoren Zielposition erreicht haben und prüft auf stop-Befehl
void waitAll() 
{
  while (Motor1->isRunning() || Motor2->isRunning() || Motor3->isRunning() || Motor4->isRunning()) {
    String cmd = "";
    if (Serial.available()) cmd = Serial.readStringUntil('\n');
    if (BT.available())     cmd = BT.readStringUntil('\n');
    cmd.trim();
    if (cmd == "stop") {
      stopAll();
      digitalWrite(EN_PIN, HIGH); //Motortreiber deaktivieren
      estop = true;
      return;
    }
  }
}
//Homing-Funktion (Fährt einzelne Achse langsam gegen Endschalter und setzt Offset)
void homeAxis(FastAccelStepper *m, volatile bool &flag, bool fwd, uint32_t searchSpd, long offset) 
{
  flag = false;
  m->setSpeedInHz(searchSpd); 
  m->setAcceleration(50000);  //Hohe Beschleunigung für schnellen Stop

  if (fwd) m->runForward(); 
  else m->runBackward();
  while (!flag) {} //Warten auf ISR-Trigger Flag auf true
  
  m->forceStopAndNewPosition(offset); 
  delay(50);
}
//Referenzfahrten für alle 4 Motoren
void homing() {
  stopAll(); delay(200); setSpeeds(); out("homing");

  //1. Z-Achse (Motor 2)
  homeAxis(Motor2, h2, true, 1500, OFF2);  
  Motor2->setSpeedInHz(spd*1.6); Motor2->setAcceleration(acc*1.6);
  Motor2->moveTo(0); while (Motor2->isRunning()) {}
  //2. Basis XY (Motor 1)
  homeAxis(Motor1, h1, false, 600, OFF1);  
  Motor1->setSpeedInHz(spd); Motor1->setAcceleration(acc);
  Motor1->moveTo(0); while (Motor1->isRunning()) {}
  //3. Arm1 XY (Motor 3)
  homeAxis(Motor3, h3, true, 600, OFF3);  
  Motor3->setSpeedInHz(spd); Motor3->setAcceleration(acc);
  Motor3->moveTo(0); while (Motor3->isRunning()) {}
  //4. Greifer (Motor 4)
  homeAxis(Motor4, h4, false, 600, OFF4);  
  Motor4->setSpeedInHz(spd); Motor4->setAcceleration(acc);
  Motor4->moveTo(0); while (Motor4->isRunning()) {}

  setSpeeds(); out("homing ok");
}

//Vorwärts Kinematik = Motorposition => kartesische Koordinaten
void fk()
{
  //Schritte zuerst in Winkel umrechnen
  float theta1 = Motor1->getCurrentPosition() / SPD + 90.0; //+90° Offset für mathematische Konvention
  float theta2 = Motor3->getCurrentPosition() / SPD;
  float zm     = Motor2->getCurrentPosition() / SPM;
  
  float theta1r = theta1 * 0.017453;  //Umrechnung in Bogenmaß
  float theta2r = theta2 * 0.017453;
  //Trigonometrische Berechnungen TCP
  float x = L1 * cos(theta1r) + L2 * cos(theta1r + theta2r);
  float y = L1 * sin(theta1r) + L2 * sin(theta1r + theta2r);
  float z = Z0 - zm; //Z-Höhe invers zur Spindel-Fahrt berechnet
  
  out("FK:"+ String(x, 1)+ "," + String(y, 1)+ "," + String(z, 1));
}

//Inverse Kinematik (IK)
bool ik(float tx, float ty, float tz) 
{
  //1. Erreichbarkeitsprüfung
  float dist = sqrt(tx*tx + ty*ty);
  if (dist > 210.0 || dist < 120.0) 
  {
    if (!silent) out("Punkt nicht im Arbeitsraum"); //Ziel außerhalb des physikalischen Radius
    return false; 
  }
  //2. Winkelprüfung
  float ang = atan2(tx, ty) * 57.2958;
  if (ang < -WMAX || ang > WMAX) 
  { 
    if (!silent) out("Winkel Error"); 
    return false; 
  }
  //Koordinaten-Transformation (X+=rechts, Y+=vorne) in mathematisch
  float mx = ty; 
  float my = -tx;
  //Berechnung des Cosinussatzes für das Ellenbogengelenk
  float c2 = (mx * mx + my * my - L1 * L1 - L2 * L2) / (2.0 * L1 * L2);
  if (c2 < -1.0) c2 = -1.0; //Kosinus wird auf Bereich -1;1 eingegrenzt
  if (c2 >  1.0) c2 =  1.0;
  float s2p = sqrt(1.0 - c2 * c2); //Positive Wurzel = "Ellenbogen oben" 
  //Berechnung der zwei mathematisch möglichen Lösungen "Ellenbogen oben","Ellenbogen unten"
  float theta1a = (atan2(my,mx) - atan2( L2*s2p, L1+L2*c2)) * 57.2958;
  float theta2a =  atan2( s2p, c2) * 57.2958;

  float theta1b = (atan2(my,mx) - atan2(-L2*s2p, L1+L2*c2)) * 57.2958;
  float theta2b =  atan2(-s2p, c2) * 57.2958;
  //Entscheidung für optimale Lösung mit geringster Basisdrehung
  float theta1 = 0;
  float theta2 = 0;
  bool loesungA_ok = (fabs(theta1a) <= WMAX);
  bool loesungB_ok = (fabs(theta1b) <= WMAX);

  if (loesungA_ok && loesungB_ok)
  {
    //Lösung mit dem kleineren Betrag
    if (fabs(theta1a) <= fabs(theta1b))
    {
      theta1 = theta1a;
      theta2 = theta2a;
    } else 
    {
      theta1 = theta1b;
      theta2 = theta2b;
    }
  } else if (loesungA_ok)
  {
    theta1 = theta1a;
    theta2 = theta2a;
  } else if (loesungB_ok)
  {
    theta1 = theta1b;
    theta2 = theta2b;
  } else {
    if (!silent) out("ik nicht möglich");
    return false;
  }
  //Greifer-Kompensation - Hält den Greifer immer parallel zur X-Achse
  float phi = +(theta1 + theta2);
  while (phi >  180.0) phi -= 360.0;
  while (phi < -180.0) phi += 360.0;
  //Umrechnung der berechneten Winkel/Höhen in Schritte für die Motoren
  long p1 = lround(theta1 * SPD);
  long p2 = lround((Z0 - tz) * SPM);
  long p3 = lround(theta2 * SPD);
  long p4 = lround(phi    * SPD);
  //Softwarebeggrenzung der Motorschritte um mechanische Kollisionen zu vermeiden
  if (p1 < -2200 || p1 > 2200)   { if (!silent) out("Limit Motor1"); return false; }
  if (p2 < -22667|| p2 > 22667)  { if (!silent) out("Limit Motor2"); return false; }
  if (p3 < -1720 || p3 > 1720)   { if (!silent) out("Limit Motor3"); return false; }
  //Motoren bewegen zu Zielschritten
  Motor1->moveTo(p1); 
  Motor2->moveTo(p2); 
  Motor3->moveTo(p3);
  if (p4 >= -2400 && p4 <= 2400)
  {
    Motor4->moveTo(p4);
  }
  waitAll();
  if (!estop && !silent) 
  {
    out("Ok");
  }
  return !estop;
}

//Funktion zum Ausführen der Sequenz
void runSeq() {
  if (sequenzAnzahl == 0) { out("Sequenz leer"); return; }
  silent = true; //Serielle Ausgaben blockieren
  for (int i = 0; i < sequenzAnzahl && !estop && !loopStop; i++) 
  {
    ik(sequenz[i].x, sequenz[i].y, sequenz[i].z);
    if (sequenz[i].greiferWinkel >= 0) { 
      servoGripper.write(sequenz[i].greiferWinkel); 
      delay(500); 
    }
  }
  silent = false;
  if (!estop) out("Sequenz ok");
}
//Führt eingespeicherte Demo-Sequenz einmalig aus
void runDemo() {
  if (estop) return;
  silent = true;
  for (int i = 0; i < DEMO_N && !estop && !loopStop; i++)
  {
    ik(DEMO[i][0], DEMO[i][1], DEMO[i][2]);
    servoGripper.write((int)DEMO[i][3]); 
    delay(500);
  }
  silent = false;
  if (!estop) out("Demo ok");
}

//Hilfsfunktion zur Überprüfung von Loop-Abbrüchen
void loopIr() {
  String inc = "";
  if (Serial.available()) inc = Serial.readStringUntil('\n');
  if (BT.available())     inc = BT.readStringUntil('\n');
  inc.trim();
  if (inc == "stop")
  { 
    stopAll(); 
    digitalWrite(EN_PIN, HIGH); 
    estop = true; 
    out("Stop"); 
  }
  if (inc == "ls") 
  { 
    loopStop = true; 
    out("Loop Stop"); 
  }
}

//Parser (Befehlsverarbeitung)
void parse(String &c) 
{
  c.trim();
  if (c.length() == 0) return;
  //Globale Systembefehle
  if (c == "stop")   { stopAll(); digitalWrite(EN_PIN, HIGH); estop = true; loopStop = true; out("Stop"); return; }
  if (c == "enable") { estop = false; loopStop = false; digitalWrite(EN_PIN, LOW); setSpeeds(); out("Enable"); return; }
  if (estop) return; //Wet Notaus aktiv alle weiteren Befehle blockieren

  //Standard-Einzelbefehle
  if (c == "ls")    { loopStop = true; out("Loop Stop"); return; }
  if (c == "h")     { homing();  return; }
  if (c == "fk")    { fk();      return; }
  if (c == "run")   { runSeq(); return; }
  if (c == "demo")  { loopStop = false; runDemo(); return; }
  if (c == "clear") { sequenzAnzahl = 0; out("Clear"); return; }
  if (c == "del")   { if (sequenzAnzahl > 0) sequenzAnzahl--; out(String(sequenzAnzahl)); return; }

  //Sequenz loopen
  if (c == "loop") 
  {
    loopStop = false; out("Loop");
    while (!estop && !loopStop)
    {
      runSeq();
      loopIr();
    }
    loopStop = false;
    return;
  }

  //Demo wiederholen
  if (c == "demoloop")
  {
    loopStop = false; out("Demo Loop");
    while (!estop && !loopStop)
    {
      runDemo();
      loopIr();
    }
    loopStop = false;
    return;
  }
  //Ausgabe der aktuellen Sequenzdaten
  if (c == "seq") 
  {
    for (int i = 0; i < sequenzAnzahl; i++) 
    {
      out(String(i + 1) + ":" + String(sequenz[i].x, 1) + "," + String(sequenz[i].y, 1) + "," + String(sequenz[i].z, 1) + "," + sequenz[i].greiferWinkel);
    }
    return;
  }

  //Befehle für Greifer und Geschwindigkeitprofile
  if (c.startsWith("gp ")) { servoGripper.write(constrain(c.substring(3).toInt(), 0, 180)); return; }
  if (c.startsWith("sp ")) { spd = c.substring(3).toInt(); setSpeeds(); out("SP:" + String(spd)); return; }
  if (c.startsWith("ac ")) { acc = c.substring(3).toInt(); setSpeeds(); out("AC:" + String(acc)); return; }

  //Direktfahrbefehle Achsen (Slider-Eingaben der SCARA Control App)
  if (c.startsWith("a1 ")) 
  {
    long t = lround(c.substring(3).toFloat() * SPD);
    if (t >= -2200 && t <= 2200) { Motor1->moveTo(t); while(Motor1->isRunning()){} } return;
  }
  if (c.startsWith("a2 "))
  {
    long t = lround((Z0 - c.substring(3).toFloat()) * SPM);
    if (t >= -22667 && t <= 22667) { Motor2->moveTo(t); while(Motor2->isRunning()){} } return;
  }
  if (c.startsWith("a3 ")) 
  {
    long t = lround(c.substring(3).toFloat() * SPD);
    if (t >= -1715 && t <= 1715) { Motor3->moveTo(t); while(Motor3->isRunning()){} } return;
  }
  if (c.startsWith("a4 ")) 
  {
    long t = lround(c.substring(3).toFloat() * SPD);
    if (t >= -1200 && t <= 1200) { Motor4->moveTo(t); while(Motor4->isRunning()){} } return;
  }

  //Direktbefehl Inverse Kinematik
  if (c.startsWith("ik "))
  {
    int a = c.indexOf(' ', 3);
    int b = c.indexOf(' ', a + 1);
    if (a > 0 && b > 0) {
      ik(c.substring(3, a).toFloat(), c.substring(a + 1, b).toFloat(), c.substring(b + 1).toFloat());
    }
    return;
  }

  //Punkt in Sequenzliste einfügen
  if (c.startsWith("sik")) 
  {
    if (sequenzAnzahl >= 50) { out("Sequenz voll"); return; }
    String p = c.startsWith("sikg ") ? c.substring(5) : c.substring(4);
    int a = p.indexOf(' ');
    int b = p.indexOf(' ', a + 1);
    int d = p.indexOf(' ', b + 1);
    
    if (a < 0|| b < 0) return;
    
    float x = p.substring(0, a).toFloat();
    float y = p.substring(a + 1, b).toFloat();
    float z; 
    int greiferWinkel = -1;
    
    if (d > 0) { 
      z = p.substring(b + 1, d).toFloat(); 
      greiferWinkel = constrain(p.substring(d + 1).toInt(), 0, 180); 
    } else { 
      z = p.substring(b + 1).toFloat(); 
    }
    
    sequenz[sequenzAnzahl++] = {x, y, z, greiferWinkel};
    out("S:" + String(sequenzAnzahl));
    return;
  }
}

void setup() 
{
  Serial.begin(115200);
  BT.begin("SCARA_DJ01"); //Bluetooth-Name
  //Pin-Modes festlegen
  pinMode(EN_PIN, OUTPUT); 
  digitalWrite(EN_PIN, LOW);
  pinMode(LIM1, INPUT_PULLUP);
  pinMode(LIM2, INPUT_PULLUP);
  pinMode(LIM3, INPUT_PULLUP);
  pinMode(LIM4, INPUT); 
  //FastAccelStepper engine starten und Pins zuweisen
  eng.init();
  Motor1 = eng.stepperConnectToPin(Motor1_STEP); Motor1->setDirectionPin(Motor1_DIR);
  Motor2 = eng.stepperConnectToPin(Motor2_STEP); Motor2->setDirectionPin(Motor2_DIR);
  Motor3 = eng.stepperConnectToPin(Motor3_STEP); Motor3->setDirectionPin(Motor3_DIR);
  Motor4 = eng.stepperConnectToPin(Motor4_STEP); Motor4->setDirectionPin(Motor4_DIR);
  setSpeeds();
  //Servo intialisieren mit min und max Pulsbreite
  servoGripper.attach(SRV_PIN, 600, 2500); 
  servoGripper.write(100);
  //Hardware-Interrupts an Endschalter pinnen
  attachInterrupt(digitalPinToInterrupt(LIM1), isr1, RISING);
  attachInterrupt(digitalPinToInterrupt(LIM2), isr2, RISING);
  attachInterrupt(digitalPinToInterrupt(LIM3), isr3, RISING);
  attachInterrupt(digitalPinToInterrupt(LIM4), isr4, RISING);

  out("SCARA-DJ01 Bereit!");
}

void loop() 
//durchgehendes Prüfen ob Befehle reinkommen über USB oder Bluetooth
{
  if (Serial.available()) 
  { 
    String cmd = Serial.readStringUntil('\n'); 
    parse(cmd); 
  }
  if (BT.available()) 
  { 
    String cmd = BT.readStringUntil('\n');     
    parse(cmd); 
  }
}
