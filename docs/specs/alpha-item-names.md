Status: FINAL / STABLE / HELD

# Alpha items: how an item record becomes a displayed name

Behavioural specification of the item-name routine of Alpha's client image `Cube.exe`
(image base 0x400000; every address below is an RVA in `Cube.exe`) and of the two places
that show coin names. The routine's entry is RVA **0x198a50**. Two side sections (§9, §10)
settle open points I-1 and I-2 of [alpha-ui-screens.md](alpha-ui-screens.md) §8.1.12.

Dictionary text is not reproduced here: words and patterns are named by their dictionary key
and child element only. Short string literals quoted below are constants of the `Cube.exe`
image itself and are cited by RVA.

Callers of the name routine (all invoke it on the world object, game object + 0x2e4): the
tooltip text block (0xa28c0, call at 0xa2a63), the chat pickup announcement (0x80fb0, at
0x81333), the frame routine (0x88ee0, at 0x8b564 and 0x97cd3), the crafting/vendor wide rows
(0x2f910 at 0x30d0a; 0xc2050 at 0xc42a2 and 0xc4792), and itself (formulas, §4).

---

## 0. Conventions

* **Item record**: the 280-byte (0x118) record of `alpha-net-packets.md` §1.1 and
  `alpha-creatures-behaviour.md` §10.1. Fields read here:

  | Off | Type | Name here |
  |---|---|---|
  | +0x00 | u8 | **type** |
  | +0x01 | u8 | **subtype** |
  | +0x04 | u32 | **seed** |
  | +0x08 | u32 | **product** (formulas, §4) / **owner id** (quest items, §6) |
  | +0x0c | u8 | **rarity** |
  | +0x0d | u8 | **material** |
  | +0x10 | i16 | level; for coins the **quantity** (§7) |
  | +0x14 | 8-byte spirit entries | byte 3 of each entry = spirit material |
  | +0x114 | i32 | spirit count |

  The level never enters the name.
* **Dictionary**: the world object's dictionary at world + 0x30 (game object + 0x314),
  loaded from `dict_en.xml` in `data4.db`. It maps each `<name key="K">` element to an entry
  holding (a) its child elements by element name (child name → text) and (b) a list of tag
  words taken from its `tags` attribute *(the splitting of the attribute into words is not
  confirmed, see N-3)*. **D(K, c)** below means "the text of child `c` of the entry for key
  K"; a missing key or a missing child gives the **empty string** (0x80e00). Keys and child
  names are compared as exact UTF-16 code-unit sequences (case-sensitive, ordinal).
  `MaterialIce` occurs twice in `dict_en.xml` with identical content, so which copy the
  loader keeps does not affect names.
* All strings are UTF-16; nothing is trimmed, re-cased or re-spaced by the routine.
* **"@"** and **"#"** are placeholder characters inside dictionary patterns. "Replace @ by X"
  always means: find the **first** "@" in the text and replace that one character by X
  (find 0xd9950, replace 0x86d00). Later occurrences are left alone; no English pattern used
  here contains more than one "@" or more than one "#".

---

## 1. The three lookup tables

### 1.1 Item key K(type, subtype) (0x19fbf0)

A sorted map at world + 0x800134 from the exact pair (type, subtype), compared as signed
integers, to a dictionary key. It is filled once in the world constructor (0x18eb00, row
inserts 0x1901ce … 0x191eb4). No row → empty key → every lookup gives "". The item word is
D(K, `singular`).

**Equipment and general items**

| (type, sub) | Key |
|---|---|
| (2, 0) | `Formula` *(no dictionary entry; never looked up, §4)* |
| (3, 0) | `Sword` |
| (3, 1) | `Axe` |
| (3, 2) | `Mace` |
| (3, 3) | `Dagger` |
| (3, 4) | `Fist` |
| (3, 5) | `Longsword` |
| (3, 6) | `Bow` |
| (3, 7) | `Crossbow` |
| (3, 8) | `Boomerang` |
| (3, 9) | `Arrow` |
| (3, 10) | `Staff` |
| (3, 11) | `Wand` |
| (3, 12) | `Bracelet` |
| (3, 13) | `Shield` |
| (3, 14) | `Arrows` |
| (3, 15) | `Greatsword` |
| (3, 16) | `Greataxe` |
| (3, 17) | `Greatmace` |
| (3, 20) | `Torch` |
| (4, 0) | `ChestArmor` |
| (5, 0) | `Gloves` |
| (6, 0) | `Boots` |
| (7, 0) | `ShoulderArmor` |
| (8, 0) | `Amulet` |
| (9, 0) | `Ring` |
| (12, 0) | `Coin` |
| (13, 0) | `PlatinumCoin` |
| (14, 0) | `Leftovers` |
| (15, 0) | `Beak` |
| (16, 0) | `Painting` |
| (18, 0), (18, 1) | `Candle` |
| (19, 0) | `Pet` *(no dictionary entry; never looked up, §5)* |
| (23, 0) | `HangGlider` |
| (23, 1) | `Boat` |
| (24, 0) | `Lamp` |
| (25, 0) | `ManaCube` |

