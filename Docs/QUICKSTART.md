# DIVE QUICKSTART

## 1. Device setup

On the **device actor** (the thing being inspected):

| Component | Purpose |
|-----------|---------|
| `UDIVEInspectableComponent` | Session entry, camera, **context menu catalog** |
| `UDIVEAnchorComponent` | Optional semantic parts / focus points |
| Pickable meshes | Static/skeletal meshes on the actor |

### Custom context menu row (example: Unscrew on screw)

**Only on the device actor.** No Events, no delegate, no Switch.

1. **DIVEInspectable** → **Pick Context Menu By Component** → add key `Screw1`, row `ActionId` = `Unscrew`, `DisplayName` = menu label.
2. **My Blueprint → Functions → +** → name **`Handle_Screw1_Unscrew`** (pattern: `Handle_{map key}_{ActionId}`).
3. Optional input: **`TargetComponent`** (Primitive Component) = picked mesh.
4. Function body: your logic (`UnscrewByRef`, etc.). Compile.

**Toggle row (`*` when active):** enable **`Toggle Active Suffix`** on the catalog row. On the device actor add either:

- bool **variable** `Is_Screw1_Unscrew` (flip it in `Handle_Screw1_Unscrew`), or
- pure bool **function** `Is_Screw1_Unscrew` that returns the state.

`*` is evaluated when the menu **opens** — after click close the menu and open again (RMB) to refresh the label.

Done. Clicking **Unscrew** in session calls that function automatically.

### Who does what (two actors)

```text
Player character (e.g. BP_FirstPersonCharacter)          Device actor (e.g. BP_MyDevice)
├─ DIVE Input                                           ├─ DIVE Inspectable  ← catalog here
├─ DIVE Context Menu UI  ← draws menu on screen        └─ Handle_Screw1_Unscrew  ← your logic here
└─ (GRIP / bridge as needed)
```

**Player character** — menu open + UI. Already handled by the plugin if components and IMC are set up (§4). **You do not add Unscrew here.**

**Device actor** — catalog + `Handle_*` functions.

### Menu open path (already in plugin — do not override for Unscrew)

```text
RMB
 → IA_DIVE_ContextMenu (Enhanced Input on player character)
 → DIVE Input::HandleContextMenuRequested()     ← C++ entry, no BP wiring for device actions
 → Session subsystem builds rows, stores pick
 → DIVE Context Menu UI shows widget at cursor
```

`HandleContextMenuRequested` is **not** your hook for device actions. It only opens/toggles the menu. It is called from **DIVE Input** on the **player character**, not from the device Blueprint.

`DIVE Context Menu UI` on the player character only **shows** the widget; it does not run `Handle_Screw1_Unscrew`.

### Menu click path (your `Handle_*` function)

```text
Click "Unscrew"
 → UI → Session subsystem::ExecuteContextMenuAction
 → DIVEInspectable calls Handle_Screw1_Unscrew on the device actor
```

Built-in rows (Focus, Isolate, Simulate Physics, Delete Mesh) are handled inside the plugin, not via `Handle_*`.

---

## 1b. Anchors (optional)

| Field | Purpose |
|-------|---------|
| `PartId` | Semantic id; can match `DefaultStartFocusId` |
| `DisplayName` | UI label |
| Transform / view rotation | Authored camera point |

**Session marker** — сфера в DIVE-сессии (видна только в PIE/игре, не в редакторе). Цвет и прозрачность задаются **материалом**, не свойствами компонента.

### Где менять mesh и material

**На всё устройство (defaults):**

1. World Outliner → выбери **device actor** (не player character).
2. Details → компонент **`DIVE Inspectable`**.
3. Секция **`DIVE | Anchor`**:
   - **Default Anchor Marker Mesh** — меш (по умолчанию engine `Sphere`).
   - **Default Anchor Marker Material** — материал или material instance (цвет и alpha в ассете).
   - **Default Anchor Marker Scale** — размер.

**На один anchor:**

1. На том же device actor раскрой дочерний компонент **`DIVE Anchor`**.
2. Секция **`DIVE | Marker`**:
   - **Marker Mesh Override** / **Marker Material Override** — только для этого anchor (пусто = defaults с Inspectable).
   - **Marker Scale** — локально (если оставить значение по умолчанию плагина, берётся с Inspectable).
   - **Show Session Marker** — вкл/выкл сферу.

