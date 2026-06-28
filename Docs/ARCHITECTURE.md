# DIVE Architecture (v0.4.0-dev)

## Layers

```text
┌─────────────────────────────────────────┐
│ ATSEP (game module: ACTS entry, EI)      │
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
│  UDIVEOperationsUIComponent — ops list  │
│  UDIVEContextMenuUIComponent — context menu │
│  UDIVEInspectableComponent              │
│  UDIVEAnchorComponent (optional)        │
│  ADIVECameraRig                         │
│  UDIVEDeviceDefinitionAsset             │
├─────────────────────────────────────────┤
│ DIVECore — FDIVEFocusTarget, IDIVEProxyDrive, types │
└─────────────────────────────────────────┘
```

## Subsystem scope (decision)

**Current:** `UGameInstanceSubsystem` (`UDIVESessionSubsystem`).

**Rationale:** ATSEP v0.2 is single-player / one local modal session per game instance. One active DIVE session matches training-sim UX. Input components already guard with `IsLocallyControlled()`.

**Future:** migrate to `ULocalPlayerSubsystem` if split-screen or multiple independent DIVE viewports are required (roadmap v0.5+).

## Input

| Component | Module | Role |
|-----------|--------|------|
| **DIVE Input** | `DIVERuntime` | Semantic `Handle*` API for Enhanced Input (no `EKeys` in runtime) |
| **DIVE Operations UI** | `DIVERuntime` | Operation list + Hold progress during session |
| **Legacy KBM** | `DIVERuntimeDev` | `BindKey` only → forwards to **DIVE Input** (PIE dev) |

Recommended pawn stack: **`UDIVEInputComponent`** + **`UDIVEOperationsUIComponent`** + **`UDIVEContextMenuUIComponent`** (+ optional **Legacy KBM** for PIE).

**Contract:** physical keys → `UInputAction` in **ATSEP Content** → `BindAction` on **PlayerController** → plugin `Handle*`. See `Project_docs/Plugin_Input_Architecture.md` and **`Docs/DeviceInteractionModel.md` §4**.

### Session chrome (always available)

Orbit, zoom, focus undo, exit, and (target) context menu work in **every** interaction mode. Camera navigation is **not** a separate «Navigate mode».

### Interaction mode (v0.5 skeleton)

`EDIVESessionInteractionMode` in **DIVECore**: **Default** | **Physical**.

- **`GetInteractionMode` / `SetInteractionMode`** on `UDIVESessionSubsystem` and `UDIVEInputComponent`
- Resets to **Default** on session start/end
- **Physical:** `TryBeginProxyDrive*` allowed; **Default:** primary action no-op

Full spec: **`DeviceInteractionModel.md` §4.2**.

### Enhanced Input — implemented (v0.5 skeleton)

| Input Action | Event | Call |
|--------------|-------|------|
| `IA_DIVE_Orbit` | Started / Completed / Triggered | `HandleOrbit*` |
| `IA_DIVE_Zoom` | Triggered | `HandleZoomIn` / `HandleZoomOut` |
| `IA_DIVE_PrimaryAction` | Started / Completed | `HandlePrimaryActionPressed` / `Released` |
| `IA_DIVE_Select` | Started / Completed | Same as PrimaryAction *(deprecated alias)* |
| `IA_DIVE_FocusTarget` | Started | `HandleFocusUnderCursor` |
| `IA_DIVE_ExecuteOperation` | Started / Completed | `HandleOperationExecute*` |
| `IA_DIVE_Back` / `IA_DIVE_Exit` | Started | `HandleNavigateBack` / `HandleExitSession` |
| `IA_DIVE_SetMode_*` | Started | `SetInteractionMode` |
| `IA_DIVE_ContextMenu` | Started | `HandleContextMenuRequested` |

Legacy PIE: **RMB** = context menu, **G** = focus (shortcut), **P** = cycle mode, **LMB** = primary action.

### Enhanced Input — optional

| Input Action | Call |
|--------------|------|
| `IA_DIVE_ToggleIsolate` | `HandleToggleIsolate` |

### DIVE context menu (v0.6)

In-session menu at cursor — **not** ACTS. Built-in: Focus, Isolate, Back (when stack > 1); device rows via `AppendContextMenuEntries`. Opened via `HandleContextMenuRequested`. See **`DeviceInteractionModel.md` §4.3**.

## Operations (v0.2)

- Anchors expose `OperationIds`; metadata lives in `UDIVEDeviceDefinitionAsset::OperationCatalog`.
- `EDIVEOperationInputMode`: **Press** (instant) or **Hold** (timer, ACTS-compatible UX).
- `ValidationRules` on device definition gate operations by `RequiredCompletedOperationIds`.
- `UDIVEOperationsUIComponent` shows the list; `RequestFocusedOperation` validates, dispatches `OnOperationRequested`, marks success in session state.

## Proxy drive (v0.4-dev, interim)

Physical panel controls live on the **device** with constraints and game state. DIVECore exposes **`IDIVEProxyDrive`** as one backend for **Physical** mode (target), not as the primary UX model.

- **`IDIVEProxyDrive`** — `CanProxyDrive`, `BeginProxyDrive`, `ApplyProxyDriveDelta`, `EndProxyDrive`
- **`FDIVEProxyDriveContext`** — screen position, focus target snapshot, hit component

**Today:** `HandlePrimaryAction*` routes by `GetInteractionMode()`. Default → no-op. Physical → `TryBeginProxyDrive*`.

**Planned:** device registry as primary backend for Physical mode.

No GRIP / ATSEP dependency in `DIVERuntime`. Full contract: **`Docs/DeviceInteractionModel.md` §4–§6**.

## Anchor (viewpoint + semantics)

`UDIVEAnchorComponent` provides:

- `PartId`, `DisplayName`, `OperationIds`
- Authored camera viewpoint (transform + optional marker)
- **Not** kinematic hinge / manipulator physics (removed in 0.4-dev)

## World dim (v0.3)

`EDIVEWorldDimPolicy` on `UDIVEInspectableComponent`:

| Policy | Behaviour |
|--------|-----------|
| `None` | No actor hiding during session |
| `HideNonDeviceActors` | Hide other level actors during session (includes player pawn) |

Device mesh isolate remains **`ToggleIsolateFocused()`** (explicit, separate from world dim).

## Session flow

### Today (v0.6)

1. `RequestSession()` → mode **Default**; camera blends to start focus.
2. **Chrome always:** orbit (MMB), zoom, Ctrl+Z, Backspace exit.
3. **RMB** / `HandleContextMenuRequested` → Focus, Isolate, Back at cursor.
4. **G** / `HandleFocusUnderCursor` → dev shortcut for explicit focus.
5. **P** / `SetInteractionMode(Physical)` → primary action enables proxy drive.
6. **F** → scenario operation (unchanged).

### Planned (host project)

- `IDIVEDeviceControlRegistry` / `IDIVEProxyDrive` on device prefabs.
- `IA_DIVE_ContextMenu`, `IA_DIVE_FocusTarget`, mode actions in Content + PC `BindAction`.

## Editor

Context menu on selected actor: **DIVE Scan Device** — logs anchors, operations, catalog warnings.

Automation smoke tests: `DIVE.Operations.ValidationRules`, `DIVE.ContextMenu.BuiltInEntries`.

## Dependencies

DIVE **must not** link ACTS, GRIP, or MESS in **DIVERuntime**. No Enhanced Input in plugin modules.

Full contract: `Project_docs/DIVE_Plugin_Design.md`
