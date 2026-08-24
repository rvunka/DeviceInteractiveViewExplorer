# DIVE and physical device controls

> **Audience:** device authors, game integration (ATSEP), VR planning.  
> **Status:** architecture contract (v0.8). Two-tier authoring: kinematic continuous actions **now**; device control components when triggered. Two verbs: **interact** (panel — monitor session + standing-VR action host) and **grab** (free bodies — GRIP). Standing-VR does **not** open the DIVE camera.  
> **Markers:** **Реализовано** — код в DIVE; **Контракт-цель** — API есть, имплементоров в дереве 0; **Обязанность хоста** — IMC / PC / префабы.  
> **Related:** `ARCHITECTURE.md`, `QUICKSTART.md`, GRIP `Docs/ARCHITECTURE.md`. Two verbs (interact vs grab): [`../Design_PhysicalControls_OneState_TwoInputs.md`](../Design_PhysicalControls_OneState_TwoInputs.md).

---

## 1. Principle: one source of truth, two tiers

**Status: Реализовано** (ярус 1) · **Контракт-цель** (ярус 2).

**DIVE must not become a second authored source of truth** for knob position, door angle, or slider value (no extra control-asset layer in the session).

State lives on the **moving mesh** (kinematic) or on a **device control component** (constraints / game). DIVE only maps input onto that state.

| Tier | Use when | Mechanism |
|------|----------|-----------|
| **1 — kinematic (now)** | Monitor session is enough; transform-as-state is enough | `UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` / `UDIVELinearDriveAction` in **Interact** mode (same hold/drag as any continuous action) |
| **2 — device control (trigger)** | VR parity, MESS telemetry, the control must work outside DIVE | Control component owns the value; the **interact verb** drives it: monitor = session gesture via `IDIVEProxyDrive` / registry, VR = host trigger-hold interactor. GRIP **grab** stays for free bodies (grip button / bridge cursor-pull). See [`../Design_PhysicalControls_OneState_TwoInputs.md`](../Design_PhysicalControls_OneState_TwoInputs.md). **Zero in-plugin implementors today.** |

| Layer | Owns |
|-------|------|
| **Device / game** | Control value, DOF limits, snap, sounds, simulation state, reuse across devices |
| **GRIP (VR / world hands)** | Hand aim, **grab** PD forces on free / demounted bodies — not panel knobs |
| **DIVE presentation** | Camera, chrome, Interact/Physical — **monitor session only** |
| **DIVE action graph** | Catalog / Bindings, primary, menu rows — monitor session **and** standing-VR action host (see design doc §8–§10) |

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
     VR standing: three gestures                  Monitor: DIVE session
     grip = GRIP grab (bodies)                    Interact LMB = panel controls
     trigger = DIVE primary (no camera)           Physical LMB = GRIP grab (bodies)
     menu button = world-space DIVE rows
