#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 18
#define NUM_LEDS 8
#define BTN_PIN 5
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "Wokwi-GUEST"; // questo poi deve essere sostituito con il nome della rete WiFi a cui ci si vuole connettere 
const char* password = ""; //idem password rete WIFI


const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic  = "friendship_lamp_2026/trigger";

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


bool isScreenActive = false;
unsigned long activeStartTime = 0;
const unsigned long displayDuration = 7000; // Resta accesa per 7 secondi

bool lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;

// Fiocchetto bianco sull'orecchio (16x12 pixel)
const unsigned char bowBitmap[] PROGMEM = {
  0b01110000, 0b00001110,
  0b11111000, 0b00011111,
  0b11111101, 0b10111111,
  0b01111111, 0b11111110,
  0b00111111, 0b11111100,
  0b00011110, 0b01111000,
  0b00111111, 0b11111100,
  0b01111111, 0b11111110,
  0b11111101, 0b10111111,
  0b11001100, 0b00110011,
  0b10000100, 0b00100001,
  0b00000000, 0b00000000
};
void callback(char* topic, byte* payload, unsigned int length) {
  isScreenActive = true;
  activeStartTime = millis();
}
void recconect() {
  while(!client.connected()){
    Serial.print("Tentativo connessione MQTT...");
    String clientId = "ESP32Client-"+String(random(0xffff), HEX);
    if(client.connect(clientId.c_str())){
      Serial.println(" CONNESSO AL BROKER!");
      client.subscribe(mqtt_topic);
      Serial.println("Iscritto al canale: " + String(mqtt_topic));
    } else {
      Serial.print(" Fallito, codice errore rc=");
      Serial.print(client.state());
      Serial.println(" Riprovo tra 2 secondi...");
      delay(2000);
    }
  }
}
void drawKitty(bool blinkEyes) {
  display.clearDisplay();

  // 1. Orecchie a punta
  display.fillTriangle(24, 28, 38, 2, 48, 24, SSD1306_WHITE);   // Orecchio sinistro
  display.fillTriangle(28, 26, 38, 8, 44, 22, SSD1306_BLACK);   // Interno orecchio sx
  display.fillTriangle(80, 24, 90, 2, 104, 28, SSD1306_WHITE);  // Orecchio destro
  display.fillTriangle(84, 22, 90, 8, 100, 26, SSD1306_BLACK);  // Interno orecchio dx

  // 2. Fiocchetto sull'orecchio sinistro
  display.drawBitmap(28, 4, bowBitmap, 16, 12, SSD1306_WHITE);

  // 3. Testa tonda e morbida della gattina
  display.fillRoundRect(22, 16, 84, 46, 22, SSD1306_WHITE);

  // 4. Occhi grandi espressivi
  if (blinkEyes) {
    // Occhietti chiusi a mezza luna felice ^ ^
    display.drawCircle(44, 34, 7, SSD1306_BLACK);
    display.fillRect(36, 34, 16, 8, SSD1306_WHITE);
    display.drawCircle(84, 34, 7, SSD1306_BLACK);
    display.fillRect(76, 34, 16, 8, SSD1306_WHITE);
  } else {
    // Occhioni grandissimi rotondi aperti
    display.fillCircle(44, 34, 10, SSD1306_BLACK);
    display.fillCircle(84, 34, 10, SSD1306_BLACK);

    // Riflessi di luce/pupille carine nei due occhi
    display.fillCircle(42, 31, 3, SSD1306_WHITE);
    display.fillCircle(46, 37, 1, SSD1306_WHITE);
    display.fillCircle(82, 31, 3, SSD1306_WHITE);
    display.fillCircle(86, 37, 1, SSD1306_WHITE);
  }

  // 5. Nasino a triangolino rovesciato
  display.fillTriangle(62, 42, 66, 42, 64, 45, SSD1306_BLACK);

  // 6. Musetto e bocca a forma di :3
  display.drawLine(64, 45, 64, 48, SSD1306_BLACK);
  // Curva sx
  display.drawPixel(63, 49, SSD1306_BLACK);
  display.drawPixel(62, 50, SSD1306_BLACK);
  display.drawPixel(61, 50, SSD1306_BLACK);
  display.drawPixel(60, 49, SSD1306_BLACK);
  // Curva dx
  display.drawPixel(65, 49, SSD1306_BLACK);
  display.drawPixel(66, 50, SSD1306_BLACK);
  display.drawPixel(67, 50, SSD1306_BLACK);
  display.drawPixel(68, 49, SSD1306_BLACK);

  // 7. Baffetti ai lati
  // Lato sinistro
  display.drawLine(14, 40, 26, 42, SSD1306_WHITE);
  display.drawLine(12, 46, 26, 46, SSD1306_WHITE);
  display.drawLine(14, 52, 26, 50, SSD1306_WHITE);
  // Lato destro
  display.drawLine(114, 40, 102, 42, SSD1306_WHITE);
  display.drawLine(116, 46, 102, 46, SSD1306_WHITE);
  display.drawLine(114, 52, 102, 50, SSD1306_WHITE);

  display.display();
}

void setup() {
  pixels.begin();
  pixels.clear();
  pixels.show(); // Parte spento
  Serial.begin(115200);
  Serial.println("\n--- AVVIO SISTEMA ---");
  pinMode(BTN_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("ERRORE: Schermo OLED non trovato!");
    for (;;);
  }

  display.clearDisplay();
  display.display(); // Parte spento/nero
  Serial.print("Connessione a WiFi in corso");
  // Tentativo di connessione con timeout (max 10 secondi)
  WiFi.begin(ssid, password);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
  }
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      recconect();
    }
    client.loop();
  }

  int reading = digitalRead(BTN_PIN);

  // Pressione tasto: riceve il segnale e sveglia lo schermo
  if (reading == LOW && lastBtnState == HIGH && (millis() - lastDebounceTime > 250)) {
    lastDebounceTime = millis();
    isScreenActive = true;
    activeStartTime = millis();
    Serial.println("Pulsante premuto! Invio pacchetto MQTT...");
    if (WiFi.status() == WL_CONNECTED && client.connected()) {
    client.publish(mqtt_topic, "TOUCH");
}
  
  }
  lastBtnState = reading;

  if (isScreenActive) {
    // accende i led 
    for (int i = 0; i < NUM_LEDS; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 140, 40));
    }
    pixels.show();

    // Sbatte gli occhi ogni ~2 secondi
    bool blink = ((millis() - activeStartTime) % 2000) < 220;
    drawKitty(blink);

    // Dopo 7 secondi si rispegne
    if (millis() - activeStartTime >= displayDuration) {
      isScreenActive = false;
      display.clearDisplay();
      display.display();

      //spegne i led
      pixels.clear();
      pixels.show();
    }
  }
}