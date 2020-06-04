
#include "Servo.h"
#include "HX711.h"

#define DEBUG true

#define COUNT_PREV             5
#define TIME_ONE_SEC           1000L

#define TIME_WATER_DELAY       (TIME_ONE_SEC*60L)*18L
#define TIME_STUFF_DELAY       (TIME_ONE_SEC*60L)*18L

#define TIME_WATER_GLAM        13.0
#define TIME_STUFF_GLAM        2.5

#define TYPE_INIT              0x00


#define TYPE_WATER             0x01
#define TYPE_FEED              0x02
#define TYPE_SEND              0x03

#define TYPE_TRUE              0x01
#define TYPE_FALSE             0x02

////////////////////////////////////////////

#define TYPE_WAIT              0x01
#define TYPE_START             0x02

#define RUN_FEEDING            0x01
#define RUN_EATING             0x02
#define RUN_CHECK_TIME         0x03
#define RUN_CHECK_EATED        0x04


////////////////////////////////////////////

float         loadcellValue_1kg   = -2150.0;
float         loadcellValue_5kg   = -2150.0; // 1KG

int           _sleep              = 0;
byte          _type_seleted       = TYPE_INIT;

////////////////////////////////////////////

struct st_data
{
  public:
    byte          _check_scale        = TYPE_TRUE;

    byte          _type_seleted       = TYPE_INIT;
    byte          _type               = TYPE_INIT;
    unsigned long _time               = 0;
    int           _time_sleep         = 350;
    
    float         _scale              = 0.0f;
    float         _scale_curr         = 0.0f;
    float         _scale_last         = 0.0f;
    byte          _scale_prev_idex    = 0x00;
    float         _scale_prev[COUNT_PREV];
};

st_data _water;
st_data _stuff;


////////////////////////////////////////////

HX711 _scale_5k;
HX711 _scale_1k;

Servo _servo;

int   _water_moter_1A = 9;
int   _water_moter_2A = 8;

int   _servo_mote     = 7;

int   _led_feed       = 6;
int   _led_water      = 5;

int   _bt_switch      = 4;

void setup() {

   Serial.begin(9600);
 
  _scale_1k.begin(13, 12);
  _scale_1k.set_scale(loadcellValue_1kg);
  _scale_1k.tare();

  _scale_5k.begin(11, 10);
  _scale_5k.set_scale(loadcellValue_1kg);
  _scale_5k.tare();

  pinMode(_water_moter_1A, OUTPUT);
  pinMode(_water_moter_2A, OUTPUT);

  pinMode(_bt_switch, INPUT);
  digitalWrite(_bt_switch, HIGH);

  pinMode(_led_water, OUTPUT);
  pinMode(_led_feed, OUTPUT);

  _servo.attach(_servo_mote);
  _servo.write(90);
  delay(100);
  _servo.detach();

 
  _type_seleted            = TYPE_FEED;
  
  _water._type_seleted     = TYPE_WAIT;
  _water._scale_prev_idex  = 0x00;
  _water._time             = 0;

  _stuff._type_seleted     = TYPE_WAIT;
  _stuff._scale_prev_idex  = 0x00;
  _stuff._time             = 0;


  for ( byte i = 0x00 ; i < COUNT_PREV ; i ++ ) {
      _water._scale_prev[i] = -100.0;

      digitalWrite(_led_water, HIGH);
      digitalWrite(_led_feed, HIGH);
      delay(300);
      digitalWrite(_led_water, LOW);
      digitalWrite(_led_feed, LOW);
      delay(30);
   }

   Serial.println("start");
}

