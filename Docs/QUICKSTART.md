# DIVE QUICKSTART

## 1. Device setup

On the **device actor** (the thing being inspected):

| Component | Purpose |
|-----------|---------|
| `UDIVEInspectableComponent` | Session entry, camera, **context menu catalog** |
| `UDIVEAnchorComponent` | Optional semantic parts / focus points |
| Pickable primitives | Static/skeletal meshes, or Box/Sphere/Capsule collision volumes |

### Camera zoom (small parts)

**DIVE Inspectable → DIVE | Camera → Camera Settings** (or Device Definition `CameraSettings` when `bUseDeviceDefinitionSettings`):

| Property | Default | Purpose |
|----------|---------|---------|
| **Zoom Sensitivity** | 40 | Base step (cm) at the reference distance |
| **Scale Zoom With Orbit Distance** | on | Near = finer steps, far = larger |
| **Zoom Distance Reference Cm** | 200 | Distance where sensitivity maps 1:1 |
| **Min / Max Orbit Distance Cm** | 20 / 2000 | Hard zoom limits |
| **Focus Orbit Fit Multiplier** | 2.75 | Focus on a mesh → distance ≈ radius × this |
| **Focus Near Padding Factor** | 1.2 | Soft near floor ≈ radius × this (avoids diving into the part) |
| **Focus Blend Duration** | 0.35 | Seconds to blend the camera to a new focus (also on Device Definition) |

Focus on a primitive fits orbit distance to its bounds; wheel zoom scales with current distance.

### Collision volumes as pick targets

When the device is one solid mesh, add **Box / Sphere / Capsule Collision** on the same actor:

1. **Collision Enabled** = `Query Only` (or Query and Physics).
2. In **Collision Responses**, set **Visibility** = **Block**  
   (DIVE’s `Pick Trace Channel` defaults to Visibility — there is no separate “Pick” channel).  
   Avoid the **Trigger** profile: it sets Visibility to Ignore.
3. Shape can stay **Hidden in Game**.
4. Target bindings by **component tag / name** (or Any Primitive for built-ins). Use **Part Id** only when primitives are **attached under a DIVE Anchor** with that PartId — PartId is not the Box/mesh component name.

Pick uses a multi-hit ray and **prefers shape / `DIVE.PickProxy` volumes** over the shell mesh in front of them. Box/Sphere/Capsule are pick-proxies without the tag; `DIVE.PickProxy` is for hidden **meshes** used as hit volumes. If the box still never wins, check Visibility is Block and the component is on the device actor.

**Pickability rules**: a primitive is considered pickable if `IsVisible() && !bHiddenInGame` (or it is a pick-proxy with collision enabled). Primitives hidden via **Isolate** (`SetHiddenInGame(true)`) are automatically excluded from picking even though the renderer may report them as visible.

**Isolate** on a pick volume keeps device **meshes** visible (the shell) and hides other volumes / unrelated primitives. Clearing Isolate restores each component’s original Hidden-in-Game state (so authored-hidden boxes stay hidden).

**Hover overlay**: applied as `OverlayMaterial` on `UMeshComponent`. DIVE saves and restores any pre-existing overlay material owned by the device, so device-authored overlays survive hover highlight/clear. **Without a Hover Overlay Material (or a per-component override) hover is a no-op** — DIVE does not trace for highlight. Scan warns when neither is set.

Hover overlay still applies only to `UMeshComponent`.

### Custom context menu (`UDIVEDeviceAction`)

**On the device actor:** `UDIVEInspectableComponent` merges **Bindings** (on the component) + **Action Catalog** (Data Asset) — union by section. Prefer the catalog for device-specific ops.

| Layer | Question | Authoring |
|-------|----------|-----------|
| **Section** | Menu group (separator + optional Header label) | Component Sections / Catalog Sections |
| **Binding** | Which parts get which actions? | Target query (tag / name / PartId / Any Primitive) + Instanced actions + `SectionId` |
| **Action** | What runs? | Your BP/C++ subclass of `UDIVEDeviceAction` / Continuous; plus shipped Focus/Isolate/Admin |

