# Raport fundamentu C++ — etap M00

Data: 25.10.2026. Zakres: rzeczywisty build C++20, zegar symulacji, runner i diagnostyka. Git zainicjalizowano lokalnie; nie ma commitu ani remote. [Brief](../work-items/m00-build-and-diagnostics.md) określał kryteria przed implementacją.

## Wykonane

- Biblioteka `reality_core` ze stałym, dodatnim krokiem zegara i kontrolą przepełnienia przed zmianą stanu.
- `reality_headless`: jawne wykonanie liczby kroków, walidacja argumentów i wynik w nanosekundach.
- `reality_tests`: osiem przypadków zegara, uruchomienie pojedynczego testu oraz faktyczna ścieżka niezaliczonej asercji.
- Presety Debug, RelWithDebInfo i AddressSanitizer, ostrzeżenia jako błędy, kontrola stylu C++.
- Kontrola procesu wymagająca jednocześnie właściwego kodu wyjścia i komunikatu; jej testy obejmują timeout, brak programu oraz niepowiązany błąd.
- Osobna poprawna i celowo błędna próbka pamięci. Błędna nie jest częścią gry ani zwykłego buildu.

Nie dodano atrap rendererów, osób, fizyki ani nawigacji. Kod domenowy zegara jest rzeczywiście używany przez program i testy.

## Wykonane komendy

| Komenda | Wynik | Status |
| --- | --- | --- |
| `cmake --preset windows-msvc` | Konfiguracja MSVC x64 i Windows SDK | ZALICZONE |
| `cmake --build --preset windows-debug` | Build z C++20 i `/W4 /WX` | ZALICZONE |
| `ctest --preset windows-debug` | 18/18 sprawdzeń | ZALICZONE |
| `cmake --build --preset windows-release` | Zwykły RelWithDebInfo | ZALICZONE |
| `ctest --preset windows-release` | 18/18 sprawdzeń, w tym faktyczna odmowa fałszywej asercji | ZALICZONE |
| `cmake --preset windows-asan` | Osobna konfiguracja z runtime diagnostycznym | ZALICZONE |
| `cmake --build --preset windows-asan` | Build instrumentowany i oddzielna próbka | ZALICZONE |
| `ctest --preset windows-asan` | 20/20 sprawdzeń, w tym poprawna próbka i wykrycie heap-buffer-overflow | ZALICZONE |
| `cmake --build --preset windows-debug --target format-check` | clang-format 19.1.5, bez błędów stylu | ZALICZONE |
| `./build/msvc/Debug/reality_headless.exe --steps 12 --step-ms 25` | 12 kroków, 300000000 ns upływu | ZALICZONE |
| `ctest --preset windows-debug -R "^clock[.]overflow_preserves_state$"` | Jeden wybrany test przechodzi | ZALICZONE |
| `python -m unittest discover -s tools/tests -p "test_*.py" -v` | 27 regresji narzędzi w Pythonie 3.11.9 | ZALICZONE |
| `python tools/check_docs.py` | Spójny rejestr, linki, ID i archiwum | ZALICZONE |

Liczby CTest obejmują również kontrolę dokumentacji i testy narzędzi; nie są liczbą niezależnych funkcji silnika. Profil ASan dodaje dwie próby diagnostyczne. Wyniki zestawów powtarzanych w różnych konfiguracjach nie są sumowane jako nowe zachowania.

## Dowody i kontrola porażki

[Log zwykłego RelWithDebInfo](evidence/m00-release-ctest.log) oraz [log AddressSanitizer](evidence/m00-asan-ctest.log) zachowują rzeczywiste wyjście CTest. [Snapshot wejść kompilacji i testów](evidence/m00-sources.json) zawiera SHA-256 źródeł i konfiguracji bez wymyślonego identyfikatora commitu.

Test z fałszywym oczekiwaniem zwraca niezerowy kod. Pusty zestaw CTest jest błędem. Próbka wychodząca poza bufor daje rzeczywisty raport ASan; sam niezerowy kod bez tego raportu zostałby odrzucony. Wrapper ma niezależne przypadki braku programu, timeoutu, sukcesu bez komunikatu i błędu innego rodzaju.

Pierwsza pełna próba ujawniła odwołanie do M00 na początku nagłówka briefu, co walidator słusznie odrzucił jako drugą definicję etapu. Zmieniono nagłówek na odwołanie w opisie; nie osłabiono kontroli jednoznacznej definicji ID.

Przegląd wykonał integrator w osobnym przejściu kodu i kontraktu. Nie wykonano niezależnego review przez drugą osobę lub agenta w tym etapie.

## Granice wyniku

Wykonano M00 i początkowy zakres SC-15 dotyczący pamięci. Nie ma renderera, GPU-testu, fizyki, nawigacji, autonomii ani zapisu świata. Nie ma wątków symulacji ani kontroli wyścigów. Nie wykonano benchmarku FPS, długiej próby obciążenia lub akceptacji gry przez człowieka.

Wynik potwierdza działające podstawy budowania i sprawdzania małego kodu, nie wykonalność całej wizji Genesis. Następny planowany etap to M01.
