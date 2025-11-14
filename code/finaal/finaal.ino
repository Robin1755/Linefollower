
// Library
#include "SerialCommand.h"
#include "EEPROMAnything.h"
#include <SoftwareSerial.h>

// Serial
const int btRxPin = 10; // Arduino RX, verbindt met TX van BT-module
const int btTxPin = 11; // Arduino TX, verbindt met RX van BT-module
SoftwareSerial Bluetooth(btRxPin, btTxPin); // Maak een virtuele seriële poort voor Bluetooth
#define SerialPort Bluetooth // Alles gaat nu via Bluetooth
#define Baudrate 9600
SerialCommand sCmd(SerialPort);

// PinOut
// DRV8833 Motor Driver
#define MotorLeft1 3
#define MotorLeft2 5
#define MotorRight1 6
#define MotorRight2 9
const int SleepPin = 4; // nSLEEP pin op de DRV8833, moet hoog zijn om te werken
const int Sensoren[] = {A7, A6, A5, A4, A3, A2, A1, A0};
const int StartKnop = 2;
const int LedBlauw = 12;
const int LedSensor = 13;

// Variabelen
int normalised[8];
bool StatusStartKnop;
bool debug;
volatile int run = 0; // Maak 'run' volatile en initialiseer op 0
float Error;
float LastError;
float Position;
float Output;
float iTerm = 0;
unsigned long Current, Previous, Difference, CalculationTime;
unsigned long CycleTime;
float PowerLeft = 0;
float PowerRight = 0;

struct Parameter_t
{
  unsigned long CycleTime;
  int Black[8];
  int White[8];
  int Power;
  float Diff;
  float Kp;
  float Ki;
  float Kd;
} Parameters;

// ISR voor de startknop. Houdt het zo kort mogelijk.
void Interrupt()
{
  // Debounce: negeer snelle bounces van de knop
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 500) // 500ms wachttijd
  {
    run = !run; // Wissel de run-status (start/stop)
    last_interrupt_time = interrupt_time;
  }
}

void onRun()
{
  run = 1;
}

void onStop()
{
  PowerLeft = 0;
  PowerRight = 0;
  digitalWrite(MotorLeft1, LOW);
  digitalWrite(MotorLeft2, LOW);
  digitalWrite(MotorRight1, LOW);
  digitalWrite(MotorRight2, LOW);
}

// Woord op de SerialPort = "calibrate"
void onCalibrate()
{
  char * Parameter = sCmd.next();

  if (strcmp(Parameter, "black") == 0)
  {
    SerialPort.println("Start calibrating black... ");
    for (int i = 0; i < 8; i++) Parameters.Black[i] = analogRead(Sensoren[i]);
    SerialPort.print("...Calibrating done");
  }

  if (strcmp(Parameter, "white") == 0)
  {
    SerialPort.println("Start calibrating white... ");
    for (int i = 0; i < 8; i++) Parameters.White[i] = analogRead(Sensoren[i]);
    SerialPort.print("...Calibrating done");
  }

  EEPROM_writeAnything(0, Parameters);
}

// Woord op de SerialPort = "set"
void onSet()
{
  char * Parameter = sCmd.next(); // 1ste argument
  char * Waarde = sCmd.next(); // 2de argument

  if (strcmp(Parameter, "cycle") == 0)
  {
    long NewCycleTime = atol(Waarde);
    float Ratio = ((float) NewCycleTime) / ((float) Parameters.CycleTime);

    Parameters.Ki *= Ratio;
    Parameters.Kd /= Ratio;

    Parameters.CycleTime = NewCycleTime;
  }

  else if (strcmp(Parameter, "ki") == 0)
  {
    float CycleTimeInSec = ((float) Parameters.CycleTime) / 1000000;
    Parameters.Ki = atof(Waarde) * CycleTimeInSec;
  }
  else if (strcmp(Parameter, "kd") == 0)
  {
    float CycleTimeInSec = ((float) Parameters.CycleTime) / 1000000;
    Parameters.Kd = atof(Waarde) / CycleTimeInSec;
  }

  else if (strcmp(Parameter, "power") == 0) Parameters.Power = atol(Waarde);
  else if (strcmp(Parameter, "diff") == 0) Parameters.Diff = atof(Waarde);
  else if (strcmp(Parameter, "kp") == 0) Parameters.Kp = atof(Waarde);

  EEPROM_writeAnything(0, Parameters);
}

// Woord op de SerialPort = "debug"
void onDebug()
{
  SerialPort.print("Cycle Time: ");
  SerialPort.println(Parameters.CycleTime);

  SerialPort.print("Output: ");
  SerialPort.println(Output);

  SerialPort.print("Error: ");
  SerialPort.println(Error);

  float CycleTimeInSec = ((float) Parameters.CycleTime) / 1000000;

  float Power = Parameters.Power;
  SerialPort.print("Power: ");
  SerialPort.println(Power);

  float Diff = Parameters.Diff;
  SerialPort.print("Diff: ");
  SerialPort.println(Diff);

  float Kp = Parameters.Kp;
  SerialPort.print("Kp: ");
  SerialPort.println(Kp);

  // volgende lijn weg
  float Ki = Parameters.Ki / CycleTimeInSec;
  //float Ki = Parameters.Ki;
  SerialPort.print("Ki: ");
  SerialPort.println(Ki);

  // volgende lijn weg
  float Kd = Parameters.Kd * CycleTimeInSec;
  //float Kd = Parameters.Kd;
  SerialPort.print("Kd: ");
  SerialPort.println(Kd);

  SerialPort.print("Normalised Values Sensoren: ");
  for (int i = 0; i < 8; i++)
  {
    normalised[i] = map(analogRead(Sensoren[i]), Parameters.Black[i], Parameters.White[i], 0, 1000);
    SerialPort.print(normalised[i]);
    SerialPort.print(" ");
  }
  SerialPort.println();

  SerialPort.print("Calculation Time: ");
  SerialPort.println(CalculationTime);
  CalculationTime = 0;
}

