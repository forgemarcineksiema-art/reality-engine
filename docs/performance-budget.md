# Sprzęt, budżety i pomiary

**Właściciel tematu:** parametry prób i metoda oceny kosztu. Wszystkie cele poniżej są PROPOZYCJĄ do pomiaru. Wykonano [pierwszy pomiar małej sceny przestrzennej](verification/m04-entry-observability.md#pomiar-dłuższego-działania), bez dowodu skalowania całego silnika lub docelowego świata.

## Potwierdzony punkt odniesienia

Odczyt lokalny z 25.10.2026: Intel Core i5-1035G1, 4 rdzenie/8 wątków; około 7,78 GiB pamięci raportowanej przez system; Intel UHD Graphics oraz NVIDIA GeForce MX330. Użytkownik określił laptop jako i5/MX330/8 GB RAM.

W M02 rzeczywiście utworzono urządzenie D3D11 na MX330 z żądanym feature level 11.0. Nie zmierzono budżetu VRAM ani maksymalnego poziomu funkcji sprzętu. Chwilowo wolna pamięć nie jest stałą własnością sprzętu. Przy dwóch GPU aplikacja raportuje faktycznie wybrany adapter; samo utworzenie urządzenia nie dowodzi celu FPS.

## Robocze parametry początkowej próby

| Parametr | Propozycja | Sposób oceny |
| --- | --- | --- |
| Początkowa społeczność | Około 12–16 trwałych osób | Scenariusze domenowe i koszt wspólnej sceny |
| Późniejsza próba skali | Około 60 osób po przejściu odbioru | Porównanie tego samego rodzaju obciążenia |
| Obraz | 1280×720, proste bryły i oświetlenie | Czytelność oraz CPU/GPU i przycięcia |
| Płynność | Hipoteza 60 FPS, około 16,67 ms na klatkę | Rozkład czasów i sterowanie w długiej próbie |
| Pamięć procesu | Miękki budżet 512 MiB | Szczyt i trend wzrostu; osobno zasoby GPU |
| Praca AI i tras | Ograniczone kolejki i liczba rozwinięć | Najstarsze oczekujące zadanie i koszt aktualizacji |
| Kompilacja | Początkowo mała równoległość | Pamięć i responsywność laptopa |

Nieosiągnięcie celu jest wynikiem do analizy, nie uprawnieniem do ukrycia próbek lub usunięcia skutków. Populacje większe od kolejnej próby i wielkie regiony należą do badań, nie do obietnicy aktualnego prototypu.

## Metoda porównania

Przed uruchomieniem zapisz wersję, build, scenariusz, seed, ustawienia, adapter, liczbę mieszkańców i czas próby. W repozytorium bez Git użyj identyfikatora snapshotu albo hashy właściwych źródeł; nie wymyślaj SHA commitu.

Oddziel czas przygotowania od pracy ustalonej. Porównuj podobne stany świata i nagrzania. Wyłącz konkurencyjne ciężkie zadania; nie zmieniaj sterowników, ustawień i obciążenia między próbami bez zapisu. Jeśli warunki się różnią, wynik nie uzasadnia prostego wniosku przyczynowego.

Mierz co najmniej czasy klatek i ich percentyle, najgorsze przycięcia, pamięć, zaległości aktualizacji, czas planowania, wyszukiwania tras i zapisu. Średni FPS nie wystarcza. Rejestrowanie nie może samo istotnie blokować ścieżki mierzonej bez ujawnienia tego kosztu.

CPU i GPU pracują częściowo równolegle. Budżet klatki oceniamy przez ścieżkę krytyczną, nie sumę wszystkich liczników. Stały krok fizyki i przyspieszanie czasu muszą raportować zaległość oraz sposób nadrabiania.

## Rodzaje scen

Spokojna społeczność, nagła niedostępność trasy, skupisko wielu wykonawców, częste przełączenie reprezentacji i zapis w trakcie aktywności. Późniejsza fizyczna destrukcja otrzyma odrębny scenariusz.

Runner bez obrazu mierzy koszt rdzenia. Nie dowodzi płynności aplikacji 3D. Wynik innego adaptera lub renderera programowego nie jest benchmarkiem MX330. Szczegółowy odbiór określa SC-12.
