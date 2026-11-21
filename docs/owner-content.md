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

## Play an original single-player save

Close Cube World Alpha, AlphaVulkan and any save editors. In a fresh engine
installation, import the original installation's `Save` folder before launching
the client. Supply the full path to the new `Save` directory **beside the
AlphaVulkan.exe you will run**. For a client in `build`:

```bat
build\AlphaSetup.exe --game "path to Cube World Alpha" --import-saves-to "build\Save"
build\AlphaVulkan.exe --start
```

Choose the imported character and world from the menus. Each copied world uses
the engine's required swimmable water and bank reeds. The importer pins the
unmodded generation profile. Leave `[Mods] Load` empty when opening the copy.
Characters, inventories, stored world changes and discovery maps come from the
copy. The original installation stays available to the original game.

The destination's parent must already exist on a local drive. If the client
already has a `Save` directory, keep it and import beside a separate fresh copy
of the engine. Setup checks both list databases and each active world's existing
state and map files. A map-only entry retains its map. An entry for a world
with no saved state keeps its place in the list. Setup copies the native
records and adds the engine profile to each staged world. It publishes the
directory only after every database passes. Other files in the folder are
skipped, including unrelated dedicated-server worlds and orphan databases.

This import supports closed, rollback-mode Alpha databases. Setup rejects
open writers or SQLite journal files for the selected databases, links or
junctions on the selected paths, damaged native records, unsafe world names
and saves containing engine extension records. Registration and asset
extraction retain their usual options. Keep imported saves private and outside distributions.
