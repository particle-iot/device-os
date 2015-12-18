// This firmware runs on a Photon/Core and will proxy all
// `Serial1` <-> `Serial` Communications, so we can use a device as an ad-hoc
// FTDI helper.

void setup() {
  Serial1.begin(9600);
  Serial.begin(9600);
}

void loop() {
  while(Serial1.available()) {
    Serial.write(Serial1.read());
  }

  while(Serial.available()) {
    Serial1.write(Serial.read());
  }
}
