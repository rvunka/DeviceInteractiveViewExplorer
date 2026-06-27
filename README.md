# DIVE — Device Interactive View Explorer

Modal detailed inspection sessions for complex devices in ATSEP / ERTOS-style simulators.

## Overview

DIVE provides:

- Orbit camera rig for device-focused sessions
- **Mesh-first focus** — LMB on mesh → orbit around bounds center
- **Anchor viewpoint** — LMB on session marker → camera at anchor transform
- Optional **`UDIVEAnchorComponent`** for semantic AOI, operations, and authored view points
- **`IDIVEProxyDrive`** extension point for monitor-side physical controls (game implements)
- Camera sensitivity on **`UDIVEInspectableComponent`** (DIVE \| Camera)
- Self-contained runtime (no ACTS / GRIP / MESS dependencies)

**Input:** `UDIVEInputComponent` on pawn — BlueprintCallable `Handle*` methods (target for Enhanced Input). Legacy dev component forwards `BindKey` only.

## Modules

| Module | Role |
|--------|------|
| **DIVECore** | Shared types, `FDIVEFocusTarget`, `IDIVEProxyDrive`, conventions |
| **DIVERuntime** | Subsystem, components, camera rig, assets, **`UDIVEInputComponent`** |
| **DIVERuntimeDev** | `UDIVELegacyKbmInputComponent` — BindKey → **DIVE Input** |

## Quick start

1. Enable plugin `DeviceInteractiveViewExplorer` in the project.
2. Add `UDIVEInspectableComponent` to a device actor (meshes on the same actor).
3. Optional: `UDIVEAnchorComponent` for operations / named view points (not required for mesh focus).
4. On pawn: add **`UDIVEInputComponent`** (required for session controls).
5. Optional PIE: add **`UDIVELegacyKbmInputComponent`** (`DIVERuntimeDev`) — forwards keys to **DIVE Input**.
6. Open session via ACTS `OpenDIVE` or `RequestSession()` in game code.
7. Isolate: call `UDIVESessionSubsystem::ToggleIsolateFocused()` from UI or Blueprint (not bound by default).

See `Docs/QUICKSTART.md`, `Docs/DeviceInteractionModel.md`, and `Project_docs/DIVE_Plugin_Design.md`.

## Version

0.4.0-dev — anchor hinge manipulator removed; `IDIVEProxyDrive` hook for monitor-side physical controls
