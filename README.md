# Projekt do předmětu ISA
Veronika Nevařilová (`xnevar00`), 19.11.2023

### Výsledné hodnocení
18.5/20 (-1.5 za dokumentaci, programová část 100%)

Program implementuje klientskou a serverovou část aplikace pro přenos souborů prostřednictvím TFTP.
Umožňuje přenášet po síti jak textové, tak binární soubory.

### Klient
Klient se spouští pomocí příkazu `tftp-client -h hostname [-p port] [-f filepath] -t dest_filepath`

* `-h hostname` IP adresa nebo doménový název vzdáleného serveru
* `-p port` nepovinný argument, port, na kterém kontaktuje klient server při pokusu o navázání komunikace. Pokud není zadán, používá se výchozí port 69.
* `-f filepath` cesta ke stahovanému souboru na serveru (download), pokud není zadán, bere se obsah stdin (upload)
* `-t dest_filepath` cesta, pod kterou bude soubor na klientu/serveru uložen

Klient je nastaven tak, že nepodporuje žádná rozšíření. Každopádně rozšíření jsou v implementaci zanesena a kromě tsize se dají i funkčně nastavit, to bylo ovšem implementováno hlavně z důvodu ladění serveru. Podobně jako server má klient výchozí timeout 2 sekundy a mód pro přenos je nastaven na `octet`.

### Server
Server se spouští příkazem `tftp-server [-p port] root_dirpath`

* `-p port` nepovinný argument, port, na kterém bude server přijímat klienty. Pokud není zadán, používá se výchozí port 69
* `root_dirpath` cesta k adresáři, do kterého se budou ukládat nebo se z něj budou číst soubory

Server implementuje následující rozšíření:
* TFTP Option Extension (RFC 2347)
* TFTP Blocksize Option (RFC 2348)
* TFTP Timeout Interval and Transfer Size Options (RFC 2349)

Výchozí timeout serveru je nastaven na 2 sekundy, přičemž při neúspěšném pokusu o obdržení odpovědi se doba čekání po novém zaslání packetu exponenciálně prodlužuje 2x. Po 5 neúspěšných pokusech server komunikaci ukončuje.

### Seznam souborů projektu:
#### Zdrojové soubory
* `tftp-client.cpp`
* `tftp-server.cpp`
* `client_class.cpp`
* `server_class.cpp`
* `tftp_packet_class.cpp`
* `output_handler.cpp`
* `helper_functions.cpp`

#### Hlavičkové soubory
* `tftp-client.hpp`
* `tftp-server.hpp`
* `client_class.hpp`
* `server_class.hpp`
* `tftp_packet_class.hpp`
* `output_handler.hpp`
* `helper_functions.hpp`

#### Další soubory
* `Makefile`
* `README.md`
