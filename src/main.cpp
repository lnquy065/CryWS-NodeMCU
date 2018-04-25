#include <Arduino.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//WIFI setup
const char *ssid = "Quy";
const char *pass = "0937437982";

//JSON setup
DynamicJsonBuffer  jsonBuffer(500);

//Oled setup
//U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0,  U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R0, 10, D4, D6);

unsigned int screenRefeshTime = 60;
unsigned long lastRefesh = screenRefeshTime*1000;
const unsigned char dollar [] = {0x10, 0x38, 0x54, 0x70, 0x1C, 0x54, 0x38, 0x10, };
const unsigned char rate_down [] = {
0x00, 0x00, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0,
0x01, 0xC0, 0x01, 0xC0, 0x0F, 0xF8, 0x07, 0xF0, 0x03, 0xE0, 0x01, 0xC0, 0x00, 0x80, 0x00, 0x00
};
const unsigned char rate_up [] = {
0x00, 0x00, 0x00, 0x80, 0x01, 0xC0, 0x03, 0xE0, 0x07, 0xF0, 0x0F, 0xF8, 0x01, 0xC0, 0x01, 0xC0,
0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0xC0, 0x00, 0x00
};
unsigned char ic1[32];
unsigned char ic2[32];
unsigned char chart7days[8];

char scrollbar = 3;

unsigned int LIST_SCREEN = 0;
unsigned int CHART_SCREEN = 1;


//API declare
char* apiCoins = "http://cryws.herokuapp.com/api/coins/tiny/icon/offset/";
char* apiChart = "http://cryws.herokuapp.com/api/coins/tiny/chart7days/";

//functions header
void oled_coinsList_render(String);
String http_request(String);
void oled_chart_render(String);



//button
int ledPin = D3;
const int next = D2, prev = D8, chart1 = D1, chart2 = D0;
unsigned long timeShortPress = 300;//ms
unsigned long timeLongPress = 500;//ms 
int btnArray[] = {next, prev, chart1, chart2};
bool btnState[4] = {false};
bool btnLastState[4] = {false};
unsigned long btnLastPressMillis[4];

//CrySetting
int page = 0;
int limit = 2;
int Current_Screen = 0;

//CryTemp
String coin1_sym ="";
String coin2_sym ="";
String current_sym = "";

void setup() {
  Serial.begin(9600);
  u8g2.begin();

    pinMode(next, INPUT);
    pinMode(prev, INPUT);
    pinMode(chart1, INPUT);
    pinMode(chart2, INPUT);

    pinMode(ledPin, OUTPUT);

  Serial.print("Station is running");
  //connect to wifi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(WiFi.localIP());


}

void loop() {
    if (millis() - lastRefesh > screenRefeshTime*1000) {
        Serial.print("render");
        if (Current_Screen == 0) {
            
            oled_coinsList_render(http_request(String(apiCoins)+(page*limit)+"/"+limit));
        }
         if (Current_Screen == 1) {
            oled_chart_render(http_request( String(apiChart)+current_sym ));
        }
      lastRefesh = millis();
  }

  for (int btnIndex = 0; btnIndex < sizeof(btnArray); btnIndex++) {
      btnState[btnIndex] = digitalRead(btnArray[btnIndex]);
       
      if (millis() - btnLastPressMillis[btnIndex] > timeShortPress && btnState[btnIndex]==1) { //vuot delay
     // Serial.println(btnState[btnIndex]);
        //    Serial.println("Pressed");
          //  Serial.println(apiCoins);
            switch (btnArray[btnIndex]) {
                case next:    //D3 Next
                    if (page<19) page++;
                    oled_coinsList_render(http_request(String(apiCoins)+(page*limit)+"/"+limit));
                     Serial.println("Next");
                    break;
                case prev:     //D4 Prev
                    if (page>0) page--;
                    oled_coinsList_render(http_request(String(apiCoins)+(page*limit)+"/"+limit));
                    Serial.println("Prev");
                    break;
                case chart1:     //L1 Chart
                    if (Current_Screen==0) Current_Screen=1;
                    else Current_Screen=0;
               
                    if (Current_Screen == 1) {  
                       oled_chart_render(http_request( String(apiChart)+coin1_sym ));
                       current_sym = coin1_sym;
                    } else {
                       oled_coinsList_render(http_request(String(apiCoins)+(page*limit)+"/"+limit));

                    }
                    break;
                case chart2:      //L2 Chart
                    if (Current_Screen==0) Current_Screen=1;
                    else Current_Screen=0;
               
                    if (Current_Screen == 1) {  
                       oled_chart_render(http_request( String(apiChart)+coin2_sym ));
                       current_sym = coin2_sym;
                    } else {
                       oled_coinsList_render(http_request(String(apiCoins)+(page*limit)+"/"+limit));
                    }
                    break;
            }
            btnLastPressMillis[btnIndex] = millis();  //cai lai tg lan cuoi nhan nut
            btnState[btnIndex]==0;
      }
  }

}