char readButtoms()
{
  byte led_w = LOW;
  byte led_f = LOW;

  byte nowButton = digitalRead(_bt_switch);
  if (LOW == nowButton) {
    //Serial.println("Pushed");

    led_w = HIGH;
    led_f = HIGH;

    byte tpy = 0x01;
    for ( int i = 0 ; i < 5 ; i++ ) {
      digitalWrite(_led_water, led_w);
      digitalWrite(_led_feed, led_f);
      delay(100);
      digitalWrite(_led_water, LOW);
      digitalWrite(_led_feed, LOW);
      delay(400);
    }

    nowButton = digitalRead(_bt_switch);
    if (LOW == nowButton) {
      tpy = 0x02;
    }

    if ( tpy == 0x01) {
      led_w = LOW;
      led_f = HIGH;
      _stuff._type_seleted = TYPE_START;
      _stuff._check_scale  = TYPE_TRUE;

    } else {
      led_w = HIGH;
      led_f = LOW;
      _water._type_seleted = TYPE_START;
      _water._check_scale  = TYPE_TRUE;
    }

    for ( int i = 0 ; i < 10 ; i++ ) {
      digitalWrite(_led_water, led_w);
      digitalWrite(_led_feed, led_f);
      delay(30);
      digitalWrite(_led_water, LOW);
      digitalWrite(_led_feed, LOW);
      delay(30);
    }

    return 0x01;
  }
  return 0x00;
}


void feed_water() {

  float scale           = _scale_1k.get_units(5);
  float scale_real      = scale - _water._scale;
  float scale_feed      = TIME_WATER_GLAM;

#if 1
  Serial.print("water \tscale: ");
  Serial.print(scale);
  Serial.print("  \twater scale: ");
  Serial.print(_water._scale);
  Serial.print("  \tscale_real: ");
  Serial.println(scale_real);
#endif

  _water._scale_curr = scale;

  if ( _water._type_seleted == TYPE_WAIT ) {
    _water._type_seleted = TYPE_INIT;
    _water._time         = millis();
    _water._check_scale  = TYPE_TRUE;
    _water._type         = RUN_CHECK_TIME;

    Serial.println("TYPE_WAIT to RUN_CHECK_TIME");
    return;
  }

  if ( _water._type_seleted == TYPE_START ) {
    _water._type_seleted = TYPE_INIT;
    _water._time         = millis();
    _water._type         = RUN_FEEDING;
    _water._time_sleep   = 400;

    if ( _water._check_scale == TYPE_TRUE ) {
      _water._scale = scale;
    }
    _water._check_scale = TYPE_FALSE;
    for ( byte i = 0x00 ; i < COUNT_PREV ; i ++ ) {
      _water._scale_prev[i] = -100.0;
    }
    Serial.println("TYPE_START to RUN_FEEDING");
    return;
  }


  if ( scale < (_water._scale - 5.0) ) {
    for ( int i = 0 ; i < 5 ; i++ ) {
      digitalWrite(_led_water, HIGH);
      delay(30);
      digitalWrite(_led_water, LOW);
      delay(30);
    }
    //Serial.println("scale < (_water._scale - 5.0)");
    return;
  }


  if ( _water._type == RUN_CHECK_TIME ) {
    unsigned long time_curr = millis() - _water._time;
    if ( time_curr > TIME_WATER_DELAY ) {
      _water._type_seleted  = TYPE_START;
      Serial.println("RUN_CHECK_TIME to TYPE_START");
    }
    return;
  }

  if ( _water._type == RUN_CHECK_EATED ) {

    int count = 0;
    for ( byte i = 0x00 ; i < COUNT_PREV ; i ++ ) {
      float data = _water._scale_prev[i];
      if (data <= (scale + 0.3) && data >= (scale - 0.3 ) ) {
        count++;
      }
    }

    _water._scale_prev_idex = (_water._scale_prev_idex + 1) % COUNT_PREV;
    _water._scale_prev[_water._scale_prev_idex] = scale;

    if ( count >= COUNT_PREV) {
      _water._time          = millis();
      _water._type          = RUN_CHECK_TIME;
      _water._check_scale   = TYPE_FALSE;
      _water._time_sleep    = 400;
      Serial.println("RUN_CHECK_EATED to RUN_CHECK_TIME");
    } else {
      digitalWrite(_led_water, HIGH);
      delay(100);
      digitalWrite(_led_water, LOW);
    }
    delay(500);
    return;
  }

  if ( _water._type == RUN_EATING ) {

    unsigned long time_curr = millis() - _water._time;
    digitalWrite(_led_water, LOW);

    if ( time_curr > TIME_WATER_DELAY ) {
      if ( scale_real <= (scale_feed - 0.8) ) {
        _water._type = RUN_CHECK_EATED;
        _sleep = 500;
        Serial.println("RUN_EATING to RUN_CHECK_EATED 1");
      } else {
        digitalWrite(_led_water, HIGH);
        delay(500);
        digitalWrite(_led_water, LOW);
      }
    } else {
      if ( scale_real <= (scale_feed - 0.8) ) {
        _water._type = RUN_CHECK_EATED;
        _sleep = 500;
        Serial.println("RUN_EATING to RUN_CHECK_EATED 2");
      }
    }
    delay(500);
    return;
  }

  if ( _water._type != RUN_FEEDING ) {
    return;
  }

  if ( scale_real >= scale_feed ) {
    for ( byte i = 0x00 ; i < COUNT_PREV ; i ++ ) {
      _water._scale_prev[i] = -100.0;
    }
    _water._type = RUN_EATING;
    Serial.println("RUN_FEEDINGING to RUN_EATING");
    return;
  }

  int time_sleep = 650;

  Serial.print("\t time_sleep: ");
  Serial.println(time_sleep);

  digitalWrite(_water_moter_1A, HIGH);
  digitalWrite(_water_moter_2A, LOW);
  delay(time_sleep);
  digitalWrite(_water_moter_1A, LOW);
  digitalWrite(_water_moter_2A, LOW);

  delay(500);

  _water._scale_last  = scale_real;
  _water._time        = millis();
}

