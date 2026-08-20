# DIVE — Device Interactive View Explorer

Modal detailed inspection sessions for complex devices in ATSEP / ERTOS-style simulators.

## Overview

DIVE provides:

- Orbit camera rig for device-focused sessions
- **Explicit focus** — context menu or `HandleFocusUnderCursor` (not the default meaning of primary action in Interact mode)
- **Interaction modes** — Interact (catalog primary / knobs) / Physical (GRIP grab of free bodies)
- **In-session context menu** — component **Bindings** (Focus/Isolate; Admin if Seed Admin Defaults) + **Action Catalog** (`UDIVEDeviceAction`). Standing-VR reuses the same action graph via a host action host — it does **not** open the camera session (see `Docs/Design_PhysicalControls_OneState_TwoInputs.md` §8–§10).
- **Primary action** — `PrimaryActionIndex` on binding; among matches LMB uses specificity (Name > PartId > Tag > Any), ties keep earlier Bindings entry (`IA_DIVE_PrimaryAction` → `HandlePrimaryAction*`); hover overlay under **DIVE | Pick | Hover**
- Optional **`UDIVEAnchorComponent`** for named camera viewpoints and semantic AOI
- **`IDIVEProxyDrive`** + **`IDIVEDeviceControlRegistry`** — monitor adapter for the interact verb (tier 2; host implements; zero in-plugin backends)
- Camera sensitivity on **`UDIVEInspectableComponent`** (`FDIVECameraSettings` under DIVE | Camera)
- Self-contained **DIVERuntime** (no ACTS / MESS); optional **GRIP** via sibling plugin **DIVEGRIPBridge** for Physical-mode pawn grab. DIVE `.uplugin` does **not** list GRIP — `DIVERuntimeDev` / the bridge detect it via UBT `ReadAvailablePlugins`. `DIVE.Dump*` implementation stays in `DIVEUncooked` and is linked from RuntimeDev only in **editor** targets.

**Input:** `UDIVEPlayerComponent` on the **locally controlled pawn** — BlueprintCallable `Handle*` methods (target for Enhanced Input). Session chrome and context menu live on the same component. Legacy dev component forwards `BindKey` only.

## Modules

| Module | Role |
|--------|------|
| **DIVECore** | Shared types, `FDIVEFocusTarget`, `UDIVEDeviceAction`, `IDIVEProxyDrive`, `IDIVEDeviceControlRegistry`, conventions |
| **DIVERuntime** | Subsystem, components, camera rig, context menu UI, **`UDIVEPlayerComponent`** (sole pawn ActorComponent) |
| **DIVERuntimeDev** | `UDIVELegacyKbmInputComponent`; registers `DIVE.Dump*` console (dump implementation in DIVEUncooked) |
| *(sibling plugin)* **DIVEGRIPBridge** | Enable separately: instanced **DIVE GRIP Physical Drive** on DIVE Player |

## Quick start

1. Enable plugin `DeviceInteractiveViewExplorer` in the project. For Physical + GRIP grab, also enable **`DIVEGRIPBridge`**.
2. Add `UDIVEInspectableComponent` to a device actor (meshes on the same actor).
3. Optional: `UDIVEAnchorComponent` for authored camera viewpoints / PartId.
4. On the locally controlled pawn: **`UDIVEPlayerComponent`**.
5. Optional PIE: **`UDIVELegacyKbmInputComponent`** (`DIVERuntimeDev`) — RMB context menu, MMB orbit, etc.
6. Open session: ACTS `OnActionExecuted` → `TryRequestSessionFromActionId` (`OpenDIVE` / `DIVE::kActionOpenDIVE`), or `RequestSession()` — see `Docs/QUICKSTART.md` §3.
7. Custom menu: **Action Catalog / Bindings** + action instances — **`Docs/QUICKSTART.md`** §1. Knobs/nuts: Rotary/Threaded drive on a tagged mesh (§2).
8. Physical controls: host implements `IDIVEDeviceControlRegistry` / `IDIVEProxyDrive` (see `Docs/Additional/DeviceInteractionModel.md` §6). For generic GRIP drag: enable **`DIVEGRIPBridge`**; on the pawn add **GRIP Rig** with slots Player + Dive (not Hand from Add Component). Player auto-creates the GRIP Physical Drive provider.

See `Docs/QUICKSTART.md`, `Docs/ARCHITECTURE.md`, and `Docs/Additional/DeviceInteractionModel.md`.

## Version

0.8-dev — object-based device actions (`UDIVEDeviceAction`); ActionSets/Roles/`IDIVEDeviceActionHandler` removed
