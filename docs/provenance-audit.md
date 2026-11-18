# Specification provenance audit

Release gate (AGENTS.md, CLEANROOM.md "Continuation provenance"): an existing spec or a
checked roadmap item does not by itself establish independent authorship or native parity.
This inventory classifies every `docs/specs` contract. It is regenerated mechanically: the
status is read from a document's first 15 lines; "recorded" means CLEANROOM.md cites the
document's current SHA-256 prefix.

Classes:
- **A, held and recorded.** A source-only author reported FINAL / STABLE / HELD and the
  implementer recorded the read and hash in CLEANROOM.md.
- **B, held, hash not recorded.** Marked held, but CLEANROOM.md does not cite the current
  hash. Earlier records may point to an older revision, a topic, or a local
  handoff. The public record still needs the current text's digest and readback.
- **C, inherited.** No held status and never named in CLEANROOM.md: carried over from the
  2026-11-07 continuation. Several mix behavioural prose with implementation notes, so they
  must not be handed to source authors unredacted.
- **E, engine documentation.** Describes engine extensions (owner mods ported), not the original;
  not a native contract and not re-certified.
- **G, implementation guidance.** A planning overview addressed to implementers; not a native
  contract. Native facts it cites live in the topic contracts.
- **S, superseded audit notes.** An audit compared against engine output whose native content is
  covered by a re-certified contract (`alpha-open-points-12.md` by `alpha-worldgen-vegetation.md`);
  kept as history, not handed to authors.

Snapshot 2026-11-17 after batch 65 and the rulings. The 284 tracked contracts
comprise 230 A, 49 B, 0 C and 5 reclassified (E/G/S).

## Remediation

1. Class C first, then B. Each document goes to a fresh source-only author
   (CLEANROOM_AUTHOR.md) who checks every statement against the original images, rewrites
   transcribed-looking or mixed wording as independent code-free prose, moves anything
   unconfirmed to an unresolved section, appends a dated re-certification record, and sets
   the held status only after a complete readback.
2. Documents that contain implementation notes or history (for example
   `alpha-net-transport.md`, `alpha-net-server.md`, `alpha-audio.md`, `sky-and-clouds.md`)
   are handled by the re-certifying author, who removes all implementer advice, verification
   procedures and replacement speculation and keeps only native behaviour. Such guidance holds
   no engine internals or history (checked on the clean side before each hand-over); documents
   that describe engine extensions rather than the original (`shadows.md`,
   `sky-and-clouds.md`, `vegetation-wind.md`) are reclassified as engine documentation, not
   native contracts.
3. The implementer then reads each re-certified document whole, records its hash in
   CLEANROOM.md, and reconciles corrected behaviour in the engine with tests.
4. The gate closes when every row is class A. Class A certifies the process record, not
   complete native parity.

