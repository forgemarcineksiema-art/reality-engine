# Brief okna i sceny laboratoryjnej — etap M02

Status: wykonane i zaliczone, autoryzowane przez użytkownika po propozycji M02. [Raport końcowy](../verification/m02-window-laboratory.md) obejmuje pełne testy oraz diagnostykę GPU. Kontrakty: INV-02, INV-03, INV-04, INV-05, INV-11, INV-12 i INV-16; odbiór obsługiwanego zakresu SC-11 oraz regresji M01.

## Zakres przed kodem

Własne okno Win32, renderer D3D11, proceduralna statyczna scena, kamera orbitalna, zaznaczanie, standardowe kontrolki Windows i inspektor. Dwa znaczniki wykonawców i jedna partia są prezentacją domeny M01. Ich rozmieszczenie nie jest jeszcze pozycją fizyczną w świecie. Rezerwacja, rozpoczęcie, przerwanie, zużycie i zapis przechodzą przez rzeczywisty WorkWorld; renderer nie mutuje domeny.

Konsument LabSession tworzy scenariusz z 3 jednostkami materiału. Czynność wymaga 3, zużywa 1 na początku i trwa 100 kroków po 50 ms. Wybrany wykonawca otrzymuje nową komendę; jej numer jest wyznaczany z odczytywanej historii komend. Odczyt obsługuje dokładnie dwóch aktywnych wykonawców i jedną aktywną partię; obcy układ jest odrzucany przed zastąpieniem bieżącego świata. Nie zmieniamy formatu M01.

Domyślnie pauza. Ręczny przyrost: 20 kroków = 1 sekunda świata. Odtwarzanie mapuje czas monotoniczny na stały krok; najwyżej 20 kroków na aktualizację, zaległość pozostaje. Pauza, minimalizacja i interaktywne przeciąganie rozmiaru okna nie naliczają nowego czasu świata. Odczyt zatrzymuje odtwarzanie i rozpoczyna nowy pomiar czasu rzeczywistego. Błąd domeny lub I/O daje widoczny komunikat; bez resetu.

Zależności: biblioteka standardowa C++20 oraz nagłówki i systemowe API Windows SDK (D3D11, DXGI, D3DCompiler, WIC, Win32, WRL do COM RAII). Bez instalowania bibliotek gry, frameworka GUI, ECS lub gotowego renderera. Wybór adaptera jest jawny; domyślnie preferencja sprzętu o wysokiej wydajności, możliwość wskazania indeksu. Bez automatycznego przejścia na renderer programowy. D3D11 feature level co najmniej 11.0. Diagnostyka GPU włączana jawnie, brak warstwy diagnostycznej daje błąd.

## Pliki i dowody

Nowe moduły `src/lab` (konsument i taktowanie), `src/graphics` (mała geometria/projekcja i D3D11), `src/platform` (okno i kontrolki), testy oraz integracja CMake. M01 dostaje wyłącznie odczyt historii komend potrzebny konsumentowi.

Odbiór: Debug, RelWithDebInfo, ASan, kontrola stylu i dokumentacji; testy konsumenta, odmów, odczytu i niezależności czasu od podziału klatek; rzeczywisty test GPU z odczytem pikseli, resize, minimalizacją/powrotem i zamknięciem; osobne sprawdzenie wejścia i obrazu przez Computer Use. Zachowujemy nazwę rzeczywistego adaptera, feature level, log i obraz. Programowy obraz nie potwierdza MX330. Przegląd integratora w osobnym przejściu.

Poza zakresem: ruch postaci, fizyka, nawigacja, autonomia, modele z plików, pełny edytor, materiały PBR, odzyskiwanie po usunięciu urządzenia GPU i benchmark FPS. Błąd urządzenia kończy aplikację z raportem. Próba techniczna nie zastępuje oceny grywalności przez człowieka.
