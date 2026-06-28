# DIVE Quickstart

> **Interaction model:** `Docs/DeviceInteractionModel.md` §4 (session chrome + interaction mode).  
> **v0.5 skeleton:** Default mode — focus is **explicit** (`HandleFocusUnderCursor`); Physical mode — primary action drives controls.

## 1. Device actor setup

Add to your device Blueprint or C++ actor:

- `UDIVEInspectableComponent` (session host)
- **Any number of mesh components** on the same actor
- **Optional:** `UDIVEAnchorComponent` for semantic AOI, operations, authored camera points

### Focus navigation (explicit)

**Default interaction mode:** primary action does **not** move the camera.

Focus the mesh or anchor under the cursor via:

- **`HandleFocusUnderCursor()`** — bind to `IA_DIVE_FocusTarget` in ATSEP
- **Context menu** *(planned)* — `HandleContextMenuRequested` → «Сфокусироваться»

Legacy PIE dev mapping: **G** → focus under cursor.

Pick filters on `UDIVEInspectableComponent`:

| Property | Default | Purpose |
|----------|---------|---------|
| `SkipComponentTag` | `DIVE.Skip` | Ignore decorated / collision-only meshes |
| `MinPickBoundsRadius` | `0` | Skip tiny meshes |

### Anchor (viewpoint + scenario ops)

Anchors provide camera viewpoints and **scenario** `OperationIds` — not physical knobs.

| Property | Purpose |
|----------|---------|
| `PartId` | Semantic id; can match `DefaultStartFocusId` |
| `DisplayName` | UI label |
| `OperationIds` | Scenario steps (inspect, demount) |
| Marker / view arrows | Editor + session pick targets for focus |

### Physical controls

Sliders, doors, knobs — **device prefabs** (constraints + game state).

1. Set session mode: **`SetInteractionMode(Physical)`** (PIE dev: **P** cycles Default ↔ Physical).
2. Primary action on a registered control / `IDIVEProxyDrive` hit → drag DOF.

Registry on device (ATSEP) is the **primary** wiring path — see `DeviceInteractionModel.md` §6.

### Camera sensitivity

On `UDIVEInspectableComponent` → **DIVE | Camera**: `OrbitSensitivity`, `ZoomSensitivity`, `DefaultOrbitDistance`.

## 2. ACTS entry (optional)

On `UACTSInteractableComponent`: **ActionId** `OpenDIVE` → `RequestSession()` in game code.

## 3. Pawn input

**`UDIVEInputComponent`** on pawn — semantic `Handle*` API (no `EKeys` in runtime).

Optional PIE: **`UDIVELegacyKbmInputComponent`** forwards dev keys only.

### Enhanced Input (ATSEP Content)

| Input Action | Call on **DIVE Input** |
|--------------|------------------------|
| `IA_DIVE_Orbit` Started / Completed / Triggered | `HandleOrbitPressed` / `Released` / `HandleOrbitDelta` |
| `IA_DIVE_Zoom` | `HandleZoomIn` / `HandleZoomOut` |
| `IA_DIVE_PrimaryAction` *(or legacy `IA_DIVE_Select`)* | `HandlePrimaryActionPressed` / `Released` |
| `IA_DIVE_FocusTarget` | `HandleFocusUnderCursor` |
| `IA_DIVE_SetMode_Physical` / `_Default` | `SetInteractionMode` |
| `IA_DIVE_ExecuteOperation` | `HandleOperationExecute*` |
| `IA_DIVE_Back` / `IA_DIVE_Exit` | `HandleNavigateBack` / `HandleExitSession` |

### Legacy KBM (DIVERuntimeDev — dev only)

| Key | Action |
|-----|--------|
| MMB + drag | Orbit |
| Wheel | Zoom |
| **G** | Focus under cursor |
| **P** | Cycle Default ↔ Physical mode |
| LMB | Primary action (Physical: drive; Default: no-op) |
| F | Scenario operation |
| Ctrl+Z | Focus stack back |
| Backspace | Exit session |
| I | Isolate |

## 4. Operations wiring

Implement `OnOperationRequested` on `UDIVEInspectableComponent` for **scenario** steps — not continuous physical controls.

## 5. Test in PIE

1. `RequestSession()` → camera blends to start focus.
2. **G** → focus on mesh under cursor (orbit around target).
3. **P** → Physical mode → primary action on proxy-drive control (when implemented on device).
4. MMB orbit anytime; Ctrl+Z focus stack; Backspace exit.
