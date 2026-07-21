# DIVEGRIPBridge

Runtime module connecting DIVE **Physical** mode to the GRIP Hand API (§7 bridge).

- **Depends on:** `DIVECore`, `DIVERuntime`; **conditionally** `GRIPCore` / `GRIPRuntime`
  when GraspRigidbodyInertialPhysics is enabled for the target (`DIVEGRIPBridge.Build.cs`).
- **Does not modify:** `DIVERuntime` (no GRIP link there).
- **Without GRIP:** the module still builds; `UDIVEGRIPBridgeComponent` is a no-op stub.
  Do **not** remove the module from `.uplugin` by hand.

## Pawn setup

Add to the **player pawn** (not device actors):

| Component | Role |
|-----------|------|
| `UGRIPHandComponent` named **`GRIP Hand`** (or `GRIP Hand Player`) | Gameplay grab |
| `UGRIPHandComponent` named **`GRIP Hand Dive`** | Physical drag (bridge default) |
| Matching `UGRIPHandAimComponent` (optional per slot) | Aim; bridge suppresses Dive Aim while dragging |
| `UDIVEGRIPBridgeComponent` | `Grip Hand Component` = `GRIP Hand Dive` |
| `UDIVEInputComponent` | Session input |

Single-Hand legacy: one Hand on the pawn + empty/unresolved Dive name → bridge uses that Hand once and logs a warning.

## Session visuals

While a DIVE session is active, the bridge toggles existing GRIP visuals
(`UGRIPHandComponent::bShowHandProxyVisuals` + `RefreshHandProxyVisuals`) — no GRIP
API changes. Toggle: `bHideGripHandProxiesDuringDiveSession` on the bridge.

## Grab start (no nudge)

`TryGrab` internally runs `InitGrabHoldDistanceFromView`, which snaps the PD target onto the
pawn view ray (lateral error vs mesh impact → a small push). After a successful grab the bridge
re-pins `HandWorldTransform` to the grab point and seeds hold distance from the same cursor
deproject used while dragging.

## Flow

```text
Physical mode — IA_DIVE_PrimaryAction (HandlePrimaryAction*)
  → DIVESessionSubsystem pick
  → device IDIVEProxyDrive (if registered) — priority 1
  → pawn IDIVEPawnPhysicalDrive (this bridge) — priority 2
  → GRIP TryGrabFromHit + SetHandWorldTransform
```

See GRIP [`INTEGRATION.md`](../../GraspRigidbodyInertialPhysics/Docs/INTEGRATION.md) for the Hand API contract.

## Migration (device-level bridge removed)

**Remove** `DIVE GRIP Proxy Drive` (`UDIVEGRIPProxyDriveComponent`) from device actors.

**Add** `DIVE GRIP Bridge` (`UDIVEGRIPBridgeComponent`) to the pawn.

Device actors keep:

- `UDIVEInspectableComponent`
- `IDIVEProxyDrive` / registry — **only** for meaningful DOF controls (sliders, hinges), not generic mesh grab.

Hanging bare `IDIVEProxyDrive` on an actor without implementing its methods does nothing.

## Manual smoke test

1. Enable admin context menu on the inspectable (`bEnableAdminContextMenuEntries`), start DIVE,
   enable Simulate Physics on a mesh.
2. `IA_DIVE_SetMode_Physical` (Legacy PIE: **Tab**).
3. Hold **primary action** on a simulating cube — it should move via GRIP (Legacy PIE: **LMB** drag).
4. While holding primary action, **hold R** (Legacy manual-rotate key) and move the mouse to rotate. Mouse switches to relative capture (no screen-edge limit). On R release the cursor returns to its pre-rotate position for drag.
5. `IA_DIVE_SetMode_Default` (Legacy PIE: **Tab**) — drag ends.
