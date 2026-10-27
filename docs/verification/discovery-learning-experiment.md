# Eksperyment odkrywania zależności i przekazu — raport

**Status: kontrole techniczne w toku; próba potwierdzająca jeszcze niewykonana.** Ten nagłówek zostanie uzupełniony wyłącznie rzeczywistymi wynikami. Właścicielem kryteriów jest [protokół discovery-v1](../work-items/discovery-learning-experiment.md), [API](../discovery-learning.md) opisuje implementację. Użytkownik zatwierdził implementację i wykonanie próby 27.10.2026.

## Historia wykonania przed zamrożeniem

Zachowujemy czerwone buildy brakujących modułów i konsumenta, pierwszy błąd zapisu komendy w planie, nieudaną fixture ściany oraz nieudany build po przestawieniu zależnych nagłówków Windows/PSAPI przez formatter. Ściana miała początkowo górę na poziomie podłogi; podniesienie góry do 2 m poprawiło fixture, bez zmiany kryterium blokowania kontaktu. Pierwszy pełny CTest 313/313 ruszył mimo nieudanego builda, dlatego nie stanowi końcowej weryfikacji aktualnych źródeł.

Niezależny odczytowy review wskazał sprzeczny stan pending po wczytaniu, błąd porównania dokładnej granicy 0,20 w float, pominiętą kontrolę integralności w CLI analyze, niepełne koszty faz i brak fizycznej kontroli spoza rodzin. Każdy brak otrzymał czerwony test. Poprawki nie zmieniły progów ani seedów. Końcowy wynik review i pełnych serii będzie zapisany po ich zakończeniu.

Nowa kontrola negatywna wykonuje rzeczywiste 16/32/64 eu przez rodzinę spoza modelu; oczekiwane wyjścia 0/32/64 prowadzą do model_inadequate. Jest to kontrola mechanizmu, odrębna od 64 światów. Kryteria D/G/S pozostają nieweryfikowane do wykonania zamrożonej próby.

## Wymagany zakres dowodów

Raport końcowy zachowa konkretne polecenia, Debug/Release/ASan, format i dokumentację, review z ograniczeniami, manifest/hash źródeł i binarnego, surowe przebiegi, analizę parami i wszystkie negatywne wyniki. Nie będzie przypisywał headless dowodu nowego obrazu 3D ani ludzkiej oceny sterowania. Nie jest to kolejny odbiór M04 ani zakończenie M05/M06.