Weapon subtypes 18, 19 and ≥ 21 (and any other pair not listed) have no row and give an
empty word.

**Consumables (type 1)**

| sub | Key | sub | Key |
|---:|---|---:|---|
| 0 | `Cookie` | 5 | `SnowBerryMash` |
| 1 | `LifePotion` | 6 | `MushroomSpit` |
| 2 | `CactusPotion` | 7 | `Bomb` |
| 3 | `ManaPotion` | 8 | `PineappleSlice` |
| 4 | `GinsengSoup` | 9 | `PumpkinMuffin` |

**Ingredients (type 11)**

| sub | Key | sub | Key |
|---:|---|---:|---|
| 0 | `Nugget` | 14 | `Spirit` |
| 1 | `Log` | 15 | `Mushroom` |
| 2 | `Feather` | 16 | `Pumpkin` |
| 3 | `Horn` | 17 | `Pineapple` |
| 4 | `Claw` | 18 | `RadishSlice` |
| 5 | `Fiber` | 19 | `ShimmerMushroom` |
| 6 | `Cobweb` | 20 | `GinsengRoot` |
| 7 | `Hair` | 21 | `OnionSlice` |
| 8 | `Crystal` | 22 | `Heartflower` |
| 9 | `Yarn` | 23 | `PricklyPear` |
| 10 | `Cube` | 24 | `FrozenHeartflower` |
| 11 | `Capsule` | 25 | `Soulflower` |
| 12 | `Flask` | 26 | `WaterFlask` |
| 13 | `Orb` | 27 | `SnowBerry` |

**Pet food (type 20)** — the subtype is the creature type the food is for. First **every**
subtype 0 … 155 is given the key `Bait` (loop insert at 0x1906fd), then these rows overwrite
it. The creature column gives the creature key that the creature key table (world + 0x800104,
filled in the same constructor) holds for that creature type:

| sub | creature key | Key |
|---:|---|---|
| 19 | `Collie` | `BubbleGum` |
| 22 | `Alpaca` | `VanillaCupcake` |
| 23 | `BrownAlpaca` | `ChocolateCupcake` |
| 25 | `Turtle` | `CinnamonRole` |
| 26 | `Terrier` | `Waffle` |
| 27 | `ScottishTerrier` | `Croissant` |
| 30 | `Cat` | `Candy` |
| 33 | `Pig` | `PumpkinMash` |
| 34 | `Sheep` | `CottonCandy` |
| 35 | `Bunny` | `Carrot` |
| 36 | `Porcupine` | `BlackberryMarmelade` |
| 37 | `GreenSlime` | `GreenJelly` |
| 38 | `PinkSlime` | `PinkJelly` |
| 39 | `YellowSlime` | `YellowJelly` |
| 40 | `BlueSlime` | `BlueJelly` |
| 50 | `Monkey` | `BananaSplit` |
| 53 | `Hornet` | `Popcorn` |
| 55 | `Crow` | `LicoriceCandy` |
| 56 | `Chicken` | `CerealBar` |
| 57 | `Seagull` | `SaltedCaramel` |
| 58 | `Parrot` | `GingerTartlet` |
| 59 | `Bat` | `MangoJuice` |
| 60 | `Fly` | `FruitBasket` |
| 61 | `Midge` | `MelonIceCream` |
| 62 | `Mosquito` | `BloodOrangeJuice` |
| 63 | `PlainRunner` | `MilkChocolateBar` |
| 64 | `LeafRunner` | `MintChocolateBar` |
| 65 | `SnowRunner` | `WhiteChocolateBar` |
| 66 | `DesertRunner` | `CaramelChocolateBar` |
| 67 | `Peacock` | `ChocolateCookie` |
| 74 | `Duckbill` | `SugarCandy` |
| 75 | `Crocodile` | `AppleRing` |
| 86 | `Spitter` | `WaterIce` |
| 87 | `Mole` | `ChocolateDonut` |
| 88 | `Biter` | `Pancakes` |
| 90 | `Squirrel` | `StrawberryCake` |
| 91 | `Raccoon` | `ChocolateCake` |
| 92 | `Owl` | `Lollipop` |
| 93 | `Penguin` | `Softice` |
| 98 | `Horse` | `CandiedApple` |
| 99 | `Camel` | `DateCookie` |
| 102 | `BarkBeetle` | `Bread` |
| 103 | `FireBeetle` | `Curry` |
| 104 | `SnoutBeetle` | `Lolly` |
| 105 | `LemonBeetle` | `LemonTart` |
| 106 | `Crab` | `StrawberryCocktail` |
| 151 | `Bumblebee` | `BiscuitRole` |

Every other subtype 0 … 155 keeps `Bait`; subtypes ≥ 156 have no row (empty word).

**Quest items (type 21)** (named by §6)

| sub | Key | sub | Key |
|---:|---|---:|---|
| 0 | `Amulet1` | 5 | `Antivenom` |
| 1 | `Amulet2` | 6 | `BandAid` |
| 2 | `JewelCase` | 7 | `Crutch` |
| 3 | `Key` | 8 | `Bandage` |
| 4 | `Medicine` | 9 | `Salve` |

