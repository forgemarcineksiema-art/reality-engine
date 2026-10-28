# Doświadczenia rozdzielające — stan wykonania

**Status: NIEUKOŃCZONE.** Audyt wykonany, agent i fizyczna sesja zaimplementowane, pierwsza próba rozwojowa wykonana. Próby potwierdzającej nie rozpoczęto. [Protokół](../work-items/discriminating-experiment.md) pozostaje właścicielem kryteriów; [plan](../work-items/discriminating-implementation.md) określa pozostały zakres. Ten raport nie zastępuje końcowego raportu badania.

## Ustalony wynik audytu

2370 klas semantycznych, 1397 klas obserwacyjnych na 1–64, 150 grup wieloelementowych z 1123 modelami; największa grupa526. W sumie139219 par nierozróżnialnych w poprzednim zakresie. Pełne drzewo minimaksowe na 1–128 ma 2370 jednoelementowych liści i maksymalną głębokość7. Równoważne składnie i nieznane prawa poza skończoną dziedziną pozostają nierozstrzygalne.

## Implementacja i rzeczywiste sprawdzenia

`ContrastAgent` generuje pełne F+gramatykę przed pierwszym pomiarem. `ContrastSession` używa fizycznego aparatu, dwóch mieszkańców, istniejącej nawigacji i dwóch stanowisk Maintain z czterema częściami typu2 każde. Udana kalibracja rozpoczyna istniejącą czynność80 kroków, zużywając jedną część w WorkWorld. Źródło i czas pomiarów pozostają przy wiadomości. Nowa otoczka `GENESIS_CONTRAST 1` odtwarza dziennik transakcyjnie; stare formaty bez zmian.

Wykonano `cmake --build --preset windows-debug --target reality_contrast reality_contrast_tests` oraz `ctest --preset windows-debug -R '^contrast[.]' --output-on-failure`: **7/7 PASS, 22.55 s**. Obejmuje cztery kontynuacje w nowych procesach: konfiguracja, impuls, trwająca konserwacja i stan po kontakcie. Testy odrzucają zdalny przekaz, błędną kalibrację i uszkodzony zapis. Regresje po review wykazały i usunęły przyjęcie impulsu bez energii oraz brak rezerwy czasu/dziennika na kończenie operacji. Zachowano rzeczywiste RED i GREEN. Pierwszy RED nowego targetu był błędem linkowania brakujących implementacji, a nie wykonaną regresją runtime.

`python -B -m unittest discover -s tools/tests -p test_contrast_experiment.py`: **3/3 PASS**. Osobno audyt2/2. Sprawdzono odrzucanie zmienionej prognozy, wyboru, pomiaru, części i energii. `python tools/check_docs.py`: PASS. Nie wykonano jeszcze pełnych końcowych profili Debug/Release/ASan ani pełnej kontroli formatu po ostatnich zmianach. Nie ma odbioru 3D ani oceny człowieka.

## Pierwsza próba rozwojowa — wynik historyczny

`build/contrast-development-01` zawiera 8 światów rozwojowych i16 kontroli spoza języka. Kopia danych jest w [archiwum](evidence/discriminating-development-2026-10-28/development-initial.zip). Rozwój użył wcześniejszego binarium, przed poprawkami granic sesji i zapisem dokładnego czasu kontaktu. Nie wolno opisywać go jako walidacji finalnej implementacji. Źródła natywne nieśledzone w chwili freeze nie weszły do tego pierwszego pakietu — to wykryta wada archiwizacji, nie kompletne odtwarzalne zamrożenie.

Na8 światach A: końcowy D100%, G100%, singleton8/8. Przewaga średniej D po prefiksach nad R: `26067156280469/320253394137600` (około8.1395 pp), test znaków7 wygranych/1 przegrana, p9/256. Rozwojowe I spełnione, A niespełnione przez próg p. Nauczyciel wykonał konserwację8/8, B z wiadomością8/8, B bez wiadomości również8/8. T niespełnione. To brak dowodu przydatności przekazu dla tej populacji zadań, mimo działającego przekazania i pracy. Nie zmieniono celów ani progów pod ten wynik. Kontrole poza językiem są zachowane, ale ich pełny raport diagnostyczny pozostaje do wykonania.

## Pozostała praca i review

Niezależny przegląd kodu: **REQUEST CHANGES**. Zamknięto regresjami brak energii oraz rezerwę czasu/dziennika, sędzia porównuje już sumy energii. Dodano dokładny czas kontaktu do sesji i wyjścia natywnego, ale sędzia wciąż dopuszcza starsze rekordy bez tego pola; wymagane jest rozdzielenie historycznego czytnika od ścisłego nowego formatu i test podmienionego learned_tick. Fizyczny kontakt sprawdza natywna sesja i jej testy; sam sędzia nie odtwarza geometrii.

Przed potwierdzeniem trzeba jeszcze: uszczelnić kompletny freeze źródeł i konfiguracji, dodać CPU/pamięć oraz pełne diagnostyki (zakresy, błędna jednomyślność, koszty nauczyciela/kontaktu), ukończyć testy runnera i zapis źródeł, ponowić rozwój finalnej implementacji w nowym katalogu, domknąć review oraz pełne kontrole. Dopiero wtedy jednorazowa próba64 światów+16 kontroli i przenośne archiwum z manifestem. Runner celowo odmawia obecnie `prepare --mode confirmatory`. Nie wdrożono jeszcze pełnej bramki gotowości, więc nie należy usuwać tej odmowy bez wykonania tych prac.
