# DIVE Quickstart

> **Interaction model:** `Docs/DeviceInteractionModel.md` §4.

## 1. Device actor setup

Add to your device Blueprint or C++ actor:

- `UDIVEInspectableComponent` (session host)
- Mesh components on the same actor
- **Optional:** `UDIVEAnchorComponent` for authored camera viewpoints / PartId

### Focus navigation (explicit)

**Default mode:** primary action does **not** move the camera.

Focus via context menu (**Focus**) or **`HandleFocusUnderCursor()`** (`IA_DIVE_FocusTarget`).

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

### Per-mesh context menu (extension)

On **`UDIVEInspectableComponent`**, fill **`PickContextMenuActions`** — one row per mesh component name (Components tab):

| Field | Example |
|-------|---------|
| `ComponentName` | `DoorMesh` |
| `ActionId` | `Door.Open` |
| `DisplayName` | `Open` |

Override in the device Blueprint:

- **`ExecuteContextMenuAction(ActionId, PickTarget)`** — door open, lamp toggle, etc.
- **`IsPickContextMenuActionActive`** — drives the `*` suffix when the action is already on (optional)
- **`AppendContextMenuEntries`** — extra dynamic rows beyond the catalog (optional)

Built-in rows (Focus, Isolate, Simulate Physics, Delete Mesh) stay in the plugin; custom rows appear below a separator when the picked component name matches.

**Dismiss:** Escape, **LMB outside the panel**, or RMB again (toggle). LMB on a menu row runs that action; the same click does not pass through to the world.

### Physical controls

**Device DOF** (sliders, doors, knobs) — constraints + game state on device prefabs:

1. `SetInteractionMode(Physical)` (PIE: **Left Alt** cycles Default ↔ Physical).
2. Register via `IDIVEDeviceControlRegistry` or implement `IDIVEProxyDrive` on a **control component** (not raw mesh).

**Generic simulating-mesh drag** — pawn bridge (no device component):

1. Pawn: `UGRIPHandComponent` (`GrabPolicy = AllowSimulatingPhysics`) + `UDIVEGRIPBridgeComponent`.
2. Admin context menu → **Simulate Physics** on a mesh.
3. Physical mode → LMB drag moves the body via GRIP PD. While dragging, **hold R** + mouse move rotates the grabbed body.

See `DeviceInteractionModel.md` §6–§7 and `Source/DIVEGRIPBridge/README.md`.

### Camera sensitivity

On `UDIVEInspectableComponent` → **DIVE | Camera**, or shared `UDIVEDeviceDefinitionAsset`.

## 2. ACTS entry (optional)

On `UACTSInteractableComponent`: **ActionId** `OpenDIVE` → `RequestSession()` in game code.

## 3. Pawn input

**Monitor DIVE session pawn stack:**

```text
UDIVEInputComponent
UDIVEContextMenuUIComponent   (optional UI host)
UGRIPHandComponent            (GrabPolicy = AllowSimulatingPhysics)
UGRIPHandAimComponent         (optional)
UDIVEGRIPBridgeComponent      (generic Physical drag — not on device actors)
```

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
| **LMB** (menu open, outside panel) | Dismiss context menu |
| **G** | Focus under cursor |
| **Left Alt** | Cycle Default ↔ Physical |
| LMB | Primary action |
| Ctrl+Z | Focus stack back |
| Backspace | Exit session |
| I | Isolate |

## 4. Test in PIE

1. `RequestSession()` → camera blends to start focus.
2. **RMB** → context menu → Focus on mesh under cursor.
3. **Left Alt** → Physical → LMB on registered proxy-drive control **or** simulating mesh (with pawn GRIP bridge).
4. MMB orbit anytime; Ctrl+Z focus stack; Backspace exit.

### GRIP drag smoke (4 cubes)

1. Device: `UDIVEInspectableComponent` only (no GRIP bridge on device).
2. Pawn: `UGRIPHandComponent` + `UDIVEGRIPBridgeComponent` + `UDIVEInputComponent`.
3. RMB → **Simulate Physics** on a cube → **Left Alt** (Physical) → LMB drag. **Hold R** while dragging to rotate.
4. **Left Alt** back to Default → drag ends, aim restores.
