#include "ESP32Gadacz.h"
#include <AudioGeneratorMilena.h>
#include <AudioOutputI2S.h>
#include <freertos/ringbuf.h>
#include <stdarg.h>

class AudioGeneratorBeep : public AudioGenerator
{
    public:

    bool begin(int freq, int duration, AudioOutputI2S *out);
    bool stop();
    bool isRunning();
    bool loop();
    
    private:
    int phase;
    int beeplen;
    int beepvol;
    int samplen;
};


static AudioGeneratorMilena *milena = NULL;
static AudioOutputI2S *player = NULL;
static AudioGeneratorBeep *beeper = NULL;

static RingbufHandle_t SpeechRB;
static TaskHandle_t speechHandle;

    
static uint8_t _pitch=12;
static uint8_t _speed=4;
static uint8_t _volume=12;
static bool _spdecipoint=false;
static bool _altcolon=false;
static uint8_t _contrast=4;
static char _decipoint[32]="przecinek";
static volatile bool is_Speaking=false;


static void (*spkCallback)(const char *);

static uint8_t _wclk_pin;
static uint8_t _bclk_pin;
static uint8_t _dout_pin;
static bool _external_dac;
static bool _use_gain;

static char _specChar;

struct SpeakerCmd {
    uint8_t command;
    union {
        const char *text;
        struct {
            int16_t freq;
            uint16_t duration;
        } s;
    } u;
};

enum {
    SCMD_STOP = 0,
    SCMD_SAY,
    SCMD_BEEP,
    SCMD_SAYCST
};

static float vols[] = {0.031, 0.047, 0.062, 0.078, 0.094, 0.109, 0.125,
0.141, 0.156, 0.172, 0.188, 0.219, 0.250, 0.281, 0.312, 0.344, 0.391,
0.438, 0.500, 0.547, 0.625, 0.703, 0.781, 0.891, 1.000};

static const float pitches[25]={0.73, 0.74, 0.76, 0.77, 0.78,
0.80, 0.81, 0.83, 0.84, 0.86, 0.87, 0.89, 0.90, 0.92,
0.93, 0.95, 0.97, 0.98, 1.00, 1.02, 1.04, 1.05, 1.07,
1.09, 1.11};
static const float timeratia[25]={
    1.000, 1.035, 1.072, 1.110, 1.149, 1.189, 1.231, 1.275, 1.320,
    1.367, 1.415, 1.465, 1.517, 1.570, 1.626, 1.683, 1.742, 1.804,
    1.868, 1.934, 2.002, 2.073, 2.146, 2.222, 2.300
};


static const char *spkMessage;
static bool spkMsgAllocated;

static void initAudioOutput(void)
{
    if (_external_dac) {
        player = new AudioOutputI2S(0,AudioOutputI2S::EXTERNAL_I2S,8,AudioOutputI2S::APLL_ENABLE);
        player->SetPinout(_bclk_pin, _wclk_pin, _dout_pin);
        player->SetGain(vols[_volume]);
    }
    else {
        player = new AudioOutputI2S(0,AudioOutputI2S::INTERNAL_DAC,8,AudioOutputI2S::APLL_ENABLE);
        if (_use_gain) {
            player->SetGain(vols[_volume]);
        }
        else {
            player->SetGain(1.0);
        }
    }
}

static void stopAll(bool keep_speaking=false)
{
    if (beeper) {
        beeper->stop();
        delete beeper;
        beeper = NULL;
    }
    if (milena) {
        milena->stop();
        delete milena;
        milena = NULL;
    }
    if (player) {
        delete player;
        player = NULL;
    }
    if (spkMessage) {
        if(spkMsgAllocated) free((void *)spkMessage);
        spkMessage = NULL;
    }
    spkMsgAllocated = false;
    if (!keep_speaking) is_Speaking=false;
}

static void speakerloop()
{
    
    if (beeper && beeper->isRunning()) {
        if (!beeper->loop()) {
            stopAll();
        }
        else delay(1);
    }
    if (milena && milena->isRunning()) {
        if (!milena->loop()) {
            stopAll();
        }
        else delay(1);
    }
}

static int siglen[51]={
200, 195, 189, 184, 179, 174, 169, 164, 159, 155, 150, 146, 142, 138,
134, 130, 126, 123, 119, 116, 113, 109, 106, 103, 100, 97, 95, 92, 89,
87, 84, 82, 80, 77, 75, 73, 71, 69, 67, 65, 63, 61, 60, 58, 56, 55, 53,
52, 50};

bool startBeep(int nfreq, int duration)
{
    stopAll(false);
    initAudioOutput();
    beeper = new AudioGeneratorBeep();
    nfreq=constrain(nfreq,-24,24);
    if (!beeper->begin(siglen[nfreq+24], duration, player)) {
        
        delete beeper;
        delete player;
        beeper = NULL;
        player = NULL;
        return false;
    }
    return is_Speaking = true;
}