Types 0, 10, 17, 22 and ≥ 26 have no rows.

Every key in these tables other than `Formula` and `Pet` has an entry with a `singular` child
in `dict_en.xml`.

### 1.2 Material key M(material) (0x19ff60)

A fixed switch on the material number (byte table at 0x1a0248, jump table at 0x1a01e0; values
outside 1 … 131 take the default). The dictionary child used for item names is **`item`**, a
pattern containing one "@" for the item word; the chat coin line uses `singular` (§7).

| Material | Key |
|---:|---|
| 1 | `MaterialIron` |
| 2 | `MaterialWood` |
| 5 | `MaterialObsidian` |
| 7 | `MaterialBone` |
| 10 | `MaterialCopper` |
| 11 | `MaterialGold` |
| 12 | `MaterialSilver` |
| 13 | `MaterialEmerald` |
| 14 | `MaterialSapphire` |
| 15 | `MaterialRuby` |
| 16 | `MaterialDiamond` |
| 17 | `MaterialSandstone` |
| 18 | `MaterialSaurian` |
| 19 | `MaterialParrot` |
| 20 | `MaterialMammoth` |
| 21 | `MaterialPlant` |
| 22 and 130 | `MaterialIce` |
| 23 | `MaterialLight` |
| 24 | `MaterialGlass` |
| 25 | `MaterialSilk` |
| 26 | `MaterialLinen` |
| 27 | `MaterialCotton` |
| 128 | `MaterialFire` |
| 129 | `MaterialUnholy` |
| 131 | `MaterialWind` |
| anything else (0, 3, 4, 6, 8, 9, 28 … 127, 132 …) | `Material` (string 0x31d52c; no dictionary entry, so both children give "") |

In `dict_en.xml` every listed material entry has both `singular` and `item`, and each `item`
pattern is the material word followed by a single "@" at its end. All of them separate the
word from "@" with a space except `MaterialLight`, whose English pattern is a German-language
leftover that joins with a hyphen and no space. `MaterialSatin` exists in the dictionary but
no material number selects it.

### 1.3 Adjective key A(rarity, seed mod 10) (switch 0x199013 … 0x199bb6)

Fifty keys, one per (rarity 0 … 4, digit d = seed mod 10, seed as unsigned 32-bit). The
rarity jump table is at 0x199ffc; the digit jump tables are at 0x19a010, 0x19a038, 0x19a060,
0x19a088 and 0x19a0b0 for rarities 0 … 4. The keys, by d = 0 … 9:

* rarity 0: `item:shabby`, `item:plain`, `item:battered`, `item:artless`, `item:unwieldy`,
  `item:used`, `item:dusty`, `item:scratched`, `item:worn`, `item:common`;
* rarity 1: `item:adjusted`, `item:balanced`, `item:battletested`, `item:good`,
  `item:handmade`, `item:fair`, `item:neat`, `item:clean`, `item:undamaged`, `item:flawless`;
* rarity 2: `item:exceptional`, `item:polished`, `item:extraordinary`, `item:exquisite`,
  `item:superb`, `item:unique`, `item:handsome`, `item:grand`, `item:magic`,
  `item:decorated`;
* rarity 3: the same ten rarity-2 keys with the suffix `2` (`item:exceptional2` …
  `item:decorated2`);
* rarity 4: `item:brilliant`, `item:shining`, `item:magnificent`, `item:sublime`,
  `item:pompous`, `item:glorious`, `item:splendid`, `item:famous`, `item:legendary`,
  `item:fabulous`.

Rarity > 4 (unsigned compare at 0x199017): no adjective key is selected (§3 step 6).

Shape of the English entries (`dict_en.xml`): every adjective entry has no `tags` attribute
and exactly one child, `<m>`, containing exactly one "@". Rarity 0 – 2 patterns contain no
"#": the adjective word precedes "@". Rarity 3 and 4 patterns contain exactly one "#" (a
generated name, §3.1): for d = 0, 4 and 6 the "#" opens the pattern in a possessive form
before the adjective and "@"; for the other seven digits the pattern is adjective, "@", then a
connector and "#" at the end. The connector is the English "of" form everywhere except
rarity 4, d = 2 (`item:magnificent`), whose text keeps a German-language connector.

---

## 2. Dispatch by type (0x198a50)

The routine tests the type byte in this order:

| Type | Rule | Section |
|---|---|---|
| 2 | formula: literal "Formula: " + name of the taught item | §4 |
| 19 | pet: literal "Pet: " + creature key | §5 |
| 21 | quest item: generated owner name + literal "'s " + word | §6 |
| 1, 18, 14 | **plain word**: D(K(type, subtype), `singular`) — no material, no adjective (0x198c71 … 0x198c83, 0x199f7f … 0x199fbf) | — |
| anything else | **material + word + adjective** | §3 |

So consumables (type 1), candles (18) and leftovers (14) always show just their word:
material, seed and rarity are ignored for them.

---

