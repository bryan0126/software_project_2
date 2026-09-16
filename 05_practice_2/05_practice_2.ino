#define LED 7
unsigned int count, toggle;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  while(!Serial) {
    ;  
  }
  count = 0;
  toggle = 0;
}

void loop() {
  digitalWrite(LED, toggle);
  delay(1000);
  digitalWrite(LED, toggle);
  delay(100);
  for(count = 1; count < 12; count++) {
    toggle = count % 2;
    digitalWrite(LED, toggle);
    delay(100);
  }
  while(1) {}
}
