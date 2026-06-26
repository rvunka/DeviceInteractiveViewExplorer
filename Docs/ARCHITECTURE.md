# DIVE Architecture (v0.2)

## Layers

```text
┌─────────────────────────────────────────┐
│ ATSEP (game module: ACTS entry, EI)      │
├─────────────────────────────────────────┤
│ DIVERuntimeDev (optional, debug only)   │
│  UDIVELegacyKbmInputComponent — BindKey │
├─────────────────────────────────────────┤
│ DIVERuntime                             │
│  UDIVESessionSubsystem — focus stack      │
│  UDIVEInputComponent — session input      │
│  UDIVEInspectableComponent              │
│  UDIVEAnchorComponent (optional)        │
│  ADIVECameraRig                         │
│  UDIVEDeviceDefinitionAsset             │
├─────────────────────────────────────────┤
│ DIVECore — FDIVEFocusTarget, DIVEHierarchy, types      │
└─────────────────────────────────────────┘
```

## Input

| Component | Module | Role |
|-----------|--------|------|
| **DIVE Input** | `DIVERuntime` | `HandleOrbit*` / `HandleZoom*` / `HandleSelect*` / `HandleNavigateBack` / `HandleExitSession` — target for Enhanced Input |
| **Legacy KBM** | `DIVERuntimeDev` | `BindKey` only → forwards to **DIVE Input** |

Recommended pawn stack: **`UDIVEInputComponent`** (+ optional **Legacy KBM** for PIE).

### Enhanced Input (game module)

| Input Action | Event | Call |
|--------------|-------|------|
| `IA_DIVE_Orbit` | Started / Completed | `HandleOrbitPressed` / `HandleOrbitReleased` (legacy MMB uses tick poll) |
| `IA_DIVE_Orbit` | Triggered (Axis2D) | `HandleOrbitDelta` — **do not combine** with Pressed/Released on the same action |
| `IA_DIVE_Zoom` | Triggered | `HandleZoomIn` / `HandleZoomOut` |
| `IA_DIVE_Select` | Started | `HandleSelectPressed` |
| `IA_DIVE_Back` | Started | `HandleNavigateBack` |
| `IA_DIVE_Exit` | Started | `HandleExitSession` |

## Focus vs semantic

| Layer | Responsibility |
|-------|----------------|
| **Focus** | Camera pivot / viewpoint, hover/focus highlight, focus stack, isolate |
| **Semantic** | Anchor registry, `PartId`, operations, optional `SemanticPartId` on mesh pick |

Mesh pick is **primary**. Anchors are **optional**.

## Camera modes

| Focus kind | Camera behaviour |
|------------|------------------|
| `DeviceRoot` | Orbit around device bounds center |
| `Primitive` | Orbit around hit mesh bounds center |
| `Anchor` | Viewpoint at anchor location + rotation; MMB rotates in place |

Input sensitivity: `UDIVEInspectableComponent` (DIVE \| Camera) or Device Definition asset; optional override on **DIVE Input** (`bOverrideCameraSensitivity`).

## Session flow

1. `RequestSession()` → `TryBeginSession` → `BuildSemanticRegistry()` (anchors only).
2. Focus stack initialized with `DeviceRoot`; camera orbits device bounds.
3. LMB → pick: marker tag → `Anchor` viewpoint; otherwise `Primitive` orbit (+ optional semantic id).
4. Backspace → pop focus stack; at root → `EndSession`.
5. `ToggleIsolateFocused()` — explicit; not tied to LMB.

## Dependencies

DIVE **must not** link ACTS, GRIP, or MESS in **DIVERuntime**. No Enhanced Input in plugin modules.

## TBD

- World dim (whole level, not device mesh isolate)
- Auto-discovery modes (`TaggedChildren`, `AutoWithRules`)

Full contract: `Project_docs/DIVE_Plugin_Design.md`
