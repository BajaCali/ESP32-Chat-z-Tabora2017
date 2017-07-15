/*
 * ESPchat
 *
 * Created: 9.7.2017
 * Author: BajaCali
 */ 

#define SERIAL_BAUDRATE 115200

#define I2C_SDA 26
#define I2C_SCL 27

#define LCD_I2C_ADDR 0x27
#define LCD_WIDTH 20
#define LCD_HEIGHT 4

#define KEYBOARD_DATA 33
#define KEYBOARD_CLK  34


#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <PS2Keyboard.h>
#include <WiFi.h>
#include <SPI.h>
#include <map>

#include "credentials.h"

#include <esp_wifi.h>
#include <esp_event_loop.h>

WiFiClient client;
const IPAddress SERVER = { 192,168,42,113 };
const int PORT = 8888;
const String NICKNAME = "BC";
const int TIPING_DELAY = 500;
IPAddress MyIP;

std::map<IPAddress, String> nicks;

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_WIDTH, LCD_HEIGHT);
PS2Keyboard keyboard;

void connectTo();
void LaunchClient(IPAddress server, int PORT, String nickname);
void keyboard_setup();
void lcd_setup();
void WiFi_setup();
void online_chat();
void startup_chat();
void PrettyPrint(String str);
void Send(char type, String msg);

int working_cnt = 0;

system_event_cb_t oldhandler;
esp_err_t hndl(void* x, system_event_t* e) 
{
    printf("Event: %d\n",e->event_id);
    fflush(stdout);
    (*oldhandler)(x, e);
}


void setup() {
    Serial.begin(SERIAL_BAUDRATE);

    keyboard_setup();
    lcd_setup();
    WiFi_setup();

    online_chat();

}

void loop() {
    if (keyboard.available()) {
        if(13 == keyboard.read())
            online_chat();
    }
}

void PrettyPrint(String str){
    Serial.print("PP: ");
    for(int i = 0; i < str.length(); i++)
        printf("\n%d: %d (%c) ",i,  str[i], str[i]);
    fflush(stdout);
    Serial.println();
}

void WiFi_setup(){
    WiFi.mode(WIFI_STA);
    connectTo();
}

void lcd_setup(){
    Wire.begin(I2C_SDA, I2C_SCL);
    lcd.begin(LCD_WIDTH, LCD_HEIGHT);
    lcd.backlight();
}

void keyboard_setup(){
    keyboard.begin(KEYBOARD_DATA, KEYBOARD_CLK);

    lcd.backlight();
    lcd.print("ahoj");
}

void connectTo(){
	WiFi.begin(ssid, password);	
    oldhandler = esp_event_loop_set_cb( hndl, nullptr );
    // delay(1000);
    Serial.print("Connecting");
    int stat = WiFi.status();
    lcd.print(" Status: ");
    lcd.print(stat);
    printf("\nStatus pred whilem: %d",stat);
	while (WiFi.status() != WL_CONNECTED){
        printf("\nStatus: %d",stat);
        lcd.print(".");
		if (WiFi.status() != WL_DISCONNECTED){
			WiFi.begin(ssid, password);
			Serial.println("Reconnecting");
		}
		//Serial.print(".");
		delay(500);
        stat = WiFi.status();
    }
    printf("\nStatus po whilu: %d",stat);
	Serial.print("\nConnected!\nIP address: ");
    MyIP = WiFi.localIP();
	Serial.print(MyIP);
}

void LaunchClient(IPAddress server, int PORT, String nickname){
    if (client.connect(server, PORT)) {
        Serial.println("\nConnected.");
        client.print("w");
        for(int i = 0; i < 4; i++){
            client.write(uint8_t(0));
        }
        client.print(nickname + '\0');
    }
    delay(5);
}

String read_user(char frst){
    String in = "";
    in += frst;
    for (int i = 0; i < 3; i++)
        in += char(client.read());
    char c = char(client.read());
    printf("\nuser reading. -> %d (%c) frst: %d (%c)", c, c, frst, frst);
    fflush(stdout);
    while(c != '\n'){
        in += c;
        c = char(client.read());
    }
    Serial.print("\nNew usr: ");
    PrettyPrint(in);
    return in;
}

