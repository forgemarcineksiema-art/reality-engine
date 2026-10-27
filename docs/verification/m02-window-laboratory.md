# Raport okna i laboratorium — etap M02

Data: 25.10.2026. Status: **ZALICZONE — pełny odbiór M02 w zakresie briefu.** Instalacja Narzędzi grafiki została ukończona i zweryfikowana. Test D3D11 z warstwą diagnostyczną oraz wszystkie pełne zestawy przeszły: Debug 45/45, RelWithDebInfo 45/45, ASan 47/47. Nie użyto filtrów ani pominięć. [Brief](../work-items/m02-window-laboratory.md) określał zakres przed kodem, [model laboratorium](../window-laboratory.md) opisuje konkretne API.

## Wykonane

Okno Win32, renderer D3D11, statyczna proceduralna scena, własna kamera i wybór obiektu po trójkątach, standardowe kontrolki i inspektor. Rezerwacja, rozpoczęcie, przerwanie i zużycie przechodzą przez rzeczywisty M01. Zapis zachowuje świat, odczyt pracy przywraca ją z pauzą. Nowy konsument obsługuje numerację komend na podstawie utrwalonej historii. Format M01 pozostał bez zmiany.

Potwierdzony adapter: NVIDIA GeForce MX330, vendor 4318, device 7446, feature level 11.0 (`0xb000`), renderer sprzętowy. Zwykły test GPU odczytał rzeczywiste piksele, zmienił rozmiar, zminimalizował i przywrócił okno oraz zapisał ukończony świat. Osobne wejście Computer Use potwierdziło wybór bryły, rezerwację, odmowę konkurenta, rozpoczęcie, krok, zapis, odtwarzanie, odczyt i maksymalizację. Nie jest to ludzka akceptacja gry.

## Końcowe sprawdzenia

| Komenda | Wynik | Status |
| --- | --- | --- |
| `ctest --preset windows-debug -R "^lab[.]hardware_gpu_integration$" -V` | MX330, `hardware=1 gpu_debug=1`, bez ostrzeżeń lub błędów warstwy D3D11; [log](evidence/m02-gpu-debug-ctest.log) | ZALICZONE |
| `ctest --preset windows-debug` | Pełne 45/45; [log](evidence/m02-debug-final-ctest.log) | ZALICZONE |
| `ctest --preset windows-release` | Pełne 45/45; [log](evidence/m02-release-final-ctest.log) | ZALICZONE |
| `ctest --preset windows-asan` | Pełne 47/47, w tym test GPU i kontrolowane próbki pamięci; [log](evidence/m02-asan-final-ctest.log) | ZALICZONE |
| `cmake --build --preset windows-debug --target format-check` | Bez błędów stylu | ZALICZONE |
| `python tools/check_docs.py` | Rejestr, linki, ID i integralność archiwum | ZALICZONE |

Odbiór obejmuje istniejące testy M00/M01, testy konsumenta i czasu, zwykły tryb GPU oraz żądaną diagnostykę w każdej konfiguracji. Źródła pozostały zgodne ze snapshotem 31 plików: do usunięcia blokady wystarczyła instalacja prawidłowego składnika systemu, bez zmiany testów lub wyłączenia diagnostyki.

Bezpośrednia próba `reality_lab.exe --smoke-test --gpu-debug` zachowała również [natywny log UTF-8](evidence/m02-gpu-debug.log), [początkowy obraz GPU](evidence/m02-gpu-debug-scene.png.before.png) i [obraz po zakończeniu oraz resize](evidence/m02-gpu-debug-scene.png). Wykazano wybór sceny, konkurencję o materiał, zapis w kroku 40, poprawny odczyt i zakończenie w kroku 100: zużycie 3, ilość 0, rezerwacja 0. Ponowne rozpoczęcie ukończonej czynności nie wywołało nowego skutku. D3D11 InfoQueue nie zgłosiła ostrzeżeń ani błędów; nie jest to gwarancja wykrycia wszystkich możliwych błędów GPU.

## Historia blokady i instalacji

Pierwszy test z `--gpu-debug` zakończył się błędem `D3D11CreateDevice HRESULT=0x887a002d`, czyli brakującym składnikiem SDK. [Zachowany log](evidence/m02-initial-gpu-debug-failure.log) zawiera tę porażkę. Odczyt lokalny nie znalazł `D3D11_3SDKLayers.dll`; zapytanie o opcjonalny składnik Windows wymaga uprawnień administratora, których proces nie ma.

Nie usunięto testu, nie dodano skip i nie osłabiono wymagania braku ostrzeżeń. `lab.hardware_gpu_integration` nadal wymaga warstwy diagnostycznej. Dodany `lab.hardware_gpu_standard` oddzielnie sprawdza zwykłą konfigurację sprzętową. Aplikacja nie wyłącza żądanej diagnostyki automatycznie. Początkowy brak pełnego odbioru został rozwiązany wykonaniem wymaganej próby po instalacji.

