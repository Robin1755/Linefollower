#define MOT_A1_PIN 9
#define MOT_A2_PIN 6
#define MOT_B1_PIN 5
#define MOT_B2_PIN 3
#define SLEEP_PIN 2

const int MAX_PWM = 255;        // maximale snelheid
const int CYCLE_TIME = 10000;    // tijd voor één volledige heen-en-weer beweging (ms)
const int UPDATE_INTERVAL = 5;  // updatefrequentie (ms) -> hoe kleiner, hoe vloeiender

void setup() {
  pinMode(MOT_A1_PIN, OUTPUT);
  pinMode(MOT_A2_PIN, OUTPUT);
  pinMode(MOT_B1_PIN, OUTPUT);
  pinMode(MOT_B2_PIN, OUTPUT);

  // SLEEP-pin aanzetten
  pinMode(SLEEP_PIN, OUTPUT);
  digitalWrite(SLEEP_PIN, HIGH);  // >>> MOET HOOG BLIJVEN <<<

  stopMotors();

  Serial.begin(9600);
  Serial.println("Start vloeiende motorbeweging vooruit en achteruit...");
}

void loop() {
  // Continue vloeiende sinusgolf: vooruit én achteruit
  smoothBidirectionalMotion();
}

// ---------------------------------------------
// Functie: vloeiende sinusbeweging in beide richtingen
// ---------------------------------------------
void smoothBidirectionalMotion() {
  unsigned long startTime = millis();

  while (true) {
    unsigned long elapsed = (millis() - startTime) % CYCLE_TIME; // loopt eindeloos door
    float t = (float)elapsed / CYCLE_TIME;                       // 0.0 – 1.0

    // Sinusvormige beweging over volledige 2π: 
    // sin(0) = 0, sin(π/2) = 1, sin(π) = 0, sin(3π/2) = -1, sin(2π) = 0
    // Daardoor: vooruit -> stop -> achteruit -> stop -> herhaal
    float speedFactor = sin(t * TWO_PI);

    // Bereken PWM (positief = vooruit, negatief = achteruit)
    int pwmValue = (int)(MAX_PWM * speedFactor);

    setMotorSpeed(pwmValue, pwmValue);
    delay(UPDATE_INTERVAL);
  }
}

// ---------------------------------------------
// Motor aansturing
// ---------------------------------------------
void setMotorSpeed(int pwmA, int pwmB) {
  setMotorPWM(pwmA, MOT_A1_PIN, MOT_A2_PIN);
  setMotorPWM(pwmB, MOT_B1_PIN, MOT_B2_PIN);

  Serial.print("PWM A = ");
  Serial.print(pwmA);
  Serial.print(" | PWM B = ");
  Serial.println(pwmB);
}

void setMotorPWM(int pwm, int IN1_PIN, int IN2_PIN) {
  if (pwm > 0) { // vooruit
    digitalWrite(IN1_PIN, LOW);
    analogWrite(IN2_PIN, pwm);
  } else if (pwm < 0) { // achteruit
    analogWrite(IN1_PIN, -pwm);
    digitalWrite(IN2_PIN, LOW);
  } else { // stilstand
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
  }
}

void stopMotors() {
  digitalWrite(MOT_A1_PIN, LOW);
  digitalWrite(MOT_A2_PIN, LOW);
  digitalWrite(MOT_B1_PIN, LOW);
  digitalWrite(MOT_B2_PIN, LOW);
}