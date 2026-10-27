# Reality Engine — własny silnik Genesis

Decyzja architektoniczna, wersja 0.2, 25 października 2026 r.

## 1. Kierunek projektu

Genesis powstaje na własnym silniku. Projekt nie używa Unreal Engine jako platformy wykonania, renderera, edytora ani źródła wymaganych modułów. Wcześniejsze odwołania do rozwiązań Epic w przeglądzie badań są wyłącznie materiałem porównawczym.

Własnością projektu są architektura, format danych wykonawczych, zasady symulacji, renderer, zarządzanie światem, fizyka, nawigacja, narzędzia i proces produkcji treści. Użytkownik wybrał własną implementację również fizyki i nawigacji oraz minimum bibliotek. Jolt i Recast/Detour przestają być kandydatami wykonawczymi. Zależności dodajemy pojedynczo, z uzasadnieniem i przypiętą wersją.

Pierwszy rezultat to grywalny fragment i laboratorium małej autonomicznej cywilizacji. Grafika ma najniższy priorytet. Pracuje użytkownik ze swoimi agentami AI, bez narzuconego terminu.

Ta decyzja zwiększa kontrolę nad kosztami i reprezentacją świata, ale nie gwarantuje automatycznie większej wydajności. Wymaga kompetencji i prac, które wcześniej miały pochodzić z gotowego silnika. Nie wykonano jeszcze implementacji ani benchmarków.

## 2. Początkowy stos techniczny

Rekomendacja robocza po doprecyzowaniu celu: C++20, CMake, Windows i Direct3D 11 jako pierwszy backend. Win32 obsługuje okno i wejście. Biblioteka standardowa C++ i systemowe API stanowią fundament; na start nie wymagamy SDL, gotowego ECS, fizyki ani nawigacji. D3D12 odkładamy: przy prostej grafice i pracy solo większą wartość ma szybsze połączenie obrazu z symulacją. To wybór zakresu prac, nie twierdzenie o przewadze wydajności D3D11.

Bieżący odczyt sprzętu: i5-1035G1, 4 rdzenie/8 wątków, około 7,78 GiB RAM, Intel UHD i MX330. Potwierdzono Visual C++ Build Tools, Windows SDK, CMake i Ninja. Uruchomienie urządzenia graficznego oraz faktyczny budżet VRAM wymagają osobnego testu. Nie traktujemy chwilowo wolnej pamięci jako stałej własności sprzętu.

| Element | Proponowany fundament | Co piszemy sami |
| --- | --- | --- |
| Okno i wejście | Win32 | Cykl aplikacji, mapowanie akcji i sterowanie |
| Dostęp do GPU | Direct3D 11 | Renderer, zasoby GPU, shadery i proste oświetlenie |
| Fizyka lokalna | Własny kod | Detekcja kolizji, ruch, kontakty, później bryły i więzy |
| Nawigacja | Własny kod | Siatka przejścia, A*, graf regionu i aktualizacje |
| Interfejs narzędzi | Na start tekst i proste własne kontrolki | Inspektory, sterowanie próbą i edycja danych |
| Profilowanie | Własne liczniki i systemowe zegary | Pochodzenie zdarzeń, czasy domen i limity pracy |
| Treści 3D | Początkowo geometria proceduralna | Wspólne definicje wyglądu, kolizji i przeszkód; import później |
| Symulacja cywilizacji | Własny kod | EARTH, EVOLUTION, CHRONICLE, zapis i reguły czynności |

Minimalizacja bibliotek nie oznacza pisania kompilatora C++, sterownika GPU ani systemu operacyjnego. Korzystamy z narzędzi budowania i systemowych API. Dokładny zakres dopuszczalnej przyszłej zależności pozostaje decyzją dla konkretnej potrzeby.

## 3. Moduły i granice

```text
Reality Engine
  Foundation        identyfikatory, pamięć, pliki, zegary, zadania, logi
  Platform          okno, wejście, obsługa systemu i wyjście audio
  Assets            import, kompilacja, cache, zależności i pakiety
  Graphics          backend GPU, renderer, materiały, światła i widoki
  World             regiony, współrzędne, streaming i reprezentacje
  Physics           własne kolizje, kontakty, ciała i solver
  Navigation        własne wyszukiwanie tras i aktualizacja przejść
  Animation         szkielety, klipy, przejścia, skinning i podstawowe IK
  Interaction       czynności, stanowiska, narzędzia i przerwania
  Simulation        EARTH, EVOLUTION i CHRONICLE
  Forge             konstrukcje, uszkodzenia, rumowiska i odbudowa
  Persistence       migawki, dzienniki, migracje i archiwizacja
  Tools             edytor, diagnostyka i walidatory
  Game              reguły Genesis, kamera, walka i interfejs gracza
```

