# DIVE Architecture (v0.7-dev)

## Layers

```text
┌─────────────────────────────────────────┐
│ Host project (ACTS entry, EI, PC)       │
├─────────────────────────────────────────┤
│ DIVERuntimeDev (optional, debug only)   │
│  UDIVELegacyKbmInputComponent — BindKey │
│  DIVEGripLegacyDevQuery (PIE wheel glue)│
├─────────────────────────────────────────┤
│ DIVEUnrealEditor (editor only)          │
│  DIVE Scan Device · validation reports  │
├─────────────────────────────────────────┤
│ DIVEGRIPBridge (optional §7)            │
│  UDIVEGRIPBridgeComponent — pawn physical │
│  drive via GRIP Hand API                │
├─────────────────────────────────────────┤
│ DIVERuntime                             │
│  UDIVESessionSubsystem — focus stack      │
│  UDIVEInputComponent — session input      │
│  UDIVEContextMenuUIComponent — context menu │
│  UDIVEInspectableComponent              │
│  UDIVEAnchorComponent (optional)        │
│  ADIVECameraRig                         │
│  UDIVEDeviceDefinitionAsset             │
├─────────────────────────────────────────┤
│ DIVECore — FDIVEFocusTarget, IDIVEProxyDrive, types │
└─────────────────────────────────────────┘
```

## Subsystem scope

**Current:** `UGameInstanceSubsystem` (`UDIVESessionSubsystem`).

One active DIVE session per game instance. Input components guard with `IsLocallyControlled()`.

## Input

| Component | Module | Role |
|-----------|--------|------|
| **DIVE Input** | `DIVERuntime` | Semantic `Handle*` API for Enhanced Input (no `EKeys` in runtime) |
| **DIVE Context Menu UI** | `DIVERuntime` | In-session menu widget; auto-found by Input |
| **Legacy KBM** | `DIVERuntimeDev` | `BindKey` only → forwards to **DIVE Input** (PIE dev) |

Recommended pawn stack: **`UDIVEInputComponent`** + **`UDIVEContextMenuUIComponent`** (+ optional **Legacy KBM** for PIE).

**Contract:** physical keys → `UInputAction` in host Content → Enhanced Input on **Pawn / PlayerController** → plugin `Handle*`. See `Project_docs/Plugin_Architecture_Principles.md` §4 and **`Docs/DeviceInteractionModel.md` §4**.

### Session chrome (always available)

Orbit, zoom, focus undo, exit, and context menu work in **every** interaction mode.

### Interaction mode

`EDIVESessionInteractionMode` in **DIVECore**: **Default** | **Physical**.

- Resets to **Default** on session start/end
- **Physical:** `TryBeginProxyDrive*` allowed; **Default:** primary action no-op

### Enhanced Input (host Content)

| Input Action | Event | Call |
|--------------|-------|------|
| `IA_DIVE_Orbit` | Started / Completed / Triggered | `HandleOrbit*` |
| `IA_DIVE_Zoom` | Triggered | `HandleZoomIn` / `HandleZoomOut` |
| `IA_DIVE_PrimaryAction` | Started / Completed | `HandlePrimaryActionPressed` / `Released` |
| `IA_DIVE_FocusTarget` | Started | `HandleFocusUnderCursor` |
| `IA_DIVE_Back` / `IA_DIVE_Exit` | Started | `HandleNavigateBack` / `HandleExitSession` |
| `IA_DIVE_SetMode_*` | Started | `SetInteractionMode` |
| `IA_DIVE_ContextMenu` | Started | `HandleContextMenuRequested` |

Legacy PIE: **RMB** = context menu, **G** = focus shortcut, **Left Alt** = cycle mode, **LMB** = primary action (Physical).

### Context menu

In-session menu at cursor — **not** ACTS. Built-in: **Focus**, **Isolate**, dev **Physics/Delete** on primitive pick; custom rows from **`PickContextMenuByComponent`** → **`Handle_{Key}_{ActionId}`** on device actor. **`UDIVEContextMenuUIComponent`** on player character draws the widget. Focus stack undo: `IA_DIVE_Back` (not in menu). **No menu on anchor pick** — LMB focuses anchor in Default mode.

## Device interaction (direct manipulation)

| Intent | Mechanism |
|--------|-----------|
| Focus / isolate / back | Context menu or `HandleFocusUnderCursor` |
| Read label, use control | Context menu row → **`Handle_{Key}_{ActionId}`** on device actor |
| Door, slider, knob | **Physical** mode + `IDIVEProxyDrive` / registry |
| Cable, grab | GRIP + MESS in host project |

## Proxy drive

Physical panel controls live on the **device**. DIVECore exposes **`IDIVEProxyDrive`** and **`IDIVEDeviceControlRegistry`** for **Physical** mode.

## Anchor (viewpoint only)

`UDIVEAnchorComponent`: `PartId`, `DisplayName`, authored camera viewpoint (transform + optional marker). No checklist operations.

**Session marker:** sphere in DIVE session (`bShowSessionMarker`). **DIVE Inspectable → DIVE | Anchor** (device mesh/material/scale) or **DIVE Anchor → DIVE | Marker** (per-anchor overrides). Color and opacity are authored in the material asset only.

## Device isolate

Context menu **Isolate** / **`ToggleIsolateFocused()`** — hides other meshes on the device host, keeps the focused branch visible. Level actors are not modified.

_Future:_ world-level focus dim via custom depth / post-process (not actor hiding).

## Session flow

1. `RequestSession()` → mode **Default**; camera blends to start focus.
2. Orbit (MMB), zoom, Ctrl+Z, Backspace exit — always.
3. **RMB** / `HandleContextMenuRequested` → Focus, Isolate at cursor pick.
4. **Left Alt** / `SetInteractionMode(Physical)` → LMB drives device controls.

## Editor

**DIVE Scan Device** — logs anchors and focus warnings.

Automation smoke tests: `DIVE.ContextMenu.BuiltInEntries`, `DIVE.PawnPhysicalDrive.Resolve` (Editor / PIE; not run in `UnrealEditor-Cmd` commandlet).

## Dependencies

| Module | ACTS | GRIP | MESS |
|--------|------|------|------|
| **DIVERuntime** | no | no | no |
| **DIVEGRIPBridge** | no | yes (§7 bridge) | no |
| **DIVERuntimeDev** | no | co-location only (§3.3 PIE) | no |

**DIVERuntime** does not link other gameplay plugins. **DIVEGRIPBridge** is the documented exception for Physical-mode pawn grab; disable the module in `.uplugin` if GRIP is not used. No Enhanced Input assets or `BindKey` in production Runtime modules.

Normative principles: [`Project_docs/Plugin_Architecture_Principles.md`](../../../Project_docs/Plugin_Architecture_Principles.md).
