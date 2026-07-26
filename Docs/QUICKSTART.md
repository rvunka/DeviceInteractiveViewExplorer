# DIVE QUICKSTART

## 1. Device setup

On the **device actor** (the thing being inspected):

| Component | Purpose |
|-----------|---------|
| `UDIVEInspectableComponent` | Session entry, camera, **context menu catalog** |
| `UDIVEAnchorComponent` | Optional semantic parts / focus points |
| Pickable primitives | Static/skeletal meshes, or Box/Sphere/Capsule collision volumes |

### Camera zoom (small parts)

**DIVE Inspectable → DIVE | Camera** (or Device Definition when `bUseDeviceDefinitionSettings`):

| Property | Default | Purpose |
|----------|---------|---------|
| **Zoom Sensitivity** | 40 | Base step (cm) at the reference distance |
| **Scale Zoom With Orbit Distance** | on | Near = finer steps, far = larger |
| **Zoom Distance Reference Cm** | 200 | Distance where sensitivity maps 1:1 |
| **Min / Max Orbit Distance Cm** | 20 / 2000 | Hard zoom limits |
| **Focus Orbit Fit Multiplier** | 2.75 | Focus on a mesh → distance ≈ radius × this |
| **Focus Near Padding Factor** | 1.2 | Soft near floor ≈ radius × this (avoids diving into the part) |

Focus on a primitive fits orbit distance to its bounds; wheel zoom scales with current distance.

### Collision volumes as pick targets

When the device is one solid mesh, add **Box / Sphere / Capsule Collision** on the same actor:

1. **Collision Enabled** = `Query Only` (or Query and Physics).
2. In **Collision Responses**, set **Visibility** = **Block**  
   (DIVE’s `Pick Trace Channel` defaults to Visibility — there is no separate “Pick” channel).  
   Avoid the **Trigger** profile: it sets Visibility to Ignore.
3. Shape can stay **Hidden in Game**.
4. Wire context menu / handlers by the **collision component name**.

Pick uses a multi-hit ray and **prefers shape / `DIVE.PickProxy` volumes** over the shell mesh in front of them. If the box still never wins, check Visibility is Block and the component is on the device actor.

**Isolate** on a pick volume keeps device **meshes** visible (the shell) and hides other volumes / unrelated primitives. Clearing Isolate restores each component’s original Hidden-in-Game state (so authored-hidden boxes stay hidden).

Hover overlay still applies only to `UMeshComponent`.

### Custom context menu row (`IDIVEDeviceActionHandler`)

**Only on the device actor.** Implement **`IDIVEDeviceActionHandler`** → `HandleDeviceAction(CatalogKey, ActionId, Target, bActiveBefore)` and return **true** when handled.

`CatalogKey` and `ActionId` are **not** duplicates:

| Arg | Meaning | Example |
|-----|---------|---------|
| **CatalogKey** | which part (map key = component name / PartId) | `Screw1`, `CoverA` |
| **ActionId** | which operation on that part | `Unscrew`, `Open`, `Toggle` |

`HandleDeviceAction` is a **thin router**, not the place for all device logic. Prefer **Switch on `ActionId`**, then use `CatalogKey` / `Target` for the instance. Same action on many meshes = one branch + key/mesh, not one branch per mesh. Put heavy logic in separate functions / components; return `true` when handled.

1. (Optional params) Content Browser → **User Defined Struct** — e.g. `UnscrewParams` with `Turns`, `Torque`.
2. Content Browser → **DIVE** → **DIVE Device Action Definition**. Set `ActionId`, display name, optional `bToggleActiveSuffix`. Under **Params**, pick your struct and fill fields.
3. **DIVEInspectable** → **Pick Context Menu By Component** → key `Screw1` → row **Definition** = that asset. Optional row **DisplayName** override.
4. Optional: **`Primary Action Id`** = Definition `ActionId` (e.g. `Unscrew`).
5. Device BP: Interface **DIVE Device Action Handler** → Switch on `ActionId`. Read params via `TryGetResolvedActionRow` → `Definition` → `Params` (break / cast to your struct).
6. Compile.

Blueprint subclass of the Definition is still fine if you prefer normal BP variables instead of `Params`.

**Toggle row (`*` when active):** set **`bToggleActiveSuffix`** on the Definition.

