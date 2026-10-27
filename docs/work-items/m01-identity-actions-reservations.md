# Brief tożsamości, czynności i zasobów — etap M01

Status: wykonane w opisanym zakresie; [raport weryfikacji](../verification/m01-identity-actions-reservations.md). Zakres wynika z M01 oraz INV-01, INV-03, INV-04, INV-05, INV-06, INV-11 i INV-12.

## Kontrakt przed implementacją

- ID są monotoniczne w obrębie kontynuowanego świata, nigdy ponownie wydawane. Nie są globalnymi GUID między niezależnymi światami. Uchwyty zawierają slot i generację; usunięcie i ponowne użycie slotu unieważnia stary uchwyt.
- Jeden właściciel danych `WorkWorld` obsługuje wykonawców, partie i czynności zużywające materiał. To jeszcze nie pełna gospodarka ani naprawa konkretnego urządzenia.
- Komenda rezerwacji ma osobne ID. Identyczne ponowienie odtwarza pierwotny wynik, także odmowę. Zmieniony payload z tym samym ID jest błędem. Operacje rozpoczęcia i przerwania są idempotentne względem ID czynności i fazy.
- Rezerwacja nie zużywa części. Rozpoczęcie może zużyć zadaną część materiału; zakończenie po terminie zużywa resztę. Przerwanie nie odtwarza zużytych jednostek. Utrata wykonawcy lub partii przerywa zależne czynności i uwalnia pozostałe rezerwacje.
- Czas to jawne kroki dotychczasowego zegara. Upływ partii kroków przetwarza znane terminy w kolejności czasu i ID. Brak autonomicznego planowania.
- Snapshot przechowuje licznik ID, generacje, zasoby, rezerwacje poprzez czynności, historię komend, terminy, zegar i dziennik. Format ma wersję, limit rozmiaru oraz kontrolę integralności. Odczyt weryfikuje referencje i bilanse przed przyjęciem stanu.
- Zapis Windows korzysta z osobnego pliku tymczasowego w katalogu docelowym. Dopiero po zapisaniu i flush następuje podmiana. Nie obiecujemy odporności na awarię sprzętu ani równoczesne zapisy z wielu procesów.

## Zakres i odbiór

Pliki: rejestr w `src/core`, świat czynności w `src/simulation`, codec i I/O w `src/persistence`, demo konsolowe i testy. Weryfikacja: Debug, RelWithDebInfo, ASan, styl, kontrole dokumentacji; odrębny przegląd integratora.

Testy: recykling slotu; konkurencja o całą partię; identyczne i sprzeczne ponowienie; przerwanie przed i po zużyciu; usunięcie wykonawcy i partii; przepełnienie czasu; zakończenie najwyżej raz; wczytanie rezerwacji i pracy; wadliwa wersja, checksum i bilans; odmowa podmiany otwartego zapisu; kontynuacja w nowym procesie; seria deterministycznych operacji z kontrolą invariantów.

Początkowy zakres SC-01, SC-02, SC-03, SC-04 i SC-08 jest ograniczony do tego modelu. Nie obejmuje pieniędzy, produkcji nowych dóbr, transportu, wiedzy, narzędzi, fizyki ani grafiki. Limit prototypu: 4096 wpisów na kolekcję i snapshot do 4 MiB; wyczerpanie jest jawnym błędem, nie kasowaniem historii.
