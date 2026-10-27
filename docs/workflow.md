# Przepływ pracy użytkownika i agentów

**Status: obowiązujący proces wykonania zadań.** Reguły kodu są w [engineering-rules.md](../engineering-rules.md), a kryteria poszczególnych obszarów w [testach](testing-strategy.md).

## Mała zmiana od kontraktu do dowodu

1. Odczytaj aktualny stan i dokument odpowiedzialny za temat. Nazwij cel oraz granice.
2. Zapisz oczekiwany wynik, dotknięte niezmienniki i sposób reprodukcji. Dla zmiany zachowania użyj [briefu](templates/change-brief.md).
3. Przejrzyj rzeczywiste API i konsumentów przed edycją. Proponowane API nie jest istniejącym API.
4. Wykonaj mały kompletny przyrost. Nie dodawaj atrap udających gotowy subsystem.
5. Uruchom właściwe komendy i zachowaj wynik. Porównaj stan z kontraktem, nie tylko z brakiem awarii.
6. Przejrzyj zmianę pod kątem konfliktów własności, błędnych odmów, nieaktualnych planów i kosztu.
7. Zaktualizuj jeden dokument odpowiedzialny za zmienioną regułę; pozostałe odsyłają do niego.
8. Oddaj [raport](templates/verification-report.md) z wynikiem i ograniczeniami. Nieukończonego dowodu nie opisuj jako ukończonego zadania.

## Repozytorium Git

Repozytorium jest lokalne, na gałęzi `main`. Pierwszy commit zachowuje zastany kod, testy, dokumentację, konfigurację agentów i historyczne dowody; nie oznacza ponownego odbioru etapów ani ukończenia M04. Nie skonfigurowano remote i niczego nie opublikowano. Zdalna kopia wymaga osobnego wskazania miejsca docelowego; lokalna historia nie chroni przed utratą dysku.

Przed pracą sprawdź stan i historię:

```powershell
git status --short --branch
git log -5 --oneline
git diff
```

Nowe zadania prowadź na gałęziach `codex/<temat>`, z małymi commitami obejmującymi powiązane źródła, testy i dokumentację. Przed commitem uruchom kontrole właściwe dla zmiany, dodaj pliki jawnie, przejrzyj indeks i dopiero zapisz zmianę:

```powershell
git diff --cached --stat
git diff --cached
git commit -m "Opisz konkretną zmianę"
```

`.gitignore` pomija buildy, cache, lokalne środowiska, ustawienia IDE i pliki `.env`. Nie pomija globalnie logów, zapisów świata ani obrazów: mogą być dowodami lub fixture'ami. Nowe ważne reprodukcje i wyniki umieszczaj w `docs/verification/evidence/`, a ich kontekst w odpowiednim raporcie. Nie usuwaj porażek po uzyskaniu zielonego wyniku.

Zastane lokalne artefakty z `build/` zachowano przed pierwszym commitem w [archiwum dowodów](verification/evidence/git-baseline-2026-10-27/manifest.json). Manifest podaje ścieżkę źródłową, ścieżkę kopii, rozmiar i SHA-256 każdego pliku oraz regułę doboru. Oryginały pozostają w `build/`; starsze raporty zachowują historyczne ścieżki i wyniki. Manifest pozwala znaleźć ich wersjonowane kopie. Katalogi kompilatora `build/msvc/` i `build/asan/` oraz tymczasowe wyniki CTest nie należą do tego archiwum. Nie jest to pełna kopia katalogu `build/` ani nowy przebieg testów.

`.gitattributes` ustala LF dla tekstu. Archiwum historyczne, dowody i fixture'y zachowują dokładne bajty, aby Git nie zmieniał zapisów ani sum kontrolnych. Autor commitów jest ustawiany lokalnie dla projektu; nie zmieniaj globalnej konfiguracji użytkownika przy obsłudze tego repozytorium.

## Podział między agentami

Zadanie ma właściciela i zakres plików. Niezależne czytanie i audyt można wykonywać równolegle w granicach autoryzacji użytkownika i narzędzi. Dwie osoby nie powinny równocześnie zmieniać tego samego kontraktu bez uzgodnionego planu integracji.

Wynik podzadania jest propozycją do sprawdzenia. Agent integrujący MUSI przeczytać zmiany oraz uruchomić istotny test całego przepływu; nie wystarcza komunikat innego agenta „gotowe”. Nie nadpisuj cudzych plików ani nie resetuj workspace w celu uzyskania czystego buildu.

## Przegląd

Zmiana przekraczająca granicę modułów, formatu zapisu lub modelu czasu wymaga dodatkowego przeglądu kontraktu. Może go wykonać inny agent, jeśli współpraca jest autoryzowana, albo osoba integrująca w osobnym przejściu kontrolnym. Brak dostępnego recenzenta nie uprawnia do wymyślenia niezależnego review.

Pytania: kto posiada pole, co się stanie po przerwaniu, skąd pochodzi wiedza, czy stary wynik jest aktualny, czy zapis zachowa czynność, czy budżet nie ukrywa utraty skutku. Testuj przypadek porażki, nie tylko drogę sukcesu.

## Zmiana wymagań i decyzji

Rutynowe naprawy zgodne z wymaganiem nie potrzebują osobnej zgody. Jeśli wynik wymaga naruszenia ustalonego kierunku użytkownika, przedstaw konkretny konflikt i opcje. Nie obchodź go cichą zmianą testu, granicy subsystemu albo zależności.

## Definicja ukończenia

Kod jest zaimplementowany w deklarowanym zakresie; wymagane testy wykonano; kontrakty i konsumenci są zgodni; koszt zmierzono, gdy zmiana go dotyczy; dokumentacja odpowiada stanowi; raport wskazuje ograniczenia. Zablokowany obowiązkowy test pozostawia odpowiednią część nieukończoną.

W dokumencie czysto projektowym ukończenie oznacza spójny opis i właściwą kontrolę dokumentacji. Nie wolno sugerować, że spisanie planu wykonało implementację lub przyszłe scenariusze.