```

### VR (target)

Standing at the panel **does not** call `RequestSession()` and does not steal the HMD. DIVE splits into **action graph** (Catalog / Bindings — reused) and **presentation** (camera / chrome — monitor only). See [`../Design_PhysicalControls_OneState_TwoInputs.md`](../Design_PhysicalControls_OneState_TwoInputs.md) §8–§10.

- **Grab** (grip button): GRIP PD on **grabbable** bodies.
- **Interact primary** (trigger hold/click): host action host → the same primary binding the monitor uses. Not a grab. Not ACTS `IA_Interact`.
- **Overflow actions** (dedicated menu button): world-space menu of `AppendConfiguredContextMenuEntries` for **this** hit, filtered (`Session` rows such as Focus/Isolate stay out). Not the session Slate widget, not the ACTS world menu.
- Large inertial parts may opt into constraint + grab — chassis kinematic/anchored. Default for small panel controls is interact.
- Optional: ACTS «точный осмотр» may still open a **full** DIVE camera session (sit-down inspect). That is not the standing-at-panel path.

### Flat monitor (target)

- **DIVE** brings the camera close and provides precise **screen-space input**.
- **Interact (now, tier 1):** pointer/ray maps onto the picked mesh's authored axis — polar angle for knobs/nuts (`UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` via `MapPointerToAxisAngle`), centimetres along the rail for sliders (`UDIVELinearDriveAction` via `MapPointerToAxisTravel`). Edge-on falls back to screen-delta helpers. One kinematic DOF. Standing-VR can feed the same helpers with a hand point later.
- **Interact (tier 2):** the same gesture forwards into the device control component (`IDIVEProxyDrive` / registry). Standing-VR uses the host action host on the same Catalog / Bindings — not a camera session.
- **Grab:** Physical-mode LMB → existing GRIP bridge cursor-pull, **free bodies only**.
- No second physics rig inside DIVE. See [`../Design_PhysicalControls_OneState_TwoInputs.md`](../Design_PhysicalControls_OneState_TwoInputs.md).

---

## 3. What belongs where

### Device / game layer (author here)

| Concern | Where |
|---------|--------|
| Hinge / slide / dial **constraints** | Device Blueprint / C++ |
| Grabbable marker (`UGRIPGrabbableComponent` / `IGRIPGrabbable`) | On **free / demounted** bodies only — not on panel knobs |
| Sounds, haptics, MESS hooks | Device control component |
| Reuse on another device | Same control prefab / component |

Recommended **control wiring**:

**Tier 1 (Interact mode, in-plugin):** bind a primitive (tag / name / PartId) to `UDIVERotaryDriveAction`, `UDIVEThreadedDriveAction`, or `UDIVELinearDriveAction`. The action is a stateless mapper; the mesh transform is the state. Catalog instances are shared — do not store per-target runtime state on the action beyond the active gesture. Min/Max angle and travel are from the part's rest pose (first grab, keyed by primitive + action class), not per-drag deltas.

Optional **domain readout** (`bUseDomainReadout` + `DomainMin`/`DomainMax` + `ReadoutSuffix` on Rotary/Linear) is a **HUD / Value Event mapping** of `Normalized` onto author units (amps, volts, …) — not a second live control value. Device sim / MESS still owns domain truth; write it from **DIVE Action Value Event** on the device graph (fan-out from the active Inspectable). Never bind catalog `Action->OnValueChanged` from BeginPlay — the catalog instance is shared across devices.

**Tier 2 (host implements, both verbs):**

1. **Interact** — control component owns value/limits/detents/events. Monitor: session gesture via `IDIVEProxyDrive` / registry. VR: host trigger-hold interactor (not GRIP, not ACTS Hold).
2. **Grab** — GRIP on simulating grabbable bodies (bridge on monitor, grip button in VR). Panel knobs are not grabbable.

```text
IDIVEProxyDrive (DIVECore) — contract-goal for monitor interact (0 in-plugin implementors)
  CanProxyDrive(FDIVEProxyDriveContext) → bool
  BeginProxyDrive(FDIVEProxyDriveContext)
  ApplyProxyDriveDelta(ScreenDelta)
  EndProxyDrive(bCommit)
  GetProxyDriveNormalizedValue(OutNormalized) → bool
```

- **Interact (tier 1):** Interact-mode primary / menu hold-drag on the built-in drive actions.
- **Interact (tier 2):** session pick → `BeginProxyDrive` / `ApplyProxyDriveDelta`. Standing-VR: host action host on the same Catalog / Bindings (no camera).
- **Grab:** GRIP (bridge cursor-pull / VR grip button) on free bodies only.
- **Sounds / live domain:** device component when value/angle actually changes (tier 2); or tier 1 via `NotifyInteractionValue` → session HUD + active Inspectable `OnActionValueChanged` → K2 **DIVE Action Value Event**. One-shot success stays on K2 **DIVE Action Event** / `OnActionExecuted`.

### DIVE plugin (stays thin)

| In scope | Out of scope |
|----------|----------------|
| Session, camera rig, focus stack | Authoring constraints on devices; standing-VR camera steal |
| Ray pick from DIVE camera | GRIP hand physics; VR controller aim (host) |
| `IDIVEProxyDrive` hook (routing in session subsystem) | Implementations on devices (game module) |
| `Handle*` input API for Enhanced Input | MESS / electrical solver |
| Context menu **data** (Catalog / Bindings, `GetDisplayState`) | Session Slate widget in HMD; ACTS world menu; host world-space menu widget |

### Direct manipulation vs physical controls

| Interaction | Mechanism |
|-------------|-----------|
| Knob, nut, kinematic lever | **Interact** mode → `UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` (tier 1) |
| Slider / rail | **Interact** mode → `UDIVELinearDriveAction` (tier 1; pointer → cm on axis) |
| Constrained / VR-parity panel control | **Interact** → host control component; monitor via `IDIVEProxyDrive` / registry (contract-goal, 0 implementors); standing-VR via host action host on the same Catalog / Bindings (no camera) |
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
| Zoom | `HandleZoomIn`, `HandleZoomOut` (pawn-drive: same handlers → `HandlePawnPhysicalGrabHoldDistanceScroll`) | `IA_DIVE_Zoom` |
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
  Interact  — interact verb: primary → binding PrimaryActionIndex (or sole continuous action); hover overlay; explicit focus via HandleFocusUnderCursor / context menu
  Physical  — grab verb: primary → pawn physical drive (GRIP bridge). Code today also tries IDIVEProxyDrive first (pre-rev2); target: grab only
  Logical   — planned; not yet in the enum (v1 roadmap)
```

