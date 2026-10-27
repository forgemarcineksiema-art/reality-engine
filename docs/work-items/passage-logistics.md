# Przejście, lokalna wiedza i odtworzenie logistyki

Status: przyrost zrealizowany i zweryfikowany 27.10.2026; wyniki i ograniczenia są w [raporcie](../verification/passage-logistics.md). To kontrolowany zakres SC-10 i części SC-14, bez odbioru M04 ani fizycznego zawalenia konstrukcji.

## Cel i wybór architektury

Łańcuch: miejscowe zamknięcie lub zużycie konstrukcji → lokalne odkrycie/przekaz → rzeczywisty objazd promem z partią żywności → posiłek → materialna naprawa → ponowne korzystanie z kładki. INV-08/09/10/12/14/16 oraz wspólne reguły materiału i czynności pozostają wiążące.

Wybór: dostępność i opcjonalny indeks Maintain należą do istniejącego PassageState. WorkWorld nadal posiada materiał i czynność; sesja aktualizuje dostępność po faktycznym ukończeniu Maintain. Odrzucono globalne usuwanie połączenia z danych planera NPC (telepatia) i samo filtrowanie A* (brak ochrony ruchu ręcznego). Planer filtruje po lokalnej pamięci, wykonawca ruchu sprawdza rzeczywistą barierę na powierzchni zamkniętej kładki. To blokada użytkowania, nie zniszczenie bryły.

Nowe passage_beliefs mają open, observed_tick, learned_tick, source_actor i via_actor. Domyślnie znane przejścia uznaje się za otwarte. Negatywna informacja ma horyzont 100 kroków; przekaz nie odmładza czasu. planned_closed_passages zapamiętuje maskę używaną przy planowaniu: zamknięcie na dalszej trasie lub świeża informacja o otwarciu wymusza ponowienie dopiero poza Step, Boarding/Riding/Exiting, czynnością i clearance. Wiek raportu dopuszcza nowy zwiad i ponowienie NoPath, ale sam nie kasuje wykonalnego objazdu. Globalna zmiana wersji nie jest sama kanałem informacji.

Obserwacja przy przyczółku: wspólna podpora, do 1,5 m i bezpośredni sweep kapsuły do przyczółka. Istniejący kierunkowy share_knowledge/F7 przenosi również te rekordy z dotychczasowym kontaktem do 2 m i ścisłym porządkiem observed_tick. Własne odkrycie zmiany/braku uruchamia ograniczone okno przekazu (40 kroków, próby co 10), odebranie raportu samo go nie uruchamia.

Zamknięcie odmawia na kapsule przy powierzchni przejścia, rozpoczętym Step i związanym clearance. Przyjęcie zwiększa wersję geometrii/topologii i zwalnia nieprzydzieloną kolejkę. wear==1000 ponawia bezpieczną próbę; nie usuwa człowieka, jeśli zamknięcie jest chwilowo niemożliwe. Ręczna operacja wymaga sterowania ręcznego i fizycznego dojścia; otwarcie przy wear==1000 wymaga rzeczywistej naprawy. Maintain zużywa typ 2, 1 jednostkę i trwa 80 kroków jak dotychczas. Naprawa nie zwraca zużytego materiału.

## Zakres i kolejność

1. Czerwone testy w tests/passage_tests.cpp i CMake: niewiedza/objazd/dostawa/posiłek, świadek, zużycie/naprawa, odmowy, chronione operacje, spokojny i zaburzony przebieg, kontynuacja i korupcja.
2. traversal_session.hpp i nowy traversal_passages.cpp: stan, lokalne API, obserwacja, maska planera, bezpieczne zamknięcie, fizyczna blokada i walidacja. Integracja traversal_session.cpp, traversal_needs.cpp i traversal_knowledge.cpp.
3. traversal_codec.cpp: wersja 6 i pełny odczyt 1–5; dotychczasowe bajty bez nowego stanu. Walidacja ID, mapowania/linku i typu materiału, czasów, masek, zamkniętej powierzchni/kolejki i aktywnych operacji; transakcyjne zatwierdzenie.
4. Osobna scena make_crossing_course / --crossing zachowuje historyczne --delivery i liczbę kroków jego regresji. Dwie osady, krótsza kładka, dłuższy prom z fizycznymi bankami po obu stronach, żywność typ 1 i konserwacja typ 2. F8 zmienia pobliskie przejście, R rozpoczyna miejscową konserwację wspólną czynnością. Inspektor odróżnia prawdę i przekonanie.
5. Pełne Debug/Release/ASan, format-check, walidator i testy Pythona, D3D11/MX330/gpu-debug. Niezależny review, raport i dokumentacja. Dowody także porażek w docs/verification/evidence/passage-logistics/. Commit wyłącznie powiązanych plików na codex/passage-logistics. Publikacja na publiczne `origin` odbywa się na polecenie użytkownika z 27.10.2026; nie zastępuje odbioru technicznego.

## Granice odbioru

Własny C++20/Win32/D3D11, bez nowych zależności. Nie podnosimy limitów ani nie skracamy starych scenariuszy. Raport rozdzieli rzeczywiste dostawy i koszty, działanie okna, granice fizyki, komfort człowieka i nadal nieukończone M04. Zamknięcie po zużyciu odrzucone na człowieku pozostawia wear==1000 i otwarte przejście do następnej bezpiecznej próby; taki stan jest poprawny także po zapisie.
