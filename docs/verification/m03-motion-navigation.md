# Weryfikacja ruchu i nawigacji — etap M03

**Status: M03 ZALICZONY w obsługiwanym zakresie, z uwagami rozwojowymi, 25.10.2026.** Zakres techniczny przeszedł testy, a użytkownik po uruchomieniu prototypu zaakceptował obecne sterowanie, komfort i czytelność. Naturalność ruchu, reakcja na cel i prowadzenie trasy wymagają dalszego dopracowania opisanego poniżej. Zakres określa [model](../motion-navigation.md), a cel [brief](../work-items/m03-motion-navigation.md). Nie zaliczamy planowanej pełnej fizyki, autonomii ani atrakcyjności Action RPG.

## Końcowe sprawdzenia

Po ostatniej zmianie kodu przebudowano wszystkie trzy konfiguracje. Pełne zestawy uruchomiono bez filtrów i pominięć. [Manifest 39 plików źródłowych i konfiguracji](evidence/m03-sources.json) obejmuje również testy i narzędzia; zgodność SHA-256 sprawdzono ponownie po testach.

| Polecenie / dowód | Wynik | Granica |
| --- | --- | --- |
| `cmake --preset windows-msvc` | ZALICZONE | MSVC x64 / Windows SDK |
| `cmake --build --preset windows-debug` i `ctest --preset windows-debug` | 59/59, [pełny log](evidence/m03-debug-final-ctest.log) | Debug, rzeczywiste testy GPU i regresje M00–M02 |
| `cmake --build --preset windows-release` i `ctest --preset windows-release` | 59/59, [pełny log](evidence/m03-release-final-ctest.log) | RelWithDebInfo |
| `cmake --preset windows-asan`, build i `ctest --preset windows-asan` | 61/61, [pełny log](evidence/m03-asan-final-ctest.log) | ASan, w tym odrębne wykrycie celowego heap-buffer-overflow |
| `cmake --build --preset windows-debug --target format-check` | ZALICZONE | Jawna lista własnych plików C++ |
| `python tools/check_docs.py` | ZALICZONE | Rejestr, linki lokalne, ID i integralność archiwum |
| `python -m unittest discover -s tools/tests -p "test_*.py"` | 29/29 | Narzędzia dokumentacji i procesów, nie gra |

Po uzupełnieniu tego raportu ponowiono kontrolę dokumentacji, testy narzędzi i stylu. Czasy CTest są czasami testów poprawności, nie benchmarkiem gry. Nie badano wyścigów wątków; nie dodano współbieżnej symulacji.

## Sprzęt i kontynuacja procesu

[Log pierwszego procesu](evidence/m03-native.log) i [wznowienia](evidence/m03-resume.log): rzeczywista NVIDIA GeForce MX330, vendor 4318, device 7446, feature level `0xb000`, `hardware=1 gpu_debug=1`. Nie zgłoszono ostrzeżeń/błędów kolejki diagnostycznej; nie użyto WARP ani cichego fallbacku.

Po krótkim i przytrzymanym wejściu, zmianie geometrii oraz zapisie starej trasy oba procesy kontynuują podróż i pracę do kroku 370. A dociera do (7; 0) m, wykonuje 25 punktów, licznik planowań wynosi 2. Suma całego stanu kontynuacji w obu przebiegach: `17725397838265440714`. Resize daje 624×721, minimalizacja i powrót zachowują krok 370. [Początek](evidence/m03-native.png.before.png), [wynik](evidence/m03-native.png), [wynik po wznowieniu](evidence/m03-resume.png) to odczyty bufora sprzętowego GPU. Automat sprawdza osobno rzeczywiste piksele i stan zapisu.

## Dotychczasowe wyniki

Własny kontroler 2,5D, A*, wersjonowanie geometrii, objazd, oczekiwanie, wspólny zegar i wersjonowany zapis podróży są połączone w `reality_lab --motion`. Nie powstały potrzeby, transport zasobów, walka ani pełny solver brył.

Testy geometryczne i integracyjne obejmują SC-06, SC-07, obsługiwany zakres SC-08 oraz ruch z SC-11. Test procesu korzysta z rzeczywistego urządzenia D3D11 z debug layer i porównuje kontynuację podróży/pracy w dwóch procesach. Nie stosuje atrapy renderera lub przygotowanej odpowiedzi A*.

## Rzeczywiste wejście

Próba Computer Use na MX330: PPM wyznaczył cel obok B, Spacja uruchomiła ruch, postać A dotarła do około (-6,30; -2,98) m. `F` wykazał lokalny kontakt z osobą ID 2, F5 zapisał, F9 przywrócił krok 138 i położenie oraz pauzę. Kamera Q zmieniła obraz bez zmiany kroku świata. [Kontakt](evidence/m03-ui-contact.png), [log kontaktu](evidence/m03-ui-contact.log).

