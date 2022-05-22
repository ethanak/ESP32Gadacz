#ifndef ESP32GADACZ_H
#define ESP32GADACZ_H

#include <Arduino.h>

namespace Gadacz {
    void begin(bool use_gain = false);
    void begin(int wclk_pin, int bclk_pin, int dout_pin);

    void setUserDict(const char * const *units, const char * const *dict);
    void sayfmt(const char *format, ...);
    void saycst(const char *text);
    void say(const char *text);
    void say(String &s);
    void waitAudio(uint32_t timeout=10000UL);
    void stop(void);
    void beep(int freq, int duration);

    void setSpecialChar(char znak);
    void setSpeakCallback(void (*fun)(const char *));

    void setSpeed(uint8_t speed);
    void setVolume(uint8_t volume);
    void setPitch(uint8_t pitch);
    void setContrast(uint8_t contrast);
    uint8_t getVolume();
    uint8_t getSpeed();
    uint8_t getPitch();
    uint8_t getContrast();

    bool changeSpeed(int delta, bool nobeep=false);
    bool changePitch(int delta, bool nobeep=false);
    bool changeVolume(int delta, bool nobeep=false);
    bool changeContrast(int delta, bool nobeep=false);
    

    void setSimpleDeciPoint(bool mode);
    bool getSimpleDeciPoint();
    bool changeSimpleDeciPoint(void);
    bool setDeciPoint(const char *dp);
    bool setDeciPoint(String &s);
    void getDeciPoint(char *buf);
    String getDeciPoint();
    void setAltColon(bool mode);
    bool getAltColon();

    bool isSpeaking();
}

#endif