**Content Browser → DIVE:**

| Create | Result |
|--------|--------|
| **Action Catalog** | Data Asset for sections + bindings |
| **Device Action** | BP parented to `DIVE Device Action` — override **Execute** |
| **Continuous Device Action** | BP for hold/drag — override **Begin / Update / End Interaction** |
| **Action Condition** | BP predicate — override **Evaluate** (hide the row **and** fail `CanExecute`) |

Then add an **instance** of your action BP inside a binding’s Actions array (Catalog or component Bindings). Shipped continuous mappers: **DIVE Rotary Drive Action** (knob), **DIVE Threaded Drive Action** (nut — detaches + Simulate Physics at `TurnsToRelease`), **DIVE Linear Drive Action** (slider / rail — centimetres along Axis), and **DIVE Momentary Press Action** (hold / PTT — value `1` while held, `0` on release). Instant click / toggle is **not** Press: a **DIVE Device Action** BP (`Execute` flips device state; `GetDisplayState.bChecked` reads it). **DIVE Notify Action** only when you do not need a menu checkmark — Notify does not set `bChecked`.

Default **`GetDisplayState`**: `CanExecute == false` → row **gray** (`bEnabled`); optional **Condition** false → row **hidden** (`bVisible`) **and** `CanExecute` false (Interact primary / LMB is blocked too). Override `GetDisplayState` only when you need a checked mark or a custom label — call Super, or keep both gates.

**Session** is a **World Subsystem** (`UDIVESessionSubsystem`). From Blueprints use **Get World Subsystem**, not Game Instance Subsystem. Runtime injects `ExecutionWorld` for the call so Blueprint world-context nodes work. Catalog copies are Outer=Inspectable and also resolve world via the actor.

**Headless / standing-VR host (no camera):** `UDIVEInspectableComponent::ExecuteAction` / `UpdateActiveInteraction` / `EndActiveInteraction` run the same Catalog / Bindings without `RequestSession()`. Pass view/ray via `MakeActionContext(..., bOverrideView=true, ...)`. Menu query: `AppendConfiguredContextMenuEntries(Pick, Entries, World)` hides Focus/Isolate (`Presentation=Session`). Camera-session menu uses the default `Session` filter.

On `UDIVEInspectableComponent`:

1. **Bindings** are filled when you add the component: Focus, Isolate, Simulate Physics, Delete Mesh. Any binding or action you delete in Details stays gone. **Add Admin Defaults** puts Simulate Physics + Delete Mesh back (creates the Admin binding or fills it if you emptied the actions). Native CDO stays empty; seeded inners are `RF_Public` so placed map actors can save.
2. Prefer **Action Catalog** for device-specific ops.

**Where to put logic (no BeginPlay dump):**

