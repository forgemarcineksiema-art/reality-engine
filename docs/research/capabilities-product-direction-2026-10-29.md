# Genesis: propozycje kolejnych sprawdzalnych zdolności świata

**Status: propozycje badawcze z 29.10.2026, niezatwierdzone jako dalszy plan; korekta 02.11.2026.** Dokument odpowiada na prośbę o konkretny plan łączący autonomiczny rozwój świata, wiarygodność techniczną, odbiór na X i drogę do produktu. Opiera się na stanie kodu `82395a3` oraz raportach, nie na nowym wykonaniu ich testów.

Statusy M00–M06 pozostają u [jednego właściciela](../prototype-plan.md): pierwszy prototyp techniczny jest ukończony i odebrany w opisanym zakresie. Użytkownik 02.11.2026 wyjaśnił, że nie wybrał jeszcze najlepszego kolejnego kroku. Pełne 3D nie jest ustalonym projektem następnym. Poniższe warianty nie są backlogiem, nową bramką M06 ani zgodą na wdrażanie; granice zapisuje DEC-019 w [rejestrze decyzji](../decisions.md).

## Cel i sposób podejmowania decyzji

Rozważany w tej propozycji cel badawczy: mieszkaniec znajduje użyteczną konfigurację elementów świata, rzeczywiście z niej korzysta, a inny mieszkaniec uczy się jej używać. Zdolność pozostaje po odejściu odkrywcy. Kolejna próba sprawdza, czy ta zdolność umożliwia dalszy postęp.

W dalszej rozmowie użytkownik potwierdził, że wizja „praktycznie nieograniczonej rozgrywki i rozrywki” obejmuje także powstawanie nowych rodzajów zabawy, których twórca wcześniej nie zaprojektował. To rozszerzenie wizji badawczej i produktu, nie wynik eksperymentu. Wymagałoby rzeczywistego tworzenia, uzgadniania, wykonywania i przekazywania praktyk oraz sprawdzenia atrakcyjności dla człowieka. Porównanie z GTA VI nie potwierdza przewagi Genesis ani granic nieujawnionej symulacji Rockstar. Te możliwości nie stają się nowymi bramkami M06.

Autor dostarcza podłoże, reguły oddziaływań, percepcję i elementarne możliwości działania. Konkretna użyteczna konfiguracja ma być wynikiem poszukiwania. Podczas zamrożonej próby nie dopisujemy recepty, semantycznej czynności oznaczającej gotowe rozwiązanie ani wyjątku zmieniającego świat na podstawie nazwy konstrukcji. Granice przestrzeni konfiguracji opisujemy jawnie; skończona przestrzeń i skończony eksperyment nie dowodzą nieograniczonego rozwoju.

Każdy przyrost ma trzy oddzielne oceny: wynik w świecie, zrozumienie przez człowieka oraz zainteresowanie korzystaniem. Odbiór techniczny nie zastępuje dwóch pozostałych. Popularność filmu nie zamyka hipotezy badawczej.

## Jeden z rozważanych wariantów dalszej pracy

