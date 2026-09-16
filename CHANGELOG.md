# Historia zmian

## 0.3.18

- Arm section dragging only after horizontal movement.
- Release pointer capture only when acquired by the mod.
- Add default-on section dragging switch and clarify README.

## 0.3.17

- Zapis kolejności według Appid w magazynie Windhawk po zakończeniu przeciągania.
- Brak dziedziczenia rangi przez inną aplikację wykorzystującą stary kontener XAML.
- Zachowanie pozycji aplikacji poza widocznym fragmentem listy.
- Pomijanie wtórnych układów przed przeliczaniem głównego paska.
- Pamięć podręczna widgetu i ograniczenie wyszukiwania do TaskbarFrame, także gdy widgetu nie ma.
- Doprecyzowanie celowego zastąpienia natywnego przeciągania i ograniczeń zapisu.

## 0.3.16

- Przeciąganie i zamiana miejsc w obrębie obu sekcji, z podglądem i anulowaniem przez Escape.
- Naprawa opóźnionego powrotu/przelotu ikony po upuszczeniu.
- Bezpośrednie sortowanie prawej grupy według zapisanej kolejności.
- Rezerwowanie miejsca przed prawostronnym widgetem pogody.
- Domyślne odstępy 0 / 0 / 8 / 48 DIP i skala prawej grupy 90%.
- Usunięcie diagnostycznego zapisu plików po przeciąganiu.
- GPL-3.0 wraz z informacjami o źródłach; 30 testów lokalnych.

## 0.2.1–0.3.15 — iteracje testowe

- Korekta licencji do GPL-3.0: wcześniejsza deklaracja niezależnego pochodzenia i MIT była błędna.
- Pozycjonowanie przez XAML Arrange, poprawki ABI, cyklu życia i obsługi ARM64.
- Iteracyjne testy przeciągania na Windows i poprawki po analizie logów.

## 0.2.0 — 2026-09-13

- Testowa implementacja układu oparta na transformacjach XAML.
- Ówczesne oznaczenie MIT zostało skorygowane na GPL-3.0 w 0.2.1.
- Opcjonalne skalowanie zamkniętych przypiętych ikon w zakresie 50–100%.
- Automatyczny powrót ikony do rozmiaru 100% po uruchomieniu aplikacji.
- Gęstość prawej grupy zmienia się razem ze skalą ikon.

## 0.1.0 — 2026-09-05

- Pierwsza wersja testowa.
- Dynamiczny podział na uruchomione aplikacje po lewej i zamknięte przypięte po prawej.
- Elastyczna pusta przestrzeń pomiędzy grupami.
- Zachowanie kolejności ikon Windows wewnątrz obu grup.
- Automatyczne przeliczanie układu po zmianie stanu uruchomienia.
- Ustawienia odstępów i położenia przycisków systemowych.
