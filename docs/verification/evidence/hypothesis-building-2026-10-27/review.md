# Przegląd kodu hypothesis-v1

**Niezależny werdykt: WITH FIXES. Integracja poprawek przez autora: READY.** Data: 27.10.2026. Recenzent: osobny agent `hypothesis_final_review`, model GPT-6 Astra, świeży kontekst, wyłącznie odczyt. Zakres: `ae8808f..2330bf6`, cały nowy kod C++/Python, protokół, plan, testy, reguły i pierwsze dowody rozwojowe. Recenzent nie wykonywał buildów, testów, benchmarków ani generowania potwierdzającego. Pierwszy [przegląd projektu](../../hypothesis-building-design.md) jest odrębnym dokumentem.

## Ustalenia niezależnego recenzenta

Recenzent potwierdził czytelny rozdział prawdy aparatu, fizycznych pomiarów i modeli, równoważność na całej dziedzinie 0–128, prywatne odtwarzanie kandydata przed publikacją odczytu oraz niezależną kontrolę deklaracji, kosztów i bilansu. Historycznie obejrzane wyniki 8/8 i 45/45 nie zostały przedstawione jako własna świeża weryfikacja recenzenta.

| Waga i problem | Skutek | Rozstrzygnięcie autora i dowód |
| --- | --- | --- |
| Important: brak rezerwacji końca impulsu w pełnym dzienniku | 4095 pustych `advance(0)` i przyjęty `begin_probe` zapełniały dziennik; dalszy `advance` odmawiał. Prognoza mogła podobnie zająć ostatnie miejsce podczas impulsu. | Regresja `hypothesis.capacity` wykazała RED. Sesja rezerwuje teraz dwa możliwe wpisy kończenia operacji; inne wywołania nie zabierają tej rezerwy. Test obejmuje obie fazy, rozdzielone kroki i odczyt blisko/na granicy. Wynik po poprawce jest zapisany w logach. |
| Important: podciąg READY akceptował NOT READY | Dokument odrzucający gotowość mógł przepuścić przygotowanie potwierdzające. | Nowy strukturalny rekord JSON wymaga dokładnego `status: READY`, protokołu, commitu review i hashy zgodnych ze źródłami. Test odrzuca negatywne/incydentalne READY oraz zmienione źródła. |
| Important: brak dowodu przerwania listy deklaracji | Pierwszy test wyczerpywał już pierwszą prognozę; nie sprawdzał zachowania wcześniejszych odpowiedzi, powrotu po zapisie ani wyczerpania planowania w toku. | Dodano `hypothesis.interrupted`: wcześniejsza prognoza i konfiguracja pozostają w dzienniku, następna kończy budżet w środku, odczyt jest identyczny, pozostałe pomiary nadal dochodzą do 24, a nowa faza nie odnawia limitu. Ten test od razu przeszedł: luka dotyczyła dowodu, nie potwierdzonego błędu algorytmu. |
| Minor według recenzenta: brak wersji narzędzi natywnych w freeze | Sam Python i OS nie określały kompilatora, CMake/MSBuild i konfiguracji. | Autor zakwalifikował jako Important względem jawnego wymogu protokołu. Dodano parser kontrolujący konfigurację/x64 i zapis rzeczywistej metadanej CMake, MSVC, MSBuild oraz ASan; regresja parsera RED→GREEN. |
| Minor: pamięć mierzona w jednym procesie dla trzech wariantów | Późniejsze odczyty peak zawierają również wcześniejsze szczyty. | Pole pozostaje `peak_process_bytes`; raport ma nazwać je kumulatywnym szczytem procesu, bez przypisywania niezależnego zużycia pamięci wariantom. |

## Zakresy odłożone przez recenzenta i decyzje autora

- Zgodność prepare z rozwojowym `development-01`: autor wykona świeżą serię na końcowym kodzie i zachowa pierwszą. Ścisły zakaz mieszania aktualnego analizatora z dawnym freeze jest zamierzony, bez zmiany guardu.
- Pełne profile, ASan, format i raport: obowiązek autora przed zamrożeniem, nie wynik tego review.
- Wyniki potwierdzające: celowo niewygenerowane i nieoglądane podczas review; nie są warunkiem technicznej gotowości badania.
- Ludzki odbiór gry/3D, szersza fizyka i nieograniczone odkrywanie: poza zakresem tego przyrostu; wcześniejszy odbiór M04 nie jest rozszerzany.
- Złośliwe, skoordynowane przepisanie wszystkich lokalnych manifestów: nie jest objęte modelem integralności. Hashy nie przedstawiamy jako podpisu zewnętrznej instytucji. Sprawdzane są rzeczywiste pomyłki, zamiany, niezgodne deklaracje i niespójna historia.

## Weryfikacja integrującego

Po poprawkach: Debug `hypothesis.*` 10/10 PASS, Python 47/47 PASS. Pełny Debug 324/324 w 116,68 s, Release 324/324 w 43,89 s, ASan 326/326 w 186,87 s oraz format i dokumentacja PASS. Regresja pojemności wcześniej nie przechodziła; test przerwanych deklaracji od razu potwierdził istniejące zachowanie. [Rekord gotowości](review-record.json) wiąże źródła i hashe logów kontroli. Nie przeprowadzono drugiego niezależnego review; zamknięcie konkretnych uwag należy do autora i testów RED→GREEN. Końcowy przebieg rozwojowy oraz osobna próba potwierdzająca następują po tej weryfikacji; nie są tu przedstawiane jako wykonane.
