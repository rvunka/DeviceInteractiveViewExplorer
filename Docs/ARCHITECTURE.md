# DIVE Architecture (v0.2)

## Layers

```text
┌─────────────────────────────────────────┐
│ ATSEP (game module: ACTS entry, Legacy input) │
├─────────────────────────────────────────┤
│ DIVERuntimeDev (optional, debug only)   │
│  UDIVELegacyKbmInputComponent — BindKey   │
├─────────────────────────────────────────┤
│ DIVERuntime                             │
│  UDIVESessionSubsystem — focus stack      │
│  UDIVEInspectableComponent              │
│  UDIVEAnchorComponent (optional)        │
│  ADIVECameraRig                         │
│  UDIVEDeviceDefinitionAsset             │
├─────────────────────────────────────────┤
│ DIVECore — FDIVEFocusTarget, DIVEHierarchy, types      │
└─────────────────────────────────────────┘
```

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

Input sensitivity: `UDIVEInspectableComponent` (DIVE \| Camera) or Device Definition asset; optional pawn override via Legacy input component.

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
