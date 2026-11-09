# Alpha items: how an item record becomes a displayed name

Clean-room specification of the item-name routine of Alpha's `Cube.exe` (entry **0x598a50**)
and of the two places that show coin names. Written from the static decompile; RVAs are
citations only. Two short side sections (§9, §10) settle open points I-1 and I-2 of
[alpha-ui-screens.md](alpha-ui-screens.md) §8.1.12.

Callers of the name routine: the tooltip text block (0x4a28c0, at 0x4a2a63), the chat pickup
announcement (0x480fb0, at 0x481333), the frame routine (0x488ee0, at 0x48b564 and 0x497cd3),
the crafting/vendor wide rows (0x42f910 at 0x430d0a; 0x4c2050 at 0x4c42a2 and 0x4c4792), and
itself (formulas, §4).

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
  | +0x14 | 32 × 8 bytes | spirits; byte 3 of each = spirit material |
  | +0x114 | i32 | spirit count |

  The level never enters the name (the tooltip appends " +" and the item's power itself,
  `alpha-ui-screens.md` §8.1.7).
* **Dictionary**: the world's dictionary (world object + 0x30, i.e. G+0x314), loaded from
  `data4.db` `dict_en.xml`. For every `<name key="K">` element it keeps (a) its child
  elements by element name (child name → text), and (b) the words of its `tags` attribute
  *(believed: a list in attribute order)*. **D(K, c)** below means "the text of child `c` of
  `<name key="K">`"; a missing key or a missing child gives the **empty string** (0x480e00).
  Keys and child names compare exactly (case-sensitive). Duplicate keys in the file
  (`MaterialIce` appears twice) have identical text, so which copy the loader keeps does not
  matter here.
* All strings are UTF-16; nothing is trimmed, re-cased or re-spaced by the routine.
* **"@"** and **"#"** are placeholder characters inside dictionary patterns. "Replace @ by X"
  always means: find the **first** "@" in the pattern and replace that one character by X
  (0x4d9950 find, 0x486d00 replace). Later "@"s are left alone (no English pattern has two).

---

## 1. The three lookup tables

### 1.1 Item key K(type, subtype) (0x59fbf0)

A sorted map from the exact pair (type, subtype) to a dictionary key, filled once in the
world constructor (0x58eb00, rows 0x5901ce … 0x591eb4). No row → empty key → every lookup
gives "". The English word is D(key, `singular`).

**Equipment and general items**

| (type, sub) | Key | `singular` (English) |
|---|---|---|
| (2, 0) | `Formula` | — *(no entry; never used, §4)* |
| (3, 0) | `Sword` | Sword |
| (3, 1) | `Axe` | Axe |
| (3, 2) | `Mace` | Mace |
| (3, 3) | `Dagger` | Dagger |
| (3, 4) | `Fist` | Fist |
| (3, 5) | `Longsword` | Longsword |
| (3, 6) | `Bow` | Bow |
| (3, 7) | `Crossbow` | Crossbow |
| (3, 8) | `Boomerang` | Boomerang |
| (3, 9) | `Arrow` | Arrow |
| (3, 10) | `Staff` | Staff |
| (3, 11) | `Wand` | Wand |
| (3, 12) | `Bracelet` | Bracelet |
| (3, 13) | `Shield` | Shield |
| (3, 14) | `Arrows` | Arrows |
| (3, 15) | `Greatsword` | Greatsword |
| (3, 16) | `Greataxe` | Greataxe |
| (3, 17) | `Greatmace` | Greatmace |
| (3, 20) | `Torch` | Torch |
| (4, 0) | `ChestArmor` | Chest Armor |
| (5, 0) | `Gloves` | Gloves |
| (6, 0) | `Boots` | Boots |
| (7, 0) | `ShoulderArmor` | Shoulder Armor |
| (8, 0) | `Amulet` | Amulet |
| (9, 0) | `Ring` | Ring |
| (12, 0) | `Coin` | Coin |
| (13, 0) | `PlatinumCoin` | Platinum Coin |
| (14, 0) | `Leftovers` | Leftovers |
| (15, 0) | `Beak` | Beak |
| (16, 0) | `Painting` | Painting |
| (18, 0), (18, 1) | `Candle` | Candle |
| (19, 0) | `Pet` | — *(no entry; never used, §5)* |
| (23, 0) | `HangGlider` | Hang Glider |
| (23, 1) | `Boat` | Boat |
| (24, 0) | `Lamp` | Lamp |
| (25, 0) | `ManaCube` | Mana Cube |

Weapon subtypes 18 and 19 (and any other pair not listed) have no row and name as "".

