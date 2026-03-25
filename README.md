# Virtual Gallery – Interactive Museum Room

C / OpenGL / SDL2 alapú, bejárható 3D virtuális galéria. A program first-person kamerát, külső fájlból betöltött OBJ modelleket, textúrázott környezetet, állítható fényeket, időalapú animációt, egérrel történő objektumkijelölést, stencil outline kiemelést, egyszerű árnyékvetítést és alap ütközéskezelést valósít meg.

A projekt célja egy esztétikus, interaktív múzeumszoba elkészítése, ahol a kötelező grafikai elemek mellett több látványos plusz funkció is működik egyetlen, egységes programban.

---

## Röviden

A program egy virtuális múzeumtermet jelenít meg, ahol a felhasználó:
- WASD + egérrel mozoghat first-person nézetben,
- külső OBJ modelleket tölt be a `scene.csv` konfiguráció alapján,
- textúrázott falakat, padlót, plafont, festményeket és szobrokat lát,
- időalapú animációt kapcsolhat a szobrokhoz,
- futás közben állíthatja a fény intenzitását,
- egérkattintással kijelölhet objektumokat,
- a kijelölt objektum stencil outline kiemelést és információs panelt kap,
- bekapcsolhat egy „Human mode” sétamódot,
- egyszerű árnyékvetítést és objektum-ütközéskezelést is használhat.

---

## Specification

Projekt: **Virtual Gallery – Interactive Museum Room**

Leírás:
A program egy bejárható, virtuális múzeumtermet jelenít meg. A felhasználó billentyűzettel és egérrel mozog a térben first-person kamerával. A teremben több, fájlból betöltött 3D modell található, valamint textúrázott felületek és festmények. A megvilágítás intenzitása futás közben állítható.

A program interaktív: az egérrel objektumok jelölhetők ki (picking), a kijelölt elem stencil outline kiemelést kap, és a képernyőn egy rövid információs panel jelenik meg róla. A mozgás két módban használható: szabad mozgás és emberi szemmagasságú sétamód.

### Kötelező elemek
- Kamera bejárás: egér + WASD
- Modellek: külső fájlból (OBJ)
- Animáció: időalapú forgás
- Textúrák: környezet és modellek textúrázása
- Fények: intenzitás állítás billentyűzettel
- F1: súgó overlay / használati útmutató

### Megvalósított plusz funkciók
- Picking egérkattintással
- Stencil buffer alapú outline kiemelés a kijelölt objektumon
- Human mode / járás jellegű mozgás szemmagassággal
- Egyszerű ütközéskezelés a jelenet objektumaival
- Átlátszó vitrin megjelenítése
- Egyszerű planar shadow vetítés
- Teljes képernyős mód és átméretezhető ablak

---

## Jelenlegi állapot

A programban jelenleg működik:
- FPS kamera: WASD + jobb egérgombbal körbenézés
- Szabad mozgás: Q / E fel-le mozgás
- Human mode (emberi szemmagasság + járásérzet): `B`
- OBJ betöltés külső fájlból
- Scene betöltés CSV-ből: `app/assets/config/scene.csv`
- Textúrázás SDL2_image használatával
- Időalapú animáció: szobrok forgása kapcsolható
- Fényintenzitás állítás: numpad `+` / `-`, valamint fő billentyűzeti `+` / `-`
- Picking egérrel
- Kijelölt objektum kiemelése stencil outline technikával
- Információs panel a kijelölt objektumról
- F1 help overlay
- Egyszerű planar shadow megjelenítés
- Egyszerű objektum-ütközéskezelés
- Átlátszó vitrin renderelés
- F11 vagy Alt+Enter teljes képernyős váltás

---

## Irányítás (Controls)

### Mozgás / kamera
- `W` / `S` – előre / hátra
- `A` / `D` – balra / jobbra
- jobb egérgomb nyomva tartva + egérmozgatás – körbenézés
- `Q` / `E` – fel / le (csak szabad mozgásban)

### Módváltás és effektek
- `B` – Human mode be/ki
- `R` – animáció be/ki
- `H` – árnyékok be/ki
- `F11` vagy `Alt+Enter` – teljes képernyő be/ki

### Interakció
- bal kattintás – picking / objektumkijelölés

### Fény
- numpad `+` vagy fő billentyűzeti `+` – fényintenzitás növelése
- numpad `-` vagy fő billentyűzeti `-` – fényintenzitás csökkentése

### Egyéb
- `F1` – súgó / controls overlay
- `ESC` – kilépés

---

## Mappaszerkezet

```text
app/
  Makefile
  src/
    app.c
    camera.c
    scene.c
    csv.c
    texture.c
    help.c
    utils.c
    main.c
  include/
    (header fájlok)
  assets/
    config/
      scene.csv
    models/
      (OBJ modellek)
    textures/
      (textúrák)
  ext/
    obj/
      include/obj/...
      src/...
demos/
  (órai gyakorlati feladatok)
```

---

## Scene konfiguráció (`scene.csv`)

A jelenlegi terem tartalma itt van definiálva:
- `app/assets/config/scene.csv`

Formátum:
- `type,model,texture,px,py,pz,rx,ry,rz,sx,sy,sz`

Megjegyzés:
- `type=statue` esetén az objektum animálható
- a `texture` mező alapján történik a textúrázás

---

## Fordítás és futtatás

Windows (MinGW + SDL2 / kurzus SDK)

Fordítás az `app/` mappában:
```bash
make
```

Futtatás:
```bash
./museum.exe
```

Takarítás:
```bash
make clean
```

Megjegyzés: a projekt a kurzus SDL2 / SDL2_image környezetéhez igazodik.

---

## Technikai áttekintés

- Kamera és mozgás: `app/src/camera.c`
- Alkalmazáslogika és eseménykezelés: `app/src/app.c`
- Scene betöltés: `app/src/csv.c` + `app/src/scene.c`
- OBJ modellbetöltés és rajzolás: `app/ext/obj`
- Textúrázás: `app/src/texture.c`
- Help / overlay: `app/src/help.c`
- Egyszerű ütközéskezelés és picking: `app/src/scene.c`

---

## Szerző

Készítette: Gál Olivér István (ISXBF3)