| Kolejność | Co robimy | Konkretny rezultat i warunek przejścia |
| --- | --- | --- |
| 1. Odebrany fundament | M06 wykonano jako odbiór techniczny, bez bramki uczestników i grywalności | [Raport M06](../verification/m06-acceptance.md) zachowuje dowody, ograniczenia i decyzję użytkownika |
| 2. Projekt podłoża dla nowej zdolności | Dopiero po wyborze tego kierunku rozważyć najmniejszy zakres fizyki i reprezentacji, w którym złożenie elementów zmienia ich rzeczywiste możliwości | Projekt z jednym właścicielem stanu, kontraktem percepcji/działania, testami i kosztem. Wybrać jeden mechanizm do pierwszej próby; nie przesądzać pełnego 3D ani zakresu innego przyszłego projektu |
| 3. Sprawdzić samą możliwość konstruowania | W małym, izolowanym laboratorium ręcznie złożyć użyteczny układ z tych samych elementów i przez te same komendy, które otrzyma agent | Rzeczywisty skutek wynika z geometrii i oddziaływań; zmiana układu zmienia skutek. Usunięcie kluczowego elementu odbiera funkcję. To kontrola podłoża, jeszcze nie dowód samodzielnego odkrycia |
| 4. Oddać poszukiwanie mieszkańcowi | Uruchomić poszukiwanie konfiguracji na podstawie lokalnych obserwacji i kosztownych prób. Porównać z prostszą strategią przy tym samym budżecie | Zamrożona ocena w nowych warunkach: wykonane działanie, koszt, skuteczność, porażki i informacja o wkładzie autora. Wyniku nie poprawiać przez zmianę zbioru po próbie |
| 5. Sprawdzić przekaz i kumulację | Drugi mieszkaniec uczy się przez dostępny kanał; odkrywca odchodzi. Osobno porównać dalsze możliwości świata z zachowaną zdolnością i bez niej | Oddzielne wyniki: przekaz działa; przekaz daje praktyczną korzyść; wcześniejsza zdolność umożliwia kolejny postęp. Uwzględnić koszt nauczyciela. Nie zakładać, że wszystkie trzy wyjdą dodatnio |
| 6. Przygotować demonstrator do sprawdzenia | Po potwierdzeniu mechanizmu udostępnić czytelny przebieg, inspektor przyczyn, zapis/wznowienie i uruchomienie przez inną osobę | Osoba spoza projektu rozumie, co zaszło, i potrafi odtworzyć opisany rezultat w zadeklarowanych warunkach. Główna demonstracja wykorzystuje rzeczywisty stan świata |
| 7. Sprawdzić odbiór i produkt | Najpierw mała zamknięta próba, następnie pakiet na X i demonstrator; rozwijać tę część doświadczenia, do której ludzie wracają | Zarejestrowane obserwacje, dobrowolne dalsze korzystanie i uzasadnienia zainteresowania. Dopiero wtedy decyzja o ofercie płatnej, cenie i zakresie |

To zależności jednego rozważanego wariantu, nie obowiązujący plan ani obietnica terminu. Wybór kierunku po ukończonym M06 pozostaje otwarty. Gdyby wybrano ten wariant, proponowane punkty oceny obejmują projekt podłoża oraz zamrożoną ocenę odkrywania. Na żadnym z nich sama liczba dodanych funkcji nie uzasadnia dalszej inwestycji.

## Zrealizowane przygotowanie i wykonanie M06

Przygotowanie opisano w [briefie M06](../work-items/m06-acceptance.md), a wykonanie i ograniczenia w [raporcie](../verification/m06-acceptance.md). Poniższa lista zachowuje historyczną logikę przygotowań; nie ustanawia otwartych zleceń ani wymogu próby z uczestnikami. Właścicielem statusu jest plan prototypu, metody pomiaru określają budżety. Następny kierunek wymaga odrębnej decyzji użytkownika.

