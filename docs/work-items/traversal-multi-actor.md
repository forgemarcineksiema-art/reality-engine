# Praktyczny odbiór toru przestrzennego wielu postaci

Status: wykonano i zweryfikowano techniczny zakres po wyraźnej zgodzie użytkownika; [raport odbioru](../verification/traversal-multi-actor.md). Podstawa: weryfikacja i wdrożenie potwierdzonych napraw z [dostarczonego audytu](../../traversal-multi-actor-audit.md). Nie jest to odbiór M04 ani ocena komfortu przez użytkownika.

Audyt pozostaje propozycją diagnoz i algorytmów, nie dowodem działania ani automatycznie normatywną implementacją. Najpierw zachowujemy reprodukcje. Zakres: kolejki kładki i promu, sterowanie i hamowanie, podparcie przy ruchu/obrocie, komendy, spójność zapisu i diagnostyka. Pliki produkcyjne zgodnie z audytem, a nowe przypadki w osobnym `tests/traversal_audit_tests.cpp`, by nie powiększać dotychczasowego runnera. Kontrakty: INV-02, INV-03, INV-09, INV-10, INV-11, INV-12 i INV-16; scenariusze SC-06, SC-07, SC-08, SC-11.

Wspólne zapytanie lokalnego podparcia wymagało również `src/physics/spatial_geometry.hpp/.cpp`; współdzieli ono istniejący wzór zamiast tworzyć inną geometrię w module platform. Nawigacja przyjmuje opcjonalny widok stojących kapsuł, a sesja wyznacza kandydatów z lokalnego otoczenia. Bez nowego stanu zapisu i zależności.

Wymagane są przypadki szybkiego retargetowania, hamowania i odwrotu przy krawędzi, stoków, obracających się podpór z małą kapsułą, wymiany pasażerów, mijania, przeciwnego ruchu na kładce, zajętego celu oraz uszkodzonych zapisów i podziału czasu. Zapytania o obraz nie mogą mutować świata; ich pozycje muszą zachować kolizję. Przebieg domyślnej sceny musi wyjść poza dotychczasowe 5 kroków. Trasy i kolejki pozostają ograniczonym rozwiązaniem, bez obietnicy dowolnego tłumu lub kompletności ciągłej.

Nie zmieniamy starych formatów M03 ani reguł materiału/pojemności. Zachowujemy wcześniejsze testy i zapisy. Pełne Debug, RelWithDebInfo, ASan, próby okna i diagnostyka D3D11, styl oraz dokumentacja. Nie zmieniamy limitów czasu dla ukrycia porażek. Nie jest to rozpoczęcie M04 ani ludzka akceptacja odczuć. Kopie plików sprzed zmian: `build/multi-audit-baseline/`.
