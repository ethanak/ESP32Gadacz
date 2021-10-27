# Simple high quality Polish language TTS library for ESP32

## DISCLAIMER

This library is provided for Polish language speakers. You **must** know
Polish to use this library, so remaining part of this description
is written in Polish.

## Kilka ogólników

### Zalety

* Dzięki wykorzystaniu zmodyfikowanej wersji Mbroli głos brzmi w miarę
naturalnie, a wykorzystanie silnika TTS pochodzącego z systemu tworzenia
audiobooków z tekstu umożliwia odczyt dowolnych, również
stosunkowo długich tekstów (np. prognozy pogody, skrótu wiadomości itp.).
* Wbudowane rozpoznawanie wielu jednostek miary
oraz możliwości uproszczonego odczytu pozwalają na bardzo proste stworzenie
programu odczytującego szybko i w czytelny sposób wyników pomiarów.
* Szybki start urządzenia - w praktyce zależnie od płytki od jednej do
niecałych dwóch sekund od chwili włączenia zasilania.
* Niska cena zarówno samego mikrokontrolera, jak i układów współpracujących
pozwala na stworzenie odpowiednika urządzenia, które w wersji komercyjnej
może być poza zasięgiem finansowym potrzebującego.

### Wady

* Duża zajętość pamięci mikrokontrolera, co pozwala na uruchomienie
na popularnych płytkach jedynie stosunkowo prostych aplikacji (tej wady
nie mają płytki z min. 8 MB pamięci flash).
* Prwdopodobnie jeszcze kilka się znajdzie...

### Przykłady zastosowania:

* Urządzenia dla niewidomych i słabo widzących, przykładowo:
  * proste urządzenia pomiarowe (np. centymetr krawiecki, taśma miernicza, dalmierz, waga itp.)
  * czujniki otoczenia (czujniki światła i koloru)
  * przystawki do urządzeń medycznych (ciśnieniomierz, glukometr, oksymetr)
  * przystawki do sprzętu AGD (kuchenka mikrofalowa, ekspres do kawy)
* Urządzenia umożliwiające odczyty głosowe pomiaru w warunkach ograniczonej możliwości dostępu do informacji wizualnej, przykładowo:
  * Multimetr
  * Suwmiarka
  * Poziomica
* Zabawki
* Urządzenia będące częścią "inteligentnego domu"

## Instalacja

Biblioteka jest przeznaczona dla płytek ESP32 i środowiska Arduino IDE.

Wymagane są następujące biblioteki:

