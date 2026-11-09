# AlphaVulkan project requirements

- These files are the project's persistent memory. At the beginning of work and
  after every context reset/compaction, read this file and CODEX-HANDOFF.md before
  continuing implementation. Update the handoff when the implementation state or
  verification evidence changes. Source-only researchers instead read this file
  and docs/CLEANROOM_AUTHOR.md; never send them the implementation handoff.
- Build a separate 64-bit Vulkan engine for Cube World Alpha, using the owner's
  installation as the behavioral and visual reference. Keep engine extensions
  distinct from verified native behavior.
- Standing owner preference: make the code as optimized as possible while keeping
  it clean, readable and independently structured. Preserve the 1:1 clean-room
  goal throughout. Use profiling and behavioral verification to guide changes;
  performance improvements must not sacrifice specified results, numeric precision,
  random-draw order, timing, provenance or readability. Keep explicit owner-requested
  extensions separately identified from native parity.
- Follow docs/CLEANROOM.md. Source researchers and implementers run in separate
  contexts. Researchers may inspect the Alpha decompile and Infinite-Cubes/alpha;
  implementers read finalized code-free contracts and project-owned code only.
- Source researchers use docs/CLEANROOM_AUTHOR.md and never inspect the engine,
  exporter, SDK, tests, captures, logs or implementation history. Confirm
  reconstruction leads against original Alpha evidence and mark uncertainties.
- Keep the original installation, saves and Claude handoff untouched. Development
  occurs in this workspace. Use isolated copies for native runtime verification.
- Keep owner assets, captures, saves and exports out of source and packages.
- Adaptation price fixes and engine quests are required core features, enabled
  automatically without user opt-in. The owner explicitly corrected this: price
  fixes are VERY important and quests are strictly non-opt-in. Keep these, the
  shared/swimmable water and bank reeds identified as intentional extensions
  alongside the 1:1 clean-room reference behavior. Do not turn them into optional
  settings in the production client/server.
- Provide an end-user setup/import tool that validates the player's owned Alpha
  executable and required content stores/interface files. Keep it distinct from
  the developer-only native behavior recorder; never package original game data.
- Support documented modding APIs and source modules compiled into the engine,
  in addition to runtime plugins. Provide a working built-in module example.
- Water must reuse the Steam engine's shared surface shader. All Alpha water,
  including its shallow walkable form, must be swimmable. Reeds belong along
  water banks. These are user-requested extensions, not native parity claims.
- Continue through actionable roadmap work after tested checkpoints. Retain open
  parity, provenance and native acceptance gates; do not describe partial results
  as a completed Alpha replacement.
- Owner publication override (2026-11-08): publish the current development source
  now as CWVulkan-Alpha on the owner's GitHub account, with a blank repository
  description and build instructions. This supersedes the earlier beta-only
  CWVulkan-AlphaEdition publication plan. Continue the roadmap after publishing.
  Retain clean-room records, SDK/source-module support, owner-content setup and
  reproducible builds. Exclude original game content and private diagnostics;
  source publication does not certify beta readiness or inherited claims.
- Audit inherited provenance before release. An existing spec or checked roadmap
  item alone does not establish independent authorship or native parity.
