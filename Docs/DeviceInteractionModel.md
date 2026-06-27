# DIVE and physical device controls

> **Audience:** device authors, game integration (ATSEP), VR planning.  
> **Status:** architecture contract (v0.4-dev).  
> **Related:** `ARCHITECTURE.md`, `QUICKSTART.md`, GRIP `Docs/ARCHITECTURE.md`, `Project_docs/Plugin_Input_Architecture.md`.

---

## 1. Principle: controls first, DIVE second

**Interactive parts (sliders, doors, knobs, switches) are not built for DIVE.**

They are **device prefabs**: simulating meshes + Chaos constraints + game-layer state (sound, MESS, training logic). DIVE is added **later** as a **flat-monitor adapter** so the player can reach small controls without walking up to the panel.

| Layer | Owns |
|-------|------|
| **Device / game** | Constraint setup, DOF limits, snap, sounds, simulation state, reuse across devices |
| **GRIP (VR / world hands)** | Hand aim, grab PD forces, two-hand manipulation at the device |
| **DIVE (monitor session)** | Close camera, pick, cursor/proxy drive into the **same** device controls |

DIVE must **not** become the source of truth for knob position, door angle, or slider value.

---

## 2. Two input paths, one device state

```text
                    ┌──────────────────────────────┐
                    │  Device control (game layer)  │
                    │  - constraint / DOF           │
                    │  - normalized value / angle   │
                    │  - OnMoved → sound, MESS      │
                    └──────────────▲───────────────┘
                                   │
              ┌────────────────────┴────────────────────┐
              │                                         │
     VR / world: GRIP (L/R hands)              Monitor: DIVE session
     PD grab @ grab point                      ray + mouse drag / click
     ManualOnly aim from controllers           «virtual finger» / proxy drive
```

### VR (target)

- Two hands, same model as **GRIP**: aim → grab → PD forces on **grabbable** primitives.
- **Constraints** (hinge, prismatic, limits) live on the **device**, not in GRIP or DIVE.
- GRIP moves the body; the constraint defines allowed motion; the device component reads result and fires events (sound, grid, scenario).

### Flat monitor (target)

- **DIVE** brings the camera close and provides precise **screen-space input**.
- Input is translated into the **same DOF** the VR hand would have moved (not a separate DIVE-only hinge or menu action).
- No requirement to stand next to the panel; no second physics rig inside DIVE.

---

## 3. What belongs where

### Device / game layer (author here)

| Concern | Where |
|---------|--------|
| Hinge / slide / dial **constraints** | Device Blueprint / C++ |
| Grabbable marker (`UGRIPGrabbableComponent` / `IGRIPGrabbable`) | On moving part |
| Sounds, haptics, MESS hooks | Device control component |
| Reuse on another device | Same control prefab / component |

Recommended **game interface** — now exported from **DIVECore** as `IDIVEProxyDrive`:

```text
IDIVEProxyDrive (DIVECore)
  CanProxyDrive() → bool
  BeginProxyDrive(FDIVEProxyDriveContext)  // ScreenPosition, FocusTarget, HitComponent
  ApplyProxyDriveDelta(ScreenDelta)
  EndProxyDrive(bCommit)
```

- **GRIP path:** hand grab applies forces; constraint + component update state.
- **DIVE path:** session pick calls `BeginProxyDrive` / `ApplyProxyDriveDelta` on the hit control (via `IDIVEProxyDrive`).
- **Sounds:** only inside the device component when value/angle actually changes.

### DIVE plugin (stays thin)

| In scope | Out of scope |
|----------|----------------|
| Session, camera rig, focus stack | Authoring constraints on devices |
| Ray pick from DIVE camera | GRIP hand physics |
| `IDIVEProxyDrive` hook (routing in session subsystem) | Implementations on devices (game module) |
| `Handle*` input API for Enhanced Input | MESS / electrical solver |
| Optional semantic **operations** (inspect, demount, scenario steps) | Defining every physical knob as an OperationId |

### Semantic operations vs physical controls

| Interaction | Mechanism |
|-------------|-----------|
| Door, slider, knob, switch | **Physical control** → proxy drive or GRIP (same state) |
| «Read label», «Run checklist step», «Demount module» | **DIVE operation** → `OnOperationRequested` delegate (game decides) |

Do not model every physical handle as a Press/Hold **operation** unless it is truly a discrete scenario action with no continuous DOF.

---

## 4. DIVE v0.4-dev (current)

v0.4-dev removes the **DIVE-local kinematic hinge** on `UDIVEAnchorComponent`. Anchors remain for **viewpoint + PartId + scenario OperationIds**.

| Removed in 0.4-dev | Replacement |
|--------------------|-------------|
| `EDIVEManipulationKind::Hinge` | Device constraint + `IDIVEProxyDrive` on control |
| `DIVEManipulation` utils | Session routing: `TryBeginProxyDriveAtScreenPosition` |
| `ATSEP.DIVE.Manipulation.HingeSnap` test | `ATSEP.DIVE.Operations.ValidationRules` |

