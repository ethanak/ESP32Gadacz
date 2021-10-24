# Przygotowanie głosu dla biblioteki Mimbrola

W folderze, w którym zainstalowano bibliotekę znajduje się
folder ```data```, przeznaczony do instalacji głosu. Do tego folderu
należy skopiować folder ```pl_alaw_app```, zawierający polski głos
skompresowany algoryemem A-law i przystosowany do wkompilowana
w aplikację.

Następnie należy zamienić plik ```config.h``` z głównego folderu
Mimbroli na załączony.

Od tej chwili Mimbrola będzie używać języka polskiego.

**UWAGA**

Polski głos został przygotowany na bazie
[głosu pl1](https://github.com/numediart/MBROLA-voices/tree/master/data/pl1),
szczegóły licencyjne w linku!
