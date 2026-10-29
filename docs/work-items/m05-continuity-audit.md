# Audyt SC-08 i zamknięcie M05 — plan wykonania

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** przypisać dowody SC-08 i sporządzić zbiorczy raport M05 z rzeczywistą deklaracją użytkownika.

**Architecture:** audyt istniejących WorkWorld/TraversalSession i wspólnego write_snapshot_atomic, bez zmiany formatu, zachowania silnika lub progów. Istniejące runnery i systemowy uchwyt Windows służą do zachowanej próby odmowy podmiany pełnego zapisu.

**Tech Stack:** C++/MSVC, Python stdlib, Win32, Markdown/JSON.

**Spec:** SC-08/09/10/13 w testing-strategy.md; INV-05/12/13/15; M05 w prototype-plan.md; bieżące polecenie użytkownika.

## Zakres i ograniczenia

Zmiany: ten plan, docs/verification/sc08-continuity-audit.md, docs/verification/m05-acceptance.md, rejestr dokumentacji, jeden plan, README oraz aktualne odsyłacze trzech raportów M05. Dowody i skrypt wykonania w build/m05-audit, następnie wersjonowane archiwum. Żadnych nowych zależności, skipów, tolerancji ani podniesienia limitów 8/18/20 s. Kontrole szeregowo. Brak push/merge. Nie wymagamy nowej rozbudowy silnika bez wykazanej luki. Oddzielamy akceptację deklaratywną od obserwacyjnego playtestu i M06.

## Review Focus

Błąd podmiany zachowuje ostatni poprawny plik i zgłasza niepowodzenie; błąd odczytu nie zmienia aktywnego świata lub wyniku; fazy checkpointów wynikają z asercji scenariusza, nie nazw; stare wersje nie zmieniają semantyki; katalog przyszłych prób awarii sprzętu nie zostaje nazwany wykonanym testem.

## Task 1: audyt i raport

- [x] Odczytać właścicieli kontraktów, mapę, implementację atomowego I/O i walidacji kandydata; przypisać istniejące asercje/testy do wymagań.
- [x] Utrwalić szesnaście par checkpoint/continue: praktyki 0–3, dwie metody, trzy dalekie fazy i siedem historii (łącznie 16); porównać pełne bajty i nienaruszony checkpoint.
- [x] Utrwalić rzeczywisty błąd podmiany TraversalSession przy uchwycie Windows, brak katalogu, uszkodzony format i nieobsługiwaną wersję. Sprawdzić nienaruszony ostatni poprawny plik/plik wejściowy oraz dalszą kontynuację.
- [x] Uruchomić dobrane istniejące testy SC-08 w Debug/Release/ASan, bez konkurencyjnego obciążenia; wcześniejsza pełna macierz 420/420/422 pozostaje osobnym dowodem z 1f620fc.
- [x] Zbiorczy raport mapuje SC-08/09/10/13, zachowuje ograniczenia i wcześniejsze porażki. Zapisuje dosłowne polecenie użytkownika jako akceptację bez osobnej próby.
- [x] Zaktualizować jeden plan i odsyłacze, wykonać niezależny przegląd całego audytu, kontrolę dokumentów, manifest CRC/SHA-256 i lokalny commit.
