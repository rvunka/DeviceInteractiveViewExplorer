# DIVE and physical device controls

> **Audience:** device authors, game integration (ATSEP), VR planning.  
> **Status:** architecture contract (v0.8).  
> **Related:** `ARCHITECTURE.md`, `QUICKSTART.md`, GRIP `Docs/ARCHITECTURE.md`, `../../../Docs/Plugin_Architecture_Principles.md`.

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

Recommended **control wiring** (game / device layer):

1. **Registry (primary)** — at session start or on prefab: map pickable primitive (or tag / component name) → control component.
2. **`IDIVEProxyDrive` (optional C++ adapter)** — thin wrapper on the same control component when registry resolves to an interface.

```text
IDIVEProxyDrive (DIVECore) — backend for Physical mode only
  CanProxyDrive() → bool
  BeginProxyDrive(FDIVEProxyDriveContext)
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
| Context menu + Action Catalog / Bindings | Defining menu operations and which parts get them (not physical knobs) |

### Direct manipulation vs physical controls

| Interaction | Mechanism |
|-------------|-----------|
| Door, slider, knob, switch | **Physical** mode → proxy drive or GRIP (same device state) |
| Read label, use button, demount module | **Context menu** on pick → **Action Catalog / Bindings** + `UDIVEDeviceAction` |
| Cable, grab | GRIP + MESS in host project |

---

## 4. Session UX: camera chrome + interaction mode (target)

DIVE is a **camera-centric modal session**, not a Blender-style mode switch where orbit disappears. **Orbit, zoom, focus stack, and exit work in every interaction mode.** Modes change **session policy** (how semantic input actions are routed), not whether the camera exists.

### 4.1. Persistent session chrome (always on)

Available regardless of `EDIVESessionInteractionMode`:

| Capability | Semantic API (plugin) | Typical `IA_*` (ATSEP Content) |
|------------|----------------------|--------------------------------|
| Orbit camera | `HandleOrbitPressed/Released`, `HandleOrbitDelta` | `IA_DIVE_Orbit` |
| Zoom | `HandleZoomIn`, `HandleZoomOut` | `IA_DIVE_Zoom` |
| Undo focus step | `HandleNavigateBack` | `IA_DIVE_Back` |
| Exit session | `HandleExitSession` | `IA_DIVE_Exit` |
| Context menu at cursor | `HandleContextMenuRequested` | `IA_DIVE_ContextMenu` |
| Focus pick under cursor | `HandleFocusUnderCursor` | `IA_DIVE_FocusTarget` |
| Mesh isolate | Context menu or `HandleToggleIsolate` | custom / UI |

**Focus / «подъехать»** is an **explicit** action (context menu item or `IA_DIVE_FocusTarget`), not the default meaning of every click. That avoids fighting physical controls.

**Physical keys live only in IMC** (`Plugin_Architecture_Principles.md`). DIVE **must not** encode `EKeys` or «LMB tool» enums in its public contract.

### 4.2. Interaction mode = session policy (not «Navigate mode»)

```text
EDIVESessionInteractionMode  (DIVECore)
  Default   — inspect: primary action → binding PrimaryActionIndex; hover overlay; explicit focus via HandleFocusUnderCursor / context menu
  Physical  — primary action → continuous DOF (door, slider, knob)
  Logical   — planned; not yet in the enum (v1 roadmap)
