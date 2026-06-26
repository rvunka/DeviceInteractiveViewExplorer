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

### Anchor focus (viewpoint)

LMB on the **session marker sphere** (or `FocusAnchor(PartId)`) → camera moves to the anchor **transform** (location + rotation). MMB/zoom adjust view **in place** at the anchor.

LMB on a **device mesh** (even under an anchor in the hierarchy) → **orbit** around mesh center, not anchor viewpoint.

| Property | Purpose |
|----------|---------|
| `PartId` | Semantic id; must match `DefaultViewAnchorId` if used |
| `DisplayName` | UI label |
| `OperationIds` | Operation ids for this AOI |
| `bShowSessionMarker` / `MarkerScale` | Pick sphere while navigating (hidden on the active anchor viewpoint) |
| `bShowViewDirection` / `ViewDirectionScale` | View-direction arrow in **editor only** (never shown in PIE/game) |
| Component rotation | View direction when `bUseComponentRotationForView` |
| `ViewRotationOverride` | Fixed rotation when component rotation is disabled |

### Camera sensitivity

On `UDIVEInspectableComponent` → **DIVE | Camera**:

| Property | Default | Purpose |
|----------|---------|---------|
| `OrbitSensitivity` | `2.5` | MMB orbit speed |
| `ZoomSensitivity` | `40` | Wheel zoom step (units per tick) |
| `DefaultOrbitDistance` | `200` | Initial orbit radius when focusing device root |
| `bUseDeviceDefinitionSettings` | `false` | Read orbit distance + sensitivity from linked **Device Definition** asset |

Optional pawn override: **DIVE Input** → enable `bOverrideCameraSensitivity`.

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
| `IA_DIVE_Select` | `HandleSelectPressed` |
| `IA_DIVE_Back` | `HandleNavigateBack` |
| `IA_DIVE_Exit` | `HandleExitSession` |

### Default KBM (DIVERuntimeDev, legacy BindKey only)

| Default key | Forwards to **DIVE Input** |
|-------------|----------------------------|
| Middle mouse (hold + drag) | `HandleOrbitPressed` / `HandleOrbitReleased` + tick orbit |
| Mouse wheel up / down | `HandleZoomIn` / `HandleZoomOut` |
| Left mouse | `HandleSelectPressed` |
| Backspace | `HandleNavigateBack` |
| Escape | `HandleExitSession` |

**Isolate** is not bound by default. From Blueprint/UI: `GetGameInstance` → `UDIVESessionSubsystem` → `ToggleIsolateFocused()`.

## 4. Operations wiring

Implement `OnOperationRequested` on `UDIVEInspectableComponent`. Request includes `FocusTarget` (mesh/anchor) and `SemanticPartId` when resolved.

## 5. Highlight (Content)

| Stencil | Meaning |
|---------|---------|
| `1` | Hovered mesh / anchor marker |
| `2` | Focused mesh |
| `3` | Reserved for anchor markers (custom depth only on hover/focus) |

Add post-process outline reading custom depth.

## 6. Test in PIE

1. Device with meshes only → `RequestSession()` → LMB on mesh → orbit reframes on mesh center.
2. Add anchors + markers → LMB on marker → camera snaps to anchor viewpoint.
3. Backspace walks **focus stack**; at root → session ends.
4. `ToggleIsolateFocused()` hides other device meshes (keeps focused subtree + attach ancestors).