**Consumables (type 1)**

| sub | Key | English | sub | Key | English |
|---:|---|---|---:|---|---|
| 0 | `Cookie` | Cookie | 5 | `SnowBerryMash` | Snowberry Mash |
| 1 | `LifePotion` | Life Potion | 6 | `MushroomSpit` | Mushroom Spit |
| 2 | `CactusPotion` | Cactus Potion | 7 | `Bomb` | Bomb |
| 3 | `ManaPotion` | Mana Potion | 8 | `PineappleSlice` | Pineapple Slice |
| 4 | `GinsengSoup` | Ginseng Soup | 9 | `PumpkinMuffin` | Pumpkin Muffin |

**Ingredients (type 11)**

| sub | Key | English | sub | Key | English |
|---:|---|---|---:|---|---|
| 0 | `Nugget` | Nugget | 14 | `Spirit` | Spirit |
| 1 | `Log` | Log | 15 | `Mushroom` | Mushroom |
| 2 | `Feather` | Feather | 16 | `Pumpkin` | Pumpkin |
| 3 | `Horn` | Horn | 17 | `Pineapple` | Pineapple |
| 4 | `Claw` | Claw | 18 | `RadishSlice` | Radish Slice |
| 5 | `Fiber` | Fiber | 19 | `ShimmerMushroom` | Shimmer Mushroom |
| 6 | `Cobweb` | Cobweb | 20 | `GinsengRoot` | Ginseng Root |
| 7 | `Hair` | Hair | 21 | `OnionSlice` | Onion Slice |
| 8 | `Crystal` | Crystal | 22 | `Heartflower` | Heartflower |
| 9 | `Yarn` | Yarn | 23 | `PricklyPear` | Prickly Pear |
| 10 | `Cube` | Cube | 24 | `FrozenHeartflower` | Iceflower |
| 11 | `Capsule` | Capsule | 25 | `Soulflower` | Soulflower |
| 12 | `Flask` | Flask | 26 | `WaterFlask` | Water Flask |
| 13 | `Orb` | Orb | 27 | `SnowBerry` | Snowberry |

**Pet food (type 20)** — the subtype is the creature type the food tames
(`alpha-creatures-behaviour.md` taming rule). First **every** subtype 0 … 155 is given the
key `Bait` (English "Pet Food", loop at 0x5906fd), then these rows overwrite it (creature
names from `alpha-ui-screens.md` §5.4.1):

| sub | creature | Key | English |
|---:|---|---|---|
| 19 | Collie | `BubbleGum` | Bubble Gum |
| 22 | Alpaca | `VanillaCupcake` | Vanilla Cupcake |
| 23 | BrownAlpaca | `ChocolateCupcake` | Chocolate Cupcake |
| 25 | Turtle | `CinnamonRole` | Cinnamon Role |
| 26 | Terrier | `Waffle` | Waffle |
| 27 | ScottishTerrier | `Croissant` | Croissant |
| 30 | Cat | `Candy` | Candy |
| 33 | Pig | `PumpkinMash` | Pumpkin Mash |
| 34 | Sheep | `CottonCandy` | Cotton Candy |
| 35 | Bunny | `Carrot` | Carrot |
| 36 | Porcupine | `BlackberryMarmelade` | Blackberry Marmelade |
| 37 | GreenSlime | `GreenJelly` | Green Jelly |
| 38 | PinkSlime | `PinkJelly` | Pink Jelly |
| 39 | YellowSlime | `YellowJelly` | Yellow Jelly |
| 40 | BlueSlime | `BlueJelly` | Blue Jelly |
| 50 | Monkey | `BananaSplit` | Banana Split |
| 53 | Hornet | `Popcorn` | Popcorn |
| 55 | Crow | `LicoriceCandy` | Licorice Candy |
| 56 | Chicken | `CerealBar` | Cereal Bar |
| 57 | Seagull | `SaltedCaramel` | Salted Caramel |
| 58 | Parrot | `GingerTartlet` | Ginger Tartlet |
| 59 | Bat | `MangoJuice` | Mango Juice |
| 60 | Fly | `FruitBasket` | Fruit Basket |
| 61 | Midge | `MelonIceCream` | Melon Ice Cream |
| 62 | Mosquito | `BloodOrangeJuice` | Bloodorange Juice |
| 63 | PlainRunner | `MilkChocolateBar` | Milk Chocolate Bar |
| 64 | LeafRunner | `MintChocolateBar` | Mint Chocolate Bar |
| 65 | SnowRunner | `WhiteChocolateBar` | White Chocolate Bar |
| 66 | DesertRunner | `CaramelChocolateBar` | Caramel Chocolate Bar |
| 67 | Peacock | `ChocolateCookie` | Chocolate Cookie |
| 74 | Duckbill | `SugarCandy` | Sugar Candy |
| 75 | Crocodile | `AppleRing` | Apple Ring |
| 86 | Spitter | `WaterIce` | Water Ice |
| 87 | Mole | `ChocolateDonut` | Chocolate Donut |
| 88 | Biter | `Pancakes` | Pancakes |
| 90 | Squirrel | `StrawberryCake` | Strawberry Cake |
| 91 | Raccoon | `ChocolateCake` | Chocolate Cake |
| 92 | Owl | `Lollipop` | Lollipop |
| 93 | Penguin | `Softice` | Softice |
| 98 | Horse | `CandiedApple` | Candied Apple |
| 99 | Camel | `DateCookie` | Date Cookie |
| 102 | BarkBeetle | `Bread` | Bread |
| 103 | FireBeetle | `Curry` | Curry |
| 104 | SnoutBeetle | `Lolly` | Lolly |
| 105 | LemonBeetle | `LemonTart` | Lemon Tart |
| 106 | Crab | `StrawberryCocktail` | Strawberry Cocktail |
| 151 | Bumblebee | `BiscuitRole` | Biscuit Role |

