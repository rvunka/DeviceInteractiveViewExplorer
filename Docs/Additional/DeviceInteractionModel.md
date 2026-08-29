# DIVE and physical device controls

> **Audience:** device authors, game integration (ATSEP), VR planning.  
> **Status:** architecture contract (v0.8). Knobs are kinematic continuous actions + K2 on the device. Two verbs: **interact** (panel — monitor session + standing-VR action host) and **grab** (free bodies — GRIP). Standing-VR does **not** open the DIVE camera.  
> **Markers:** **Реализовано** — код в DIVE; **Обязанность хоста** — IMC / PC / префабы.  
> **Related:** `ARCHITECTURE.md`, `QUICKSTART.md`, GRIP `Docs/ARCHITECTURE.md`. Two verbs (interact vs grab): [`../Design_PhysicalControls_OneState_TwoInputs.md`](../Design_PhysicalControls_OneState_TwoInputs.md).

---

## 1. Principle: one source of truth, two tiers

**Status: Реализовано.**

**DIVE must not become a second authored source of truth** for knob position, door angle, or slider value (no extra control-asset layer in the session).

State lives on the **moving mesh** (kinematic). Domain / sounds live on the **device** (variables + K2 Action / Value Event). DIVE maps the gesture onto the mesh.

| Path | Use when | Mechanism |
|------|----------|-----------|
| **Kinematic (now)** | Transform-as-state is enough | `UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` / `UDIVELinearDriveAction` in **Interact** mode |
| **Standing-VR** | Same part without the DIVE camera | Host action host on the same Catalog / Bindings. GRIP **grab** stays for free bodies. See [`../Design_PhysicalControls_OneState_TwoInputs.md`](../Design_PhysicalControls_OneState_TwoInputs.md). |

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
- **Interact:** pointer/ray maps onto the picked mesh's authored axis — polar angle for knobs/nuts (`UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` via `MapPointerToAxisAngle`), centimetres along the rail for sliders (`UDIVELinearDriveAction` via `MapPointerToAxisTravel`). Edge-on falls back to screen-delta helpers. One kinematic DOF. Standing-VR uses the host action host on the same Catalog / Bindings — not a camera session.
- **Grab:** Physical-mode LMB → existing GRIP bridge cursor-pull, **free bodies only**.
- No second physics rig inside DIVE. See [`../Design_PhysicalControls_OneState_TwoInputs.md`](../Design_PhysicalControls_OneState_TwoInputs.md).

---

## 3. What belongs where

### Device / game layer (author here)

| Concern | Where |
|---------|--------|
| Hinge / slide / dial **constraints** | Device Blueprint / C++ |
| Grabbable marker (`UGRIPGrabbableComponent` / `IGRIPGrabbable`) | On **free / demounted** bodies only — not on panel knobs |
| Sounds, haptics, MESS hooks | Device graph (K2 Action / Value Event) |
| Reuse on another device | Same control prefab / component |

Recommended **control wiring**:

**Tier 1 (Interact mode, in-plugin):** bind a primitive (tag / name / PartId) to `UDIVERotaryDriveAction`, `UDIVEThreadedDriveAction`, or `UDIVELinearDriveAction`. The mesh transform is the live pose. Each Inspectable holds a catalog **copy**; rest pose / committed travel lives on that copy keyed by primitive (`FDrivenRestStore`), not on the asset template. Min/Max angle and travel are from the part's rest pose (first grab), not per-drag deltas.

Optional **domain readout** (`bUseDomainReadout` + `DomainMin`/`DomainMax` + `ReadoutSuffix` on Rotary/Linear) is a **HUD / Value Event mapping** of `Normalized` onto author units (amps, volts, …) — not a second live control value. Device sim / MESS still owns domain truth; write it from **DIVE Action Value Event** on the device graph (fan-out from the active Inspectable). Never bind catalog-asset `Action->OnValueChanged` from BeginPlay — the asset is a template; runtime executes per-Inspectable copies.

**Grab** — GRIP on simulating grabbable bodies (bridge on monitor, grip button in VR). Panel knobs are not grabbable.

- **Interact:** Interact-mode primary / menu hold-drag on the built-in drive actions. Standing-VR: host action host on the same Catalog / Bindings (no camera).
- **Grab:** GRIP (bridge cursor-pull / VR grip button) on free bodies only.
- **Sounds / live domain:** `NotifyInteractionValue` → session HUD + active Inspectable `OnActionValueChanged` → K2 **DIVE Action Value Event**. One-shot success stays on K2 **DIVE Action Event** / `OnActionExecuted`.

