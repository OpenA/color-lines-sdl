## Classic «Color Lines» game engine

Based on Peter Kosyh [code](https://code.google.com/p/color-lines).

|    | 🌈 |    |    |    |    | 🔵 |    | 🔴
|----|----|----|----|----|----|----|----|----
|    | ⚪ | 🟢 |    | ⚪ |    | 🔵 |    |
| 🔴 | 🟢 |    |    |    |    |    |    | 🔴
| 🟢 |    | 🟤 |    |    | 🟣 | 🌩️ |    | 🔴
|    | 🟤 | 🟢 | ⚪ | ⚪ | 🟤 |    |    | 🔴
|    | 🟤 |    | 🟡 | 🟤 |    |    | 🟣 |
| 🧨 |    |    | 🟣 |    | 🟣 |    |    |
|    | 🟡 |    |    | 🔴 |    |    | 🟠 | 🟠
|    |    | 🟢 |    |    | 🔴 | 🎨 |    |

🔴🟤🟢🟡🟠🔵🟣⚫⚪🎨🪩💣💥🧨⛈️🌩️💎🌟⭐🌈

#### Build instructions
The following dependencies are required:

`git sdl2 sdl2_mixer sdl2_image`

On macOS (and Linux) you can use https://brew.sh pkg manager to install them.

When everything is ready, do
```sh
./configure [options] # Run with -h to check options you can change.
make
```

You can do the following things with `make` command:
* `make install`   - to install game in to prefix.
* `make run`       - just run game from build directory.
* `make debug run` - build and run debug version.
* `make clean`     - cleanup build directory.

## TODO:
* support «Wonder Lines» game logic and `.level` files
* optional pseudo-gui
