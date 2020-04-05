#include <ArduinoJson.h>
#include <M5Atom.h>
#include <wally_core.h>
#include <wally_crypto.h>
#include <utility/ccan/ccan/base64/base64.h>
#include <HTTPClient.h>
#include <iostream>
#include "Op.h"
#include "Timestamp.h"
#include "DetachedFile.h"
#include "Common.h"

#define FIRMWARE_NAME "Crypto"
#define FIRMWARE_VERSION "0.0.1"

const char* ssid = "";
const char* password =  "";

void setup() {
  M5.begin();
  Serial.begin(115200);
  while (!Serial) continue;

  uint64_t chipid;
  chipid = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(chipid >> 32);
  char chipid_str[20];
  snprintf(chipid_str, 20, "%04X%08X", chip, (uint32_t)chipid);

  StaticJsonDocument<1000> init_doc;
  init_doc["status"] = "init";
  init_doc["FIRMWARE_NAME"] = FIRMWARE_NAME ;
  init_doc["FIRMWARE_VERSION"] = FIRMWARE_VERSION ;
  init_doc["file"] = __FILE__;
  init_doc["compiled"] = __DATE__ ;
  init_doc["chipid"] = chipid_str ;
  Serial.println();
  serializeJson(init_doc, Serial);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
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
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
    return 1;
  }
}

void loop() {
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
  res = get_fragment(hashhex, "https://a.pool.eternitywall.com/digest", &fragment_1);
  res = get_fragment(hashhex, "https://ots.btc.catallaxy.com/digest", &fragment_2);

  newots = String("004f70656e54696d657374616d7073000050726f6f6600bf89e2e884e89294") + String("01") + String("08") + String(hashhex) + String("ff") + String(fragment_1) + String(fragment_2);
  Serial.print(newots);

  res = wally_free_string(hashhex);
  res = wally_free_string(fragment_1);
  res = wally_free_string(fragment_2);


  delay(1000);
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

  delay(100000);
}