1. **Default:** put the handler in the **action BP** (`Execute` / continuous overrides). Use `Context.DeviceHost` / `Context.Target` there. Call a Blueprint Interface or function on the device when you need domain side-effects.
2. **Device-owned (no action logic):** in the binding’s Actions add **DIVE Notify Action**. Set **DisplayName** (menu label, default `Notify`) and optional **BindingId**. On the device Event Graph add **DIVE Action Event (DIVE Notify Action)**. Empty BindingId = any Notify slot; set BindingId to split two Notify rows. Use `Context.Target` when several parts share one binding (e.g. tagged lamps). Toggle / latching / N-position click is Instant, not Momentary Press: use a **DIVE Device Action** BP if the menu needs `bChecked` (override `GetDisplayState`, read the device). Notify + Action Event is enough when the device graph owns the flip and the row does not need a checkmark. Instant LMB needs `PrimaryActionIndex` (unlike a sole continuous action).
3. **Momentary hold (PTT, spring):** bind **DIVE Momentary Press Action** as Interact primary (or as the binding’s only continuous action). Down → `Value.Normalized = 1`, release → `0`. Cursor drag is ignored. Device: **DIVE Action Value Event (DIVE Momentary Press Action)** — not catalog `OnValueChanged`. **DIVE Action Event** fires at **Begin** (finger down), not at release. From the context menu a continuous action latches until the next primary — prefer LMB / trigger hold for PTT; do not put Press in the menu unless latch-until-click is the UX you want. **Do not** use Linear/Rotary Drive as a button.
4. **Optional Condition:** instanced on the action (`Condition`; **None** = always visible). Content Browser → **DIVE → Action Condition** → override **Evaluate(`Context`)**. Keep **Evaluate** pure (no flags, no Notify) — `GetDisplayState` may call it more than once. `false` omits the menu row and fails `CanExecute`, so the session will not Execute / Begin that action (menu click and Interact primary share `ExecuteResolvedAction`). It runs when the menu is **built** and again at Execute/Begin — after the last bolt pops, open the menu again or primary the cover. Read **device** state (`Context.DeviceHost` / `Context.Target`) — not fields on the catalog **asset template** (runtime executes per-Inspectable copies). If you override `CanExecute` on the action, call Super so Condition still applies. Cover-after-bolts recipe below.
5. **Device Action Event (EI-like):** on the device Actor BP Event Graph, search **DIVE Action Event** or browse **DIVE → Events**. Pick the entry for your action class. Empty **BindingId** (Details) = any instance of that class; set BindingId to fire only for that catalog/component slot (two Unscrew bindings can have two event nodes). Fires when that action succeeds on this actor's Inspectable (instant `Execute` returned true, or continuous **`Begin`** — start of the drag, **not** Threaded `TurnsToRelease`). Pins: typed `Action` + `Context` (`Context.BindingId` is filled from the matched slot). Still prefer putting heavy logic in the action BP; use the event when the device graph must react.
6. **Device Action Value Event (live drag):** same palette (**DIVE → Events**), same class + BindingId picker as Action Event — **not Rotary-only**. Any continuous class that calls `NotifyInteractionValue` / `NotifyValueChanged` (Rotate, Slide, Unscrew, Momentary Press, continuous BP). Fires while the session reports live `FDIVEInteractionValue` on **this** actor's Inspectable (not other devices) — every drag tick, not a one-shot “complete”. Pins: typed `Action` + `Context` + `Value`. Use `Value.Absolute` for MESS / sound. Threaded: `Absolute` is turns, `AbsoluteMax` is `TurnsToRelease`; a domain flag is `Absolute >= AbsoluteMax` (same frame as detach). Momentary Press: `Normalized` is `1` (down) or `0` (up), edges only. Unlimited Linear: `Absolute` is centimetres, `AbsoluteMax` is `0` (not a 0% position). **Do not** bind catalog `Action->OnValueChanged` from BeginPlay — the asset is a template; runtime executes copies. Bind DIVE Action Value Event on the device.
7. **Device fan-out (optional):** Inspectable **On Action Executed** / **On Action Value Changed** (Details **+**) still broadcast — useful for logging/debug. Prefer the K2 Event nodes over Cast chains.
8. **Avoid** wiring everything from device BeginPlay via `FindActionInstance` + per-action `OnExecuted` / `OnValueChanged` unless you intentionally listen to a specific Inspectable copy (or a foreign C++ action you own). Do not bind the catalog **asset** template.

Matching bindings are **unioned** by section (not winner-take-all). Duplicate DisplayNames in one section log a warning. Duplicate non-empty **BindingId** is an error.

Example — bolts then cover (Threaded Drive + Condition):

Two bindings, two actions. The cover does **not** subscribe to catalog `Action->OnExecuted`. **DIVE Action Event** on Threaded Drive fires at **Begin** (you started unscrewing), not when the nut detaches.

