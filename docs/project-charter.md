# Karta projektu

**Właściciel tematu:** cele, ustalenia użytkownika i granice produktu. Algorytmy, liczby prób i wyniki należą do dokumentów specjalistycznych.

## Ustalone przez użytkownika

- Genesis łączy otwarty świat, Action RPG i trwałą symulację społeczeństwa.
- Docelowy kierunek to przyziemne, realistyczne i brutalne fantasy: magia, królestwa i potwory są częścią autonomicznej, ewoluującej cywilizacji. Świat ma wiarygodne stosunki społeczne, gospodarkę, politykę, ekosystemy, kulturę i historię; nie jest wyłącznie zbiorem zadań i przeciwników.
- Magia i istoty fantastyczne podlegają określonym regułom, ograniczeniom, kosztom i konsekwencjom. Realizm oznacza spójność przyczynową świata, nie twierdzenie, że fantastyka odpowiada znanej fizyce rzeczywistej.
- Ludzie mają pamięć, relacje, ograniczoną wiedzę oraz własne plany; organizacje i miasta odczuwają skutki działań.
- Zmiany gospodarcze i społeczne wynikają z reguł świata. Historie autorskie współistnieją z emergencją.
- Silnik, fizyka i nawigacja są własne. Unreal Engine został odrzucony. Zależności ograniczamy do minimum.
- Pierwszy rezultat łączy grywalny fragment i laboratorium małej autonomicznej społeczności. Grafika ma najmniejszy priorytet.
- Projekt realizuje użytkownik ze swoimi agentami AI. Nie ma narzuconego terminu.
- Punktem odniesienia jest laptop użytkownika i5/MX330/8 GB RAM. Dokładny odczyt i proponowane budżety są w [budżetach](performance-budget.md).

## Hierarchia wartości

Przyjęta na potrzeby pracy interpretacja: poprawność i trwałość → grywalna interakcja z autonomiczną społecznością → większa skala i głębia → rozbudowa grafiki. Jest to robocze uszczegółowienie odpowiedzi użytkownika, nie zgoda na pogorszenie sterowania lub czytelności.

Jakość kamery, ruchu, bezpośredniej interakcji i późniejszej walki jest częścią grywalności; nie odkładamy jej razem z jakością grafiki. Projekt Action RPG rozwija się równolegle z symulacją. Sprawne laboratorium samo nie realizuje celu produktu.

Brak terminu pozwala rozwijać systemy kolejno. Nie oznacza nieskończonego budżetu RAM, wywołań modeli, rozmiaru zadania ani równoległych zmian.

## Pierwszy dowód produktu

Gracz wpływa na dostępność przejścia, co zmienia realny transport, dostępność zasobu i działanie mieszkańców. Ludzie reagują zgodnie z posiadaną wiedzą. Świat kontynuuje po zapisie, zachowując stan i zobowiązania. Gra oraz runner korzystają ze wspólnego modelu domenowego.

Ten rezultat weryfikują scenariusze w [strategii testów](testing-strategy.md). Sukces techniczny i zainteresowanie gracza są odrębnymi dowodami.

## Granice zatwierdzenia fikcji

Kierunek artystyczny i produktowy jest zatwierdzony w DEC-008. Szczegóły świata, poziom technologii, system magii i pozostałe wymienione tam wybory pozostają otwarte. Wskazówkę MCU traktujemy jako inspirację rozległością i spójnością uniwersum, bez kopiowania własności innego świata.

DEC-011 zachowuje silnik, fundamenty i plan M00–M06. Pierwsza scena dwóch osad i mostu otrzymuje kontekst fantasy, ale zachowuje koszty, transport, ograniczoną wiedzę, autonomię i kontynuację. Magia, potwory i rozbudowana walka nie są warunkami jej technicznego prototypu. Nie rozpoczynamy wielkiej produkcji zawartości przed sprawdzeniem podstaw. Wolne Dorzecze nie jest obowiązującym miejscem akcji; jego wartościowe mechaniki pozostają inspiracją.

## Statusy twierdzeń

| Status | Znaczenie |
| --- | --- |
| USTALONE | Bezpośrednia decyzja użytkownika, zapisana powyżej lub w rejestrze |
| PROPOZYCJA | Odwracalny wybór projektowy do sprawdzenia |
| PLAN | Wymagana praca, której jeszcze nie wykonano |
| WYNIK | Wykonana próba z dostępnym zapisem metody i rezultatu |

Nie wolno zamieniać propozycji w wynik przez powtarzanie jej w dokumentacji. [Rejestr decyzji](decisions.md) odróżnia ustalenia od roboczego stosu technicznego i pytań.

## Poza pierwszym dowodem

Pełna kampania, duże miasta, multiplayer, dowolne cięcie geometrii, pełna symulacja materiałów, epoki, otwarta ewolucja i zaawansowany rendering pozostają dalszą wizją lub badaniami. Nie są automatycznie zatwierdzonym backlogiem.

Wrażenie życia, autonomia, adaptacja, reprodukcja i świadomość nie są synonimami. Świadomość nie jest warunkiem ukończenia gry ani twierdzeniem, które można wyprowadzić z przekonującej rozmowy.