## 3. The general rule (material, word, adjective)

Used for every type not listed in §2: weapons and armour (3–9), ingredients (11), coins (12),
platinum coins (13), beaks (15), paintings (16), pet food (20), hang glider/boat (23), lamp
(24), mana cube (25), and the unnamed types (0, 10, 17, 22, ≥ 26).

1. **Spirit tally (no effect on the name).** The routine counts the item's spirits whose
   material byte is ≥ 128 into four counters indexed by material − 128 (0x198c89 …
   0x198cc6), but that storage is reused for other data before the counters are ever read
   (0x198ef2), so spirits never change the name. A spirit material above 131 would increment
   storage beyond the four counters before it is initialised.
2. **Material pattern** P = D(M(material), `item`) (0x198d97 … 0x198dc0).
3. **No "@" in P** (including P empty, i.e. every material without an entry; test at
   0x198e1f): name = D(K(type, subtype), `singular`). **Stop** — no adjective is ever added
   in this case, whatever the seed and rarity (0x198e1f … 0x198ec3).
4. **Word** W = D(K(type, subtype), `singular`) (0x198ec8 … 0x198f71). A missing word gives
   "", so the name is the pattern with its "@" simply removed.
5. name = P with its first "@" replaced by W (0x199003).
6. **Adjective**, only when **seed ≠ 0** (0x199008) and **rarity ≤ 4** (0x199017):
   * key = A(rarity, seed mod 10) (§1.3); its entry gives a set of children by element name
     (a missing adjective entry gives an empty set).
   * The child is chosen by the word's tags (0x199bbb … 0x199d83): the tags of the entry for
     K(type, subtype) are visited in order; at the first tag for which the adjective entry
     has a child of that element name, that child's text is taken. If its text contains "@",
     the first "@" is replaced by the current name and the result becomes the name. Then, if
     the name contains "#", its first "#" is replaced by the generated name G(seed) of §3.1.
     If the "@" replacement happened, the adjective step ends here.
   * Otherwise (no tag matched, or the matched text had no "@"), if the adjective entry has
     any child, its **first** child in ordinal element-name order is used the same way: first
     "@" replaced by the current name (when present), then the first "#" in the name replaced
     by G(seed) (0x199d89 … 0x199ea5).
   * In `dict_en.xml` every adjective entry has only `<m>`, so English names always use the
     `<m>` text, whether reached through a tag `m` or through the fallback.
7. The name is written into a wide string stream (0x199eb8) and the stream's text becomes the
   output (0x199f75); the stream adds nothing.

### 3.1 The generated name G(seed) for "#"

G uses the title maker 0x1a0ed0 (described in `alpha-ui-screens.md` §5.4.1 item 3 and §5.9.1
"The name maker") with:

* n = seed (u32),
* kind k = (7 · seed mod 2³²) mod 11 (unsigned; 0x199d12 … 0x199d20).

There are **no random draws**: the name is a fixed function of the seed. With all arithmetic
unsigned 32-bit with wrap-around:

* a = (⌊n/7⌋ + 13·n) mod 2³², first index i = a mod m;
* b = (7·n + ⌊n/10⌋) mod 2³², second index j = b mod m;
* G = First_k[i] followed directly by Second_k[j] (no space, no case change).

m = 10 for k = 0 … 5 and 7 … 10; the title maker has no case for k = 6, which therefore uses
its 20-entry default pair (m = 20). The tables are constants of `Cube.exe` built on first use
inside 0x1a0ed0:

| k | First (index 0 … m−1) | Second (index 0 … m−1) |
|---:|---|---|
| 0 | El, Li, Dri, Elan, Le, Ly, A, Tho, Da, Les | ric, kun, zic, dor, las, sander, reon, reas, lundra, andor |
| 1 | Ar, A, Si, Mer, Lu, Ari, Su, Sira, Hia, Li | weya, luna, laya, leya, ma, lia, matra, vy, zyna, ly |
| 2 | De, Ro, As, Ge, An, Pat, Wolf, Ale, Mi, Ben | rek, man, gram, rald, rick, ram, sander, kal, ton, ny |
| 3 | Sa, Jen, Kla, Mi, La, Auri, Mela, Alu, Ve, Est | rah, ny, ria, elle, na, ga, ma, riana, rona, una |
| 4 | Gra, Xe, Ur, Bel, Chu, Ki, Go, Zim, Kubo, Raz | zic, nax, tik, tuk, ruk, bo, rix, mi, tok, nor |
| 5 | Kur, Xe, Ki, Am, Zifa, Zu, Ma, Chi, Zi, Mia | ra, lia, bi, ba, ly, ki, bara, mi, zy, xa |
| 6 (default) | Kro, Aru, As, Ge, Kur, Lugo, Iko, Liku, Tero, Var, An, The, Ga, Lan, Dura, Dama, Se, Thal, Nar, San | no, mi, la, sel, ron, ion, kor, lon, rok, tar, den, gar, dar, dara, lan, ka, gor, rior, ria, mor |
| 7 | Cho, Zer, Kaz, Kraz, Zel, Drak, Lo, Raz, Spi, Zen | rux, rek, zic, tac, mec, kor, rax, zor, ro, go |
| 8 | Zi, Zer, Az, Ki, Iza, Drak, Li, May, Spi, Zan | rya, ri, zia, ki, mah, ira, maya, ana, ra, ya |
| 9 | Tor, Gem, Bar, Me, As, Tem, Arak, Ur, Grim, Xor | lox, bart, kor, thos, kun, bur, thor, lok, li, bor |
| 10 | Mum, Grun, Brun, Hei, Mo, Hil, Ur, Lum, Grim, Xor | pie, hild, di, muna, ki, trud, sa, axa, ira, ika |

