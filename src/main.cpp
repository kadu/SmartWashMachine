#include <SPI.h>
#include <Wire.h>
#include <Homie.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ClosedCube_HDC1080.h>
#include <SimpleTimer.h>
#include <OneButton.h>
#include <images.h>


#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
#define BUTTON_PIN     D3

const int sensorIn                = A0;
int mVperAmp                      = 66;
double Voltage                    = 0;
double VRMS                       = 0;
double AmpsRMS                    = 0;
bool laundryIsON                  = false;
bool laundryOffWasShowed          = false;
const int TEMPERATURE_INTERVAL    = 300;
const int laundryOffDelay         = 30000; //seconds
unsigned int countBoot            = 0;
unsigned long lastTemperatureSent = 0;
unsigned long lastLaundryOn       = 0;
float Wattage;


int buttonState = 0;
bool buttonPressed = false;

HomieNode temperatureNode("temperature", "Temperature", "temperature");
HomieNode humidityNode("humidity","Humidity","humidity");
HomieNode laundryNode("laundry","Laundry","laundry");
HomieNode clothesLineNode("clothes-line","Clothes-line","button");


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OneButton button(BUTTON_PIN, true);
ClosedCube_HDC1080 hdc1080;
SimpleTimer timer;

float getVPP() {
  float result;

  int readValue = 0; //value read from the sensor
  int maxValue = 0; // store max value here
  int minValue = 1024; // store min value here

  uint32_t start_time = millis();

  while((millis()-start_time) < 100) {
    readValue = analogRead(sensorIn);
    if (readValue > maxValue) {
      maxValue = readValue;
    }
    if (readValue < minValue) {
      minValue = readValue;
    }

  }
  /*
    Serial.print(readValue);
    Serial.println(" readValue ");
    Serial.print(maxValue);
    Serial.println(" maxValue ");
    Serial.print(minValue);
    Serial.println(" minValue ");*/
    // delay(1000);

  // Subtract min from max
  result = ((maxValue - minValue) * 5)/1024.0;

  return result;
}

void updateACS712() {
  Voltage = getVPP();
  VRMS = (Voltage/2.0) *0.707; // sq root
  AmpsRMS = (VRMS * 1000)/mVperAmp;
  Wattage = (220*AmpsRMS)-18; //Observed 18-20 Watt when no load was connected, so substracting offset value to get real consumption.

  if(AmpsRMS < 0.35) {
    AmpsRMS = 0;
    Wattage = 0;
  }
}

void clearDisplay() {
  display.clearDisplay();
}

void displayTemperature() {
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(hdc1080.readTemperature());
  display.setTextSize(1);
  display.println(F("      graus celcius\n"));
  display.setTextSize(2);
  display.print(hdc1080.readHumidity());
  display.println(" % rh");
  display.display();
}

void clotheslineMessage() {
    display.clearDisplay();
    display.drawBitmap(0, 0, myBitmap7, 128, 64, 1);
    display.display();
    timer.setTimeout(5000,  displayTemperature);
}

void sendClothesLineCheck() {
  if(laundryOffWasShowed) {
    clothesLineNode.setProperty("clothes-line").send("CHECKED");
    lastLaundryOn = 0;
    laundryOffWasShowed = false;
  }
}

void loopHandler() {
  timer.run();
  button.tick();

  if (millis() - lastTemperatureSent >= TEMPERATURE_INTERVAL * 1000UL || lastTemperatureSent == 0) {
    temperatureNode.setProperty("degrees").send(String(hdc1080.readTemperature()));
    humidityNode.setProperty("humidity").send(String(hdc1080.readHumidity()));
    Homie.getLogger() << "Temperature: " << hdc1080.readTemperature() << " °C" << endl;
    Homie.getLogger() << "Humidity: " << hdc1080.readHumidity() << " % rH" << endl;
    Serial.print("Temperature: "); Serial.print(hdc1080.readTemperature()); Serial.println(" degrees C");
    Serial.print("Humidity: "); Serial.print(hdc1080.readHumidity()); Serial.println("% rH");
    displayTemperature();
    lastTemperatureSent = millis();
  }

  // Serial.printf("Amps[%0.2f] - laundryIsON [%s] - laundryOffWasShowed [%s]\n", AmpsRMS, laundryIsON? "T" : "F", laundryOffWasShowed? "T" : "F");
  if(AmpsRMS > 0) {
    lastLaundryOn = millis();
  }

  if((AmpsRMS > 0) && (laundryIsON == false)) {
    display.clearDisplay();
    display.drawBitmap(0, 0, myBitmap5, 128, 64, 1);
    display.display();
    timer.setTimeout(5000, displayTemperature);
    laundryIsON = true;
    laundryOffWasShowed = false;
    laundryNode.setProperty("laundry").send("true");
    clothesLineNode.setProperty("clothes-line").send("UNCHECKED");
  }

  if((laundryOffWasShowed == false) && (AmpsRMS == 0) && (lastLaundryOn + laundryOffDelay > millis()) && (lastLaundryOn > 0)) {
    Serial.println("entrou");

    Serial.printf("lastLaundryOn [%d] - laundryOffDelay [%d] - millis [%d]", lastLaundryOn, laundryOffDelay, millis());
    display.clearDisplay();
    display.drawBitmap(0, 0, myBitmap6, 128, 64, 1);
    display.display();
    timer.setTimeout(5000,  displayTemperature);
    timer.setTimeout(10000, clotheslineMessage);
    timer.setTimeout(15000,  displayTemperature);
    laundryNode.setProperty("laundry").send("false");
    laundryOffWasShowed = true;
    laundryIsON = false;
    Serial.println(" laundryOffWasShowed ta no true");
  }
}

void setupHandler() {
  hdc1080.begin(0x40);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.drawBitmap(0, 0, myBitmap2, 128, 64, 1);
  display.display();
  delay(700);
  display.clearDisplay();
  display.drawBitmap(0, 0, myBitmap4, 128, 64, 1);
  display.display();
  button.attachClick(sendClothesLineCheck);
  timer.setInterval(15000, updateACS712);
}

void setup() {
  Serial.begin(115200);
  pinMode(A0, INPUT);

  Serial << endl << endl;
  Homie_setFirmware("awesome-temperature", "1.0.0");
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);

  temperatureNode.advertise("degrees").setName("Degrees")
                                      .setDatatype("float")
                                      .setUnit("ºC");
  humidityNode.advertise("humidity").setName("Humidity")
                                      .setDatatype("float")
                                      .setUnit("% rH");
  laundryNode.advertise("laundry").setName("Laundy")
                                      .setDatatype("boolean");
  clothesLineNode.advertise("clothes-line").setName("Button")
                                .setDatatype("enum")
                                .setFormat("CHECKED,UNCHECKED")
                                .setRetained(false);

  Homie.setup();
}

void loop() {
  Homie.loop();
}