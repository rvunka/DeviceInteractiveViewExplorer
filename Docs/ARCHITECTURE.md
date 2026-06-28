# DIVE Architecture (v0.7-dev)

## Layers

```text
┌─────────────────────────────────────────┐
│ Host project (ACTS entry, EI, PC)       │
├─────────────────────────────────────────┤
│ DIVERuntimeDev (optional, debug only)   │
│  UDIVELegacyKbmInputComponent — BindKey │
├─────────────────────────────────────────┤
│ DIVEUnrealEditor (editor only)          │
│  DIVE Scan Device · validation reports  │
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

**Contract:** physical keys → `UInputAction` in host Content → `BindAction` on **PlayerController** → plugin `Handle*`. See `Project_docs/Plugin_Input_Architecture.md` and **`Docs/DeviceInteractionModel.md` §4**.

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

Legacy PIE: **RMB** = context menu, **G** = focus shortcut, **P** = cycle mode, **LMB** = primary action.

### Context menu

In-session menu at cursor — **not** ACTS. Built-in: Focus, Isolate, Back (when stack > 1); device rows via `AppendContextMenuEntries`. Opened via `HandleContextMenuRequested`.

## Device interaction (direct manipulation)

| Intent | Mechanism |
|--------|-----------|
| Focus / isolate / back | Context menu or `HandleFocusUnderCursor` |
| Read label, use control | Context menu row on pick (`AppendContextMenuEntries`) |
| Door, slider, knob | **Physical** mode + `IDIVEProxyDrive` / registry |
| Cable, grab | GRIP + MESS in host project |

## Proxy drive

Physical panel controls live on the **device**. DIVECore exposes **`IDIVEProxyDrive`** and **`IDIVEDeviceControlRegistry`** for **Physical** mode.

## Anchor (viewpoint only)

`UDIVEAnchorComponent`: `PartId`, `DisplayName`, authored camera viewpoint (transform + optional marker). No checklist operations.

## World dim

`EDIVEWorldDimPolicy` on `UDIVEInspectableComponent`: hide non-device actors during session (optional).

Device mesh isolate: **`ToggleIsolateFocused()`** via context menu.

## Session flow

1. `RequestSession()` → mode **Default**; camera blends to start focus.
2. Orbit (MMB), zoom, Ctrl+Z, Backspace exit — always.
3. **RMB** / `HandleContextMenuRequested` → Focus, Isolate, Back at cursor.
4. **P** / `SetInteractionMode(Physical)` → LMB drives device controls.

## Editor

**DIVE Scan Device** — logs anchors and focus warnings.

Automation smoke test: `DIVE.ContextMenu.BuiltInEntries`.

## Dependencies

DIVE **must not** link ACTS, GRIP, or MESS in **DIVERuntime**. No Enhanced Input in plugin modules.
