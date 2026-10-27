# Brief pierwszego przyrostu C++ — etap M00

Status: wykonane w opisanym zakresie; [dowód](../verification/m00-build-and-diagnostics.md). Cel: wykonać M00 z [planu](../prototype-plan.md) i rozpocząć implementację, bez udawania gotowego świata.

## Zakres

CMake, presety MSVC x64, ostrzeżenia, kontrola stylu, build zwykły i AddressSanitizer. Minimalnym rzeczywistym konsumentem rdzenia jest program konsolowy wykonujący jawnie zadaną liczbę kroków zegara. Nie powstają puste moduły renderera, populacji, fizyki ani nawigacji.

Zakres plików: konfiguracja w katalogu głównym, zegar w `src/core`, program w `src/headless`, testy C++ w `tests`, próbka diagnostyczna w `tests/diagnostics`, wrapper procesu w `tools` i odpowiadające mu testy oraz aktualizacja aktualnego stanu dokumentacji. Lokalny Git zostaje zainicjalizowany bez remote i publikacji.

## Kontrakt zegara

Zegar ma dodatni stały krok w nanosekundach, początkowo zero wykonanych kroków i zero upływu. Czas świata rośnie wyłącznie po jawnym wywołaniu. Zero dodatkowych kroków jest dozwolonym no-op. Konstruktor odrzuca krok niedodatni. Przyrost przekraczający zakres reprezentowalnego upływu jest odrzucony przed zmianą stanu. Nie ma zależności od zegara ściennego. Dotyczy INV-11; to nie scheduler ani zapis.

## Dowody odbioru

- Build C++20 Debug i RelWithDebInfo/ASan z ostrzeżeniami jako błędami.
- Testy początkowego stanu, niedozwolonego kroku, dokładnego upływu, równoważnych partii i braku mutacji po przepełnieniu.
- Osobne uruchomienie rzeczywistego testu z celowo fałszywym oczekiwaniem musi zwrócić błąd i właściwy komunikat.
- Oddzielna próbka poprawna i próbka heap-buffer-overflow muszą wykazać sprawność diagnostyki zgodnie z początkowym zakresem SC-15. Próbka z błędem nie jest częścią kodu gry.
- Wrapper procesu nie może zaliczyć braku programu, timeoutu, innego błędu ani komunikatu bez właściwego kodu wyjścia.
- Program konsolowy sprawdza argumenty i daje deterministyczny wynik jawnego postępu zegara.
- Istniejące kontrole dokumentacji i nowe testy narzędzi muszą przejść.

## Granice

Nie badamy GPU, FPS, autonomii, kolizji, tras, zapisu świata ani wyścigów wątków. Wątków symulacji jeszcze nie ma. Wynik końcowy i dokładne komendy znajdą się w osobnym raporcie.
