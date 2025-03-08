# grafika-hf-template

Linuxon tesztelve. Nem hivatalos.

## Dependencies

- meson ([Guide](https://mesonbuild.com/Quick-guide.html))

## Fordítás

A `meson setup build` paranccsal tudod setupolni a projektet, utána pedig a `meson compile -C build` paranccsal fordítani, végül pedig a `./build/out` paranccsal futtatni a programot.

Ha már egyszer setupoltál, akkor újrafordításhoz elég csak a `meson compile -C build` vagy egyszerűen a `ninja -C build` parancs.

Visual Studio Code felhasználóknak elég egy F5 a setup után.

Ha egy clean buildet szeretnél, akkor a `meson setup --wipe build` parancs újragenerálja a build mappát. Ha ez sem elég, akkor töröld ki, és setupold a projektet újra.

### MSVC jóságok

A `framework.h` egy bizonyos `_HAS_CXX17` flaget vizsgál, hogy definiálva van-e. Tudtommal ezt a flaget csak az MSVC használja, gcc és clang esetén más flagek generálódnak amikor megadod, hogy milyen standardot használsz. A `framework.h` fájl átírása helyett én inkább a `-D_HAS_CXX17=1` compiler arg-ot adtam meg. Ha valami C++ chad tud erre valami szebb megoldást, akkor szeretettel várom a PR-okat.
