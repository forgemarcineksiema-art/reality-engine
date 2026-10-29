# Koszt konstrukcji i odtwarzania uczenia

Status: naprawa zweryfikowana technicznie, 29.10.2026; baza f9fe6d7.

Cel: usunac wykazana przyczyne przekroczen czasu two_methods.fresh/native i practice.native. Zachowac wszystkie kroki, asercje, limity procesu 8/18 s i CTest 20 s. WorkWorld, ruch, wiedza, liczba modeli, kolejnosc kanoniczna, koszty badawcze i historyczne bajty pozostaja niezmienne.

Zakres po diagnozie: rzeczywisty goracy kod research/contrast_agent, hypothesis_program albo konsumenci PracticeState; testy roznicowe i regresje budzetow, raport oraz jeden plan. Bez nowej bramki M04 i bez zmiany protokolow badan.

Dowod: zachowane RED z f9fe6d7; profil zimnej konstrukcji/odtworzenia CPU/wall; porownanie wszystkich modeli/tablic/kosztow i starych checkpointow; niezmienione testy obu metod i praktyk; pelne szeregowe Debug/Release/ASan, format, Python/dokumentacja. Tymczasowa instrumentacja nie jest produkcyjna optymalizacja i zostanie usunieta przed pelnymi kontrolami.

## Wykonana diagnoza i zakres naprawy

Profil faz konstruktora wskazal powtarzana walidacje i przejscie po programie dla kazdego wejscia tablicy oraz alokacje kopii programu i ponowna alokacje przy jego wydluzeniu. Naprawa ogranicza sie do hypothesis_program.cpp; ContrastAgent, budzety, protokoly, formaty i gospodarka pozostaja bez zmian. Nowy test hypothesis.table_equivalence porownuje cala gramatyke z poprzednim generatorem i kazda tablice z niezmienionym skalarnym evaluatorem. RED stanowia zachowane przekroczenia historycznych limitow, nie nowy sztuczny prog czasu. Kontrola roznicowa przechodzila takze przed optymalizacja.

[Raport i wyniki](../verification/learning-cost.md).