1. Bool `Is_{Key}_{ActionId}` (default matches initial light state).
2. Handler reads **current** `Is_*` / `bActiveBefore` and applies the toggle.
3. DIVE then flips `Is_*` **after** the handler.

Do not also flip `Is_*` inside the handler.

Done. Action runs from the context menu, or from **primary action** when `Primary Action Id` is set.

### Default mode: hover highlight

**DIVE Inspectable → DIVE | Pick | Hover** (not in context menu catalog):

| Property | Purpose |
|----------|---------|
| **Hover Overlay Material** | Device-wide default for all interactive meshes under the cursor |
| **Pick Hover Overlay By Component** | Optional per-mesh override (component name or PartId → material) |

Uses `UMeshComponent::SetOverlayMaterial` (Default mode only). Non-mesh primitives are skipped. Assign a material authored for mesh overlay (outline / tint).

### Pick interaction exclusions

**DIVE Inspectable → DIVE | Pick → Pick Interaction Exclusions** — component names or PartIds that DIVE ignores entirely: no context menu, no handler, no primary action, no hover. Same key rules as the context menu catalog.

Alternative: tag meshes with **Skip Component Tag** (`DIVE.Skip` by default) to exclude them from pick at a lower level (visibility/bounds rules still apply).

### Who does what (two actors)

```text
Player character (e.g. BP_FirstPersonCharacter)          Device actor (e.g. BP_MyDevice)
├─ DIVE Input                                           ├─ DIVE Inspectable  ← catalog + Definition
├─ DIVE Context Menu UI  ← draws menu on screen        └─ IDIVEDeviceActionHandler
└─ (GRIP / bridge as needed)
```

**Player character** — menu open + UI. Already handled by the plugin if components and IMC are set up (§4). **You do not add device actions here.**

**Device actor** — catalog (**Definition** per row) + **`IDIVEDeviceActionHandler`**.

### Menu open path (already in plugin — do not override for device actions)

```text
IA_DIVE_ContextMenu (project IMC — e.g. RMB in Legacy PIE)
 → DIVE Input::HandleContextMenuRequested()     ← C++ entry, no BP wiring for device actions
 → Session subsystem builds rows, stores pick
 → DIVE Context Menu UI shows widget at cursor
```

`HandleContextMenuRequested` is **not** your hook for device actions. It only opens/toggles the menu. It is called from **DIVE Input** on the **player character**, not from the device Blueprint.

`DIVE Context Menu UI` on the player character only **shows** the widget; it does not run device handlers.

### Menu click path (handler)

```text
Click "Unscrew"
 → UI → Session subsystem::ExecuteContextMenuAction
 → DIVEInspectable → IDIVEDeviceActionHandler::HandleDeviceAction
```

Same handler from **primary action** (`IA_DIVE_PrimaryAction` → `HandlePrimaryActionPressed`) in Default mode when **`Primary Action Id`** is set on that catalog entry (no menu open).

### Primary action path (production)

```text
IA_DIVE_PrimaryAction Started
 → DIVE Input::HandlePrimaryActionPressed()
 → Session::ExecutePrimaryActionAtScreenPosition (Default mode)
 → DIVEInspectable → HandleDeviceAction on the device actor
```

In Physical mode the same `HandlePrimaryAction*` routes to proxy drive / GRIP — not catalog `Primary Action Id`.

Built-in rows (Focus, Isolate) are handled inside the plugin. **Simulate Physics / Delete Mesh** appear only when the active inspectable has **Enable Admin Context Menu Entries** checked (and not in Shipping). Custom catalog rows require **`UDIVEDeviceActionDefinition`** + **`IDIVEDeviceActionHandler`**.

---

## 1b. Anchors (optional)

| Field | Purpose |
|-------|---------|
| `PartId` | Semantic id; can match `DefaultStartFocusId` |
| `DisplayName` | UI label |
| Transform / view rotation | Authored camera point |

Focus via mesh pick, context menu, or `DefaultStartFocusId`. Optional **Show View Direction** arrow is editor-only (hidden in PIE/game).

---

## 2. Physical controls

**Device DOF** (sliders, doors, knobs) — constraints + game state on device prefabs:

1. `SetInteractionMode(Physical)` (`IA_DIVE_SetMode_Physical`; Legacy PIE: **Tab** cycles Default ↔ Physical).
2. Register via `IDIVEDeviceControlRegistry` or implement `IDIVEProxyDrive` on a **control component** (not raw mesh).