Mode is **not** tied to a single mouse button. It is **policy** for routing **multiple** semantic actions, for example:

| Policy area | Example behaviour per mode |
|-------------|----------------------------|
| `HandlePrimaryAction*` | Physical → grab (code today: device proxy first, then GRIP); Interact → catalog action / hover |
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
2. **Default entries**: component **Bindings** (Focus, Isolate, Simulate Physics, Delete Mesh unless removed; hidden in Shipping).
3. **Device extensions**: **Action Catalog** (and/or extra Bindings) — `UDIVEDeviceAction` instances.
4. Custom row click → **`UDIVEDeviceAction::Execute`** / continuous **`BeginInteraction`**. See **`QUICKSTART.md`** §1.
5. Display state (enabled / checked / visible / label) comes from **`GetDisplayState`** on the action — no `Is_*` reflection. Default: `CanExecute` false → gray; **Condition** false → hidden.
6. Section **Header** (optional) is drawn above the separator between menu sections.

Remapping «open menu» to RMB, Q, or gamepad — **IMC only**. Standing-VR uses a **separate** world-space menu (design doc §10), not this Slate widget and not ACTS.

### 4.4. Primary action (semantic)

Target API: **`HandlePrimaryActionPressed/Released`** (project maps **`IA_DIVE_PrimaryAction`**).

| Mode | Primary action (press/hold/release) |
|------|-------------------------------------|
| **Interact** | Binding `PrimaryActionIndex` when configured, or the binding's only action if it is continuous (Name > PartId > Tag > Any; ties = earlier Bindings entry); hover overlay on pickable mesh. Kinematic knobs/nuts: bind Rotary/Threaded as primary (polar rim). Sliders: bind Linear as primary (cm along Axis; edge-on falls back to `CmPerPixel`). Begin isolates the driven child from a simulating parent (unweld, Query Only for the gesture). Engine cylinder is Z-up: Rotary Axis Z spins in place; Linear default Axis X is travel. |
| **Physical** | Pawn GRIP grab via DIVEGRIPBridge. Code today also tries `IDIVEProxyDrive` via `InternalProxyDriveAction` first — target (tier 2): proxy lives on Interact, Physical is grab-only. |
| **Logical** | Planned (v1 roadmap); not yet in the enum |

While held, the session ticks `UpdateInteraction(FDIVEInteractionUpdate)` (cursor, ray, view — not ScreenDelta alone). Still **semantic**, not raw «mouse moved».

### 4.5. Layer diagram (target)

```text
  IMC (ATSEP)          IA_DIVE_*  ──►  UDIVEPlayerComponent::Handle*
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
| `EDIVESessionInteractionMode` + `SetInteractionMode` | Interact / Physical |
| `HandlePrimaryAction*` | Interact → binding `PrimaryActionIndex` (when set, or sole continuous action); hover overlay; Physical → pawn GRIP grab (code today also tries device proxy first) |
| Context menu + widget | Focus, Isolate, Simulate, Delete (Bindings; Admin hidden in Shipping); custom from Catalog |
| Action Catalog / Bindings + `UDIVEDeviceAction` | Object actions; continuous via shared interaction slot |
| Hover overlay | `DIVE|Pick|Hover` on inspectable; exclusions via `PickInteractionExclusions` |
| `UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` / `UDIVELinearDriveAction` | Interact-mode knobs/nuts/sliders; author on the device via Bindings / Catalog |
| `IDIVEDeviceControlRegistry` / `IDIVEProxyDrive` | Host implements on devices (contract-goal; 0 in-plugin implementors) |
| Anchor | Viewpoint + PartId only |

### Host project (remaining)

| Item | Owner |
|------|--------|
| `IA_DIVE_*` Content + `IMC_DIVE` | Host Content |
| Registry / proxy drive on prefabs | Host / devices |
| Context menu rows per pick | Action Catalog / Bindings + `UDIVEDeviceAction` |
| GRIP / MESS for cables | Host project |

---

## 6. `IDIVEProxyDrive` — contract-goal, not UX

**Status: Контракт-цель.**

`IDIVEProxyDrive` (DIVECore) is the **monitor adapter for the interact verb** (tier 2). The session forwards the same hold/drag used by tier-1 rotary/threaded/linear actions onto a device control component. Standing-VR talks to the **same Catalog / Bindings** through a host action host — not through GRIP, not through `RequestSession()`. DIVERuntime ships **zero** implementors; do not treat this as a finished product path until a host device actually implements it.

```text
IDIVEProxyDrive (DIVECore)
  CanProxyDrive(FDIVEProxyDriveContext) → bool
  BeginProxyDrive(FDIVEProxyDriveContext)   // Context includes ViewLocation/Rotation + PickRayDir
  ApplyProxyDriveDelta(ScreenDelta)
  EndProxyDrive(bCommit)
  GetProxyDriveNormalizedValue(OutNormalized) → bool
