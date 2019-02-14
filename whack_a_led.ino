#include <CapacitiveSensor.h>
#include <Entropy.h>
#include <SPI.h>
#include <Ethernet.h>
#include <TimerOne.h>
#include <LiquidCrystal.h>

#define NUM_OF_SAMPLES  20   // higher number - more delay but more consistent readings
#define THRESHOLD  600       // from capasitive sensors
#define GAME_TIME  30        // seconds
#define GAME_SPEED 600
LiquidCrystal lcd(40, 41, 42, 43, 44, 45);

int pin_Out_A = 31;
int pin_Out_B = 30;
int dataPin = 34;
int latchPin = 33;
int clockPin = 32;
int max1 = -1, max2 = -1;
int temp1, temp2;
int counter1, counter2;
int score1 = 0, score2 = 0;
int start = 0, temp_start;
int set_speed, game_index = 0;
int minutes, seconds;
int winLED = 0;

String readString;
String player1 = "Player1", player2 = "Player2"; // max. 7 ch
String game = "0";

long Mux1_State[4] = {0};
long Mux2_State[4] = {0};
long total1, total2;
long random1 = -1, random2 = -1;

byte leds = 0;

volatile int time_;
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xE9, 0xFE, 0xE1 };

CapacitiveSensor   CS_1 = CapacitiveSensor(27, 28);
CapacitiveSensor   CS_2 = CapacitiveSensor(27, 29);

EthernetServer server(80);

void setup()                    
{  
   lcd.begin(16, 2);
   Serial.begin(9600);
   Entropy.Initialize();
   
   CS_1.set_CS_AutocaL_Millis(0xFFFFFFFF);  // Calibrate the sensors 
   CS_2.set_CS_AutocaL_Millis(0xFFFFFFFF); 

   DDRC |= B11111000;
   
   DDRA |= B00001111; // 0-1-2-3 output pins: buzzer, red-yellow-green LEDs

   Ethernet.begin(mac);
   
   if(Ethernet.hardwareStatus() == EthernetNoHardware){
     Serial.println("Ethernet shield was not found");

     while(true){
        delay(1); // do nothing when there is no running hardware
     }
   }
   if(Ethernet.linkStatus() == LinkOFF){
      Serial.println("Ethernet cable is not connected");
   }
  
  Timer1.initialize(1000000);   // period 50 ms         
  Timer1.attachInterrupt(Timer_int_routine);  // attaches callback() as a timer overflow interrupt

  server.begin();
  Serial.println(server.available());
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
  
  set_speed = GAME_SPEED;
  //game = "start";
}

void loop()                    
{  
  //listenForEthernetClients(); 
  if (start == 0){              // when no game yet started
    leds = 255;                 // all leds are on
    updateShiftRegister();      
    
    if (game == "start"){       // ask if any game is chosen to be played
      countDown();              // play count down "music"
      time_ = 0;                // reset game time when game is chosen
      game_index = 1;           // indication that game is on
    }    
  }
    
  if(game_index == 1){          // when game is on...
    if(time_ < GAME_TIME){      // if actual game time didn't yet reach the set time
      randomLED();              // randomize 2 indexes for eacg player
      start++;                  // counter increases when game is on
      
      if(time_ >= GAME_TIME / 2 && time_ < GAME_TIME - GAME_TIME / 4){     // when game reaches half-way
        set_speed = set_speed - 10;   // leds turn on faster
      }
      if(time_ >= GAME_TIME - GAME_TIME / 4){     
        set_speed = set_speed -5;   
      }
      
      updateShiftRegister();    // update LEDs with the current randomized indexes
      delay(set_speed - 100);   // set delay between turning leds on
      touchCheck();             // set touch capasitors values and control which are touched
    }
  }
}
void Timer_int_routine()  // Interrupt routine with Arduino IDE syntax
{
  if(start != 0){         // if game is on, 
    time_++;              // increase time by 1 second
  }
  LCDdisplay();           // show time on LCD
  if(time_ >=GAME_TIME){  // when game is over, blink winner's leds or both players' leds' if draw
    winLED++;
    if(score1 > score2){
      if(winLED % 2 == 0){
        leds = B00001111;
      }
      else
        leds = 0;
    }
    if(score1 < score2){
      if(winLED % 2 == 0){
        leds = B11110000;
      }
      else
        leds = 0;
    }
    if(score1 == score2){
      if(winLED % 2 == 0){
        leds = 255;
      }
      else
        leds = 0;
    }
    updateShiftRegister();
  }
}

void randomLED()   
{ 
  if(start == 0){                     // when game is just starting
    random1 = Entropy.random(0, 4);   // randomize starter indexes for players
    random2 = Entropy.random(4, 8);
  }
  if(start > 0){                      // when game is already on
    leds = 0;
    temp1 = random1;                  // randomize a new index, which is not
    random1 = randomExcept_1(temp1);  // the previous one
    
    temp2 = random2;                  // same for the other player
    random2 = randomExcept_2(temp2);
  }
  bitSet(leds, random1);              // set '1' to these indexes 
  bitSet(leds, random2);              // to be later sent to the shift register
}

