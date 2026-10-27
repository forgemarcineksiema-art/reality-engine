# Wielorazowa pojemność stanowiska

Status: wykonano i zweryfikowano ograniczony zakres; [raport odbioru](../verification/reusable-capacity.md). Kontynuacja fundamentu przed pełnym M04.

Cel: odpoczynek zajmuje miejsce, nie zużywa go jak materiał. `WorkWorld` otrzymuje niezmienny rodzaj zasobu: materiał albo pojemność. Aktywna czynność rezerwuje wymaganą pojemność do zakończenia lub anulowania; nie może jej zużywać. Zniszczenie obiektu i usunięcie wykonawcy przerywają czynności istniejącą ścieżką. Bez osobnego mutowalnego licznika zajętości, replenishmentu i obejścia bilansów. Jedna czynność nadal używa jednego zasobu; jednoczesne wymaganie materiału i stanowiska pozostaje poza tym przyrostem.

Zakres: domena `WorkWorld`, jej codec, konsument odpoczynku w `TraversalSession`, testy domeny i sesji oraz aktualna dokumentacja. Chronione kontrakty: INV-03, INV-04, INV-05, INV-06, INV-10, INV-12 i INV-16. API tworzenia zasobu zachowuje domyślny rodzaj materiału dla obecnych konsumentów. Rodzaj nie jest zmieniany po utworzeniu.

Format świata z pojemnością otrzymuje wersję 2 z dodatkowym polem rodzaju zasobu. Światy wyłącznie materiałowe nadal zapisujemy jako wersję 1, bez zmiany bajtów. Czytamy obie wersje; w wersji 1 wszystkie zasoby pozostają materiałami. Nie reinterpretujemy historycznych czynności odpoczynku ani nie odtwarzamy zużytego historycznego zasobu. Zewnętrzny rekord sesji traversal zachowuje swój format i wersjonowany zagnieżdżony świat. Nie zmieniamy zapisów starego M03.

Dowód: reprodukcja zużywania miejsca odpoczynku przed poprawką; dwie postacie korzystające kolejno z jednego miejsca; blokada drugiej rezerwacji podczas zajęcia; zwolnienie po zakończeniu, anulowaniu, usunięciu wykonawcy/obiektu; ponowienia komend; zero zdarzeń zużycia pojemności; zapis w toku i ponowne wykorzystanie po odczycie; odrzucenie semantycznie uszkodzonej wersji 2 przy poprawnej sumie; kompatybilność wersji 1. Pełne Debug, RelWithDebInfo, ASan, diagnostyka GPU, styl i dokumentacja. Nie oznacza odbioru społeczności M04.