```

`FindProxyDriveForHit` walks the attach-root + `ForEachDeviceActor`. N>1 registry results or N>1 drive components on the hit owner → Warning + `nullptr` (no silent first-wins).

Prefer **device registry** at session start (primitive / tag → control component) when Blueprint authoring is easier than `Implement Interface` on every part.

- **Interact (tier 1, code):** catalog continuous actions in Interact mode.
- **Interact (tier 2, target):** same gesture / binding forwards to `IDIVEProxyDrive` / registry. Not yet the Physical-mode pick path.
- **Grab (Physical):** GRIP bridge cursor-pull / VR grip button on free bodies.
- **Physical pick (code today, pre-rev2):** `HandlePrimaryAction*` → registry or `IDIVEProxyDrive` first, then pawn GRIP. Move proxy off this path when tier 2 starts.
- **Value HUD:** compact chip next to the driven primitive (cursor fallback). `UDIVEProxyDriveForwardAction` polls `GetProxyDriveNormalizedValue` at Begin and after each delta (`NotifyValueChanged`). Rotary/Threaded/Linear send `FDIVEInteractionValue` via `NotifyInteractionValue` (signed Absolute + Unit; widget formats).
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
(self-contained)     (self-contained)     UObject provider on DIVE Player
     │                    │                        │
     │  pick / session    │  hand / grab           │
     └────────────────────┴────────────────────────┘
                          │
              Pawn setup (game or ATSEP)
              - UGRIPRigComponent (slots Player + Dive; Hand is not Add Component)
              - UDIVEPlayerComponent (GRIP Physical Drive auto-created)
              - VR: Rig slots only, no extra DIVE grab Hand from the picker
```

**Option A — Device proxy drive (tier 2 interact, not a grab)**

1. Device registers controls (registry) or implements `IDIVEProxyDrive` on control components.
2. **Target:** Interact-mode catalog / `UDIVEProxyDriveForwardAction` routes pick → device `IDIVEProxyDrive` (deterministic DOF). **Code today** still auto-discovers this on Physical primary (`InternalProxyDriveAction`) — leftover from mixing verbs; do not treat as the product path.
3. GRIP is not involved on the monitor for these controls; VR uses a host trigger-hold interactor on the same component, not a grab.

**Option B — Pawn physical drive (generic simulating-mesh drag)**

1. Pawn keeps **two** grip instances when DIVE must not steal the player hand — author them as **GRIP Rig** slots, not as Hand from Add Component:
   - Slot **Player** → `GRIP Hand` — gameplay grab
   - Slot **Dive** → `GRIP Hand Dive` — Physical drag
   - DIVE Player **Physical Drive Provider**: picker → `UGRIPRigComponent::GetHand(Dive)` (property **GRIP Rig Slot**, default `Dive`) → name `GRIP Hand Dive` (no sole-Hand fallback)
2. In **Physical** mode, if no device proxy handles the pick, session falls back to `IDIVEPawnPhysicalDrive` on the pawn.
3. Provider calls GRIP Hand API (`TryGrabFromHit`, `SetHandWorldTransform`, aim suppress via Rig `SetSlotAimSuppressed` on Dive) — see GRIP `INTEGRATION.md` / multi-hand slots.
4. Cursor-follow drag: provider deprojects screen position each tick onto the grab-depth ray. **Hold R** while dragging to manual-rotate. The session does **not** push `ScreenDelta`. Value HUD / `OnInteractionValueChanged` are **not** fed from this pawn path. Device `IDIVEProxyDrive` feeds the HUD only when `GetProxyDriveNormalizedValue` returns true (polled by `UDIVEProxyDriveForwardAction`). Wheel while pawn-drive: `HandleZoomIn/Out` → `HandlePawnPhysicalGrabHoldDistanceScroll`.
5. Do not add a Physical-drive ActorComponent; the provider is a UObject on DIVE Player.