static bool startTalking()
{
    const char *msg = spkMessage;
    int p=_pitch;
    if (_specChar && *msg == _specChar) {
        msg++;
        if (p < 20) p += 4;
        else p -=4;
    }
    if (!*msg) return is_Speaking = false;
    
    milena=new AudioGeneratorMilena();
    if (!milena) return is_Speaking = false;
    initAudioOutput();
    if (!player) {
        delete milena;
        return is_Speaking = false;
    }
        
    milena->setPitch(pitches[p]);
    
    milena->setSpeed(timeratia[_speed]);
    milena->setContrast(_contrast);
    milena->setSimpleDeciPoint(_spdecipoint);
    milena->setDeciPoint(_decipoint);
    milena->setAltColon(_altcolon);
    if (!_external_dac) {
        milena->setVolume(1.4);
        milena->setInternalDAC(1);
    }
    if (!milena->begin(msg, player)) {
        delete milena;
        delete player;
        milena = NULL;
        player = NULL;
        return is_Speaking = false;
    }
    return is_Speaking = true;
}

static void speechThread(void *dummy)
{
    struct SpeakerCmd *rc;
    size_t itemsize;
    char *txtadr;
    for (;;) {
        speakerloop();
        rc = (struct SpeakerCmd *)xRingbufferReceive(SpeechRB, &itemsize, is_Speaking ? 0 : portMAX_DELAY);
        if (!rc) continue;
        switch (rc->command) {
            case SCMD_STOP:
            stopAll(false);
            break;

            case SCMD_BEEP:
            stopAll(false);
            startBeep(rc->u.s.freq, rc->u.s.duration);
            break;

            case SCMD_SAY:
            stopAll(true);
            spkMessage = rc->u.text;
            spkMsgAllocated = true;
            if (spkMessage) startTalking();
            break;

            case SCMD_SAYCST:
            stopAll(true);
            spkMessage = rc->u.text;
            spkMsgAllocated = false;
            startTalking();
            break;
        }
        vRingbufferReturnItem(SpeechRB, (void *)rc);
    }
}


static void startGadaczTask()
{
    SpeechRB = xRingbufferCreateNoSplit(32,8);
    xTaskCreatePinnedToCore(speechThread,"speech",10000,NULL,3,&speechHandle,0);
}

void Gadacz::begin(bool use_gain)
{
    _external_dac = false;
    _use_gain = use_gain;
    startGadaczTask();
}

void Gadacz::begin(int wclk_pin, int bclk_pin, int dout_pin)
{
    _wclk_pin = wclk_pin;
    _bclk_pin = bclk_pin;
    _dout_pin = dout_pin;
    _external_dac = true;
    startGadaczTask();
}

void Gadacz::setUserDict(const char * const *units, const char * const *dict)
{
    microlena_setUserDict(units, dict);
}


void Gadacz::say(const char *txt)
{
    struct SpeakerCmd scmd;
    scmd.command = SCMD_SAY;
    scmd.u.text = strdup(txt);
    is_Speaking=true;
    if (spkCallback) spkCallback(scmd.u.text);
    xRingbufferSend(SpeechRB, &scmd, sizeof(scmd), 0);
}

void Gadacz::say(String &s)
{
    struct SpeakerCmd scmd;
    scmd.command = SCMD_SAY;
    scmd.u.text = strdup(s.c_str());
    is_Speaking=true;
    if (spkCallback) spkCallback(scmd.u.text);
    xRingbufferSend(SpeechRB, &scmd, sizeof(scmd), 0);
}

void Gadacz::saycst(const char *txt)
{
    struct SpeakerCmd scmd;
    scmd.command = SCMD_SAYCST;
    scmd.u.text = txt;
    is_Speaking=true;
    if (spkCallback) spkCallback(scmd.u.text);
    xRingbufferSend(SpeechRB, &scmd, sizeof(scmd), 0);
}

static char *mkMessage(const char *fmt, va_list ap)
{
    char *p = NULL;
    int size = 0;
    size = vsnprintf(p, size, fmt, ap);
    if (size <= 0) return NULL;
    size++;
    p = (char *)malloc(size);
    if (!p) return NULL;
    size = vsnprintf(p, size, fmt, ap);
    if (size <= 0) {
        free(p);
        return NULL;
    }
    return p;
}

void Gadacz::sayfmt(const char *format, ...)
{
    struct SpeakerCmd scmd;
    scmd.command = SCMD_SAY;
    va_list ap;
    va_start(ap, format);
    scmd.u.text=mkMessage(format, ap);
    va_end(ap);
    is_Speaking=true;
    if (spkCallback) spkCallback(scmd.u.text);
    xRingbufferSend(SpeechRB, &scmd, sizeof(scmd), 0);
}

void Gadacz::beep(int freq, int duration)
{
    struct SpeakerCmd scmd;
    scmd.command = SCMD_BEEP;
    scmd.u.s.freq = constrain(freq, -24, 24);
    scmd.u.s.duration = constrain(duration, 25, 1000);
    if (spkCallback) spkCallback(NULL);
    xRingbufferSend(SpeechRB, &scmd, sizeof(scmd), 0);
}