// Woord niet herkend op de SerialPort => Error
void onUnknownCommand(char * command)
{
  SerialPort.print("unknown command: \"");
  SerialPort.print(command);
  SerialPort.println("\"");
}


void setup()
{
  Serial.begin(Baudrate); // Start de USB-serial poort voor eventuele debug in de toekomst
  SerialPort.begin(Baudrate);

  // Set commando
  sCmd.addCommand("set", onSet);
  sCmd.addCommand("debug", onDebug);
  sCmd.addCommand("calibrate", onCalibrate);
  sCmd.addCommand("run", onRun);
  sCmd.addCommand("stop", onStop);
  sCmd.setDefaultHandler(onUnknownCommand);

  EEPROM_readAnything(0, Parameters);

  // Sensoren
  for (int i = 0; i < 7; i++)
  {
    pinMode(Sensoren[i], INPUT);
  }

  // Sensoren actief
  pinMode(LedSensor, OUTPUT);
  digitalWrite(LedSensor, HIGH);

  // Startknop
  pinMode(StartKnop, INPUT_PULLUP); // Gebruik interne pull-up weerstand
  attachInterrupt(digitalPinToInterrupt(StartKnop), Interrupt, FALLING); // Reageer als de knop naar GND wordt getrokken

  // LED
  pinMode(LedBlauw, OUTPUT);

  // Motoren
  pinMode(MotorLeft1, OUTPUT);
  pinMode(MotorLeft2, OUTPUT);
  pinMode(MotorRight1, OUTPUT);
  pinMode(MotorRight2, OUTPUT);
  pinMode(SleepPin, OUTPUT);
  digitalWrite(SleepPin, HIGH); // Activeer de DRV8833 driver

  SerialPort.println("Ready To Rumble");
}

void loop()
{
  sCmd.readSerial();

  // Als de robot niet draait, stop de motoren en reset de PID-termen.
  if (!run)
  {
    onStop();
    iTerm = 0;
    LastError = 0;
  }

  Current = micros();
  // Update de LED status gebaseerd op de 'run' variabele
  digitalWrite(LedBlauw, run ? HIGH : LOW);

  if (Current - Previous >= Parameters.CycleTime)
  {
    Previous = Current;

    for (int i = 0; i < 8; i++)
    {
      normalised[i] = map(analogRead(Sensoren[i]), Parameters.Black[i], Parameters.White[i], 0, 1000);
    }

    // === Nieuwe, robuustere positiebepaling ===
    // Gebaseerd op een gewogen gemiddelde van alle sensoren.
    // Dit is stabieler dan de vorige methode.
    float weightedSum = 0;
    float sumVal = 0;
    for (uint8_t i = 0; i < 8; i++) {
      // We gebruiken alleen sensoren die de lijn daadwerkelijk zien
      if (normalised[i] > 100) { // Drempelwaarde, kan aangepast worden
        weightedSum += (float)normalised[i] * i;
        sumVal += normalised[i];
      }
    }

    if (sumVal > 0) {
      // Bereken de positie van 0.0 tot 7.0
      Position = weightedSum / sumVal;
    }

    // Error = setpoint - input
    // We willen naar het midden (positie 3.5), dus de fout is het verschil.
    Error = Position - 3.5;

    // P regelaar
    Output = Error * Parameters.Kp;

    // I regelaar
    iTerm = 0;
    iTerm += Parameters.Ki * Error;
    iTerm = constrain(iTerm, -510, 510);
    Output += iTerm;

    // D regelaar
    LastError;
    Output += Parameters.Kd * (Error - LastError);
    LastError = Error;

    // Output begrenzen tot wat fysiek mogelijk is
    Output = constrain(Output, -510, 510);

    // Als alle sensoren wit zien (lijn kwijt), stop met corrigeren
    if (normalised[0] + normalised[1] + normalised[2] + normalised[3] + normalised[4] + normalised[5] + normalised[6] + normalised[7] < 200)
    {
      Output = 0;
    }

    if (run) 
    {
      if (Output >= 0)
      {
        PowerLeft = constrain((Parameters.Power + (Parameters.Diff * Output)), -255, 255);
        PowerRight = constrain((PowerLeft - Output), -255, 255);
        PowerLeft = PowerRight + Output;
      }
      else
      {
        PowerRight = constrain((Parameters.Power - (Parameters.Diff * Output)), -255, 255);
        PowerLeft = constrain((PowerRight + Output), -255, 255);
        PowerRight = PowerLeft - Output;
      }

      // Stuur de motoren aan
      if (PowerLeft > 0)     // Motor Links Vooruit
      {
        analogWrite(MotorLeft1, PowerLeft);
        digitalWrite(MotorLeft2, LOW);
      }
      else                   // Motor Links Achteruit
      {
        digitalWrite(MotorLeft1, LOW);
        analogWrite(MotorLeft2, -PowerLeft);
      }
      if (PowerRight > 0)    // Motor Rechts Vooruit
      {
        analogWrite(MotorRight1, PowerRight);
        digitalWrite(MotorRight2, LOW);
      }
      else                   // Motor Rechts Achteruit
      {
        digitalWrite(MotorRight1, LOW);
        analogWrite(MotorRight2, -PowerRight);
      }
    }
  }
  Difference = micros() - Current;
  if (Difference > CalculationTime) CalculationTime = Difference;
}