```

Mode is **not** tied to a single mouse button. It is **policy** for routing **multiple** semantic actions, for example:

| Policy area | Example behaviour per mode |
|-------------|----------------------------|
| `HandlePrimaryAction*` | Physical → grab/drive; Default → catalog action / hover |
| Context menu contents | Component Bindings + Action Catalog |
| Hit highlight / filter | Physical may prefer grabbable primitives |
| HUD / cursor | Show active mode label |

Mode does **not** disable orbit, context menu, or focus stack.

Switch mode via `SetInteractionMode` from ATSEP (`IA_DIVE_SetMode_*` or cycle action in IMC).

### 4.3. DIVE context menu (not ACTS)

**ACTS** = world interaction before session («Детальный осмотр»).  
**DIVE context menu** = in-session, ray under cursor, **not** the ACTS radial/world menu.

Flow:

1. `HandleContextMenuRequested` → pick at screen position → build entry list.
2. **Default entries**: component **Bindings** (Focus, Isolate; Admin unless removed / Shipping).
3. **Device extensions**: **Action Catalog** (and/or extra Bindings) — `UDIVEDeviceAction` instances.
4. Custom row click → **`UDIVEDeviceAction::Execute`** / continuous **`BeginInteraction`**. See **`QUICKSTART.md`** §1.
5. Display state (enabled / checked / visible / label) comes from **`GetDisplayState`** on the action — no `Is_*` reflection.
6. Section **Header** (optional) is drawn above the separator between menu sections.

Remapping «open menu» to RMB, Q, or gamepad — **IMC only**.

### 4.4. Primary action (semantic)

Target API: **`HandlePrimaryActionPressed/Released`** (project maps **`IA_DIVE_PrimaryAction`**).

| Mode | Primary action (press/hold/release) |
|------|-------------------------------------|
| **Default** | Binding `PrimaryActionIndex` when configured (Name > PartId > Tag > Any; ties = earlier Bindings entry); hover overlay on pickable mesh |
| **Physical** | Begin/update/end drive on hit control (`IDIVEProxyDrive` via `InternalProxyDriveAction`, or pawn bridge via ATSEP) |
| **Logical** | Planned (v1 roadmap); not yet in the enum |

Drag delta while held is driven by the same action lifecycle + tick/Triggered axis if needed — still **semantic**, not «mouse moved».

### 4.5. Layer diagram (target)

```text
  IMC (ATSEP)          IA_DIVE_*  ──►  UDIVEInputComponent::Handle*
                                              │
                    ┌─────────────────────────┴─────────────────────────┐
                    │     EDIVESessionInteractionMode (session policy)     │
                    └─────────────────────────┬─────────────────────────┘
                                              │
         ┌────────────────────────────────────┼────────────────────────────┐
         │ chrome (always)                    │ mode-dependent              │
         │ orbit · zoom · back · exit         │ primary action · menu extras│
         │ context menu · focus under cursor  │ physical / logical backends │
         └────────────────────────────────────┴────────────────────────────┘
                                              │
                              device controls · constraints · GRIP (VR)
```

---

## 5. Implementation status (v0.8)

v0.4-dev removed the DIVE-local kinematic hinge. v0.7 removed the checklist **operations** stack (F-key, OperationIds, Operations UI). v0.8 replaced ActionSets/Roles/`IDIVEDeviceActionHandler` with object actions.

| Current behaviour | Notes |
|-------------------|-------|
| `EDIVESessionInteractionMode` + `SetInteractionMode` | Default / Physical |
| `HandlePrimaryAction*` | Default → binding `PrimaryActionIndex` (when set); hover overlay; Physical → proxy drive |
| Context menu + widget | Focus, Isolate (Bindings); Admin unless removed (hidden in Shipping); custom from Catalog |
| Action Catalog / Bindings + `UDIVEDeviceAction` | Object actions; continuous via shared interaction slot |
| Hover overlay | `DIVE|Pick|Hover` on inspectable; exclusions via `PickInteractionExclusions` |
| `IDIVEDeviceControlRegistry` / `IDIVEProxyDrive` | Host implements on devices |
| Anchor | Viewpoint + PartId only |

### Host project (remaining)

| Item | Owner |
|------|--------|
| `IA_DIVE_*` Content + `IMC_DIVE` | Host Content |
| Registry / proxy drive on prefabs | Host / devices |
| Context menu rows per pick | Action Catalog / Bindings + `UDIVEDeviceAction` |
| GRIP / MESS for cables | Host project |

---

## 6. `IDIVEProxyDrive` — backend, not UX

`IDIVEProxyDrive` (DIVECore) is **one implementation path** for **Physical** mode, not the main interaction model:

```text
IDIVEProxyDrive (DIVECore)
  CanProxyDrive() → bool
  BeginProxyDrive(FDIVEProxyDriveContext)
  ApplyProxyDriveDelta(ScreenDelta)
  EndProxyDrive(bCommit)
```

Prefer **device registry** at session start (primitive / tag → control component) when Blueprint authoring is easier than `Implement Interface` on every part.

- **GRIP path (VR):** hand grab → constraint → device state.
- **DIVE Physical path:** `HandlePrimaryAction*` → registry or `IDIVEProxyDrive` → same device state.
- **Sounds / MESS:** only in device component when value changes.

**Do not** hang the interface on `UStaticMeshComponent` — put it on a **control component** on the actor and register which primitives it owns.

---

## 7. Reusing GRIP as the grab engine for DIVE

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

### Recommended pattern: sibling plugin `DIVEGRIPBridge`

```text
DIVERuntime          GRIPRuntime          DIVEGRIPBridge (sibling plugin)
(self-contained)     (self-contained)     pawn component only
     │                    │                        │
     │  pick / session    │  hand / grab           │
     └────────────────────┴────────────────────────┘
                          │
              Pawn setup (game or ATSEP)
              - UGRIPHandComponent + UGRIPHandAimComponent
              - UDIVEGRIPBridgeComponent (generic Physical drag)
              - UDIVEInputComponent
              - VR: two UGRIPHandComponent, no DIVE grab