Pilot (2026-11-14): `alpha-relationship-kind-89-admission.md`,
`alpha-hit-scalar-reduction-order.md`, `alpha-forward-link-zero-gates.md`,
`alpha-fatal-object-descriptor-association.md` and `alpha-inventory-equality-consumers.md`.
Batch 2: `alpha-data-stores.md`, `alpha-category13-inventory-counter.md`,
`alpha-generic-hit-vector-adjustment.md`, `alpha-yaw-trig-scalar-abi.md`,
`alpha-fatal-context-field-producers.md`, `alpha-world-creature-health-admission.md` and
`alpha-linked-item-storage-producers.md`. Both batches are class A (CLEANROOM.md).
Batch 3b: `alpha-light-admission-plane-production.md`, `alpha-hit-item-presentation-vector.md`,
`alpha-opaque-output-transfer-state.md` and `alpha-terrain-color-pipeline-public.md`.
Batch 3a: `alpha-creature-light-attachment-producers.md`, `alpha-client-buff-event-transmission.md`,
`alpha-spatial-association-producers.md` and `alpha-fatal-object-association-producers.md`.
Batch 4: `alpha-terrain-light-source-lifecycle.md`, `alpha-worldgen-clouds.md`,
`alpha-periodic-hit-producer-state.md`, `alpha-backing-reference-producers.md` and
`alpha-creature-yaw-light-gates.md`.
Batch 5: `alpha-server-buff-update-admission.md`, `alpha-hit-response-vector-consumer.md`,
`alpha-client-buff-action-boundaries.md`, `alpha-hit-application-mutations.md`,
`alpha-hit-threat-death-lifecycle.md` and `alpha-open-points-9.md`.
Batch 6b: `alpha-player-control.md` and `alpha-open-points-11.md`.
Batch 6a: `alpha-worldgen-random.md`, `alpha-worldgen-vegetation.md` and `alpha-house-furniture.md`;
then `alpha-net-transport.md`, `alpha-net-server.md` and `alpha-saves.md`; `alpha-open-points-7.md` and `alpha-open-points-8.md`; `alpha-worldgen-regions.md` and
`alpha-items-generator.md`; `alpha-projectiles.md`, `alpha-open-points-1.md`, `alpha-world-items.md` and
`alpha-open-points-2.md`; `alpha-worldgen-terrain.md` and `alpha-open-points-5.md` (plus the new ruling
`alpha-tick-period-rulings.md`); `alpha-worldgen-caves.md`, `alpha-spawn-objects.md`, `alpha-loot-rolls.md`,
`alpha-open-points-10.md` (plus the ruling `alpha-rulings-formation-angle-box.md`); `alpha-net-packets.md` and `alpha-skills.md`
(plus the ruling `alpha-rulings-knockback-snap.md`); `alpha-travel.md`, `alpha-particles.md`,
`alpha-crafting.md`, `alpha-open-points-3.md` (plus the ruling `alpha-rulings-walk-flag-pitch.md`); `alpha-path-search.md`, `alpha-worldgen-locations.md`,
`alpha-item-names.md` and `alpha-worldgen-trees.md`; `alpha-server-simulation.md` and `alpha-physics-camera.md`; `alpha-interaction.md`, `alpha-open-points-4.md`, `alpha-item-services.md` and `alpha-block-edits.md` (plus the ruling `alpha-rulings-melee-reach.md`); `alpha-talk-vendors.md`, `alpha-open-points-6.md` and `alpha-location-creatures.md`; `alpha-creatures-behaviour.md` (plus the ruling `alpha-rulings-beam-flag-strike-origin.md`); `alpha-worldgen-structures.md`; `alpha-ui-screens.md` (plus the rulings `alpha-rulings-knockback-taming-charge.md` and `alpha-rulings-corridor-third-line.md`); `alpha-creatures-models.md`, `alpha-missions.md` and `alpha-rendering.md` (plus the rulings `alpha-rulings-mission-rewards.md`, `alpha-rulings-pose-49-50-mount.md`, `alpha-rulings-rendering-clouds-lights.md` and `alpha-rulings-shimmer-phase.md`); `alpha-pets-bombs.md` and `alpha-character-screens.md`; `alpha-materials-wildlife-routes.md` (plus the ruling `alpha-rulings-held-light-factor.md`); `alpha-trails-poses-schedules.md`, `alpha-ui-plasma.md`, `alpha-audio.md` and `alpha-extractor.md` (plus the rulings `alpha-rulings-pose-69-arc.md`, `alpha-rulings-manacubebar-sequence.md`, `alpha-rulings-audio-ui-sounds.md`, `alpha-rulings-zone-fields.md`, `alpha-rulings-spawn-rounds-ruin-routes.md` and `alpha-rulings-channel2-explosion-window.md`).

