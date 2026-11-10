# CWVulkan-Alpha

A Vulkan engine for **Cube World Alpha**, built the way CubeVulkan is built for the Steam
release: like OpenMW for Morrowind, it is its own engine and reads the content of the owner's
own Alpha install. No game content is included in this repository or its builds.

Status: development snapshot, before beta. Native 1:1 behavior and visual acceptance
are still being verified; the inherited implementation and specifications also
remain subject to provenance review. Known issues include bright daytime terrain,
HUD overlap with large panels, incomplete combat/frame timing and unfinished native
interactive acceptance. See the [beta gates](docs/beta-release.md).

The engine reads Alpha's content stores and saves, generates Alpha's worlds from
a seed (terrain, locations, towns, lakes, rock formations, roads, trees, plants, clouds),
streams them round the camera with Alpha's look plus shadows, the moon, swaying plants,
drifting clouds, the Steam engine's shared water surface and TAA + SMAA, plays on Alpha's network protocol with a
dedicated server, and supports a C DLL SDK plus source modules compiled into the engine. See [docs/roadmap.md](docs/roadmap.md) for
what is done and what comes next.

The runtime's swimmable shallows and bank reeds are explicit extensions; see
[water behavior and compatibility](docs/water-extension.md). Test and native
comparison commands are in [verification.md](docs/verification.md).
Adaptation price fixes and engine quests are built-in core features, enabled
automatically. These and the requested water changes are intentional extensions
alongside the native behavior reference.
Controller gameplay and menu mappings are documented in
[controller support](docs/controller-support.md), with physical acceptance limits
kept separate from supplied-state tests.

## Build

Build on 64-bit Windows with Git for Windows and Visual Studio 2022 Build Tools
or Visual Studio 2022 Community. Install **Desktop development with C++**, including
the MSVC x64/x86 tools, a Windows SDK and **C++ CMake tools for Windows** (CMake and
Ninja). Internet access is needed for the first dependency fetch. Running the
client requires a Vulkan 1.3-capable GPU and driver.

No Vulkan SDK or Python is needed. Dependencies are fetched as source at pinned
versions, and a small glslang tool compiles the shaders during the build.

```bat
git clone https://github.com/DieuDeGlace/CWVulkan-Alpha.git
cd CWVulkan-Alpha
build.cmd --parallel 4
```

Run these commands from Command Prompt. `build.cmd` selects the x64 compiler and
builds the client, dedicated server, owner-content setup tool, SDK example and
test tools. A fresh checkout writes executables to `build\`; set
`ALPHAVULKAN_BUILD_DIR` before invoking the script to choose another build folder.

For a manual build, open **x64 Native Tools Command Prompt for VS 2022**:

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

To include content/save/UI checks with your owned installation, configure with
`-DALPHAVULKAN_TEST_GAME_PATH="path to Cube World Alpha"`. Some tests skip their
owner-content checks if that installation is unavailable. See
[verification instructions](docs/verification.md) for the distinction between
synthetic checks and native acceptance.

## Run

First validate and register your owned installation with
`build\AlphaSetup.exe --game "path to Cube World Alpha" --register AlphaVulkan.ini`.
See [owner content setup and private extraction](docs/owner-content.md) for the
required executable/assets and optional decoded cache.

```bat
build\AlphaVulkan.exe                         :: model viewer (Left/Right, drag, wheel)
build\AlphaVulkan.exe --model airship.cub
build\AlphaVulkan.exe --list .cub             :: keys in the stores (.cub .wav .png .xml)
build\AlphaVulkan.exe --model airship.cub --capture shot.png

build\AlphaVulkan.exe --world --seed 123      :: fly over a world (WASD, Q/E, drag, Shift)
build\AlphaVulkan.exe --world --seed 123 --at 8389628 8401500 --pitch 40 --hour 18
build\AlphaVulkan.exe --world --save-world mine --character 0   :: a saved single-player world
build\AlphaServer.exe                         :: a dedicated server on port 12345
build\AlphaVulkan.exe --world --connect 127.0.0.1 --name Me

build\StoreTests.exe                          :: and the other *Tests.exe: the spec checks
```

The install is taken from `AlphaVulkan.ini` `[Game] Path` (see `config/`), or `--game DIR`,
or looked for in the usual folders. The stores are opened read-only and immutable; nothing in
the install is ever written. Saved worlds go to the `Save` folder beside the executable, in
Alpha's own format.

`AlphaServer --bind 127.0.0.1` runs a local test server. The default bind address is
`0.0.0.0`, accepting connections on all IPv4 interfaces. `--port 0` chooses a free
port and prints it at startup.

## Modding

See [docs/modding.md](docs/modding.md), the SDK headers in `sdk/`, the TimeLapse
DLL example and the opt-in StoneTint engine-module example under `examples/`.

## How it is written

Clean-room, as CubeVulkan (see [the process](docs/CLEANROOM.md)): behaviour is first written as a
plain-language spec in `docs/specs/` from the Alpha decompile, then implemented from the spec
alone, then checked against the game's own output.

The continuation is developed in this workspace; the original Claude workfiles
and owner's game installation are retained separately. Inherited parity claims
remain subject to native verification and a final provenance audit.

## License

GPL-3.0-only for AlphaVulkan's own code. Cube World and its content belong to their owners.