Osobne aplikacje korzystają ze wspólnych bibliotek: gra, edytor, runner laboratoryjny i kompilator zasobów. Nie tworzymy osobnego silnika dla laboratorium. Każda aplikacja ładuje tylko potrzebne moduły.

Rdzeń symulacji nie dołącza nagłówków GPU i nie wymaga okna. Renderer otrzymuje spójny widok danych. Nie steruje biografią postaci. Przekazanie widoku ogranicza konflikty wątków, ale wymaga jawnego opóźnienia i procedury synchronizacji ważnych zmian kolizji.

## 4. Fundamenty wykonania

Potrzebujemy zarządzania cyklem aplikacji, pamięcią CPU/GPU, plikami, błędami, konfiguracją, wątkami, wejściem i czasem. Pierwsza wersja stosuje prosty kontrolowany scheduler; złożony graf zadań powstaje po pomiarach.

Identyfikator osoby nie jest indeksem mesha, obiektu fizycznego ani elementu edytora. Każda reprezentacja odwołuje się do stabilnej tożsamości. Interfejsy zewnętrzne są zamknięte w modułach, dzięki czemu typy API systemu i GPU nie przenikają do formatu zapisu świata.

Zadania równoległe mogą przygotowywać propozycje i zasoby, ale zmiany konfliktujących danych mają określoną kolejność zatwierdzania. Hot reload danych wymaga ponownej walidacji; nie zakładamy bezpiecznej podmiany dowolnego kodu w każdym momencie.

## 5. Własny renderer

Pierwszy renderer obejmuje urządzenie D3D11, swap chain, obsługę zmiany rozmiaru, bufor głębi, proste shadery i instancjonowaną geometrię. Logujemy wybrany adapter oraz obsługiwany poziom funkcji; przy dwóch GPU nie zakładamy, że domyślny adapter to MX330. Zarządzamy czasem życia zasobów i aktualizacjami, mimo że API wykonuje część synchronizacji automatycznie.

Pierwszy obraz świata pokazuje kamerę, geometrię, głębię, jednolite kolory i proste oświetlenie. Czytelne sylwetki i wyróżnienie interakcji mają pierwszeństwo. Tekstury, PBR i cienie dodajemy dopiero po sprawdzeniu ich wartości oraz kosztu.

Kolejne kroki zależą od pomiarów: LOD, oświetlenie wielu źródeł, animowane postacie, teren, roślinność, przezroczystość i efekty atmosferyczne. Zaawansowane GI, ray tracing, wirtualizowana geometria i rozbudowane rekonstrukcje obrazu są późniejszymi opcjami.

Na początku wystarczą jawne, uporządkowane przebiegi renderowania. Rozbudowany graf renderowania ma sens po ustaleniu potrzeb; nawet prosty renderer od razu wymaga poprawnych zależności i barier zasobów.

Pierwszy backend jest wyspecjalizowany w D3D11. Renderer ma mały interfejs, aby późniejsza wymiana backendu nie wymagała zmiany gospodarki i AI. Port do D3D12 lub innego API byłby osobnym, mierzalnym zakresem prac.

## 6. Własny proces przygotowania treści

W najwcześniejszym grywalnym fragmencie korzystamy z proceduralnych brył oraz prostych danych scenariusza. Pełny importer i biblioteka animacji nie są warunkiem rozpoczęcia badań autonomii. Poniższy proces opisuje późniejszy kierunek rozwoju treści.

Plik źródłowy z narzędzia artysty przechodzi import, sprawdzenie jednostek i osi, walidację materiałów, przygotowanie geometrii, kolizji, animacji i metadanych. Wynik trafia do wersjonowanego formatu wykonawczego.

Każdy wynik ma identyfikator, wersję importera i listę zależności. Zmiana materiału albo definicji podpory unieważnia odpowiednie dane pochodne. Cache nie może ukrywać nieaktualnych konstrukcji.

W pierwszej wersji ograniczamy obsługiwany podzbiór glTF/GLB i komunikujemy nieobsługiwane funkcje. Nie obiecujemy importu dowolnego pliku 3D. Eksportowany asset wymaga dodatkowych danych Genesis: funkcji obiektu, punktów interakcji, podpór i instalacji.

Kompilator zasobów działa bez edytora. Te same dane można przygotować lokalnie i w automatycznym buildzie. Błędy treści powinny być wykrywane przed wejściem gracza do danej komórki świata.

## 7. World: streaming i reprezentacja

Mapa jest podzielona na regiony i komórki treści. Osobno zarządzamy wczytaniem geometrii, kolizji, animacji i danych domenowych. Usunięcie mesha z RAM nie usuwa osoby lub magazynu z symulacji.