1. Zapisać badany commit, konfigurację Release, rzeczywisty adapter, rozdzielczość i scenę. Po [audycie gotowości](../verification/m06-preflight.md) punktem wyjścia dla próby bezobsługowej jest scena `--community --learning`; `--two-methods` wymaga świadomego wyboru gracza i pozostaje osobną próbą interaktywną. Nie zakładać, że osobne tryby historii i podróży tworzą jeden zintegrowany przebieg.
2. Wykonać krótką kontrolę istniejącej diagnostyki: czy są dostępne czasy klatek, pamięć, zaległości kroku, koszt uczenia/nawigacji/zapisu i postęp mieszkańców. Dopisać tylko brak pomiaru koniecznego do M06. Dokładny zakres plików ustalić po tej kontroli; nie wymyślać obecnych liczników ani poleceń.
3. Zamrozić protokół przed długim uruchomieniem. Propozycja czasu to 2 minuty rozgrzewki i 30 minut pomiaru; 1280×720 na MX330 pozostaje celem wymagającym kontroli rzeczywistego widoku. Audyt wykazał potrzebę osobnego zachowania pierwszego uczenia, które może wydarzyć się podczas rozgrzewki, oraz oddzielenia przebiegu bezobsługowego od ingerencji i kontynuacji. Przed próbą określić dostępne sterowanie i oczekiwany postęp każdej fazy. Aktualny zakres przygotowania określa [brief M06](../work-items/m06-acceptance.md); nie zakładać gotowej automatyzacji wszystkich faz.
4. Zamrozić również zapasy i oczekiwany zakres działania. Skończona, przygotowana scena nie jest samowystarczalną gospodarką. Nie uzupełniać jej niewidocznie podczas próby. Sprawdzić ryzyko limitu historii i zatrzymania aktywności wskutek niedoborów; scena, w której wszyscy bezczynnie czekają, nie dowodzi kosztu aktywnej społeczności.
5. Rozliczyć wyniki wobec wcześniej zapisanych kryteriów. 60 FPS i miękkie 512 MiB pozostają propozycjami z właściciela budżetów; brief musi przed próbą ustalić ich dokładną interpretację i bramki odbioru. Zachować pełne rozkłady, przycięcia, moment zapisu i trend pamięci. Nie usuwać złych próbek ani nie podnosić historycznych limitów testów.
6. Zakres M06 skorygowano decyzją użytkownika: odbiór dotyczył fundamentu technicznego, bez uczestników i kryterium chęci dalszej gry. Wcześniejsza propozycja pięciu osób nie jest obowiązującym zadaniem.
7. Naprawiać potwierdzone problemy blokujące ten odbiór, z reprodukcją i porównaniem. Następnie wykonać wymagane regresje Debug/Release/ASan oraz kontrole narzędzi i dokumentacji według [strategii testów](../testing-strategy.md). Raport rozdziela wynik techniczny, odbiór ludzi i decyzję użytkownika.

Pakiety przygotowania, pomiaru, potwierdzonych napraw i raportu technicznego zostały wykonane w M06. Brak uczestników nie pozostawia otwartej bramki tego etapu. Dalszy wariant badawczy lub produktowy wymaga dopiero decyzji użytkownika; poniższa propozycja nie stanowi zlecenia.

## Rozważany eksperyment: konstrukcja uzyskuje funkcję

Rozważone drogi: rozpocząć od rozwoju ciał; najpierw rozbudować ogólną fizykę i świat 3D; zacząć od małego mechanizmu kompozycji, a następnie sprawdzić odkrywanie i transfer. Rekomendacja autora planu: trzecia droga jako jeden z wariantów wymagających wyboru, bez przypisania jej do zatwierdzonego projektu 3D. Najbliżej łączy dotychczasową pracę nad uczeniem z nowym problemem: powstawaniem użytecznych konfiguracji. Rozwój ciał pozostaje osobnym późniejszym badaniem, a sam rozrost silnika nie jest kryterium sukcesu.

Pierwszy projekt techniczny powinien rozważyć proste bryły, kontakty, podparcie i połączenia, których układ daje możliwość dostępu, podtrzymania lub przeniesienia rzeczywistego ładunku. Wybrać jeden rodzaj skutku do pierwszej próby. Nazwy typu most, schody czy dźwignia mogą opisywać wynik; nie mogą nadawać mu funkcji w kodzie. Zakres dynamiki, jednostki, stabilność i tolerancje wymagają jawnego modelu przed implementacją. Obecna kinematyka nie jest gotowym solverem takich konstrukcji.

Agent ma otrzymać elementarne operacje i obserwacje, a nie katalog gotowych budowli. Każda próba ma rzeczywisty koszt i skutek. Badamy osiągalność nowych stanów świata; sam nowy kształt albo wpis w pamięci nie wystarcza.

Przed implementacją zamrożonej oceny przygotować osobny protokół według [szablonu eksperymentu](../templates/experiment-record.md):

