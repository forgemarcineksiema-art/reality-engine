# Zbiorczy raport M05 — ciągłość, przejście i historia

**Status: M05 zaliczone w zatwierdzonym zakresie prototypu, 29.10.2026.** Podstawa: dowody techniczne poniżej i bezpośrednia deklaracja użytkownika bez oddzielnego playtestu. [Jeden plan](../prototype-plan.md) pozostaje właścicielem statusu etapów; M06 nie jest przez to wykonane.

## Zakres i dowody zamknięcia

| Wymaganie | Wynik i granica |
|---|---|
| SC-08 — ciągłość i błąd zapisu | [Audyt](sc08-continuity-audit.md): 16 dokładnych kontynuacji w nowych procesach, rzeczywista odmowa podmiany pełnego pliku, uszkodzony/nieobsługiwany odczyt, idempotencja i stare wersje. Nie jest to próba awarii sprzętu |
| SC-09 — rzeczywista zmiana reprezentacji | [Jeden kurier lokalny/daleki](m05-distant-delivery.md): przejęcie odpowiedzialności za ten sam transport, ciągłość ID/partii/czasu i bezpieczny powrót. Ograniczony model statycznych terminali; nie ogólny streaming, pojazdy lub fizyka między terminalami |
| SC-10 — dwie działające metody | [Naprawa albo objazd](m05-two-methods.md): poznana metoda, koszt energii/części i rzeczywista konserwacja albo transport istniejącym promem. Obie metody dają 8/8 i posiłek; pozostawiają inny stan mostu i koszt |
| SC-13 — historia respektuje fakty | [Zaopatrzenie](m05-story.md): dostawa 4/4 i posiłek albo określona odmowa po rzeczywistym usunięciu rozmówcy/statycznego miejsca lub przeniesieniu zapasu przed startem. Jedna autorska scena, bez ogólnej śmierci, zawalenia i silnika questów |

Systemy korzystają ze wspólnej TraversalSession i właścicieli domenowych; sceny są odrębnymi konfiguracjami tego prototypu. Nie twierdzimy, że wszystkie sytuacje połączono w jeden nieprzerwany przebieg gracza. Nie zmieniono starego zachowania materiału/pojemności, reguł kontaktu, chronionych operacji ani dawnych formatów. WorkWorld posiada zasoby i skutki; wiedza NPC pozostaje lokalna. Daleki model nie udaje lokalnej fizyki, a usunięcie statycznego elementu nie udaje fizycznego rozpadu.

## Weryfikacja techniczna

Końcowa pełna macierz implementacji SC-13 na 1f620fc: Debug 420/420, Release 420/420, ASan 422/422 PASS, format oraz dokumentacja PASS, Python 58/58 PASS. [Raport i manifest](m05-story.md) wiążą te wyniki z dokładnymi źródłami i programami. Obrazy i rzeczywiste próby Win32/D3D11 są zachowane w raportach przyrostów. To wykonana wcześniejsza macierz, nie ponowne pełne uruchomienie w tym audycie.

Audyt M05 ponownie wykonał szeregowo buildy i wybrane 123/123 w Debug, 123/123 w Release, 123/123 pod ASan oraz dodatkowe kontynuacje i rzeczywiste błędy plikowe opisane w SC-08. Nie zmieniono C++ ani formatów. Aktualizacja README wymagała odświeżenia metadanych natywnych źródeł przez build; źródła silnika i testów pozostają te same co 1f620fc. Aktualne wyniki kontroli dokumentów i przeglądu są w archiwum audytu.

Wcześniejsze timeouty Debug/ASan oraz pojedynczy timeout narzędzi Release pozostają w raportach dwóch metod i kuriera. [Naprawa kosztu uczenia](learning-cost.md) usunęła wykazany koszt CPU bez zmiany modeli, asercji lub limitów; pełne wyniki po naprawie nie wymazują wcześniejszych porażek. Nie wyprowadzamy z nich gwarancji braku przyszłych timeoutów ani przewagi planera nad rutyną.

## Decyzja użytkownika

29.10.2026 użytkownik polecił w tym czacie:

> Wykonaj audyt SC-08 i zbiorczy raport M05. Mój odbiór pomiń, ja akceptuję, ufam na słowo.

To bezpośrednia akceptacja przedstawionego zakresu M05 i odstąpienie od osobnej próby odbiorowej nowych sytuacji. Zamyka bramkę decyzji użytkownika; nie pozostawiamy jej ponownie do zatwierdzenia. Nie wpisujemy jednak nieistniejącego grania, obserwacji czy ocen komfortu/czytelności. Nie wykonano obserwacyjnego playtestu tych sytuacji przez użytkownika. Deklaracja nie jest empirycznym pomiarem ergonomii ani akceptacją przyszłych systemów.

## Pozostały zakres i następny etap

W zatwierdzonym zakresie M05 nie pozostała otwarta bramka implementacji lub odbioru. M06 pozostaje planem: zamrożony protokół dłuższej próby aplikacji, pomiar na wskazanym sprzęcie, odbiór i decyzja o skali według jednego planu. Obecne polecenie nie wykonuje ani nie znosi wymagań M06. Pełne 3D, ogólny streaming, dynamiczne zawalenia, pełne instytucje i rozbudowana walka pozostają poza tym zaliczeniem.

## Dowody i sposób wykonania

[Manifest](evidence/m05-audit-2026-10-29/manifest.json) i [archiwum](evidence/m05-audit-2026-10-29/runs.zip) zachowują wykonanie audytu i dokładne źródła. Odsyłacze do wcześniejszych manifestów są sprawdzane bez przepisywania ich wyników. Niezależny przegląd całego audytu zakończył się APPROVE, bez istotnych findings. Reviewer odczytał kod i dowody oraz niezależnie porównał SHA-256 wszystkich 16 par; nie uruchamiał buildów lub testów. Kontrola rejestru, odsyłaczy i checksumów dokumentacji przeszła; narzędzia Pythona przeszły 58/58. Wyniki są zachowane w archiwum.

Pracowano w istniejącym checkout na nowej gałęzi codex/m05-continuity-audit, zachowując kontekst dowodów zamiast dodatkowego worktree. Ledger i artefakty zachowano w archiwum zamiast ich usuwania. Są to jawne decyzje wykonawcze, nie nowe wymagania produktu.
