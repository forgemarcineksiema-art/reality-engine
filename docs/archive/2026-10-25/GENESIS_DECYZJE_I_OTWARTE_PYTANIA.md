# Genesis — rejestr decyzji i pytań podstawowych

25 października 2026 r. Stan po odpowiedziach użytkownika na trzy pytania z audytu.

Dokument rozdziela intencje użytkownika, propozycje projektowe i parametry wymagające doświadczeń. Sam zapis propozycji w dokumentacji nie jest jej zatwierdzeniem.

## 1. Ustalone kierunki

- Gra łączy otwarty świat, symulację społeczeństwa i Action RPG.
- Ważne są trwałość, pamięć, relacje, plany i skutki materialne oraz społeczne.
- Historie autorskie współistnieją z sytuacjami emergentnymi.
- Agenci działają w rzeczywistych ograniczeniach systemów gry.
- Projekt korzysta z własnego silnika; Unreal Engine został odrzucony.
- Pierwszy sukces łączy grywalny fragment, własny silnik i laboratorium małej autonomicznej cywilizacji. Grafika ma najmniejszy priorytet.
- Własna implementacja obejmuje również fizykę i nawigację. Biblioteki zewnętrzne ograniczamy do minimum.
- Projekt rozwija użytkownik samodzielnie z pomocą swoich agentów AI. Nie ma narzuconego terminu; kolejne etapy rozstrzygamy na podstawie działania, poprawności i pomiarów.
- Sprzętem odniesienia jest laptop użytkownika: i5, MX330 i 8 GB RAM. Bieżący odczyt potwierdził i5-1035G1 (4 rdzenie, 8 wątków), około 7,78 GiB pamięci raportowanej przez system oraz obecność Intel UHD i MX330.

Docelowa szczegółowość, wielkość populacji i poziom samodzielnego rozwoju cywilizacji pozostają hipotezami do sprawdzenia. Nie potwierdzono wykonalności całego zakresu naraz.

## 2. Odpowiedzi użytkownika

| Pytanie | Dlaczego jest teraz istotne | Status |
| --- | --- | --- |
| Co ma być pierwszym dużym sukcesem: grywalny fragment, laboratorium cywilizacji czy zaawansowany silnik 3D? | Ustala kolejność i sposób rozstrzygania konfliktów zakresu | Grywalny fragment i laboratorium na własnym silniku; mała autonomiczna cywilizacja i prosta grafika |
| Czy własny silnik może wykorzystywać biblioteki fizyki i nawigacji? | Wyznacza zakres prac fundamentowych i potrzebne kompetencje | Własna fizyka i nawigacja; minimum bibliotek |
| Jakie są dostępne osoby, czas pracy i komputer pierwszego prototypu? | Umożliwia dobór realnego zakresu i pomiarów | Użytkownik i jego agenci AI; bez narzuconego terminu; i5, MX330, 8 GB RAM |

Brak terminu nie oznacza nieograniczonego zakresu pojedynczego etapu, pamięci ani kosztu obliczeń. Agenci pomagają w implementacji i weryfikacji, a działający build oraz test pozostają dowodem wykonania.

## 3. Robocze propozycje wymagające rozstrzygnięcia

| Obszar | Obecna propozycja | Co zależy od decyzji |
| --- | --- | --- |
| Tożsamość gry | Osobista przygoda z możliwością wpływu na organizacje i miasta | Główne czynności, kamera, UI i produkcja scen |
| Świat | Wolne Dorzecze i alternatywna bliska przyszłość | Technologia świata, broń, transport, styl i stawki fabuły |
| Postać | Jeden fizycznie obecny bohater; później opcjonalny następca | Porażka, śmierć, wiedza gracza i ciągłość relacji |
| Tryb | Pierwsza wersja dla jednego gracza, lokalna | Autorytet danych, infrastruktura sieciowa i testy |
| Platforma | Roboczo Windows, C++20 i prosty renderer Direct3D 11; D3D12 odłożone | Backend GPU, wymagania i zakres przenośności |
| AI generatywna | Opcjonalna warstwa z lokalnym wariantem zastępczym | Koszt, sprzęt, dystrybucja i dostępność funkcji |
| Czas | Dokładna przygoda i dobrowolne dłuższe przeskoki | Produkcja, podróże, starzenie i tempo historii |
| Prototyp | Propozycja: najpierw 12–16 osób; po pomiarach około 60; most, transport i zakład | Dane scenariusza i katalog pierwszych czynności |