### DIVE plugin (stays thin)

| In scope | Out of scope |
|----------|----------------|
| Session, camera rig, focus stack | Authoring constraints on devices; standing-VR camera steal |
| Ray pick from DIVE camera | GRIP hand physics; VR controller aim (host) |
| Catalog continuous actions (Rotary / Threaded / Linear / Press) | Device-side K2; Physical auto-discovery of catalog actions |
| `Handle*` input API for Enhanced Input | MESS / electrical solver |
| Context menu **data** (Catalog / Bindings, `GetDisplayState`) | Session Slate widget in HMD; ACTS world menu; host world-space menu widget |

### Direct manipulation vs physical controls

| Interaction | Mechanism |
|-------------|-----------|
| Knob, nut, kinematic lever | **Interact** mode → `UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` (tier 1) |
| Slider / rail | **Interact** mode → `UDIVELinearDriveAction` (tier 1; pointer → cm on axis) |
| Standing-VR panel control | Same Catalog / Bindings via a host action host (no camera) |
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
  Physical  — grab verb: primary → pawn physical drive (GRIP bridge) only
  Logical   — planned; not yet in the enum (v1 roadmap)
```

Mode is **not** tied to a single mouse button. It is **policy** for routing **multiple** semantic actions, for example:

| Policy area | Example behaviour per mode |
|-------------|----------------------------|
| `HandlePrimaryAction*` | Physical → grab (pawn GRIP); Interact → catalog action / hover |
| Context menu contents | Component Bindings + Action Catalog |
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
5. Display state (enabled / checked / visible / label) comes from **`GetDisplayState`** on the action — no `Is_*` reflection. Default: `CanExecute` false → gray; **Condition** false → hidden **and** `CanExecute` false (primary blocked). Predicate reads device state, not the catalog **asset template**. Cover-after-bolts: **`QUICKSTART.md`** §1.
6. Section **Header** (optional) is drawn above the separator between menu sections.

Remapping «open menu» to RMB, Q, or gamepad — **IMC only**. Standing-VR uses a **separate** world-space menu (design doc §10), not this Slate widget and not ACTS.

### 4.4. Primary action (semantic)

Target API: **`HandlePrimaryActionPressed/Released`** (project maps **`IA_DIVE_PrimaryAction`**).

| Mode | Primary action (press/hold/release) |
|------|-------------------------------------|
| **Interact** | Binding `PrimaryActionIndex` when configured, or the binding's only action if it is continuous (Name > PartId > Tag > Any; ties = earlier Bindings entry); hover overlay on pickable mesh. Kinematic knobs/nuts: bind Rotary/Threaded as primary (polar rim). Sliders: bind Linear as primary (cm along Axis; edge-on falls back to `CmPerPixel`). Begin isolates the driven child from a simulating parent (unweld, Query Only for the gesture). Engine cylinder is Z-up: Rotary Axis Z spins in place; Linear default Axis X is travel. |
| **Physical** | Pawn GRIP grab via DIVEGRIPBridge. Catalog continuous actions stay on Interact. |
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
| `HandlePrimaryAction*` | Interact → binding `PrimaryActionIndex` (when set, or sole continuous action); hover overlay; Physical → pawn GRIP grab |
| Context menu + widget | Focus, Isolate, Simulate, Delete (Bindings; Admin hidden in Shipping); custom from Catalog |
| Action Catalog / Bindings + `UDIVEDeviceAction` | Object actions; continuous via shared interaction slot |
| Hover overlay | `DIVE|Pick|Hover` on inspectable; exclusions via `PickInteractionExclusions` |
| `UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` / `UDIVELinearDriveAction` | Interact-mode knobs/nuts/sliders; author on the device via Bindings / Catalog |
| Anchor | Viewpoint + PartId only |

### Host project (remaining)

| Item | Owner |
|------|--------|
| `IA_DIVE_*` Content + `IMC_DIVE` | Host Content |
| Context menu rows per pick | Action Catalog / Bindings + `UDIVEDeviceAction` |
| GRIP / MESS for cables | Host project |

---

## 6. Interact vs Physical

**Status: Реализовано.**

- **Interact:** catalog continuous actions (Rotary / Threaded / Linear / Press). Value HUD from `NotifyInteractionValue` / `NotifyValueChanged`. Sounds and domain on the device K2 graph.
- **Grab (Physical):** `HandlePrimaryAction*` → pawn GRIP only (`IDIVEPawnPhysicalDrive`).
- **Standing-VR:** same Catalog / Bindings through a host action host — not GRIP, not `RequestSession()`.

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

**Pawn physical drive (generic simulating-mesh drag)**

1. Pawn keeps **two** grip instances when DIVE must not steal the player hand — author them as **GRIP Rig** slots, not as Hand from Add Component:
   - Slot **Player** → `GRIP Hand` — gameplay grab
   - Slot **Dive** → `GRIP Hand Dive` — Physical drag
   - DIVE Player **Physical Drive Provider**: picker → `UGRIPRigComponent::GetHand(Dive)` (property **GRIP Rig Slot**, default `Dive`) → name `GRIP Hand Dive` (no sole-Hand fallback)
2. In **Physical** mode the session routes the pick to `IDIVEPawnPhysicalDrive` on the pawn.
3. Provider calls GRIP Hand API (`TryGrabFromHit`, `SetHandWorldTransform`, aim suppress via Rig `SetSlotAimSuppressed` on Dive) — see GRIP `INTEGRATION.md` / multi-hand slots.
4. Cursor-follow drag: provider deprojects screen position each tick onto the grab-depth ray. **Hold R** while dragging to manual-rotate. The session does **not** push `ScreenDelta`. Value HUD / `OnInteractionValueChanged` are **not** fed from this pawn path. Wheel while pawn-drive: `HandleZoomIn/Out` → `HandlePawnPhysicalGrabHoldDistanceScroll`.
5. Do not add a Physical-drive ActorComponent; the provider is a UObject on DIVE Player.

See GRIP [`ARCHITECTURE.md`](../../GraspRigidbodyInertialPhysics/Docs/ARCHITECTURE.md) § Grip instances.

Use this for «grab and pull» on simulating bodies (admin **Simulate Physics** + Physical mode). Knobs stay Interact + Rotary.

**Routing priority in Physical mode**

```text
Pick hit → IDIVEPawnPhysicalDrive (pawn GRIP) → fail (Warning log)
```

### Dependency summary

| Approach | DIVE → GRIP link | When |
|----------|------------------|------|
| None (default) | No | DIVE-only titles, tests |
| `DIVEGRIPBridge` sibling plugin | Enable separately; `DIVERuntime` stays GRIP-free | Generic Physical drag with GRIP |

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

Query (`AppendConfiguredContextMenuEntries`, `TryResolvePrimaryAction`) and headless execute (`Inspectable::ExecuteAction` / Update / End) do **not** require an active camera session. Session wrappers (`BuildContextMenuEntries`, `TryBeginContinuousAction`) and Focus/Isolate **execution** still do. Standing-VR host uses the Inspectable API + its own tick/input — see [`../Design_PhysicalControls_OneState_TwoInputs.md`](../Design_PhysicalControls_OneState_TwoInputs.md) §8–§10.

---

## 9. Authoring checklist (device team)

1. Tag the moving mesh; bind Rotary or Threaded drive in the Action Catalog / Inspectable Bindings (Interact mode). Sounds / domain: K2 Action / Value Event on the device.
2. Standing-VR: host action host on the **same** Catalog / Bindings (no camera session).
3. Mark **free / demounted** bodies grabbable for GRIP. After a nut crosses the release threshold, *then* it becomes grabbable.
4. Add `UDIVEInspectableComponent` on device root for session entry.
5. Optional anchors for **viewpoints** and **non-physical** operations.

---

## 10. Open work

| Item | Owner |
|------|--------|
| `IA_DIVE_*` + IMC for DIVE session | Host Content |
| Optional virtual GRIP hand | Sibling plugin `DIVEGRIPBridge` |
| VR standing action host + world menu | Host / DeviceControlKit — design doc §8–§10; not DIVERuntime Slate |
| VR two-hand + OpenXR | GRIPVR / game |

DIVE object-action model (v0.8): `UDIVEDeviceAction`, Catalog + component Bindings, continuous slot — done. Sample Unscrew module removed; author BP via Content Browser → DIVE factories. See `Audit_DeviceActions_Architecture.md` §12.

---

## 11. One-line summary

**Kinematic knobs and nuts are Interact-mode continuous actions (transform-as-state). Sounds and domain values hang on the device K2 graph. Standing-VR reuses the action graph without the camera session (design doc §8–§10). DIVE presentation stays the flat-screen adapter: camera chrome, explicit focus via menu, interaction modes as session policy — composed in the game layer without a hard GRIP dependency in the plugin.**