void feed_stuff()
{
  float scale           = _scale_5k.get_units(5);
  float scale_real      = scale - _stuff._scale;
  float scale_feed      = TIME_STUFF_GLAM;

#if 1
  Serial.print("stuff \tscale: ");
  Serial.print(scale);
  Serial.print("  \tstuff scale: ");
  Serial.print(_stuff._scale);
  Serial.print("  \tscale_real: ");
  Serial.println(scale_real);
#endif

  _stuff._scale_curr = scale;

  if ( _stuff._type_seleted == TYPE_WAIT ) {
    _stuff._type_seleted = TYPE_INIT;
    _stuff._time         = millis();
    _stuff._check_scale  = TYPE_TRUE;
    _stuff._time_sleep   = 0;
    _stuff._type         = RUN_CHECK_TIME;

    Serial.println("TYPE_WAIT to RUN_CHECK_TIME");
  }

  if ( _stuff._type_seleted == TYPE_START ) {
    _stuff._type_seleted = TYPE_INIT;
    _stuff._time         = millis();
    _stuff._type         = RUN_FEEDING;
    _stuff._time_sleep   = 0;

    if ( _stuff._check_scale == TYPE_TRUE ) {
      _stuff._scale = scale;
    }
    _stuff._check_scale = TYPE_FALSE;
    for ( byte i = 0x00 ; i < COUNT_PREV ; i ++ ) {
      _stuff._scale_prev[i] = -100.0;
    }
    Serial.println("TYPE_START to RUN_FEEDING");
    return;
  }

  if ( scale < (_stuff._scale - 10.0) ) {
    for ( int i = 0 ; i < 5 ; i++ ) {
      digitalWrite(_led_feed, HIGH);
      delay(30);
      digitalWrite(_led_feed, LOW);
      delay(30);
    }
    //Serial.println("scale < (_stuff._scale - 5.0)");
    return;
  }


  if ( _stuff._type == RUN_CHECK_TIME ) {
    unsigned long time_curr = millis() - _stuff._time;
    if ( time_curr > TIME_STUFF_DELAY ) {
      _stuff._type_seleted  = TYPE_START;
      _stuff._check_scale   = TYPE_TRUE;
      Serial.println("RUN_CHECK_TIME to TYPE_START");
    }
    return;
  }

  if ( _stuff._type == RUN_CHECK_EATED ) {

    int count = 0;
    for ( byte i = 0x00 ; i < COUNT_PREV ; i ++ ) {
      float data = _stuff._scale_prev[i];
      if (data <= (scale + 0.5) && data >= (scale - 0.5 ) ) {
        count++;
      }
    }

    _stuff._scale_prev_idex = (_stuff._scale_prev_idex + 1) % COUNT_PREV;
    _stuff._scale_prev[_stuff._scale_prev_idex] = scale;

    if ( count >= COUNT_PREV) {
      _stuff._time          = millis();
      _stuff._type          = RUN_CHECK_TIME;
      _stuff._check_scale   = TYPE_FALSE;
      Serial.println("RUN_CHECK_EATED to RUN_CHECK_TIME");
    } else {
      digitalWrite(_led_feed, HIGH);
      delay(100);
      digitalWrite(_led_feed, LOW);
    }
    delay(500);
    return;
  }

  if ( _stuff._type == RUN_EATING ) {

    unsigned long time_curr = millis() - _stuff._time;
    digitalWrite(_led_feed, LOW);

    if ( time_curr > TIME_STUFF_DELAY ) {
      if ( scale_real <= (scale_feed - 1.8) ) {
        _stuff._type = RUN_CHECK_EATED;
        _sleep = 500;
        Serial.println("RUN_EATING to RUN_CHECK_EATED 1");
      } else {
        digitalWrite(_led_feed, HIGH);
        delay(500);
        digitalWrite(_led_feed, LOW);
      }
    } else {
      if ( scale_real <= (scale_feed - 1.8) ) {
        _stuff._type = RUN_CHECK_EATED;
        _sleep = 500;
        Serial.println("RUN_EATING to RUN_CHECK_EATED 2");
      }
    }
    delay(500);
    return;
  }

  if ( _stuff._type != RUN_FEEDING ) {
    return;
  }

  if ( scale_real >= scale_feed ) {
    for ( byte i = 0x00 ; i < COUNT_PREV ; i ++ ) {
      _stuff._scale_prev[i] = -100.0;
    }
    _stuff._type = RUN_EATING;
    Serial.println("RUN_FEEDINGING to RUN_EATING");
    return;
  }

  int angle   = 40;
  int sleep   = 180;

  if (_stuff._scale_last <= (scale_real + 0.5) && _stuff._scale_last >= (scale_real - 0.5 ) ) {
        Serial.println("empty to feed");
        _stuff._time_sleep++;    
        if ( _stuff._time_sleep == 100) {
          angle   = 20;
          sleep   = 220;
          _stuff._time_sleep = 0;
        }
    } else {
      _stuff._time_sleep = 0;
    }
  
  Serial.print("stuff \t angle: ");
  Serial.print(angle);

  Serial.print("\t sleep: ");
  Serial.println(sleep);

  _servo.attach(_servo_mote);

  _servo.write(angle);
  delay(sleep);

  
  _servo.write(90);
  delay(300);
  _servo.detach();
  
  //delay(500);

  _stuff._time        = millis();
  _stuff._scale_last  = scale_real;
}

void loop() {

#if 0
  float scale1           = _scale_5k.get_units(5);
  float scale2           = _scale_1k.get_units(5);

  Serial.print("_scale_1k \tscale: ");
  Serial.print(scale2);
  Serial.print("  \_scale_5k scale: ");
  Serial.println(scale1);
#endif


  digitalWrite(_led_water, LOW);
  digitalWrite(_led_feed, LOW);
  delay(100);

  if ( readButtoms() == 0x01 ) {
    delay(999);
    return;
  }

  if ( _type_seleted == TYPE_FEED ) {
      feed_stuff();
      _type_seleted = TYPE_WATER;
  } else if ( _type_seleted == TYPE_WATER ) {
      feed_water();
      _type_seleted = TYPE_FEED;
  } 

  delay(1000);

}
