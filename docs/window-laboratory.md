# Okno i laboratorium świata

**Status: M02 zaimplementowany i zaliczony.** Ten dokument opisuje zachowany domyślny statyczny tryb. Nowy tryb `--motion` i zmiany współdzielonego czasu/I/O opisuje [model ruchu M03](motion-navigation.md). [Raport M02](verification/m02-window-laboratory.md) obejmuje wcześniejszy odbiór sprzętowy; aktualne wyniki są w [raporcie M03](verification/m03-motion-navigation.md). Komendy są w [środowisku pracy](development.md).

## Granice i własność

[LabSession](../src/lab/lab_session.hpp) jest rzeczywistym konsumentem [modelu M01](work-domain.md). Tworzy dwóch wykonawców i partię trzech jednostek. Przydziela nowe ID komend na podstawie największego ID odczytanego z historii; przepełnienie daje błąd. Wszystkie skutki zatwierdza `WorkWorld`. Nowy publiczny odczyt `commands()` jest widokiem tylko do odczytu, o tym samym czasie życia co dotychczasowy dziennik. Format M01 pozostaje bez zmiany.

Rezerwacja wybranego wykonawcy wymaga trzech jednostek, zużywa jedną na początku i trwa 100 kroków po 50 ms. Rozpoczęcie i przerwanie dotyczą jego ostatniej utworzonej czynności. Pełną historię udostępnia inspektor. Model nie blokuje możliwości równoczesnych czynności jednej osoby; reguły zajętości i narzędzi są późniejszym zakresem.

Odczyt najpierw buduje i waliduje kandydata M01, następnie sprawdza dokładnie dwóch żywych wykonawców i jedną aktywną partię. Inny układ jest odrzucany bez zastąpienia bieżącego świata. Wykonawcy są przyporządkowani A/B według rosnących ID. Po udanym odczycie następuje pauza, wyzerowanie akumulatora prezentacji i ponowny pomiar czasu rzeczywistego. Zapis obejmuje świat; kamera, wybór i odtwarzanie są stanem narzędzia, nie formatem świata.

## Obraz i wejście

[Geometria i kamera](../src/graphics/lab_scene.hpp) budują proceduralne trójkąty z kolorami ścian. Własna projekcja ortograficzna obsługuje obrót, nachylenie i zoom kamery. Głębia i zasłanianie są sprawdzane na GPU. Wybór myszą używa tych samych rzutowanych trójkątów i interpolowanej głębokości; tło oraz podłoga nie są obiektami domeny.

W domyślnym trybie M02 pozycje znaczników są stałym układem laboratoryjnym, bez ruchu i kolizji. Ruch istnieje w osobnym `--motion`; żaden z trybów nie ma autonomii potrzeb. Stan aktywnej pracy M02 widać jako zielony znacznik; wybór jako jasną podstawę. Materiał wolny jest zielony, zarezerwowany pomarańczowy. Widok pokazuje najwyżej trzy kostki; dokładne liczby pozostają w panelu i inspektorze, także po odczycie większej partii.

[Aplikacja Win32](../src/platform/lab_main.cpp) posiada okno, wybór, kamerę, kontrolki i mapowanie wejścia. Standardowe kontrolki Windows wywołują ten sam konsument co testy. Panel podaje adapter, krok i czas, fazę, procent upływu czasu pracy, zużycie, wynik komendy i pełny tekst inspektora M01. Procent czasu nie jest dodatkowym zużyciem ani dowodem wykonania pracy poza regułami domeny.

## Czas i zawieszenie prezentacji

`SimulationPacer` mapuje monotoniczny czas rzeczywisty na kroki zegara świata, niezależnie od liczby narysowanych klatek. Jedna aktualizacja rozlicza najwyżej 20 kroków; pozostały czas zostaje w akumulatorze. Pauza zachowuje już naliczoną zaległość, a nie nalicza nowego czasu. Ręczny przyrost działa tylko przy pauzie i wykonuje 20 kroków.

