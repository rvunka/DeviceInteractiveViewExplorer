# DIVEGRIPBridge

Optional runtime module connecting DIVE **Physical** mode to the GRIP Hand API.

- **Depends on:** `DIVECore`, `GRIPCore`, `GRIPRuntime`
- **Does not modify:** `DIVERuntime` (no GRIP link there)

## Pawn setup

Add to the **player pawn** (not device actors):

| Component | Role |
|-----------|------|
| `UGRIPHandComponent` | PD grab (`GrabPolicy = AllowSimulatingPhysics` for admin physics test) |
| `UGRIPHandAimComponent` | Optional; bridge suppresses aim during drag |
| `UDIVEGRIPBridgeComponent` | Implements `IDIVEPawnPhysicalDrive` |
| `UDIVEInputComponent` | Session input (already required for DIVE) |

## Flow

```text
Physical mode LMB
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

1. Start DIVE session on a device with simulating meshes (context menu → Simulate Physics).
2. **Left Alt** → Physical mode.
3. LMB drag on a simulating cube — it should move via GRIP.
4. While holding LMB, **hold R** and move the mouse to rotate. Mouse switches to relative capture (no screen-edge limit). On R release the cursor returns to its pre-rotate position for drag.
5. **Left Alt** → Default — drag ends.
