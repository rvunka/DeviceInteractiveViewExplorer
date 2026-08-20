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
| **Action Condition** | BP predicate — override **Evaluate** (optional menu visibility) |

Then add an **instance** of your action BP inside a binding’s Actions array (Catalog or component Bindings). Shipped continuous mappers: **DIVE Rotary Drive Action** (knob) and **DIVE Threaded Drive Action** (nut — detaches + Simulate Physics at `TurnsToRelease`).

Default **`GetDisplayState`**: `CanExecute == false` → row **gray** (`bEnabled`); optional **Condition** false → row **hidden** (`bVisible`). Override `GetDisplayState` only when you need a checked mark or a custom label.

**Session** is a **World Subsystem** (`UDIVESessionSubsystem`). From Blueprints use **Get World Subsystem**, not Game Instance Subsystem. Catalog-hosted actions get a valid world via runtime `ExecutionWorld` injection — Blueprint world-context nodes work when the action runs.

On `UDIVEInspectableComponent`:

1. **Bindings** CDO-seed Focus + Isolate. **Seed Admin Defaults** (on by default) also seeds Simulate Physics / Delete Mesh. Uncheck it in Details to drop that seeded Admin binding; Shipping hides Admin rows regardless.
2. Prefer **Action Catalog** for device-specific ops.

**Where to put logic (no BeginPlay dump):**

1. **Default:** put the handler in the **action BP** (`Execute` / continuous overrides). Use `Context.DeviceHost` / `Context.Target` there. Call a Blueprint Interface or function on the device when you need domain side-effects.
2. **Device-owned (no action logic):** in the binding’s Actions add **DIVE Notify Action**. Set **DisplayName** (menu label, default `Notify`) and optional **BindingId**. On the device Event Graph add **DIVE Action Event (DIVE Notify Action)**. Empty BindingId = any Notify slot; set BindingId to split two Notify rows. Use `Context.Target` when several parts share one binding (e.g. tagged lamps).
3. **Optional Condition:** Content Browser → **DIVE → Action Condition** → `Evaluate` returns false to **hide** the menu row (example: “Remove Cover” only after bolts are gone). Assign the Condition instance on the action. Leave **None** if always visible.
4. **Device Action Event (EI-like):** on the device Actor BP Event Graph, search **DIVE Action Event** or browse **DIVE → Events**. Pick the entry for your action class. Empty **BindingId** (Details) = any instance of that class; set BindingId to fire only for that catalog/component slot (two Unscrew bindings can have two event nodes). Fires when that action succeeds on this actor's Inspectable (instant `Execute` returned true, or continuous `Begin`). Pins: typed `Action` + `Context` (`Context.BindingId` is filled from the matched slot). Still prefer putting heavy logic in the action BP; use the event when the device graph must react.
5. **Device fan-out (optional):** Inspectable **On Action Executed** (Details **+**) still broadcasts every successful action — useful for logging/debug. Prefer DIVE Action Event nodes over Cast chains.
6. **Avoid** wiring everything from device BeginPlay via `FindActionInstance` + per-action `OnExecuted` unless you intentionally listen to a shared C++/foreign action class.

Matching bindings are **unioned** by section (not winner-take-all). Duplicate DisplayNames in one section log a warning. Duplicate non-empty **BindingId** is an error.

Example — tagged bolts → continuous unscrew BP:

1. Tag bolt meshes `DIVE.Bolt`.
2. Content Browser → **DIVE → Continuous Device Action** → `BP_Unscrew` (params + Begin/Update/End; on complete call device interface e.g. `NotifyBoltRemoved`).
3. **DIVE → Action Catalog** → section `Maintenance`, binding Match=Component Tag `DIVE.Bolt`, add `BP_Unscrew` instance.
4. Assign catalog on Inspectable. Optional: `PrimaryActionIndex` for LMB in Interact mode. Binding Details shows **Matched → Targets: N components** (same pick rules as the session) and **Select** highlights those meshes on a placed device. When several bindings match the pick, LMB uses the **most specific** Match Mode (Component Name > Part Id > Component Tag > Any Primitive); equal specificity keeps the **earlier** binding in the Bindings array (component, then catalog). Menu rows and sections follow the same array order.

