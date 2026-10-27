# Raport tożsamości, czynności i zasobów — etap M01

Data: 25.10.2026. Status: **ZALICZONE w zakresie M01.** Zakres określał [brief](../work-items/m01-identity-actions-reservations.md); konkretne API i ograniczenia opisuje [model domeny](../work-domain.md). Lokalny Git nadal nie ma commitu ani remote.

## Wykonany przyrost

- Trwałe ID, osobne uchwyty z generacją i bezpieczne ponowne użycie slotu.
- Jeden właściciel wykonawców, partii i czynności; konkurencja o rezerwację, częściowe zużycie, przerwanie i terminy.
- Utrwalony rejestr wyników komend, również odmów; identyczne ponowienie bez powtarzania skutku.
- Jawne usunięcie uczestnika z przerwaniem zależnych czynności i rozliczeniem materiału.
- Wersjonowany snapshot z walidacją, checksum, limitami, zapisem przez plik tymczasowy i błędem zamiast cichego resetu.
- Demo i inspektor tekstowy, rzeczywiste wznowienie w osobnym procesie oraz błędy zapisu i wczytania.

Nie dodano nowych bibliotek runtime. Zegar M00 i jego testy zachowano.

## Wykonane sprawdzenia

| Komenda | Rzeczywisty wynik | Status |
| --- | --- | --- |
| `cmake --preset windows-msvc` | Konfiguracja MSVC x64 | ZALICZONE |
| `cmake --build --preset windows-debug` | Build C++20 z `/W4 /WX` | ZALICZONE |
| `ctest --preset windows-debug` | 42/42 kontrole | ZALICZONE |
| `cmake --build --preset windows-release` | Zwykły RelWithDebInfo | ZALICZONE |
| `ctest --preset windows-release` | 42/42 kontrole | ZALICZONE |
| `cmake --preset windows-asan` | Osobna konfiguracja diagnostyczna | ZALICZONE |
| `cmake --build --preset windows-asan` | Build instrumentowany | ZALICZONE |
| `ctest --preset windows-asan` | 44/44 kontrole, w tym poprawna próbka i rzeczywiste wykrycie heap-buffer-overflow | ZALICZONE |
| `cmake --build --preset windows-debug --target format-check` | Bez błędów clang-format | ZALICZONE |
| `python tools/check_docs.py` | Rejestr, linki, ID i archiwum | ZALICZONE |
| `python -m unittest discover -s tools/tests -p "test_*.py" -v` | 27 regresji narzędzi | ZALICZONE |
| `./build/msvc/Debug/reality_work_demo.exe --create ./build/m01-world.save` | Krok 2, 2 jednostki fizyczne i zarezerwowane, 1 zużyta, termin 5, druga komenda odrzucona | ZALICZONE |
| `./build/msvc/Debug/reality_work_demo.exe --inspect ./build/m01-world.save` | Ten sam stan po odczycie z pliku | ZALICZONE |
| `./build/msvc/Debug/reality_work_demo.exe --resume ./build/m01-world.save` | Krok 5, czynność ukończona, 3 zużyte, brak pozostałego materiału i rezerwacji | ZALICZONE |

Nowy zakres to 21 przypadków C++, integracja odrębnych procesów oraz dwie próby odmowy błędnego działania nowego runnera. Pozostałe 18 kontroli pochodzi z M00. Liczby CTest obejmują dokumentację i jedną pozycję uruchamiającą 27 testów narzędzi; nie oznaczają 42 niezależnych funkcji silnika. ASan dodaje dwie próbki diagnostyczne. Nie sumujemy konfiguracji jako nowych zachowań.

Testy wymagają konkretnych bilansów, faz i wyników. Porównują skok czasu z pojedynczymi krokami, zachowują seed `0x4d3031` dla serii 100 operacji i sprawdzają niezmienniki przy kolejnych zatwierdzeniach. Uszkodzone fixture'y z ponownie policzonym checksum badają także walidację semantyczną, zamiast zatrzymywać się na samej korupcji bajtów. Próba z uchwytem Windows wywołuje rzeczywistą odmowę podmiany pliku; sprawdza zachowanie poprzedniego zapisu i usunięcie własnego pliku tymczasowego. Nie wstrzykiwano awarii alokatora, braku miejsca na dysku ani błędu samego flush.

## Zachowane dowody

[Debug](evidence/m01-debug-ctest.log), [RelWithDebInfo](evidence/m01-release-ctest.log) i [AddressSanitizer](evidence/m01-asan-ctest.log) zawierają rzeczywiste wyjście CTest. [Stan przed wznowieniem](evidence/m01-demo-create.log) i [po wznowieniu](evidence/m01-demo-resume.log) zachowują inspektor. [Snapshot źródeł](evidence/m01-sources.json) zawiera SHA-256 wejść kompilacji i testów, bez wymyślonego identyfikatora commitu. Porównanie hashy potwierdziło, że kod zegara, jego CLI i testy oraz dotychczasowe narzędzia pozostały bajtowo zgodne ze snapshotem M00.

## Przegląd i granice

Przegląd integratora w osobnym przejściu objął własność stanu, żywotność iteratorów podczas alokacji, kolejność zatwierdzania, przepełnienia, dekodowanie i granicę podmiany pliku. Nie jest to niezależne review drugiego agenta. Dodano kontrolę historycznych typów rejestru, terminu mieszczącego się w zegarze i zgodności cyklu życia z dziennikiem. Strumień serializacji zgłasza błąd zamiast zwracać niepełny bufor. Walidacja nie jest pełnym odtworzeniem historii planowania.

Pierwszy build testów odrzucił ignorowane wyniki `nodiscard` i kolizję nazwy testu z limitem danych. Poprawiono konsumentów i asercje, zachowując `/W4 /WX`. Nie wyłączono ostrzeżeń ani testów.

Zakres scenariuszy i brakujące części są jawne w [strategii testów](../testing-strategy.md#obecny-zakres-domenowy). Nie ma autonomii, GPU, fizyki, nawigacji, produkcji, transportu ani biologicznej śmierci. Nie wykonano benchmarku, próby długotrwałej ani akceptacji gry przez człowieka. Nie sprawdzono awarii sprzętu, odcięcia zasilania ani nagłego zakończenia procesu w środku zapisu. Limit kolekcji jest zabezpieczeniem prototypu, nie polityką retencji wieloletniej cywilizacji.