void Gadacz::waitAudio(uint32_t timeout)
{
    uint32_t ms=millis();
    while (is_Speaking) {
        if (timeout && millis() - ms > timeout) break;
        delay(10);
    }
}

void Gadacz::stop()
{
    struct SpeakerCmd scmd;
    scmd.command = SCMD_STOP;
    if (spkCallback) spkCallback(NULL);
    xRingbufferSend(SpeechRB, &scmd, sizeof(scmd), 0);
}

void Gadacz::setContrast(uint8_t contrast)
{
    _contrast = (contrast > 100) ? 100: contrast;
}


uint8_t Gadacz::getContrast()
{
    return _contrast;
}

void Gadacz::setVolume(uint8_t volume)
{
    _volume = (volume > 24) ? 24 : volume;
}

uint8_t Gadacz::getVolume()
{
    return _volume;
}

void Gadacz::setSpeed(uint8_t speed)
{
    _speed = (speed > 24) ? 24: speed;
}

uint8_t Gadacz::getSpeed()
{
    return _speed;
}

void Gadacz::setPitch(uint8_t pitch)
{
    _pitch = (pitch > 24) ? 24 : pitch;
}

uint8_t Gadacz::getPitch()
{
    return _pitch;
}

bool Gadacz::changeSpeed(int delta, bool nobeep)
{
    if (!delta) return false;
    if (delta > 0 && _speed == 24) return false;
    if (delta < 0 && _speed == 0) return false;
    _speed = constrain((int)_speed + delta, 0,24);
    if (!nobeep) beep(2 * ((int)_speed - 12), 50);
    return true;
}

bool Gadacz::changePitch(int delta, bool nobeep)
{
    if (!delta) return false;
    if (delta > 0 && _pitch == 24) return false;
    if (delta < 0 && _pitch == 0) return false;
    _pitch = constrain((int)_pitch + delta, 0,24);
    if (!nobeep) beep(2 * ((int)_pitch - 12), 50);
    return true;
}

bool Gadacz::changeVolume(int delta, bool nobeep)
{
    if (!_external_dac && !_use_gain) {
        return false;
    }
    if (!delta) return false;
    if (delta > 0 && _volume == 24) return false;
    if (delta < 0 && _volume == 0) return false;
    _volume = constrain((int)_volume + delta, 0,24);
    if (!nobeep) beep(0, 50);
    return true;
}

bool Gadacz::changeContrast(int delta, bool nobeep)
{
    if (!delta) return false;
    if (delta > 0 && _contrast == 100) return false;
    if (delta < 0 && _contrast == 0) return false;
    _contrast = constrain((int)_contrast + delta, 0,100);
    if (!nobeep) beep((24 * (int)_contrast - 50)/50, 50);
    return true;
}

void Gadacz::setSimpleDeciPoint(bool mode)
{
    _spdecipoint = mode;
}

bool Gadacz::getSimpleDeciPoint()
{
    return _spdecipoint;
}

bool Gadacz::changeSimpleDeciPoint(void)
{
    return _spdecipoint = !_spdecipoint;
}

void Gadacz::setAltColon(bool mode)
{
    _altcolon = mode;
}

bool Gadacz::getAltColon()
{
    return _altcolon;
}

bool Gadacz::setDeciPoint(const char *dp)
{
    if (!*dp || strlen(dp) > 31) return false;
    strcpy(_decipoint, dp);
    return true;
}

bool Gadacz::setDeciPoint(String &s)
{
    if (!s.length() || s.length() > 31) return false;
    strcpy(_decipoint, s.c_str());
    return true;
}

void Gadacz::getDeciPoint(char *buf)
{
    strcpy(buf, _decipoint);
}

String Gadacz::getDeciPoint()
{
    return String(_decipoint);
}

bool Gadacz::isSpeaking()
{
    return is_Speaking;
}

void Gadacz::setSpeakCallback(void (*fun)(const char *))
{
    spkCallback = fun;
}

void Gadacz::setSpecialChar(char znak)
{
    _specChar = znak;
}


/* beeper */
bool AudioGeneratorBeep::begin(int freq, int duration, AudioOutputI2S *out)
{
    phase = 0;
    beepvol = (_external_dac) ? 1000 : 5000;
    samplen = freq;
    if (duration < 25) duration = 25;
    beeplen = duration * 44;
    output=out;
    running=true;
    out->SetRate(44100);
    out->SetBitsPerSample(16);
    out->SetChannels(1);
    return out->begin();
}

bool AudioGeneratorBeep::stop()
{
    if (!running)
        return true;
    running = false;
    output->stop();
    return true;
}

bool AudioGeneratorBeep::isRunning()
{
    return running;
}

bool AudioGeneratorBeep::loop(void)
{
    while(running) {
        if (phase >= beeplen) {
            stop();
            break;
        }
        int pos = phase % samplen;
        int16_t sample = (pos > samplen / 2) ? beepvol : -beepvol;
        lastSample[AudioOutput::LEFTCHANNEL] =
            lastSample[AudioOutput::RIGHTCHANNEL] = sample;
        if (!output->ConsumeSample(lastSample))
            break;
        phase++;
    }
    return running;
}
