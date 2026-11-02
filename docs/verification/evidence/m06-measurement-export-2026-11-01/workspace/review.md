# Niezależny przegląd i rozstrzygnięcie

Recenzent: `/root/measurement_export_review`, model `gpt-6-astra`, fresh context, read-only. Zakres: `3bf2d60f..f2493a32`; pakiet `review.diff`. Recenzent nie uruchamiał buildów ani aplikacji. Poniżej zachowano treść końcowego wyniku; dalsza poprawka i jej testy są pracą implementera, bez ponownego review.

## Wynik recenzenta

Przegląd `3bf2d60f..f2493a32`: **REQUEST CHANGES — jedna uwaga Important.**

Mocne strony: niezależny obserwator nie zmienia świata ani formatów zapisu; cofnięcie ticka jest dozwolone; rozgrzewka pozostaje poza starymi statystykami; `CREATE_NEW` chroni istniejący CSV. Testy sprawdzają dane, zgodność podsumowania, konkurencyjny plik i wcześniejsze zamknięcie okna.

**Critical:** brak.

**Important — błąd raportu blokuje ratowanie dostępnego CSV.** `src/platform/traversal_main.cpp:607`.

`export_trace()` zapisuje metadane do `log_` przed `write_new_csv()`, już po ustawieniu `trace_export_attempted_`. Jeżeli obsłużony wyjątek pętli pochodzi z zapisu raportu, strumień nadal ma ustawiony stan błędu i włączone wyjątki. Próba eksportu ponownie rzuca przy zapisie metadanych i w ogóle nie podejmuje zapisu CSV. Cały zebrany bufor zostaje utracony również wtedy, gdy katalog CSV nadal jest zapisywalny — przykładowo raport i CSV znajdują się na różnych woluminach, a na woluminie raportu zabrakło miejsca.

Należy oddzielić próbę zapisania bufora od powodzenia logowania: błąd raportu powinien nadal dawać kod 1, lecz nie może blokować dostępnego eksportu. Dodać regresję z niedziałającym strumieniem raportu i sprawnym miejscem eksportu, sprawdzając zachowane wiersze oraz brak sukcesu procesu. Status niepełnego przebiegu również wymaga określonego zachowania przy niedostępnym raporcie.

**Minor:** brak dodatkowych uwag.

**Declined to judge:**

- Zachowanie po twardym zabiciu procesu i utracie zasilania — jawnie wyłączone z gwarancji tego przyrostu.
- Długotrwała wydajność, izolowany narzut instrumentacji i odbiór przez ludzi — osobny zakres M06.

**Gotowość do scalenia: po poprawce i wymaganej końcowej weryfikacji.** Przegląd był wyłącznie odczytowy; nie uruchamiałem buildów, testów ani aplikacji. Wyniki wcześniejszych macierzy pochodzą z przekazanego ledgeru, nie z mojego niezależnego wykonania. Trwającego uzupełniania raportu nie traktuję jako defektu kodu.

## Rozstrzygnięcie implementera

Uwaga Important przyjęta. Nowa próba natywna blokuje prawdziwy plik raportu przez `LockFileEx` po starcie aplikacji. Na recenzowanej wersji kończy się kodem 1 i wykazuje utratę CSV. Poprawka zapisuje CSV i niezależny `status.txt` przed logowaniem, jawnie opróżnia raport oraz chroni obie ścieżki. Regresja po poprawce potwierdziła zachowane wiersze, status niepełny, kod 1 i nienaruszony początkowy świat. Druga regresja wykazała czerwony, a następnie zielony wynik konkurencyjnego pliku statusu. Końcowa pełna macierz jest osobnym, ostatnim dowodem w ledgerze.
