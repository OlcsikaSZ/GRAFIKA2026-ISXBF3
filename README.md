# Virtual Gallery – Interactive Museum Room

**C / OpenGL / SDL2** alapú 3D “mini-múzeum”: bejárható terem first-person kamerával, OBJ modellekkel, textúrázással, állítható megvilágítással, időalapú animációval, valamint egérrel kijelöléssel (picking) és stencil outline kiemeléssel.

A projekt célja egy interaktív múzeumszoba megvalósítása, ahol több grafikai funkció egyszerre jelenik meg (kamera, modellek, fények, animáció, scene betöltés), és erre építve plusz effektek (picking, korlátozás/ütközés, köd, kiemelés).

---

## Röviden

A program egy virtuális múzeumtermet jelenít meg, ahol a felhasználó:
- WASD + egér vezérléssel mozog (FPS kamera),
- külső OBJ modelleket jelenít meg (scene.csv alapján),
- textúrázott fal/padló/képek és modellek láthatók (SDL2_image),
- van időalapú animáció (szobor forgása kapcsolható),
- van állítható megvilágítás (Numpad + / -),
- egérkattintással lehet kijelölni objektumokat (picking),
- a kijelölt objektum stencil outline kiemelést kap,
- megjelenik egy info panel a kijelölésről,
- van “Human mode” (emberi szemmagasság + járás-szerű mozgás), kapcsolható.

---

## Specification

Projekt: “Virtual Gallery – Interactive Museum Room”

Leírás:  
A program egy bejárható, virtuális múzeumtermet jelenít meg. A felhasználó egérrel és billentyűzettel mozog a térben (first-person kamera). A teremben több, fájlból betöltött 3D modell található, valamint textúrázott felületek (fal/padló és képek). A megvilágítás intenzitása futás közben állítható.  
A program interaktív: a felhasználó egérrel kijelölhet objektumokat (picking), a kijelölt elem stencil outline kiemelést kap, és a képernyőn megjelenik egy rövid info panel a kijelölésről.  
A mozgás két módban használható: szabad repülés és ember mód (szemmagasság + “walk” mozgás).

Kötelező elemek:
- Kamera bejárás: egér + WASD
- Modellek: külső fájlból (OBJ)
- Animáció: időalapú forgás (szobor)
- Textúrák: fal/padló/képek + modellek textúrázása (SDL2_image)
- Fények: intenzitás állítás
- F1: súgó overlay / segítség

Plusz funkciók (min. 3):
- Picking (egérkattintás) + kijelölés kiemelése (stencil outline)
- Human mode (szemmagasság + “walk” jellegű mozgás)
- Térkorlátozás / clamp (ne lehessen a padlón/plafonon “átmenni”)
- (később bővíthető: AABB ütközés tárgyakkal, fog, shadow)

---

## Jelenlegi állapot

Már működik:
- FPS kamera: WASD + egér
- Szabad repülés: Q/E fel/le mozgás
- Human mode (emberi szemmagasság + járás): B kapcsoló
- Terem korlátozás / clamp: padlón és plafonon nem lehet átmenni
- OBJ betöltés külső fájlból (ext/obj loader)
- Scene betöltés CSV-ből: assets/config/scene.csv
- Textúrázás SDL2_image-vel (fal/padló/képek/modellek)
- Időalapú animáció: szobor forgása (kijelöléssel kapcsolható)
- Megvilágítás + intenzitás állítás: Numpad + / Numpad -
- Picking (egérrel kijelölés)
- Kijelölt objektum kiemelése: stencil outline
- Info panel: kijelölt objektum neve / rövid segítség a képernyőn
- F1: help / controls overlay megjelenítése

---

## Tervezett (következő nagy modulok)

- Árnyék (shadow): egyszerű megoldás (pl. blob shadow / planar)
- AABB ütközés tárgyakkal (ne csak fal/terem clamp legyen)
- Köd (fog) paraméterezve

---

## Irányítás (Controls)

Mozgás / kamera:
- W / S – előre / hátra
- A / D – balra / jobbra
- Mouse – körbenézés (kamera forgatás)
- Q / E – fel / le (szabad repülésben)

Módváltás:
- B – Human mode (emberi szemmagasság + walk mozgás) be/ki

Interakció:
- Left Click – picking (kijelölés)
  - kijelöléskor: stencil outline kiemelés + info panel
  - szobor kijelölésekor: animáció (forgás) kapcsolható

Fény:
- Numpad + – fényintenzitás növelése
- Numpad - – fényintenzitás csökkentése

Egyéb:
- F1 – súgó / controls overlay
- ESC – kilépés

---

## Mappaszerkezet

```
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
      (fal/padló/képek/modellek textúrái)
  ext/
    obj/
      include/obj/...
      src/...
demos/
  (példák / segédanyagok)
```

---

## Scene konfiguráció (scene.csv)

A jelenlegi terem tartalma itt van definiálva:
- app/assets/config/scene.csv

Formátum:
- type,model,texture,px,py,pz,rx,ry,rz,sx,sy,sz

Megjegyzés:
- type=statue → animálható (forgás)
- A texture mező alapján történik a textúrázás

---

## Fordítás és futtatás

Windows (MinGW + SDL2 / kurzus SDK)

Fordítás (app/ mappában):
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

Megjegyzés: A projekt a kurzus SDK-s környezethez igazodik (SDL2 + SDL2_image).

---

## Technikai áttekintés (röviden)

- Kamera / mozgás: src/camera.c
- Scene betöltés: src/csv.c + src/scene.c
- OBJ betöltés/rajzolás: ext/obj
- Textúrázás: src/texture.c (SDL2_image)
- Animáció: időalapú frissítés (szobor forgás)
- Picking: egérkattintás → kijelölt entity
- Kiemelés: stencil buffer alapú outline
- Overlay / help / info panel: src/help.c (és kapcsolódó modulok)

---

## Szerző

Készítette: Gál Olivér István (ISXBF3)
