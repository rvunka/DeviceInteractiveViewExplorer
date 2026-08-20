# DIVE Architecture (v0.8-dev)

## Layers

```text
┌─────────────────────────────────────────┐
│ Host project (ACTS entry, EI, PC)       │
├─────────────────────────────────────────┤
│ DIVERuntimeDev (optional, debug only)   │
│  UDIVELegacyKbmInputComponent — BindKey │
│  DIVEGripLegacyDevQuery (PIE wheel glue)│
├─────────────────────────────────────────┤
│ DIVEUnrealEditor (editor only)          │
│  DIVE Scan Device · validation reports  │
├─────────────────────────────────────────┤
│ DIVEGRIPBridge (sibling plugin, optional §7) │
│  UDIVEGRIPPhysicalDriveProvider — UObject    │
│  on DIVE Player; IDIVEPawnPhysicalDrive      │
├─────────────────────────────────────────┤
│ DIVERuntime                             │
│  UDIVESessionSubsystem — focus stack      │
│  UDIVEPlayerComponent — sole pawn AC      │
│    input · chrome · context menu · drive  │
│  UDIVEInspectableComponent              │
│  UDIVEAnchorComponent (optional)        │
│  ADIVECameraRig                         │
│  UDIVEDeviceDefinitionAsset             │
├─────────────────────────────────────────┤
│ DIVECore — FDIVEFocusTarget, IDIVEProxyDrive, UDIVEDeviceAction, types │
└─────────────────────────────────────────┘
```

## Subsystem scope

**Current:** `UWorldSubsystem` (`UDIVESessionSubsystem`) — one active DIVE session per game/PIE world (`DoesSupportWorldType` filters out Editor preview worlds).

DIVE Player guards with `IsLocallyControlled()`. Owner **must** be a locally controlled `APawn` (validated by `IsDataValid` on Player). Session ends on world tear-down (`OnWorldBeginTearDown` + `Deinitialize`), on `UDIVEPlayerComponent::EndPlay` for the locally controlled pawn, and when that pawn loses local control while a session is active.

**Invariant:** one active session per world ⇒ Action Catalog instances are shared and must not be cloned per device. Continuous actions use `ensure(!bInteractionActive)`.

**Input restore note:** session end approximates prior mode via `EDIVEPreservedInputMode` (GameAndUI only if both cursor and click events were on). PlayerController does not expose the previous `FInputMode` directly.

Replication / listen-server presentation on a remote pawn is a non-goal.

## Input

| Component | Module | Role |
|-----------|--------|------|
| **DIVE Player** | `DIVERuntime` | Sole pawn ActorComponent: `Handle*`, chrome, context menu, optional Physical provider |
| **Legacy KBM** | `DIVERuntimeDev` | `BindKey` only → forwards to **DIVE Player** (PIE dev). Player does not create it |

Recommended pawn stack: one **`UDIVEPlayerComponent`** (+ optional **Legacy KBM** for PIE). When **DIVEGRIPBridge** is enabled, Player auto-creates an instanced **DIVE GRIP Physical Drive** UObject (`IDIVEPawnPhysicalDrive`). Physical GRIP on the pawn is **GRIP Rig** with slots **Player** and **Dive** (Hand is not in Add Component). Bind Enhanced Input to Player `Handle*`.

**Contract:** physical keys → `UInputAction` in host Content → Enhanced Input on **Pawn / PlayerController** → plugin `Handle*`. See `../../../Docs/Plugin_Architecture_Principles.md` §4 and **`Docs/Additional/DeviceInteractionModel.md` §4**.

`HandleExitSession` while the context menu is open is **intentional** — Exit is not suppressed by the menu.

### Session chrome (always available)

Orbit, zoom, focus undo, exit, and context menu work in **every** interaction mode.

Zoom steps scale with orbit distance by default; focusing a primitive fits distance to its
bounds and raises a soft near floor from the part radius (`FDIVECameraSettings` on Inspectable, or Device Definition when `bUseDeviceDefinitionSettings`).

Pick accepts visible meshes and **shape collision** volumes (including Hidden-in-Game shapes);
hidden meshes need tag `DIVE.PickProxy`.

### Interaction mode

`EDIVESessionInteractionMode` in **DIVECore**: **Interact** | **Physical**.

- Resets to **Interact** on session start/end
- **Physical (code today):** `TryBeginProxyDrive*` via `HandlePrimaryAction*` — device `IDIVEProxyDrive` first, then pawn GRIP grab. **Target:** GRIP grab only; proxy belongs to Interact (tier 2).
- **Interact:** binding `PrimaryActionIndex` (when set); hover overlay on pick; explicit focus via `HandleFocusUnderCursor` / context menu

### Enhanced Input (host Content)