- Opisać, co jest nowe względem stanu początkowego i wiedzy agenta, a co dostarczył autor lub wcześniej wytrenowany model.
- Oddzielić pilotaż od zbioru potwierdzającego; ustalić liczby prób, generatory warunków, seedy, progi, koszty i reguły zakończenia przed oceną. Agent nie otrzymuje seedów ani ukrytych parametrów świata.
- Porównać poszukiwanie z prostą strategią przy tym samym dostępie do obserwacji, elementarnych działań i budżecie. Ręczna konstrukcja sprawdza wykonalność, nie jest konkurentem o równych informacjach.
- Sprawdzić kontrfakty: bez kluczowego elementu; w zmienionej geometrii/obciążeniu; bez pamięci doświadczeń; bez przekazu. Zmianę jednego mechanizmu oddzielić od zmiany fizyki i wyposażenia.
- Osobno mierzyć działający transfer i jego korzyść względem samodzielnego odkrywania. Całkowity koszt obejmuje nauczyciela oraz ucznia.
- Utrwalić wszystkie próby, także nieudane; zweryfikować kontynuację po zapisie i odczycie. Kod oceny obserwuje stan świata, bez przyznawania sukcesu na podstawie wypowiedzi modelu.

Maksymalnie dwa ograniczone warianty mechanizmu w jednym cyklu badawczym to proponowany limit organizacyjny. Po cyklu następuje decyzja: kontynuować, zmienić hipotezę albo zatrzymać kierunek. Jeżeli podłoże wymaga dopisywania konkretnej recepty po każdym niepowodzeniu albo poszukiwanie wykorzystuje błąd modelu, nie rozszerzamy demonstracji. Jeżeli prostsza strategia działa równie dobrze, zachowujemy ten wynik i nie ogłaszamy przewagi uczenia.

Kumulację badamy dopiero po działającej pojedynczej zdolności: ta sama rodzina reguł ma pozwolić, żeby wcześniejsze odkrycie udostępniło zasób lub działanie potrzebne do dalszego postępu. Sam napis odblokowujący technologię nie jest dowodem. Przebieg porównawczy musi sprawdzać ten związek, a nie tylko kolejność zdarzeń.

## Wykorzystanie modeli AI i przegląd rynku

Podział pracy: jeden model przygotowuje rozwiązanie na podstawie zatwierdzonego briefu; drugi szuka kontrprzykładów i ukrytych założeń; wyniki rozstrzyga kod, rzeczywisty stan świata i odpowiedni odbiór człowieka. Role dobieramy według sprawdzonych wyników, bez stałego przypisywania producenta do jednej specjalności. Nowy model najpierw rozwiązuje małe zadanie porównawcze.

Raz w tygodniu oraz przed wyborem kosztownej architektury przeprowadzić przegląd oficjalnych zmian OpenAI, Anthropic i Google. Zapisywać: co jest dostępne dla użytkownika, w jakim produkcie, z jakimi ograniczeniami oraz który problem Genesis może to zmienić. Oczekiwane przyszłe modele pozostają oczekiwaniami do czasu potwierdzenia dostępu. To proponowany rytm pracy, nie uruchomione automatyczne monitorowanie.

Paczka porównawcza modeli: diagnoza zachowanej regresji; rozwiązanie jednego ograniczonego problemu kodu z testami; zaprojektowanie rozróżniającego doświadczenia przy ograniczonych obserwacjach; krytyka twierdzenia o emergencji. Porównywać wykonany wynik, czas, zużycie dostępnych limitów i pomoc człowieka. Do próby nie potrzebujemy od razu wbudowanego API w mieszkańcach.

Dostęp do narzędzi tworzenia kodu i dokumentów oraz koszt uruchomienia świata u klienta to dwa różne budżety. Ewentualny mieszkaniec sterowany LLM wymaga osobnej próby: ten sam kanał obserwacji i działania, jawna wcześniejsza wiedza, opóźnienie, koszty oraz zachowanie przy braku usługi. Abonamenty użytkownika nie dowodzą dostępu ani finansowania API produktu.

## Pokaz, wiarygodność i produkt

Już po M06 można przygotować materiał o faktycznie działającym ograniczonym uczeniu. Demonstrację odkrywania konstrukcji przygotować dopiero po odpowiednim eksperymencie. Materiały rozróżniają te dwa osiągnięcia.