void set_nicks(){
    Send('a', "");
    String in = "";
    Serial.print("\nw8ing for a from server.");
    while(1){
        if(client.available() > 0)
            if(client.read() == 'a')
                break;
    }
    Serial.print("\na arrived.");
    for (int i = 0; i < 4; i++)
        client.read();
    while(1){
        char frst = client.read(); 
        if(frst == '\0')
            break;
        in = read_user(frst);
        IPAddress IP_in(reinterpret_cast<uint8_t*>(&in[0]));
        nicks[IP_in] = in.substring(4, in.length());
    }
}

void startup_chat(){
    LaunchClient( SERVER, PORT, NICKNAME);
    Serial.println("\n\n\nESP32chat");
    lcd.clear();
    lcd.print(" *Welcome to chat!*");
    lcd.setCursor(0,3);
    lcd.print(" Press Enter to go!");
    String loading = "\\/|*";
    char c = ' ';
    int i = 0;
    while(c != 13){
        if (keyboard.available()) {
            c = keyboard.read();
        }
        lcd.setCursor(10, 1);
        lcd.print(loading[i]);
        i++;
        if(i > 3)
            i = 0;
        delay(500);
    }
    nicks[WiFi.localIP()] = NICKNAME;
    set_nicks();
    lcd.clear();
    lcd.print("Data loaded.");
    delay(2000);
    lcd.clear();
}

void working(){
    String working = "-\\|/";
    if(millis() % 500 == 0){
        lcd.setCursor(19, 0);
        lcd.print(working[working_cnt]);
        working_cnt++;
        if(working_cnt > 3)
            working_cnt = 0;
    }
}

void Send(char type, String msg){
    client.write(type);
    for(int i = 0; i < 4; i++)
        client.write(uint8_t(0));
    if(msg != "")
        client.write(msg.c_str(), msg.length());
    client.write((uint8_t)'\0');
}

void clearLine(int line){
    lcd.setCursor(0, line);
    for(int i = 0; i < 20; i++)
        lcd.print(' ');
    lcd.setCursor(0, line);
}

String backspace(String msg){
    msg = msg.substring(0, msg.length()-2);
    clearLine(3);
    lcd.print(msg);
    return msg;
}

String input(){
    String str = "";
    for (int i = 0; i < 5; i++)
        str += char(client.read());
    str += client.readStringUntil('\0');
    return str;
}

void online_chat(){
    int end = 0;
    startup_chat();
    String msg = "";
    while(end != 1){
        working();
        if(keyboard.available()){
            char c = keyboard.read();
            lcd.setCursor(msg.length(), 3);
            lcd.write(c);
            printf("\nvstup: %c (%d)",c,c);
            msg += c;
            if(c == 13){
                msg = msg.substring(0, msg.length()-1);
                Send('m', msg);
                Serial.print("\n-> Sending: " + msg);
                msg = "";
                clearLine(3);
            }
            else if(c == 27){
                lcd.clear();
                lcd.setCursor(7,1);
                lcd.print("*KONEC*");
                lcd.setCursor(0,3);
                lcd.print(" *Press Enter to go");
                client.stop();
                Serial.println("Odpojen");
                return;
            }
            else if(c == 127){
                msg = backspace(msg);
                Serial.println("del");
            }
            Serial.println(msg);
        }
        if(msg != "" && millis() % TIPING_DELAY == 0){
            Send('t', "");
            Serial.println("tiping..");
        }
        if(client.available() > 0){
            String raw_input = input();
            Serial.print("server input: ");
            char type = raw_input[0];
            PrettyPrint(raw_input); 
            IPAddress IP_in(reinterpret_cast<uint8_t*>(&raw_input[1]));  // = IPAddress IP_in(raw_input[1], raw_input[2], raw_input[3], raw_input[4]);
            if(type == 'c'){
                clearLine(0);
                lcd.print("c: "+ IP_in.toString());
                nicks[IP_in] = raw_input.substring(5, raw_input.length());
                lcd.setCursor(20 - nicks[IP_in].length(), 1);
                lcd.print(nicks[IP_in]);
            }
            else if(type == 'd'){
                clearLine(0);
                lcd.print("d: " + IP_in.toString());
                lcd.setCursor(20 - nicks[IP_in].length(), 1);
                lcd.print(nicks[IP_in]);
            }
            else if (type == 'm'){
                clearLine(0);
                lcd.print("m: " + IP_in.toString());
                clearLine(1);
                lcd.print(nicks[IP_in] + ":");
                clearLine(2);
                lcd.print(raw_input.substring(5, raw_input.length()));
            }
        }
    }
}