Streaming potrzebuje priorytetów, limitów pracy, anulowania nieaktualnych żądań i kontroli pamięci. Ruch pojazdu wymaga przewidywania obszaru, do którego gracz dojedzie. Aktualizacje uszkodzonych konstrukcji unieważniają również ich odległe reprezentacje.

Duży świat wymaga świadomego rozwiązania precyzji współrzędnych. Możliwy model to pozycja regionu oraz lokalne współrzędne; przesunięcie początku układu musi objąć fizykę, kamerę i renderowanie w uzgodnionej granicy. Wybór testujemy przy docelowych odległościach.

## 8. Animacja, fizyka i destrukcja

Własna animacja zaczyna się od klipów szkieletowych, prostych przejść, skinningu i sygnałów wykonania czynności. Trzeba ustalić, kiedy ruch wynika ze sterowania, animacji albo fizyki. Zapobiega to kilku konkurującym właścicielom położenia.

Własna fizyka mapuje ciała na trwałe obiekty świata. Kontakty i przerwania są przetwarzane w kontrolowanej fazie, a zapis obejmuje uzgodniony stan potrzebny do kontynuacji. Nie zakładamy identycznej fizyki na różnych urządzeniach.

Pierwsza faza obejmuje stały krok czasu, grawitację, proste kształty, przeszkody nieruchome i kontroler postaci. Rozdzielamy wyszukiwanie możliwych par kolizyjnych od dokładnego badania kontaktu. Początkowy indeks przestrzenny może być siatką, a geometria ograniczona do kształtów osiowych i kul lub kapsuł. Obsługa brył obracających się, tarcia, stosów i więzów jest następnym etapem własnego solvera.

Testy obejmują spoczynek na podłożu, start wewnątrz przeszkody, przejście narożnika, kontakt kilku ścian i szybki ruch względem cienkiej przeszkody. Samo wypchnięcie po wykryciu przecięcia nie rozwiązuje tunelowania; krytyczne ruchy wymagają testu ciągłego albo jawnie kontrolowanego podziału kroku. Nie nazywamy uproszczonego kontrolera gotowym silnikiem dowolnych brył sztywnych.

FORGE utrzymuje własny graf połączeń konstrukcyjnych. Pierwszy most składa się z przygotowanych elementów. Zerwanie połączenia powoduje lokalny rozpad fizyczny i zmianę przejezdności. To praktyczny początek trwałej destrukcji.

Dowolne cięcie geometrii i pełne naprężenia materiałów są osobnymi rozszerzeniami badawczymi. Przed uruchomieniem rozpadu brył musimy mieć odpowiednią obsługę dynamiki. Wcześniejszy test zamknięcia przejścia może zmieniać stan przygotowanej konstrukcji, ale nie dowodzi wtedy fizycznego rozpadu. Po destrukcji aktualizujemy również nawigację, sieci instalacji oraz widok świata.

### 8.1. Własna nawigacja

Początek to ograniczona mapa 2,5D z komórkami przejścia, kosztem terenu i jawnymi połączeniami warstw wysokości. Most jest połączeniem, które może utracić dostępność. Wyszukiwanie A* ma limit pracy i wersję grafu, na którym obliczono wynik. Regionalny graf miejsc pozwala później ograniczać obszar dokładnego wyszukiwania.

Przeszkody i wolne przejścia wynikają ze wspólnych definicji kolizji. Uwzględniamy rozmiar postaci i zakaz przecinania narożników. Śledzenie trasy wykonuje kontroler ruchu z testem kolizji; sama znaleziona ścieżka nie gwarantuje przejazdu. Zmiana geometrii unieważnia zależne fragmenty tras i powoduje ograniczone przeplanowanie.

Testy obejmują brak drogi, drogę zastępczą, przejście zbyt wąskie, zamknięcie mostu w czasie podróży oraz zapis podczas oczekiwania na wynik. Własny navmesh i pełne swobodne poruszanie po wielopoziomowej geometrii pozostają późniejszym rozszerzeniem.

## 9. Własny edytor

Edytor początkowo pozwala otworzyć scenariusz, umieścić obiekt, zmienić właściwości, obejrzeć identyfikatory, uruchomić próbę oraz zapisać zmiany. Potrzebuje cofania, rozróżnienia edycji od symulacji i obsługi błędnych danych.

Następnie otrzymuje edycję połączeń konstrukcyjnych, tras, stanowisk, organizacji i warunków scen. Inspektor pokazuje wiedzę mieszkańca, plan, rezerwacje i łańcuch przyczyn. To narzędzie rozwoju technologii od pierwszych etapów.

