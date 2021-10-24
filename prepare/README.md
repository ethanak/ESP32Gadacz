# Przygotowanie środowiska

Ponieważ biblioteki zawierają dość dużą ilość stablicowanych danych
(w przypadku biblioteki Mimbrola i języka polskiego jest to ok 2.5MB),
konieczne jest przeznaczenie całej dostęonej pamięci flash dla aplikacji.
Niestety - autorzy Arduino IDE nie przewidzieli takiej sytuacji,
ale można sobie z tym łatwo poradzić.

Prede wszystkim: trzeba zlokalizować folder, w którym znajdują się
dane pakietu ESP32. Dla typowej komfiguracji Linuksa będzie to:

```~/.arduino15/packages/esp32/hardware/esp32/<numer_wersji>/```

czyli np. dla wersji 2.0.0 (aktualnej w chwili pisania):

```~/.arduino15/packages/esp32/hardware/esp32/2.0.0/```

Wewnątrz tego folderu znajdziemy folder ```tools```. a w nim folder
```partitions```, zawierający opisy partycji flash w postaci plików csv.
Należy skopiować tam plik ```apponly.csv```, w którym zawarto opis
partycji przeznaczający maksymalną dostępność pamięci flash dla
aplikacji.

To niestety nie wszystko.

Teraz należy otworzyć dowolnym porządnym edytorem (tzn. takim, który
nie będzie mądrzejszy od użytkownika i nie będzie próbował zmieniać
zakończeń linii na windowsowe czy dopisywać jakieś swoje mądre rzeczy,
np. BOM, porządnym edytorem pod Windows jest np. Notepad++) plik
```boards.txt```, najlepiej uprzednio kopiując go gdzieś w bezpieczne
miejsce. W pliku znajdują się opisy różnych płytek, należy znaleźć
tę która nas interesuje. Załóżmy, że będzie nią Lolin32 Lite. W pliku
lokalizujemy sekcję odpowiadającą naszej płytce (można po prostu szukać
ciągu znaków ```".name="```), a w niej linie odpowiedzialne za dobór
odpowiedniego opisu partycji (czyli PartitionScheme). Za wszystkimi
tymi liniami należy dopisać linijki dodające odpowiednią pozycję
do menu. 

Przykładowe linijki dla różnych płytek:

```
#dla płytki WEMOS Lolin32 Lite
lolin32-lite.menu.PartitionScheme.apponly=4M Flash (Application only)
lolin32-lite.menu.PartitionScheme.apponly.build.partitions=apponly
lolin32-lite.menu.PartitionScheme.apponly.upload.maximum_size=4128768

# dla płytki ESP32 Dev Kit
esp32.menu.PartitionScheme.apponly=4M Flash (Application only)
esp32.menu.PartitionScheme.apponly.build.partitions=apponly
esp32.menu.PartitionScheme.apponly.upload.maximum_size=4128768
```

Teraz możemy uruchomić Arduino IDE. W menu 'Partition Scheme' dla
naszej płytki znajdzie się pozycja "4M Flash (Application only)",
i tę trzeba wybrać kompilując nasze aplikacje.

Oczywiście dla płytek z większą ilością pamięci Flash można stworzyć
różne opisy partycji, zawierające np. przestrzeń dla FATFS lub SPIFS,
oddzielną partycję dla danych Mbroli, partycję dla OTA i co tam jeszcze
sobie wymarzymy. Odsyłam do dokumentacji biblioteki
[Mimbrola](https://github.com/ethanak/Mimbrola).