W pierwszej próbie krótkie D nie przesunęło postaci: keydown i keyup mogły zmieścić się pomiędzy krokami. Poprawiono bufor pojedynczego impulsu, dodano test modelu i komunikatów Win32. Powtórzona próba prawdziwej klawiatury D, potem N: pozycja z (-7; 0) do około (-6,87; -0,08) m, czyli jeden krok ruchu 0,15 m, przy 20 krokach świata. [Dowód poprawki](evidence/m03-ui-short-key.png).

G zamknął główne przejście, T wyznaczył [trasę objazdową](evidence/m03-ui-detour.png). H zamknął objazd: poprzednia trasa została zaznaczona jako stara. N wykonał ponowne planowanie, stan [no_path](evidence/m03-ui-no-path.png), pozycja pozostała bez zmiany. [Log po poprawce](evidence/m03-ui-fixed.log). Własne okno testowe zamknięto normalnie.

To dowód działania wejścia i integracji, nie ludzka ocena komfortu, czytelności lub atrakcyjności gry.

## Ocena użytkownika

25.10.2026 użytkownik przekazał własną ocenę po uruchomieniu laboratorium M03. Nie zgłosił większych uwag do ruchu ręcznego, komfortu ani czytelności; dla obecnego etapu ocenił sterowanie jako akceptowalne. Zgłosił potrzebę bardziej płynnej i logicznej reakcji na wskazanie celu, wyboru trasy i jej wykonywania. Szybkość oraz naturalność ruchu i rytmu symulacji pozostawił do przemyślenia i dopracowania. Nie wskazał konkretnej docelowej prędkości ani nie zatwierdził ostatecznego modelu ruchu.

Interpretacja odbioru: **ZALICZONE dla obecnego prototypu, z uwagami rozwojowymi**, a nie bezwarunkowa akceptacja docelowej lokomocji. Nie zgłoszono błędu blokującego obecne sterowanie; uwag o nawigacji nie usuwamy po zamknięciu etapu. Nie przypisujemy użytkownikowi wykonania poszczególnych scenariuszy technicznych, kontaktu F ani testu zapisu — zakres jego relacji dotyczy oceny odczuć. To opinia jednego użytkownika, nie kontrolowane badanie wielu graczy ani odbiór pełnego Action RPG.

Przegląd kodu potwierdza czterokierunkową siatkę, przechodzenie przez centra komórek i stałą prędkość 3 m/s bez rozpędzania oraz hamowania. Mogą wyjaśniać część odczuwanej kanciastości; nie zidentyfikowano konkretnego błędu reakcji na cel ani nie zmierzono opóźnienia wejścia. Kierunek dalszej poprawy i warunki jego weryfikacji zapisano w [projekcie gry](../game-design.md#wczesna-próba-bezpośredniej-rozgrywki).

## Przegląd integratora

Osobne przejście kontrolne objęło własność zegara i położenia, granice zatwierdzania kroków, semantykę krótkich naciśnięć, odrzucenie starej trasy, zatrzymanie zamiast ślizgu na trasie oraz walidację kandydata zapisu. Dodano regresję zerowej podróży: zmiana mapy nie powoduje wycieczki do centrum komórki, gdy postać już stoi u celu. Polecenie nowego celu czyści wcześniejsze wejście. Nie wykonano niezależnego review przez drugiego autora.

Początkowa próba dwóch testów miała błędne fixture: prosty odcinek przez otwarte przejście został mylnie oznaczony jako kolizja, a fixture sumy szukała wersji geometrii sprzed zamknięcia. Poprawiono dane według znanej geometrii i kolejności komend; nie usunięto przypadków odrzucenia, nie zwiększono tolerancji ani nie oznaczono porażek jako sukcesów. Sprawdzenie dokumentów ujawniło też fałszywe ID w adresie publikacji; regresje narzędzia zachowują kontrolę widocznej etykiety i lokalnych odnośników.

## Nadal otwarte

Dopracowanie płynności prowadzenia do celu, reakcji na zmianę celu, szybkości i naturalności ruchu; szersze próby z graczami, długie obciążenie i pomiary FPS/pamięci, inne platformy oraz geometria spoza kontraktu. Mapa jest globalnym narzędziem diagnostycznym; postać podążająca do celu nie dowodzi autonomii lub wiedzy NPC. Kamerę gry i fabularną tożsamość toru pozostawiono otwarte.