int randomExcept_1(int temp1)         // randomizes a new index, that is not the previous one
{
    const int random1_0 [] = {1, 2, 3};   // for each index, set an array with the rest -
    const int random1_1 [] = {0, 2, 3};   // - of the indexes that doesn't include itself
    const int random1_2 [] = {0, 1, 3};
    const int random1_3 [] = {0, 1, 2};

    if(temp1 == -1)
      random1 = Entropy.random(0, 4);
    if(temp1 == 0)                                    // randomize a new index, which
        random1 = random1_0[Entropy.random(0, 3)];    // is not the previous one
        else if(temp1 == 1)
          random1 = random1_1[Entropy.random(0, 3)];
          else if(temp1 == 2)
            random1 = random1_2[Entropy.random(0, 3)];
            else
              random1 = random1_3[Entropy.random(0, 3)];
              
  return random1;                                     // return the new index
}

int randomExcept_2(int temp2)     // same for the second player
{ 
    const int random2_4 [] = {5, 6, 7};
    const int random2_5 [] = {4, 6, 7};
    const int random2_6 [] = {4, 5, 7};
    const int random2_7 [] = {4, 5, 6};

    if(temp2 == -1)
      random2 = Entropy.random(4, 8);
    if(temp2 == 4)
        random2 = random2_4[Entropy.random(0, 3)];
        else if(temp2 == 5)
          random2 = random2_5[Entropy.random(0, 3)];
          else if(temp2 == 6)
            random2 = random2_6[Entropy.random(0, 3)];
            else
              random2 = random2_7[Entropy.random(0, 3)];
              
  return random2;
}

void touchCheck()     // set touch capasitors values and check which sensor is touched
{
  counter1 = 0;       // number of touched sensors
  counter2 = 0;
    for(int i=0; i < 4; i++){   // for each sensor on both sides
       if(bitRead(i, 0) == 1)   // set multiplexer's channel selects A&B as 00, 10, 01, 11
          PORTC |= 1 << PC6;    
       if(bitRead(i, 0) == 0)
          PORTC &= ~(1 << PC6);
       if(bitRead(i, 1) == 1)
          PORTC |= 1 << PC7;
       if(bitRead(i, 1) == 0)
          PORTC &= ~(1 << PC7);
       
       total1 = CS_1.capacitiveSensor(NUM_OF_SAMPLES);    // read each touch capasitor's 
       total2 = CS_2.capacitiveSensor(NUM_OF_SAMPLES);    // values for each player
       
       Mux1_State[i] = total1;                    // assign those values to an array
       Mux2_State[i] = total2;                    // for each player
       
       if(Mux1_State[i] > THRESHOLD){    // check if any of 4 pads is touched
        max1 = i;                        // assign touched LED's index as maximum
        counter1++;                      // set counter to check later how many pads
       }                                 // are touched
       if(Mux2_State[i] > THRESHOLD){    // same for the other player
         max2 = i + 4;
         counter2++;
       }
    }
     
    if(max1 == random1){       // if touched pad has the lit LED -
        score1++;              // randomize a new index for the 1st player
    }            
    if(max2 == random2){       // same for the other player
        score2++;
    }
    max1 = -1;                 // set maximum values back to initial values
    max2 = -1;                 // for the next check
}

void updateShiftRegister()      // update LEDs with the current randomized indexes
{
   PORTC &= ~(1<<PC4);          // latch pin down
   shiftOut(dataPin, clockPin, MSBFIRST, leds);   // led indexes are sent to shift register
   PORTC |= 1<<PC4;             // latch pin up
}

void countDown()
{
    PORTA &= B11110000;   // all OFF when game is about to start
    
    delay(10);
    PORTA |= B00000011;   // buzzer ON, red_LED ON
    delay(800);
    PORTA &= ~(1<<PA0);   // buzzer OFF, red_LED ON
    delay(400);
    PORTA |= B00000101;   // buzzer ON, yellow_LED ON
    PORTA &= ~(1<<PA1);   // red_LED OFF
    delay(800);
    PORTA &= ~(1<<PA0); // buzzer OFF, yellow_LED ON
    delay(400);
    PORTA |= B00001001;   // buzzer ON, green_LED ON
    PORTA &= ~(1<<PA2);   // yellow_LED OFF
    delay(1000);
    PORTA |= B00001000; // buzzer OFF, green_LED ON 
    delay(500);
    PORTA &= B11110000;
    delay(300);
}

