# grafika-hf-template

[![Continuous Integration](https://github.com/levy04/grafika-hf-template/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/levy04/grafika-hf-template/actions/workflows/ci.yml)

Egy template projekt a grafika házifeladatoknak. Nem hivatalos.

## Dependencies

- meson ([Guide](https://mesonbuild.com/Quick-guide.html))

## Setup

A `meson setup build` paranccsal tudod setupolni a projektet, utána pedig a `meson compile -C build` paranccsal fordítani, végül pedig a `./build/out` paranccsal futtatni a programot.

Ha már egyszer setupoltál, akkor újrafordításhoz elég csak a `meson compile -C build` vagy egyszerűen a `ninja -C build` parancs.

Visual Studio Codeban elég egy F5 a `meson setup build` után.

Ha egy clean buildet szeretnél, akkor a `meson setup --wipe build` parancs újragenerálja a build mappát.

#### MSVC jóságok

A `framework.h` egy bizonyos `_HAS_CXX17` flaget vizsgál, hogy definiálva van-e. Tudtommal ezt a flaget csak az MSVC használja, gcc és clang esetén más flagek generálódnak amikor megadod, hogy milyen standardot használsz. A `framework.h` fájl átírása helyett inkább a `-D_HAS_CXX17=1` compiler argumentumot adtam meg.