**Continuous actions:**
- **Primary (hold):** press → `BeginInteraction` → drag while held → release → `EndInteraction`.
- **Context menu:** click row → `BeginInteraction` (modal drag without hold) → finish with next primary click / Escape / self-complete. Same LMB-up that confirms the menu row is ignored so the gesture is not cancelled immediately.
- Session marks the action active and subscribes to `OnValueChanged` **before** `Begin`, so the initial `NotifyValueChanged` reaches the HUD. For self-finishing gestures call **`NotifyInteractionCompleted`** from `UpdateInteraction` (do not rely on setting a hidden flag in Begin).

Value HUD listens to session `OnInteractionValueChanged` (overridable widget class on DIVE Player). Continuous actions feed it via **`NotifyValueChanged`** (BlueprintCallable). `UDIVEProxyDriveForwardAction` polls `IDIVEProxyDrive::GetProxyDriveNormalizedValue` at Begin and after each delta. **Pawn GRIP Physical drag does not** (cursor-pull has no normalized value).

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

**Device actor** — **Action Catalog + Bindings** + action instances (defaults Focus/Isolate, Admin if Seed Admin Defaults, and/or your BP/C++ subclasses).

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

Same path from **primary action** (`IA_DIVE_PrimaryAction` → `HandlePrimaryActionPressed`) in Interact mode when **`PrimaryActionIndex`** is set on a matching binding. Winner among overlapping primaries: **Name > PartId > Tag > AnyPrimitive**; equal specificity → earlier binding in the list (Scan warns if more than one).

### Primary action path (production)

```text
IA_DIVE_PrimaryAction Started
 → DIVE Player::HandlePrimaryActionPressed()
 → Session::ExecutePrimaryActionAtScreenPosition (Interact mode)
 → most-specific matching binding PrimaryActionIndex → UDIVEDeviceAction::Execute / BeginInteraction
```

In Physical mode the same `HandlePrimaryAction*` routes to proxy drive / GRIP — not catalog primary actions.

Default rows come from component **Bindings** (Focus, Isolate, Admin if **Seed Admin Defaults**). Device-specific rows: **Action Catalog** (and/or extra rows in Bindings). To hide Admin in editor, uncheck **Seed Admin Defaults** or remove those actions from Bindings; Shipping hides them automatically.
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

**Two tiers**

| Tier | When | Where state lives |
|------|------|-------------------|
| **1 — kinematic (now)** | Knobs, unscrewable nuts, levers that only need to move in the DIVE session | Mesh transform. Bind **DIVE Rotary Drive Action** / **DIVE Threaded Drive Action** (Interact mode, same hold/drag as any continuous action). |
| **2 — device control (by trigger)** | VR parity, Chaos constraints, MESS, the same part usable outside DIVE | Control component on the device + `IDIVEProxyDrive` / registry. Same **Interact** gesture on the monitor; standing-VR reuses Catalog / Bindings via a host action host (**no** camera session). **Physical** mode is GRIP grab of free bodies. DIVE ships the contract; host implements it (zero in-plugin backends today). |

Do **not** add a second DIVE-only control asset layer — that would be another source of truth.

**Author a knob or nut on your device** (the plugin does not ship a sample actor):

1. On the moving mesh: a component tag (e.g. `DIVE.Knob` / `DIVE.Nut`). Visibility **Block** on the pick channel.
2. On `UDIVEInspectableComponent` Bindings (or Action Catalog): Match Mode **Component Tag**, that tag, instanced **DIVE Rotary Drive Action** or **DIVE Threaded Drive Action**, `PrimaryActionIndex = 0`.
3. Open a session from the device (`Request Session` / ACTS `OpenDIVE`). Interact mode: LMB-drag is the primary. Threaded: at `TurnsToRelease` the part detaches and simulates — then Physical + GRIP can pick it up.

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