| Current behaviour | Notes |
|-------------------|-------|
| Pick mesh → orbit | Unchanged |
| Pick anchor marker → viewpoint | Unchanged |
| LMB on `IDIVEProxyDrive` hit → drag | Game implements interface; no default impl in plugin |
| `OperationIds` on anchor | Scenario steps only (inspect, demount) |

Roadmap: ATSEP implements `IDIVEProxyDrive` on device controls; optional GRIP bridge for virtual-hand grab.

---

## 5. Reusing GRIP as the grab engine for DIVE

### What GRIP already provides

- Hand aim (trace / external provider / ManualOnly)
- Grab lifecycle (press, release, PD pull at grab point)
- Manual rotate while grabbed
- Grabbable discovery (`UGRIPGrabbableComponent`, `IGRIPGrabbable`)

GRIP is a **hand / grab engine**, not a **slider constraint engine**. Constraints remain on the device. GRIP applies forces; constraints limit motion; the device component owns state and feedback.

### Can DIVE call GRIP directly?

**Technically yes, architecturally no** — inside `DIVERuntime`:

- Plugin rule: **no module dependency** on GRIP (same as ACTS / MESS). `DIVERuntime.Build.cs` must stay self-contained.
- Shipping projects without VR should not pull GRIP because DIVE is enabled.

### Recommended pattern: compose in the game module

```text
DIVERuntime          GRIPRuntime          ATSEP_Fundamental (game)
(self-contained)     (self-contained)     (composition only)
     │                    │                        │
     │  pick / session    │  hand / grab           │
     └────────────────────┴────────────────────────┘
                          │
              ATSEP bridge / pawn setup
              - DIVE camera → aim provider for a «session hand»
              - Or: pick → IDeviceProxyDrive on hit component
              - VR: two UGRIPHandComponent, no DIVE grab
```

**Option A — Proxy drive (preferred for sliders / hinged panels in DIVE)**

1. Device implements `IDIVEProxyDrive` (game code on control actor/component).
2. DIVE select + drag → `UDIVESessionSubsystem` routes to active proxy drive.
3. GRIP not involved in the monitor session; VR still uses GRIP on the same mesh.

**Option B — Virtual GRIP hand during DIVE (reuse grab PD)**

1. Pawn keeps `UGRIPHandComponent` + `UGRIPHandAimComponent`.
2. While DIVE active: aim = ray from **DIVE camera** (ManualOnly / external provider); grab button = DIVE select hold.
3. GRIP pulls the **same** constrained body VR would grab.
4. Wiring lives in **ATSEP** (or a small optional `DIVEGRIPBridge` module), not in `DIVERuntime`.

Use **B** when the control is truly «grab and pull»; use **A** when you want deterministic kinematic drive on monitor (training sim, snap ticks, no physics jitter).

**Option C — Optional adapter module (no hard dependency in core)**

If many projects need B:

```text
DIVECore / DIVERuntime     — unchanged
GRIPRuntime                — unchanged
DIVEGRIPAdapter (optional) — depends on DIVERuntime + GRIPRuntime; game opt-in
```

Only ATSEP (or titles that need it) enable the adapter plugin. DIVE repo stays usable without GRIP.

### Dependency summary

| Approach | DIVE → GRIP link | When |
|----------|------------------|------|
| None (default) | No | DIVE-only titles, tests |
| Game composition | ATSEP wires pick → proxy or hand | **ATSEP (recommended)** |
| Optional adapter module | Soft, opt-in plugin | Multiple products share same bridge |

**MUST NOT:** `#include` GRIP headers or link `GRIPRuntime` from `DIVERuntime`.

---

## 6. Session interaction with GRIP in the world

While `UDIVESessionSubsystem` is active, ATSEP PlayerController **blocks** normal ACTS / GRIP gameplay input (early return). That matches «modal inspect» on monitor.

VR planning:

| Mode | GRIP | DIVE |
|------|------|------|
| Standing at device in VR | Both hands, full GRIP | Session optional / off |
| «Precision panel» in VR | Could use laser + proxy drive | Same session API, different IMC |
| Flat monitor | Blocked | Camera + proxy or virtual hand |

Exact VR policy (DIVE session in HMD or not) is a **game** decision; DIVE exposes session + pick only.

---

## 7. Authoring checklist (device team)

1. Build control prefab: mesh + constraint + state component + sounds.
2. Mark moving part grabbable for GRIP (VR).
3. Implement proxy drive interface (game) for monitor / DIVE.
4. Add `UDIVEInspectableComponent` on device root for session entry only.
5. Optional anchors for **viewpoints** and **non-physical** operations — not as a substitute for step 3.

---

## 8. Open work (not in DIVE core)

| Item | Owner |
|------|--------|
| `IDIVEProxyDrive` implementations on device controls | ATSEP |
| Optional virtual GRIP hand from DIVE camera | ATSEP or `DIVEGRIPAdapter` |
| VR two-hand + OpenXR | GRIPVR / game |

---

## 9. One-line summary

**Sliders and doors are physical device controls (constraints + GRIP in VR); DIVE is the flat-screen adapter that drives the same controls — composed in the game layer, without a hard GRIP dependency inside the DIVE plugin.**
