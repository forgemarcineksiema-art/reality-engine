# Naprawa odroczonych poleceń i własności przejść

Status: wykonano i zweryfikowano ograniczony zakres naprawy na zlecenie użytkownika po GENESIS-HARDENING-001; [raport odbioru](../verification/traversal-command-hardening.md). Zakres obejmuje naprawę B i C, osobną reprodukcję blokady ręcznego przechodzenia między stykającymi się płytami oraz kontynuację po zapisie. Wcześniejsze dowody porażek pozostają zachowane.

## Zasady zmiany

- Najnowsza przyjęta komenda zastępuje poprzednią odroczoną komendę. Ponowienie bieżącego dokładnego celu usuwa odroczony cel/podejście/stop, zachowując bieżącą trasę, licznik planowań, prędkość i bezpieczne dokończenie kroku lub transferu.
- Własność przejścia trwa podczas wejścia oraz fizycznej obecności kapsuły w jego obszarze. Zwalnianie używa wspólnej reguły po ruchu i po zmianie planu; nie zależy od posiadania celu. Zmiana identyfikatora podparcia w obszarze nakładających się płyt sama nie dowodzi wyjścia kapsuły z przejścia.
- Ręczne prowadzenie może korzystać z ciągłego podparcia na sąsiadujących współpłaszczyznowych powierzchniach. Sam wolny punkt końcowy na innej płycie nie wystarcza: trzeba wykazać pokrycie całego odcinka. Nie dodajemy skoku nad luką, nowego solvera ani zmiany zasad stopni.

## Zakres i dowód

Pliki produkcyjne: sesja traversal oraz zapytanie o ciągłe podparcie w nawigacji powierzchniowej. Testy: dotychczasowe czerwone regresje B/C bez osłabiania asercji, ręczna zmiana płyty, luka i różnica wysokości, ponowienie celu podczas kroku/przejazdu, kolejka przy wyjściu oraz zapis/odczyt. Chronione kontrakty: INV-02, INV-09, INV-10, INV-11, INV-12 i INV-16. Format zapisu nie otrzymuje nowych pól.

Wymagane są buildy i pełne testy Debug, RelWithDebInfo oraz ASan, styl i kontrola dokumentacji. Wpisujemy istniejące konfiguracje `.opencode/agents/` do mapy jako dokumenty narzędziowe; nie nadajemy im odpowiedzialności za kontrakty silnika ani nie zmieniamy ich treści. To usuwa zastaną zaległość rejestru, bez wyłączania kontroli.

Naprawa nie jest odbiorem całego rozszerzenia geometrii/autonomii ani nową oceną odczuć użytkownika. M00–M06 i zatwierdzona wizja pozostają bez zmiany.
