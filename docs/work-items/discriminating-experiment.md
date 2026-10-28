# Doświadczenie rozdzielające wyjaśnienia — protokół discrimination-v1

**Status: zakres wykonania zlecony przez użytkownika 28.10.2026; szczegóły poniżej są decyzjami autora zapisanymi przed nową próbą.** Audyt starej aparatury jest analizą skończonego języka, nie wynikiem nowego zbioru potwierdzającego. [Plan wykonania](discriminating-implementation.md) i późniejszy raport oddzielają implementację od wyników. Dawne protokoły i ich negatywne wyniki pozostają bez zmian.

## Cel i podejścia

Użytkownik zlecił audyt rozróżnialności, następny eksperyment z konkurencyjnymi wyjaśnieniami i samodzielnym wyborem interwencji, równe możliwości/budżety, oddzielną ocenę oraz zastosowanie i przekaz zdolności w jednym zadaniu mieszkańca.

Rozważono: więcej prób 1–64 (nie usuwa nierozróżnialności), nową fizykę i nowy język (zmienia zbyt wiele przyczyn naraz), oraz udostępnienie istniejącego zakresu urządzenia 1–128 obu wariantom. Wybór autora: trzecie podejście oraz pełne konkurencyjne modele od początku. Nadal jest to ograniczona identyfikacja i planowanie doświadczeń w dostarczonym języku, bez tezy o świadomości, odkryciu nowych operatorów albo fizyce poza aparatem.

## Audyt i granice

Wyliczyć F i gramatykę poprzedniej próby: 20440 zapisów gramatyki, 2370 różnych tabel 0–128. Każde prawo ma f(0)=0. Grupować po wynikach wszystkich wejść 1–64: 1397 grup, 150 wieloelementowych, 1123 modeli w tych grupach, największa 526, 139219 par. Przykład: loss48→fraction12 i loss48→cap16→fraction12 są identyczne na 1–64; pierwsza różnica przy 66, przy 96 wyniki 36 i 12.

Wyliczyć całe drzewo polityki minimaksowej na 1–128 dla wszystkich możliwych wyników: maksymalna głębokość 7, 2370 jednoelementowych liści, brak nierozstrzygniętych. To dowód osiągalności dla skończonego zbioru i tej polityki, nie dowód optymalności. Różnych składni o tej samej tabeli nie rozróżnimy żadnym dostępnym łańcuchem. Prawa poza językiem mogą zgadzać się na dowolnym skończonym zestawie pomiarów; zgodność modeli nie dowodzi prawdy ani zachowania poza 0–128. Nie dajemy agentowi drzewa audytu ani prawdy świata.

## Aparatura, warianty i koszty

Nowy konsument C++ używa wspólnych `DiscoveryWorld`, `TraversalSession`, `WorkWorld` i istniejących programów. Dawne API i formaty pozostają niezmienione. Dwa warianty **R** i **A** mają ten sam runtime generator F+gramatyka, kanonizację po całej tabeli 0–128, pomiary, ciało, komendy i budżety. R używa wspólnej permutacji 1–128, A minimalizuje największą liczbę pozostałych modeli po wyniku, remis mniejsze wejście. Po singletonie/pustym zbiorze/wyczerpaniu obliczeń oba używają permutacji. Każdy wykonuje dokładnie **8 różnych impulsów u**, od początku bez trzech narzuconych prób. Każdy dostaje limit 1024 eu, 1000 kroków, 10 mln zastosowań prymitywów, 300 mln odczytów modeli. Koszt generowania pełnej gramatyki liczymy obu. Faktyczne zużycie i czas CPU/ścienny/pamięć raportujemy oddzielnie; sufit nie oznacza identycznego zużycia.

Agent dostaje tylko pomiar z zakończonego impulsu i jego pochodzenie, nie seed, przypadek, tabelę prawdy ani zadania oceny. Zachowuje wszystkie zgodne klasy semantyczne. Prognoza: dolna mediana i min–max; pusty zbiór i wyczerpanie obliczeń jawne. Limity są trwałe, częściowe obliczenia nie stają się pewnym modelem. Ocena nie resetuje kosztów. Energia urządzenia 8192 eu, dwa kroki na konfigurację/impuls po 50 ms; bilans po każdym kroku.

