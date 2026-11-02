# Odbiór techniczny M06

**Status: praca w toku, 02.11.2026.** Ten dokument nie zalicza jeszcze całego zakresu technicznego. Właścicielem statusu etapu pozostaje [jeden plan](../prototype-plan.md#m06--odbiór-i-decyzja-o-skali); wykonanie określa [plan techniczny](../work-items/m06-technical-acceptance.md).

## Decyzja użytkownika

02.11.2026 użytkownik wyłączył próbę grywalności i uczestników z M06, deklarując odbiór i zlecając kontrolę solidności oraz wiarygodności fundamentu: „Zrób kurwa swoje i M06 jest odebrany. Fundament ma być kurwa solidny i wiarygodny.” Deklaracja użytkownika i wykonane kontrole techniczne są oddzielnymi dowodami. Nie przypisujemy tej decyzji automatycznym testom i nie ogłaszamy potwierdzenia atrakcyjności przyszłej gry.

## Zakres

Obecny świat 2,5D: osiem trwałych osób, siedem autonomicznych i jedna ręczna; wspólne ruch/kolizja/nawigacja, potrzeby, fizyczna dostawa, konserwacja, lokalna wiedza, ograniczone praktyki badawcze oraz zapis. Bez nowych reguł gospodarki, pełnej fizyki 3D i zwiększania populacji. Przygotowany świat ma 10000 jednostek w każdym z trzech stanowisk materiałowych; dwa stanowiska odpoczynku zachowują pojemność 2. W trakcie prób niczego nie uzupełniamy.

Opcjonalny profil zachowuje dawny `samples.csv` i dodaje CPU/Present/czas CPU procesu, nawigację, obsługę praktyk i zapis, rzeczywiste przedziały GPU, przebieg stanowisk oraz każdej osoby. Szczegół komend i jednostek: [instrukcja](../development.md#profil-wykonania-m06). GPU jest obserwowany przez timestamp/disjoint i nieblokujące odczyty, zgodnie z [kontraktem D3D11](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_query); brak wyniku nie staje się zerem.

## Zachowane porażki i naprawy

- Pierwszy pełny Release: 432/434. `contrast.native_case` wykrył niezgodność podpisu binarnego ze źródłami zmienianymi podczas wykonania; `tooling.tests` znalazł nowy test przed utworzeniem modułu analizatora. Finalne źródła wymagają zamrożenia, ponownej konfiguracji i pełnego przebiegu.
- Kandydat Release: 433/434. Natywny profil wykazał brak eksportu `actors.csv`. Niezależny przegląd również wskazał tę lukę.
- Przegląd: cztery uwagi Important i jedna Minor dotycząca niepełnych argumentów testu aliasów. Reprodukcja hard linku zmniejszyła kopię wejściowego świata z 290432 do 34 bajtów. Nowa ochrona rozpoznaje tożsamość istniejących plików; regresja musi zachować wszystkie bajty źródła.
- Analizator przyjmował uciętą historię osób, zdublowane stanowisko i inny świat/protokół. Pięć regresji najpierw wykazało fałszywy sukces. Wspólny evaluator sprawdza teraz tożsamość wejścia, procesu, protokołu, kompletne unikalne szeregi i odstęp do 1500 ms. Wynik analizatora dotyczy bramek pomiarowych, a kontynuacja wymaga osobnego odtworzenia świata.
- Pierwszy profilowany pilotaż 60 s: p95 17,0017 ms, p99 20,1225 ms, 30 ukończonych czynności. Powtórzenie: p95 22,3983 ms i p99 35,2771 ms — przekroczenie wstępnych bramek. Wyniku nie usunięto ani nie podniesiono progów.
- Diagnostyczny przebieg z walidacją każdego kroku wykazał niepoprawny stan w kroku 915: po lokalnej obserwacji już naprawionego stanowiska usuwano cel konserwacji, zostawiając trasę. Regresja `m06.completed_target` najpierw odmówiła walidacji, następnie przeszła po zwolnieniu nawigacji i wyczyszczeniu zakończonej trasy. Format świata i bilanse pozostają bez zmiany.

## Pozostałe dowody

Końcowe profile Debug/Release/ASan, pilotaż finalnej wersji, zamrożony długi pomiar, kontrola końcowego świata w nowym procesie oraz manifest dokładnych bajtów zostaną wpisane po wykonaniu. Niewykonane próby pozostają nieweryfikowane.
