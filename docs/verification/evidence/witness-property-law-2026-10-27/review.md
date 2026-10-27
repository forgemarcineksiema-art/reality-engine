# Niezależny review: law_review

Recenzent był osobnym agentem z czystym kontekstem; wykonywał odczyt kodu, testów, dokumentów i wskazanych czerwonych logów. Nie edytował źródeł, nie uruchamiał kompilacji ani GPU. Poniżej zachowana treść ustaleń i końcowej oceny, nie wynik dodatkowego CTest.

## Pierwszy przebieg — REQUEST CHANGES

P1: prepare_law_transfer rezerwowało testimonies.size()+incidents.size()+1 dla każdej osoby, blokując 171. zwrot mimo rzeczywistego rozmiaru 342<512; limit incydentów był sprawdzany także przed autoryzacją legalnego mieszkańca/kuriera. Po usunięciu przygotowania rzeczywiste limity muszą być sprawdzane przy push_back, bo tick nie wykonuje pełnego validate przed zatwierdzeniem.

Luki dowodowe: rzeczywisty relay A→B→C z zapisem, zastąpienie sesji z prawem formatami 2/3/5 i nieprzerwanie rozpoczętej pracy przez otrzymany raport. make_law_course/law_smoke nie ujawniły dodatkowego defektu.

Autor dodał czerwony repeated_return i poprawił limity; dodał limits, multi_relay, legacy_sections i safe_action. Samodzielnie wykrył także przeciek niewidzianej kradzieży podczas obserwacji zwrotu, z czerwonym unseen_return.

## Drugi przebieg — REQUEST CHANGES

P1: validate_law dereferencjonowało pusty optional world_.resource(station.lot) przed sprawdzeniem istnienia partii. Potrzebna jawna odmowa przy checksum-valid korupcji pustej chronionej sceny.

P2: świadectwo uregulowania sprawdzało istnienie wiedzy o kradzieży, lecz nie jej chronologię. Osobisty świadek musi znać incydent najpóźniej w observed_tick zwrotu; odbiorca najpóźniej w learned_tick otrzymanego raportu.

Autor dodał czerwone empty_corruption i settlement_chronology, a następnie kontrole istnienia partii i wcześniejszego learned_tick. Czerwony log: law-validation-red.log.

## Końcowa ocena

APPROVE — bez pozostałych zastrzeżeń do przejrzanego zakresu.

Obie walidacje poprawiono właściwie: brak zasobu daje jawny błąd, a świadectwo restytucji wymaga wcześniejszej wiedzy o incydencie. Nowe regresje sprawdzają również zachowanie sesji po odrzuconym wczytaniu. Przeczytany log potwierdza czerwone reprodukcje obu problemów przed naprawą.

Ocena dotyczy kodu i testów po poprawkach. Nie uruchamiałem buildów ani GPU; końcowe wyniki weryfikacji pozostają po stronie głównego przebiegu.
