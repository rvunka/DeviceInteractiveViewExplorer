# DIVE QUICKSTART

## 1. Device setup

On the **device actor** (the thing being inspected):

| Component | Purpose |
|-----------|---------|
| `UDIVEInspectableComponent` | Session entry, camera, **context menu catalog** |
| `UDIVEAnchorComponent` | Optional semantic parts / focus points |
| Pickable meshes | Static/skeletal meshes on the actor |

### Custom context menu row (example: Unscrew on screw)

**Only on the device actor.** No Events, no delegate, no Switch.

1. **DIVEInspectable** → **Pick Context Menu By Component** → add key `Screw1`, row `ActionId` = `Unscrew`, `DisplayName` = menu label.
2. Optional on the same catalog entry: **`Primary Action Id`** = `Unscrew` (primary action in Default mode invokes the same handler as the menu row).
3. **My Blueprint → Functions → +** → name **`Handle_Screw1_Unscrew`** (pattern: `Handle_{map key}_{ActionId}`).
4. Optional input: **`TargetComponent`** (Primitive Component) = picked mesh.
5. Function body: your logic (`UnscrewByRef`, etc.). Compile.

**Toggle row (`*` when active):** enable **`Toggle Active Suffix`** on the catalog row. On the device actor add either:

- bool **variable** `Is_Screw1_Unscrew` (flip it in `Handle_Screw1_Unscrew`), or
- pure bool **function** `Is_Screw1_Unscrew` that returns the state.

`*` is evaluated when the menu **opens** — after click close the menu and open again (`IA_DIVE_ContextMenu`) to refresh the label.

Done. **Unscrew** runs from the context menu, or from **primary action** when `Primary Action Id` is set.

### Default mode: hover highlight

**DIVE Inspectable → DIVE | Pick | Hover** (not in context menu catalog):

| Property | Purpose |
|----------|---------|
| **Hover Overlay Material** | Device-wide default for all interactive meshes under the cursor |
| **Pick Hover Overlay By Component** | Optional per-mesh override (component name or PartId → material) |

Uses `UMeshComponent::SetOverlayMaterial` (Default mode only). Non-mesh primitives are skipped. Assign a material authored for mesh overlay (outline / tint).

### Pick interaction exclusions

**DIVE Inspectable → DIVE | Pick → Pick Interaction Exclusions** — component names or PartIds that DIVE ignores entirely: no context menu, no `Handle_*`, no primary action, no hover. Same key rules as the context menu catalog.

Alternative: tag meshes with **Skip Component Tag** (`DIVE.Skip` by default) to exclude them from pick at a lower level (visibility/bounds rules still apply).

### Who does what (two actors)

```text
Player character (e.g. BP_FirstPersonCharacter)          Device actor (e.g. BP_MyDevice)
├─ DIVE Input                                           ├─ DIVE Inspectable  ← catalog here
├─ DIVE Context Menu UI  ← draws menu on screen        └─ Handle_Screw1_Unscrew  ← your logic here
└─ (GRIP / bridge as needed)
```

**Player character** — menu open + UI. Already handled by the plugin if components and IMC are set up (§4). **You do not add Unscrew here.**

**Device actor** — catalog + `Handle_*` functions.

### Menu open path (already in plugin — do not override for Unscrew)

```text
IA_DIVE_ContextMenu (project IMC — e.g. RMB in Legacy PIE)
 → DIVE Input::HandleContextMenuRequested()     ← C++ entry, no BP wiring for device actions
 → Session subsystem builds rows, stores pick
 → DIVE Context Menu UI shows widget at cursor
```

`HandleContextMenuRequested` is **not** your hook for device actions. It only opens/toggles the menu. It is called from **DIVE Input** on the **player character**, not from the device Blueprint.

`DIVE Context Menu UI` on the player character only **shows** the widget; it does not run `Handle_Screw1_Unscrew`.

### Menu click path (your `Handle_*` function)

```text
Click "Unscrew"
 → UI → Session subsystem::ExecuteContextMenuAction
 → DIVEInspectable calls Handle_Screw1_Unscrew on the device actor
```

Same handler from **primary action** (`IA_DIVE_PrimaryAction` → `HandlePrimaryActionPressed`) in Default mode when **`Primary Action Id`** is set on that catalog entry (no menu open).

### Primary action path (production)

```text
IA_DIVE_PrimaryAction Started
 → DIVE Input::HandlePrimaryActionPressed()
 → Session::ExecutePrimaryActionAtScreenPosition (Default mode)
 → DIVEInspectable → Handle_Screw1_Unscrew on the device actor
```

In Physical mode the same `HandlePrimaryAction*` routes to proxy drive / GRIP — not catalog `Primary Action Id`.

Built-in rows (Focus, Isolate, Simulate Physics, Delete Mesh) are handled inside the plugin, not via `Handle_*`.

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

See `DeviceInteractionModel.md` §6–§7 and `Source/DIVEGRIPBridge/README.md`.

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

## 5. Editor

**DIVE device scan** (editor utility on selected actor with `UDIVEInspectableComponent`) — validates anchors, catalog keys, **`Primary Action Id`**, reserved `ActionId` values, **Pick Interaction Exclusions**, and **Pick Hover Overlay By Component** keys.

## 6. Compliance

Runtime input: `UDIVEInputComponent` **Handle\*** only (no `BindKey` in `DIVERuntime`). Legacy KBM in `DIVERuntimeDev` for PIE. Physical keys and `IA_*` assets live in **host Content** — see `Project_docs/Plugin_Architecture_Principles.md`.
