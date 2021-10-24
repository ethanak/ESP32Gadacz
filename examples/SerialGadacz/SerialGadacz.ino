#include <ESP32Gadacz.h>
#include <ctype.h>

char serbuf[256];
int serpos;

void setup()
{
    Serial.begin(115200);
    Gadacz::begin(14, 12, 13);
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