"#" appears only in rarity-3 and rarity-4 patterns, so only those items carry a generated
name.

**Values of G** (seed → k, i, j → G):

| seed | k | i | j | G |
|---:|---:|---:|---:|---|
| 10 | 4 | 1 | 1 | Xenax |
| 3 | 10 | 9 | 1 | Xorhild |
| 7 | 5 | 2 | 9 | Kixa |
| 12 | 7 | 7 | 5 | Razkor |
| 77 | 0 | 2 | 6 | Drireon |
| 12345 | 10 | 8 | 9 | Grimika |
| 4294967295 | 8 | 3 | 2 | Kizia |

Worked: seed 12345 → 7·12345 = 86415, 86415 mod 11 = 10 → k = 10; a = 1763 + 160485 =
162248 → i = 8 "Grim"; b = 86415 + 1234 = 87649 → j = 9 "ika"; G = "Grimika".

---

## 4. Formulas (type 2) (0x198aac … 0x198b37)

1. Copy the whole record. In the copy: **type := the low byte of the product field
   (+0x08)**, then the whole u32 product field := 0. Subtype, seed, rarity, material, level
   and spirits are kept.
2. Name the copy with this same routine (so the full §2/§3 rules apply to the taught item,
   adjective included).
3. Result = the literal **"Formula: "** (string 0x31d540, trailing space included) followed by
   that name.

The product types and seeds that recipes carry are described in `alpha-items-generator.md`
§3 (not re-checked here). A product type of 1, 14 or 18 gives the literal plus the plain word;
a product of 2 recurses once more, with type 0 (the cleared product field), which then follows
§3 with no item word.

---

## 5. Pets (type 19) and pet food (type 20)

**Pets (0x198b3c … 0x198b68).** Result = the literal **"Pet: "** (string 0x31d554) followed
by the **creature key** that the creature key table (world + 0x800104, read by 0x19aa60; the
same table as `alpha-ui-screens.md` §5.4.1 item 1) holds for the creature type given by the
subtype. The key is concatenated **as is — it is not looked up in the dictionary**. So a pet of
subtype 19 reads "Pet: " + `Collie`, subtype 20 "Pet: " + `ShepherdDog`, subtype 23 "Pet: " +
`BrownAlpaca`; a creature type with no key row (for example 95 or 129) gives "Pet: " alone.
Material, seed and rarity are ignored.

**Pet food (type 20)** goes through the general rule (§3): the word is the food key of §1.1.
With a material that has no pattern (for example 0), P is empty and the name is just the food
word, with no adjective even if seed and rarity are set. *Which material creature drops give
pet food is not confirmed here (N-4).* The tooltip prepends the narrow literal "Pet food: "
(string 0x302870, test at 0xa2a3c … 0xa2a4d) before the name for type 20.

---

## 6. Quest items (type 21) (0x198b6a … 0x198c6c)

Result = O + the literal **"'s "** (string 0x31d560) + D(K(21, subtype), `singular`), where O
is the title maker 0x1a0ed0 applied with **n = the u32 at +0x08** and **kind = the u32 at
+0x04**. The title maker selects its tables by kind: own 10-entry tables for kinds 0, 1, 4, 5
and 7 … 16; kinds 2, 18, 43 and 83 share one 10-entry pair; kinds 3, 45 and 84 share another;
every other kind (6, 17, 19 … , and any value outside its cases) uses the 20-entry default
pair. Rarity, material and spirits are ignored. *Which values the server stores at +0x04 and
+0x08 for quest items is not confirmed (N-2).*

Example: kind 4, n = 1000 → i = (142 + 13000) mod 10 = 2 "Ur", j = (7000 + 100) mod 10 = 0
"zic", O = "Urzic"; a type-21 subtype-4 item reads "Urzic" + "'s " + D(`Medicine`,
`singular`).

---

## 7. Coins

Coins are type **12**, subtype 0, with the coin kind in the **material** — 10 copper,
12 silver, 11 gold — and the quantity in the i16 at +0x10 (`alpha-server-simulation.md` loot
step 4; not re-checked here).

* **Item name** (tooltip, bag rows, the routine): general rule §3, i.e. the material pattern
  with D(`Coin`, `singular`) in place of "@". A coin with a non-zero seed and rarity ≤ 4
  would also get an adjective (*coin seeds at creation not confirmed, N-4*).
