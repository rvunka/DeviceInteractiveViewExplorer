# DIVE — Device Interactive View Explorer

Modal detailed inspection sessions for complex devices in ATSEP / ERTOS-style simulators.

## Overview

DIVE provides:

- Orbit camera rig for device-focused sessions
- **Explicit focus** — context menu or `HandleFocusUnderCursor` (not default LMB)
- **Interaction modes** — Default (inspect) / Physical (proxy drive on device controls)
- **In-session context menu** — Focus, Isolate, Back at cursor
- Optional **`UDIVEAnchorComponent`** for semantic AOI, operations, and authored view points
- **`IDIVEProxyDrive`** + **`IDIVEDeviceControlRegistry`** for monitor-side physical controls (game implements)
- Camera sensitivity on **`UDIVEInspectableComponent`** (DIVE | Camera)
- Self-contained runtime (no ACTS / GRIP / MESS dependencies)

**Input:** `UDIVEInputComponent` on pawn — BlueprintCallable `Handle*` methods (target for Enhanced Input). Legacy dev component forwards `BindKey` only.

## Modules

| Module | Role |
|--------|------|
| **DIVECore** | Shared types, `FDIVEFocusTarget`, `IDIVEProxyDrive`, `IDIVEDeviceControlRegistry`, conventions |
| **DIVERuntime** | Subsystem, components, camera rig, context menu UI, **`UDIVEInputComponent`** |
| **DIVERuntimeDev** | `UDIVELegacyKbmInputComponent` — BindKey → **DIVE Input** |

## Quick start

1. Enable plugin `DeviceInteractiveViewExplorer` in the project.
2. Add `UDIVEInspectableComponent` to a device actor (meshes on the same actor).
3. Optional: `UDIVEAnchorComponent` for operations / named view points.
4. On pawn: **`UDIVEInputComponent`** + **`UDIVEOperationsUIComponent`** + **`UDIVEContextMenuUIComponent`**.
5. Optional PIE: **`UDIVELegacyKbmInputComponent`** (`DIVERuntimeDev`) — RMB context menu, MMB orbit, etc.
6. Open session via ACTS `OpenDIVE` or `RequestSession()` in game code.
7. Physical controls: host project implements `IDIVEDeviceControlRegistry` / `IDIVEProxyDrive` on devices (see `DeviceInteractionModel.md` §6).

See `Docs/QUICKSTART.md`, `Docs/DeviceInteractionModel.md`, and `Project_docs/DIVE_Plugin_Design.md`.

## Version

0.6-dev — in-session context menu; interaction mode policy; `IDIVEDeviceControlRegistry` hook (host implements)
