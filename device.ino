#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_MLX90614.h>
#include <Arduino.h>
#include "HX711.h"
#include "soc/rtc.h"

long irValue;
bool b = false;
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

#define trigPin 12
#define echoPin 13
#define LOADCELL_DOUT_PIN 16
#define LOADCELL_SCK_PIN 4

long duration;
float distance, reading, lastReading, BMI;

//REPLACE WITH YOUR CALIBRATION FACTOR
#define CALIBRATION_FACTOR -471.497

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
MAX30105 particleSensor;
HX711 scale;

void setup() {

  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1);
  };
  if (!particleSensor.begin()) //Use default I2C port, 400kHz speed //Wire, I2C_SPEED_FAST
  {
    Serial.println("MAX30102 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  rtc_cpu_freq_config_t config;
  rtc_clk_cpu_freq_get_config(&config);
  rtc_clk_cpu_freq_to_config(RTC_CPU_FREQ_80M, &config);
  rtc_clk_cpu_freq_set_config_fast(&config);

  Serial.println("Initializing the scale");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();

}

void loop() {

  //  max30102Read();
  //  mlxRead();
  hightRead();
  scaleRead();
  BMICAL();
  delay(1000);

}
void max30102Read() {

  irValue = particleSensor.getIR();

  if (irValue < 50000) {
    Serial.print(" No finger?");
  } else {
    b = true;

  }
  while (b) {

    irValue = particleSensor.getIR();

    if (checkForBeat(irValue) == true)
    {
      //We sensed a beat!
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20)
      {
        rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
        rateSpot %= RATE_SIZE; //Wrap variable

        //Take average of readings
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }

    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.print(beatAvg);



    if (irValue < 50000) {
      Serial.print(" No finger?");
      b = false;
    }
    Serial.println();
  }

  Serial.println();
}
void mlxRead() {
  int body_t = mlx.readObjectTempC();
  Serial.print("Object temperature = ");
  Serial.print(mlx.readObjectTempC());
  Serial.println("°C");
}
void hightRead() {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  distance = distance / 100;

  Serial.print("Distance M: ");
  Serial.println(distance);
}
void scaleRead() {
  if (scale.wait_ready_timeout(200)) {
    reading = round(scale.get_units());
    reading = reading / 1000;
    if (reading != lastReading) {
      Serial.print("Weight: ");
      Serial.println(reading);
    }
    lastReading = reading;
  }
  else {
    Serial.println("HX711 not found.");
  }
}
void BMICAL() {
  BMI = reading / (distance * distance);
  Serial.print("BMI: ");
  Serial.println(BMI);

  if (BMI < 18.5) {
    Serial.println("Under weight");
  } else if (BMI > 18.5 && BMI < 24.9) {
    Serial.println("Normal weight");
  } else if (BMI > 25.0 && BMI < 29.9) {
    Serial.println("Over weight");
  } else if (BMI > 30.0 && BMI < 34.9) {
    Serial.println("Obesity (Class 1)");
  } else if (BMI > 35.0 && BMI < 39.9) {
    Serial.println("Obesity (Class 2)");
  } else if (BMI > 40) {
    Serial.println("Obesity (Class 3)");
  }
}
