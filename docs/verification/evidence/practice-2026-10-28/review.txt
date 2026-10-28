# Przegląd integracji praktyk

Niezależny przegląd całego zakresu7f4aa87..198c30f, świeży kontekst GPT-6 Astra: **REQUEST CHANGES**, cztery Important, bez Critical i bez odłożonych Minor. Reviewer nie zmieniał źródeł ani nie uruchamiał pełnych zestawów testów.

1. Własna próba przy24 otrzymanych obserwacjach była przyjmowana, a dopisanie wyniku przekraczało pojemność pamięci. Regresja `practice.evidence_capacity` odtwarza trzech świadków po8 pomiarów, fizyczne przekazy i odmowę25. wyniku bez zużycia energii ani konfiguracji.
2. Zapis pozwalał usunąć WORKING z trwającej konserwacji. `practice.forged_work` zmienia wyłącznie ten rekord i checksum; odczyt musi transakcyjnie odmówić, zachowując ochronę wykonawcy i zajętość urządzenia.
3. Rozmowa mogła zakończyć się na innej podporze niż zapisana na początku i stworzyć stan nieczytelny po odtworzeniu. `practice.contact_support` naprawdę przeprowadza dwie osoby przez dwa równoległe przejścia i rozpoczyna przekaz przed wspólną zmianą podparcia. Pierwszy zakres wymaga utrzymania zapisanej nieruchomej podpory; zmiana przerywa przekaz.
4. Zapis dopuszczał oznaczenie oczekującego impulsu zamrożonego umysłu jako dalszego uczenia. `practice.frozen_pending` fałszuje oba znaczniki i odpowiednio zwiększa liczniki. Odczyt ma porównać klasyfikację każdego impulsu z odtworzoną chronologią zamrożenia, także przed ukończeniem pomiaru.

Autor odtworzył RED wszystkich czterech przypadków przed poprawkami. Pierwszy fixture przypadku4 szukał po zakończeniu pierwszej80-krokowej pracy zbyt wcześnie; po zapewnieniu dojścia do rzeczywistego oczekującego impulsu odtworzono właściwy błąd. Limit CTest20 s i budżety mieszkańca pozostają bez zmiany.

Reviewer pozostawił do decyzji autora brak PERMIT: dotychczasowa kolejka sama usuwała zgodę przed nową przyjętą czynnością. Uzgodniono kontrakt: zgodę zużywa przyjęta konserwacja albo zastępuje nowa przyjęta konfiguracja; sama nieudana próba wyboru pracy jej nie usuwa. Usunięto przedwczesne czyszczenie i dodano `practice.missing_permit`: RED usunięcie niezużytej zgody z poprawnym checksumem → wymagane odrzucenie.

Oględziny rzeczywistego okna wykazały dodatkowo obcięcie ostatniej linii ręcznego ustawienia po pojawieniu się historii i nieaktualną etykietę kolejki po odejściu do innej potrzeby. Zwiększono pole stanu i uzależniono etykietę od bieżącego zamiaru oraz obecności przy urządzeniu. To oddzielna korekta czytelności, nie nowa reguła uczenia.

Końcowe wyniki GREEN, profile kompilacji i pomiary należą do [raportu integracji](practice-in-prototype.md). Nie wykonywano kolejnego re-review; zamknięcie poprawek wymaga regresji i pełnej weryfikacji autora. Ocena czytelności przez użytkownika pozostaje osobna.
