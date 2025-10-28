const byte buttonPin = 2;   // Knop met interrupt
const byte ledPin = 13;     // LED om status aan te tonen

// --- Variabelen ---
volatile bool buttonPressed = false;  // Wordt aangepast in interrupt
bool running = false;                 // Houdt de status bij

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);  // Interne pull-up gebruiken
  pinMode(ledPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);
  // Interrupt bij vallende flank (knop ingedrukt)

  Serial.begin(9600);
  Serial.println("Systeem gereed. Druk op de knop om te starten/stoppen.");
}

void loop() {
  if (buttonPressed) {
    buttonPressed = false;  // Reset de vlag

    // Toggle de status
    running = !running;
    digitalWrite(ledPin, running ? HIGH : LOW);

    Serial.print("Status: ");
    Serial.println(running ? "GESTART" : "GESTOPT");
  }

  // (optionele simulatie van lopend proces)
  if (running) {
    // Doe iets terwijl het systeem 'loopt'
    delay(100);  // Tijdelijke vertraging
  }
}

// --- Interrupt Service Routine ---
void buttonISR() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  // Eenvoudige debounce (200 ms)
  if (interruptTime - lastInterruptTime > 600) {
    buttonPressed = true;
    lastInterruptTime = interruptTime;
  }
}