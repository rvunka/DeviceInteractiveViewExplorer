# DIVE Architecture (v0.7-dev)

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
│ DIVECore — FDIVEFocusTarget, IDIVEProxyDrive, types │
└─────────────────────────────────────────┘
```

## Subsystem scope

**Current:** `UGameInstanceSubsystem` (`UDIVESessionSubsystem`).

One active DIVE session per game instance. Input components guard with `IsLocallyControlled()`.

**Lifecycle note:** `UDIVEInputComponent::EndPlay` ends an active session (`EndSession(Forced)`) for the locally controlled pawn. This is intentional for single-player / one local pawn; multi-pawn or listen-server setups may need a different session owner policy.

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
- **Physical:** `TryBeginProxyDrive*` via `HandlePrimaryAction*`; **Default:** catalog `Primary Action Id` or anchor focus; hover overlay on pick

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

In-session menu at cursor — **not** ACTS. Built-in: **Focus**, **Isolate**; administrator **Simulate Physics / Delete Mesh** only when `UDIVEInspectableComponent::bEnableAdminContextMenuEntries` is true (default **false**) and **not** in Shipping builds. Custom rows from **`PickContextMenuByComponent`** → prefer **`UDIVEDeviceActionDefinition`** (shared `ActionId`) + **`IDIVEDeviceActionHandler`**; legacy free `ActionId` FName / **`Handle_{Key}_{ActionId}`** fallback. **`Primary Action Id`** = resolved id via **`HandlePrimaryActionPressed`**. Hover: **DIVE | Pick | Hover**. Exclusions: **Pick Interaction Exclusions**. **`UDIVEContextMenuUIComponent`** on player character. Focus stack undo: `IA_DIVE_Back`. **No menu on anchor pick** — primary focuses anchor in Default mode when no `Primary Action Id`.

## Device interaction (direct manipulation)

| Intent | Mechanism |
|--------|-----------|
| Focus / isolate / back | Context menu or `HandleFocusUnderCursor` |
| Custom device action | Context menu / primary → **Definition asset** + **`IDIVEDeviceActionHandler`** (legacy FName / `Handle_*`) |
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

**DIVE Scan Device** — logs anchors and focus warnings.

Automation smoke tests: `DIVE.ContextMenu.BuiltInEntries`, `DIVE.PawnPhysicalDrive.Resolve` (Editor / PIE; not run in `UnrealEditor-Cmd` commandlet).

## Dependencies

| Module | ACTS | GRIP | MESS |
|--------|------|------|------|
| **DIVERuntime** | no | no | no |
| **DIVEGRIPBridge** (sibling plugin) | no | yes (§7 bridge) | no |
| **DIVERuntimeDev** | no | co-location only (§3.3 PIE) | no |

**DIVERuntime** does not link other gameplay plugins. **DIVEGRIPBridge** is a **separate sibling plugin** (`Plugins/DIVEGRIPBridge/`) for Physical-mode pawn grab: enable it in the host `.uproject` alongside DIVE. It **links GRIP only when GraspRigidbodyInertialPhysics is enabled** for the target (see `DIVEGRIPBridge.Build.cs`). Without GRIP the bridge compiles as a no-op stub. DIVE `.uplugin` must **not** depend on `DIVEGRIPBridge` (cycle) or Optional GRIP (GRIP is owned by the bridge / host; `DIVERuntimeDev` co-locates via Build.cs only). No Enhanced Input assets or `BindKey` in production Runtime modules.

**Pawn physical drive:** `IDIVEPawnPhysicalDrive` is cursor-pull (backend ticks / reads cursor). The session does not push `ScreenDelta` to the pawn bridge (device `IDIVEProxyDrive` still receives deltas).

**Diagnostics:** `DIVE.DumpDevice` / `DIVE.DumpAll` live in **`DIVERuntimeDev`** (PIE / Editor). Not registered from Shipping `DIVERuntime`.

Normative principles: [`Docs/Plugin_Architecture_Principles.md`](../../../Docs/Plugin_Architecture_Principles.md).