1. Tag bolt meshes `DIVE.Bolt`. Tag (or name) the cover `DIVE.Cover`. Visibility **Block** on the pick channel.
2. **DIVE → Action Catalog**, section e.g. `Maintenance`:
   - Binding Match = Component Tag `DIVE.Bolt` → instanced **DIVE Threaded Drive Action**. A binding whose only action is that drive is LMB primary. At `TurnsToRelease` the bolt **detaches** (`DetachFromComponent`) and **Simulate Physics**.
   - Binding Match = Component Tag `DIVE.Cover` → **DIVE Device Action** `BP_RemoveCover` (override **Execute**: hide/destroy the cover, or call a device interface; return **true** so Action Event can fire).
3. **DIVE → Action Condition** `BP_CoverAfterBolts`. **Evaluate** returns `true` only when no tagged bolt is still fastened. With shipped Threaded Drive, walk `Context.DeviceHost` for primitives tagged `DIVE.Bolt` (detach does **not** destroy them — do not count components). Treat a bolt as released when **attach parent is empty** (`DetachFromComponent`). Do **not** key off `IsSimulatingPhysics` alone: Admin **Simulate Physics** can enable it without unscrewing. If any tagged bolt still has an attach parent, return `false`. Assign this Condition on the **Remove Cover** instance — not on Threaded. Leave Threaded’s Condition **None**.
4. Assign the catalog on Inspectable. Optional: `PrimaryActionIndex` on the bolt binding if that binding has more than one action. Binding Details shows **Matched → Targets: N components** (same pick rules as the session) and **Select** highlights those meshes on a placed device or in the Blueprint viewport / Components tree. When several bindings match the pick, LMB uses the **most specific** Match Mode (Component Name > Part Id > Component Tag > Any Primitive); equal specificity keeps the **earlier** binding in the Bindings array (component, then catalog). Menu rows and sections follow the same array order.

Need a domain flag (MESS / sound) instead of querying meshes: **DIVE Action Value Event** on Threaded Drive — when `Value.Absolute >= Value.AbsoluteMax`, record `Context.Target` on the device; Condition reads that set. Same-frame as detach; earlier ticks are still below the cap. A custom Continuous BP that calls a device interface from `NotifyInteractionCompleted` is the same idea. State stays on the device, not on the catalog template.

**Continuous actions:**
- **Primary (hold):** press → `BeginInteraction` → drag while held → release → `EndInteraction`.
- **Context menu:** click row → `BeginInteraction` (modal drag without hold) → finish with next primary click / Escape / self-complete. Same LMB-up that confirms the menu row is ignored so the gesture is not cancelled immediately.
- Session marks the action active and subscribes to `OnValueChanged` **before** `Begin`, so the initial `NotifyInteractionValue` / `NotifyValueChanged` reaches the HUD. For self-finishing gestures call **`NotifyInteractionCompleted`** from `UpdateInteraction` (do not rely on setting a hidden flag in Begin).
- **Rotate / Unscrew:** drag around the rim in the plane perpendicular to the axis (polar angle maps 1:1 to degrees/turns). Edge-on views or a dead zone near the axis fall back to `DegreesPerPixel` on `ScreenDelta`. VR can reuse the same polar helper with a hand point later.
- **Slide:** drag along the rail — pointer maps 1:1 to centimetres on Axis (`MapPointerToAxisTravel`). Looking along the rail falls back to `CmPerPixel` on `ScreenDelta`.
- **Press:** no pointer mapping. Begin emits `1`, End emits `0`, Update is silent. Cancel still emits `0`.
- **Domain readout (optional, Rotary / Linear):** with angle/travel limited, enable **Use Domain Readout**, set **Domain Min/Max** and free **Readout Suffix** (`A`, `V`, `Ω`). Gesture stays in degrees/cm; HUD and Value Event expose `Absolute` as the lerp onto that scale. Not a second live state — mapping for chip/graph only. Unlimited wrap does not map to domain units.

Value HUD listens to session `OnInteractionValueChanged` (overridable widget class on DIVE Player). Default widget is a compact chip next to the driven primitive (cursor fallback). Payload is `FDIVEInteractionValue` (Normalized 0..1 plus signed Absolute + Unit / optional DisplaySuffix). Rotary/Threaded/Linear call **`NotifyInteractionValue`**; Momentary Press uses **`NotifyValueChanged`** (Normalized only). The widget formats the string — actions do not. **Pawn GRIP Physical drag does not** (cursor-pull has no normalized value).

