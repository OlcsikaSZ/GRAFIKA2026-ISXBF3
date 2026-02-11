# 🏛️ Virtual Gallery – Interactive Museum Room

**C / OpenGL / SDL2** alapú 3D “mini-múzeum”: bejárható terem first-person kamerával, fájlból betöltött OBJ modellekkel, állítható megvilágítással és időalapú animációval.

A projekt célja egy **interaktív múzeumszoba** megvalósítása, ahol több grafikai funkció egyszerre jelenik meg (kamera, modellek, fények, animáció, scene betöltés), és erre építve plusz effektek (picking, ütközés, köd, textúrázás).

---

## ✨ Röviden

A program egy virtuális múzeumtermet jelenít meg, ahol a felhasználó:
- **WASD + egér** vezérléssel bejárhatja a teret (FPS kamera),
- több **külső fájlból betöltött OBJ modell** látható (scene.csv alapján),
- van **időalapú animáció** (a `type=statue` objektum folyamatosan forog),
- van **megvilágítás**, aminek intenzitása futás közben állítható (**numpad + / -**),
- a padló/terem alap geometriája is megjelenik (plane).

---

## ✅ Specification (beadható, konkrét leírás)

**Projekt: “Virtual Gallery – Interactive Museum Room”**

**Leírás:**  
A program egy bejárható, virtuális múzeumtermet jelenít meg. A felhasználó egérrel és billentyűzettel mozog a térben (first-person kamera). A teremben több, fájlból betöltött 3D modell található (szobrok, vitrin, pad, oszlopok), valamint a falakon textúrázott “képek” láthatók. A megvilágítás több fényforrásból áll (pl. fő mennyezeti fény + spot a kiemelt tárgyra), amelyek intenzitása futás közben állítható.  
A program interaktív: a felhasználó kijelölhet tárgyakat, mozgatni/forgatni tudja őket, és bizonyos elemek animáltak (pl. lassan forgó kiemelt szobor, nyíló ajtó, pulzáló spotfény).  
A kezelési útmutató **F1** megnyomására jelenik meg a képernyőn.

**Kötelező elemek:**
- Kamera bejárás: egér + WASD
- Modellek: külső fájlból (OBJ)
- Animáció: időalapú forgás/ajtó animáció
- Textúrák: falak/képek és modellek textúrázása
- Fények: +/- intenzitás állítás
- F1: súgó overlay

**Tervezett plusz funkciók (min. 3):**
- Egérrel kijelölés (picking) + kijelölt tárgy kiemelése (outline vagy színváltás)
- Ütközésvizsgálat (AABB): falakon, tárgyakon nem lehet átsétálni
- Köd (fog) állítható paraméterrel (F2/F3)  
  (+ opcionális: átlátszó vitrin, egyszerű árnyék)

---

## 📌 Jelenlegi állapot (a beadott ZIP alapján)

**Már működik:**
- ✅ FPS kamera: **WASD + egér**, valamint **Q/E** fel/le mozgás
- ✅ OBJ modellek betöltése külső fájlból (`ext/obj` loader)
- ✅ Scene betöltés CSV-ből: `assets/config/scene.csv`
- ✅ Időalapú animáció: `type=statue` objektum **folyamatosan forog**
- ✅ Megvilágítás + intenzitás állítás: **Numpad + / Numpad -**
- ✅ Padló kirajzolva (plane)

**Előkészítve / részben kész:**
- 🟡 Textúrázás: van `texture.c` (SDL_image), és a scene-ben már szerepel a texture mező, de jelenleg a render még nem köti be minden modellre (`texture_id` alapértelmezésben 0).

**Tervezett (még nincs bekötve ebben a verzióban):**
- ❌ F1 help overlay a képernyőn (jelenleg debug jellegű)
- ❌ Picking + kiemelés
- ❌ AABB ütközés
- ❌ Köd (fog)

---

## 🎮 Irányítás (Controls)

**Mozgás / kamera:**
- `W` / `S` – előre / hátra
- `A` / `D` – balra / jobbra
- `Q` / `E` – fel / le (vertikális mozgás)
- `Mouse` – körbenézés (kamera forgatás)

**Fény:**
- `Numpad +` – fényintenzitás növelése
- `Numpad -` – fényintenzitás csökkentése

**Egyéb:**
- `ESC` – kilépés
- `F1` – jelenleg debug jelzés (később: help overlay)

---

## 🗂️ Mappaszerkezet

```
app/
  Makefile
  museum.exe
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
      duck.obj
    textures/
      duck.jpg
  ext/
    obj/
      include/obj/...
      src/...
demos/
  (példák / segédanyagok)
```

---

## 🧩 Scene konfiguráció (scene.csv)

A jelenlegi terem tartalma itt van definiálva:
- `app/assets/config/scene.csv`

**Formátum:**
- `type,model,texture,px,py,pz,rx,ry,rz,sx,sy,sz`

**Példa sor:**
- `statue,assets/models/duck.obj,assets/textures/duck.jpg,0,0,-3,0,0,0,1,1,1`

Megjegyzés:
- `type=statue` → automatikusan **animált** (forgás).
- A `texture` mező már jelen van, a textúra-bekötés jelenleg fejlesztés alatt.

---

## 🏗️ Fordítás és futtatás

### Windows (MinGW + SDL2 / kurzus SDK)

Lépj be az `app/` mappába, majd fordíts:

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

Megjegyzés: A projekt a kurzusos/SDK-s környezethez igazodik. Ha a fordításhoz külön `.bat` vagy SDK shell kell, azt a beadandó környezet szerint kell indítani.

---

## 🧠 Technikai áttekintés (röviden)

- **Kamera:** `src/camera.c` kezeli a first-person mozgást és a nézetet.
- **Scene betöltés:** `src/csv.c` beolvassa a `scene.csv`-t, `src/scene.c` létrehozza az entity-ket.
- **OBJ betöltés/rajzolás:** `ext/obj` modul (OBJ loader + draw).
- **Animáció:** időalapú frissítés a scene update-ben (statue forgás).
- **Lighting:** OpenGL fixed pipeline fény, intenzitás szorzóval állítható.

---

## 🚀 Tervezett bővítések (plusz funkciók)

A “múzeum” jelleghez és a grafikai követelményekhez illő következő lépések:
- 🎯 Picking + kiemelés (objektum kijelölése egérrel)
- 🧱 AABB ütközés (ne lehessen falon/tárgyon átmenni)
- 🌫️ Köd (fog) paraméterezve (F2/F3)
- 🖼️ Textúrák teljes bekötése (modellek és fal-képek)

---

## 👤 Szerző

Készítette: **Gál Olivér István (ISXBF3)**
