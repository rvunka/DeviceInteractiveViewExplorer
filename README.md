# DIVE — Device Interactive View Explorer

Modal detailed inspection sessions for complex devices in ATSEP / ERTOS-style simulators.

## Overview

DIVE provides:

- Orbit camera rig for device-focused sessions
- **Explicit focus** — context menu or `HandleFocusUnderCursor` (not default LMB)
- **Interaction modes** — Default (inspect) / Physical (proxy drive on device controls)
- **In-session context menu** — Focus, Isolate on pick; device rows via **`PickContextMenuActions`** catalog + optional `AppendContextMenuEntries`; style via `MenuStyle` on `UDIVEContextMenuUIComponent`
- Optional **`UDIVEAnchorComponent`** for named camera viewpoints and semantic AOI
- **`IDIVEProxyDrive`** + **`IDIVEDeviceControlRegistry`** for monitor-side physical controls (host implements)
- Camera sensitivity on **`UDIVEInspectableComponent`** (DIVE | Camera)
- Self-contained **DIVERuntime** (no ACTS / MESS); optional **GRIP** via **DIVEGRIPBridge** for Physical-mode pawn grab (§7)

**Input:** `UDIVEInputComponent` on pawn — BlueprintCallable `Handle*` methods (target for Enhanced Input). Legacy dev component forwards `BindKey` only.

## Modules

| Module | Role |
|--------|------|
| **DIVECore** | Shared types, `FDIVEFocusTarget`, `IDIVEProxyDrive`, `IDIVEDeviceControlRegistry`, conventions |
| **DIVERuntime** | Subsystem, components, camera rig, context menu UI, **`UDIVEInputComponent`** |
| **DIVERuntimeDev** | `UDIVELegacyKbmInputComponent` — BindKey → **DIVE Input** |
| **DIVEGRIPBridge** | `UDIVEGRIPBridgeComponent` — pawn physical drive via GRIP (optional) |

## Quick start

1. Enable plugin `DeviceInteractiveViewExplorer` in the project.
2. Add `UDIVEInspectableComponent` to a device actor (meshes on the same actor).
3. Optional: `UDIVEAnchorComponent` for authored camera viewpoints / PartId.
4. On pawn: **`UDIVEInputComponent`** + **`UDIVEContextMenuUIComponent`** (Input auto-finds UI by class).
5. Optional PIE: **`UDIVELegacyKbmInputComponent`** (`DIVERuntimeDev`) — RMB context menu, MMB orbit, etc.
6. Open session via ACTS `OpenDIVE` or `RequestSession()` in game code.
7. Device-specific actions: fill **`PickContextMenuActions`** on inspectable; override **`ExecuteContextMenuAction`** / optional **`AppendContextMenuEntries`** in Blueprint.
8. Physical controls: host implements `IDIVEDeviceControlRegistry` / `IDIVEProxyDrive` (see `DeviceInteractionModel.md` §6).

See `Docs/QUICKSTART.md`, `Docs/ARCHITECTURE.md`, and `Docs/DeviceInteractionModel.md`.

## Version

0.7-dev — remove operations/checklist stack; context menu + direct manipulation model
