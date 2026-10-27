# Autonomia po niedostępności pilnego celu

Status: wykonano i zweryfikowano ograniczony zakres; [raport](../verification/autonomy-need-fallback.md). Kontynuacja zatwierdzonego rozszerzenia ruchu i autonomii, bez odbioru całej społeczności.

## Cel i zakres

Wyboru zadania w `TraversalSession` nie może blokować jeden rodzaj potrzeby, którego znane cele są chwilowo niedostępne. Głód zachowuje pierwszeństwo przed zmęczeniem, ale po fizycznym stwierdzeniu braku jedzenia i w czasie zapamiętanego terminu ponowienia postać może wybrać potrzebny odpoczynek. Tak samo brak fizycznej trasy do jedzenia pozwala rozważyć odpoczynek. Nie tworzymy żywności, nie odczytujemy zapasów na odległość i nie przerywamy rozpoczętego kroku, przejazdu ani czynności dla tego wyboru.

Kolejność: jedzenie przy głodzie co najmniej 250; następnie odpoczynek przy energii co najwyżej 300; konserwacja tylko bez tych pilnych potrzeb. Znane lokalizacje są publiczną wiedzą początkową tej małej sceny; dostępność zapasu nadal jest sprawdzana na miejscu. Brak wybranego zadania zachowuje jawne oczekiwanie i termin ponowienia. Nie jest to pełny model wiedzy ani planowanie społeczeństwa M04.

Pliki: `src/lab/traversal_session.cpp`, `tests/traversal_tests.cpp`, rejestr CTest, opis bieżącego zakresu i raport. Wykorzystujemy istniejące `station_retry`, `retry_at`, stan potrzeb i rezerwacje `WorkWorld`; bez nowego formatu zapisu lub dodatkowego autorytetu zasobów. Chronione kontrakty: INV-03, INV-04, INV-05, INV-06, INV-08, INV-10, INV-12, INV-16.

## Dowód

Przed poprawką: reprodukcja głodu i wyczerpania z pustym znanym magazynem i dostępnym odpoczynkiem oraz wariant z magazynem na niepołączonej powierzchni. Po poprawce: faktyczne dotarcie, rezerwacja i ukończenie odpoczynku bez fałszywego posiłku; zapis po lokalnej odmowie i podczas czynności, porównanie kontynuacji. Osobna konkurencja dwóch postaci o jedną porcję: dokładnie jedno zużycie i ukończenie, brak podwójnej nagrody po odczycie. Pełne Debug, RelWithDebInfo, ASan, próba aplikacji z D3D11 debug layer, styl i dokumentacja. Dotychczasowe reprodukcje B/C pozostają obowiązkowe.
