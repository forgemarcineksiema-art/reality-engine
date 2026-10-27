# Krótka próba społeczności dla użytkownika

Status: **ZALICZONE przez użytkownika 27.10.2026**. Po uruchomieniu przygotowanej sceny użytkownik przekazał: „M04 Zaliczone”. To bezpośrednia akceptacja etapu, odrębna od [raportu technicznego](m04-acceptance.md). Nie dopisujemy niewypowiedzianych ocen szczegółowych ani potwierdzenia wykonania każdego kroku instrukcji. Poniżej zachowano instrukcję przygotowanej próby. Około 5–10 minut, cztery świeże uruchomienia tej samej sceny, bez cudzych zapisów. Nie trzeba czytać technicznego inspektora ani sterować mieszkańcami.

## Uruchomienie

Z katalogu projektu: `python tools/start_m04_playtest.py`. Launcher uruchamia Release `reality_traversal --community` i tworzy odrębny katalog `build/m04-playtest-DATA-ID` na zapis F5, log i diagnostykę. Nie wczytuje istniejącego świata. Jeśli exe brakuje: `cmake --build --preset windows-release`. Sam launcher nie jest wykonaniem testu człowieka. Zamknij okno przed następną próbą i ponownie uruchom to samo polecenie.

Start w pauzie. Niebieska osoba 1 to podróżny przy przyczółku. Osoba 2 to autonomiczny kurier, 3 i 7 odbierają żywność po prawej, 4 konserwuje przejście, 5 opiekuje się częściami, 6 i 8 są mieszkańcami po lewej. Klawisze 1–8 zmieniają tylko wybór, F2 przełącza sterowanie; do próby nie przejmuj NPC. Rola wybranej osoby i liczniki widoczne są w panelu.

Spacja uruchamia/pauzuje; N przesuwa świat o 1 s w pauzie. F8 zmienia pobliską przeprawę; R próbuje lokalnej pracy; F4 pobiera albo oddaje partię; F7 przekazuje własną informację najbliższej osobie. F5 zapisuje, F9 wczytuje i pauzuje. Most jest u góry sceny, prom niżej; zielone znaczniki to żywność, złoty części/konserwacja, fioletowe odpoczynek. Zbliżenie kamery: +/-, obrót Q/E.

## A. Ludzie bez poleceń

1. Uruchom świeżą scenę i naciśnij Spację. Przez około 30 s obserwuj kuriera i panel. Pozostaw mieszkańców autonomicznych.
2. Oczekiwany kierunek: rośnie Dostawa i liczba Posiłków po prawej; ktoś odpoczywa, trwa praca konserwatora. Osiem osób nie oznacza nieskończonej gospodarki: zapasy są skończone.
3. W pauzie F5, następnie N kilka razy i F9. Czy potrafisz rozpoznać, że wrócił wcześniejszy świat i jego stan?

## B. Osobista ingerencja

1. W nowym uruchomieniu, w pauzie, naciśnij F8 przy wybranej osobie 1. Most powinien zmienić kolor i panel pokazać zamknięcie.
2. Uruchom czas lub używaj N. Obserwuj reakcję kuriera i konserwatora. NPC poznają przeszkodę na miejscu; sam kolor mostu nie jest wiadomością dla nich. Konserwator może sam otworzyć przeprawę po rzeczywistej pracy.
3. W kolejnym świeżym uruchomieniu zamknij most i spróbuj R, aby osobiście pomóc w pracy. Porównaj to z pozostawieniem mieszkańców samym sobie. Jeśli odmówi, zanotuj komunikat zamiast zakładać wykonanie.

## C. Przejęcie i wiedza

1. W nowym uruchomieniu, w pauzie, F4 pobiera cztery części do rzeczywistej torby podróżnego. Panel pokazuje ładunek. W pobliżu jest świadek; opiekun stoi dalej.
2. N pozwala świadkowi osobiście przekazać wiadomość. Panel powinien pokazać odmowę obsługi podróżnego; R nie rozpoczyna jego pracy.
3. Ponowne F4 fizycznie oddaje partię. N pozwala przekazać informację o zwrocie. Obsługa powinna wrócić; R ponownie pozwala pomóc. Brak wiedzy o zwrocie może pozostawić starszą odmowę — samo opróżnienie torby nie jest wiadomością.

## Odpowiedź po próbie

Napisz krótko:

- Czy było jasne, kto co robi i dlaczego dostawa/posiłek lub odmowa nastąpiły?
- Czy dostrzegłeś samodzielne działanie mieszkańców oraz skutek własnej ingerencji?
- Co przeszkadzało: wybór postaci, kamera, tempo, komunikaty, odczyt ładunku lub stanu mostu?
- Czy zapis/odczyt wyglądał wiarygodnie? Przy problemie podaj katalog z launchera i użyj F6, jeśli okno nadal działa.

Odbiór M04 został potwierdzony powyższą odpowiedzią użytkownika. Ewentualne dalsze zgłoszenia wymagają własnej reprodukcji; zaliczenie nie usuwa ograniczeń opisanych w raporcie. Szersze badanie graczy i długi benchmark należą do M06.