Na start wystarczą własne proste kontrolki, tekst diagnostyczny oraz dane scenariusza edytowane w pliku. Nie wymagamy Dear ImGui ani rozbudowanego edytora 3D do uruchomienia pierwszego wycinka. Model dokumentów, undo/redo i bardziej rozbudowane narzędzia dodajemy w miarę potrzeb. Interfejs gracza ma osobne wymagania czytelności i dostępności.

## 10. Kolejność budowy

| Etap | Rezultat | Sprawdzenie |
| --- | --- | --- |
| E0 | Build C++ i runner bez obrazu | Powtarzalny scenariusz i jawne błędy |
| E1 | Win32, minimalne D3D11, prosta geometria i pomiary | Poprawny resize, wybór adaptera i zamykanie aplikacji |
| E2 | Własne podstawowe kolizje, ruch oraz nawigacja | Brak przechodzenia przez ścianę; działająca droga zastępcza |
| E3 | Dane świata, czynności, zapis, inspektor i 12–16 mieszkańców | Grywalny wycinek i identyczne reguły w runnerze; autonomia i trwałość |
| E4 | Komórki świata, podróż i ciągłość reprezentacji | Odejście i powrót nie dublują obiektów ani zasobów |
| E5 | Transport, most, naprawa i podstawowi agenci | Destrukcja zmienia plan i rzeczywistą dostawę |
| E6 | Po pomiarach około 60 mieszkańców oraz jedna historia | Wyjaśnialne reakcje, długie próby i grywalność |

Przy pracy użytkownika z agentami AI przyrosty pozostają małe. Każde zadanie określa kontrakt, pliki objęte zmianą i wykonywalny test. Łączymy pracę dopiero po kompilacji, sprawdzeniu scenariusza i pomiarze kosztu. Sam trójkąt na ekranie nie dowodzi Genesis, ale jest potrzebnym sprawdzeniem fundamentów GPU.

Wcześniejsza orientacyjna mapa 12 tygodni nie obowiązuje. Użytkownik nie narzuca terminu. Zakres kolejnej pracy wynika z rezultatu poprzedniej; nie zwiększamy jednocześnie populacji, szczegółowości fizyki i jakości obrazu.

## 11. Polityka zakresu

Pierwszy cel obejmuje jeden backend graficzny, proceduralne bryły, jeden rodzaj konstrukcji i ograniczony katalog czynności. Mała społeczność korzysta z potrzeb, zasobów, pracy i planów. Prosty wygląd nie zwalnia z czytelnej informacji o skutkach. Każde rozszerzenie ma reprezentatywną scenę, budżet oraz kryterium ukończenia.

Oryginalność technologii ma wynikać ze wspólnego modelu świata i działania Genesis. Zależności służą wąskim funkcjom. Dokładny wykaz wersji, konfiguracji budowania i licencji utrwalamy razem z kodem.

### 11.1. Budżety pierwszego prototypu

Propozycje do sprawdzenia, nie wyniki: około 12–16 mieszkańców na początku, prosta scena 3D, robocza rozdzielczość 1280×720 i miękki budżet pamięci procesu 512 MiB. 60 FPS jest hipotezą celu do pomiaru. Około 60 mieszkańców jest następną próbą skali, a nie deklaracją wydajności.

Fizyka korzysta ze stałego kroku, a planowanie i gospodarka z rzadszych aktualizacji oraz zdarzeń. Liczba kroków nadrabiania i koszt wyszukiwania tras są ograniczone; zaległości raportujemy zamiast ukrywać. Nie rezerwujemy wszystkich wątków CPU, a kompilacje początkowo ograniczamy do małej równoległości. Konkretne limity ustalamy z pomiarów.

Pierwsza wersja nie potrzebuje lokalnego modelu językowego. Agenci deweloperscy pomagający pisać kod są odrębni od mieszkańców symulacji. AI mieszkańca to początkowo własne procedury, priorytety, wiedza i planowanie. Opcjonalne modele można później podłączyć przez kontrolowaną granicę.

Próba wydajności musi obejmować dłuższe działanie z renderowaniem i powtarzalny scenariusz. Weryfikujemy pamięć, przycięcia, koszty aktualizacji i sterowanie. Sam odczyt sprzętu nie jest benchmarkiem.

## 12. Źródła pierwotne sprawdzone dla tej propozycji

- [Microsoft: tworzenie urządzenia Direct3D 11 i wybór adaptera](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-d3d11createdevice).
- [Microsoft: jawna synchronizacja i zarządzanie zasobami](https://learn.microsoft.com/en-us/windows/win32/direct3d12/important-changes-from-directx-11-to-directx-12).
- [Khronos: glTF](https://www.khronos.org/gltf/).
