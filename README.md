# DIVE — Device Interactive View Explorer

Modal detailed inspection sessions for complex devices in ATSEP / ERTOS-style simulators.

## Overview

DIVE provides:

- Orbit camera rig for device-focused sessions
- **Explicit focus** — context menu or `HandleFocusUnderCursor` (not the default meaning of primary action in Default mode)
- **Interaction modes** — Default (inspect) / Physical (proxy drive on device controls)
- **In-session context menu** — component **Bindings** (Focus/Isolate/Admin) + **Action Catalog** (`UDIVEDeviceAction`)
- **Primary action** — `PrimaryActionIndex` on binding; among matches LMB uses specificity (Name > PartId > Tag > Any), ties keep earlier Bindings entry (`IA_DIVE_PrimaryAction` → `HandlePrimaryAction*`); hover overlay under **DIVE | Pick | Hover**
- Optional **`UDIVEAnchorComponent`** for named camera viewpoints and semantic AOI
- **`IDIVEProxyDrive`** + **`IDIVEDeviceControlRegistry`** for monitor-side physical controls (host implements)
- Camera sensitivity on **`UDIVEInspectableComponent`** (DIVE | Camera)
- Self-contained **DIVERuntime** (no ACTS / MESS); optional **GRIP** via sibling plugin **DIVEGRIPBridge** for Physical-mode pawn grab (§7)

**Input:** `UDIVEInputComponent` on the **player character** — BlueprintCallable `Handle*` methods (target for Enhanced Input). Legacy dev component forwards `BindKey` only.

## Modules

| Module | Role |
|--------|------|
| **DIVECore** | Shared types, `FDIVEFocusTarget`, `UDIVEDeviceAction`, `IDIVEProxyDrive`, `IDIVEDeviceControlRegistry`, conventions |
| **DIVERuntime** | Subsystem, components, camera rig, context menu UI, **`UDIVEInputComponent`** |
| **DIVERuntimeDev** | `UDIVELegacyKbmInputComponent`, **`DIVE.DumpDevice` / `DIVE.DumpAll`** diagnostics |
| *(sibling plugin)* **DIVEGRIPBridge** | Enable separately: `UDIVEGRIPBridgeComponent` — pawn physical drive via GRIP |

## Quick start

1. Enable plugin `DeviceInteractiveViewExplorer` in the project. For Physical + GRIP grab, also enable **`DIVEGRIPBridge`**.
2. Add `UDIVEInspectableComponent` to a device actor (meshes on the same actor).
3. Optional: `UDIVEAnchorComponent` for authored camera viewpoints / PartId.
4. On pawn: **`UDIVEInputComponent`** + **`UDIVEContextMenuUIComponent`** (Input auto-finds UI by class).
5. Optional PIE: **`UDIVELegacyKbmInputComponent`** (`DIVERuntimeDev`) — RMB context menu, MMB orbit, etc.
6. Open session: ACTS `OnActionExecuted` → `TryRequestSessionFromActionId` (`OpenDIVE` / `DIVE::kActionOpenDIVE`), or `RequestSession()` — see `Docs/QUICKSTART.md` §3.
7. Custom menu: **Action Catalog / Bindings** + action instances — **`Docs/QUICKSTART.md`** §1.
8. Physical controls: host implements `IDIVEDeviceControlRegistry` / `IDIVEProxyDrive` (see `DeviceInteractionModel.md` §6). For generic GRIP drag: enable **`DIVEGRIPBridge`** and add `UDIVEGRIPBridgeComponent` on the pawn.

See `Docs/QUICKSTART.md`, `Docs/ARCHITECTURE.md`, and `Docs/DeviceInteractionModel.md`.

## Version

0.8-dev — object-based device actions (`UDIVEDeviceAction`); ActionSets/Roles/`IDIVEDeviceActionHandler` removed