Batch 27 covers `alpha-terrain-chunk-point-lights.md` and
`alpha-terrain-shader-color-numerics.md`.
Batch 28 covers `alpha-creature-update-phases.md`,
`alpha-client-frame-world-step-order.md` and `alpha-world-random-stream-boundaries.md`.
Batch 29 covers `alpha-client-action-input-phase.md`,
`alpha-client-action-binding-producer.md` and `alpha-rulings-action-99-lens.md`.
Batch 30 covers `alpha-packet6-sender-turn.md` and
`alpha-rulings-packet6-bomb-default.md`.
Batch 31 covers `alpha-packet6-client-queue.md`.
Batch 32 covers `alpha-packet6-placement-producers.md`.
Batch 33 covers `alpha-packet6-quick-wheel.md`.
Batch 34 covers `alpha-timed-consumables.md`.
Batch 35 covers `alpha-blueprint-zone-delivery.md`.
Batch 36 covers `alpha-riding-actor-step.md`.
Batch 37 covers `alpha-creature-receipt-order.md`.
Batch 38 covers `alpha-connected-actor-turn.md`.
Batch 39 covers `alpha-connected-walk-phases.md`.
Batch 40 covers `alpha-common-action-processing-state.md`,
`alpha-connected-action-audio.md` and `alpha-rulings-channel36-cadence.md`.
Batch 41 covers `alpha-sneak-light-inputs.md`.
Batch 42 covers `alpha-connected-melee-candidates.md` and `alpha-connected-combo-cleanup.md`.
Batch 43 covers `alpha-connected-hit-consumption.md`.
Batch 44 covers `alpha-ordinary-creature-link.md`.
Batch 45 covers `alpha-connected-melee-actions-2-through-7.md`,
`alpha-rulings-connected-melee-scalar-selectors.md` and `alpha-visibility-ray-geometry.md`.
Batch 46 covers `alpha-stock-blueprint-placement.md`.
Batch 47 covers `alpha-connected-melee-actions-9-through-21.md`.
Batch 48 covers `alpha-connected-melee-actions-30-through-33.md` and
`alpha-rulings-connected-melee-30-through-33-stats.md`.
Batch 49 covers `alpha-connected-ranged-dispatch.md` and the re-certified
`alpha-critical-and-haste-stat-numerics.md`.
Batch 50 covers `alpha-native-startup-window-input-boundary.md` and
`alpha-native-initial-direct3d9-device.md`.
Batch 51 covers `alpha-connected-projectile-receipt-step.md`,
`alpha-rulings-connected-client-receipt-order.md` and the re-certified
`alpha-world-call-phase-boundary.md`.
Batch 52 covers `alpha-rulings-projectile-ray-and-variant.md` and
`alpha-rulings-projectile-steering-and-impact.md`.
Batch 53 covers `alpha-dedicated-projectile-step.md`.
Batch 54 covers `alpha-rulings-projectile-point-impact.md`.
Batch 55 covers `alpha-stock-blueprint-writers-and-persistence.md`.
Batch 56 covers `alpha-native-terrain-vertex-light-alpha.md`.
Batch 57 covers `alpha-native-fixed-coordinates-terrain-light.md` and
`alpha-native-terrain-environment-palette.md`.
Batch 58 covers `alpha-native-mesh-cache-light-padding.md`.
Batch 59 covers `alpha-native-light-rectangle-propagation.md`.
Batch 60 covers `alpha-primary-terrain-fog-upload-state.md`,
`alpha-native-terrain-rgb-and-composition.md` and the re-certified
`alpha-light-propagation-lifecycle.md`.
Batch 61 covers `alpha-combat-terrain-lighting.md`.
Batch 62 covers `alpha-connected-receipt-supplement.md` and
`alpha-taming-progress-and-capture.md`.
Batch 63 covers `alpha-objective-state-one-late-interaction.md` and
`alpha-objective-receipt-formatting-map-supplement.md`.
Batch 64 covers `alpha-client-elapsed-adjustment-and-world-readiness.md`
and `alpha-client-world-change-lifecycle.md`.
Batch 65 covers `alpha-client-connected-spawn-recovery.md`.

