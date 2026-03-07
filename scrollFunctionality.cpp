const int joyX = A0;
const int joyY = A1;
const int joySW = 7;

void setup() {
  Serial.begin(115200);
  pinMode(joySW, INPUT_PULLUP);
}

void loop() {
  int x = analogRead(joyX);
  int y = analogRead(joyY);
  int sw = digitalRead(joySW);

  Serial.print("X: ");
  Serial.print(x);
  Serial.print(" | Y: ");
  Serial.print(y);
  Serial.print(" | SW: ");
  Serial.println(sw);

  delay(50);
}