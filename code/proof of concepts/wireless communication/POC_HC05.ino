#include <SoftwareSerial.h>

SoftwareSerial BT(3, 2); // RX, TX

char inkomend;


void setup() {
  pinMode(13, OUTPUT);         // LED pin
  Serial.begin(9600);          // Seriële monitor
  BT.begin(9600);              // Bluetooth standaard baudrate
  Serial.println("Bluetooth test gestart. Typ '1' om LED aan te zetten.");
}

void loop() {
    // Als er data van GSM (Bluetooth) komt
  if (BT.available()) {
    inkomend = BT.read();
    Serial.print("Ontvangen via GSM: ");
    Serial.println(inkomend);

    if (inkomend == '1') {
      digitalWrite(13, HIGH);
      Serial.println("LED AAN");
    } 
    else if (inkomend == '0') {
      digitalWrite(13, LOW);
      Serial.println("LED UIT");
    }
  }

  // Optioneel: stuur ook data terug naar GSM wat op de seriële monitor binnenkomt
  if (Serial.available()) {
    BT.write(Serial.read());
  }
}

