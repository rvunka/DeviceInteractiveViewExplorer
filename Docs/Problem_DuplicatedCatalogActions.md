# Context-menu catalog: ActionSets + Roles

> **Audience:** device authors, DIVE maintainers.  
> **Status:** historical — Role-based catalog (v0.7). **Superseded** by object-based actions in v0.8 (`UDIVEDeviceAction` + `UDIVEActionCatalogAsset`). See [`Audit_DeviceActions_Architecture.md`](Audit_DeviceActions_Architecture.md) and [`QUICKSTART.md`](QUICKSTART.md) §1.  
> **Related:** `QUICKSTART.md` §1 (custom context menu), `DeviceInteractionModel.md`, `ARCHITECTURE.md`.

---

## Problem (historical)

Custom in-session context menu rows used to be authored as **`PickContextMenuByComponent`**: a map from each part key → full action list.

Many devices have **many pickable parts that share the same custom operation** (dozens of bolts that all expose **Unscrew**). Authors repeated the same row data once per part. Cost grew with part count, not with the number of distinct operations.

A shared **Action Definition DataAsset** was tried and **rejected**: it reduced drift of ActionId/label but still required N map entries, added a second source of truth, and did not scale membership.

---

## Solution: ActionSet + Role

Authoring on `UDIVEInspectableComponent` is now two layers:

| Layer | Question | Authoring |
|-------|----------|-----------|
| **ActionSet** (`PickActionSets`) | Which operations / labels / primary / toggle? | Stated **once** per operation set |
| **Role** (`PickContextMenuRoles`) | Which parts receive that set? | Membership rule (tag / name / PartId) |
| **CatalogKey** (runtime) | Which part was picked? | See Match modes below |
| **Handler** | What does the operation do? | `IDIVEDeviceActionHandler` (unchanged) |

### Example — 40 bolts → Unscrew

1. Tag each bolt mesh with `DIVE.Bolt` (Components → Component Tags).
2. **Pick Action Sets:** `SetId=UnscrewOps`, row `ActionId=Unscrew`, optional Primary.
3. **Pick Context Menu Roles:** `RoleId=Bolts`, Match=**Component Tag**, Values=`DIVE.Bolt`, ActionSetId=`UnscrewOps`.

Add another bolt = tag it (or duplicate a tagged mesh). Change label/toggle = edit one ActionSet.

### One-off parts

Role Match=**Component Name**, Values=`CoverA`, with its own ActionSet — no separate override map.

### Match modes

| Mode | Membership | CatalogKey passed to handler |
|------|------------|------------------------------|
| **Component Tag** | `UActorComponent::ComponentTags` — preferred for identical parts | Primitive `FName` |
| **Component Name** | Components-panel name (exact / normalized) | Matching **MatchValue** (authored; stable for `Is_*` / migration) |
| **Part Id** | Semantic / anchor PartId | That **PartId** |

When several Roles match the same pick, the higher **Priority** wins; equal Priority → first in array + validation warning.

### Dispatch (unchanged)

`HandleDeviceAction(CatalogKey, ActionId, Target, bActiveBefore)` — CatalogKey is the **part**, not RoleId / SetId.

### Migration from `PickContextMenuByComponent`

Breaking. For each old map entry: create (or reuse) an ActionSet with the same Actions/Primary, add a Role with Match=Component Name and that key in MatchValues. Deduplicate identical action lists into one shared Set.

---

## Out of scope

- Domain parameters (turns, torque) — device BP / handler, not DIVE catalog.
- Action Definition DataAssets — not used.
- Camera / orbit presets (`UDIVEDeviceDefinitionAsset`) — unrelated.
