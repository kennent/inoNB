#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h> 

//서버에 전송하기 위한 초기설정
const char* server = "api.thingspeak.com";
String apiKey = "API KEY";//api key
const char* MY_SSID = "Open Hive";//와이파이 이름
const char* MY_PWD = "ground21_open";//와이파이 비밀번호

const int EchoPin = D1;// Echo핀
const int TrigPin = D2;//Trig핀
const int RST_PIN = D3;// reset핀
const int SS_PIN = D4;// SS핀

unsigned long Start_time = 0;
unsigned long End_time = 0;
double Stream_time = 0;

MFRC522 mfrc(SS_PIN, RST_PIN); //RIFD

// 초기 설정(시리얼 통신, 핀 설정)
void setup() {
	Serial.begin(115200);
	pinMode(TrigPin, OUTPUT);
	pinMode(EchoPin, INPUT);
	SPI.begin();//RFID 초기화 
	mfrc.PCD_Init();//초기화
	delay(10);

	//와이파이 연결
	Serial.print("Connecting to " + *MY_SSID);

	WiFi.begin(MY_SSID, MY_PWD);

	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("Connected");
	Serial.println("");
}

void loop() {
	int Tag = RFID();
	int Length = SR04();

	//RFID Tag를 인식시 서버에 전송
	if (Tag != 0) {
		Serial.print("ID : ");
		Serial.println(Tag);
		Tag_Value(Tag);
	}

	//거리가 20cm 이상일시 문이 열린걸로 인식, 서버에 전송
	if (Length > 25) {
		Start_time = millis();//시작 시간

		//문이 열린 시간 측정
		while (SR04() > 25)
		{
			Tag = RFID();

			//시간 측정시 RFID Tag가 접속되면 서버에 전송
			if (Tag != 0) {
				Serial.print("ID : ");
				Serial.println(Tag);
				Tag_Value(Tag);
			}

			Serial.print("CM :");
			Serial.println(Length);
		}

		End_time = millis();//종료 시간
		Stream_time = End_time - Start_time;//시작 시간 - 종료 시간 = 실행 시간
		Stream_time /= 1000.0;//밀리초를 초로 변환

		//시간이 1초 이상일때 서버에 전송
		if (Stream_time >= 1) {
			Serial.print(Stream_time);
			Serial.println("s");
			Length_Value(Stream_time); 
		}
	}
}

//RFID태그 읽기
int RFID() {
	int Tag = 0;

	if (mfrc.PICC_IsNewCardPresent())
	{
		if (mfrc.PICC_ReadCardSerial())
		{
			Tag = mfrc.uid.uidByte[0];
			//Serial.print("Tag UID :");
			//Serial.println(Tag);
		}
	}

	return Tag;
}

//거리 측정
int SR04() {
	long duration, distance;
	digitalWrite(TrigPin, LOW);
	delayMicroseconds(2);
	digitalWrite(TrigPin, HIGH);
	delayMicroseconds(10);
	digitalWrite(TrigPin, LOW);
	duration = pulseIn(EchoPin, HIGH);
	distance = duration * 17 / 1000;
	//Serial.println(distance);
	delay(500);

	return distance;
}

//RFID Tag 데이터 전송
void Tag_Value(int Tag) {
	WiFiClient client;

	if (client.connect(server, 80)) { // 서버에 전송
		Serial.println("WiFi Client connected");

		String postStr = apiKey;;
		postStr += "&field1=";
		postStr += String(Tag);
		postStr += "&field2=";
		postStr += NAN;
		postStr += "\r\n\r\n";

		client.print("POST /update HTTP/1.1\n");
		client.print("Host: api.thingspeak.com\n");
		client.print("Connection: close\n");
		client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
		client.print("Content-Type: application/x-www-form-urlencoded\n");
		client.print("Content-Length: ");
		client.print(postStr.length());
		client.print("\n\n");
		client.print(postStr);
		delay(1000);
	}
	Serial.println("Transfer completed");
	client.stop();
}

//거리, 문열림 데이터 전송
void Length_Value(double Length) {
	WiFiClient client;

	if (client.connect(server, 80)) { // 서버에 전송
		Serial.println("WiFi Client connected");

		String postStr = apiKey;
		postStr += "&field1=";
		postStr += NAN;
		postStr += "&field2=";
		postStr += String(Length);
		postStr += "\r\n\r\n";

		client.print("POST /update HTTP/1.1\n");
		client.print("Host: api.thingspeak.com\n");
		client.print("Connection: close\n");
		client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
		client.print("Content-Type: application/x-www-form-urlencoded\n");
		client.print("Content-Length: ");
		client.print(postStr.length());
		client.print("\n\n");
		client.print(postStr);
		delay(1000);
	}
	Serial.println("Transfer completed");
	client.stop();
}
