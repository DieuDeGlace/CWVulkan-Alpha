# Owner content setup

AlphaVulkan is a separate engine. Each player supplies an owned Cube World Alpha
installation. The engine and packages contain no Cube World content.

Run the setup tool before playing (substitute your build directory if different):

```bat
build\AlphaSetup.exe --game "path to Cube World Alpha" --check
build\AlphaSetup.exe --game "path to Cube World Alpha" --register AlphaVulkan.ini
```

Setup identifies the supported original `Cube.exe` by SHA-256, checks the required
files and decodes every content-store record. It also loads the interface documents
and both fonts. The supported executable hash is
`84a7a132a84d4282338e7ea45a1940d32066d64e39cbafed8f2418d5a6dc30bf`.
Other Alpha executable versions need a separately verified compatibility profile.
Identification establishes the supported image, not proof of purchase.

Keep these files together in the owned installation:

- `Cube.exe` and `data1.db` through `data4.db`;
- `resource1.dat` and `resource2.dat` (fonts);
- `gui.plx`, `start.plx`, `help.plx`, `quest-tag.plx` and `cursor.plx`.

The base executable is required for identification. The content stores and loose
files supply the assets; the replacement does not execute or patch `Cube.exe`.
The client and dedicated server check the executable and required file presence
at startup. Setup's full check additionally decodes the content.

An optional extraction command creates a private, decoded asset cache:

```bat
build\AlphaSetup.exe --game "path to Cube World Alpha" --extract-to "D:\Private\AlphaAssets"
```

Choose a new directory whose parent already exists. Setup refuses existing
directories, destinations inside the installation and ancestors of the installation.
It preflights flat asset names for Windows devices, invalid names and collisions,
copies seven loose interface/font files, then writes `OWNER-CONTENT.txt` with the
source image hash and SHA-256 of each output. The executable and databases are not
copied. A failed extraction can leave partial private files; use a new directory
for a retry. The completed manifest is written only after all output hashes exist.

The engine currently reads the original stores directly, opened read-only and
immutable. Register the original installation, rather than the optional cache, as
`[Game] Path`. Registration preserves other settings and replaces the configuration
through a temporary file. Extracted assets are owner data and must remain outside
source control and engine distributions.

INI registration handles indented keys/sections and comments without confusing
commented examples with the real Game section. Paths are quoted literals so
Unicode and semicolons survive configuration reloads.

This end-user tool is separate from `tools/alpha-extractor`, the developer-only
32-bit recorder used in an isolated native-game copy for behavioral comparisons.