| Spec | Class | Lines | Implementation words |
| --- | --- | ---: | ---: |
| `alpha-actor-creation-configuration.md` | A | 130 | 0 |
| `alpha-actor-identity-publication.md` | B | 60 | 2 |
| `alpha-actor-inventory-consumers.md` | A | 70 | 1 |
| `alpha-airship-pilot-behaviour.md` | A | 236 | 0 |
| `alpha-airship-steering.md` | A | 240 | 0 |
| `alpha-attack-routine.md` | A | 418 | 1 |
| `alpha-audio.md` | A | 520 | 5 |
| `alpha-backing-reference-producers.md` | A | 127 | 0 |
| `alpha-beam-height-leftovers.md` | A | 228 | 0 |
| `alpha-behavior-target-selection.md` | B | 78 | 0 |
| `alpha-block-edits.md` | A | 688 | 1 |
| `alpha-blueprint-zone-delivery.md` | A | 245 | 0 |
| `alpha-buff-record-replication-recovered.md` | B | 414 | 2 |
| `alpha-camera-follow-collision.md` | A | 119 | 4 |
| `alpha-camera-initial-state.md` | A | 227 | 1 |
| `alpha-camera-ray-geometry.md` | A | 196 | 1 |
| `alpha-camera-view-production.md` | A | 100 | 3 |
| `alpha-category13-inventory-counter.md` | A | 76 | 0 |
| `alpha-character-screens.md` | A | 913 | 0 |
| `alpha-class-83-descriptor-production.md` | A | 157 | 0 |
| `alpha-client-action-binding-producer.md` | A | 127 | 0 |
| `alpha-client-action-input-phase.md` | A | 187 | 1 |
| `alpha-client-buff-action-boundaries.md` | A | 183 | 3 |
| `alpha-client-buff-event-transmission.md` | A | 91 | 3 |
| `alpha-client-connected-spawn-recovery.md` | A | 90 | 0 |
| `alpha-client-coordinate-scalar-producers.md` | B | 112 | 2 |
| `alpha-client-elapsed-adjustment-and-world-readiness.md` | A | 219 | 0 |
| `alpha-client-frame-world-step-order.md` | A | 84 | 1 |
| `alpha-client-hit-event-transmission.md` | B | 90 | 2 |
| `alpha-client-incoming-hit-mutations.md` | B | 102 | 2 |
| `alpha-client-primary-start-boundary.md` | B | 186 | 3 |
| `alpha-client-projectile-hit-call-boundary.md` | B | 117 | 3 |
| `alpha-client-receiver-publication-locks.md` | B | 131 | 1 |
| `alpha-client-startup-window-boundary.md` | B | 81 | 0 |
| `alpha-client-world-change-lifecycle.md` | A | 173 | 0 |
| `alpha-client-world-clock-input.md` | B | 52 | 2 |
| `alpha-column-scalar-cache-publication.md` | B | 105 | 0 |
| `alpha-column-value-production.md` | A | 413 | 0 |
| `alpha-combat-aim-triplet.md` | A | 266 | 0 |
| `alpha-combat-approach-path.md` | A | 322 | 0 |
| `alpha-combat-beam-leftovers.md` | A | 378 | 0 |
| `alpha-combat-cast-points.md` | A | 201 | 0 |
| `alpha-combat-helper-tables.md` | A | 348 | 0 |
| `alpha-combat-stat-queries.md` | A | 294 | 0 |
| `alpha-combat-terrain-lighting.md` | A | 183 | 0 |
| `alpha-common-action-processing-state.md` | A | 141 | 0 |
| `alpha-common-hit-input-construction.md` | B | 203 | 1 |
| `alpha-companion-direct-route-geometry.md` | A | 109 | 1 |
| `alpha-connected-action-audio.md` | A | 315 | 0 |
| `alpha-connected-actor-turn.md` | A | 463 | 0 |
| `alpha-connected-combo-cleanup.md` | A | 88 | 0 |
| `alpha-connected-hit-consumption.md` | A | 304 | 0 |
| `alpha-connected-melee-actions-2-through-7.md` | A | 294 | 0 |
| `alpha-connected-melee-actions-30-through-33.md` | A | 394 | 0 |
| `alpha-connected-melee-actions-9-through-21.md` | A | 426 | 0 |
| `alpha-connected-melee-candidates.md` | A | 452 | 0 |
| `alpha-connected-projectile-receipt-step.md` | A | 459 | 0 |
| `alpha-connected-ranged-dispatch.md` | A | 257 | 0 |
| `alpha-connected-receipt-supplement.md` | A | 285 | 0 |
| `alpha-connected-walk-phases.md` | A | 317 | 0 |
| `alpha-controller-aim-writers.md` | A | 186 | 1 |
| `alpha-crafting.md` | A | 649 | 2 |
| `alpha-created-actor-behavior.md` | B | 122 | 2 |
| `alpha-created-actor-combat.md` | A | 379 | 0 |
| `alpha-created-candidate-appearances.md` | A | 95 | 1 |
| `alpha-created-companion-navigation.md` | B | 120 | 2 |
| `alpha-creature-light-attachment-producers.md` | A | 91 | 1 |
| `alpha-creature-map-key-order.md` | B | 70 | 1 |
| `alpha-creature-receipt-order.md` | A | 146 | 0 |
| `alpha-creature-update-phases.md` | A | 172 | 0 |
| `alpha-creature-yaw-light-gates.md` | A | 125 | 1 |
| `alpha-creatures-behaviour.md` | A | 986 | 0 |
| `alpha-creatures-models.md` | A | 1920 | 0 |
| `alpha-critical-and-haste-stat-numerics.md` | A | 342 | 1 |
| `alpha-data-stores.md` | A | 155 | 0 |
| `alpha-dedicated-projectile-step.md` | A | 405 | 0 |
| `alpha-descriptor-activation-refresh.md` | A | 89 | 1 |
| `alpha-descriptor-equipment-setup.md` | A | 133 | 1 |
| `alpha-descriptor-inventory-transfer.md` | A | 85 | 1 |
| `alpha-extractor.md` | A | 683 | 0 |
| `alpha-fatal-branch-order.md` | A | 202 | 0 |
| `alpha-fatal-context-field-producers.md` | A | 84 | 0 |
| `alpha-fatal-credit-level-consumer.md` | B | 108 | 2 |
| `alpha-fatal-descriptor-tail-lifecycle.md` | B | 103 | 1 |
| `alpha-fatal-disposal-callers.md` | A | 301 | 0 |
| `alpha-fatal-loot-branches-generator-recount.md` | A | 79 | 0 |
| `alpha-fatal-loot-branches.md` | A | 278 | 1 |
| `alpha-fatal-object-association-producers.md` | A | 102 | 2 |
| `alpha-fatal-object-descriptor-association.md` | A | 93 | 1 |
| `alpha-fatal-region-credit-order.md` | B | 143 | 1 |
| `alpha-fatal-world-drop-production.md` | A | 72 | 1 |
| `alpha-floating-point-environment.md` | B | 272 | 1 |
| `alpha-forward-link-zero-gates.md` | A | 99 | 1 |
| `alpha-front-end-widget-states.md` | A | 102 | 1 |
| `alpha-generic-hit-presentation-draws.md` | B | 65 | 1 |
| `alpha-generic-hit-record-production.md` | B | 116 | 3 |
| `alpha-generic-hit-source-auxiliary.md` | B | 119 | 2 |
| `alpha-generic-hit-vector-adjustment.md` | A | 76 | 0 |
| `alpha-hazards-blast-ticks.md` | A | 397 | 2 |
| `alpha-head-pitch-and-pick.md` | A | 215 | 0 |
| `alpha-hit-application-mutations.md` | A | 151 | 2 |
| `alpha-hit-item-presentation-vector.md` | A | 104 | 1 |
| `alpha-hit-response-vector-consumer.md` | A | 153 | 2 |
| `alpha-hit-scalar-reduction-order.md` | A | 70 | 0 |
| `alpha-hit-threat-death-lifecycle.md` | A | 157 | 1 |
| `alpha-hostility-test.md` | A | 188 | 0 |
| `alpha-house-furniture.md` | A | 222 | 1 |
| `alpha-interaction.md` | A | 773 | 3 |
| `alpha-inventory-equality-consumers.md` | A | 94 | 0 |
| `alpha-item-names.md` | A | 666 | 0 |
| `alpha-item-services.md` | A | 668 | 2 |
| `alpha-items-generator.md` | A | 426 | 0 |
| `alpha-light-admission-plane-production.md` | A | 104 | 1 |
| `alpha-light-propagation-lifecycle.md` | A | 229 | 0 |
| `alpha-linked-item-storage-producers.md` | A | 87 | 0 |
| `alpha-location-creatures.md` | A | 519 | 0 |
| `alpha-loot-rolls.md` | A | 503 | 0 |
| `alpha-materials-wildlife-routes.md` | A | 905 | 0 |
| `alpha-maximum-health-numeric-contract-recovered.md` | B | 135 | 3 |
| `alpha-missions.md` | A | 1062 | 0 |
| `alpha-mode-twenty-production.md` | A | 88 | 0 |
| `alpha-native-fixed-coordinates-terrain-light.md` | A | 110 | 0 |
| `alpha-native-initial-direct3d9-device.md` | A | 66 | 0 |
| `alpha-native-startup-window-input-boundary.md` | A | 132 | 1 |
| `alpha-native-light-rectangle-propagation.md` | A | 157 | 0 |
| `alpha-native-mesh-cache-light-padding.md` | A | 158 | 0 |
| `alpha-native-terrain-environment-palette.md` | A | 252 | 0 |
| `alpha-native-terrain-rgb-and-composition.md` | A | 207 | 0 |
| `alpha-native-terrain-vertex-light-alpha.md` | A | 194 | 0 |
| `alpha-net-packets.md` | A | 642 | 0 |
| `alpha-net-server.md` | A | 501 | 0 |
| `alpha-net-transport.md` | A | 450 | 8 |
| `alpha-network-creature-state-defaults.md` | B | 140 | 1 |
| `alpha-noise-cosine-boundary.md` | B | 118 | 2 |
| `alpha-noise-cosine-numeric-contract.md` | B | 217 | 2 |
| `alpha-opaque-output-transfer-state.md` | A | 105 | 1 |
| `alpha-objective-receipt-formatting-map-supplement.md` | A | 186 | 0 |
| `alpha-objective-state-one-late-interaction.md` | A | 103 | 0 |
| `alpha-open-points-1.md` | A | 522 | 0 |
| `alpha-open-points-10.md` | A | 390 | 1 |
| `alpha-open-points-11.md` | A | 212 | 1 |
| `alpha-open-points-12.md` | S | 357 | 1 |
| `alpha-open-points-2.md` | A | 465 | 1 |
| `alpha-open-points-3.md` | A | 696 | 0 |
| `alpha-open-points-4.md` | A | 668 | 1 |
| `alpha-open-points-5.md` | A | 485 | 0 |
| `alpha-open-points-6.md` | A | 697 | 0 |
| `alpha-open-points-7.md` | A | 256 | 1 |
| `alpha-open-points-8.md` | A | 291 | 0 |
| `alpha-open-points-9.md` | A | 114 | 0 |
| `alpha-ordinary-creature-link.md` | A | 148 | 0 |
| `alpha-particles.md` | A | 607 | 0 |
| `alpha-packet6-client-queue.md` | A | 333 | 0 |
| `alpha-packet6-placement-producers.md` | A | 127 | 0 |
| `alpha-packet6-quick-wheel.md` | A | 200 | 0 |
| `alpha-packet6-sender-turn.md` | A | 439 | 0 |
| `alpha-path-expansion-order.md` | A | 124 | 1 |
| `alpha-path-following-update.md` | A | 108 | 2 |
| `alpha-path-reroot-reference.md` | A | 14 | 0 |
| `alpha-path-search.md` | A | 622 | 0 |
| `alpha-path-terrain-classification.md` | A | 121 | 1 |
| `alpha-path-world-fallback.md` | A | 60 | 1 |
| `alpha-periodic-buff-hit.md` | B | 117 | 2 |
| `alpha-periodic-hit-application-details.md` | B | 98 | 2 |
| `alpha-periodic-hit-producer-state.md` | A | 127 | 2 |
| `alpha-pets-bombs.md` | A | 1057 | 0 |
| `alpha-physics-camera.md` | A | 944 | 0 |
| `alpha-player-control.md` | A | 406 | 0 |
| `alpha-pose-owner-inputs.md` | A | 212 | 1 |
| `alpha-pose-selectors-01-18.md` | A | 360 | 0 |
| `alpha-pose-selectors-19-24.md` | A | 354 | 0 |
| `alpha-pose-selectors-25-30.md` | A | 289 | 1 |
| `alpha-pose-selectors-31-49.md` | A | 536 | 0 |
| `alpha-power-helper-call-boundary.md` | B | 69 | 3 |
| `alpha-primary-terrain-fog-upload-state.md` | A | 82 | 0 |
| `alpha-projectile-world-ownership.md` | B | 105 | 3 |
| `alpha-projectiles.md` | A | 565 | 0 |
| `alpha-projection-input-production.md` | A | 97 | 2 |
| `alpha-protection-state-producers.md` | B | 107 | 1 |
| `alpha-ranged-action-scheduling.md` | B | 135 | 2 |
| `alpha-ranged-audio-boundaries.md` | B | 148 | 1 |
| `alpha-recipe-grant-entry-boundary.md` | B | 86 | 1 |
| `alpha-relationship-kind-89-admission.md` | A | 55 | 0 |
| `alpha-remote-creature-interpolation.md` | A | 223 | 0 |
| `alpha-renderer-fixed-point-reference-origins.md` | B | 77 | 1 |
| `alpha-rendering.md` | A | 1108 | 3 |
| `alpha-riding-actor-step.md` | A | 203 | 1 |
| `alpha-rock-color-precision.md` | B | 333 | 4 |
| `alpha-rulings-action-99-lens.md` | A | 141 | 0 |
| `alpha-rulings-audio-ui-sounds.md` | A | 131 | 3 |
| `alpha-rulings-beam-flag-strike-origin.md` | A | 117 | 1 |
| `alpha-rulings-camera-lens-pan.md` | A | 192 | 1 |
| `alpha-rulings-channel2-explosion-window.md` | A | 192 | 1 |
| `alpha-rulings-channel36-cadence.md` | A | 66 | 0 |
| `alpha-rulings-connected-client-receipt-order.md` | A | 206 | 0 |
| `alpha-rulings-connected-melee-30-through-33-stats.md` | A | 217 | 0 |
| `alpha-rulings-connected-melee-scalar-selectors.md` | A | 129 | 0 |
| `alpha-rulings-corridor-third-line.md` | A | 120 | 0 |
| `alpha-rulings-formation-angle-box.md` | A | 81 | 1 |
| `alpha-rulings-held-light-factor.md` | A | 88 | 0 |
| `alpha-rulings-knockback-snap.md` | A | 80 | 0 |
| `alpha-rulings-knockback-taming-charge.md` | A | 199 | 1 |
| `alpha-rulings-manacubebar-sequence.md` | A | 186 | 0 |
| `alpha-rulings-melee-reach.md` | A | 71 | 0 |
| `alpha-rulings-mission-rewards.md` | A | 172 | 0 |
| `alpha-rulings-packet6-bomb-default.md` | A | 112 | 0 |
| `alpha-rulings-pose-49-50-mount.md` | A | 216 | 0 |
| `alpha-rulings-pose-69-arc.md` | A | 179 | 0 |
| `alpha-rulings-projectile-point-impact.md` | A | 209 | 0 |
| `alpha-rulings-projectile-ray-and-variant.md` | A | 293 | 0 |
| `alpha-rulings-projectile-steering-and-impact.md` | A | 229 | 0 |
| `alpha-rulings-rendering-clouds-lights.md` | A | 487 | 1 |
| `alpha-rulings-shimmer-phase.md` | A | 107 | 0 |
| `alpha-rulings-spawn-rounds-ruin-routes.md` | A | 204 | 0 |
| `alpha-rulings-walk-flag-pitch.md` | A | 163 | 0 |
| `alpha-rulings-zone-fields.md` | A | 211 | 0 |
| `alpha-saves.md` | A | 499 | 0 |
| `alpha-selector17-periodic-record-production.md` | B | 100 | 0 |
| `alpha-server-buff-update-admission.md` | A | 144 | 3 |
| `alpha-server-hit-record-routing.md` | B | 77 | 1 |
| `alpha-server-mode-association-origins.md` | A | 84 | 1 |
| `alpha-server-simulation.md` | A | 732 | 0 |
| `alpha-server-vendor-stock.md` | A | 139 | 1 |
| `alpha-skills.md` | A | 624 | 0 |
| `alpha-sneak-light-inputs.md` | A | 283 | 0 |
| `alpha-spatial-association-producers.md` | A | 94 | 1 |
| `alpha-spatial-bucket-production.md` | A | 76 | 1 |
| `alpha-spatial-record-association-production.md` | A | 75 | 0 |
| `alpha-spatial-scalars-1ef040-1f0720.md` | B | 130 | 1 |
| `alpha-spawn-objects.md` | A | 590 | 1 |
| `alpha-stock-blueprint-placement.md` | A | 253 | 0 |
| `alpha-stock-blueprint-writers-and-persistence.md` | A | 239 | 0 |
| `alpha-specialized-vendor-stock.md` | A | 106 | 0 |
| `alpha-strike-call-sites.md` | A | 277 | 0 |
| `alpha-strike-cell-helpers.md` | A | 280 | 0 |
| `alpha-talk-vendors.md` | A | 698 | 1 |
| `alpha-taming-progress-and-capture.md` | A | 262 | 0 |
| `alpha-target-relation.md` | A | 178 | 0 |
| `alpha-template-stamping-markers.md` | A | 504 | 0 |
| `alpha-terrain-candidate-sort-abi.md` | B | 105 | 1 |
| `alpha-terrain-candidate-sort-order.md` | B | 86 | 1 |
| `alpha-terrain-chunk-point-lights.md` | A | 102 | 0 |
| `alpha-terrain-color-pipeline-public.md` | A | 112 | 2 |
| `alpha-terrain-light-source-lifecycle.md` | A | 123 | 1 |
| `alpha-terrain-shader-color-numerics.md` | A | 196 | 1 |
| `alpha-tick-period-rulings.md` | A | 145 | 0 |
| `alpha-timed-consumables.md` | A | 263 | 0 |
| `alpha-trails-poses-schedules.md` | A | 900 | 2 |
| `alpha-trap-generation.md` | A | 255 | 1 |
| `alpha-travel.md` | A | 641 | 0 |
| `alpha-ui-plasma.md` | A | 778 | 2 |
| `alpha-ui-screens.md` | A | 3719 | 2 |
| `alpha-vendor-stock-generation.md` | A | 113 | 1 |
| `alpha-vendor-widget-transactions.md` | A | 82 | 3 |
| `alpha-visibility-ray-geometry.md` | A | 85 | 0 |
| `alpha-voxel-fallback-record-initialization.md` | A | 87 | 1 |
| `alpha-water-material-contract.md` | B | 88 | 1 |
| `alpha-water-projectile-contact-event.md` | B | 113 | 3 |
| `alpha-weapon-attachment-poses.md` | A | 157 | 0 |
| `alpha-world-actor-creation.md` | B | 199 | 0 |
| `alpha-world-call-phase-boundary.md` | A | 443 | 0 |
| `alpha-world-creature-health-admission.md` | A | 84 | 0 |
| `alpha-world-hit-fatal-effects.md` | B | 91 | 1 |
| `alpha-world-hit-target-admission.md` | B | 101 | 2 |
| `alpha-world-item-list-population.md` | A | 163 | 0 |
| `alpha-world-items.md` | A | 547 | 0 |
| `alpha-world-objects.md` | A | 249 | 0 |
| `alpha-world-random-stream-boundaries.md` | A | 123 | 1 |
| `alpha-world-relationship-record-delivery.md` | B | 115 | 2 |
| `alpha-worldgen-caves.md` | A | 491 | 0 |
| `alpha-worldgen-clouds.md` | A | 91 | 0 |
| `alpha-worldgen-locations.md` | A | 508 | 0 |
| `alpha-worldgen-overview.md` | G | 210 | 4 |
| `alpha-worldgen-random.md` | A | 306 | 0 |
| `alpha-worldgen-regions.md` | A | 440 | 0 |
| `alpha-worldgen-structures.md` | A | 1740 | 0 |
| `alpha-worldgen-terrain.md` | A | 517 | 0 |
| `alpha-worldgen-trees.md` | A | 617 | 1 |
| `alpha-worldgen-vegetation.md` | A | 357 | 0 |
| `alpha-yaw-trig-scalar-abi.md` | A | 77 | 1 |
| `alpha-zone-spawn-lifecycle.md` | B | 111 | 1 |
| `shadows.md` | E | 194 | 3 |
| `sky-and-clouds.md` | E | 481 | 6 |
| `vegetation-wind.md` | E | 294 | 1 |