| Input Action | Event | Call |
|--------------|-------|------|
| `IA_DIVE_Orbit` | Started / Completed / Triggered | `HandleOrbit*` |
| `IA_DIVE_Zoom` | Triggered | `HandleZoomIn` / `HandleZoomOut` (while pawn physical drive is active, these forward to `HandlePawnPhysicalGrabHoldDistanceScroll`) |
| `IA_DIVE_PrimaryAction` | Started / Completed | `HandlePrimaryActionPressed` / `Released` |
| `IA_DIVE_FocusTarget` | Started | `HandleFocusUnderCursor` |
| `IA_DIVE_Back` / `IA_DIVE_Exit` | Started | `HandleNavigateBack` / `HandleExitSession` |
| `IA_DIVE_SetMode_*` | Started | `SetInteractionMode` |
| `IA_DIVE_ContextMenu` | Started | `HandleContextMenuRequested` |

Bind these on **DIVE Player**.

Legacy PIE (`UDIVELegacyKbmInputComponent`): **RMB** = context menu, **G** = focus, **Tab** = cycle mode, **LMB** = `HandlePrimaryAction*` (see table in `QUICKSTART.md` §4).

### Context menu

In-session menu at cursor — **not** ACTS. Focus / Isolate / Admin are normal **Bindings** on `UDIVEInspectableComponent` (editable/removable; **Add Admin Defaults** restores Simulate/Delete if missing; Admin hidden in Shipping). Device ops from **Action Catalog** (Content Browser → **DIVE → Action Catalog**) and/or extra component Bindings. Create action / continuous / condition Blueprints via **DIVE → Device Action / Continuous Device Action / Action Condition**. Optional section **Header** labels above separators. **PrimaryActionIndex** on a matching binding drives **`HandlePrimaryActionPressed`** (winner = most specific Match Mode: Name > PartId > Tag > Any; equal specificity keeps the earlier binding in the Bindings array; menu/section order follows those arrays). Binding Details shows **Matched: N** for the current Match query (**Select** highlights matching primitives on a placed device or Blueprint viewport preview). Hover: **DIVE | Pick | Hover**. Exclusions: **Pick Interaction Exclusions**. **`UDIVEPlayerComponent`** on the player character owns the menu widget. Focus stack undo: `IA_DIVE_Back`. Pick resolves to **primitives** (or device root); anchors are viewpoints / PartIds, not a separate primary-pick focus path.

## Device interaction (direct manipulation)

| Intent | Mechanism |
|--------|-----------|
| Focus / isolate / back | Context menu or `HandleFocusUnderCursor` |
| Custom device action | Context menu / primary → **Action Catalog / Bindings** + **`UDIVEDeviceAction`** |
| Interact-mode hover | **DIVE \| Pick \| Hover** — `Hover Overlay Material` + optional `Pick Hover Overlay By Component` |
| Non-interactive meshes | `Pick Interaction Exclusions` or `Skip Component Tag` |
| Door, slider, knob (kinematic) | **Interact** mode + `UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` on a binding |
| Device-owned DOF / VR-parity control | **Interact** verb → host control component; monitor via `IDIVEProxyDrive` / registry (zero in-plugin implementors); standing-VR via host action host on the same Catalog / Bindings (**no** camera session — [`Design_PhysicalControls_OneState_TwoInputs.md`](Design_PhysicalControls_OneState_TwoInputs.md) §8–§10) |
| Cable, grab | GRIP + MESS in host project |

## Proxy drive

Panel controls are an **interact** verb (deterministic gesture), not a GRIP grab — see [`Design_PhysicalControls_OneState_TwoInputs.md`](Design_PhysicalControls_OneState_TwoInputs.md). DIVECore exposes **`IDIVEProxyDrive`** / **`IDIVEDeviceControlRegistry`** so the monitor session can forward that gesture to a device control component (contract-goal — zero in-plugin implementors). Standing-VR reuses the **action graph** (Catalog / Bindings) through a host action host — it does **not** open the DIVE camera session. GRIP grab (bridge cursor-pull / grip button) stays for **free bodies**. Kinematic knobs/nuts in-session use the built-in continuous actions above (state = mesh transform).

## Anchor (viewpoint only)

`UDIVEAnchorComponent`: `PartId`, `DisplayName`, authored camera viewpoint.
Focus via mesh pick, context menu, or `DefaultStartFocusId`. Optional editor-only
view-direction arrow (`DIVE|Editor`).

## Device isolate

Context menu **Isolate** / **`ToggleIsolateFocused()`** — hides other meshes on the device host, keeps the focused branch visible. Level actors are not modified.

_Future:_ world-level focus dim via custom depth / post-process (not actor hiding).

## Session flow

1. `RequestSession()` → mode **Interact**; camera blends to start focus.
2. Orbit (`IA_DIVE_Orbit`), zoom, back, exit — always.
3. **`IA_DIVE_ContextMenu`** / `HandleContextMenuRequested` → Focus, Isolate at cursor pick.
4. **`IA_DIVE_SetMode_Physical`** / `SetInteractionMode(Physical)` → **`IA_DIVE_PrimaryAction`** drives pawn GRIP grab (and today still tries `IDIVEProxyDrive` first).

## Editor

