# Varnam Fcitx5

A wrapper to add Varnam Input Method Engine support in Fcitx5 Input Method.

> The project is still a work in progress. Please report bugs by raising [issues](https://github.com/varnamproject/varnam-fcitx5/issues).

## Dependencies

* [Meson](https://github.com/mesonbuild/meson)
* A c++ compiler that supports c++17 standard.
* [Varnam](https://github.com/varnamproject/govarnam)
* [Fcitx5](https://github.com/fcitx/fcitx5)

Install `fcitx5-modules-dev` if you're building it on a debian based distribution.

## Build & Install

```bash
git clone https://github.com/varnamproject/varnam-fcitx5.git
cd varnam-fcitx5
meson setup builddir
cd builddir
meson compile
sudo meson install
```

To enable debug logs set `varnam_debug` to `true` by using the following command, before compiling the project. 

```bash
meson configure -Dvarnam_debug=true
```

If meson version is less than `1.1` run the following command before `meson setup`.

```bash
mv meson.options meson_options.txt
```

## Uninstall
```
cd buildir
sudo ninja uninstall
```

## Configuration

Varnam Fcitx can be configured using `fcitx5-configtool`. Please refer the [official documentation](https://fcitx-im.org/wiki/Configtool_(Fcitx_5)).

![Config Tool](assets/screenshot-fcitx-configtool-01.png)
