# Weryfikacja dopracowania prowadzenia do celu

**Status: zakres techniczny ZALICZONY, 25.10.2026.** [Brief](../work-items/navigation-polish.md) określa zakres. Osobista ocena nowego odczucia przez użytkownika pozostaje otwarta; jego wcześniejsza akceptacja dotyczy bazowego M03.

## Końcowy odbiór techniczny

Przebudowano i sprawdzono wszystkie trzy konfiguracje, bez filtrów i pominięć w pełnych zestawach. [Manifest 40 plików](evidence/navigation-polish-sources.json) obejmuje źródła, konfigurację, testy, narzędzia i stary plik podróży. Zgodność SHA-256 sprawdzono po zakończeniu testów. Nie zmieniano potem kodu ani fixture.

| Polecenia | Wynik | Dowód |
| --- | --- | --- |
| `cmake --build --preset windows-debug`, `ctest --preset windows-debug` | 65/65 | [Pełny Debug](evidence/navigation-polish-debug-ctest.log) |
| `cmake --build --preset windows-release`, `ctest --preset windows-release` | 65/65 | [Pełny RelWithDebInfo](evidence/navigation-polish-release-ctest.log) |
| `cmake --preset windows-asan`, `cmake --build --preset windows-asan`, `ctest --preset windows-asan` | 67/67 | [Pełny ASan](evidence/navigation-polish-asan-ctest.log), również fixture wykrywająca celowy błąd pamięci |
| `cmake --build --preset windows-debug --target format-check` | ZALICZONE | Wszystkie zadeklarowane pliki C++ |
| `python tools/check_docs.py` | ZALICZONE | Rejestr, lokalne linki, ID i archiwum |
| `python -m unittest discover -s tools/tests -p "test_*.py"` | 29/29 | Narzędzia dokumentów i procesów, nie gra |

Po uzupełnieniu raportu powtórzono kontrolę dokumentacji. Testy nie są pomiarem FPS, opóźnienia wejścia, długiego obciążenia ani odporności na utratę zasilania. Nie dodano współbieżności i nie badano wyścigów.

## Reprodukcje przed zmianą

[Próba przed poprawką](evidence/navigation-polish-before.log): trzy przypadki NIEZALICZONE — dojście do centrum komórki w otwartym terenie, reset planu identycznym celem i cofanie do centrum po zmianie mapy. Czwarty, stabilność dojścia, już przechodził i pozostał regresją. [Powtórzenie po poprawce](evidence/navigation-polish-after.log) przechodzi 4/4 bez zmiany oczekiwań i tolerancji.

| Obserwacja na tym samym odcinku (-7; 0) → (7; 0) m | Przed | Po |
| --- | --- | --- |
| Punkty do przejścia | 16 | 1 |
| Długość planowanej łamanej | 15,4142 m | 14 m |
| Prędkość i krok | 3 m/s, 50 ms | 3 m/s, 50 ms |

Są to wyniki geometrii trasy, nie benchmark FPS, pomiar opóźnienia wejścia ani ocena inteligencji NPC.

## Kontynuacja starego zapisu

Przed zmianą zachowano lokalny program bazowy `build/navigation-baseline/reality_lab.exe`, SHA-256 `19c3039ffb540d211d5e9f7adbf89be673c80be53a42039eb0d1dd3bf839dd4d`. Ten rzeczywisty program utworzył [fixture podróży](../../tests/data/m03-v1-inflight.save) w kroku 70, z pracą w toku i trasą sprzed zmiany geometrii. [Log programu bazowego](evidence/navigation-polish-baseline-native.log) zachowuje adapter i wynik jego własnej kontynuacji.

Fixture SHA-256: `564495c65638d8a9099c8da7ea926e398b5bafd63989d560543c5b7de5486e3a`. Test sprawdza tę wartość przed użyciem; plik nie jest generowany nowym kodekiem. Nowy proces odczytuje go i kontynuuje do kroku 370, zachowuje osoby oraz ukończenie pracy i zero materiału. Nowa trasa może różnić się od bazowej po koniecznym przeplanowaniu, więc nie wymagamy identycznego hasha starego i nowego algorytmu. Dwa procesy nowej wersji nadal muszą dawać identyczną kontynuację tego samego nowego zapisu.

[Pełny zapis próby sprzętowej](evidence/navigation-polish-hardware.log) pokazuje MX330, feature level `0xb000`, `hardware=1 gpu_debug=1`, brak zgłoszonych ostrzeżeń i błędów kolejki diagnostycznej oraz normalne zamknięcie trzech procesów. Dwie kontynuacje nowego zapisu mają sumę `8654590736095958524`; odczyt starego pliku kończy się sumą `2521746865868298419`. Wszystkie dochodzą do (7; 0) m i kroku 370, z trasą 4/4 po przeplanowaniu. Zwykły tryb sprzętowy i regresje M02 również są w pełnych zestawach. Do czytelnego przechwycenia polskich komunikatów w logu verbose ustawiono `PYTHONIOENCODING=utf-8`; nie zmienia to reguł gry ani asercji.

## Rzeczywista mysz i klawiatura

Computer Use, MX330 z debug layer. T dał [jeden punkt celu](evidence/navigation-polish-ui-direct.png); N przesunął A z (-7; 0) do (-4; 0) m w 20 krokach. PPM zmienił cel [bez zmiany pozycji](evidence/navigation-polish-ui-retarget.png). Powtórzenie PPM w tym samym miejscu zachowało licznik `plans=2`. F5 zapisał podróż.

G i H zamknęły przejścia; N pokazał [„Brak drogi do celu”](evidence/navigation-polish-ui-blocked.png) bez przesunięcia A. F9 przywrócił krok 20, położenie (-4; 0), obie otwarte drogi oraz nowy cel. Spacja uruchomiła ciągły ruch; A [osiągnął cel](evidence/navigation-polish-ui-arrived.png) około (5,87; 0,96) m. Kolejne N rozliczyło 20 kroków, a [pozycja i licznik planowań pozostały bez zmiany](evidence/navigation-polish-ui-stopped.png). [Log](evidence/navigation-polish-ui.log) kończy się normalnym zamknięciem własnego okna testowego.

Jest to próba działania interfejsu i widocznego zachowania. Nie przypisujemy jej człowiekowi i nie zastępujemy nią oceny naturalności nowej wersji.

## Przegląd i granice

Osobny przegląd integratora objął wspólną geometrię, ograniczenie liczby zapytań, zachowanie ogona trasy po wyczerpaniu budżetu, zmianę celu, aktualność wyniku oraz zgodność zapisu. Nie wykonano niezależnego review przez innego autora. Surowe A* nadal ma własny test BFS. Skróty mają testy znanej ściany (w tym niezależny warunek przecięcia jej osi), narożników, szerokości, wysokości i stabilności blokady. Sprawdzono też zachowanie `BudgetExceeded` wyszukiwania niezależnie od limitu skracania.

Nie dodano rozpędzania, hamowania, animacji, interpolacji renderowania, kooperacyjnego omijania ludzi ani autonomii potrzeb. Planowanie widzi mapę narzędzia; nie dowodzi to wiedzy NPC. M04 pozostaje następnym etapem. Docelowe tempo i przyjemność ruchu nadal wymagają prób w kontekście gry.