**DIVE Scan Device** — validates anchors on the device actor **and attached children**, Catalog / Bindings (`SectionId`, `PrimaryActionIndex`, MatchValues / AnyPrimitive), equal-specificity primary overlaps (warning; earlier binding wins), PartId→anchor coverage, shape pick-channel Block, exclusions, missing hover overlay.

**DIVE Dump Device** (same menu) or console in PIE — dump implementation in `DIVEUncooked`; console from `DIVERuntimeDev`:

Automation smoke tests: `DIVE.ContextMenu.DefaultBindings`, `DIVE.Actions.BindingResolve`, `DIVE.Actions.CollectMatchingPrimitives`, `DIVE.Actions.ComponentNameMatch`, `DIVE.Actions.ExecutionWorld`, `DIVE.Inspectable.IsDataValid`, `DIVE.Drive.Mapping`, `DIVE.Drive.BuiltInActions`, `DIVE.ProxyDrive.ResolveHierarchy`, `DIVE.PawnPhysicalDrive.Resolve`, `DIVE.Player.IsDataValid`, `DIVE.Session.Lifecycle` (Editor / PIE; not run in `UnrealEditor-Cmd` commandlet).

| Smoke | What it covers |
|-------|----------------|
| `DIVE.ContextMenu.DefaultBindings` | Native CDO Bindings empty; instance seeds public Focus/Admin; register instances foreign private actions |
| `DIVE.Actions.BindingResolve` | Primary index, specificity, continuous/notify, Focus `CanExecute` |
| `DIVE.Actions.ExecutionWorld` | Catalog action world injection |
| `DIVE.Actions.CollectMatchingPrimitives` | CDO Collect empty; live owner + tagged sphere when a world exists |
| `DIVE.Actions.ComponentNameMatch` | Name normalize / FocusId is not a name match |
| `DIVE.Inspectable.IsDataValid` | Authored Standard+Admin valid; duplicate SectionId fails; live Inspectable Valid; Add Admin Defaults restores BuiltIn.Admin |
| `DIVE.Drive.Mapping` | Camera-basis angle / travel helpers |
| `DIVE.Drive.BuiltInActions` | Rotary/Threaded begin-update-cancel on a live primitive |
| `DIVE.ProxyDrive.ResolveHierarchy` | Registry on root resolves a child-actor hit; N>1 registries → null |
| `DIVE.PawnPhysicalDrive.Resolve` | Null pawn; N>1 unnamed → null; named match (when a live world exists). Production resolve requires **exactly one** implementor |
| `DIVE.Player.IsDataValid` | CDO valid; non-Pawn owner invalid (live world) |
| `DIVE.Session.Lifecycle` | Live world + local PC: Begin → active → End. Skips without PIE/PC (no `CreateWorld`) |

## Dependencies

| Module | ACTS | GRIP | MESS |
|--------|------|------|------|
| **DIVERuntime** | no | no | no |
| **DIVEGRIPBridge** (sibling plugin) | no | yes (§7 bridge) | no |
| **DIVERuntimeDev** | no | UBT enable check (§6.2.9) | no |

**DIVERuntime** does not link other gameplay plugins. **DIVEGRIPBridge** is a **separate sibling plugin** (`Plugins/DIVEGRIPBridge/`) for Physical-mode pawn grab: enable it in the host `.uproject` alongside DIVE. It **links GRIP only when GraspRigidbodyInertialPhysics is enabled** for the target (`ProjectDescriptor` / `Plugins.ReadAvailablePlugins` in `DIVEGRIPBridge.Build.cs` and `DIVERuntimeDev.Build.cs`). Unlisted project plugins still follow `IsEnabledByDefault` (GRIP is not required in the host `.uproject`). Without GRIP the bridge compiles as a no-op stub. DIVE `.uplugin` must **not** depend on `DIVEGRIPBridge` (cycle) or Optional GRIP (GRIP is owned by the bridge / host). No Enhanced Input assets or `BindKey` in production Runtime modules.

**Pawn physical drive:** `IDIVEPawnPhysicalDrive` is cursor-pull (backend ticks / reads cursor). The session does not push `ScreenDelta` to the pawn drive. Production resolve requires **exactly one** implementor (N>1 unnamed → nullptr). Pawn-drive drag does not broadcast `OnInteractionValueChanged` / value HUD. Device `IDIVEProxyDrive` (when implemented) receives deltas; `UDIVEProxyDriveForwardAction` polls `GetProxyDriveNormalizedValue` at Begin and after each delta; rotary/threaded call `NotifyInteractionValue` (`FDIVEInteractionValue`); proxy forward uses `NotifyValueChanged` (Normalized only).

**Diagnostics:** `DIVE.DumpDevice` / `DIVE.DumpAll` live in **`DIVEUncooked`** (editor menu + file write). Console commands are registered from **`DIVERuntimeDev`**. `DIVEUnrealEditor` does not link RuntimeDev.

Normative principles: [`Docs/Plugin_Architecture_Principles.md`](../../../Docs/Plugin_Architecture_Principles.md).
