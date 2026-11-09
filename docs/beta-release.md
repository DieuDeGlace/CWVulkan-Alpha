# Alpha beta release gate

**CWVulkan-Alpha** hosts the development source. Publication is separate from
reaching a verified beta comparable to the Steam engine's beta. A public source
snapshot, passing checkpoint, inherited checked roadmap item or local diagnostic
package does not establish that milestone.

Before publishing the beta:

- Build the 64-bit client, server, owner setup tool, DLL SDK and source-module
  example reproducibly; pass the relevant tests with an owned Alpha installation.
- Exercise setup, a new/saved game, movement and swimming, combat, world edits,
  save/reload and matching-client multiplayer. Record native comparisons and
  known discrepancies honestly. Keep native behavior separate from explicit
  water, vegetation and other engine extensions.
- Retain adaptation price fixes and engine quests as required, automatically
  enabled core features. Neither gets a production opt-in/off setting; this is
  the owner's explicit requirement, alongside the water and bank-reed changes.
- Audit clean-room handoffs and inherited provenance. Resolve or bound uncertain
  specifications; do not use unavailable evidence as a parity claim.
- Inspect a fresh allowlisted package and repository contents. Exclude original
  executables/assets, extracted owner caches, saves, captures, private diagnostics,
  dependency caches and local machine paths. Include project licences/notices,
  setup instructions, SDK examples and reproducible build instructions.
- Write beta notes around the tested state, required owned files, mod/profile
  compatibility, remaining limitations and reproduction steps for known issues.

Optimization and readability remain standing requirements. Changes must preserve
specified numeric behavior, random-draw order and timing; performance is measured
and verified against behavior rather than assumed from a faster-looking rewrite.
