# Weryfikacja uporządkowanej dokumentacji

Data: 25.10.2026. Zakres: dokumentacja, archiwizacja oraz walidator dokumentów. Workspace bez historii Git; raport nie podaje identyfikatora commitu.

To historyczny raport stanu sprzed implementacji M00. Aktualny stan i późniejszą weryfikację przedstawia [raport fundamentu C++](m00-build-and-diagnostics.md).

## Wykonane zmiany

- Jeden plan etapów zastąpił nakładające się wcześniejsze listy prac.
- Oddzielono ustalenia użytkownika, propozycje, reguły, przyszłe testy i aktualny stan.
- Powstały AGENTS.md, engineering-rules.md, kontrakty, strategia testów i pozostałe dokumenty wskazane w głównym indeksie.
- Sześć oryginalnych dokumentów przeniesiono bez zmiany bajtów do archiwum. SHA-256 sprawdzono po przeniesieniu i walidatorem.
- Dodano narzędzie dokumentacyjne oparte wyłącznie na bibliotece standardowej Pythona oraz jego regresje.

## Wykonane sprawdzenia

| Polecenie / próba | Wynik | Status |
| --- | --- | --- |
| `python tools/check_docs.py` | Rejestr, lokalne linki i kotwice, ID oraz sumy archiwum bez błędów | ZALICZONE |
| `python -m unittest discover -s tools/tests -p "test_*.py" -v` | 21 testów, kod wyjścia 0 | ZALICZONE |
| `python tools/tests/test_check_docs.py DocumentationValidationTests.test_rejects_missing_local_link` | Pojedynczy wskazany test przechodzi | ZALICZONE |
| Porównanie SHA-256 oryginałów przed i po przeniesieniu | Zgodne dla wszystkich sześciu źródeł | ZALICZONE |
| Kontrola rozmiaru AGENTS.md | 285 słów przy prostym podziale białymi znakami | ZALICZONE |

Testy narzędzia obejmują brakujące pliki i kotwice, źle zapisane lub nieznane ID, definicję poza właścicielem, alias ścieżki, pusty rejestr ID, błędną rolę, wyjście poza workspace, zmianę archiwum oraz zakres komunikatów CLI. Uwzględniono różnicę bajtów LF/CRLF.

Przypadki błędów referencyjnych i walidacji rejestru najpierw wykazały porażkę na starym walidatorze, potem przeszły po naprawie. Nie osłabiono sum archiwum ani oczekiwań testów, aby uzyskać sukces.

## Przegląd niezależny

Wykonano osobne odczyty dokumentacji i kodu walidatora przez agentów audytujących. Uwagi dotyczące ilości wolnej/zarezerwowanej, diagnostyki pamięci C++ oraz fałszywie dodatnich wyników walidatora zostały uwzględnione. Integrator przeczytał uwagi i uruchomił właściwe kontrole po zmianach.

## Ograniczenia

Nie wykonano kompilacji C++, testów silnika, fizyki, nawigacji, renderera, benchmarków ani akceptacji gry przez człowieka. Te implementacje jeszcze nie istnieją. Zewnętrzne strony źródeł nie były sprawdzane przez walidator.

Walidator obsługuje określony podzbiór Markdown. Referencje i HTML zgłasza jako nieobsługiwane; nie jest pełnym parserem języka. Kontrola SHA porównuje archiwum z lokalnym manifestem i chroni przed niezamierzoną zmianą; manifest nie jest podpisem kryptograficznym.

Same instrukcje nie gwarantują jakości przyszłego kodu. Tworzą obowiązki i sprawdzalne kryteria, które trzeba rzeczywiście wykonywać.