void listenForEthernetClients() 
{
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //read char by char HTTP request
        if (readString.length() < 100) 
          {
            //store characters to string
            readString += c;      
          }
        
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        
        if (c == '\n') 
        {
          Serial.println(readString); //print to serial monitor for debuging 
          // send a standard http response header
          
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          
          // print the current readings, in HTML format:

          client.println("<meta http-equiv='refresh' content='4'/>");   // web page update interval 2 second
          client.println("<HTML>");
          client.println("<HEAD>");
          client.println("<TITLE>SPEED TEST</TITLE>");
          client.println("</HEAD>");
          client.println("<BODY>");

          client.println("<h1>LEDIPELI</h1>");
          client.println("<br>");
          client.println("<br>");
          //client.println("<FORM>");
          client.print("<table>");
          client.print("<tr>");
          client.print("<th><h1><FONT color=""green"">"); 
          client.print("Green Player</FONT></h1></th>");
          client.print("<th><h1><FONT color=""green"">:</FONT></h1></th>");
          client.print("</h1></th>");
          client.print("<th><h1>");
          client.println(player1);
          client.print("</h1></th></tr>");
          client.println("<br>");
          
          client.print("<tr>");
          client.print("<th><h1><FONT color=""red"">"); 
          client.print("Red Player</FONT></h1></th>");
          client.print("<th><h1><FONT color=""red"">:</FONT></h1></th>");
          client.print("</h1></th>");
          client.print("<th><h1>");
          client.println(player2);
          client.print("</h1></th></tr>");  
          client.print("</table>");
          client.println("<br>");

          client.println("<style>");
          client.println("th {text-align:left}");
          client.println("body {background-color:lightgray }");
          client.println("</style>");
          
          if(time_ >= GAME_TIME){ 
            client.print("<h1>");
            if(score1 > score2){
              client.print("<FONT color=""green"">");
              client.print("Winner is "); 
              client.print(player1);
              client.print("!");
              client.println("</FONT>");
            }
            if(score2 > score1){
              client.print("<FONT color=""red"">"); 
              client.print("Winner is ");
              client.print(player2);
              client.print("!");
              client.print("</FONT>");
            }
            if(score1 == score2){
              client.print("<FONT color=""blue"">"); 
              client.println("It's a draw!");
              client.print("</FONT>");
            }
            client.println("<br>");
            client.print("<FONT color=""blue"">"); 
            client.print("New game?");
            client.print("</FONT>");
            client.print("</h1><br>");
            client.print("<input type=submit value=YES style=width:90px;height:45px;color:black;font-weight:bold; onClick=location.href='/?yes;'><br>");
          }
          client.println("<br>");
          if(start == 0){
            client.print("<input type=submit value=START style=width:90px;height:45px;color:black;font-weight:bold; onClick=location.href='/?start;'>");
          }
          if(start != 0 && time_ < GAME_TIME){
            client.print("<h1>");
            client.println("GAME ON!");
            client.print("</h1>"); 
          }
          
          //client.println("</form>");
          client.println("<br/>");
          client.println("</BODY>");
          client.println("</HTML>");
            
          delay(1);
          //stopping client
          client.stop();

           ///////////////////// control arduino pin
         
          if(readString.indexOf("start") > 0 && game_index != 2){
            game = "start";
            //digitalWrite(25,HIGH);
          }
          if(readString.indexOf("yes") > 0){
            time_ = 0;
            start = 0;
            score1 = 0;
            score2 = 0;
            game = "";
            game_index = 0;
            set_speed = GAME_SPEED;
            lcd.setCursor(8, 0);
            lcd.print("  ");
            lcd.setCursor(8, 1);
            lcd.print("  ");
          }
          Serial.print("readString = ");
          Serial.println(readString); 
          //clearing string for next read
          
          readString="";
        }
      }
    }
  }
}

void LCDdisplay()
{
  lcd.setCursor(0, 0);    // first player's name and score
  lcd.print(player1);
  lcd.setCursor(7, 0);
  lcd.print(":");
  lcd.setCursor(8, 0);
  lcd.print(score1);

  lcd.setCursor(0, 1);    // second player's name and score
  lcd.print(player2);
  lcd.setCursor(7, 1);
  lcd.print(":");
  lcd.setCursor(8, 1);
  lcd.print(score2);
  
  seconds = time_ % 60;   // game time
  minutes = time_ / 60;

  if(minutes < 10){       // print game time
    lcd.setCursor(11, 0);
    lcd.print("0");
    lcd.setCursor(12, 0);
    lcd.print(minutes);
    lcd.print(":");
  }
  if(minutes > 9){
    lcd.setCursor(11, 0);
    lcd.print(minutes);
    lcd.print(":");
  }
  if(seconds < 10){
    lcd.setCursor(14, 0);
    lcd.print("0");
    lcd.setCursor(15, 0);
    lcd.print(seconds);
  }
  if(seconds > 9){
    lcd.setCursor(14, 0);
    lcd.print(seconds);
  }
}
