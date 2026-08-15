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
│  UDIVEGRIPBridgeComponent — pawn physical │
│  drive via GRIP Hand API                │
├─────────────────────────────────────────┤
│ DIVERuntime                             │
│  UDIVESessionSubsystem — focus stack      │
│  UDIVEInputComponent — session input      │
│  UDIVEContextMenuUIComponent — context menu │
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

Input components guard with `IsLocallyControlled()`. Session ends on world tear-down (`OnWorldBeginTearDown` + `Deinitialize`) and on `UDIVEInputComponent::EndPlay` for the locally controlled pawn.

**Input restore note:** session end approximates prior mode via `EDIVEPreservedInputMode` (GameAndUI only if both cursor and click events were on). PlayerController does not expose the previous `FInputMode` directly.

## Input

| Component | Module | Role |
|-----------|--------|------|
| **DIVE Input** | `DIVERuntime` | Semantic `Handle*` API for Enhanced Input (no `EKeys` in runtime) |
| **DIVE Context Menu UI** | `DIVERuntime` | In-session menu widget; auto-found by Input |
| **Legacy KBM** | `DIVERuntimeDev` | `BindKey` only → forwards to **DIVE Input** (PIE dev) |

Recommended pawn stack: **`UDIVEInputComponent`** + **`UDIVEContextMenuUIComponent`** (+ optional **Legacy KBM** for PIE).

**Contract:** physical keys → `UInputAction` in host Content → Enhanced Input on **Pawn / PlayerController** → plugin `Handle*`. See `../../../Docs/Plugin_Architecture_Principles.md` §4 and **`Docs/DeviceInteractionModel.md` §4**.

### Session chrome (always available)

Orbit, zoom, focus undo, exit, and context menu work in **every** interaction mode.

Zoom steps scale with orbit distance by default; focusing a primitive fits distance to its
bounds and raises a soft near floor from the part radius (`DIVE | Camera` on Inspectable).

Pick accepts visible meshes and **shape collision** volumes (including Hidden-in-Game shapes);
hidden meshes need tag `DIVE.PickProxy`.

### Interaction mode

`EDIVESessionInteractionMode` in **DIVECore**: **Default** | **Physical**.

- Resets to **Default** on session start/end
- **Physical:** `TryBeginProxyDrive*` via `HandlePrimaryAction*`; **Default:** binding `PrimaryActionIndex` (when set); hover overlay on pick; explicit focus via `HandleFocusUnderCursor` / context menu

### Enhanced Input (host Content)

| Input Action | Event | Call |
|--------------|-------|------|
| `IA_DIVE_Orbit` | Started / Completed / Triggered | `HandleOrbit*` |
| `IA_DIVE_Zoom` | Triggered | `HandleZoomIn` / `HandleZoomOut` |
| `IA_DIVE_PrimaryAction` | Started / Completed | `HandlePrimaryActionPressed` / `Released` |
| `IA_DIVE_FocusTarget` | Started | `HandleFocusUnderCursor` |
| `IA_DIVE_Back` / `IA_DIVE_Exit` | Started | `HandleNavigateBack` / `HandleExitSession` |
| `IA_DIVE_SetMode_*` | Started | `SetInteractionMode` |
| `IA_DIVE_ContextMenu` | Started | `HandleContextMenuRequested` |

Legacy PIE (`UDIVELegacyKbmInputComponent`): **RMB** = context menu, **G** = focus, **Tab** = cycle mode, **LMB** = `HandlePrimaryAction*` (see table in `QUICKSTART.md` §4).

### Context menu

In-session menu at cursor — **not** ACTS. Focus / Isolate / Admin are normal **Bindings** on `UDIVEInspectableComponent` (editable/removable; Admin hidden in Shipping). Device ops from **Action Catalog** (Content Browser → **DIVE → Action Catalog**) and/or extra component Bindings. Create action / continuous / condition Blueprints via **DIVE → Device Action / Continuous Device Action / Action Condition**. Optional section **Header** labels above separators. **PrimaryActionIndex** on a matching binding drives **`HandlePrimaryActionPressed`** (winner = most specific Match Mode: Name > PartId > Tag > Any; equal specificity keeps the earlier binding in the Bindings array; menu/section order follows those arrays). Binding Details shows **Targets: N** for the current Match query (Select highlights matching primitives on a placed device). Hover: **DIVE | Pick | Hover**. Exclusions: **Pick Interaction Exclusions**. **`UDIVEContextMenuUIComponent`** on player character. Focus stack undo: `IA_DIVE_Back`. Pick resolves to **primitives** (or device root); anchors are viewpoints / PartIds, not a separate primary-pick focus path.