Domyślnie aplikacja jest zatrzymana. Minimalizacja oraz interaktywne zmienianie rozmiaru zawieszają dopływ czasu rzeczywistego; po powrocie pomiar zaczyna się od nowa. Nie jest to reguła docelowego życia cywilizacji poza ekranem, tylko jawna polityka laboratorium. Odtwarzanie zachowuje swój przełącznik po minimalizacji. Utrata budżetu podczas zwykłego odtwarzania nie usuwa zaległości. Błąd symulacji zatrzymuje odtwarzanie z komunikatem.

## Backend i platforma

[D3D11Renderer](../src/graphics/d3d11_renderer.hpp) posiada urządzenie, kontekst, swap chain, bufory i shadery przez COM RAII. Korzysta wyłącznie z systemowych bibliotek Windows SDK. Nie dodano silnika, biblioteki GUI, matematycznej ani renderera zewnętrznego. Shader jest mały i kompilowany z kodu aplikacji; błąd kompilacji jest błędem uruchomienia.

Adaptery są enumerowane przez [EnumAdapterByGpuPreference](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_6/nf-dxgi1_6-idxgifactory6-enumadapterbygpupreference) w kolejności wysokiej wydajności. Domyślnie wybierany jest pierwszy sprzętowy; `--adapter` wskazuje indeks z logu. Nie ma przejścia na WARP lub inny adapter po błędzie wskazanego urządzenia. Minimalny i żądany feature level to 11.0. Log zapisuje rzeczywistą nazwę, vendor/device ID, poziom oraz tryb diagnostyczny.

Swap chain używa dwóch buforów i flip discard. Zmiana rozmiaru zwalnia odwołania do starego render targetu przed `ResizeBuffers`, potem tworzy także nowy bufor głębokości. Minimalizacja nie tworzy bufora o zerowym rozmiarze. `Present` używa synchronizacji; zasłonięcie sygnalizowane przez DXGI powoduje oczekiwanie na wiadomości zamiast aktywnej pętli. Usunięcie urządzenia kończy aplikację z błędem i logiem; automatyczne odzyskanie nie jest wdrożone.

`--gpu-debug` wymaga [warstwy diagnostycznej D3D11](https://learn.microsoft.com/en-us/windows/win32/direct3d11/using-the-debug-layer-to-test-apps). Ostrzeżenie, błąd lub korupcja zgłoszone do `ID3D11InfoQueue` oznaczają porażkę. Brak składnika daje jawny błąd; aplikacja nie wyłącza flagi w ponownej próbie. Zwykły tryb sprzętowy i tryb z diagnostyką mają osobne testy, a porażka drugiego nie jest sukcesem pierwszego.

## Weryfikacja i ograniczenia

[Testy konsumenta](../tests/lab_tests.cpp) obejmują konkurencję, częściowe zużycie i anulowanie, zgodność różnych podziałów klatek, zaległość i pauzę, odczyt pracy, odrzucenie obcego świata, projekcję i wybór obiektu po zmianie kamery. [Test sprzętowy](../tests/lab_gpu_integration.py) uruchamia prawdziwy program, urządzenie i okno, wywołuje ścieżkę komunikatów kontrolek, zapisuje i odtwarza pracę, zmienia rozmiar, minimalizuje/przywraca oraz zamyka program.

Odczyt obrazu kopiuje rzeczywisty bufor GPU do staging texture i zapisuje PNG przez WIC. Jest wykonywany tylko w teście, nie w zwykłej klatce. Test sprawdza piksele inne niż tło i nagłówek PNG oraz stan zapisanego świata. Nie dowodzi to jakości sterowania. Osobne wejście myszy i klawiatury jest sprawdzane przez Computer Use. [Ocena użytkownika dla sterowania M03](verification/m03-motion-navigation.md#ocena-użytkownika) jest odrębna od nadal niewykonanego odbioru pełnej gry.

Nie ma jeszcze próby wielu monitorów i zmiany DPI między nimi, odzyskiwania urządzenia, długotrwałego obciążenia ani benchmarku FPS. Kod taktowania nie jest schedulerem cywilizacji. Limity M01 nadal obowiązują i nie rozwiązują retencji historii.
