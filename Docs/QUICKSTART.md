# DIVE Quickstart

> **Interaction model:** `Docs/DeviceInteractionModel.md` §4.

## 1. Device actor setup

Add to your device Blueprint or C++ actor:

- `UDIVEInspectableComponent` (session host)
- Mesh components on the same actor
- **Optional:** `UDIVEAnchorComponent` for authored camera viewpoints / PartId

### Focus navigation (explicit)

**Default mode:** primary action does **not** move the camera.

Focus via context menu (**Focus here**) or **`HandleFocusUnderCursor()`** (`IA_DIVE_FocusTarget`).

Pick filters on `UDIVEInspectableComponent`:

| Property | Default | Purpose |
|----------|---------|---------|
| `SkipComponentTag` | `DIVE.Skip` | Ignore decorated / collision-only meshes |
| `MinPickBoundsRadius` | `0` | Skip tiny meshes |

### Anchor (viewpoint)

| Property | Purpose |
|----------|---------|
| `PartId` | Semantic id; can match `DefaultStartFocusId` |
| `DisplayName` | UI label |
| Transform / view rotation | Authored camera point |

### Device-specific menu actions

Override on `UDIVEInspectableComponent`:

- `AppendContextMenuEntries(PickTarget, …)` — add rows for picked mesh/part
- `ExecuteContextMenuAction(ActionId, PickTarget)` — handle custom ActionId

### Physical controls

Sliders, doors, knobs — on device prefabs (constraints + game state).

1. `SetInteractionMode(Physical)` (PIE: **P** cycles Default ↔ Physical).
2. Primary action on `IDIVEProxyDrive` / registry hit → drag DOF.

See `DeviceInteractionModel.md` §6.

### Camera sensitivity

On `UDIVEInspectableComponent` → **DIVE | Camera**, or shared `UDIVEDeviceDefinitionAsset`.

## 2. ACTS entry (optional)

On `UACTSInteractableComponent`: **ActionId** `OpenDIVE` → `RequestSession()` in game code.

## 3. Pawn input

**`UDIVEInputComponent`** + **`UDIVEContextMenuUIComponent`** on pawn.

Input auto-finds Context Menu UI by component class (ACTS-style). Optional PIE: **`UDIVELegacyKbmInputComponent`**.

### Enhanced Input (host Content)

| Input Action | Call on **DIVE Input** |
|--------------|------------------------|
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
| **RMB** | Context menu |
| **G** | Focus under cursor |
| **P** | Cycle Default ↔ Physical |
| LMB | Primary action |
| Ctrl+Z | Focus stack back |
| Backspace | Exit session |
| I | Isolate |

## 4. Test in PIE

1. `RequestSession()` → camera blends to start focus.
2. **RMB** → context menu → Focus on mesh under cursor.
3. **P** → Physical → LMB on proxy-drive control (when registered).
4. MMB orbit anytime; Ctrl+Z focus stack; Backspace exit.
