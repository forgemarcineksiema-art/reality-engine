# Koszt tworzenia i odtwarzania umyslow

**Status: naprawa zweryfikowana technicznie, 29.10.2026.** Baza f9fe6d7, [brief](../work-items/learning-cost.md). Przyrost usuwa wykazany koszt CPU, zachowujac historyczne limity, kroki i asercje. Nie zmienia wynikow badan, kosztow logicznych ani protokolow.

## Przyczyna i pomiar

Poprzednie pelne Debug/ASan mialy powtarzalne timeouty two_methods.fresh_repair/native_repair/fresh_detour/native_detour, a czesc przebiegow takze practice.native. Release raz przekroczyl limit tooling.tests; pozniejsza oddzielna proba 58/58 przeszla. Porażki pozostaja w [raporcie kuriera](m05-distant-delivery.md), bez przypisywania wszystkim timeoutom jednej udowodnionej przyczyny.

Nowy profil zimnego procesu rozdzielil kazdy z osmiu konstruktorow ContrastAgent: generowanie gramatyki okolo 49 ms, tablice 82 ms, deduplikacja 42 ms. Pelna konstrukcja swiata okolo 1,57 s. Te pomiary sa z nowej sesji; nie porownujemy ich bezposrednio jako przyspieszenia ze starszym 3,75 s w innych warunkach.

hypothesis_table dla kazdego z 129 wejsc powtarzal walidacje i iteracje programu przez kontrolowane iteratory wektora. Teraz zachowuje walidacje scalar hypothesis_apply(program,0), po czym stosuje kazda operacje do ograniczonej tablicy wejsc. Zakres i arytmetyka pozostaja te same. Zmierzony koszt tablic spadl z okolo 82 do 23 ms na umysl; pelna konstrukcja do okolo 1,09 s.

Generator kopiowal wektor programu z pojemnoscia rowna dlugosci, po czym dopisanie operacji ponownie go alokowalo. Teraz rezerwuje miejsce na cala skonczona gramatyke i alokuje kazdy program raz w docelowej dlugosci, kopiujac jego prefiks. Porzadek i 20440 programow pozostaja identyczne. Koszt gramatyki spadl do okolo 21 ms na umysl, konstrukcja swiata do 0,92 s. Dodatkowe scalanie krotkiego F moze teraz realokowac kontener o dokladnej pojemnosci; pomiar obejmuje takze ten koszt, bez ukrywania go. Deduplikacji nie zmieniono. Nie ma wspolnego cache wiedzy/modeli ani pomijania konstrukcji za plecami budzetu.

## Dowod zgodnosci

Nowy test hypothesis.table_equivalence porownuje wszystkie programy i ich kolejnosc z poprzednim generatorem, wszystkie wyjscia 0..128 calej gramatyki, F i pelnych zakresow pojedynczych operatorow z niezmienionym hypothesis_apply. Sprawdza takze odmowe zlej dlugosci, parametrow i operatora. Istniejacy contrast.model_order zachowuje 2370 konkurentow, reprezentantow kanonicznych, budzety 0/129 i dokladne liczniki 7822044 prymitywow, 20440 wygenerowanych programow i zero odczytow. Oba testy przechodzily przed i po zmianie; RED jest rzeczywista wczesniejsza porazka testow czasowych, nie roznica semantyki ani nowy zmienny test czasu.

Po kazdym z dwoch krokow optymalizacji zimny checkpoint naprawy i pelny wynik referencyjny maja identyczne SHA-256 jak przed zmiana. Tymczasowa instrumentacja chrono zostala zachowana w dowodach i usunieta z kodu produkcyjnego przed koncowymi buildami.

## Kontrole koncowe

Zrodla zamrozono po usunieciu instrumentacji. Wszystkie buildy i zestawy wykonywano szeregowo, bez rownoleglego obciazania laptopa kompilacja i testami.

| Komenda | Wynik |
| --- | --- |
| cmake --preset windows-msvc; cmake --build --preset windows-debug; ctest --preset windows-debug | 382/382 PASS, 201,24 s |
| cmake --build --preset windows-release; ctest --preset windows-release | 382/382 PASS, 61,79 s |
| cmake --preset windows-asan; build i ctest windows-asan | 384/384 PASS, 302,13 s |
| cmake --build --preset windows-debug --target format-check | PASS |
| python tools/check_docs.py | PASS |
| python -m unittest discover -s tools/tests -p "test_*.py" | 58/58 PASS, 7,200 s |

Wczesniejszy selektywny Debug: 10/10 PASS, 42,04 s. Pelny Debug: native_repair 6,47 s, native_detour 7,28 s, practice.native 7,44 s. Pelny ASan: native_repair 10,70 s, native_detour 10,68 s, practice.native 9,40 s przy niezmienionym limicie 18 s. Kontynuacje obu metod w ASan trwaly 9,02/9,92 s na CALY przypadek z dwoma procesami; kazdy proces nadal ma limit 8 s, a CTest 20 s. Zaden test nie zostal rozdzielony, skrocony, pominiety ani oslabiony.

Osobny koncowy zimny proces Debug bez instrumentacji: 3,214 s wall, 3,156 s CPU; konstrukcja po 1,028 s, pelny zapis referencyjny po 3,177 s. Nie przypisujemy zmianie calej roznicy miedzy starszymi i aktualnymi czasami zestawow: warunki laptopa sa zmienne. Wykazano spadek konkretnych kosztow faz i przejscie niezmienionych bramek. Wczesniejszemu pojedynczemu timeoutowi tooling.tests nie przypisujemy niewykazanej przyczyny; narzedzi Pythona nie zmieniono, teraz przeszly we wszystkich trzech pelnych profilach i osobno.

Niezalezny przeglad swiezego kontekstu: APPROVE, brak Critical/Important. Recenzent odczytal arytmetyke, indeksowanie, walidacje, kolejnosc i budzety; nie uruchamial kompilacji/testow rownolegle z pomiarami. Kontrole uruchomieniowe nalezaly do integratora.

## Kontynuacja i dowody

Po koncowym buildzie Debug odtworzono prawdziwy checkpoint naprawy sprzed optymalizacji z f9fe6d7 poleceniem reality_two_methods_tests.exe continue repair. Pelne koncowe bajty sa identyczne ze stara referencja. Takze nowy checkpoint i nowa referencja sa identyczne z poczatkowa proba tej sesji. Wyniki SHA-256, wejscia i wyjscia pozostaja w dowodach. Starsze formaty oraz pozostale scenariusze obejmuja pelne zestawy.

[Manifest SHA-256](evidence/learning-cost-2026-10-29/manifest.json) i [archiwum](evidence/learning-cost-2026-10-29/runs.zip) zachowuja stare porazki, profile kolejnych pojedynczych zmian, tymczasowa instrumentacje, oryginaly zmienionego kodu, zrodla koncowe, checkpointy, log review i pelne kontrole. Pakowanie sprawdza CRC oraz kazdy rozmiar i SHA-256. Manifest zawiera hashe uzytych programow trzech profili; binaria nie sa kopiowane, archiwalny checkout nie byl budowany ponownie. Oryginaly pozostaja w build/learning-cost i build/distant-final.

To zamyka bramke techniczna ograniczonego kursu kuriera. Nie stanowi odbioru tego kursu przez uzytkownika, zakonczenia calego M05/M06 ani gwarancji czasu przy dowolnym obciazeniu komputera.
