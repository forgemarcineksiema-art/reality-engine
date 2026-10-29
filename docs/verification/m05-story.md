# Historia po zmianie świata — dowody SC-13

**Aktualizacja odbioru 29.10.2026:** użytkownik zaakceptował opisany zakres bez osobnego playtestu. [Zbiorczy raport M05](m05-acceptance.md#decyzja-użytkownika) zachowuje dosłowną deklarację i oddziela ją od obserwacji; wcześniejsze wzmianki o oczekującym odbiorze opisują stan przed tą decyzją.

**Status: zatwierdzony zakres techniczny wdrożony i zweryfikowany, 29.10.2026.** [Projekt](../work-items/m05-story-design.md), [plan wykonania](../work-items/m05-story-implementation.md), [API](../story-scene.md). Odbiór przez użytkownika oraz zakończenie całego M05/M06 pozostają osobne.

## Zachowanie i granice

Wspólna aplikacja i runner prowadzą jeden świat. T przyjmuje historię przy znanym punkcie spotkania. F2 jawnie oddaje wykonawcę dotychczasowej autonomii; ręczny ruch i F4 również pozwalają ją ukończyć. Sukces wymaga fizycznych transferów źródło→torba→cel, pełnych 4/4 i ukończonego posiłku odbiorcy rozpoczętego po dostawie. Nie wymaga powrotu ani nagrody.

Trzy zaburzone początki wykonują rzeczywiste operacje: WorkWorld usuwa rozmówcę, sesja usuwa statyczny element geometrii, zwykły transfer przenosi zapas do torby drugiej osoby. Odmowy nie odtwarzają świata ani nie wymagają rozmowy z nieobecną osobą. Role używają trwałych ID, a narrator nie przekazuje NPC globalnej wiedzy. Termin pozostaje równy 600 kroków po 50 ms. Zwykłe anulowanie zachowuje materiał oraz rozpoczęte chronione operacje.

Usunięcia są ograniczonym przygotowaniem przy ticku 0. To nie pełna śmierć, fizyczne zawalenie ani ogólny silnik questów. Pierwotny rekord dostawy pozostaje związany z tą jedną historią również po zakończeniu; assign_delivery nie zastępuje go nowym kursem. Ręczny transport, potrzeby i ruch pozostają dostępne. Po sukcesie anulowanie pozostałego powrotu zamyka wykonany kurs bez zmiany fizycznego świata lub wyniku.

Opcjonalna otoczka GENESIS_TRAVERSAL 10 zapisuje historię. WorkWorld i ścieżki formatów 1–9 pozostają bez zmiany; starszy obcy świat usuwa poprzedni stan historii. Odczyt waliduje kandydata przed zastąpieniem sesji.

## Końcowe kontrole

Wykonano szeregowo, z katalogu głównego, przy zamrożonych źródłach:

| Kontrola | Wynik |
|---|---|
| cmake --preset windows-msvc; cmake --build --preset windows-debug; ctest --preset windows-debug | 420/420 PASS; CTest 205,02 s |
| cmake --build --preset windows-release; ctest --preset windows-release | 420/420 PASS; CTest 66,91 s |
| cmake --preset windows-asan; cmake --build --preset windows-asan; ctest --preset windows-asan | 422/422 PASS; CTest 311,73 s |
| cmake --build --preset windows-debug --target format-check | PASS |
| python tools/check_docs.py | PASS |
| python -m unittest discover -s tools/tests -p "test_*.py" | 58/58 PASS |

Każdy profil obejmuje 38 story.*: 27 przypadków domeny/zapisu, siedem kontynuacji w nowych procesach oraz cztery native. ASan ma dodatkowo dwie dotychczasowe kontrole diagnostyki. Nie podniesiono limitów procesów 8/18 s ani CTest 20 s i nie zmieniono dawnych wyjątków. Powyższe czasy są czasami kontroli, nie pomiarem wydajności aplikacji.

Siedem zachowanych kontynuacji obejmuje trzy zaburzenia przed startem, rzeczywisty chód z pełną torbą, odmowę po fizycznym zwrocie, sukces i niewykonanie. Końcowe bajty procesu odniesienia i kontynuacji są identyczne; checkpoint pozostaje nienaruszony. Ślady i hashe znajdują się w final-fresh/results.json wewnątrz archiwum.

Cztery zachowane próby Win32/D3D11 sprawdzają T/F2/F5/F9, rzeczywisty status kontrolki, GPU/debug, brak błędów symulacji oraz poprawny obcy świat po F9. Launcher Release przeszedł także własny --smoke-test. Obejrzano końcowe obrazy normal, missing-person i missing-place: odpowiednio cztery/trzy/cztery kapsuły i faktyczny brak elementu miejsca. Obrazy obejmują viewport; panel potwierdza osobna kontrola GetWindowText. Automat nie zastępuje ludzkiej oceny czytelności.

## Zachowane porażki i przegląd

Archiwum zachowuje wszystkie fazy, nie tylko końcowy sukces:

- API RED, następnie rzeczywisty runtime RED przyjęcia; rozszerzenie początkowo 8/15 z brakującym kodekiem i resetem obcego świata. Ponowne użycie slotu usuniętej osoby wykazało błąd ról; walidacja używa teraz historycznego ActorCreated.
- Dalsze RED wykazały brak kontekstu usuniętego miejsca, błędną podporę kontaktu, możliwość podmiany przyjętego kursu, odczyt anulowanego kursu jako aktywnego, błędny materiał/pracę posiłku, przyszłe zdarzenia obserwacji i ruchome wiązania ról.
- terminal-red wykazał fałszywe Failed przy aktywnym kursie oraz przypisanie późniejszego zlecenia. Uzupełniający review wykazał poszerzalne order_end. terminal-followup-runtime-red wykonał późniejszy ręczny transport i posiłek, następnie sfałszował zakończenie. Naprawa chroni pierwotny rekord i granice komend oraz sprawdza jego rzeczywisty wynik.
- completed-return-runtime-red wykazał sprzeczność po anulowaniu powrotu już ukończonej ręcznej historii. Poprawka zachowuje sukces i fizyczne bajty WorkWorld; manual sprawdza również roundtrip po tej operacji.

Korekty fixture'ów są jawne w logach: ściana wymagała przesunięcia układu świata x=3, jednorazowe wejście dowodu przemieszczenia zamiast końcowej prędkości, źródło rzeczywistego zużycia zamiast nieistniejącej pozostałej rezerwacji, a Step rozpiętości 0,6 zgodnej z limitem 0,65. Ręczny kurier po rozładunku fizycznie odsłania dojście odbiorcy i pozostawia czas na lokalne ponowienie w tym samym horyzoncie 600. Próba edycji native początkowo przerwała się na dekodowaniu CP1250 przed zapisem; poprawiona używa UTF-8. Wczesne błędy dispatchu i nazwy serializatora w testach są błędami przygotowania, nie dowodami wykonania.

Pierwszy niezależny reviewer podał częściowy finding, po czym zatrzymał go limit modelu. Zastępczy reviewer przejrzał cały pion względem 83852e9 i dwa uzupełnienia; końcowy wynik APPROVE, oba jego findings zamknięte. Był to przegląd read-only bez uruchamiania testów; wyniki macierzy pochodzą z wykonania integrującego.

## Dowody i decyzje wykonawcze

[Manifest](evidence/story-sc13-2026-10-29/manifest.json) oraz [archiwum](evidence/story-sc13-2026-10-29/runs.zip) obejmują źródła, ledger, wszystkie fazy RED/GREEN, komendy z kodami wyjścia/czasami, przegląd, obrazy, zachowane zapisy i hashe programów. Sprawdzono CRC ZIP oraz SHA-256 każdego wpisu. source-freeze.json wiąże macierz z dokładnymi bajtami źródeł.

Wykonano pracę w istniejącym checkout na gałęzi codex/m05-story-design, bez dodatkowego worktree; zachowano dotychczasowy kontekst i dowody kosztem braku osobnego katalogu izolacji. Ledger i artefakty zachowano w archiwum zamiast ich usuwania, zgodnie z zasadami repozytorium.

## Uruchomienie i odbiór

Po buildzie Release: `python tools/start_story_playtest.py`. Pozostałe początki: `--case missing-person`, `--case missing-place`, `--case moved-supplies`. Launcher wypisuje prywatny nowy katalog dowodów. T przy punkcie spotkania prosi o przyjęcie, F2 oddaje/przejmuje autonomię, F4 przenosi materiał, Spacja/N uruchamia/dodaje czas, F5/F9 zapisuje i odtwarza.

Nie przeprowadzono jeszcze odbioru tej historii przez użytkownika. Wcześniejsza akceptacja M04 i uczenia nie obejmuje SC-13. Nie przypisujemy automatom akceptacji odczucia sterowania, czytelności lub całego M05.