Po wyraźnej zgodzie użytkownika uruchomiono [instalator](../../tools/install_graphics_tools.ps1) w podniesionym procesie PowerShell. Skrypt zainstalował wyłącznie `Tools.Graphics.DirectX~~~~0.0.1.0`, zapisał wynik i sprawdził stan; nie zmieniał execution policy ani sterowników. [Końcowy raport instalacji](evidence/m02-graphics-tools-install.json) zawiera `status=installed`, `restartNeeded=false`, `error=null`. Potwierdzono również obecność `D3D11_3SDKLayers.dll`; dopiero potem uruchomiono wymagane testy. Sama obecność pliku nie była kryterium ich zaliczenia.

## Zachowane wcześniejsze sprawdzenia

Poniższe przebiegi pokazują wcześniejszy stan blokady. Końcowy odbiór określają pełne zestawy powyżej; zachowujemy również wcześniejsze porażki oraz jawnie opisane podzbiory.

| Sprawdzenie | Wynik i ograniczenie |
| --- | --- |
| Build Debug i RelWithDebInfo | Zbudowane z C++20 oraz `/W4 /WX`; po naprawie parsera przebudowano oba |
| Pełny Debug i RelWithDebInfo przed końcowym odbiorem | 44/45 w każdym; porażka żądanej warstwy GPU. [Debug](evidence/m02-debug-ctest.log), [RelWithDebInfo](evidence/m02-release-ctest.log) |
| Dostępny podzbiór Debug i RelWithDebInfo po poprawce parsera | 44/44 w każdym; komenda jawnie wyklucza jedną niedostępną próbę GPU, która nadal jest wymagana w pełnym zestawie. [Debug](evidence/m02-debug-available-tests.log), [RelWithDebInfo](evidence/m02-release-available-tests.log) |
| Build AddressSanitizer | Zbudowany z nowym konsumentem, rendererem, aplikacją i testami |
| Dostępny podzbiór ASan | 46/46 po jawnym wyłączeniu z komendy jednej niedostępnej próby GPU, bez zmiany rejestru CTest. Nie jest to pełny wynik 47 testów. [Log](evidence/m02-asan-available-tests.log) |
| Zwykły sprzętowy smoke po naprawie | MX330, termin 100, zużycie 3 i ilość 0, zapis/odczyt, resize i zamknięcie. [Log](evidence/m02-gpu-standard.log) |
| Odczyt GPU | Przed wznowieniem 190473 piksele geometrii; po zakończeniu i resize 151898. [Obraz początkowy](evidence/m02-scene.png.before.png), [obraz końcowy](evidence/m02-scene.png) |
| Wejście OS przez Computer Use | Wybór bryły, odmowa, skróty, zapis i odczyt, odtwarzanie, kamera oraz maksymalizacja; [wygląd okna](evidence/m02-window-ui.png) |
| Minimalizacja z odtwarzaniem | Rzeczywiste wejście OS: krok 169 przy minimalizacji i przywróceniu, po przerwie. [Log](evidence/m02-ui-lifecycle.log) |
| Regresja parsera | Timeout przed poprawką, niezerowy proces i raport po poprawce; [analiza](identity-fantasy-and-assessment.md#code-review) |
| Styl i dokumentacja | `format-check`, `python tools/check_docs.py` i 27 regresji narzędzi przeszły |

Liczby testów obejmują także narzędzia, dokumentację i dotychczasowe M00/M01. Nie sumujemy konfiguracji jako nowych funkcji. Odczyty pikseli nie są benchmarkiem, a Computer Use nie zastępuje oceny gry przez człowieka. Wcześniejsze filtrowane podzbiory nie są dowodem pełnego odbioru; końcowe pełne zestawy wykonano oddzielnie po instalacji.

[Snapshot wejść kompilacji i testów](evidence/m02-sources.json) zachowuje SHA-256 źródeł i narzędzi. [Odpowiedź na ocenę i decyzję o fantasy](identity-fantasy-and-assessment.md) oddziela zatwierdzoną wizję, propozycje mechanik i wykonane dowody. Zmiana fikcji nie była powodem usunięcia testu ani modyfikacji semantyki rdzenia.

## Granice

Nie ma ruchu, fizyki, nawigacji, autonomii ani benchmarku FPS. Obiekty mają stałe rozmieszczenie narzędziowe. Nie sprawdzono odzyskiwania urządzenia, wielu monitorów i zmiany DPI ani dłuższej próby obciążenia. Limity danych M01 nadal obowiązują.
