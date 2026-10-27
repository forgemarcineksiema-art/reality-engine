# Rozszerzenie ruchu, geometrii i autonomicznego planowania

Status: w realizacji na zlecenie użytkownika. Użytkownik zaakceptował płynność poprzedniej wersji i wyjaśnił, że platformy oznaczają podesty, kładki, poziomy i ruchome konstrukcje, nie porty na inne systemy operacyjne.

## Cel i kolejność

Rozszerzamy fundament M03 i wdrażamy rzeczywistego konsumenta autonomicznego wyboru zadań. Plan M00–M06 pozostaje; odbiór małej sceny autonomii nie oznacza zaliczenia całej społeczności M04. Kolejność zależności:

1. Wspólna geometria przestrzenna: wypukłe graniastosłupy z obróconym obrysem i nachylonym wierzchem, powierzchnie podparcia, kapsuła postaci, ciągłe sprawdzanie odcinków. Ten sam opis dostarcza kolizję, powierzchnie tras i trójkąty obrazu.
2. Planowanie na powierzchniach i prześwitach, z kosztami długości w metrach, bez zależności od centrów dawnej siatki. Jawne połączenia między poziomami oraz czynność przejazdu platformą. Brak gwarancji pełności dla dowolnej geometrii ciągłej.
3. Rozpędzanie i hamowanie z ograniczeniami przyspieszenia oraz jego zmian; orientacja z ograniczeniami prędkości i przyspieszenia obrotu. Limit nachylenia, rozmiar i wysokość są wymaganiami wykonawcy, nie wyłącznie obrazem.
4. Ruchome i obracające się podpory, lokalne punkty oparcia, rzeczywiste przenoszenie pasażera, wykrywanie blokady. Konstrukcja kinematyczna może zatrzymać się przed zgnieceniem; nie udajemy solvera sił i destrukcji.
5. Wielu wykonawców: przewidywanie kolizji, kontrola postępu, rozstrzyganie pierwszeństwa i kolejki przy zasobie/przejściu. Dokładny cel gracza i jawny cel podejścia w zasięg są różnymi komendami.
6. Autonomia: potrzeba → wybór znanego celu → dojście → rezerwacja/czynność M01 → rzeczywisty skutek. Nie wystarcza chodzenie między losowymi punktami ani napis „praca”. Wiedza, odmowy, terminy ponowień i postęp są stanem świata.
7. Wersjonowana kontynuacja, interaktywna scena w istniejącym rendererze, inspektor przyczyn, pełne odbiory.

## Granice kodu

[Praktyczny audyt wielu postaci](../verification/traversal-multi-actor.md) obejmuje kolejne reprodukcje i naprawy oraz rzeczywiste wejście do okna. Jego odbiór techniczny nie jest ludzką akceptacją odczucia ani odbiorem M04. Pozostają kontrolowane pomiary dłuższego działania i osobna ocena użytkownika.

Wykonane ograniczone przyrosty i ich dowody: [polecenia oraz własność przejść](../verification/traversal-command-hardening.md), [wybór potrzeby po niedostępności celu](../verification/autonomy-need-fallback.md) i [wielorazowa pojemność](../verification/reusable-capacity.md). Aktualne reguły autonomii małej sceny opisuje [model](../traversal-autonomy.md). Nie sumujemy ich automatycznie do odbioru całego zlecenia. Pozostają między innymi szersza próba interaktywna nowego sterowania, dłuższe wieloosobowe przebiegi i pomiary na docelowym laptopie. Pojemność jest już oddzielona od zużywanego materiału, ale jedna czynność nadal nie łączy kilku zasobów.

Moduły: geometria/fizyka, nawigacja powierzchniowa, lokomocja, sesja zaawansowanego toru, konsument autonomii, scena/okno, kodek i testy. Nie dodajemy biblioteki fizyki, nawigacji ani AI. Stary tor M03 i historyczny zapis pozostają regresją; nowy opis geometrii i dodatkowy stan otrzymają własny format, bez reinterpretacji starego pliku. Nie tworzymy drugiego zegara ani konkurencyjnych zasobów w rendererze.

Chronione kontrakty: INV-01, INV-02, INV-03, INV-04, INV-08, INV-09, INV-10, INV-11, INV-12 i INV-16. Parametry lokomocji są roboczym strojeniem do sprawdzenia, nie zaakceptowanym odczuciem nowej wersji.

## Dowód

Osobne sprawdzenia: ograniczenia rozpędzania/hamowania/obrotu; retarget podczas hamowania; zatrzymanie i utrata wejścia; stok dopuszczalny i za stromy; podpora nad inną powierzchnią i prześwit pod nią; obrócony narożnik; cienka ściana przy dużej prędkości; kładka bez centrów starej siatki; właściwy poziom celu; jazda, wsiadanie, wysiadanie i blokada platformy; wieloosobowe krzyżowanie tras, wąskie przejście i zajęty cel; brak zasobu i wykonanie autonomicznej czynności; zapis w trakcie ruchu, obrotu, oczekiwania oraz przejazdu. Kontrola kolizji obejmie niezależne przypadki analityczne, nie tylko porównanie dwóch konsumentów tej samej funkcji.

Po zmianach wymagane są pełne Debug, RelWithDebInfo i ASan, diagnostyka GPU, rzeczywista interakcja, dokumentacja oraz styl. W raporcie oddzielamy właściwości wykazane dla małej sceny od braku dowodu dowolnego tłumu, pełnego solvera i globalnej optymalności. Bez pomiaru nie zgłaszamy FPS ani docelowej liczebności populacji.