Every other subtype 0 … 155 → "Pet Food"; subtypes ≥ 156 → "".

**Quest items (type 21)** (named by §6)

| sub | Key | English | sub | Key | English |
|---:|---|---|---:|---|---|
| 0 | `Amulet1` | Amulet | 5 | `Antivenom` | Antivenom |
| 1 | `Amulet2` | Amulet | 6 | `BandAid` | Band Aid |
| 2 | `JewelCase` | Jewel Case | 7 | `Crutch` | Crutch |
| 3 | `Key` | Key | 8 | `Bandage` | Bandage |
| 4 | `Medicine` | Medicine | 9 | `Salve` | Salve |

Types 0, 10, 17, 22 and ≥ 26 have no rows.

### 1.2 Material key M(material) (0x59ff60)

A fixed switch on the material byte. The dictionary child used for item names is **`item`**,
a pattern with one "@" for the item word; the chat coin line uses `singular` (§7).

| Material | Key | `item` pattern | `singular` |
|---:|---|---|---|
| 1 | `MaterialIron` | "Iron @" | Iron |
| 2 | `MaterialWood` | "Wood @" | Wood |
| 5 | `MaterialObsidian` | "Obsidian @" | Obsidian |
| 7 | `MaterialBone` | "Bone @" | Bone |
| 10 | `MaterialCopper` | "Copper @" | Copper |
| 11 | `MaterialGold` | "Gold @" | Gold |
| 12 | `MaterialSilver` | "Silver @" | Silver |
| 13 | `MaterialEmerald` | "Emerald @" | Emerald |
| 14 | `MaterialSapphire` | "Saphhire @" *(sic)* | Saphhire |
| 15 | `MaterialRuby` | "Ruby @" | Ruby |
| 16 | `MaterialDiamond` | "Diamond @" | Diamond |
| 17 | `MaterialSandstone` | "Sandstone @" | Sandstone |
| 18 | `MaterialSaurian` | "Saurian @" | Saurian |
| 19 | `MaterialParrot` | "Parrot @" | Parrot |
| 20 | `MaterialMammoth` | "Mammoth @" | Mammoth |
| 21 | `MaterialPlant` | "Plant @" | Plant |
| 22 and 130 | `MaterialIce` | "Ice @" | Ice |
| 23 | `MaterialLight` | "Licht-@" *(German leftover, no space)* | Licht |
| 24 | `MaterialGlass` | "Glass @" | Glass |
| 25 | `MaterialSilk` | "Silk @" | Silk |
| 26 | `MaterialLinen` | "Linen @" | Linen |
| 27 | `MaterialCotton` | "Cotton @" | Cotton |
| 128 | `MaterialFire` | "Fire @" | Fire |
| 129 | `MaterialUnholy` | "Unholy @" | Unholy |
| 131 | `MaterialWind` | "Wind @" | Wind |
| anything else (0, 3, 4, 6, 8, 9, 28 … 127, 132 …) | `Material` | — *(no entry: "")* | "" |

`MaterialSatin` exists in the dictionary but no material number selects it.

### 1.3 Adjective key A(rarity, seed mod 10) (switch at 0x599013 … 0x599b9b)

