# Projekt: Grafika Komputerowa - Symulacja 3D

Projekt w technologii OpenGL renderujący model 3D szczytu górskiego z obsługą proceduralnego śniegu, oświetlenia oraz wyznaczaniem ścieżek.

## Główne Funkcjonalności

*   **Proceduralny Śnieg na Powierzchni:** Generowany dynamicznie we fragment shaderze (Value Noise, FBM). Posiada konfigurowalną skalę szumu oraz poziom zniekształcenia krawędzi (Distortion), które można zmieniać w czasie rzeczywistym.
*   **Animacja Opadającego Śniegu:** Pełny system cząsteczkowy symulujący padający śnieg. Uwzględnia takie parametry fizyczne jak grawitacja i siła wiatru. Cząsteczki precyzyjnie reagują na ukształtowanie terenu (rozbijają się o górę).
*   **Pathfinding (A*):** Algorytm wyznaczający najkrótszą trasę na szczyt. Unika terenów stromych oraz obszarów pokrytych dużą ilością śniegu. Rzutuje siatkę 3D na mapę 2D (Height Grid).
*   **Normal Mapping:** Obliczany z użyciem pochodnych ekranowych (`dFdx`, `dFdy`). Symuluje wypukłości i chropowatość bez dodatkowej geometrii.
*   **Oświetlenie:** Model kierunkowy (Diffuse) i otoczenia (Ambient).
*   **Kamera Free-Fly:** Sterowana klawiaturą, oparta na kątach Eulera. Blokada osi Pitch na 89 stopniach zapobiega zjawisku Gimbal Lock.

## Sterowanie i Interfejs

*   **W / S / A / D** - Ruch poziomy.
*   **Q / E** - Lot pionowy (oś Y).
*   **Strzałki** - Obrót kamery (Pitch i Yaw).
*   **Podwójne kliknięcie (W/S/A/D/Q/E w 0.4s)** - Tryb "Sprint", x5 prędkości.
*   **Interfejs (ImGui)** pozwala na zaawansowaną konfigurację w locie:
    * Zmiana gęstości pokrywy śnieżnej, skali szumu (Noise Scale) i siły zniekształceń krawędzi śniegu (Distortion).
    * Zarządzanie animacją padającego śniegu (ilość cząsteczek, grawitacja, siła wiatru).
    * Wybór rogu startowego dla algorytmu wyznaczającego trasę (Pathfinding).

## Wymagania (macOS)

```bash
brew install glfw glew glm
```

## Kompilacja i Uruchomienie

```bash
mkdir build && cd build
cmake ..
make
./grafika_app
```

## Galeria

![Symulacja pokrywy śnieżnej](photos/photo1.png)

![Wizualizacja wyznaczonej trasy wspinaczkowej](photos/photo2.png)
