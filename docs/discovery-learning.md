# Laboratorium odkrywania — API i obsługiwany zakres

**Status: implementacja discovery-v1; stan kontroli i eksperymentu wskazuje [raport](verification/discovery-learning-experiment.md).** Kryteria przed próbą pozostają w [zatwierdzonym protokole](work-items/discovery-learning-experiment.md). To odrębny konsument headless Genesis, bez nowego okna 3D i bez zmiany odbioru M04.

## Własność i interfejsy

`DiscoveryWorld` posiada trzy niezmienne reguły elementów, dwa stanowiska aparatury, energię, konfigurację, przyjęte komendy i rzeczywiste pomiary. `configure(command, actor, bench, chain)` ustawia łańcuch po jednym kroku; `pulse` rezerwuje impuls, a kolejny krok zatwierdza wyjście i stratę. Powtórzenie przyjętej komendy jest idempotentne; ten sam ID z innym payloadem daje Conflict. Odmowa nie wydaje energii. Źródło obejmuje energię zarezerwowaną podczas impulsu, rozliczenie następuje przy końcu.

`DiscoveryAgent` posiada własne rekordy pomiarów, źródła i czasy, 75 początkowych hipotez na symbol oraz dostarczoną metodę ich eliminacji. Konstruktor nie otrzymuje reguł świata, seeda ani referencji do aparatury. `choose_probe`, `predict`, `choose_chain` i `teaching_packet` korzystają wyłącznie z tego stanu. `freeze` zabrania dalszych obserwacji w modelu ocenianym. Pusty zbiór hipotez jest jawnym `model_inadequate`, bez automatycznej naprawy.

`DiscoverySession` komponuje aparaturę i dwa modele wiedzy z istniejącą `TraversalSession`. ID, położenia, kolizja, nawigacja i zegar 50 ms pozostają po stronie starej sesji przestrzennej. Dwa nowe tylko odczytowe zapytania `can_interact_at` i `can_exchange_knowledge` wywołują istniejące kontrole geometrii. Nie ma drugiej fizyki ani zdalnego odczytu ukrytych parametrów przez agenta.

`begin_probe` wymaga fizycznego dostępu i zapisuje prognozę przed skutkiem. Dwa wywołania `advance(1)` kończą ustawienie i impuls. `probe` wykonuje te dwa kroki synchronicznie. `train` respektuje limity A/B i zamraża zakończony model. `share` przekazuje tylko istniejące pomiary odkrywania po kontakcie; `meet_and_share` realizuje dojście po wspólnej geometrii. Wiedza otrzymana nie staje się własną obserwacją i nie uruchamia automatu przekaźnika.

Kontrolowany harmonogram, cel i rodziny praw są wyposażeniem od autora. Potrzeby starej autonomii są wyłączone w tej scenie. Energia aparatury jest odrębną wielkością eu; nie kopiuje jedzenia ani części WorkWorld. Nie twierdzimy, że zrealizowano produkcję przedmiotów, fizyczną elektrotechnikę lub własną motywację.

## Zapis i niezależne kopie eksperymentalne

`GENESIS_DISCOVERY 1` jest nową zewnętrzną otoczką. Zawiera niezmieniony zapis podróży, aktualną aparaturę, archiwa wcześniejszych faz, pamięć agentów, stan generatorów, limity, prognozy oraz potwierdzenie kontaktu. Hipotezy odtwarza z danych; aparat odtwarza ze swojej historii komend. Pending musi zgadzać się z komendą, konfiguracją, śladem, etapem i stanem obserwatora. Odczyt buduje i sprawdza kandydata przed podmianą.

`evaluation_copy` jest jawną nową sesją laboratoryjną z 8192 eu na stanowisko. Zachowuje wcześniejsze rzeczywiste pomiary w archiwum, aby wiedza miała źródło. Nie jest funkcją odzyskiwania energii lub teleportacji w świecie gry. Nie zmieniono `GENESIS_TRAVERSAL 7`, dawnych formatów ani starego F9.

Ślad ruchu jest diagnostyką tylko do odczytu, bez wpływu na sterowanie i bez serializacji. W badaniu zapisuje go runner. Kontynuacja świata zachowuje rzeczywiste pozycje i potwierdzenie kontaktu, ale nie odtwarza diagnostycznej historii ruchu sprzed checkpointu. Checksum wykrywa korupcję; nie uwierzytelnia autora pliku ani historycznego przebiegu fizycznego.

## Konsument i analiza

`reality_discovery` wykonuje jeden przypadek z manifestu lub kontrolowaną fixture kontynuacji. Plik przypadku oraz zapis zawierają prawdę eksperymentatora; nie są wejściem modelu wiedzy. Surowy JSON zachowuje prognozy sprzed doświadczenia, interwencje, wyniki, pakiet, ślad dojścia i bilanse. Liczniki kosztu obejmują przyrosty każdej fazy, również kontrolę bez doświadczenia, wybór konfiguracji oraz łączny koszt pary bez podwójnego liczenia wspólnego checkpointu. Czas CPU mierzy GetProcessTimes, szczyt working set — GetProcessMemoryInfo. To nie pomiar FPS ani długiej społeczności.

Python `run_discovery_experiment.py` generuje manifest zgodnie z protokołem, zamraża źródła/binarne/protokół przez SHA-256, kopiuje exe do odrębnego katalogu i odmawia ponownego wykonania tej samej próby. Niezależnie od C++ sprawdza wyniki reguł, bilanse i zgodność metryk z surowymi danymi. `analyze` sprawdza także freeze, marker rozpoczęcia, hashe i komplet danych wobec manifestu. Dokładne ułamki zachowują progi 0,20 i 0,05/3; nie stosujemy powiększonej tolerancji pod wynik.

Odmienne konteksty A/B mogą mieć pokrywające się cele; liczba pokryć pozostaje w JSON. Rodziny praw i algorytm zostały zaprojektowane. Pozytywny wynik tego eksperymentu jest wynikiem identyfikacji i zastosowania zależności w małej znanej klasie, nie odkryciem nieznanej ludzkości teorii, życia lub świadomości.