Fifty keys, one per (rarity 0 … 4, digit d = seed mod 10, seed as unsigned 32-bit). The
dictionary child read is whatever matches the item word's tags (§3 step 6); in `dict_en.xml`
every adjective entry has a single child `<m>`, whose text is shown here.

In code order (0x599040 … 0x599b9b) the keys are: rarity 0 → `item:shabby`, `plain`,
`battered`, `artless`, `unwieldy`, `used`, `dusty`, `scratched`, `worn`, `common`; rarity 1 →
`item:adjusted`, `balanced`, `battletested`, `good`, `handmade`, `fair`, `neat`, `clean`,
`undamaged`, `flawless`; rarity 2 → `item:exceptional`, `polished`, `extraordinary`,
`exquisite`, `superb`, `unique`, `handsome`, `grand`, `magic`, `decorated`; rarity 3 → the
same ten words with the suffix `2` (`item:exceptional2` …); rarity 4 → `item:brilliant`,
`shining`, `magnificent`, `sublime`, `pompous`, `glorious`, `splendid`, `famous`, `legendary`,
`fabulous`. Their English `<m>` texts:

| d | rarity 0 | rarity 1 | rarity 2 | rarity 3 | rarity 4 |
|---:|---|---|---|---|---|
| 0 | shabby @ | adjusted @ | exceptional @ | #'s exceptional @ | #'s brilliant @ |
| 1 | plain @ | balanced @ | polished @ | polished @ of # | shining @ of # |
| 2 | battered @ | battle-tested @ | extraordinary @ | extraordinary @ of # | magnificent @ von # |
| 3 | artless @ | good @ | exquisite @ | exquisite @ of # | sublime @ of # |
| 4 | unwieldy @ | handmade @ | superb @ | #'s superb @ | #'s pompous @ |
| 5 | used @ | fair @ | unique @ | unique @ of # | glorious @ of # |
| 6 | dusty @ | neat @ | handsome @ | #'s handsome @ | #'s splendid @ |
| 7 | scratched @ | clean @ | grand @ | grand @ of # | famous @ of # |
| 8 | worn @ | undamaged @ | magic @ | magic @ of # | legendary @ of # |
| 9 | common @ | flawless @ | decorated @ | decorated @ of # | fabulous @ of # |