Interaction parameters live on the action instance; device domain state lives on the device (see `Additional/DeviceInteractionModel.md`).

### Interact mode: hover highlight

**DIVE Inspectable → DIVE | Pick | Hover** (not in context menu catalog):

| Property | Purpose |
|----------|---------|
| **Hover Overlay Material** | Device-wide default for all interactive meshes under the cursor |
| **Pick Hover Overlay By Component** | Optional per-mesh override (component name or PartId → material) |

Uses `UMeshComponent::SetOverlayMaterial` (Interact mode only). Non-mesh primitives are skipped. Assign a material authored for mesh overlay (outline / tint). Empty default + empty per-component map = no hover trace.

### Pick interaction exclusions

**DIVE Inspectable → DIVE | Pick → Pick Interaction Exclusions** — component names or PartIds that DIVE ignores entirely: no context menu, no primary action, no hover.

Alternative: tag meshes with **Skip Component Tag** (`DIVE.Skip` by default) to exclude them from pick at a lower level (visibility/bounds rules still apply).

### Who does what (two actors)

```text
Player character (e.g. BP_FirstPersonCharacter)          Device actor (e.g. BP_MyDevice)
├─ DIVE Player  ← sole pawn DIVE ActorComponent        ├─ DIVE Inspectable  ← Action Catalog + Bindings
│                                                      └─ (optional) BP subclasses of UDIVEDeviceAction
└─ (optional) GRIP Rig (slots Player + Dive) for Physical
```

**Player character** — menu open + UI. Already handled by the plugin if **DIVE Player** and IMC are set up (§4). **You do not add device actions here.**

**Device actor** — **Action Catalog + Bindings** + action instances (defaults Focus/Isolate/Simulate/Delete, and/or your BP/C++ subclasses).

### Menu open path (already in plugin — do not override for device actions)

```text
IA_DIVE_ContextMenu (project IMC — e.g. RMB in Legacy PIE)
 → DIVE Player::HandleContextMenuRequested()  ← C++ entry, no BP wiring for device actions
 → Session subsystem builds rows, stores pick
 → DIVE Player shows the context menu widget at cursor
```

`HandleContextMenuRequested` is **not** your hook for device actions. It only opens/toggles the menu. It is called from **DIVE Player** on the **player character**, not from the device Blueprint.

DIVE Player on the player character **shows** the widget; it does not execute device actions.

### Menu click path

```text
Click menu row
 → UI → Session subsystem::ExecuteContextMenuAction(Action object)
 → UDIVEDeviceAction::Execute / BeginInteraction
```

Same path from **primary action** (`IA_DIVE_PrimaryAction` → `HandlePrimaryActionPressed`) in Interact mode when **`PrimaryActionIndex`** is set on a matching binding, or when that binding's **only** action is continuous (Rotate / Unscrew / Press). Winner among overlapping primaries: **Name > PartId > Tag > AnyPrimitive**; equal specificity → earlier binding in the list (Scan warns if more than one).

### Primary action path (production)

```text
IA_DIVE_PrimaryAction Started
 → DIVE Player::HandlePrimaryActionPressed()
 → Session::ExecutePrimaryActionAtScreenPosition (Interact mode)
 → most-specific matching binding PrimaryActionIndex (or the sole continuous action) → UDIVEDeviceAction::Execute / BeginInteraction
```

In Physical mode the same `HandlePrimaryAction*` routes to pawn GRIP grab only — not catalog primary actions.

Default rows come from component **Bindings** (Focus, Isolate, Simulate Physics, Delete Mesh). Device-specific rows: **Action Catalog** (and/or extra rows in Bindings). To hide Admin in editor, delete that binding or those actions; **Add Admin Defaults** restores them. Shipping hides Admin automatically.
---

## 1b. Anchors (optional)

