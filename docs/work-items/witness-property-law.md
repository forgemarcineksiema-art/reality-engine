# Świadek, informacja i prawo

Zakres zlecony 27.10.2026: SC-05, INV-06, INV-07, INV-08 i INV-12. Przyrost nie kończy M04. Bez commitów.

## Projekt i wybór

Rozszerzamy istniejącą sesję, transfer, pamięć i kontakt F7. Alternatywa z kontrolą własności w WorkWorld blokowałaby fizyczną kradzież; globalna lista przestępców dla NPC łamałaby ograniczoną wiedzę. Wybrana reguła stanowiska wskazuje opiekuna, mieszkańców uprawnionych do pobrania i opcjonalnego kuriera (autoryzacja kuriera tylko podczas aktywnego Deliver z tego źródła). Jest to konfiguracja sceny, nie uprawnienie gracza do zmiany prawa.

Nieautoryzowane pobranie nadal wykonuje WorkWorld::transfer do rzeczywistego cargo. Incydent identyfikuje wspólne ID komendy transferu, sprawcę, stanowisko, partię, ilość i krok. Sesja posiada reguły i incydenty, WorkWorld zasób i bilans. Integralność nie ocenia prawa jako warunku poprawności transferu.

Świadek: inna osoba do 2 m od sprawcy, ta sama podpora, obustronny sweep kapsuły bez przeszkód (ten sam kontrakt co rozmowa). Zdarzenie obserwowane jest w momencie transferu; późniejsze podejście nie odkrywa sprawcy. Pamięć osoby zawiera kopię faktu oraz źródło, pośrednika i oba czasy. Osobista obserwacja otwiera ograniczone okno przekazu 40 kroków/co 10; przekaz nie uruchamia kolejnego automatu. F7 używa istniejącego API.

Deterministyczna reakcja: legalna obsługa stanowiska (Eat/Rest/Maintain) jest odmawiana sprawcy tylko jeśli jego opiekun zna niezakończony incydent. Samo pobranie cargo nigdy nie staje się niemożliwe przez prawo. Rozpoczęte czynności i ruch pozostają chronione. Nie dodajemy odmowy clearance: wystarcza jeden jawny skutek instytucji.

Fizyczne wpłaty z tej samej partii cargo do poszkodowanego stanowiska spłacają incydenty FIFO, do rzeczywiście oddanej ilości, bez podwójnego zaliczenia. Pełna spłata jest osobnym faktem obserwowalnym i przekazywalnym. Niepoinformowany opiekun zachowuje starsze przekonanie także po zwrocie poza jego zasięgiem; reakcja znika po lokalnej obserwacji zwrotu lub przekazie świadka. Zwrot zastępczego materiału tego samego typu z tej samej torby jest dopuszczalną restytucją; nie śledzimy atomów materiału po zmieszaniu.

Format 7 zapisuje reguły, incydenty i osobne świadectwa pobrania/uregulowania. Stare wersje 1–6 pozostają czytelne i bajtowo zachowane bez nowych danych. Walidacja łączy incydenty z przyjętymi transferami i rekonstruuje FIFO wpłat, sprawdza trwałe ID, ilości, czasy, duplikaty i osobiste źródło każdego przekazu. Odczyt tworzy kandydata i nie podmienia sesji przy błędzie. Inspektor odróżnia prawdę incydentu od pamięci osób i reakcji opiekuna.

## Plan wykonania i odbioru

1. `tests/law_tests.cpp`, CMakeLists.txt: cztery czerwone scenariusze, dodatkowo geometria, legalny kurier/mieszkaniec, relay/automat, wielokrotne naruszenia, błędne rekordy i stare wersje. Zachować czerwony build i przebieg przed logiką.
2. `src/lab/traversal_session.hpp`, nowy `traversal_law.cpp`, delivery/knowledge/needs/session: reguły sceny, fizyczny transfer, świadkowie, kontakt, lokalna reakcja i spłata.
3. `traversal_codec.cpp`: format 7, limity i transakcyjna walidacja; test kontynuacji po każdym kroku i korupcja z poprawną sumą kontrolną.
4. `src/platform/traversal_main.cpp`: scena `--law`, F4/F7 i próba rzeczywistego D3D11 z inspektorem; dotychczasowe sceny pozostają dostępne.
5. Dokumenty właścicielskie i raport: pełne Debug/Release/ASan, format-check, kontrola dokumentów, Python, MX330 z --gpu-debug i niezależny review.

Szczególnie sprawdzamy: brak świadka; ściana/inna podpora; opiekun niepoinformowany; brak odmładzania/automatycznego relay; brak zdalnego zamykania incydentu; brak wielokrotnego zaliczenia jednej wpłaty; dane z przyszłości i fałszywe źródło.
