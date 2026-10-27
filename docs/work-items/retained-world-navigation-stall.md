# Stagnacja po kontynuacji długiego świata

Status: naprawiono defekt wykonywania starej trasy; końcowa weryfikacja opisana w [raporcie](../verification/retained-world-navigation-stall.md). Ten przyrost oddzielił rzeczywistą blokadę Step od defektu starej trasy. Późniejsza [lokalna współpraca](../verification/local-clearance.md) odblokowuje ten zachowany przypadek. Ogólne konflikty tłumu i pełny odbiór przed dostawami pozostają osobnym zakresem. Nie jest dowodem matematycznej nieosiągalności celu ani dowolnego zakleszczenia.

## Dowód

Zapis końca wcześniejszego pomiaru przy kroku 6599 kontynuowano o 1000 kroków aktualną polityką potrzeb. Obserwator zgłosił osoby 2 (6885), 6 i 7 (6938), 8 (7352), ponownie 6 (7567). Większość później wykazała postęp. Dla osoby 7 końcowy raport przy 7599 nadal podaje ostatni postęp 6838 i 761 kroków stagnacji zamiaru, mimo chwilowej prędkości około 0,81 m/s. Nie przypisujemy przyczyny samemu głodowi, promowi ani lokalnemu omijaniu bez reprodukcji.

Zachowano [świat przy pierwszym alarmie osoby 7](../verification/evidence/retained-world-stall-6938/world.save) oraz [obserwację](../verification/evidence/retained-world-stall-6938/diagnostic.txt). To rzeczywisty zapis zewnętrznej wersji 2, bez zmiany danych w celu wygenerowania problemu. Log całej próby: `build/diagnostics-needs-retained-captured.log`. Runner bez dodatkowej opcji nie zapisuje plików. Jawne `--diagnostics NOWY_KATALOG` tworzy najwyżej osiem osobnych migawek i odmawia użycia istniejącego katalogu.

## Następny dowód wymagany przed naprawą

Wczytać migawkę w rzeczywistym oknie i headless, odtworzyć bez nowych komend. Oddzielić: osiągalność przy aktualnym układzie kapsuł, zaangażowany Step, cykliczną zmianę obejścia, kolejkę i ruch platformy. Zmniejszyć fixture, zachowując mechanizm. Sprawdzić wersję z istniejącym obejściem oraz rzeczywiście zamkniętą drogą. Dopiero potem test czerwony i naprawa właściwego właściciela stanu.

Odbiór nie może polegać na wyciszeniu alarmu, zwiększeniu jego okna, zmianie prędkości lub teleportacji. Musi wykazać rzeczywisty postęp albo stabilny, wyjaśniony stan braku drogi; bez społecznej zgody nie przesuwać innych osób ani gracza. Zaliczone testy fundamentu nie zamykają tego zgłoszenia.


## Zatwierdzona naprawa i odbiór

Użytkownik polecił naprawić pozostały problem. Stosujemy systematic-debugging i test-driven-development, z osobnym review drugiego agenta. Zakres: `src/lab/traversal_session.cpp`, regresje w `tests/traversal_audit_tests.cpp`, rejestr CTest i dokumentacja. Kontrakty: aktualność trasy, dokładny zamiar gracza, bezpieczeństwo Step/transportu, wspólna kolizja oraz zgodna kontynuacja. Zegar, prędkości, diagnostyka i format zapisu pozostają.

Ślad `build/stall-before-trace.log` wykazuje cykliczny ruch osoby 7 na tym samym odcinku oraz kolejne nieudane próby dojścia. Sprawdzamy minimalną reprodukcję: początkowo poruszająca się osoba zatrzymuje się w wąskim przejściu już po wyznaczeniu dojścia w zasięg. Wymagany czerwony test pozostawiania starej trasy, potem stabilny NoPath z zachowaniem celu, odczyt zapisu i wznowienie po rzeczywistym odejściu blokującego. Wariant szerokiej podłogi musi dojść obejściem, także na stoku/obróconej geometrii. Rzeczywista migawka musi przestać wykonywać ten sam zablokowany odcinek. Pełne profile, kontrola stylu, próba okna i niezależny przegląd zamykają odbiór tego przypadku; M04 nadal pozostaje nieukończone.

Ponowienia muszą sprawdzać dozwolone alternatywy w zasięgu, także po początkowym NoPath bez wybranego punktu. Najwyżej cztery wyszukiwania kandydatów plus kontrola starego punktu na 40 kroków; dokładny cel bez zasięgu zachowuje wcześniejszy limit.
