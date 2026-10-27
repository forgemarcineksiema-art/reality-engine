# Wejście w M04: zakres zatwierdzonego kierunku

Użytkownik 26.10.2026 polecił rozpocząć pracę według rekomendacji: ograniczone domknięcie ruchu, rzeczywista dostawa między osadami, minimalne rozwiązywanie konfliktu i osobisty udział gracza. To zgoda na ten kierunek i jego wykonanie etapami, bez zmiany planu M00–M06. Ten dokument nie stwierdza implementacji przyszłych części.

Rozważone kierunki: dalsza samodzielna rozbudowa fizyki daje większy zakres geometrii, lecz odkłada sprawdzenie gry; samodzielna rozbudowa negocjacji daje zachowania społeczne bez realnego celu; wybrany mały fragment dostawy wiąże technologię z konsekwencją dla mieszkańców. Rozwój pozostaje na własnym C++20/Win32/D3D11, bez nowych bibliotek fizyki lub AI.

## Pierwszy przyrost: obserwacja postępu i odbiór ruchu

Historyczny zakres pierwszego obserwatora został rozszerzony przez [domknięcie diagnostyki i bezpieczne potrzeby](diagnostics-needs-hardening.md). Poniższy opis zachowuje punkt wyjścia; aktualny kontrakt obserwacji i wersji zapisu podaje [model autonomii](../traversal-autonomy.md).

Potrzebny jest rzeczywisty konsument diagnostyki w oknie: podejrzenie utknięcia nie może opierać się na samym `speed` albo `blocked`. Obserwator ma wykrywać brak poprawy odległości do końca bieżącego odcinka Walk przez 100 kroków świata (5 s), przy wymaganej poprawie o 0,1 m. Nowy cel lub koniec odcinka zaczyna nowe okno. Sam wzrost licznika planów nie resetuje obserwacji, jeśli cel odcinka się nie zmienia. Oczekiwanie w kolejce, przejazd, brak drogi, osiągnięty cel i bezczynność mają odrębne klasyfikacje. Duży brak obserwacji nie jest dowodem utknięcia.

Jest to diagnostyczne podejrzenie, nie dowód braku dowolnej drogi. Nie mutuje świata i nie podejmuje decyzji NPC. Próba wpływania nim na zachowanie wymaga osobnej reprodukcji i testu. Obserwator nie trafia do formatu świata; po wczytaniu rozpoczyna pomiar od nowa. Sam świat i istniejące formaty pozostają bez zmiany.

F6 zapisuje nowy katalog z `world.save` i raportem przyczyn. Automatyczna migawka przy pierwszym podejrzeniu ma limit liczby katalogów na sesję. Błąd zapisu jest widoczny i nie dotyka zwykłego zapisu F5. Migawka jest stanem do analizy, nie pełnym odtworzeniem wcześniejszej historii klawiszy. Nie wysyłamy danych poza komputer.

Pomiar okna obejmuje rzeczywiste czasy klatek, pracy symulacji i zaległość zegara, liczbę wykonanych kroków, rozmiar widoku, adapter i pamięć procesu. Brak osobnego pomiaru czasu GPU nie uprawnia do diagnozy wąskiego gardła GPU. Krótka próba kontrolna oraz dłuższa próba 30 s przygotowania + 300 s pomiaru w RelWithDebInfo, bez warstwy debug GPU i konkurencyjnych buildów. Domyślne osiem postaci i skończone zapasy; nie interpretujemy wyczerpania sceny jako dowodu gospodarki. Wynik negatywny zatrzymuje rozszerzanie skali i jest raportowany, nie ukrywany.

Odbiór: klasyfikacja rzeczywistego postępu i krążenia, zachowanie po replanningu, brak fałszywego alarmu w kolejce/przejeździe, reset po wczytaniu, poprawna migawka możliwa do wczytania istniejącym F9, jawny błąd katalogu, pełne regresje, styl i próba GPU. Kontrakty INV-02, INV-09, INV-11, INV-12, INV-16; scenariusze SC-07, SC-08, SC-11, SC-12. Limit czasu CTest nie zmienia się.

## Następne przyrosty zatwierdzonego kierunku

1. Transfer istniejącego materiału w `WorkWorld`, z bilansem, idempotencją i zapisem. Bez konkurencyjnego licznika żywności w sesji lub rendererze.
2. Dostawa: pobranie na miejscu → rzeczywisty niesiony zasób → przejście → oddanie odbiorcy → powrót, z przerwaniem i potrzebami wykonawcy. Opóźnienie wpływa na zapas dostępny dla posiłków. Najpierw obecna mała scena; 12–16 osób wymaga pomiaru.
3. Lokalna prośba o ustąpienie, niezależna zgoda/odmowa, bezpieczny ruch i alternatywa po odmowie. Bez automatycznej agresji lub przejęcia gracza.
4. Gracz przenosi ten sam ładunek lub fizycznie wpływa na przejście, widzi przyczynę i skutek. Ocena gry oraz szerszy odbiór M04 są osobne.

Te części otrzymają plany z konkretnymi interfejsami po odczycie wyników pierwszego przyrostu. Nie tworzymy teraz pustych managerów, atrap transportu ani fikcyjnych sukcesów dostawy.