Decyzje autora po builder-check: konfigurację wybieramy jak poprzednio, maksymalizując liczbę modeli trafiających w cel, potem minimalizując rozrzut, długość i leksykograficzny zapis. Zero trafień oznacza brak deklaracji. Odczyt modelu to jedno użycie elementu tabeli: filtrowanie pomiarem, każdy kandydat wejścia w selekcji oraz każde u w prognozie/łańcuchu. Znane v/w to prosta arytmetyka bez odczytu modelu. Wszystkie fazy i deklaracje obciążają ten sam licznik, także B po przekazie; odtwarzanie odbudowuje licznik, nie daje agentowi nowych obliczeń. Wygenerowanie tabeli obciąża licznik prymitywów o długość programu razy129 przed deduplikacją, dla wszystkich 20440 zapisów i dodatkowego F. Pusty program nie stosuje operatora. Przekroczenie limitu blokuje dalsze wnioskowanie trwale; pomiary do fizycznego limitu nadal wykonujemy.

## Niezależna ocena i zbiór

Osobny Python, bez natywnych bindingów, odtwarza programy, podziały modeli, wybory A/R, pomiary, prognozy i cele. Po każdym prefiksie 1–8 agent deklaruje wyniki u dla wszystkich 128 wejść, lecz **nie otrzymuje wyniku oceny**. Główny D liczymy tylko dla wejść jeszcze niezmierzonych w tym wariancie; D na wszystkich wejściach jest diagnostyką. Po 8 próbach zamrażamy wiedzę. Sędzia przygotowuje oddzielnie 20 dodatnich celów (wejście 1–128, konfiguracje zawierające u, długość 1–3); zadania ujawniamy dopiero po zamrożeniu. Losowanie ze zwracaniem, liczba różnych zadań ujawniona. Cel wybierany z wszystkich osiągalnych dodatnich par, nie z sukcesów agenta. Każda wybrana konfiguracja wykonywana fizycznie; brak deklaracji daje porażkę bez impulsu. Nie jest to ekstrapolacja poza dostępną dziedzinę pomiaru.

Generator świata jak poprzednio: długość 2/3 równomiernie, operacje niezależnie z 27 prymitywów. Odrzuca pełne tabele z F, przykład dydaktyczny poprzedniej próby, brak dodatniego zadania. W potwierdzeniu odrzuca także tabele przyjęte w rozwoju. Nie filtruje pod skuteczność A/R. Niezależne losowania ze zwracaniem, raport powtórzeń i odrzuceń. Limit 100000 kandydatów na seed. Nowe domeny RNG: 60 prawda, 61 permutacja prób, 62 zadania. Rozwój 8 seedów `0x44560000..07`, potwierdzenie 64 `0x44561000..3F`. Kontrola 4 praw kwadratowych floor(x*x/d), d=128/160/192/256, po cztery powtórzenia `0x44562000..0F`, osobno od głównego wyniku. Nie oczekujemy wykrycia każdej nieadekwatności języka.

Przed potwierdzeniem zamrozić źródła, binarium, protokół, audyt, plan oceny, narzędzia i ich SHA-256 oraz rzeczywistą konfigurację kompilacji. Wykonać rozwój, przegląd kodu i kontrole. Potwierdzenie raz w nowym katalogu; zachować marker startu, także przy porażce. Analiza ponowna dozwolona, ponawianie próby nie. Zmiana kryterium lub implementacji po rozpoczęciu nie poprawia starego wyniku: wymaga nowego protokołu.

## Hipotezy i wyniki negatywne

