#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <Bounce2.h>
#include <ESP32Gadacz.h>

// pin LED-y
#define LED_PIN 18

// pin klawisza (0 = BOOT)
#define KEY_PIN 0

// piny I2C

#define PIN_SCL SCL
#define PIN_SDA SDA
/*
 albo np. dla nietypowej konfiguracji:
#define PIN_SCL 23
#define PIN_SDA 19
*/

// współczynniki wyrównania wstępnego RGB
#define MPX_R 0.52
#define MPX_B 1.351

// poziomy odniesienia czerni
#define ZERO_R 22.5
#define ZERO_G 35
#define ZERO_B 35

// poziom odniesienia bieli
#define WHITE_C 208.0

// odkomentuj dla wzmacniacza podłączonego do GPIO25
//#define INTERNAL_DAC

// piny DAC I2S
#define WCLK_PIN 14
#define BCLK_PIN 12
#define DOUT_PIN 13

// parametry mowy
int spk_speed = 4;
int spk_volume = 12;
int spk_pitch = 5; 


Bounce guzik=Bounce();

// Autorange class for TCS34725
// Based on autorange class from Adafruit library example

class tcs34725 {
private:
    struct tcs_agc {
        tcs34725Gain_t ag;
        uint8_t at;
        uint16_t mincnt;
        uint16_t maxcnt;
    };
    static const tcs_agc agc_lst[];
    uint16_t agc_cur;

    void setGainTime(void);
    Adafruit_TCS34725 tcs;

    float fmax3(float f1, float f2, float f3);
    bool fnorm3(float *f1, float *f2, float *f3);
    void rgb2hsv(float r, float g, float b, float *h, float *s, float *v);

public:
    tcs34725(uint8_t led_pin);
    bool begin(void);
    bool getData(void);

    bool isAvailable, isSaturated;
    uint16_t againx, atime, atime_ms;
    uint16_t r, g, b, c;
    uint16_t r_comp, g_comp, b_comp, c_comp;
  
    float rf, gf, bf, cf, hf, sf, vf;
    bool light;
    uint8_t _led_pin;
};
//
// Gain/time combinations to use and the min/max limits for hysteresis
// that avoid saturation. They should be in order from dim to bright.
//
// Also set the first min count and the last max count to 0 to indicate
// the start and end of the list.
//
const tcs34725::tcs_agc tcs34725::agc_lst[] = {
    { TCS34725_GAIN_60X, TCS34725_INTEGRATIONTIME_614MS,     0, 20000 },
    { TCS34725_GAIN_60X, TCS34725_INTEGRATIONTIME_154MS,  4990, 63000 },
    { TCS34725_GAIN_16X, TCS34725_INTEGRATIONTIME_154MS, 16790, 63000 },
    { TCS34725_GAIN_4X,  TCS34725_INTEGRATIONTIME_154MS, 15740, 63000 },
    { TCS34725_GAIN_1X,  TCS34725_INTEGRATIONTIME_154MS, 15740, 0 }
};
tcs34725::tcs34725(uint8_t led_pin) : agc_cur(0), isAvailable(0), isSaturated(0), light(true) {
    _led_pin=led_pin;
}

// initialize the sensor
boolean tcs34725::begin(void) {
    pinMode(_led_pin, OUTPUT);
    digitalWrite(_led_pin, 0);
    tcs = Adafruit_TCS34725(agc_lst[agc_cur].at, agc_lst[agc_cur].ag);
    if ((isAvailable = tcs.begin())) setGainTime();
    return(isAvailable);
}

// Set the gain and integration time
void tcs34725::setGainTime(void) {
    tcs.setGain(agc_lst[agc_cur].ag);
    tcs.setIntegrationTime(agc_lst[agc_cur].at);
    atime = int(agc_lst[agc_cur].at);
    atime_ms = ((256 - atime) * 2.4);
    switch(agc_lst[agc_cur].ag) {
        case TCS34725_GAIN_1X:
        againx = 1;
        break;

        case TCS34725_GAIN_4X:
        againx = 4;
        break;

        case TCS34725_GAIN_16X:
        againx = 16;
        break;

        case TCS34725_GAIN_60X:
        againx = 60;
        break;
    }
}

// Retrieve data from the sensor and do the calculations
bool tcs34725::getData(void) {
  // read the sensor and autorange if necessary
    if (light) {
        digitalWrite(_led_pin, 1);
        delay(1000);
    }
    tcs.getRawData(&r, &g, &b, &c);
    while(1) {
        if (agc_lst[agc_cur].maxcnt && c > agc_lst[agc_cur].maxcnt)
            agc_cur++;
        else if (agc_lst[agc_cur].mincnt && c < agc_lst[agc_cur].mincnt)
            agc_cur--;
        else break;
        setGainTime();
        delay((256 - atime) * 2.4 * 2); // shock absorber
        tcs.getRawData(&r, &g, &b, &c);
        break;
    }
    if (light) {
        digitalWrite(_led_pin, 0);
    }
    int ir = (r + g + b > c) ? (r + g + b - c) / 2 : 0;
    r_comp = r - ir;
    g_comp = g - ir;
    b_comp = b - ir;
    c_comp = c - ir;
    
    int saturation = ((256 - atime) > 63) ? 65535 : 1024 * (256 - atime);
    saturation = (atime_ms < 150) ? (saturation - saturation / 4) : saturation;
    if (c >= saturation) return false;
    float mp_r=(float)g_comp/(float)r_comp;
    float mp_b=(float)g_comp/(float)b_comp;

    printf("MPX_R=%.3f MPX_B=%.3f\n",mp_r, mp_b);
    
    rf = (r_comp * MPX_R)/(float)againx;
    gf = g_comp/(float)againx;
    bf = (b_comp * MPX_B)/(float)againx;

    printf("ZERO_R=%.2f ZERO_G=%.2f ZERO_B=%.2f\n", rf, gf, bf);
    rf -= ZERO_R;
    gf -= ZERO_G;
    bf -= ZERO_B;
    
    printf("WHITE_C %f\n", fmax3(rf,bf,gf));
    cf = fmax3(rf,bf,gf)/ WHITE_C;
    fnorm3(&rf, &gf, &bf);
    rgb2hsv(rf, gf, bf, &hf, &sf, &vf);
    //printf("RGB %.2f %.2f %.2f, hsv %.2f %.2f %.2f cf=%.2f\n", rf,gf,bf,hf,sf,vf,cf);
    return true;

}

