#include <WiFi.h>
#include <WebServer.h>
#include <Stepper.h>

// WiFi bilgileri
const char* ssid = "Aleyna iPhone'u";
const char* password = "alina1601";

// Stepper motor ayarları
const int stepsPerRevolution = 200;
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11); // D8-D11 (ESP32 GPIO'lara göre ayarla)

// Giriş/Çıkış pinleri
const int buttonPin = 2;  // GPIO2 - Buton
const int ledPin = 3;     // GPIO3 - LED

// Ultrasonik sensör pinleri
const int trigPin = 5;
const int echoPin = 18;

// Besleme parametreleri
const int yaprakAcisi = 10;
const int birBeslemeGramaji = 20;
const int beslemeAlaniGramaji = 5;

// Web sunucu
WebServer server(80);

int gecikmeSuresi = 0; // saniye cinsinden

// Ultrasonik mesafe ölçüm fonksiyonu
long olcMesafe() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long sure = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
  if (sure == 0) return -1; // timeout olursa -1 döndür
  long mesafe = sure * 0.034 / 2; // cm cinsinden mesafe

  return mesafe;
}

void mamaVer() {
  long mesafe = olcMesafe();
  Serial.print("Ölçülen mesafe: ");
  Serial.print(mesafe);
  Serial.println(" cm");

  if (mesafe > 0 && mesafe < 10) {  // Mama kabı dolu (mesafe 10 cm'den küçük)
    digitalWrite(ledPin, HIGH);
    int adimSayisi = birBeslemeGramaji / beslemeAlaniGramaji;
    for (int i = 0; i < adimSayisi; i++) {
      myStepper.step(yaprakAcisi);
      delay(1000);
    }
    digitalWrite(ledPin, LOW);
  } else { // Boş
    Serial.println("Mama kabı boş! Besleme yapılamaz.");
    server.send(200, "text/html", "<html><body><h1>Mama kabı boş! Lütfen doldurun.</h1><a href='/'>Geri dön</a></body></html>");
  }
}

void handleRoot() {
  long mesafe = olcMesafe();
  String durum = (mesafe > 0 && mesafe < 10) ? "DOLU" : "BOŞ";

  server.send(200, "text/html", R"rawliteral(
    <html>
    <head><title>Akıllı Mama Kabı</title></head>
    <body>
      <h1>Akıllı Mama Kabı</h1>
      <p><b>Mama Kabı Durumu:</b> )rawliteral" + durum + R"rawliteral(</p>
      <form action="/set" method="GET">
        <label>Zaman (saniye):</label>
        <input type="number" name="zaman" min="1" max="3600">
        <input type="submit" value="Zamanla">
      </form>
      <p><a href="/start"><button>Şimdi Yem Ver</button></a></p>
    </body>
    </html>
  )rawliteral");
}

void handleStart() {
  mamaVer();
  // MamaVer fonksiyonu kendi içinde cevap döndürüyor; 
  // ama handleStart için bir cevap da verelim:
  server.send(200, "text/html", "<html><body><h1>Yem verme işlemi gerçekleştirildi.</h1><a href='/'>Geri dön</a></body></html>");
}

void handleSetZaman() {
  if (server.hasArg("zaman")) {
    gecikmeSuresi = server.arg("zaman").toInt();
    server.send(200, "text/html", "<html><body><h1>Zaman ayarlandı: " + String(gecikmeSuresi) + " saniye</h1><a href='/'>Geri dön</a></body></html>");
    delay(gecikmeSuresi * 1000);
    mamaVer();
  } else {
    server.send(400, "text/plain", "Eksik argüman.");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  myStepper.setSpeed(60);

  WiFi.begin(ssid, password);
  Serial.print("WiFi baglaniyor");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi baglandi!");
  Serial.print("IP adresi: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.on("/set", handleSetZaman);
  server.begin();
}

void loop() {
  server.handleClient();

  if (digitalRead(buttonPin) == LOW) {
    mamaVer();
    delay(1000); // buton sıçramasını önle
  }
}
