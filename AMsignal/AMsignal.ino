void setup() {
  Serial.begin(9600);
  pinMode(11, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    int pwm = input.toInt();

    // Asegura que esté entre 0 y 255
    pwm = constrain(pwm, 0, 255);

    analogWrite(11, pwm);  // Aplica PWM al pin 11
  }
}

