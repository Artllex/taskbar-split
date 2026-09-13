# Taskbar Split

[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](LICENSE)
[![Tests](https://github.com/Artllex/taskbar-split/actions/workflows/tests.yml/badge.svg)](https://github.com/Artllex/taskbar-split/actions/workflows/tests.yml)

Mod Windhawk dla Windows 11, który dzieli pasek zadań na dwie dynamiczne strefy:

`[Start / system] [uruchomione aplikacje]  <wolna przestrzeń>  [zamknięte przypięte] [zasobnik / zegar]`

Gdy przypięta aplikacja zostaje uruchomiona, jej przycisk przechodzi do lewej strefy. Po zamknięciu wraca do prawej. Mod nie odpina aplikacji i nie modyfikuje zapisanej przez Windows kolejności przypięć — zmienia wyłącznie bieżący układ wizualny paska.

## Instalacja

1. Pobierz plik `taskbar-split.wh.cpp` z sekcji [Releases](https://github.com/Artllex/taskbar-split/releases).
2. Zainstaluj i uruchom Windhawk.
3. Wejdź w **Explore** → **Create a new mod**.
4. Usuń przykładowy kod i wklej całą zawartość pliku `taskbar-split.wh.cpp`.
5. Kliknij **Compile Mod**, a następnie **Exit Editing Mode**.
6. Włącz mod. Pasek powinien przebudować się bez restartowania komputera.

Jeśli układ nie odświeży się od razu, w Windhawk wyłącz i ponownie włącz mod. Nie trzeba ręcznie restartować `explorer.exe`.

## Ustawienia

- **Left edge padding** — odstęp pierwszego elementu od lewej krawędzi.
- **Gap after system buttons** — odstęp pomiędzy Start/Wyszukaj/Widok zadań a uruchomionymi aplikacjami.
- **Gap before tray** — odstęp między prawą grupą a zasobnikiem.
- **Minimum middle gap** — minimalna preferowana przerwa pomiędzy obiema grupami.
- **Closed pinned icon size** — rozmiar ikon w prawej grupie, od 50% do 100%. Mniejsza wartość jednocześnie gęściej je układa. Po uruchomieniu aplikacji ikona wraca do 100%.
- **Keep system buttons on the left** — wymusza lewą pozycję Start/Wyszukaj/Widżety/Widok zadań. Zalecane.

## Zakres wersji

- Windows 11, pasek poziomy, x64 oraz ścieżka obsługi ARM64.
- Poprawki 0.2.1 wymagają ponownego testu na Windows; obsługa ARM64 nie została jeszcze potwierdzona na urządzeniu.
- Obejmuje główny pasek na monitorze podstawowym.
- Kolejność ikon wewnątrz obu grup jest zachowywana według kolejności Windows.
- Przy skrajnie dużej liczbie ikon odstępy mogą zostać ścieśnione, a przyciski mogą na siebie nachodzić. Zmniejsz rozmiar prawych ikon lub środkową przerwę, albo odepnij część aplikacji.

## Bezpieczne wycofanie

Wyłączenie albo usunięcie moda w Windhawk przywraca standardowy układ Windows. Mod nie zmienia rejestru ani trwałej listy przypiętych aplikacji.

## Zgodność i ważne informacje

Nie łącz z modem **Start button always on the left** ani innymi modami przesuwającymi lub skalującymi te same przyciski. Mogą wzajemnie nadpisywać układ. Odstępy są podawane w jednostkach DIP, które uwzględniają skalowanie ekranu Windows.

Mod korzysta z nieudokumentowanych elementów wewnętrznych paska Windows 11. Duża aktualizacja Windows może zmienić symbole `Taskbar.View.dll`/`ExplorerExtensions.dll`; w takim przypadku należy wyłączyć mod i zaktualizować jego kod.

Kod bieżącej wersji jest udostępniany na licencji [GPL-3.0](LICENSE). Wykorzystuje techniki i fragmenty z modów **taskbar-labels** i **taskbar-start-button-position** Michaela Maltseva (m417z) oraz **Taskbar Start Button Centered Origin** autorstwa rick/rycalvo (GPL-3.0). Technika odnajdywania hosta XAML paska korzysta również z **Taskbar multi-tray** autorstwa EDM115 oraz **Island Media Controls** autorstwa usho (MIT); ich informacja licencyjna pozostaje w kodzie.