See GRIP [`ARCHITECTURE.md`](../../GraspRigidbodyInertialPhysics/Docs/ARCHITECTURE.md) § Grip instances.

Use **B** for «grab and pull» on simulating bodies (admin **Simulate Physics** + Physical mode). Use **A** for deterministic kinematic drive (training sim, snap ticks, no physics jitter) — Interact verb, not Physical grab.

**Routing priority in Physical mode (code today)**

```text
Pick hit → IDIVEProxyDrive (device) → IDIVEPawnPhysicalDrive (pawn GRIP) → fail (Warning log)
```

**Target (rev 2, when tier 2 starts):** Physical pick is pawn GRIP only. Device proxy is an Interact binding / ForwardAction, same gesture as Rotary/Threaded.

### Dependency summary

| Approach | DIVE → GRIP link | When |
|----------|------------------|------|
| None (default) | No | DIVE-only titles, tests |
| `DIVEGRIPBridge` sibling plugin | Enable separately; `DIVERuntime` stays GRIP-free | Generic Physical drag with GRIP |
| Device `IDIVEProxyDrive` | No GRIP on monitor | Sliders, hinges, custom DOF |

**MUST NOT:** `#include` GRIP headers or link `GRIPRuntime` from `DIVERuntime`.

---

## 8. Session interaction with GRIP in the world

**Status: Обязанность хоста.**

While a DIVE session is active, **suppressing gameplay ACTS / GRIP input is a host duty** (PlayerController / IMC swap). DIVE does not block those plugins itself. RuntimeDev only **polls** whether ACTS is present for debug dump — it is not an input gate.

VR planning (standing at the panel — **product decision**, not «game later»):

| Belt | Input | Mechanism |
|------|--------|-----------|
| World | ACTS `IA_Interact` | ACTS world menu; may `OpenDIVE` for sit-down camera session |
| Panel | trigger + dedicated menu button | DIVE **action graph** via host action host; **no** camera session |
| Grab | grip | GRIP on grabbable bodies |

`BuildContextMenuEntries` / `TryBeginContinuousAction` today require an active session. Query (`AppendConfiguredContextMenuEntries`, `TryResolvePrimaryAction`) does not. Headless action host is tier-2/VR work — do not start without a trigger. Full table: [`../Design_PhysicalControls_OneState_TwoInputs.md`](../Design_PhysicalControls_OneState_TwoInputs.md) §8–§10.

---

## 9. Authoring checklist (device team)

1. **Tier 1:** tag the moving mesh; bind Rotary or Threaded drive in the Action Catalog / Inspectable Bindings (Interact mode).
2. **Tier 2 (when triggered):** control prefab (mesh + control component + sounds). No grabbable marker on panel knobs. Monitor: `IDIVEProxyDrive` / registry. VR standing: host action host on the **same** Catalog / Bindings (no camera session).
3. Mark **free / demounted** bodies grabbable for GRIP. After a nut crosses the release threshold, *then* it becomes grabbable.
4. Add `UDIVEInspectableComponent` on device root for session entry.
5. Optional anchors for **viewpoints** and **non-physical** operations.

---

## 10. Open work

| Item | Owner |
|------|--------|
| `IA_DIVE_*` + IMC for DIVE session | Host Content |
| Control registry + drive backends on device prefabs | Host / devices |
| Optional virtual GRIP hand | Sibling plugin `DIVEGRIPBridge` |
| VR standing action host + world menu | Host / DeviceControlKit — design doc §8–§10; not DIVERuntime Slate |
| VR two-hand + OpenXR | GRIPVR / game |

DIVE object-action model (v0.8): `UDIVEDeviceAction`, Catalog + component Bindings, continuous slot — done. Sample Unscrew module removed; author BP via Content Browser → DIVE factories. See `Audit_DeviceActions_Architecture.md` §12.

---

## 11. One-line summary

**Kinematic knobs and nuts are Interact-mode continuous actions (transform-as-state). Constrained / VR-parity controls stay on the device (`IDIVEProxyDrive` is a host contract). Standing-VR reuses the action graph without the camera session (design doc §8–§10). DIVE presentation stays the flat-screen adapter: camera chrome, explicit focus via menu, interaction modes as session policy — composed in the game layer without a hard GRIP dependency in the plugin.**