"magnificent @ von #" is the dictionary's own text (a German leftover). Rarity > 4: no
adjective (the switch's default).

---

## 2. Dispatch by type (0x598a50)

The routine looks at the type byte first:

| Type | Rule | Section |
|---|---|---|
| 2 | formula: "Formula: " + name of the taught item | §4 |
| 19 | pet: "Pet: " + creature key | §5 |
| 21 | quest item: generated owner name + "'s " + word | §6 |
| 1, 14, 18 | **plain word**: D(K(type, subtype), `singular`) — no material, no adjective | — |
| anything else | **material + word + adjective** | §3 |

So consumables (type 1), leftovers (14) and candles (18) always show just their word:
material, seed and rarity are ignored for them.

---

## 3. The general rule (material, word, adjective)

Used for every type not listed in §2: weapons and armour (3–9), ingredients (11), coins (12),
platinum coins (13), beaks (15), paintings (16), pet food (20), hang glider/boat (23), lamp
(24), mana cube (25), and the unnamed types.

1. **Spirit tally (no effect).** The routine counts the item's spirits whose material is
   ≥ 128 into four counters (0x598c89 … 0x598cc6), but the counters' storage is overwritten
   before it is read (0x598ef2), so spirits never change the name. A reimplementation skips
   this. (A spirit material above 131 would make the original write outside the four
   counters; with real data only 128–131 occur.)
2. **Material pattern** P = D(M(material), `item`) (0x598d97 … 0x598dc0).
3. **No "@" in P** (including P empty, i.e. every material without an entry):
   name = D(K(type, subtype), `singular`). **Stop** — no adjective is ever added in this
   case, whatever the seed and rarity (0x598e1f … 0x598ec3).
4. **Word** W = D(K(type, subtype), `singular`) (0x598ec8 … 0x598f71; a missing word gives
   "", so the name would end in a trailing space, e.g. "Iron ").
5. name = P with its first "@" replaced by W (0x599003). Example: "Iron @" + "Sword" →
   "Iron Sword".
6. **Adjective**, only when **seed ≠ 0** (0x599008) and **rarity ≤ 4** (0x599017):
   * key = A(rarity, seed mod 10) (§1.3); its entry gives a set of children by element name.
   * The child is chosen by the word's tags: for each tag of `<name key="K(type,subtype)">`
     in order, if the adjective entry has a child with that element name, that child is used
     (0x599bbb … 0x599d83). If no tag matches but the adjective entry has any child, its
     **first** child in element-name order is used (0x599d89 …). With `dict_en.xml` every
     adjective has only `<m>`, so English names always use the `<m>` text; the tag matching
     only matters for a gendered dictionary.
   * In the chosen text, the first "@" is replaced by the current name (step 5), and the
     result becomes the name.
   * Then, if the name contains "#", its first "#" is replaced by the **generated name**
     G(seed) of §3.1.
7. The name is written through a wide string stream into the output (0x599ead, 0x599f75);
   the stream adds nothing.

### 3.1 The generated name G(seed) for "#"

G uses the creature-title maker 0x5a0ed0 (described in `alpha-ui-screens.md` §5.4.1 item 3
and §5.9.1 "The name maker") with:

* n = seed (u32),
* kind k = (7 · seed mod 2³²) mod 11 (unsigned; 0x599d12 … 0x599d20).

There are **no random draws** (no `rand()`; nothing from `alpha-worldgen-random.md`): the
name is a fixed function of the seed. As in the title maker, with all arithmetic unsigned
32-bit with wrap-around:

* a = (⌊n/7⌋ + 13·n) mod 2³², first index i = a mod m;
* b = (7·n + ⌊n/10⌋) mod 2³², second index j = b mod m;
* G = First_k[i] followed directly by Second_k[j] (no space, no case change).

m = 10 for k = 0 … 5 and 7 … 10; k = 6 has no own tables and uses the 20-entry default pair
(m = 20). The tables (the same ones creature titles use for creature types 0 … 10):

| k | First (index 0 … m−1) | Second (index 0 … m−1) |
|---:|---|---|
| 0 | El, Li, Dri, Elan, Le, Ly, A, Tho, Da, Les | ric, kun, zic, dor, las, sander, reon, reas, lundra, andor |
| 1 | Ar, A, Si, Mer, Lu, Ari, Su, Sira, Hia, Li | weya, luna, laya, leya, ma, lia, matra, vy, zyna, ly |
| 2 | De, Ro, As, Ge, An, Pat, Wolf, Ale, Mi, Ben | rek, man, gram, rald, rick, ram, sander, kal, ton, ny |
| 3 | Sa, Jen, Kla, Mi, La, Auri, Mela, Alu, Ve, Est | rah, ny, ria, elle, na, ga, ma, riana, rona, una |
| 4 | Gra, Xe, Ur, Bel, Chu, Ki, Go, Zim, Kubo, Raz | zic, nax, tik, tuk, ruk, bo, rix, mi, tok, nor |
| 5 | Kur, Xe, Ki, Am, Zifa, Zu, Ma, Chi, Zi, Mia | ra, lia, bi, ba, ly, ki, bara, mi, zy, xa |
| 6 | Kro, Aru, As, Ge, Kur, Lugo, Iko, Liku, Tero, Var, An, The, Ga, Lan, Dura, Dama, Se, Thal, Nar, San | no, mi, la, sel, ron, ion, kor, lon, rok, tar, den, gar, dar, dara, lan, ka, gor, rior, ria, mor |
| 7 | Cho, Zer, Kaz, Kraz, Zel, Drak, Lo, Raz, Spi, Zen | rux, rek, zic, tac, mec, kor, rax, zor, ro, go |
| 8 | Zi, Zer, Az, Ki, Iza, Drak, Li, May, Spi, Zan | rya, ri, zia, ki, mah, ira, maya, ana, ra, ya |
| 9 | Tor, Gem, Bar, Me, As, Tem, Arak, Ur, Grim, Xor | lox, bart, kor, thos, kun, bur, thor, lok, li, bor |
| 10 | Mum, Grun, Brun, Hei, Mo, Hil, Ur, Lum, Grim, Xor | pie, hild, di, muna, ki, trud, sa, axa, ira, ika |

"#" appears only in rarity-3 and rarity-4 patterns, so only those items carry a name.

**Test vectors of G** (seed → k, i, j → G):

| seed | k | i | j | G |
|---:|---:|---:|---:|---|
| 10 | 4 | 1 | 1 | Xenax |
| 3 | 10 | 9 | 1 | Xorhild |
| 7 | 5 | 2 | 9 | Kixa |
| 77 | 0 | 2 | 6 | Drireon |
| 12345 | 10 | 8 | 9 | Grimika |
| 4294967295 | 8 | 3 | 2 | Kizia |

Worked: seed 12345 → 7·12345 = 86415, 86415 mod 11 = 10 → k = 10; a = 1763 + 160485 =
162248 → i = 8 "Grim"; b = 86415 + 1234 = 87649 → j = 9 "ika"; G = "Grimika".

---

## 4. Formulas (type 2) (0x598aac … 0x598b37)

1. Copy the whole record. In the copy: **type := the low byte of the product field
   (+0x08)**, then the product field := 0. Subtype, seed, rarity, material, level and spirits
   are kept.
2. Name the copy with this same routine (so the full §2/§3 rules apply to the taught item,
   adjective included).
3. Result = **"Formula: "** + that name (the literal includes the trailing space, string
   0x71d540).

The recipe generator (`alpha-items-generator.md` §3) stores product types 3–9 and seeds
0 … 99, so formulas read like "Formula: Iron Sword" (seed 0) or "Formula: shabby Iron Sword".
A product type of 1, 14 or 18 would give "Formula: " + the plain word; a product of 2 would
recurse once more with type 0.

---

## 5. Pets (type 19) and pet food (type 20)

**Pets (0x598b3c … 0x598b68).** Result = **"Pet: "** (0x71d554) + the **creature key** of the
creature type given by the subtype, taken from the creature key table (0x59aa60, the same
table as `alpha-ui-screens.md` §5.4.1 item 1). The key is used **as is — it is not looked up
in the dictionary**. So a pet Collie (subtype 19) is "Pet: Collie", a shepherd dog (20) is
"Pet: ShepherdDog", a brown alpaca (23) "Pet: BrownAlpaca", a type with no key row (95, 129)
"Pet: ". Material, seed and rarity are ignored. (The tooltip shows no " +power" for type 19;
its line 2 shows "LVL" instead, ui spec §8.1.7.) *Worth confirming by capture that the
CamelCase key really shows; the code reads that way.*

**Pet food (type 20)** goes through the general rule (§3): the word is the food of §1.1. Pet
food dropped by creatures carries material 0 *(believed — the drop rule sets type and
subtype only, `alpha-server-simulation.md` loot step 2)*, so P is empty and the name is just
the word ("Bubble Gum"), with no adjective even if seed and rarity are set. The tooltip then
prefixes "Pet food: " (ui spec §8.1.7), giving "Pet food: Bubble Gum". A food for a creature
without its own row reads "Pet food: Pet Food".

---

## 6. Quest items (type 21) (0x598b6a … 0x598c6c)

Result = O + **"'s "** (0x71d560) + D(K(21, subtype), `singular`), where O is the title maker
0x5a0ed0 applied with **n = the u32 at +0x08** and **kind = the u32 at +0x04** (as a signed
integer). Because kind can be any creature type here, the full title table applies
(`alpha-ui-screens.md` §5.4.1 item 3: 10-entry tables for creature types 0–5, 7–16 and the
grouped human types 18, 43, 83 / 45, 84; the 20-entry default for every other kind).
*Believed*: the quest giver's creature type is stored at +0x04 and the low 32 bits of its id
at +0x08, so the item names the same NPC whose title floats over its head ("Urzic's
Medicine"). Rarity, material and spirits are ignored.

---

## 7. Coins

Coins are type **12**, subtype 0, with the coin kind in the **material** — 10 copper,
12 silver, 11 gold — and the quantity in the i16 at +0x10 (`alpha-server-simulation.md` loot
step 4).

* **Item name** (tooltip, bag rows, the routine): general rule §3 → "Copper Coin",
  "Silver Coin", "Gold Coin". Coins with a non-zero seed would get an adjective; the coin drop
  rule does not set a seed *(believed: seed 0)*, so normally there is none.
* **Chat announcement** (0x480fb0; type 12 only, 0x481234 … 0x481290): the coin part is
  built in a wide string stream as the **quantity** (+0x10 as a signed 16-bit decimal, no
  padding), one **space**, and **D(M(material), `singular`)** — the bare material word, not
  the item name. So the line reads "You receive " + "37 Copper" + ".\n" (or
  "<player> receives …"), all in white (1, 1, 1, 1); "2 Silver", "1 Gold". A coin with a
  material outside the table would read "37 " (empty word). No " +power" is appended for
  coins.
* For comparison, the other announcement forms (from the same routine): quantity types (13,
  21, 11 except subtype 14, 0, 25, 20, 24, 23) print the quantity, then " X " (0x701754),
  then the item name; all other items print the item name followed by " +" and the item's
  power (0x6fd424); the line ends with ".\n" (0x70175c).

Type 13 (`PlatinumCoin`, "Platinum Coin") is the experience item; it is consumed on receipt
and never reaches the chat line (ui spec §8.1.9). Its item name follows §3 ("Platinum Coin"
for material 0).

---

## 8. Worked examples

Iron sword, level 5 (type 3, subtype 0, material 1). The level is irrelevant to the name.

| seed | rarity | Steps | Name |
|---:|---:|---|---|
| 0 | any | P "Iron @", W "Sword"; seed 0 → no adjective | **Iron Sword** |
| 10 | 0 | d 0 → "shabby @" | **shabby Iron Sword** |
| 10 | 2 | d 0 → "exceptional @" | **exceptional Iron Sword** |
| 10 | 3 | d 0 → "#'s exceptional @"; G(10) = Xenax (k 4) | **Xenax's exceptional Iron Sword** |
| 3 | 4 | d 3 → "sublime @ of #"; G(3) = Xorhild (k 10) | **sublime Iron Sword of Xorhild** |
| 12345 | 4 | d 5 → "glorious @ of #"; G(12345) = Grimika | **glorious Iron Sword of Grimika** |
| 12 | 4 | d 2 → "magnificent @ von #"; k = 84 mod 11 = 7; i = (1 + 156) mod 10 = 7 "Raz", j = (84 + 1) mod 10 = 5 "kor" | **magnificent Iron Sword von Razkor** |
| 10 | 7 | rarity > 4 → no adjective | **Iron Sword** |

In the tooltip the name line is then this name + " +" + the item's power *p*, e.g.
"exceptional Iron Sword +*p*" (ui spec §8.1.7).

Other items:

| Record | Name |
|---|---|
| bow (3, 6), material 2, seed 0 | Wood Bow |
| sword (3, 0), material 6, seed 55, rarity 2 | Sword (material 6 has no pattern → plain word, **no** adjective) |
| chest (4, 0), material 25 (mage set), seed 0 | Silk Chest Armor |
| ring (9, 0), material 11, seed 7, rarity 1 | clean Gold Ring |
| weapon (3, 18), material 1, seed 0 | "Iron " (no key row → empty word) |
| health potion (1, 1), any material/seed/rarity | **Life Potion** |
| mana potion (1, 3) | Mana Potion |
| copper coins (12, 0), material 10, quantity 37 | item name **Copper Coin**; chat "You receive 37 Copper." |
| gold nugget (11, 0), material 11, seed 0 | **Gold Nugget** |
| same, seed 7, rarity 0 | scratched Gold Nugget |
| emerald drop (11, 0), material 13, rarity 1, seed 51 | balanced Emerald Nugget |
| parrot feather (11, 2), material 19 | Parrot Feather |
| ice crystal (11, 8), material 22 | Ice Crystal |
| cotton (11, 11), material 27 | Cotton Capsule |
| mushroom (11, 15), material 0, seed 9, rarity 3 | Mushroom (no pattern → no adjective) |
| formula: type 2, product 3, subtype 0, material 1, seed 0 | **Formula: Iron Sword** |
| formula: type 2, product 3, subtype 0, material 1, seed 42, rarity 1 | Formula: battle-tested Iron Sword |
| formula: type 2, product 9, material 11, seed 0 | Formula: Gold Ring |
| pet (19, 19) | Pet: Collie |
| pet food (20, 19), material 0 | Bubble Gum (tooltip "Pet food: Bubble Gum") |
| quest item (21, 4), +0x04 = 4, +0x08 = 1000 | Urzic's Medicine (title kind 4: i = (142 + 13000) mod 10 = 2 "Ur", j = (7000 + 100) mod 10 = 0 "zic") |
| candle (18, 1), material 1 | Candle (type 18 ignores material) |

---

## 9. Healing of actions 80/81 (open point I-1)

Inside the world step (0x60c510, at 0x61891e … 0x618bdd), for a creature in action **80**
(drink) or **81** (eat), holding the used item in its held-item field (+0x1e8). The block
runs only for creatures this machine simulates (the same "authoritative or local player"
test as the surrounding code, 0x6188fd … 0x618918).

* **Use limit** U (0x4c6b80): type 1 subtype 1 (Life Potion) → **3000 ms**; any other type-1
  item → **10000 ms**; non-type-1 → 0 (nothing happens).
* Let t = the action time (+0x6c) after this step and t₀ = t − dt (dt = the step's
  milliseconds). If t₀ ≥ U, nothing more happens (the effect is over).
* **Ticks**: a tick happens in a step where ⌊t₀ / 200⌋ ≠ ⌊t / 200⌋ (integer division,
  0x618953 … 0x61897b) — i.e. once per 200 ms boundary crossed. A long step that crosses
  several boundaries still gives one tick.
* **Per-tick fraction** f = 200 / U, halved (× 0.5) when the creature's controller kind
  (+0x60) is not 0 (non-players heal at half speed).
* **Health**: amount = heal(item) × f, where heal(item) is the tooltip's "HP +" value
  (0x4c6e10: subtypes 1, 2, 4, 5, 6 → 200 · F(L, R); 8, 9 → 100 · F(L, R); others 0; F as
  `alpha-creatures-behaviour.md` §10.2 with the item's level and rarity). If amount > 0: a
  floating-number/hit record carrying −amount at the creature's position is emitted
  (0x486290 — *believed* the green "+HP" number), health (+0x16c) += amount, then clamped to
  the maximum health (0x444db0).
* **Second effect** (subtypes 4 Ginseng Soup and 6 Mushroom Spit only, 0x4c7a90): v =
  G(L, R) · 1.5 (G = F/8), scaled by f and divided by 2^(0.25 · creature level); it is added
  to the creature's float at **+0x13c**. If that value is now ≥ 0, it is reset to 0, and if
  it was negative before the addition an effect of kind 0x2f is spawned at the creature
  (0x618b1e … 0x618b52). *Meaning of +0x13c not identified; believed a negative
  status (poison-like) that these two foods cure.*
* A further visual effect is spawned at every 400 ms boundary of the action time
  (0x618b61 … 0x618b90, *believed* the eating/drinking particles).

**Example.** Player drinks a Life Potion, level 5, rarity 0: heal = 200 · F(5, 0) =
200 · 1.4142135 = 282.84; U = 3000 ms, f = 200/3000; 15 ticks of 18.856 HP each, total
282.84 over 3 s (if not clamped). A level-1 Cookie (subtype 0) heals nothing (heal 0), but the
action still runs for up to 10 s.

---

## 10. Choosing the world item under the interaction target (open point I-2)

The frame routine (0x488ee0) resets the target G+0x800a78 to (−1, −1, index 0) every frame
(0x498541 … 0x49855a) and then, at 0x499538 … 0x499dae, scans world items. *Read at the level
below; the box-projection details follow the creature focus test of ui spec §5.4.1 item 6.*

1. **Zones scanned**: the 3 × 3 zones around the local player's zone (G+0x2bc, ±1 in x and
   y); for each loaded zone, every world item in its list (zone +0x30).
2. **Filter** (0x43e550): a mana cube (type 25) is a candidate only when its +0x04 equals the
   player's mana-cube counter c (+0x1164), reduced as c mod 100 + 100 when c > 100. Every
   other type passes.
3. **Reach**: squared distance from the player (+0x10) to the item (+0x118), in blocks
   (1/65536 units), must be **≤ 16** (4 blocks; a second test ≤ 100 is then always true).
4. **Camera distance**: squared distance from the camera position (G+0x140) to the item.
5. **Carry penalty** (0x43e4a0): if the player could not carry the item (types 1, 11, 20:
   more than 49 already held, or for type 20 any already held), both squared distances get
   **+16**.
6. **Nearest fallback**: the candidate with the smallest (penalised) player distance is
   remembered.
7. **Aim test** (only when its camera distance beats the best aimed one so far): the line of
   sight from the camera to the item must be clear within range 200 (0x59ee90), the item must
   have a model (0x4ec400), and the item's model box (model size × item scale, centred, the
   eight corners at ±0.5 of each side) projected with the game camera (G+0x26c) must cover
   the screen centre: among the corners in front of the camera there is one with normalised
   x < 0, one with x > 0, one with y < 0 and one with y > 0. A hit makes this item the target
   (closest to the camera wins) and clears the creature target (G+0x800a70) and G+0x800a84.
8. If no item passed the aim test, the **nearest fallback** of step 6 becomes the target
   (0x499d81 … 0x499dae).

So: within 4 blocks, the item under the crosshair nearest the camera; otherwise simply the
nearest item within 4 blocks (preferring ones you can carry). *Believed in the details of the
box size (scale values read at +0x130 and +0x134 of the world-item record) — confirm by capture.*

---

## 11. Open points

* **N-1** Pet names: the code shows the raw creature key ("Pet: ShepherdDog"); confirm by
  capture.
* **N-2** Quest items (type 21): which fields the server stores at +0x04/+0x08 (believed:
  giver's creature type and id).
* **N-3** The dictionary's per-entry tag list: believed to be the `tags` attribute split into
  words; irrelevant for English output.
* **N-4** Coin and pet-food seeds/materials at creation (believed 0 seed for coins, material
  0 for pet food); if non-zero, §3 gives them adjectives.
* **N-5** §9 field +0x13c and the two spawned effects; §10 box scale fields.