**Generic simulating-mesh drag** — pawn bridge (no device component):

1. Player character: `UGRIPHandComponent` (`GrabPolicy = AllowSimulatingPhysics`) + `UDIVEGRIPBridgeComponent`.
2. Admin context menu → **Simulate Physics** on a mesh.
3. Physical mode → primary-action hold drag moves the body via GRIP PD. Legacy PIE: **LMB** + drag; **hold R** + mouse move rotates the grabbed body.

See `DeviceInteractionModel.md` §6–§7 and `../../DIVEGRIPBridge/README.md`.

### Camera sensitivity

On `UDIVEInspectableComponent` → **DIVE | Camera**, or shared `UDIVEDeviceDefinitionAsset`.

## 3. ACTS entry (optional)

On `UACTSInteractableComponent`: **ActionId** `DIVE::kActionOpenDIVE` (`OpenDIVE`) → `RequestSession()` in game code.

## 4. Player character input

Components on **the player character** (not the device):

```text
UDIVEInputComponent              ← input handlers (including open menu)
UDIVEContextMenuUIComponent      ← menu widget host
UGRIPHandComponent               (GrabPolicy = AllowSimulatingPhysics)
UDIVEGRIPBridgeComponent         (generic Physical drag)
```

`UDIVEInputComponent` auto-finds `UDIVEContextMenuUIComponent` on the same actor. Optional PIE-only: **`UDIVELegacyKbmInputComponent`** (`DIVERuntimeDev`).

### Enhanced Input (host Content)

Map once on the player character's IMC — **not** on the device:

| Input Action | Call on **DIVE Input** (player character) |
|--------------|-------------------------------------------|
| `IA_DIVE_Orbit` Started / Completed / Triggered | `HandleOrbitPressed` / `Released` / `HandleOrbitDelta` |
| `IA_DIVE_Zoom` | `HandleZoomIn` / `HandleZoomOut` |
| `IA_DIVE_PrimaryAction` | `HandlePrimaryActionPressed` / `Released` |
| `IA_DIVE_FocusTarget` | `HandleFocusUnderCursor` |
| `IA_DIVE_ContextMenu` | `HandleContextMenuRequested` |
| `IA_DIVE_SetMode_Physical` / `_Default` | `SetInteractionMode` |
| `IA_DIVE_Back` / `IA_DIVE_Exit` | `HandleNavigateBack` / `HandleExitSession` |

Remove unused `IA_DIVE_ExecuteOperation` from Content / IMC if present.

### Legacy KBM (DIVERuntimeDev — dev only)

| Key | Action |
|-----|--------|
| MMB + drag | Orbit |
| Wheel | Zoom |
| **RMB** | Context menu (`HandleContextMenuRequested` via Legacy → DIVE Input) |
| **LMB** | `HandlePrimaryActionPressed` / `Released` (widget dismisses menu on pointer down outside panel) |
| **G** | Focus under cursor |
| **Tab** | Cycle Default ↔ Physical |
| Ctrl+Z | Focus stack back |
| Backspace | Exit session |
| I | Isolate |

## 5. Editor / diagnostics

**DIVE Scan Device** (RMB on actor in level → DIVE) — validates anchors, catalog keys, **`Primary Action Id`**, reserved `ActionId` values, exclusions, hover keys.

**DIVE Dump Device** (same menu) or console in PIE:

| Command | What |
|---------|------|
| `DIVE.DumpDevice` | Active session device (or first inspectable). Optional arg: name substring, e.g. `DIVE.DumpDevice ElectricalPanel` |
| `DIVE.DumpAll` | Every actor with `UDIVEInspectableComponent` |

On success: cyan on-screen toast `DIVE dump saved: Saved/DIVE/Dumps/...` (same idea as MESS). Also **Output Log** filter `LogDIVE`, file under `Saved/DIVE/Dumps/`.

Use the dump when custom menu rows are missing: compare **CatalogKey** to each prim **FName** / **Normalized**. Section **Cross-check** prints `OK` / `FAIL` per key. `CatalogMatch custom rows: (none)` on a Switch means the key did not match that component.

## 6. Compliance

Runtime input: `UDIVEInputComponent` **Handle\*** only (no `BindKey` in `DIVERuntime`). Legacy KBM in `DIVERuntimeDev` for PIE. Physical keys and `IA_*` assets live in **host Content** — see `../../../Docs/Plugin_Architecture_Principles.md`.