## Device interaction (direct manipulation)

| Intent | Mechanism |
|--------|-----------|
| Focus / isolate / back | Context menu or `HandleFocusUnderCursor` |
| Custom device action | Context menu / primary → **Action Catalog / Bindings** + **`UDIVEDeviceAction`** |
| Default-mode hover | **DIVE \| Pick \| Hover** — `Hover Overlay Material` + optional `Pick Hover Overlay By Component` |
| Non-interactive meshes | `Pick Interaction Exclusions` or `Skip Component Tag` |
| Door, slider, knob | **Physical** mode + `IDIVEProxyDrive` / registry |
| Cable, grab | GRIP + MESS in host project |

## Proxy drive

Physical panel controls live on the **device**. DIVECore exposes **`IDIVEProxyDrive`** and **`IDIVEDeviceControlRegistry`** for **Physical** mode.

## Anchor (viewpoint only)

`UDIVEAnchorComponent`: `PartId`, `DisplayName`, authored camera viewpoint.
Focus via mesh pick, context menu, or `DefaultStartFocusId`. Optional editor-only
view-direction arrow (`DIVE|Editor`).

## Device isolate

Context menu **Isolate** / **`ToggleIsolateFocused()`** — hides other meshes on the device host, keeps the focused branch visible. Level actors are not modified.

_Future:_ world-level focus dim via custom depth / post-process (not actor hiding).

## Session flow

1. `RequestSession()` → mode **Default**; camera blends to start focus.
2. Orbit (`IA_DIVE_Orbit`), zoom, back, exit — always.
3. **`IA_DIVE_ContextMenu`** / `HandleContextMenuRequested` → Focus, Isolate at cursor pick.
4. **`IA_DIVE_SetMode_Physical`** / `SetInteractionMode(Physical)` → **`IA_DIVE_PrimaryAction`** drives proxy / GRIP.

## Editor

**DIVE Scan Device** — validates anchors on the device actor **and attached children**, Catalog / Bindings (`SectionId`, `PrimaryActionIndex`, MatchValues / AnyPrimitive), equal-specificity primary overlaps (warning; earlier binding wins), PartId→anchor coverage, shape pick-channel Block, exclusions, missing hover overlay.

**DIVE Dump Device** / `DIVE.DumpDevice` / `DIVE.DumpAll` — component + Catalog bindings, sections, per-primitive menu resolution (`DIVERuntimeDev`; not in Shipping).

Automation smoke tests: `DIVE.ContextMenu.DefaultBindings`, `DIVE.Actions.BindingResolve`, `DIVE.Actions.ExecutionWorld`, `DIVE.Actions.CanExecuteGate`, `DIVE.PawnPhysicalDrive.Resolve`, `DIVE.Camera.DefinitionBlendDuration` (Editor / PIE; not run in `UnrealEditor-Cmd` commandlet).

## Dependencies

| Module | ACTS | GRIP | MESS |
|--------|------|------|------|
| **DIVERuntime** | no | no | no |
| **DIVEGRIPBridge** (sibling plugin) | no | yes (§7 bridge) | no |
| **DIVERuntimeDev** | no | co-location only (§3.3 PIE) | no |

**DIVERuntime** does not link other gameplay plugins. **DIVEGRIPBridge** is a **separate sibling plugin** (`Plugins/DIVEGRIPBridge/`) for Physical-mode pawn grab: enable it in the host `.uproject` alongside DIVE. It **links GRIP only when GraspRigidbodyInertialPhysics is enabled** for the target (see `DIVEGRIPBridge.Build.cs`). Without GRIP the bridge compiles as a no-op stub. DIVE `.uplugin` must **not** depend on `DIVEGRIPBridge` (cycle) or Optional GRIP (GRIP is owned by the bridge / host; `DIVERuntimeDev` co-locates via Build.cs only). No Enhanced Input assets or `BindKey` in production Runtime modules.

**Pawn physical drive:** `IDIVEPawnPhysicalDrive` is cursor-pull (backend ticks / reads cursor). The session does not push `ScreenDelta` to the pawn bridge (device `IDIVEProxyDrive` still receives deltas). Pawn-bridge drag does not broadcast `OnInteractionValueChanged` / value HUD.

**Diagnostics:** `DIVE.DumpDevice` / `DIVE.DumpAll` live in **`DIVERuntimeDev`** (PIE / Editor). Not registered from Shipping `DIVERuntime`.

Normative principles: [`Docs/Plugin_Architecture_Principles.md`](../../../Docs/Plugin_Architecture_Principles.md).