float tcs34725::fmax3(float f1, float f2, float f3)
{
    if (f2 > f1) f1=f2;
    if (f3 > f1) return f3;
    return f1;
}
    
bool tcs34725::fnorm3(float *f1, float *f2, float *f3)
{
    if (*f1 <= 0 && *f2 <= 0 && *f3 <= 0) return false;
    float fm = fmax3(*f1, *f2, *f3);
    *f1 /= fm;
    *f2 /= fm;
    *f3 /= fm;
    return true;
}

void tcs34725::rgb2hsv(float r, float g, float b, float *h, float *s, float *v)
{
    float fH, fS, fV;
    float fCMax = fmax3(r, g, b);
    float fCMin = fmin(fmin(r, g), b);
    float fDelta = fCMax - fCMin;

    if(fDelta > 0) {
        if(fCMax == r) {
            fH = 60 * (fmod(((g - b) / fDelta), 6));
        } else if(fCMax == g) {
            fH = 60 * (((b - r) / fDelta) + 2);
        } else if(fCMax == b) {
            fH = 60 * (((r - g) / fDelta) + 4);
        }
        if(fCMax > 0) {
            fS = fDelta / fCMax;
        } else {
            fS = 0;
        }
        fV = fCMax;
    } else {
        fH = 0;
        fS = 0;
        fV = fCMax;
    }
    if(fH < 0) {
        fH = 360 + fH;
    }
    *h=fH;
    *s=fS;
    *v=fV;
}


tcs34725 rgb_sensor(LED_PIN);


void setup(void) {
    Serial.begin(115200);
#ifdef INTERNAL_DAC
    Gadacz::begin()
#else
    Gadacz::begin(WCLK_PIN, BCLK_PIN, DOUT_PIN);
#endif
    Gadacz::setSpeed(spk_speed);
    Gadacz::setVolume(spk_volume);
    Gadacz::setPitch(spk_pitch);
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
    if (!rgb_sensor.begin()) {
        Gadacz::saycst("Nie wykryto czujnika");
        for (;;) delay(100);
    }
    guzik.attach(KEY_PIN,INPUT);
    Gadacz::saycst("Gotowy do pracy");
}

const char *cnames[15]={"czerwony","pomarańczowy","żółty","żółto-zielony","zielony","trawa","cyjan","lazur",
      "niebieski","fioletowy","magenta","różowy","czarny","szary","biały"};
const char *satnames[]={"","złamany ","blady ","intensywny "};
const char *brinames[]={"","ciemny ","jasny ","bardzo jasny "};

void whatColor(int *wheel, int *bright, int *sat)
{
    float rlv = rgb_sensor.vf * rgb_sensor.cf;
    if (rlv < 0.03) { // czarny
        *wheel = 12;
        *sat = 0;
        *bright = 0;
        return;
    }
    if (rgb_sensor.sf < 0.15) { // szary
        if (rlv < 0.8) {
            if (rlv < 0.20) *bright = 1;
            else if (rlv < 0.40) *bright = 0;
            else if (rlv < 0.55) *bright=2;
            else *bright=3;
            *wheel = 13;
        }
        else {
            *wheel = 14;
            *bright = -1;
        }
        *sat = 0;
        return;
    }
    *wheel = (((int)rgb_sensor.hf + 15) /30) % 12;

    if (rlv < 0.20) *bright = 1;
    else if (rlv < 0.40) *bright = 0;
    else if (rlv < 0.55) *bright=2;
    else *bright=3;

    if (rgb_sensor.sf < 0.40) {
        *sat = (rlv < 0.3) ? 1:2;
    }
    else if (rgb_sensor.sf < 0.80) {
        *sat = 0;
    }
    else *sat = 3;
}

    

void loop(void) {

    int wheel, sat, bright;
    bool rc;

    guzik.update();
    if (!guzik.fell()) return;
    Gadacz::saycst("Pomiar");
    rc=rgb_sensor.getData();
    Gadacz::waitAudio(5000);
    if (!rc) {
        printf("Saturacja\n");
        Gadacz::saycst("Błąd pomiaru");
        return;
    }
    whatColor(&wheel, &bright, &sat);
    printf("Kolor: %s%s%s\n", brinames[bright], satnames[sat], cnames[wheel]);
    Gadacz::sayfmt("Kolor: %s%s%s", brinames[bright], satnames[sat], cnames[wheel]);
}