Пример: Material Instance с **Blend Mode = Translucent**, нужный цвет и opacity в самом MI.

Marker collision: **query-only** на DIVE pick channel — не блокирует physics/GRIP. GRIP aim trace пропускает тег `DIVE.AnchorMarker`.

---

## 2. Physical controls

**Device DOF** (sliders, doors, knobs) — constraints + game state on device prefabs:

1. `SetInteractionMode(Physical)` (PIE: **Left Alt** cycles Default ↔ Physical).
2. Register via `IDIVEDeviceControlRegistry` or implement `IDIVEProxyDrive` on a **control component** (not raw mesh).

**Generic simulating-mesh drag** — pawn bridge (no device component):

1. Player character: `UGRIPHandComponent` (`GrabPolicy = AllowSimulatingPhysics`) + `UDIVEGRIPBridgeComponent`.
2. Admin context menu → **Simulate Physics** on a mesh.
3. Physical mode → LMB drag moves the body via GRIP PD. While dragging, **hold R** + mouse move rotates the grabbed body.

See `DeviceInteractionModel.md` §6–§7 and `Source/DIVEGRIPBridge/README.md`.

### Camera sensitivity

On `UDIVEInspectableComponent` → **DIVE | Camera**, or shared `UDIVEDeviceDefinitionAsset`.

## 3. ACTS entry (optional)

On `UACTSInteractableComponent`: **ActionId** `DIVE::kActionOpenDIVE` (`OpenDIVE`) → `RequestSession()` in game code.

## 4. Player character input

Components on **the player character** (not the device):

```text
UDIVEInputComponent              ← input handlers (including open menu)
UDIVEContextMenuUIComponent      ← menu widget host
UGRIPHandComponent               (GrabPolicy = AllowSimulatingPhysics)
UDIVEGRIPBridgeComponent         (generic Physical drag)
```

`UDIVEInputComponent` auto-finds `UDIVEContextMenuUIComponent` on the same actor. Optional PIE-only: **`UDIVELegacyKbmInputComponent`** (`DIVERuntimeDev`).

### Enhanced Input (host Content)

Map once on the player character's IMC — **not** on the device:

| Input Action | Call on **DIVE Input** (player character) |
|--------------|-------------------------------------------|
| `IA_DIVE_Orbit` Started / Completed / Triggered | `HandleOrbitPressed` / `Released` / `HandleOrbitDelta` |
| `IA_DIVE_Zoom` | `HandleZoomIn` / `HandleZoomOut` |
| `IA_DIVE_PrimaryAction` | `HandlePrimaryActionPressed` / `Released` |
| `IA_DIVE_FocusTarget` | `HandleFocusUnderCursor` |
| `IA_DIVE_ContextMenu` | `HandleContextMenuRequested` |
| `IA_DIVE_SetMode_Physical` / `_Default` | `SetInteractionMode` |
| `IA_DIVE_Back` / `IA_DIVE_Exit` | `HandleNavigateBack` / `HandleExitSession` |

Remove unused `IA_DIVE_ExecuteOperation` from Content / IMC if present.

### Legacy KBM (DIVERuntimeDev — dev only)

| Key | Action |
|-----|--------|
| MMB + drag | Orbit |
| Wheel | Zoom |
| **RMB** | Context menu (`HandleContextMenuRequested` via Legacy → DIVE Input) |
| **LMB** (menu open, outside panel) | Dismiss context menu |
| **G** | Focus under cursor |
| **Left Alt** | Cycle Default ↔ Physical |
| LMB | Primary action |
| Ctrl+Z | Focus stack back |
| Backspace | Exit session |
| I | Isolate |

## 5. Editor

**DIVE device scan** (editor utility on selected actor with `UDIVEInspectableComponent`) — validates anchors, catalog, duplicate action ids.

## 6. Compliance

Runtime input: `UDIVEInputComponent` **Handle\*** only (no `BindKey` in `DIVERuntime`). Legacy KBM in `DIVERuntimeDev` for PIE.