* **Chat announcement** (0x80fb0). The line is built as three white (1, 1, 1, 1) segments
  for coins:
  1. the receiver part: the literal "You receive " (string 0x30170c) when the receiving
     creature is the local player (game object + 0x8006d0), otherwise the creature's name
     (narrow text at creature + 0x1168, widened) followed by the literal " receives "
     (string 0x301728);
  2. the coin part (type 12 only, 0x81213 … 0x812de): written into a wide string stream as
     the **quantity** (+0x10 as a signed 16-bit decimal, no padding), the literal " "
     (string 0x2fcfbc), and **D(M(material), `singular`)** — the bare material word, not the
     item name. A coin with a material outside the table gives the quantity and a space
     followed by nothing. No item name and no power is printed for coins;
  3. the literal ".\n" (string 0x30175c).
* For comparison, the other announcement forms (same routine): for types 13, 21, 0, 25, 20,
  24, 23, and type 11 with any subtype other than 14 (tests at 0x812e3 … 0x81307), the item
  part is the quantity (signed 16-bit), the literal " X " (string 0x301754), then the item
  name; for all other types it is the item name followed by the literal " +" (string
  0x2fd424) and the item's power (0xc76a0) as a decimal integer. For non-coin items the item
  part is coloured by 0xc7d20 instead of white; the receiver part and ".\n" stay white.

Type 13 (`PlatinumCoin`) names through §3. Whether it ever reaches the chat line is outside
this routine (see `alpha-ui-screens.md` §8.1.9).

---

## 8. Worked examples (structure)

Written with D(·,·) for dictionary text; "pat(K)" means D(K, `item`) and "word(K)" means
D(K, `singular`).

Iron sword, level 5 (type 3, subtype 0, material 1). The level is irrelevant to the name.

| seed | rarity | Steps | Name |
|---:|---:|---|---|
| 0 | any | pat(`MaterialIron`) with "@" := word(`Sword`); seed 0 → no adjective | base name N |
| 10 | 0 | d 0 → `item:shabby` `<m>`, "@" := N | adjective form of N |
| 10 | 2 | d 0 → `item:exceptional` | adjective form of N |
| 10 | 3 | d 0 → `item:exceptional2`; "#" := G(10) = Xenax (k 4) | possessive form with "Xenax" first |
| 3 | 4 | d 3 → `item:sublime`; "#" := G(3) = Xorhild (k 10) | N inside the pattern, "Xorhild" at the end |
| 12345 | 4 | d 5 → `item:glorious`; "#" := Grimika | "Grimika" at the end |
| 12 | 4 | d 2 → `item:magnificent`; k = 84 mod 11 = 7; i = (1 + 156) mod 10 = 7 "Raz", j = (84 + 1) mod 10 = 5 "kor" | "Razkor" at the end, German connector |
| 10 | 7 | rarity > 4 → no adjective | N |

Other records:

| Record | Name |
|---|---|
| bow (3, 6), material 2, seed 0 | pat(`MaterialWood`) with word(`Bow`) |
| sword (3, 0), material 6, seed 55, rarity 2 | word(`Sword`) only (material 6 has no pattern → **no** adjective) |
| chest (4, 0), material 25, seed 0 | pat(`MaterialSilk`) with word(`ChestArmor`) |
| ring (9, 0), material 11, seed 7, rarity 1 | `item:clean` applied to pat(`MaterialGold`) with word(`Ring`) |
| weapon (3, 18), material 1, seed 0 | pat(`MaterialIron`) with "@" removed (no key row → empty word) |
| health potion (1, 1), any material/seed/rarity | word(`LifePotion`) |
| copper coins (12, 0), material 10, quantity 37 | item name pat(`MaterialCopper`) with word(`Coin`); chat "You receive " + "37 " + word(`MaterialCopper`) + ".\n" |
| nugget (11, 0), material 11, seed 7, rarity 0 | `item:scratched` applied to pat(`MaterialGold`) with word(`Nugget`) |
| nugget (11, 0), material 13, seed 51, rarity 1 | `item:balanced` applied to pat(`MaterialEmerald`) with word(`Nugget`) |
| mushroom (11, 15), material 0, seed 9, rarity 3 | word(`Mushroom`) only (no pattern → no adjective) |
| formula: type 2, product 3, subtype 0, material 1, seed 0 | "Formula: " + pat(`MaterialIron`) with word(`Sword`) |
| formula: type 2, product 3, subtype 0, material 1, seed 42, rarity 1 | "Formula: " + `item:battletested` applied to that name |
| pet (19, 19) | "Pet: " + `Collie` |
| pet food (20, 19), material 0 | word(`BubbleGum`); tooltip prepends "Pet food: " |
| candle (18, 1), material 1 | word(`Candle`) (type 18 ignores material) |

---

## 9. Healing of actions 80/81 (open point I-1)

Inside the world step (0x20c510, at 0x21891e … 0x218bdd), for a creature whose action byte
(+0x68) is **80** or **81**, using the item in its held-item field (+0x1e8).

