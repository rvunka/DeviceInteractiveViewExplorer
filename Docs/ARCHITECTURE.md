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
| **DIVE Input** | `DIVERuntime` | Orbit / zoom / select / back / exit / isolate / operation execute |
| **DIVE Operations UI** | `DIVERuntime` | Operation list + Hold progress during session |
| **Legacy KBM** | `DIVERuntimeDev` | `BindKey` only → forwards to **DIVE Input** |

Recommended pawn stack: **`UDIVEInputComponent`** + **`UDIVEOperationsUIComponent`** (+ optional **Legacy KBM** for PIE).

### Enhanced Input (game module)

| Input Action | Event | Call |
|--------------|-------|------|
| `IA_DIVE_Orbit` | Started / Completed | `HandleOrbitPressed` / `HandleOrbitReleased` |
| `IA_DIVE_Orbit` | Triggered (Axis2D) | `HandleOrbitDelta` — **do not combine** with Pressed/Released on the same action |
| `IA_DIVE_Zoom` | Triggered | `HandleZoomIn` / `HandleZoomOut` |
| `IA_DIVE_Select` | Started / Completed | `HandleSelectPressed` / `HandleSelectReleased` (proxy drive or pick/focus) |
| `IA_DIVE_ExecuteOperation` | Started / Completed | `HandleOperationExecutePressed` / `HandleOperationExecuteReleased` |
| `IA_DIVE_Back` | Started | `HandleNavigateBack` (camera / focus undo) |
| `IA_DIVE_Exit` | Started | `HandleExitSession` |

Legacy PIE defaults: **F** = execute operation, **I** = isolate, **LMB** = select / proxy drag / focus.

## Operations (v0.2)

- Anchors expose `OperationIds`; metadata lives in `UDIVEDeviceDefinitionAsset::OperationCatalog`.
- `EDIVEOperationInputMode`: **Press** (instant) or **Hold** (timer, ACTS-compatible UX).
- `ValidationRules` on device definition gate operations by `RequiredCompletedOperationIds`.
- `UDIVEOperationsUIComponent` shows the list; `RequestFocusedOperation` validates, dispatches `OnOperationRequested`, marks success in session state.

## Proxy drive (v0.4-dev)

Physical panel controls (sliders, doors, knobs) live on the **device** with constraints and game state. DIVE exposes an extension point in **DIVECore**:

- **`IDIVEProxyDrive`** — `CanProxyDrive`, `BeginProxyDrive`, `ApplyProxyDriveDelta`, `EndProxyDrive`
- **`FDIVEProxyDriveContext`** — screen position, optional focus target snapshot, hit component

**Select routing** (`UDIVEInputComponent`):

1. LMB down → `TryBeginProxyDriveAtScreenPosition` if hit actor/component implements `IDIVEProxyDrive`
2. else → `SelectAtScreenPosition` (mesh orbit / anchor viewpoint)
3. LMB up → `EndProxyDrive(true)`

No GRIP / ATSEP dependency in `DIVERuntime`. Game modules implement the interface on device controls. Full contract: **`Docs/DeviceInteractionModel.md`**.

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

1. `RequestSession()` → `TryBeginSession` → `BuildSemanticRegistry()` (anchors only).
2. Camera rig spawns at the player view, then blends to `InitialFocusId` / `DefaultStartFocusId` (anchor PartId or mesh component name) or device root.
3. Optional world dim applied; focus stack initialized with `DeviceRoot`.
4. LMB → proxy drive (if hit implements interface) **or** pick/focus; focused anchor → operations list; **F** → execute / hold.
5. Ctrl+Z → pop focus stack; at root → no-op.
6. Backspace → `EndSession`.

## Editor

Context menu on selected actor: **DIVE Scan Device** — logs anchors, operations, catalog warnings.

Automation smoke test: `ATSEP.DIVE.Operations.ValidationRules`.

## Dependencies

DIVE **must not** link ACTS, GRIP, or MESS in **DIVERuntime**. No Enhanced Input in plugin modules.

Full contract: `Project_docs/DIVE_Plugin_Design.md`