- **I**: na 64 światach A ma po 8 próbach jednoznaczny model w ≥90% światów, końcowy D ≥95% i G ≥90%. Wszystkie trzy warunki. Model prawdziwy musi pozostać w zbiorze na każdym prefiksie; to dodatkowa kontrola techniczna, nie kanał agenta.
- **A**: średnia D po prefiksach 1–8 jest u A większa niż u R o co najmniej 5 punktów procentowych i jednostronny dokładny test znaków po światach ma p≤0.025. Remisy usuwamy z mianownika. Nie używamy zadań z jednego świata jako niezależnych replik. Dla I raportujemy odsetki i zakresy po światach; brak wymogu przewagi A, jeśli oba wystarczają.
- **T**: co najmniej 90% światów pozwala nauczycielowi zakończyć rzeczywistą pracę, B z wiadomością także ≥90%, a przewaga B z wiadomością nad B bez wynosi ≥20 pp z jednostronnym testem znaków p≤0.025. Niespełnienie dowolnego warunku oznacza brak wsparcia dla pełnego T, nawet jeśli pokazano udane przykłady. Progi i testy osobne; nie ogłaszamy jednego wspólnego odkrycia statystycznego.

Raport obejmuje błędną jednogłośność, pokrycie min–max (nie przedział ufności), liczbę modeli, informacyjne wybory, czas osiągnięcia singletona, koszty i kontrole poza językiem. Wynik negatywny jest prawidłowym wynikiem badania.

## Jedno zadanie mieszkańca i przekaz

Autor wybiera **kalibrację napędu do pojedynczej czynności konserwacji stanowiska**. Jawna nowa reguła tego konsumenta: impuls o dokładnie wymaganym dodatnim wyjściu pozwala rozpocząć jedną istniejącą czynność Maintain na stanowisku przy aparacie. Zła energia nie rozpoczyna pracy ani nie zużywa części. To ustanowiona reguła sprzężenia, nie samorzutnie odkryte zastosowanie fizyki naszego świata. Metodą odkrytą przez mieszkańca jest dobór łańcucha z pomiarów do celu.

Każde stanowisko ma 4 fizyczne części typu 2 w WorkWorld. Udane uruchomienie zużywa jedną przez istniejącą rezerwację/czynność, trwa 80 kroków i kończy się rzeczywistym przyrostem ukończonych konserwacji. Używamy prawdziwych aktorów/ruchu/stanowisk; nie flagi sukcesu ani zdalnego odczytu zapasu do decyzji. Prawa i zadanie są identyczne dla nauczyciela i B. Pierwsza z 20 par celów jest zadaniem konserwacji.

Po uczeniu A, zamrożeniu i zadaniu nauczyciela tworzymy dwie jawne kopie kontrfaktyczne tego samego stanu: kontakt z wiadomością i kontakt bez wiadomości. Nauczyciel w obu naprawdę dochodzi do B; kontakt wymaga istniejącego `can_exchange_knowledge`, wspólnej podpory, dystansu i braku przeszkód. Wiadomość przenosi surowe 8 pomiarów z oryginalnym źródłem i czasem, via nauczyciel, nowy learned_tick. Nie przenosi prawdy ani odpowiedzi na zadania. B sam filtruje te same modele. W obu ramionach B wykonuje dokładnie 2 własne aktywnie wybrane pomiary, limit 256 eu, następnie zamraża wiedzę, wybiera łańcuch i wykonuje tę samą pracę. Nauczyciel po kontakcie wraca poza stanowisko B. Koszt nauczyciela, kontaktu, obliczeń B i własnych prób raportujemy w obu; brak przewagi całego systemu kosztowego nie jest ukrywany.

Nowa sesja ma osobną wersjonowaną otoczkę z dziennikiem operacji i transakcyjnym odtworzeniem; zawiera ruch, aparat, modele, źródła/czasy, liczniki, części, czynność i pełną permutację. Kontynuacja w świeżym procesie obejmuje konfigurację, impuls, kontakt i trwającą konserwację. Nie zmieniamy historycznych zapisów. Brak widoku 3D oznacza brak odbioru prezentacji/komfortu, nie brak bilansu headless.