* [Mimbrola](https://github.com/ethanak/Mimbrola) - z repozytorium git
* [microlena](https://github.com/ethanak/microlena) - z repozytorium git
* ESP8266Audio (poprzez managera bibliotek)

Przed użyciem biblioteki należy przygotować środowisko Arduino IDE (opis
w folderze [prepare](./prepare/)) oraz zainstalować polski głos dla
biblioteki Mimbrola z folderu [voice](./voice/).

## Opis funkcji

Przed wywołaniem jakiejkolwiek funkcji należy wywołać funkcję ```begin```.
Zależnie od użytego sprzętu ma ona dwie postacie:

Dla zewnętrznego przetwornika DAC I2S:

```Gadacz::begin(wclk_pin, bclk_pin, dout_pin)```

Podajemy tu trzy piny:
* wclk (podłączony do pinu WCLK lub LRCLK przetwornika)
* bclk (podłączony do pinu BCLK przetwornika)
* dout (podłączony do pinu DIN przetwornika)

Dla wewnętrznego 8-bitowego przetwornika:

```Gadacz::begin(use_gain=false)```

Można tu uzyć dowolnego prostego wzmacniacza z potencjometrem regulacji
głośności, podłączonego do pinu 25 lub 26 płytki ESP32. W normalnych
warunkach parametr use_gain można pominąć lub ustawić na ```false``` -
regulacja głośności będzie odbywać się za pomocą regulatora wzmacniacza,
a funkcja ```setVolume``` nie będzie powodować żadnyuch skutków. Takie
działanie spowodowane jest zbyt dużymi zniekształceniami, ale pozostawiono
możliwość jej użycia w celach doświadczalnych.

### Ustawienia

Wszystkie ustawienia są brane pod uwagę w przypadku wywołania dowolnej
funkcji generującej mowę (lub beep). W czasie mówienia zmiana parametrów
nie powoduje zmiany ustawień.

* ```void Gadacz::setVolume(uint8_t volume)```
* ```uint8_t Gadacz::getVolume()```

Ustawia (lub pobiera) głośność. Dopuszczalne wartości od 0 do 24.

* ```void Gadacz::setPitch(uint8_t pitch)```
* ```uint8_t Gadacz::getPitch()```

Ustawia (lub pobiera) wysokość głosu. Dopuszczalne wartości od 0 do 24.

* ```void Gadacz::setSpeed(uint8_t speed)```
* ```uint8_t Gadacz::getSpeed()```

Ustawia (lub pobiera) prędkość mówienia. Dopuszczalne wartości od 0 do 24.

* ```void Gadacz::setContrast(uint8_t contast)```
* ```uint8_t Gadacz::getContrast()```

Ustawia (lub pobiera) parametr kontrastu audio. Wyższy kontrast powoduje
zwiększenie głośnoścu mowy bez zwiększania amplitudy sygnału, lecz
jednocześnie powoduje wzrost zniekształceń. Dopuszczalne wartości od 0 do 100.

* ```void Gadacz::setSimpleDeciPoint(bool mode)```
* ```bool Gadacz::getSimpleDeciPoint()```

Ustawia tryb uproszczonego czytania wartości ułamkowych. Przykładowo
"2.50 cm" w trybie normalnym będzie odczytane jako "dwa przecinek pięćdziesiąt centymetra",
w trybie uproszczonym "dwa i oł centymetra".

* ```bool Gadacz::setDeciPoint(const char *dp)```
* ```bool Gadacz::setDeciPoint(String &s)```
* ```void Gadacz::getDeciPoint(char *buf)```
* ```String Gadacz::getDeciPoint()```

Ustawia lub pobiera słowo określające kropkę dziesiętną dla trybu normalnego
czytania (domyślnie "przecinek"). Słowo może mieć od 1 do 31 bajtów w UTF-8.
W przypadku funkcji getDeciPoint z parametrem char* bufor musi pomieścić
napis.

* ```void Gadacz::setAltColon(bool mode)```
* ```bool Gadacz::getAltColon()```

Ustawia lub pobiera tryb melodii dla fraz zakończonych dwukropkiem.

### Funkcje syntezy mowy

Wywołanie którejkolwiek funkcji syntezy w czasie trwania syntezy mowy
lub generowania dżwięku przerywa natychmiast trwającą syntezę/generowanie.

* ```void Gadacz::say(const char *text)```
* ```void Gadacz::say(String &s)```

Powoduje wypowiedzenie tekstu z parametru. Parametr będzie skopiowany
przed wyjściem z funkcji.

* ```void Gadacz::saycst(const char *text```

Powoduje wypowiedzenie tekstu z parametru. Parametr będzie przekazany
do funkcji poprzez adres, tak że nie wolno go zmieniać. Stosowany
w przypadku wypowiadania tekstów stałych, np:
```Gadacz::saycst("Cześć");```

* ```void Gadacz::sayfmt(const char *format, ...)```

Zachowuje się jak printf (w rzeczywistości używa funkcji vsnprintf),
z tym, że wyjściem jest tu syntezator.

### Pozostałe funkcje:

* ```void Gadacz::beep(int freq, int duration)```

Powoduje wydanie krótkiego dźwięku. Parametr ```freq``` określa
przybliżoną częstotliwość dźwięku względem tonu podstawowego 440 Hz,
z precyzją ćwierćtonu, ```duration``` określa czas trwania w milisekundach.
Dopuszczalne wartości dla ```freq``` to od -24 do 24, dla ```duration```
od 25 do 1000. Wywołanie funkcji w czasie trwania syntezy mowy
lub generowania dżwięku przerywa natychmiast trwającą syntezę/generowanie.



* ```bool Gadacz::isSpeaking()```

zwraca ```true```, jeśli trwa synteza mowy lub generowanie dźwięku.

* ```void Gadacz::stop()```

Przerywa trwającą syntezę mowy lub generowanie dźwięku.

* ```void Gadacz::waitAudio(uint32_t timeout=10000)```

czeka na zakończenie syntezy lub generowania dźwięku. Jeśli parametr
```timeout``` jest większu od zera, wraca po upływie określonego czasu (w milisekundach).


## Przykłady

### SerialGadacz

Program pozwala na wprowadzenie polecenia z interfejsu Serial.
Jeśli polecenie rozpoczyna się od znaku ```\``` (backslash), interpretowane jest
jako komenda nakazująca zmianę ustawień lub wykonanie określonej
czynności. W przeciwnym wypadku tekst zosttanie wypowiedziany.

Komendy:

* ```\v liczba``` - ustaw głośność
* ```\s liczba``` - ustaw prędkość
* ```\p liczba``` - ustaw wysokość
* ```\c liczba``` - ustaw kontrast
* ```\b liczba liczba``` - wydaj dżwięk

W przykładzie użyto ustawień dla podłączenia konwertera I2S do pinów
12, 13 i 14.

Odkomentoeując linię:

```C
#include "scifi.h"
```
włączamy do programu słownik wygenerowany na podstawie pliku scifi.txt
(patrz dokumentacja microleny w folderze userdic)

### Kolor

Program pokazuje, jak użyć Gadacza w aplikacji - w tym przypadku jest to czujnik koloru.
Program jest przystosowany do modułu z diodą oświetlającą i mierzy kolor
oświetlanej przez diodę powierzchni.

**UWAGA!!!**

Program służy jedynie do demonstracji, może jednak być uzżyty jako baza do napisania
pełnej użytkowej aplikacji.

Potrzebne elementy:
- Dowolna płytka ESP32 z co najmniej sześcioma pinami wolnymi (pięcioma, jeśli
płytka ma przycisk BOOT)
- Moduł czujnika koloru TCS34725 (np. Adafruit)
- Moduł I2S z głośnikiem lub moduł wzmacniacza z regulacją głośności i głośnikiem
- Przycisk (jeśli nie wykorzystujemy przycisku BOOT)

Dodatkowe biblioteki (do zainstalowania przez mabagera bibliotek):
- Adafruit_TCS34725
- Bounce2

Przed skompilowaniem przykładu należy sprawdzić, czy numery pinów
w programie są zgodne z naszymi połączeniami. Należy pamiętać,
że w przypadku użycia wewnętrznego przetwornika należy odkomentować linię:
```C
#define INTERNAL_DAC
```

Ponieważ zachowanie czujnika zależy od wielu czynników (obudowa,
umieszczenie diody podświetlającej itd.) należy dokonać wstępnej
kalibracji. W tym celu:

* Podkładamy pod czujnik białą kartkę i dokonujemy pomiaru. Odczytujemy
z monitora Serial wartości MPX_R i MPX_B, wpisujemy je w odpowiednie
miejsce w programie i kompilujemy jeszcze raz. W ten sposób wyrównujemy
różnice poziomów RGB dla białej powierzchni.
* Podkładamy pod czujnik czarną matową powierzchnię lub lepiej w ciemnym
pomieszczeniu kierujemy czujnik w ciemny kąt i dokonujemy pomiaru.
Odczytane wartości ZERO_R, ZERO_G i ZERO_B wpisujemy do programu
i powtórnie kompilujemy. W ten sposób eliminujemy efekt crosstalku,
gdy część światła diody pada na czujnik.
* Ponownie podkładamy białą kartkę, dokonujemy pomiaru, odczytujemy wartość
WHITE_C i wpisujemy w odpowiednie miejsce w programie.
* Po ponownej kompilacji program jest gotowy do pracy.


