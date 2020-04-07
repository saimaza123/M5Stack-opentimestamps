#include <M5Stack.h>
#undef min // workaroud for compiling problems due to unsafe macro
#include <WiFi.h>
#include <WiFiMulti.h>
#include <wally_core.h>
#include <wally_crypto.h>
#include <HTTPClient.h>
#include <iostream>
#include "Op.h"
#include "Timestamp.h"
#include "DetachedFile.h"
#include "Common.h"
#include "config.h"

#define ORIENTATION         1
#define BRIGHTNESS          100

WiFiMulti WiFiMulti;

void setup() {
  M5.begin();
  M5.Power.begin();

  // Lcd display
  M5.Lcd.setBrightness(BRIGHTNESS);
  Wire.begin();
  SD.begin();
  M5.Speaker.mute();
  M5.Lcd.println("M5Stack-opentimestamps");
  WiFiMulti.addAP(ssid, password);
  M5.Lcd.print("Connecting to WiFi");
  while(WiFiMulti.run() != WL_CONNECTED) {
    M5.Lcd.print(".");
    delay(500);
  }
  M5.Lcd.print(" Connected! Ip address: ");
  M5.Lcd.println(WiFi.localIP());
  M5.Lcd.println("A - Timestamp string 'pippo' 1 server");
  M5.Lcd.println("B - Timestamp string 'pippo' 2 servers");
  M5.Lcd.println("C - Decode an opentimestamps proof (crash)");
}


int get_fragment(char *hash, char *aggregator, char **fragment) {
  int res;
  HTTPClient http1;
  http1.begin(aggregator);
  http1.addHeader("Accept", "application/vnd.opentimestamps.v1");
  http1.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http1.POST(hash);
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
    String response = http1.getString();
    Serial.println(response);
    res = wally_hex_from_bytes((const unsigned char*)response.c_str(), response.length(), fragment);
    return 0;
  } else {
    M5.Lcd.println("Error on sending POST");
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
    return 1;
  }
}

void loop() {
  M5.update();
  if (M5.BtnA.wasReleased()) {
    M5.Lcd.println("--- one server ---");
    int res;
    unsigned char message[255] = "pippo";
    size_t messagelen = strlen((char*)message);
    size_t hashlen = SHA256_LEN;
    unsigned char hash[hashlen];
    res = wally_sha256(message, messagelen, hash, hashlen);
    char *hashhex;
    String newots;
    res = wally_hex_from_bytes(hash, hashlen, &hashhex);

    char *fragment_1;
    M5.Lcd.println("https://a.pool.eternitywall.com/digest");
    res = get_fragment(hashhex, "https://a.pool.eternitywall.com/digest", &fragment_1);

    newots = String("004f70656e54696d657374616d7073000050726f6f6600bf89e2e884e89294") + String("01") + String("08") + String(hashhex) +  String(fragment_1);
    Serial.print(newots);
    M5.Lcd.println(newots);

    res = wally_free_string(hashhex);
    res = wally_free_string(fragment_1);

  } else if (M5.BtnB.wasReleased()) {
    M5.Lcd.println("--- two servers ---");
    int res;
    unsigned char message[255] = "pippo";
    size_t messagelen = strlen((char*)message);
    size_t hashlen = SHA256_LEN;
    unsigned char hash[hashlen];
    res = wally_sha256(message, messagelen, hash, hashlen);
    char *hashhex;
    String newots;
    res = wally_hex_from_bytes(hash, hashlen, &hashhex);

    char *fragment_1;
    char *fragment_2;
    char *fragment_3;
    char *fragment_4;
    M5.Lcd.println("https://a.pool.eternitywall.com/digest");
    res = get_fragment(hashhex, "https://a.pool.eternitywall.com/digest", &fragment_1);
    M5.Lcd.println("https://ots.btc.catallaxy.com/digest");
    res = get_fragment(hashhex, "https://ots.btc.catallaxy.com/digest", &fragment_2);
    /*M5.Lcd.println("https://a.pool.opentimestamps.com/digest");
    res = get_fragment(hashhex, "https://a.pool.opentimestamps.org/digest", &fragment_3);
    M5.Lcd.println("https://b.pool.opentimestamps.com/digest");
    res = get_fragment(hashhex, "https://b.pool.opentimestamps.org/digest", &fragment_4);*/

    newots = String("004f70656e54696d657374616d7073000050726f6f6600bf89e2e884e89294") + String("01") + String("08") + String(hashhex) + String("ff") + String(fragment_1) + String("") + String(fragment_2);
    Serial.print(newots);
    M5.Lcd.println(newots);

    res = wally_free_string(hashhex);
    res = wally_free_string(fragment_1);
    res = wally_free_string(fragment_2);
    /*res = wally_free_string(fragment_3);
    res = wally_free_string(fragment_4);*/

  } else if (M5.BtnC.wasReleased()) {
    M5.Lcd.println("--- decode - hic sunt leones ---");
    // Try decode
    Serial.println("create ots");
    const std::string ots("004f70656e54696d657374616d7073000050726f6f6600bf89e2e884e89294010805c4f616a8e5310d19d938cfd769864d7f4ccdc2ca8b479b10af83564b097af9f010e754bf93806a7ebaa680ef7bd0114bf408f010b573e8850cfd9e63d1f043fbb6fc250e08f10457cfa5c4f0086fb1ac8d4e4eb0e70083dfe30d2ef90c8e2e2d68747470733a2f2f616c6963652e6274632e63616c656e6461722e6f70656e74696d657374616d70732e6f7267");

    //std::cout << ots << std::endl;

  	// DESERIALIZE
    Serial.println("DESERIALIZE");
  	unsigned char otsBytes [ots.length()/2];
  	ots::toBytes(ots, otsBytes);

  	const size_t len = ots.length()/2;
  	std::vector<unsigned char> buffer(otsBytes,otsBytes+len);
  	ots::Deserialize ctx(buffer);
    Serial.println("...");
  	std::unique_ptr <ots::DetachedFile> uniqueDetachedFile (ots::DetachedFile::deserialize(&ctx));
    Serial.println("end");
  	//std::cout << *uniqueDetachedFile << std::endl;

  	// SERIALIZE
    Serial.println("SERIALIZE");
  	ots::Serialize serialize;
  	uniqueDetachedFile->serialize(&serialize);
    Serial.println("end");
  	//std::cout << serialize << std::endl;
  }
}
