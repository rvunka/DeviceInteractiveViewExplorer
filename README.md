# DIVE — Device Interactive View Explorer

Modal detailed inspection sessions for complex devices in ATSEP / ERTOS-style simulators.

## Overview

DIVE provides:

- Orbit camera rig for device-focused sessions
- **Mesh-first focus** — LMB on mesh → orbit around bounds center
- **Anchor viewpoint** — LMB on session marker → camera at anchor transform
- Optional **`UDIVEAnchorComponent`** for semantic AOI, operations, and authored view points
- Camera sensitivity on **`UDIVEInspectableComponent`** (DIVE \| Camera)
- Self-contained runtime (no ACTS / GRIP / MESS dependencies)

**Input:** `DIVERuntime` — только session API. Для PIE: `UDIVELegacyKbmInputComponent` из **DIVERuntimeDev** (`BindKey`). Enhanced Input в плагине не используется.

## Modules

| Module | Role |
|--------|------|
| **DIVECore** | Shared types, `FDIVEFocusTarget`, conventions |
| **DIVERuntime** | Subsystem, components, camera rig, assets |
| **DIVERuntimeDev** | `UDIVELegacyKbmInputComponent` — debug `BindKey` only |

## Quick start

1. Enable plugin `DeviceInteractiveViewExplorer` in the project.
2. Add `UDIVEInspectableComponent` to a device actor (meshes on the same actor).
3. Optional: `UDIVEAnchorComponent` for operations / named view points (not required for mesh focus).
4. On pawn: add `UDIVELegacyKbmInputComponent` (dev) for session controls.
5. Open session via ACTS `OpenDIVE` or `RequestSession()` in game code.
6. Isolate: call `UDIVESessionSubsystem::ToggleIsolateFocused()` from UI or Blueprint (not bound by default).

See `Docs/QUICKSTART.md` and `Project_docs/DIVE_Plugin_Design.md`.

## Version

0.2.0 — focus-first (mesh pick, focus stack, optional anchors, explicit isolate)
