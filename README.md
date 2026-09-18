# Taskbar Split

[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](LICENSE)
[![Tests](https://github.com/Artllex/taskbar-split/actions/workflows/tests.yml/badge.svg)](https://github.com/Artllex/taskbar-split/actions/workflows/tests.yml)

Oficjalnie opublikowany mod Windhawk dla Windows 11, który dzieli pasek zadań na dwie dynamiczne strefy:

![Taskbar Split: running applications on the left and closed pinned applications on the right](https://raw.githubusercontent.com/Artllex/taskbar-split/main/assets/taskbar-split.png)

`[Start / system] [uruchomione aplikacje]  <wolna przestrzeń>  [zamknięte przypięte] [zasobnik / zegar]`

Gdy przypięta aplikacja zostaje uruchomiona, jej przycisk przechodzi do lewej strefy. Po zamknięciu wraca do prawej. Mod nie odpina aplikacji i nie modyfikuje zapisanej przez Windows kolejności przypięć — zmienia wyłącznie bieżący układ wizualny paska.

## Instalacja

1. Zainstaluj i uruchom Windhawk.
2. Wejdź w **Explore**.
3. Wyszukaj **Taskbar Split: Running Left, Pinned Right**.
4. Kliknij **Install**.

Wersja opublikowana w katalogu jest utrzymywana w
[oficjalnym repozytorium Windhawk](https://github.com/ramensoftware/windhawk-mods/blob/main/mods/taskbar-split.wh.cpp).
Plik w tym repozytorium służy do rozwoju następnej wersji i może czasowo
wyprzedzać wydanie dostępne w katalogu.

Jeśli układ nie odświeży się od razu, w Windhawk wyłącz i ponownie włącz mod. Nie trzeba ręcznie restartować `explorer.exe`.

## Ustawienia

- **Left edge padding** — odstęp pierwszego elementu od lewej krawędzi.
- **Gap after system buttons** — odstęp pomiędzy Start/Wyszukaj/Widok zadań a uruchomionymi aplikacjami.
- **Gap before tray** — odstęp między prawą grupą a zasobnikiem.
- **Minimum middle gap** — minimalna preferowana przerwa pomiędzy obiema grupami.
- **Closed pinned icon size** — rozmiar ikon w prawej grupie, od 50% do 100%. Mniejsza wartość jednocześnie gęściej je układa. Po uruchomieniu aplikacji ikona wraca do 100%.
- **Keep system buttons on the left** — wymusza lewą pozycję Start/Wyszukaj/Widżety/Widok zadań. Zalecane.

Wartości domyślne: Left edge padding **0**, Gap after system buttons **0**, Gap before tray **8**, Minimum middle gap **48**, Closed pinned icon size **90%**. Zapisane ustawienia istniejącej instalacji pozostają zachowane. Gdy przyciski systemowe zachowują położenie Windows, prawa grupa kończy się przed prawostronnym widgetem pogody.

## Zakres wersji

- Windows 11, pasek poziomy, x64 oraz ścieżka obsługi ARM64.
- Obejmuje główny pasek na monitorze podstawowym.
- Przeciąganie zmienia kolejność tylko wewnątrz danej sekcji, z podglądem podczas ruchu. Escape anuluje zmianę.
- Kolejność aplikacji jest zapisywana po upuszczeniu ikony w magazynie moda Windhawk, osobno dla obu sekcji. Odtworzenie kontenera XAML nie przenosi kolejności na inną aplikację.
- Lewy przycisk myszy celowo obsługuje przeciąganie wewnątrz sekcji zamiast natywnej zmiany przypięć Windows. Przyciski bez identyfikatora `Appid: ` zachowują natywną obsługę i nie są zapisywane. Osobne okna tej samej aplikacji mają wspólną rangę; zapis jest ograniczony do 256 aplikacji na sekcję.
- Nowo uruchomiona aplikacja przechodząca z prawej grupy trafia na koniec lewej grupy.
- Przy skrajnie dużej liczbie ikon odstępy mogą zostać ścieśnione, a przyciski mogą na siebie nachodzić. Zmniejsz rozmiar prawych ikon lub środkową przerwę, albo odepnij część aplikacji.

## Bezpieczne wycofanie

Wyłączenie albo usunięcie moda w Windhawk przywraca standardowy układ Windows. Mod nie zmienia rejestru ani trwałej listy przypiętych aplikacji.

## Zgodność i ważne informacje

Nie łącz z modami **Start button always on the left**, **Taskbar Start Button Centered Origin** (`taskbar-centered-start-split-icons`) ani innymi modami przesuwającymi lub skalującymi te same przyciski. Mogą wzajemnie nadpisywać układ. Taskbar Split dzieli aplikacje według stanu uruchomienia, a Taskbar Start Button Centered Origin według położenia okien na ekranie. Odstępy są podawane w jednostkach DIP, które uwzględniają skalowanie ekranu Windows.

## Weryfikacja wydania 0.3.18

Wersja 0.3.0 zmienia rzeczywiste prostokąty układu przycisków poprzez XAML `Arrange`. Skalowanie dotyczy wyłącznie zamkniętych przypiętych przycisków, łącznie z ich podświetleniem. Przycisk przepełnienia trafia za grupę uruchomionych aplikacji; o zawartości menu przepełnienia nadal decyduje Windows.

Taskbar Split pozostaje osobnym modem, ponieważ tworzy układ od lewego Startu do prawego zasobnika, z rozdzieleniem aplikacji uruchomionych i zamkniętych. Centered Origin organizuje okna względem środka ekranu. Wspólna technika pozycjonowania nie oznacza identycznego sposobu użycia.

Na Windows x64 potwierdzono zwykłe kliknięcia, przeciąganie w obu sekcjach,
miniatury po najechaniu, menu prawego przycisku, działanie po restarcie
Eksploratora, lewe i środkowe wyrównanie paska oraz ukrywanie i przywracanie
Wyszukiwania, Widoku zadań i Widżetów. Potwierdzono też rezerwowanie miejsca
dla pogody po wyłączeniu **Keep system buttons on the left**.

Wydanie przechodzi obie kontrole CI katalogu Windhawk oraz 33 lokalne testy
regresyjne. Nie przeprowadzono testu uruchomieniowego na ARM64 ani pełnego
restartu systemu.

Mod korzysta z nieudokumentowanych elementów wewnętrznych paska Windows 11. Duża aktualizacja Windows może zmienić symbole `Taskbar.View.dll`/`ExplorerExtensions.dll`; w takim przypadku należy wyłączyć mod i zaktualizować jego kod.

Kod bieżącej wersji jest udostępniany na licencji [GPL-3.0](LICENSE). Wykorzystuje techniki i fragmenty z modów **taskbar-labels** i **taskbar-start-button-position** Michaela Maltseva (m417z) oraz **Taskbar Start Button Centered Origin** autorstwa rick/rycalvo (GPL-3.0). Technika odnajdywania hosta XAML paska korzysta również z **Taskbar multi-tray** autorstwa EDM115 oraz **Island Media Controls** autorstwa usho (MIT); ich informacja licencyjna pozostaje w kodzie.

## Rozwój i zgłaszanie błędów

To repozytorium pozostaje stroną projektu i miejscem przygotowywania zmian.
Błędy i propozycje można zgłaszać w [GitHub Issues](https://github.com/Artllex/taskbar-split/issues).
Po sprawdzeniu zmiany są wysyłane do oficjalnego katalogu Windhawk.

Opcja **Enable section dragging** (domyślnie włączona) steruje przeciąganiem w obrębie sekcji. Po wyłączeniu mysz obsługuje Windows; podział paska pozostaje aktywny, ale granice przeciągania nie są wymuszane. Zmiana obowiązuje od następnego gestu.
