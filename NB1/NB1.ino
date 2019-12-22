#include <ESP8266WiFi.h>
#include <DHT.h> 

//서버에 전송하기 위한 초기설정
const char* server = "api.thingspeak.com";
String apiKey = "API KEY";
const char* MY_SSID = "WIFI NAME";
const char* MY_PWD = "WIFI PASSWD";

const int DHTPin = D4;//온습도센서핀
const float VRefer = 3.3; //전압
const int pinAdc = A0;//핀번호
DHT dht(DHTPin, DHT11); //DHT22(온습도센서)

float Temp = 0;
float Humi = 0;
float Oxy = 21.00;

// 초기 설정(시리얼 통신, 핀 설정)
void setup() {
	Serial.begin(115200);
	delay(10);
	dht.begin();//온습도센서 초기화

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
	float SumTemp = 0;
	float SumHumi = 0;
	float SumOxy = 0;
	int CountT = 0;
	int CountH = 0;
	int CountO = 0;

	//데이터 안정화
	for (int i = 0; i < 10; i++) {
		float Temp = dht.readTemperature();
		float Humi = dht.readHumidity();
		float Oxy = readConcentration();

		if (Temp != NAN) {
			SumTemp += Temp;
			CountT++;
		}

		if (Temp != NAN) {
			SumHumi += Humi;
			CountH++;
		}

		if (Oxy != NAN) {
			SumOxy += Oxy;
			CountO++;
		}

		delay(500);
	}

	Temp = SumTemp / CountT;
	Humi = SumHumi / CountH;
	Oxy = SumOxy / CountO;

	if (15 > Oxy || 25 < Oxy) {
		Oxy = 21.00;
	}

	Serial.print("온도 : ");
	Serial.println(Temp);
	Serial.print("습도 : ");
	Serial.println(Humi);
	Serial.print("산소 : ");
	Serial.println(Oxy);

	//온습도, 산소농도 데이터 서버 전송
	WiFiClient client;

	if (client.connect(server, 80)) { // 서버에 전송
		Serial.println("WiFi Client connected");

		String postStr = apiKey;
		postStr += "&field1=";
		postStr += String(Temp);
		postStr += "&field2=";
		postStr += String(Humi);
		postStr += "&field3=";
		postStr += String(Oxy);
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
	client.stop();
}

//산소 농도 측정
float readO2Vout()
{
	long sum = 0;
	for (int i = 0; i < 32; i++)
	{
		sum += analogRead(pinAdc);
	}

	sum >>= 5;

	float MeasuredVout = sum * (VRefer / 1023.0);
	return MeasuredVout;
}

float readConcentration()
{
	float MeasuredVout = readO2Vout();
	float Concentration = MeasuredVout * 0.21 / 2.0;
	float Concentration_Percentage = Concentration * 100;
	return Concentration_Percentage;
}