* **Gate** (0x2188fd … 0x218918): the block runs for the local player creature (the pointer
  at +0xb8 of the step's context object); in addition, when the byte at +0xb4 of that context
  object is 0, it also runs for every creature whose controller-kind byte (+0x60) is non-zero.
  *The meaning of the +0xb4 flag is not identified (N-5).*
* **Use limit** U (0xc6b80): type 1 subtype 1 (`LifePotion`) → **3000 ms**; any other type-1
  item → **10000 ms**; non-type-1 → 0.
* Let t = the action time (+0x6c, signed ms) and t₀ = t − dt (dt = the step's milliseconds).
  If t₀ ≥ U (signed compare), nothing more happens.
* **Ticks**: a tick happens in a step where t₀ / 200 ≠ t / 200, with signed integer division
  truncating toward zero (0x218953 … 0x21897b) — i.e. once per 200 ms boundary crossed. A
  long step that crosses several boundaries still gives one tick.
* **Per-tick fraction** f = 200 / U (single precision; constant 0x2fce60), multiplied by 0.5
  (0x345da0) when the creature's controller-kind byte (+0x60) is not 0.
* **Health**: amount = heal(item) × f, where heal(item) (0xc6e10) is, for type 1: subtypes
  1, 2, 4, 5, 6 → 200 · F(L, R); subtypes 8, 9 → 100 · F(L, R); every other subtype → 0;
  non-type-1 → 0. L is the item level (+0x10 as signed 16-bit, converted to float), R the
  rarity byte, and F(L, R) (0x45f60) = 2^(3 · (1 − 1 / (1 + 0.05 · (L − 1)))) · 2^(0.25 · R),
  as in `alpha-creatures-behaviour.md` §10.2. If amount > 0: a hit record carrying −amount,
  the creature's two u32 fields at +0x08 and +0x0c (*believed its id*) and its position
  (+0x10) is emitted (0x86290; *believed
  to be the floating heal number*), health (+0x16c) += amount, then health is clamped to the
  maximum health (0x44db0).
* **Second effect** (0xc7a90; type 1 subtypes 4 `GinsengSoup` and 6 `MushroomSpit` only,
  otherwise 0): v = G(L, R) · 1.5, where G(L, R) (0x45ff0) = F(L, R) / 2³; v is divided by
  2^(0.25 · the i32 at creature + 0x190), multiplied by f, and added to the creature's float
  at **+0x13c**. This runs in every tick, whether or not health was healed. If the new value
  is ≥ 0: when the value before the addition was negative, an effect of kind 0x2f is spawned
  at the creature's position; then the value is reset to 0 (0x218b00 … 0x218b57).
* **Every 400 ms** (only evaluated in a tick step): when t₀ / 400 ≠ t / 400 (same signed
  truncating division), an effect of kind 0x2c with parameter 1.0 is spawned at the
  creature's position (0x218b61 … 0x218bd6).

**Example.** A player (controller kind 0) uses a `LifePotion` of level 5, rarity 0: F(5, 0)
= 2^0.5 ≈ 1.4142135, heal = 282.8427; U = 3000 ms, f = 200/3000. With steps shorter than
200 ms starting from t = 0, the boundaries 200 … 3000 give 15 ticks of ≈ 18.856 each, total
≈ 282.84 over 3 s (before clamping). A level-1 `Cookie` (subtype 0) heals 0 and has
U = 10000 ms.

---

## 10. Choosing the world item under the interaction target (open point I-2)

The frame routine (0x88ee0) resets the item target at game object + 0x800a78 to (−1, −1,
index 0) every frame (0x98541 … 0x9855a) and then, at 0x99538 … 0x99dae, scans world items.

1. **Zones scanned**: the 3 × 3 zones around the zone coordinates held at game object
   + 0x2bc (x − 1 … x + 1, y − 1 … y + 1); for each loaded zone, every world item in its
   list (zone + 0x30). The world item's item record starts at the entry and its position is
   at entry + 0x118.
2. **Filter** (0x3e550, on the local player creature): a mana cube (type 25) is a candidate
   only when its +0x04 equals the player's counter c (+0x1164), replaced by (c mod 100) + 100
   when c > 100. Every other type passes.
3. **Reach**: the squared distance from the player's position (+0x10) to the item, as
   computed by the position-difference helpers, must be **≤ 16** (constant 0x345e78); a
   second test against 100 (0x2fd0b4) is then always passed.
4. **Camera distance**: squared distance from the camera position (game object + 0x140) to
   the item.
5. **Carry penalty** (0x3e4a0): for types 1, 11 and 20, the player's held count of items
   identical to this one (item comparison 0x2f4a0, summed over the slot at +0x11ec with its
   count at +0x11e8 and over every inventory stack) is formed; the player cannot take the
   item when that count is ≥ 50, or for type 20 when it is ≥ 1. If it cannot be taken, both
   squared distances get **+16**.
6. **Nearest fallback**: the candidate with the smallest (penalised) player distance is
   remembered (strictly smaller replaces, so the first of equal candidates stays).
7. **Aim test** (only when its penalised camera distance is smaller than the best aimed one
   so far, or none exists yet): the line from the camera to the item must be clear within
   range 200 (0x19ee90), the item must have a model (0xec400), and the item's model box,
   projected with the game camera (game object + 0x26c), must cover the screen centre.
   *The box construction (model size and the scale values read at world item + 0x130 and
   + 0x134, with a type-dependent choice for types 3 … 13) is believed, not fully traced
   (N-5).* A hit makes this item the target (closest to the camera wins) and clears the
   creature target (game object + 0x800a70) and the value at + 0x800a84 (0x99c54 … 0x99c92).
8. If no item passed the aim test, the **nearest fallback** of step 6, when one exists,
   becomes the target (0x99d81 … 0x99dae).

So: within the reach limit, the item under the screen centre nearest the camera; otherwise
the nearest item within reach, preferring ones the player can take.

---

## 11. Open points

* **N-1** Pet names: the code concatenates the raw creature key after "Pet: "; how that key
  appears on screen in every pet context has not been observed.
* **N-2** Quest items (type 21): which fields the server stores at +0x04/+0x08 (believed:
  the giver's creature type and the low 32 bits of its id).
* **N-3** The dictionary's per-entry tag list: believed to be the `tags` attribute split into
  words; the English output is the same either way.
* **N-4** Coin and pet-food seeds/materials at creation (believed seed 0 for coins, material
  0 for pet food); if different, §3 gives them adjectives or material patterns.
* **N-5** §9: the context flag at +0xb4, the meaning of creature + 0x13c and + 0x190, and the
  two spawned effect kinds; a non-type-1 held item during actions 80/81 has U = 0, and in a
  step with t₀ < 0 the per-tick fraction becomes a division by zero (consequences not
  traced). §10: the model-box construction and scale fields.

---

## Re-certification (2026-11-15)

Re-checked against `Cube.exe` (static decompile, instruction listing, `dumpbin` disassembly
and image bytes) and, for key names and entry shapes only, `dict_en.xml`. No reconstruction
source was consulted.

**Confirmed.** Callers and call sites; record offsets and widths used; dispatch order
2 → 19 → 21 → {1, 18, 14} → general; every (type, subtype) → key row including the `Bait`
loop over 0 … 155 and all 47 pet-food overrides; the material switch (all 25 keyed values,
default `Material`); all 50 adjective keys and their rarity/digit order; seed ≠ 0 and
rarity ≤ 4 gates; first-"@"/first-"#" replacement; no-pattern → plain word, no adjective;
spirit tally overwritten before use; formula copy (type := low byte of +0x08, +0x08 := 0)
and recursion; "Pet: " + raw creature key; quest owner name with n = +0x08, kind = +0x04;
G's kind formula, index formulas, k = 6 default and all 11 syllable tables; all G values and
worked indices; coin chat composition, signed 16-bit quantity, quantity-type list, " X ",
" +", ".\n"; heal limits 3000/10000 ms, 200 ms ticks, f and 0.5 factor, heal multipliers
200/100 by subtype, F and G = F/8, 1.5 factor, +0x13c handling, 400 ms effect; §10 reset,
3 × 3 zones, mana-cube filter, reach 16, +16 penalty, line-of-sight range 200, fallback.

**Corrected.**
* Addresses: the old text gave image VAs labelled as RVAs; all are now RVAs in `Cube.exe`
  (e.g. 0x598a50 → 0x198a50).
* Adjective block extent: 0x599013 … 0x599b9b → 0x199013 … 0x199bb6.
* Tag matching: the old text omitted that a matched tag whose text has no "@" falls through
  to the first-child fallback, and that "#" replacement runs in both branches.
* Missing word: "trailing space" example replaced by the general statement that "@" is
  removed (the trailing space was an English-text consequence).
* Chat line: now three white segments (receiver part, coin part, ".\n"); added the
  " receives " form source and that non-coin item parts use a separate colour.
* Carry penalty: "more than 49 already held" → count of items identical to the candidate
  (slot plus stacks) ≥ 50, or ≥ 1 for type 20.
* Quest kinds: the shared tables are stated precisely (2/18/43/83 and 3/45/84).
* §9 gate: "authoritative or local player" replaced by the exact local-player /
  context-flag / controller-kind condition; division stated as truncating toward zero.
* Heal example: Cookie line no longer claims how long the action runs; the 15-tick figure
  states its step-length assumption.

No reversal of a previously stated rule and no change to any gameplay number (limits,
multipliers, thresholds) resulted. Displayed-name composition is unchanged; only its
description was refined (the tag-fallback edge case cannot change English names).

**Removed.** All dictionary display text (English words, material patterns, adjective
texts, spelling remarks) and the English renderings in the examples, replaced by key/child
references and structural descriptions; the implementer-directed sentence telling a
reimplementation to skip the spirit tally; "confirm by capture" requests (moved to open
points without procedure); tooltip claims about type 19 and the type-13 chat claim that this
routine does not show.

**Unresolved.** N-1 … N-5 above; cross-references to companion specifications (record
layout, title maker description, F definition, loot rules, recipe products) are pointers
only and were not re-checked here.
