# DIVE Quickstart

## 1. Device actor setup

Add to your device Blueprint or C++ actor:

- `UDIVEInspectableComponent` (session host)
- **Any number of mesh components** on the same actor — pick/focus works without extra setup
- **Optional:** `UDIVEAnchorComponent` for semantic AOI, operations, authored camera points (empty scene node OK)

### Mesh focus (default)

LMB in session → trace → orbit camera around **hit mesh bounds center**.

Pick filters on `UDIVEInspectableComponent`:

| Property | Default | Purpose |
|----------|---------|---------|
| `SkipComponentTag` | `DIVE.Skip` | Ignore decorated / collision-only meshes |
| `MinPickBoundsRadius` | `0` | Skip tiny meshes (set e.g. `1.0` to reduce noise) |

### Anchor focus (viewpoint + scenario ops)

LMB on the **session marker sphere** (or `FocusAnchor(PartId)`) → camera moves to the anchor **transform** (location + rotation). MMB/zoom adjust view **in place** at the anchor.

LMB on a **device mesh** (even under an anchor in the hierarchy) → **orbit** around mesh center, not anchor viewpoint.

| Property | Purpose |
|----------|---------|
| `PartId` | Semantic id; can match `DefaultStartFocusId` for session start |
| `DisplayName` | UI label |
| `OperationIds` | Scenario operation ids for this AOI (inspect, demount — not physical knobs) |
| `bShowSessionMarker` / `MarkerScale` | Pick sphere while navigating (hidden on the active anchor viewpoint) |
| `bShowViewDirection` / `ViewDirectionScale` | View-direction arrow in **editor only** (never shown in PIE/game) |
| Component rotation | View direction when `bUseComponentRotationForView` |
| `ViewRotationOverride` | Fixed rotation when component rotation is disabled |

### Physical controls (proxy drive)

Sliders, doors, knobs, and switches are **device prefabs** (constraints + game state), not DIVE anchors.

For monitor sessions, implement **`IDIVEProxyDrive`** (DIVECore) on the control actor or component. LMB on a proxy-drive hit starts drag; release commits. If nothing implements the interface, LMB behaves as pick/focus only.

See `Docs/DeviceInteractionModel.md` for the full ATSEP / GRIP composition model.

### Camera sensitivity

On `UDIVEInspectableComponent` → **DIVE | Camera**:

| Property | Default | Purpose |
|----------|---------|---------|
| `OrbitSensitivity` | `2.5` | MMB orbit speed |
| `ZoomSensitivity` | `40` | Wheel zoom step (units per tick) |
| `DefaultOrbitDistance` | `200` | Initial orbit radius when focusing device root |
| `bUseDeviceDefinitionSettings` | `false` | Read orbit distance + sensitivity from linked **Device Definition** asset |

Optional pawn override: **DIVE Input** → enable `bOverrideCameraSensitivity`.

### Session start focus

On `UDIVEInspectableComponent` → **DIVE | View**:

| Property | Purpose |
|----------|---------|
| `DefaultStartFocusId` | Anchor `PartId` **or** pickable mesh **component name** — camera blends here when the session starts |

## 2. ACTS entry (optional)

On `UACTSInteractableComponent`, add action:

- **ActionId:** `OpenDIVE` (see `DIVE::kActionOpenDIVE`)
- **Label:** «Детальный осмотр» / «ТОиР»

In game code, subscribe to `UACTSInteractionComponent::OnFocusedActionExecuted` and call `UDIVEInspectableComponent::RequestSession()` when `ActionId == OpenDIVE`.

## 3. Pawn input

Add to your **pawn**:

- **`UDIVEInputComponent`** (required) — session presentation + `Handle*` API for Enhanced Input.

Optional PIE demo: **`UDIVELegacyKbmInputComponent`** (`DIVERuntimeDev`) — `BindKey` forwards to **DIVE Input**.

### Enhanced Input (production, game module)

| Input Action | Call on **DIVE Input** |
|--------------|------------------------|
| `IA_DIVE_Orbit` Started / Completed | `HandleOrbitPressed` / `HandleOrbitReleased` |
| `IA_DIVE_Orbit` Triggered (Axis2D) | `HandleOrbitDelta` |
| `IA_DIVE_Zoom` | `HandleZoomIn` / `HandleZoomOut` |
| `IA_DIVE_Select` Started / Completed | `HandleSelectPressed` / `HandleSelectReleased` |
| `IA_DIVE_ExecuteOperation` Started / Completed | `HandleOperationExecutePressed` / `HandleOperationExecuteReleased` |
| `IA_DIVE_Back` | `HandleNavigateBack` (camera / focus undo; bind Ctrl+Z in IMC) |
| `IA_DIVE_Exit` | `HandleExitSession` (bind Backspace in IMC) |

### Default KBM (DIVERuntimeDev, legacy BindKey only)

| Default key | Forwards to **DIVE Input** |
|-------------|----------------------------|
| Middle mouse (hold + drag) | `HandleOrbitPressed` / `HandleOrbitReleased` + tick orbit |
| Mouse wheel up / down | `HandleZoomIn` / `HandleZoomOut` |
| Left mouse | `HandleSelectPressed` / `HandleSelectReleased` (proxy drive or pick/focus) |
| F | `HandleOperationExecutePressed` / `HandleOperationExecuteReleased` |
| Ctrl+Z | `HandleNavigateBack` |
| Backspace | `HandleExitSession` |
| I | `HandleToggleIsolate` |

Enhanced Input: isolate is not bound by default — call `ToggleIsolateFocused()` from UI/Blueprint or map a custom action.

## 4. Operations wiring

Implement `OnOperationRequested` on `UDIVEInspectableComponent`. Request includes `FocusTarget` (mesh/anchor) and `SemanticPartId` when resolved.

## 5. Test in PIE

1. Device with meshes only → `RequestSession()` → camera blends from player view → LMB on mesh → orbit reframes on mesh center.
2. Set `DefaultStartFocusId` to a mesh component name or anchor `PartId` → session opens focused on that target.
3. Add anchors + markers → LMB on marker → camera moves to anchor viewpoint.
4. Ctrl+Z walks **focus stack** (previous camera focus); at root it has no effect.
5. Backspace exits the session.
6. `ToggleIsolateFocused()` hides other device meshes (keeps focused subtree + attach ancestors).