| Field | Purpose |
|-------|---------|
| `PartId` | Semantic id for focus / bindings; **not** a component name |
| `DisplayName` | UI label |
| Transform / view rotation | Authored camera point |

Anchors are viewpoints — they are **not** pickable. A binding with Match Mode **Part Id** matches any **primitive attached under** that anchor in the Components hierarchy (Attach Parent). Sibling mesh/box next to the anchor does not inherit the PartId.

Focus via mesh pick, context menu, or `DefaultStartFocusId`. Optional **Show View Direction** arrow is editor-only (hidden in PIE/game).

---

## 2. Physical controls

**Paths**

| Path | When | Where state lives |
|------|------|-------------------|
| **1 — kinematic (now)** | Knobs, unscrewable nuts, sliders, levers; hold buttons | Mesh transform (drives) or 1/0 (Press). Bind **DIVE Rotary Drive Action** / **DIVE Threaded Drive Action** / **DIVE Linear Drive Action** / **DIVE Momentary Press Action** (Interact mode). Sounds / domain: K2 Action / Value Event on the device. |
| **Standing-VR** | Same parts without the DIVE camera | Same Catalog / Bindings via a host action host (**no** camera session). **Physical** mode is GRIP grab of free bodies. |

Do **not** add a second DIVE-only control asset layer — that would be another source of truth.

**Author a knob, nut, or slider on your device** (the plugin does not ship a sample actor):

1. On the moving mesh: a component tag (e.g. `DIVE.Knob` / `DIVE.Nut` / `DIVE.Slider`). Visibility **Block** on the pick channel. Mobility can stay Static — Begin promotes it to Movable. If the device body simulates, Begin isolates the child (unweld, Query Only) and does not disable parent physics. Engine **cylinder is Z-up**: default Rotary **Axis Z** is spin-in-place; default Linear **Axis X** is rail travel. **Min/Max** (Rotary angle, Linear travel) are from the part’s rest pose (authored transform on first grab), not from each mouse-down — releasing and grabbing again does not grant another full sweep. Extra drag past Min/Max is discarded; reversing leaves the stop immediately.
2. On `UDIVEInspectableComponent` Bindings (or Action Catalog): Match Mode **Component Tag**, that tag, instanced **DIVE Rotary Drive Action**, **DIVE Threaded Drive Action**, or **DIVE Linear Drive Action**. A binding whose only action is that drive is LMB primary; otherwise set `PrimaryActionIndex` to the drive slot.
3. Open a session from the device (`Request Session` / ACTS `OpenDIVE`). Interact mode: knobs/nuts — LMB-drag around the rim; sliders — drag along the rail (pointer maps to centimetres on Axis; edge-on falls back to `CmPerPixel`). Threaded: at `TurnsToRelease` the part detaches and simulates — then Physical + GRIP can pick it up.

**Generic simulating-mesh drag** — pawn physical drive (no device component):

1. Player character: **GRIP Rig** with slots **Player** and **Dive** (Hand is not in Add Component; Rig creates or adopts `GRIP Hand` / `GRIP Hand Dive`). `UDIVEPlayerComponent` creates the GRIP Physical Drive UObject when DIVEGRIPBridge is enabled. Provider resolves Dive via picker → `GetHand(Dive)` → name.
2. Admin context menu → **Simulate Physics** on a mesh.
3. Physical mode → primary-action hold drag moves the body via GRIP PD. Legacy PIE: **LMB** + drag; **hold R** + mouse move rotates the grabbed body.

See `Additional/DeviceInteractionModel.md` §6–§7 and `../../DIVEGRIPBridge/README.md`.

### Camera sensitivity

On `UDIVEInspectableComponent` → **DIVE | Camera → Camera Settings**, or the same `FDIVECameraSettings` on `UDIVEDeviceDefinitionAsset` when `bUseDeviceDefinitionSettings`.

## 3. ACTS entry (optional)

On the **device actor**, bind `UACTSInteractableComponent::OnActionExecuted` to
`UDIVEInspectableComponent::Try Request Session From Action Id`.

