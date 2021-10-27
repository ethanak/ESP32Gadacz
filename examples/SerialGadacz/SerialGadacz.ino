#include <ESP32Gadacz.h>
#include <ctype.h>

// Odkomentuj linię poniżej jeśli chcesz włączyć dodatkowy słownik
//#include "scifi.h"


// odkomentuj dla wzmacniacza podłączonego do GPIO25
//#define INTERNAL_DAC

// piny DAC I2S
#define WCLK_PIN 14
#define BCLK_PIN 12
#define DOUT_PIN 13



char serbuf[256];
int serpos;

void setup()
{
    Serial.begin(115200);
#ifdef USER_UNITS
    Gadacz::setUserDict(USER_UNITS, USER_LINES);
#endif
#ifdef INTERNAL_DAC
    Gadacz::begin()
#else
    Gadacz::begin(WCLK_PIN, BCLK_PIN, DOUT_PIN);
#endif
    Gadacz::setSpeed(4);
    Gadacz::saycst("Dzień dobry");
    serpos=0;
}

void doSerialCmd()
{
    char *c=serbuf;
    while (*c && isspace(*c)) c++;
    if (!*c) return;
    if (*c != '\\') {
        Gadacz::say(c);
        return;
    }
    c++;
    int znak = *c++;
    int val = strtol(c,&c, 10);
    switch (znak) {
        case 'b':
        Gadacz::beep(val, strtol(c, NULL, 10));
        break;
        
        case 'v':
        Gadacz::setVolume(val);
        Gadacz::sayfmt("Głośność %d", Gadacz::getVolume());
        break;
        
        case 'p':
        Gadacz::setPitch(val);
        Gadacz::sayfmt("Wysokość %d", Gadacz::getPitch());
        break;

        case 's':
        Gadacz::setSpeed(val);
        Gadacz::sayfmt("Prędkość %d", Gadacz::getSpeed());
        break;

        case 'c':
        Gadacz::setContrast(val);
        Gadacz::sayfmt("Kontrast %d", Gadacz::getContrast());
        break;
    }
}

bool serialCmd()
{
    bool rc=false;
    while (Serial.available()) {
        int znak = Serial.read();
        if (znak == '\r' || znak == '\n') {
            if (!serpos) continue;
            serbuf[serpos]=0;
            serpos=0;
            doSerialCmd();
            rc = true;
            
        }
        else if (znak) {
            if (serpos<255) serbuf[serpos++] = znak;
        }
    }
    return rc;
}

void loop()
{
    if (!serialCmd()) delay(10);
}
