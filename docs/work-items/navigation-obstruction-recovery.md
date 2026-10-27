# Odzyskiwanie ruchu po blokadzie

Status: wykonano opisany zakres techniczny; [raport i granice odbioru](../verification/navigation-obstruction-recovery.md). Zlecenie po zgłoszeniu użytkownika z trzema zrzutami z 26 października 2026. Sesja została zamknięta bez zapisu, więc zrzuty są obserwacją objawu, nie dokładną reprodukcją stanu ani dowodem przyczyny.

Cel: postać ma omijać nieruchomego sąsiada tam, gdzie istnieje dostępna trasa, a przy rzeczywistej blokadzie stabilnie oczekiwać i ponowić planowanie po jej zwolnieniu. Obracanie i drobny ruch bez postępu nie stanowią zaliczenia. Zachować cel gracza, fizyczną kolizję, kolejki, ograniczenie wiedzy do otoczenia i deterministyczną kontynuację.

Zakres: sesja przestrzenna, jej planer powierzchniowy i diagnostyka, regresje w `tests/traversal_audit_tests.cpp`, rejestracja CTest oraz aktualna dokumentacja. Kontrakty INV-09, INV-10, INV-11, INV-12 i INV-16; scenariusze SC-06, SC-07, SC-08 i SC-11. Bez przebudowy fizyki, gotowej biblioteki tłumu i otwierania M04. Przed zmianą produkcji zachować kod bazowy oraz czerwony wynik.

Odbiór: rzeczywiste dotarcie w otwartym wariancie ściana–sąsiad, stabilne położenie i orientacja przy zamkniętym korytarzu, wznowienie po odejściu blokującego, brak penetracji, kontynuacja po zapisie w trakcie blokady oraz zachowanie dotychczasowych regresji. Pełne Debug, RelWithDebInfo i ASan, styl, dokumentacja i właściwa próba GPU. Nie podnosić limitów czasu testów.

## Zachowania społeczne — projekt dalszego przyrostu

Użytkownik oczekuje również szukania rozwiązania przez postać: rozpoznania blokującej osoby, prośby o ustąpienie oraz możliwej odmowy. Samo zatrzymanie nie jest docelową autonomią. Ta warstwa nie jest obecnie zaimplementowana i nie może być zastąpiona automatycznym przesuwaniem innych.

Proponowana kolejność: fizyczne wykrycie lokalnej blokady → sprawdzone obejście → jawna prośba skierowana do rozpoznanej osoby → jej niezależna decyzja → fizycznie wykonalne ustąpienie albo odmowa i zmiana planu proszącego. Wymaga to określenia kanału percepcji/komunikacji, priorytetów własnych zadań, miejsca i kosztu ustąpienia, terminów odpowiedzi, ochrony przed wzajemnym żądaniem oraz zapisu zobowiązania. Gracz zachowuje kontrolę nad własną postacią. Nie ma jeszcze zatwierdzonych reguł konfliktu, agresji lub konsekwencji społecznych; nie dopisujemy ich jako ukrytej reguły ruchu.

Oddzielny przyszły test ma wykazać faktyczną prośbę, zgodę i ruch, odmowę bez wymuszonego ruchu, dwie równoczesne prośby, niewykonalne ustąpienie oraz kontynuację po zapisie. Ten przyrost naprawia podstawę nawigacji; nie zalicza tych przyszłych zachowań.