Wskazanie tych propozycji nie unieważnia wizji. Pozwala odróżnić własności docelowe od decyzji, które można jeszcze zmienić bez kosztownej przebudowy.

## 4. Zasadnicze luki wymagające projektu

### Hierarchia priorytetów

Użytkownik wybrał wspólny grywalny i laboratoryjny rezultat; grafika jest ostatnia. Robocze rozstrzygnięcie inżynierskie: najpierw poprawność i trwałość, następnie sterowalny fragment z autonomią, potem skala i szczegółowość, na końcu rozbudowa wyglądu. Czytelność obiektów, działań i informacji pozostaje wymogiem również przy prostej grafice.

### Katalog możliwości świata

Hasło „bezprecedensowa swoboda” wymaga skończonego, jawnego zestawu czynności, właściwości i sposobów ich łączenia. Należy określić, co oznacza możliwość wejścia do budynku, przeniesienia przedmiotu, złożenia obietnicy, zatrudnienia osoby lub zmiany własności. Zestaw ten wyznacza wspólny zakres fizyki, AI, animacji i autorstwa.

### Przemoc i trwała populacja

Typowy rytm częstych śmiertelnych starć może usuwać uczestników gospodarki i historii szybciej, niż świat potrafi się odtworzyć. Trzeba ustalić częstość takich sytuacji, rolę poddania i wycofania, leczenia, zastępowania funkcji oraz pochodzenie nowych mieszkańców. Przywracanie konkretnej osoby bez wyjaśnienia naruszałoby obietnicę trwałości.

### Czas i ciągłość doświadczenia

Animacja, podróż, doba, produkcja i starzenie działają w różnych skalach. Wybór współczynników zmienia grę: zbyt szybki czas odbiera znaczenie relacjom, zbyt wolny utrudnia obserwowanie przemian społecznych. Potrzebny jest kontrakt czasowy oraz zasady przyspieszania i przerywania oczekiwania.

### Odporność świata i miejsce gracza

Należy zdecydować, ile problemów mieszkańcy potrafią rozwiązać samodzielnie oraz jakie porażki mogą zajść poza uwagą gracza. Autonomia wymaga własnych działań świata, a czytelna przygoda wymaga informacji, czasu na reakcję i przestrzeni na spokojną eksplorację.

### Granice cywilizacji

Import, migracja i kontakty z otoczeniem wpływają na wszystkie bilanse. Potrzebujemy jawnych granic modelu: skąd przybywają ludzie, dobra i wiedza oraz co dzieje się po opuszczeniu regionu. Otoczenie nie powinno być ukrytym źródłem dowolnego ratunku albo przeciwników.

### Tworzenie i odnawianie treści

Trwała zmiana miasta potrzebuje projektów remontu, układów wnętrz, animacji, dokumentów, dialogów i wyglądu kolejnych stanów. Należy zmierzyć koszt stworzenia jednego interaktywnego miejsca i możliwości jego ponownego użycia. Bogaty model danych nie zapewnia automatycznie dostatecznej liczby wartościowych treści.

### Warunek sukcesu pierwszego etapu

Wynik techniczny i wynik gry wymagają osobnych kryteriów. Poprawne bilanse dowodzą poprawności modelu, a zainteresowanie gracza wymaga obserwacji rozgrywki. Dopiero po ustaleniu priorytetu można zdefiniować, jaki dowód uzasadnia zwiększenie inwestycji.

## 5. Czego nie trzeba teraz zamykać

Nie musimy wybierać ostatecznej powierzchni mapy, liczby wszystkich mieszkańców, pełnej listy profesji ani każdego algorytmu. Te decyzje powinny wynikać z potwierdzonej pętli rozgrywki, możliwości produkcji treści i pomiarów.

Pierwszy cel, granica własnego kodu i zaplecze pracy zostały ustalone. Nadal robocze pozostają m.in. świat fabularny, perspektywa, tempo czasu i dokładny zakres pierwszych interakcji. Można je sprawdzać małymi eksperymentami bez ponownego otwierania decyzji o własnym silniku, fizyce i nawigacji.
