# Quests (AlphaVulkan's own addition)

Cube World Alpha has only its map-cell missions (`docs/specs/alpha-missions.md`), and those
stay exactly as they are. On top of them AlphaVulkan adds a quest board in the spirit of the
later Cube World builds: townspeople ask for help, the player goes out, does the job and comes
back for the reward. Nothing here comes from another game's code or data; the system is built
from the engine's own creatures, talk, missions and saves. Quests are a required
core engine feature, enabled automatically in the production client and server.
They require no opt-in setting. This is an explicit owner-requested extension,
kept distinct from the original Alpha mission reference.

## Kinds

| Kind | Out there | Done when |
|---|---|---|
| **Rescue** | a camp of 4-6 hostile guards round 1-3 tied-up prisoners | every prisoner is freed (R next to a prisoner, with no living guard within 15 blocks) |
| **Battle** | a band of 4-6 armed townspeople facing 5-7 hostiles, 16 blocks apart; the two sides fight each other | every hostile of the battle is dead, whoever killed it |
| **Hunt** | 5-9 creatures of one kind roaming round a place | that many of them are dead |
| **Champion** | a champion three levels above the land, with 2-4 followers of its kind round it | the champion is dead (its followers do not count) |

Freed prisoners thank the player and walk back toward the giver's town; allies of a battle
cheer and stay a while. Quest creatures come from the creature types Alpha's missions use,
at the level of the giver's land.

## Givers and the board

* Every third town resident (by its spawn id) is a quest giver. Each day a giver has one quest
  to offer, made from the world seed, the giver and the day, so every player of a world sees
  the same board.
* Talking to a giver with an offer takes the quest (the speech bubble tells what is wanted
  and where). A player holds up to three quests.
* The place is 120-300 blocks from the giver, in a direction told in words ("north-east").
  Its creatures are made when the zone there is loaded and are found on the ground there.
* Coming back to the giver with the job done hands the quest in: the reward is the mission
  reward of Alpha (experience item and a piece of armour or a weapon at the quest's level,
  `missionRewards`) plus coins.

## Icons

The quest icons are the owner's own Steam-style pictures (`assets/quests/quest-icons.png`, MIT,
the same picture CubeVulkan uses):

| Icon | Over a giver's head and on the maps |
|---|---|
| orange **!** | the giver has a quest to offer |
| grey **?** | the player holds that giver's quest, not yet done (also over the quest's place on the maps) |
| orange **?** | the job is done: hand it in |

The quest tag (Alpha's `quest-tag.plx`) shows the latest quest's objective and progress.

## Where it runs

`src/Quests.*` holds the board, the quests, their creatures and rewards, and is used by an
offline game and by the server alike. The creatures are `Npcs`' quest spawns: a quest's list
of spawns activated near players like a zone's own, never respawned. Captives are inert
(controller 6) until freed; allies are townspeople (controller 3) that also take hostile
creatures as targets, and the swings of either side land on the other.

Saves: the world's `av_quests` value (AlphaVulkan's own key; Alpha ignores it).

Online, the server keeps the board and every player's quests; talking, freeing and handing in
go through Alpha's own talk and use packets, and the quest state reaches AlphaVulkan clients
in a value Alpha clients do not read, so original clients still play (without quests).