String http_request(String api) {
   digitalWrite(ledPin, HIGH);
    HTTPClient http;
    String payload = "";
    http.begin(api);
    int httpCode = http.GET();
    while (httpCode == -1) {
        Serial.println("reconnect");
        http.begin(api);
        httpCode = http.GET();
    }
    payload = http.getString();
    http.end();
    Serial.println(payload);
    return payload;
}

void append(char* s, char c) {
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}


void oled_chart_render(String jsondata) {
    jsonBuffer.clear();
    u8g2.clear();
    //128 x 64
    // Serial.print(jsondata);
    char* tmp = new char[jsondata.length()+1];
    jsondata.toCharArray(tmp, jsondata.length()+1);
    JsonArray& root = jsonBuffer.parseArray(tmp);
    if (!root.success()) {
        Serial.write("JSON Failed");
    } else {
        root[0]["ic"].asArray().copyTo(ic1);
        root[0]["c7"].asArray().copyTo(chart7days);

      digitalWrite(ledPin, LOW);
      char Ox = 62;
      char Oy = 3;
         u8g2.firstPage();
        do {
          //  u8g2.drawBitmap(0, 0, 2, 16, ic1);
            u8g2.setFont(u8g2_font_7x14_tf   );
            u8g2.drawStr(50,15,root[0]["sb"]);


            u8g2.drawLine(3, Ox+1, 123, Ox+1);
            u8g2.drawLine(3, 12, 3, Ox);
            u8g2.drawLine(2, 12, 2, Ox);
            int c=0;
            for ( c= 0;c<=7; c++) {
                u8g2.drawLine(3+(c*15), Ox-chart7days[c], 3+( (c+1)*15), Ox-chart7days[c+1]);
                u8g2.drawLine(3+(c*15), Ox, 3+ (c*15), Ox-1);
                u8g2.drawCircle(3+(c*15), Ox-chart7days[c], 2,U8G2_DRAW_ALL);
            }
            c=8;
            u8g2.drawLine(3+(c*15), Ox, 3+ (c*15), Ox-1);
            u8g2.drawCircle(3+(c*15), Ox-chart7days[c], 2,U8G2_DRAW_ALL);

        } while ( u8g2.nextPage() );
    }
}

//function
void oled_coinsList_render(String jsondata) {
    jsonBuffer.clear();
    u8g2.clear();
    //128 x 64
    // Serial.print(jsondata);
    char* tmp = new char[jsondata.length()+1];
    jsondata.toCharArray(tmp, jsondata.length()+1);
    JsonArray& root = jsonBuffer.parseArray(tmp);
    if (!root.success()) {
        Serial.write("JSON Failed");
    } else {


        root[0]["ic"].asArray().copyTo(ic1);
        root[1]["ic"].asArray().copyTo(ic2);
        
        float c24_1 = root[0]["c24"];
        float c24_2 = root[1]["c24"];

        char *c24_c1 = new char[7];
        char *c24_c2 = new char[7];
        String(c24_1).toCharArray(c24_c1, 7);
        String(c24_2).toCharArray(c24_c2, 7);
        append(c24_c1, '%');
        append(c24_c2, '%');

        coin1_sym = root[0]["sb"].asString();
        coin2_sym = root[1]["sb"].asString();

       digitalWrite(ledPin, LOW);
        //oled-render
        u8g2.firstPage();
        do {
            u8g2.drawBitmap(0, 0, 2, 16, ic1);
            u8g2.setFont(u8g2_font_10x20_mf);
            u8g2.drawStr(19,16,root[0]["sb"]);

            u8g2.drawBitmap(0, 20, 1, 8,  dollar);
            u8g2.setFont(u8g2_font_lucasfont_alternate_tr );
            u8g2.drawStr(8,28,root[0]["pr"]);

            u8g2.setFont(u8g2_font_nine_by_five_nbp_tf );
            u8g2.drawStr(97,28,c24_c1);

            u8g2.drawBitmap(111, 0, 2, 16, c24_1 > 0? rate_up:rate_down);
            
            u8g2.drawLine(3, 32, 124, 32);
            //------------------
            int offset = 36;
            u8g2.drawBitmap(0, 0+offset, 2, 16,  ic2);
            u8g2.setFont(u8g2_font_10x20_mf);
            u8g2.drawStr(19,16+offset,root[1]["sb"]);

            u8g2.drawBitmap(0, 20+offset, 1, 8,  dollar);
            u8g2.setFont(u8g2_font_lucasfont_alternate_tr );
            u8g2.drawStr(8,28+offset,root[1]["pr"]);

            u8g2.setFont(u8g2_font_nine_by_five_nbp_tf );
            u8g2.drawStr(97,28+offset,c24_c2);

            u8g2.drawBitmap(111, 0+offset, 2, 16, c24_1 > 0? rate_up:rate_down);

            //---- Scrollbar
            u8g2.drawLine(127, page*scrollbar,127, page*scrollbar+scrollbar);
        } while ( u8g2.nextPage() );
        //giai phong bo nho
        delete[] c24_c1;
        delete[] c24_c2;
    }

    
}