```

**Option A — Device proxy drive (sliders / hinged panels)**

1. Device registers controls (registry) or implements `IDIVEProxyDrive` on control components.
2. In **Physical** mode, session routes pick → device `IDIVEProxyDrive` **first** (deterministic DOF).
3. GRIP not involved on monitor for these controls; VR still uses GRIP on the same mesh.

**Option B — Pawn GRIP bridge (generic simulating-mesh drag)**

1. Pawn keeps **two** grip instances when DIVE must not steal the player hand:
   - `GRIP Hand` / `GRIP Hand Player` — gameplay grab
   - `GRIP Hand Dive` — Physical drag (+ matching Aim if needed)
   - `UDIVEGRIPBridgeComponent` with **GRIP Hand Component** = `GRIP Hand Dive`
2. Single-Hand fallback: leave Dive name empty only when there is **one** Hand on the pawn (legacy hijack + warning).
3. In **Physical** mode, if no device proxy handles the pick, session falls back to `IDIVEPawnPhysicalDrive` on the pawn.
4. Bridge calls GRIP Hand API (`TryGrabFromHit`, `SetHandWorldTransform`, aim suppress **on the Dive Aim**) — see GRIP `INTEGRATION.md` / multi-hand slots.
5. Cursor-follow drag: bridge deprojects screen position each tick onto the grab-depth ray. **Hold R** while dragging to manual-rotate.
6. **Do not** add bridge components to device actors; one bridge per pawn.

See GRIP [`ARCHITECTURE.md`](../../GraspRigidbodyInertialPhysics/Docs/ARCHITECTURE.md) § Grip instances and [`MultiInstance_Anchor_Architecture.md`](../../GraspRigidbodyInertialPhysics/Docs/MultiInstance_Anchor_Architecture.md).

Use **B** for «grab and pull» on simulating bodies (admin **Simulate Physics** + Physical mode). Use **A** for deterministic kinematic drive (training sim, snap ticks, no physics jitter).

**Routing priority in Physical mode**

```text
Pick hit → IDIVEProxyDrive (device) → IDIVEPawnPhysicalDrive (pawn bridge) → fail (verbose log)
```

### Dependency summary

| Approach | DIVE → GRIP link | When |
|----------|------------------|------|
| None (default) | No | DIVE-only titles, tests |
| `DIVEGRIPBridge` sibling plugin | Enable separately; `DIVERuntime` stays GRIP-free | Generic Physical drag with GRIP |
| Device `IDIVEProxyDrive` | No GRIP on monitor | Sliders, hinges, custom DOF |

**MUST NOT:** `#include` GRIP headers or link `GRIPRuntime` from `DIVERuntime`.

---

## 8. Session interaction with GRIP in the world

While `UDIVESessionSubsystem` is active, ATSEP PlayerController **blocks** normal ACTS / GRIP gameplay input (early return). That matches «modal inspect» on monitor.

VR planning:

| Mode | GRIP | DIVE |
|------|------|------|
| Standing at device in VR | Both hands, full GRIP | Session optional / off |
| «Precision panel» in VR | Could use laser + proxy drive | Same session API, different IMC |
| Flat monitor | Blocked | Camera + proxy or virtual hand |

Exact VR policy (DIVE session in HMD or not) is a **game** decision; DIVE exposes session + pick only.

---

## 9. Authoring checklist (device team)

1. Build control prefab: mesh + constraint + state component + sounds.
2. Mark moving part grabbable for GRIP (VR).
3. Register physical controls (registry or `IDIVEProxyDrive`) for **Physical** mode — not on raw mesh components.
4. Add `UDIVEInspectableComponent` on device root for session entry only.
5. Optional anchors for **viewpoints** and **non-physical** operations — not as a substitute for step 3.

---

## 10. Open work

| Item | Owner |
|------|--------|
| `IA_DIVE_*` + IMC for DIVE session | Host Content |
| Control registry + drive backends on device prefabs | Host / devices |
| Optional virtual GRIP hand | Sibling plugin `DIVEGRIPBridge` |
| VR two-hand + OpenXR | GRIPVR / game |

DIVE object-action model (v0.8): `UDIVEDeviceAction`, Catalog + component Bindings, continuous slot — done. Sample Unscrew module removed; author BP via Content Browser → DIVE factories. See `Audit_DeviceActions_Architecture.md` §12.

---

## 11. One-line summary

**Sliders and doors are device controls (constraints + GRIP in VR). DIVE is the flat-screen adapter: persistent camera chrome, explicit focus via menu, interaction **modes** as session policy, Enhanced Input semantics in ATSEP — composed in the game layer without a hard GRIP dependency in the plugin.**