When ACTS executes ActionId **`OpenDIVE`** (`DIVE::kActionOpenDIVE`), the helper calls
`RequestSession()`. Any other ActionId is ignored.

Do not wire this in the game module C++ — Content/Blueprint on the device is the host glue.

## 4. Player character input

Recommended on **the locally controlled pawn** (not the device):

```text
UDIVEPlayerComponent             ← sole DIVE ActorComponent (Handle*, chrome, menu, optional Physical)
                                 ← auto GRIP Physical Drive UObject when DIVEGRIPBridge is enabled
UGRIPRigComponent                ← optional Physical + GRIP: slots Player + Dive (CDO has Player; add Dive).
                                 ← Hand is not in Add Component (Rig creates it).
```

Optional PIE-only: **`UDIVELegacyKbmInputComponent`** (`DIVERuntimeDev`) — Player does **not** create it.

`HandleExitSession` with the context menu open is intentional (Exit is not blocked by the menu).

### Enhanced Input (host Content)

Map once on the player character's IMC — **not** on the device:

| Input Action | Call on **DIVE Player** |
|--------------|-------------------------------------------|
| `IA_DIVE_Orbit` Started / Completed / Triggered | `HandleOrbitPressed` / `Released` / `HandleOrbitDelta` |
| `IA_DIVE_Zoom` | `HandleZoomIn` / `HandleZoomOut` (pawn-drive: same handlers move GRIP hold distance) |
| `IA_DIVE_PrimaryAction` | `HandlePrimaryActionPressed` / `Released` |
| `IA_DIVE_FocusTarget` | `HandleFocusUnderCursor` |
| `IA_DIVE_ContextMenu` | `HandleContextMenuRequested` |
| `IA_DIVE_SetMode_Physical` / `_Interact` | `SetInteractionMode` |
| `IA_DIVE_Back` / `IA_DIVE_Exit` | `HandleNavigateBack` / `HandleExitSession` |

### Legacy KBM (DIVERuntimeDev — dev only)

| Key | Action |
|-----|--------|
| MMB + drag | Orbit |
| Wheel | Zoom |
| **RMB** | Context menu (`HandleContextMenuRequested` via Legacy → DIVE Player) |
| **LMB** | `HandlePrimaryActionPressed` / `Released` (widget dismisses menu on pointer down outside panel) |
| **G** | Focus under cursor |
| **Tab** | Cycle Interact ↔ Physical |
| Ctrl+Z | Focus stack back |
| Backspace | Exit session |
| I | Isolate |

## 5. Editor / diagnostics

**DIVE Scan Device** (RMB on actor in level → DIVE) — validates anchors, **Catalog / Bindings** (`SectionId`, `PrimaryActionIndex`, MatchValues / AnyPrimitive), equal-specificity primary overlaps (warning; earlier binding wins), PartId→anchor coverage, shape pick-channel Block, exclusions.

**DIVE Dump Device** (same menu) or console in PIE:

| Command | What |
|---------|------|
| `DIVE.DumpDevice` | Active session device (or first inspectable). Optional arg: name substring, e.g. `DIVE.DumpDevice ElectricalPanel` |
| `DIVE.DumpAll` | Every actor with `UDIVEInspectableComponent` |

On success: cyan on-screen toast `DIVE dump saved: Saved/DIVE/Dumps/...` (same idea as MESS). Also **Output Log** filter `LogDIVE`, file under `Saved/DIVE/Dumps/`.

Use the dump when menu rows are missing: check **Bindings + Catalog** and per-prim **Menu rows** / **Resolved bindings**. Match modes: tag / name / PartId / AnyPrimitive.

## 6. Compliance

Runtime input: **`UDIVEPlayerComponent`** `Handle*` (no `BindKey` in `DIVERuntime`). Legacy KBM in `DIVERuntimeDev` for PIE. Physical keys and `IA_*` assets live in **host Content** — see `../../../Docs/Plugin_Architecture_Principles.md`.
