# mod_combat_override (M7)

Standalone C11 native module; links only the published C ABI.
Build: cmake -S modules/native/mod_combat_override -B <build>; cmake --build <build> --config RelWithDebInfo.

Subscribes to combat during on_init. Damage > 5000 is integer-halved through the host stack AeternumDamageMutation buffer; otherwise PASS. No module allocation, HostAlloc, PASS logging or per-session state. Actual overrides emit fixed-stack raw/override/spell/GUID telemetry through host log_info. Host logging can allocate internally; the module damage calculation and OUT buffer do not use the heap. Only aeternum_module_entry is exported.

ABI v1 is extended by struct_size: host subscribe_combat/subscribe_spell are appended, old PASS/DROP/MUTATE values stay 0/1/2. OVERRIDE_DAMAGE=3 and CANCEL_SPELL=4 are event-specific. Both GUID words are exposed (raw low/high), preserving the core 128-bit ObjectGuid.

All damage subscribers see the original base_damage. Each receives a fresh host stack OUT slot; PASS/fault/unknown verdict ignores its contents, last successful OVERRIDE wins. Faults quarantine the module and arm the existing taint/60s graceful policy. Four callbacks per event, boot-only subscription then freeze.

Spell interception runs after explicit target setup and SpellEvent ownership, but before disables, scripts, CheckCast and actual cast. CANCEL returns SPELL_FAILED_INTERRUPTED using the existing SendCastResult/finish failure cleanup; the event reclaims the spell. No spell final damage exists at prepare: base_damage is explicitly 0, and actual hit damage is exposed by the combat hook.

Catch2 [m7][blocco1] loads the real module plus test-only cancellation/fault images. Spell cancellation uses a native-body double and independent source wiring assertions; it is not a live-client spell QA. No deploy.

Live QA variant 0.2.0 also subscribes to Spell::prepare and cancels only Healing Touch (5185), learned and on action bar slot 2 in Roky's isolated 22:06:18 Europe/Rome PKT capture. Other spells PASS without logs. Actual cancellations emit fixed-stack spell/GUID event telemetry. The GM no-school damage command now sends DealDamage's returned result, not the stale requested amount, to the client combat log.