Pakiet przyszłego mocnego pokazu: krótki film pokazujący problem, działającą konfigurację i skutek; dłuższy przebieg z przyczynami oraz oznaczonym przyspieszeniem czasu; uruchamialny eksperyment z konfiguracją i wynikami wszystkich prób. Ujawnić wkład autora, gotowej wiedzy modeli i wybór efektownego przebiegu. Przed szerokim pokazem uzyskać zewnętrzną próbę odtworzenia oraz krytyczne uwagi techniczne. Samo udostępnienie kodu nie jest niezależnym potwierdzeniem nowości naukowej.

Mały test komunikacji: pięciu odbiorców ogląda film bez komentarza autora i własnymi słowami opisuje, co się wydarzyło oraz co ich zdaniem było przygotowane. Jeżeli przypisują światu zdolności, których nie ma, poprawić przekaz. Jeżeli nie widzą skutku, poprawić czytelność prezentacji. Taki test sprawdza zrozumienie, nie przewiduje viralowości.

Hipoteza pierwszego produktu: małe laboratorium świata na PC, w którym użytkownik uruchamia różne warunki, obserwuje, interweniuje, zapisuje i wraca do własnej historii. Interfejs korzysta ze wspólnego stanu świata. To propozycja sposobu udostępnienia silnika; nie zastępuje zatwierdzonej wizji fantasy/Action RPG i nie zatwierdza osobnego produktu komercyjnego.

Po zrozumiałej demonstracji przygotować zamkniętą próbę z 10–15 chętnymi osobami. To propozycja małego badania jakościowego, nie reprezentatywna prognoza sprzedaży. Zapisywać: samodzielne uruchomienie, zrozumienie skutku, własną interwencję, dobrowolny powrót i konkretny powód zainteresowania. Wyświetlenia, udostępnienia, pytania techniczne, uruchomienia i powroty raportować oddzielnie. Przed udostępnieniem określić sposób zbierania obserwacji i zgody; nie zakładać istniejącej telemetrii.

Gdy zainteresowanie dotyczy głównie oglądania, sprawdzić wartość obserwacji i dzielenia się przebiegami. Gdy ludzie chcą samodzielnie eksperymentować, rozwijać sterowanie, inspektor i przygotowanie warunków. Gdy nie wracają, zbadać przyczynę przed dodaniem skali. Oferta płatna wymaga użytecznej obecnej wersji oraz osobnej decyzji o wartości i cenie. Rekrutacja, wiadomości, posty, publikacja i sprzedaż są późniejszymi działaniami; przygotowanie tego planu ich nie wykonuje.

## Punkty odniesienia sprawdzone 29.10.2026

- [Genie 4, Google DeepMind](https://deepmind.google/models/genie/): interaktywne generowanie światów; producent opisuje m.in. ograniczenia czasu interakcji, trwałości obiektów i złożonych interakcji agentów. To punkt porównania i wskazówka do pytania badawczego, bez dowodu przewagi Genesis.
- [Evolved Virtual Creatures, Karl Sims](https://www.karlsims.com/evolved-virtual-creatures.html): ewolucja budowy i sterowania w zadaniach ruchowych była demonstrowana w 1994 roku. Powstające ciała wymagają porównania z wcześniejszą pracą.
- [Open-Ended Evolution, ISAL](https://alife.org/encyclopedia/introduction/open-ended-evolution/): pomiary muszą odróżniać istotną trwałą nowość od szumu; przebiegi kontrolne pomagają w interpretacji. Pojedynczy ciekawy wynik nie wystarcza do szerokiej tezy o otwartej ewolucji.
- [Steamworks, Early Access](https://partner.steamgames.com/doc/store/earlyaccess): podstawą oferty ma być użyteczna obecna wersja. Drogi do produktu nie opieramy wyłącznie na obietnicy przyszłych zdolności.

Dalszy projekt techniczny wymaga tematycznego przeglądu wcześniejszych prac i porównywalnych rozwiązań. Ta krótka lista nie jest wyczerpującym audytem nowości.
