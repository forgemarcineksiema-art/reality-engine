# Audyt praktycznych przypadków ruchu i obrazu

Status: zrealizowany technicznie, na zlecenie użytkownika po próbie dopracowania nawigacji. [Raport](../verification/motion-adversarial-audit.md) zachowuje odtworzone błędy, poprawki, pełne testy i ograniczenia; nowa ocena odczuć użytkownika pozostaje niewykonana. Cel: odtworzyć utknięcie po szybkiej zmianie celu, sprawdzić granice kolizji i usunąć skokowe wyświetlanie ruchu przed M04. Stosujemy przegląd całej wskazanej implementacji, nie fikcyjny diff repozytorium bez commitów.

## Hipotezy do reprodukcji

Poprawna pozycja przy ścianie może leżeć w komórce o zablokowanym centrum. Po nieudanym bezpośrednim odcinku dotychczasowy planer odrzuca wtedy cały cel. Podobny problem dotyczy celu blisko ściany. Druga postać nie należy do mapy A*, więc samo oczekiwanie może trwać bez końca mimo wolnego miejsca obok. Pozycje rysowane prosto ze stałego kroku 50 ms mogą wyglądać skokowo, niezależnie od płynności GPU. Skrajne współrzędne wymagają sprawdzenia różnicy dwóch poprawnych punktów, a nie walidacji wektora jak położenia.

## Zakres

- Zachować referencję BFS i jedno źródło geometrii. Dodać sprawdzane sweepem lokalne przyłącza końców do sąsiednich centrów, jeśli ich własne centrum jest niedostępne. Rozdzielić błędny fizycznie punkt od braku połączenia w przybliżonym grafie.
- Uzupełnić ograniczoną próbę obejścia blokującej postaci, z deterministycznym odstępem ponowień opartym o zegar świata. Cel faktycznie zajęty przez inną osobę nadal wymaga oczekiwania; nie przenosimy wykonawcy ani nie rozwiązujemy pełnej kooperacji tłumu.
- Rejestrować rzeczywisty przebieg ostatniego kroku (także ślizg i przejście przez zakręt) oraz interpolować wyłącznie obraz. Prosta cięciwa pomiędzy końcami nie może wizualnie przecinać przeszkody. Renderowane pozycje dwóch osób również mają zachować rozłączność. Pauza, odczyt, przełączenie czasu i zmiana geometrii synchronizują historię obrazu.
- Zachować 3 m/s i stały krok. Rozpędzanie, nowa geometria i inne platformy nie są warunkiem naprawy odtworzonych błędów; ich rozszerzenie wymaga konkretnego dowodu potrzeby. Nie zgłaszamy akceptacji odczuć użytkownika za niego.

Pliki: fizyka, nawigacja, konsument ruchu, widok sceny i Win32, testy, CMake i dokumenty opisujące aktualny kontrakt. Kontrakty INV-02, INV-09, INV-10, INV-11, INV-12, INV-16; scenariusze SC-06, SC-07, SC-08, SC-11. Stan do kontynuacji pozostaje w zapisie; historia ostatniej klatki jest pochodnym stanem prezentacji i po odczycie startuje z bieżących pozycji.

## Wymagany dowód

Uzupełnienie po odtworzeniu błędu: dokładny końcowy kontakt ma zachować płaszczyznę styku, żeby błąd zaokrąglenia nie powodował `InvalidStart` następnego ruchu. Minimum siatki 1 mm i tolerancja pokrycia uwzględniają precyzję przesuniętych współrzędnych; obliczony rozmiar siatki musi pozostać skończony. Dawny zapis błędnego `InvalidEndpoint` dla legalnego celu wymaga próby ponownego planowania. Te przypadki mają oddzielne regresje, bez osłabiania tolerancji.

Zachować porażki przed naprawą. Dodać zmiany celu przy ścianie, cel przy krawędzi, serię szybkich celów, zmianę przejścia podczas marszu, blokadę przez osobę i mijanie, próbki obrazu na zakręcie i przy ślizgu oraz deterministyczne próby losowe z niezależną oceną penetracji. Zapis/odczyt i podział czasu mają zachować wynik. Wykonać pełne Debug, RelWithDebInfo i ASan, format i dokumentację, rzeczywisty GPU z debug layer oraz mysz i klawiaturę. Raport odróżni naprawione błędy, nadal otwarte ograniczenia i przypadki niewykonane; nie obiecamy pokrycia wszystkich możliwych scenariuszy.
