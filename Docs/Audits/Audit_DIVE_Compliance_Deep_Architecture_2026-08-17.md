# Аудит DIVE — соответствие принципам, Pawn-deployment, мёртвый код, упрощение

> **Дата:** 2026-08-17  
> **Плагин:** `DeviceInteractiveViewExplorer` v0.8-dev  
> **Охват:** все модули (`DIVECore`, `DIVERuntime`, `DIVERuntimeDev`, `DIVEUncooked`, `DIVEUnrealEditor`), `.uplugin` / `Build.cs` / `Config/Engine.ini`, все документы плагина, граница sibling `DIVEGRIPBridge`.  
> **Эталоны:** [`Docs/Plugin_Architecture_Principles.md`](../../../../Docs/Plugin_Architecture_Principles.md) (рев. 2026-08-17), [`Docs/Audits/Audit_Pawn_Component_Deployment_ACTS_GRIP_DIVE_2026-08-17.md`](../../../../Docs/Audits/Audit_Pawn_Component_Deployment_ACTS_GRIP_DIVE_2026-08-17.md).  
> **Предыдущие аудиты плагина (не переписывать, сверять статус):** [`Audit_DeviceActions_Architecture.md`](Audit_DeviceActions_Architecture.md), [`Audit_v08_CodeHealth_QoL.md`](Audit_v08_CodeHealth_QoL.md), [`Audit_2026-08-14.md`](Audit_2026-08-14.md).  
> **Код не менялся.** Это доказательный снимок, не план реализации.

Приоритеты: **P0** — ломает контракт / shipping / авторинг без обхода; **P1** — шов, который разъедется или блокирует целевую архитектуру; **P2** — гигиена, QoL, эффективность.  
Confidence: **High** — подтверждено C++ call sites / module graph; **Medium** — reflection/Blueprint surface без C++ callers (нужен Reference Viewer); **Low** — host Content / PIE operational.

---

## 0. Вердикт

DIVE v0.8-dev — **здоровый self-contained плагин** с правильной объектной моделью действий, семантическим `Handle*`, карантином Legacy в `DIVERuntimeDev` и GRIP только в sibling-bridge. Health-pass 2026-08-11/14 закрыл реальные P0 (мир каталога, `CanExecute` на primary, isolation-pick, overlay restore, единый binding-валидатор, WorldSubsystem). **Повторять их как открытые нельзя.**

Главный открытый разрыв — не «сломанный action-model», а **публичная гранулярность развёртывания**:

- принципы §4.4 и Pawn-аудит 2026-08-17 требуют один Feature Root (`UDIVEPlayerComponent`);
- код и QUICKSTART по-прежнему требуют **два authored production-компонента** на Pawn (`UDIVEInputComponent` + `UDIVEContextMenuUIComponent`) плюс optional Bridge и Legacy;
- `UDIVEPlayerComponent` **не существует**.

Это SHOULD/целевая архитектура, а не runtime-баг. MUST-слой input/modules/Bridge в основном соблюдён.

Вторые по важности открытые швы (не закрытые предыдущими проходами):

1. `.uplugin` объявляет Optional GRIP вопреки собственному `ARCHITECTURE.md`.
2. `DIVEPawnPhysicalDriveResolve::FindOnPawn` молча берёт **первый** implementor.
3. Нет обработки late possession без пересоздания Pawn.
4. `IsDataValid` на Inspectable проверяет якоря только на owner-акторе, тогда как runtime/Scan ходят по `ForEachDeviceActor`.
5. Ссылки в README/ARCHITECTURE/QUICKSTART на `Docs/DeviceInteractionModel.md` битые после переноса в `Docs/Additional/`.

Упрощать каркас v0.8 (object actions, catalog, WorldSubsystem, RuntimeDev quarantine, sibling Bridge) **не нужно**. Упрощать надо **authoring surface**, **device-god Inspectable** и несколько параллельных источников истины.

---

## 1. Метод

Прочитаны:

- нормативные документы плагина: `README.md`, `Docs/ARCHITECTURE.md`, `Docs/QUICKSTART.md`, `Docs/Additional/DeviceInteractionModel.md`;
- historical: `Docs/Additional/Problem_DuplicatedCatalogActions.md`;
- предыдущие аудиты в `Docs/Audits/`;
- `.uplugin`, все `*.Build.cs`, `Config/Engine.ini`;
- public headers и реализации пяти модулей (~93 исходника);
- граница `Plugins/DIVEGRIPBridge` только там, где DIVE объявляет контракт (`IDIVEPawnPhysicalDrive`).

Каждый вывод привязан к файлам. Закрытые пункты health-pass сверены с кодом, а не со статусом предыдущего аудита. Blueprint/Content хоста KDP **не** разбирались бинарно: `UFUNCTION` без C++ callers помечены Medium, не «мёртвый код».

---

## 2. Фактическая архитектура (снимок)

```text
Host Content / BP
  IA_DIVE_* · IMC · Pawn wiring · ACTS OnActionExecuted → TryRequestSessionFromActionId
        │
DIVERuntimeDev (DeveloperTool, снимаемый)
  UDIVELegacyKbmInputComponent  BindKey → Handle*
  DIVEGripLegacyDevQuery        PIE wheel glue (DIVE_WITH_GRIP)
  DIVE.Dump* / smoke tests
        │
DIVEUnrealEditor / DIVEUncooked
  Scan · factories · Details · K2 Action Event
        │
Sibling DIVEGRIPBridge (отдельный плагин)
  UDIVEGRIPBridgeComponent : IDIVEPawnPhysicalDrive
        │
DIVERuntime
  UDIVESessionSubsystem (UWorldSubsystem, 1 session / world)
  UDIVEInputComponent + UDIVEContextMenuUIComponent   ← authored pawn stack
  UDIVEInspectableComponent ± UDIVEAnchorComponent    ← device
  ADIVECameraRig · built-in actions · widgets
        │
DIVECore
  types · UDIVEDeviceAction · catalog · IDIVEProxyDrive · IDIVEPawnPhysicalDrive
```

**Production pawn stack сегодня:** 2 authored компонента.  
**С Physical GRIP:** + `UGRIPHandComponent` (+ Aim) + `UDIVEGRIPBridgeComponent`.  
**PIE без EI:** + `UDIVELegacyKbmInputComponent`.

`CanContainContent = false`. Модули: Core / Runtime / RuntimeDev(DeveloperTool) / Uncooked / UnrealEditor.

---

## 3. Соответствие `Plugin_Architecture_Principles.md`

| # | Требование | Статус | Evidence | Заметка |
|---|------------|--------|----------|---------|
| П1 | Game module не владеет биндингом плагина | ✅ | `TryRequestSessionFromActionId` на Inspectable; QUICKSTART §3 | Host glue — BP. C++ KDP не содержит символов DIVE (compliance хоста не часть этого аудита) |
| П2 | RuntimeDev не протекает в Core/Runtime/Bridge | ✅ / ⚠️ | grep Runtime→RuntimeDev = 0; `DIVEUnrealEditor.Build.cs:31` PrivateDependency RuntimeDev | Editor→Dev ради Dump. Не shipping leak; граница размыта |
| П3 | Legacy-конфликты только в RuntimeDev | ✅ | `DIVEGripLegacyDevQuery.cpp`; `DIVELegacyKbmInputComponent.cpp:181-185` | Правильно. При ACTS+GRIP+DIVE Legacy на одном Pawn порог §5.1 (≥3) уже достигнут — coordinator ещё нет |
| П4 | Развёртывание без C++ | ✅ / ⚠️ | QUICKSTART §4; нет BindAction в Runtime | Без C++ возможно, но authoring = ручная сборка 2+ компонентов, не один root |
| П5 | Плагин не шипит IA/IMC/demo Content | ✅ | `.uplugin:12` `CanContainContent: false` | Default hover material отсутствует намеренно; Scan предупреждает |
| П6 | Feature Root SHOULD | ❌ | Нет `UDIVEPlayerComponent`; QUICKSTART:219-224 | Input уже близок к root по API, UI — отдельный authored leaf |
| §3.1 | Core тонкий | ⚠️ | `DIVECore.Build.cs` тянет Engine; actions/catalog в Core | Shared gameplay contract, не CoreUObject-only. Accepted |
| §3.2 | Runtime: Handle* без EKeys/IA | ✅ | `DIVEInputComponent.h:37-88`; EKeys только `DIVEContextMenuWidget.cpp:158` Escape | UI-жест допустим §6.2.5 |
| §3.3 | Legacy только RuntimeDev, forward на Handle* | ✅ | `DIVELegacyKbmInputComponent.cpp:159-331`, BindKey:389-456 | Production root Legacy не создаёт |
| §4.4 MUST NOT mega-root | ✅ | Нет ACTS+GRIP+DIVE god-component | |
| §4.4 MUST NOT per-pawn state в subsystem | ✅ / ⚠️ | Session world-scoped документирован; presentation на Pawn | Камера/focus/menu cache в subsystem — доменная сессия, не «спрятать компоненты» |
| §5 BindKey в Runtime | ✅ | BindKey только RuntimeDev | |
| §6.1.4 local control | ✅ | `DIVEInputComponent.cpp:201-205`, UI/Legacy аналогично | Owner = `APawn`; PC placement не поддерживается (совпадает с Pawn-аудитом §5.1.7) |
| §6.1.7 owner scope валидируется | ⚠️ | Cast к Pawn без `IsDataValid` на player-компонентах | Device Inspectable валидируется; player stack — нет |
| §6.2.9 Directory.Exists | ⚠️ | `DIVERuntimeDev.Build.cs:44-67` папка + DisablePlugins + regex Enabled:false | Не единственный критерий, но не `IsPluginEnabledForTarget`. `*_WITH_GRIP` — PrivateDefinitions ✅ |
| §6.2.12 silent first-by-class при N>1 | ❌ | `DIVEPawnPhysicalDriveResolve.cpp:19-27` | Input→UI использует `SharedComponentResolve` (warning при N>1). Pawn drive — нет |
| §6.3 IsDataValid | ✅ device / ❌ player | Catalog, Inspectable, DeviceDefinition | Нет IsDataValid на Input/UI |
| §6.3 smoke begin/end/possession | ⚠️ | `DIVESmokeTests.cpp` — CDO/unit | Нет session lifecycle / late possess |
| §7 Bridge | ✅ sibling / ⚠️ uplugin | `DIVEGRIPBridge/` отдельный плагин; DIVERuntime не линкует GRIP | `.uplugin:26-29` Optional GRIP **противоречит** `ARCHITECTURE.md:138` |
| §8 документация минимума | ⚠️ | ARCHITECTURE / QUICKSTART / README есть | Битые пути на DeviceInteractionModel; нет Feature Root / managed internals / owner policy в форме §8 |

### Чеклист §9 (перед merge) — снимок DIVE

- [x] Runtime без BindKey / BindAction; Escape в виджете — ок  
- [x] Legacy — opt-in RuntimeDev, forwards на Handle*  
- [ ] ≥3 Legacy-enabled фич на Pawn — coordinator не оценён в DIVE (межплагинная тема)  
- [x] DIVERuntime.Build.cs без gameplay-плагинов  
- [x] Bridge — отдельный плагин  
- [ ] Optional GRIP в DIVE `.uplugin` без conditional-модуля в этом плагине  
- [x] Runtime/Core не ссылаются на RuntimeDev  
- [ ] QUICKSTART описывает root/minimal stack — описывает **два** компонента  
- [ ] Внутренние роли не требуют ручного wiring — Context Menu UI требует  
- [ ] Owner scope совпадает с guards — guards есть, валидация player-компонента нет  
- [ ] Нет silent by-class при N>1 — нарушено для pawn physical drive  
- [x] Нет зашитых IA/IMC  
- [x] Таблица IA_* в ARCHITECTURE  
- [ ] Сборка без RuntimeDev не прогонялась этим аудитом (операционный пункт)

---

## 4. Соответствие Pawn-аудиту 2026-08-17

Pawn-аудит — **диагностика + план**. Шапка: «код не менялся»; §11 — пустые чекбоксы. Интерпретировать §7.4 / §11 как текущий код **нельзя**.

### 4.1. Диагностика (§1–4) — актуальна

| Утверждение | Факт |
|-------------|------|
| DIVE без Physical = 2 production ActorComponents | ✅ `UDIVEInputComponent` + `UDIVEContextMenuUIComponent` |
| Input ≈ естественный кандидат в root | ✅ semantic Handle*, chrome, tick, input mode |
| Context Menu UI — presentation, actor identity не обязательна | ✅ виджеты на PC; компонент только host |
| Session — world-scoped | ✅ `UDIVESessionSubsystem : UWorldSubsystem` |
| Per-pawn input/UI нельзя переносить в subsystem | ✅ соблюдено |
| Bridge — отдельный authored leaf | ✅ `UDIVEGRIPBridgeComponent` |
| «Pawn stack 4–6 без wizard» (аудит 08-14) | ✅ 2 DIVE + GRIP hand(s) + Bridge + Legacy |

### 4.2. MUST §5.1 текущего кода

| MUST | Статус |
|------|--------|
| Нет BindKey в Runtime | ✅ |
| IA/IMC в Content проекта | ✅ (контракт; assets хоста этим аудитом не подтверждены) |
| Typed Handle* | ✅ на Input, не на root |
| DIVERuntime self-contained | ✅ |
| DIVE не линкует GRIP | ✅ |
| RuntimeDev снимается без поломки API | ✅ API на Input |
| Per-pawn state не в global GI subsystem | ✅ WorldSubsystem = сессия |
| External providers | ✅ `IDIVEPawnPhysicalDrive`, `IDIVEProxyDrive` |
| Миграционный путь explicit→managed | ❌ explicit — единственный путь |

### 4.3. Целевая архитектура §7.4–7.6 — не начата

| Цель | Статус |
|------|--------|
| `UDIVEPlayerComponent` | ❌ класс отсутствует |
| Context UI — managed internal | ❌ authored leaf + resolve by name/class (`DIVEInputComponent.cpp:207-211`) |
| Managed/External modes, `EnsureInitialized`, duplicate detection | ❌ |
| `IsDataValid` на root (mixed old/new) | ❌ |
| Instanced `UDIVEPawnPhysicalDriveProvider` в root | ❌ interface живёт на Bridge-компоненте |
| Один Legacy Coordinator | ❌ отдельный `UDIVELegacyKbmInputComponent` |

### 4.4. Критерии готовности §11 (только DIVE)

Authoring: **0/3** (один компонент; provider внутри root; ≤1 Legacy coordinator).  
Архитектура deps: **в основном да** (Runtime без GRIP, без RuntimeDev, без key bindings).  
Managed без name lookup: **N/A** (managed нет).  
Lifecycle: EndPlay/local-control/no UI на non-local — **да**; late possession / travel re-bind — **частично**; managed duplicates — **N/A**.  
Миграция: **0/3**.

### 4.5. Lifecycle, которого нет в целевом корне, но есть риск сейчас

`UDIVEInputComponent::BeginPlay` (`:21-34`) при уже активной сессии повторно вызывает `HandleSessionStarted`. Это покрывает **respawn нового pawn**, пока сессия жива.

Нет `NotifyControllerChanged` / possess handler. Late `Possess` того же pawn без EndPlay/BeginPlay **не** переинициализирует presentation. World tear-down → `EndSession(Forced)` (`DIVESessionSubsystem.cpp` `OnWorldBeginTearDown`) — ок; seamless travel на новый pawn ждёт его BeginPlay.

Репликации нет. Для local-only контракта это согласовано с §9.1 Pawn-аудита. Документировать non-goal MP в ARCHITECTURE — P2.

---

## 5. Что уже закрыто и не переоткрывать

Из `Audit_v08_CodeHealth_QoL.md` и `Audit_2026-08-14.md`, подтверждено кодом 2026-08-17:

| ID | Суть | Evidence сейчас |
|----|------|-----------------|
| B1 ExecutionWorld | Catalog actions получают мир | `FDIVEActionWorldScope` в `ExecuteResolvedAction` (`DIVESessionSubsystem.cpp:413`) |
| B2 CanExecute на primary | Общий путь | тот же `ExecuteResolvedAction` |
| B3 isolation pick | `bHiddenInGame` | `DIVEInspectableComponent.cpp:207-209` |
| B4 overlay restore | Previous overlay | `PickHoverPreviousOverlay` в subsystem |
| Binding validator | Один util | `DIVEActionBindingValidation` ← Catalog / Inspectable / Scan |
| Device proxy → continuous slot | `InternalProxyDriveAction` | `DIVESessionSubsystem.h:254-260` |
| Dead Anchor pick branches | Pick только Primitive/DeviceRoot | `DIVEPick.cpp:49,131` |
| bChecked в меню | Пронесён в entry | health-pass; не регрессировать |
| Widget class overrides | Chrome + Context menu | `SessionChromeWidgetClass`, `ContextMenuWidgetClass` |
| Admin gate только в действиях | Subsystem API открыт | `ToggleMeshPhysicsForTarget` без shipping gate |
| WorldSubsystem + TearDown | | `DIVESessionSubsystem.h:42`; `OnWorldBeginTearDown` |
| CDSO seed | NewObject в Seed | `DIVEInspectableComponent.cpp:33-74` |
| FocusBlendDuration в Definition | | smoke `DIVE.Camera.DefinitionBlendDuration` |
| Scan якорей по иерархии | | `DIVEDeviceScan.cpp:75-85` `ForEachDeviceActor` |
| ExecuteResolvedAction | меню и primary | Closed |
| Dual OnExecuted fan-out | один choke | `NotifyActionExecuted` `:512-522` |
| `TryRequestSessionFromActionId` | | Inspectable + QUICKSTART §3 |
| FormatActiveLabelSuffix / SetModeHintVisible / public ShouldDeferMouseWheelToGrip | удалены | не возвращать |

PIE-чеклист из Audit_v08 §11 остаётся **operational**, не code debt.

---

## 6. Мёртвый код, рудименты, ложные срабатывания

### 6.1. Не мёртвое (не удалять)

| Что | Почему |
|-----|--------|
| `UDIVELegacyKbmInputComponent` | Dev PIE без EI Content |
| `DIVEGripLegacyDevQuery` | Dev wheel policy; `#if DIVE_WITH_GRIP` |
| `DIVE::kActionOpenDIVE` | Контракт хоста / ACTS ActionId |
| `EDIVEFocusKind::Anchor` | Жив на start-focus / isolation; pick его не возвращает — это контракт, не dead enum |
| `FindOnPawn` / `FindOnPlayerController` | Production Physical fallback + smoke |
| `UK2Node_DIVEActionBoundEvent` | ExpandNode wildcard BindingId |
| CoreRedirects K2 Editor→Uncooked | `Config/Engine.ini` — старые графы |
| `GetActionInstances` / `FindActionInstance` | Ярус B / Dump / K2; **нет C++ callers** → Medium, Reference Viewer |
| `OnSessionLifecycle`, `OnFocusChanged`, `CanNavigateBack` | Blueprint surface |
| `IDIVEProxyDrive` / registry | Host extension; implementors могут быть только в Content |
| Historical `Problem_DuplicatedCatalogActions.md` | Помечен superseded |

TODO/FIXME/HACK в `Source/` — **0**. Закомментированных кусков нет. Это плюс.

### 6.2. Рудименты и гигиена (P2, с оговорками)

| ID | Finding | Severity | Confidence | Рекомендация |
|----|---------|----------|------------|--------------|
| D1 | 9× `GetEffective*` — тонкие обёртки над `GetEffectiveCameraSettings()` (`DIVEInspectableComponent.h:200-228`, `.cpp:288-330`). C++ callers нет | Info | Medium (BP) | Не удалять без Reference Viewer; deprecate в пользу struct |
| D2 | `DIVEContextMenu::BuildEntries` — 12 строк forward (`DIVEContextMenu.cpp:9-20`) | Info | High | Inline в subsystem/smoke |
| D3 | `CanNavigateBack` дублирует проверку `NavigateBack` | Info | Medium | Оставить как query API или использовать внутри NavigateBack |
| D4 | `bSessionActive` на Inspectable параллельно subsystem (`h:178,305`; `NotifySessionLifecycle:506-509`) | P2 | High | Query subsystem; shim getter для BP |
| D5 | Dual camera UPROPERTY: Inspectable + `UDIVEDeviceDefinitionAsset` | P2 | High | Общий `FDIVECameraSettings` |
| D6 | `IsDataValid` якоря: `Owner->GetComponents` (`:1376-1377`) vs Scan/runtime `ForEachDeviceActor` | P1 | High | **Новый шов.** Child-actor duplicate PartId не поймает Data Validation |
| D7 | Битые doc-пути: README/ARCHITECTURE/QUICKSTART → `Docs/DeviceInteractionModel.md`, файл в `Docs/Additional/` | P2 | High | Починить ссылки; то же для historical Problem_* |
| D8 | ARCHITECTURE smoke-список неполный: нет `CollectMatchingPrimitives`, `ComponentNameMatch` | Info | High | Синхронизировать таблицу |
| D9 | `Logical` mode только в DeviceInteractionModel как planned | Info | High | Won't-fix product; ссылки ок, если везде «planned» |
| D10 | K2 `static bool bHooksInstalled` без unregister (`K2Node_DIVEActionEvent.cpp`) | Low | High | При рефакторе — module shutdown; сейчас типичный UE editor hook |
| D11 | Scan дублирует empty/duplicate PartId цикл (`DIVEDeviceScan.cpp:74-115`) рядом с Inspectable `IsDataValid:1376-1409`, причём **разный обход иерархии** | P1 | High | Один helper `CollectDeviceAnchors` + ForEachDeviceActor |

### 6.3. Как проверять Blueprint-only API перед удалением

1. Reference Viewer на UFunction/delegate.  
2. Asset Registry: subclass `UDIVEDeviceAction`, pawn BP с DIVE-компонентами.  
3. Не grep `.uasset`.  
4. PIE + `LogDIVE`.  
5. Существующие smoke в RuntimeDev.

---

## 7. Разрывы систем, костыли, архитектурная грязь

### 7.1. Оправданные workaround (оставить, не размножать)

| Костыль | Где | Почему законен |
|---------|-----|----------------|
| `ExecutionWorld` + RAII | `DIVEDeviceAction.h:118-165` | Asset-hosted UObject без мира |
| `bHiddenInGame` в pick | Inspectable:207-209 | `IsVisible()` не отражает isolation |
| Multi-hit + pick-proxy | `DIVEPick.cpp` | Visibility бьёт в оболочку |
| Overlay save/restore | PickOps | Hover инвазивен |
| `EDIVEPreservedInputMode` | Input:17-23, cpp:384 | PC не отдаёт предыдущий `FInputMode` |
| `bIgnoreNextPrimaryActionRelease` | Session | LMB-up после строки меню |
| `InternalProxyDriveAction` | Subsystem:254-260 | Единый continuous slot для device proxy |
| `NormalizeComponentToken` | Core | BP component names нестабильны |
| Dual mouse delta (GetInputMouseDelta → cursor) | Input:243-264, 536-566 | EI vs Legacy orbit/drag |
| `OnWorldBeginTearDown` | Session | Сессия не должна переживать world |

### 7.2. Хрупкие швы (открытые)

**S1. Optional GRIP в `.uplugin` (P1, High)**  
`DeviceInteractiveViewExplorer.uplugin:26-29` — `"Optional": true` на GraspRigidbodyInertialPhysics.  
`ARCHITECTURE.md:138` явно: DIVE `.uplugin` **must not** depend on Optional GRIP.  
Runtime GRIP не линкует — функционально self-contained. Декларация врёт и нарушает дух §7 (Optional без conditional-модуля **в этом** плагине). RuntimeDev co-location должен остаться в Build.cs.

**S2. Silent first `IDIVEPawnPhysicalDrive` (P1, High)**  
`DIVEPawnPhysicalDriveResolve.cpp:19-27` возвращает первый implementor без warning.  
`SharedComponentResolve` при N>1 отказывается. Bridge для GRIP Hand при нескольких руках **не** делает silent FindComponentByClass (`DIVEGRIPBridgeComponent.cpp:263`), но **сам** резолвит `UDIVEInputComponent` через `FindComponentByClass` (`:73`, `:479`) — тот же класс бага на границе sibling.

**S3. Player wiring по имени/классу (P1, High)**  
Input → Context Menu UI: `SharedComponentResolve::FindComponentByNameOrClass` (`DIVEInputComponent.cpp:207-211`). Это и есть «каждая роль = единица развёртывания».

**S4. Late possession (P1, High)**  
Нет re-init при Possess без recreate. Session delegates только из BeginPlay.

**S5. Editor → RuntimeDev (P2, High)**  
`DIVEUnrealEditor.Build.cs:31` + `DIVEUnrealEditorModule.cpp:5,79` ради Dump. Сборка Editor без RuntimeDev сломает меню Dump.

**S6. RuntimeDev plugin detect (P2, High)**  
`Directory.Exists` + regex `.uproject`. Комментарий ссылается на §6.2.9, но это не UBT-native enable check. Дублируется в Bridge/ACTS (вне скоупа, но тот же parser).

**S7. Shared catalog instance state (P2, High)**  
Instanced actions в DataAsset общие на все устройства. Корректность = «одна сессия на world» + `ensure(!bInteractionActive)`. В ARCHITECTURE инвариант не формализован (был в закрытом B5).

**S8. Friend Session*Ops (accepted)**  
Разбиение по файлам, полный доступ к private. Не выделять IsolationSubsystem.

**S9. Три пути «нажать на деталь»**  
Default LMB и меню сведены в `ExecuteResolvedAction`. Physical LMB → InternalProxy **или** pawn bridge. Pawn-drag намеренно без value-HUD (документировано). Не склеивать с Catalog.

**S10. Два канала событий действия**  
`Inspectable.OnActionExecuted` и `Action->OnExecuted` оба из `NotifyActionExecuted`. K2 слушает Inspectable. Дубль не баг, но «куда биндиться» всё ещё два места.

**S11. Dual mouse/orbit tick**  
Input `bCanEverTick=true`, tick включается на сессию: orbit fallback, hover, proxy drag. EI `HandleOrbitDelta` параллелен. Shipping платит tick пока сессия активна — приемлемо; вынести Legacy-orbit в RuntimeDev — P2.

**S12. Host EI path в репо не виден**  
Плагин не шипит IA_*. Production path не верифицируется исходниками KDP. PIE, скорее всего, Legacy. Это compliance **хоста**, не плагина; принцип 4 end-to-end в этом репо не доказан.

---

## 8. «Можно проще, эффективнее, без потери функций»

Сохранять: object actions + catalog; WorldSubsystem session; RuntimeDev quarantine; sibling Bridge; `FDIVEActionWorldScope`; InternalProxyDriveAction; primary specificity ranking; dual authoring component+catalog (union).

### Уровень 1 — Feature / deployment (максимальный ROI)

**P-Root. `UDIVEPlayerComponent`**  
Один authored component: проксирует все `Handle*`, создаёт managed Context Menu UI (ActorComponent с actor outer **или** UObject-presenter), хранит chrome/input config, extension slot для Physical provider.  
Эквивалентность: те же виджеты, делегаты, EI targets (redirect Input→Root).  
Trade-off: +1 класс, deprecation cycle; −ручной wiring, −name lookup.  
Риск: низкий–средний (Pawn-аудит Фаза 1). Приоритет: **P1 architecture**.

Не делать: mega-root ACTS+GRIP+DIVE; не переносить session в Input; не auto-create Legacy.

### Уровень 2 — Ownership / state

| Proposal | Эквивалентность | Риск | Приоритет |
|----------|-----------------|------|-----------|
| Inspectable `IsSessionActive` → query subsystem | Те же BP events | Низкий (shim) | P2 |
| Явный slot/name для pawn physical drive | 1 bridge без изменений; N>1 детерминизм | Низкий | P1 |
| Формализовать «1 session/world ⇒ shared catalog instances ok» в ARCHITECTURE | Поведение то же | Нулевой | P2 |
| Не клонировать catalog per session | — | Клон = GC/cost, не нужен | Won't |

### Уровень 3 — Input / UI

| Proposal | Эквивалентность | Риск | Приоритет |
|----------|-----------------|------|-----------|
| UI как managed service root (P-Root) | Те же widgets | Средний (зависит от root) | P1 |
| Tick orbit только для Legacy; EI = HandleOrbitDelta | EI без изменений | Средний (проверить gamepad) | P2 |
| Pool ValueReadout как chrome | Тот же HUD | Низкий | P2 |
| Exit при открытом меню — документировать как intentional | Уже работает (`HandleExitSession` без suppress) | Нулевой | Docs |

### Уровень 4 — Device Inspectable (~1480 LOC)

Не дробить ради «чистых подсистем». Имеет смысл **вынести логику, оставив facade UFUNCTION**:

- `FDIVEBindingCatalog` — merge, primary resolve, menu rows (сейчас `AppendConfiguredContextMenuEntries`, `GatherAuthoredBindings`);
- `FDIVEPickPolicy` — pickable / exclusions / hover material;
- `FDIVECameraSettings` USTRUCT вместо копипаста Definition ↔ Inspectable.

Эквивалентность: те же Scan/Dump/menu. Риск средний, без breaking, если публичные методы остаются на Inspectable. Приоритет **P2 maintainability**, после root.

Не убирать dual Bindings+Catalog: component = defaults/one-off, catalog = 100 болтов.

Admin seed: сейчас Seed всегда кладёт Simulate/Delete (`:67-73`); Shipping прячет через `GetDisplayState`. Можно seed только Standard — QoL, не MUST.

### Уровень 5 — Actions / events

Не упрощать модель объектов. Кандидаты:

- канонический BP-канал = `Inspectable.OnActionExecuted`; `Action->OnExecuted` оставить как fan-out (уже так) и не плодить третий;
- K2 ~880 LOC (`K2Node_DIVEActionEvent.cpp`) **не удалять** — authoring UX; не расширять;
- `UDIVENotifyAction` оставить.

### Уровень 6 — Modules / build

| Proposal | Эквивалентность | Риск | Приоритет |
|----------|-----------------|------|-----------|
| Убрать Optional GRIP из DIVE `.uplugin` | Runtime без изменений; Dev co-location через Build.cs | Низкий | P1 |
| Dump перенести в UnrealEditor; Editor не линкует RuntimeDev | Те же меню; PIE console остаётся в Dev | Низкий | P2 |
| UBT-native plugin enable вместо Directory+regex | Тот же `DIVE_WITH_GRIP` | Средний (три Build.cs в репо) | P2 |
| DeviceDefinition в Core (data-only) | Те же поля | Низкий | P2 optional |
| Не сливать Core+Runtime | — | — | Won't |

### Уровень 7 — Tick / tests

- Один latent smoke: BeginSession → menu → primary → EndSession → EndPlay restore input/view. Принципы §6.3. Operational **P2**.  
- Smoke на `FindOnPawn` при двух implementors (сейчас только null).  
- Не автоматизировать PIE catalog Focus как code-debt blocker.

---

## 9. Документы плагина — drift

| Документ | Статус vs код |
|----------|----------------|
| README | Актуален по стеку 2 компонентов; **битая** ссылка `Docs/DeviceInteractionModel.md`; не упоминает Feature Root (ещё нет) |
| ARCHITECTURE | Норматив v0.8; таблица IA_*; **противоречие** Optional GRIP; неполный список smoke; нет §8 Feature Root / managed / owner |
| QUICKSTART | Лучший авторский гайд; pawn stack = explicit; ссылка DeviceInteractionModel без Additional/ |
| DeviceInteractionModel | Контракт Physical/proxy/bridge; Logical = planned — ок |
| Problem_DuplicatedCatalogActions | Historical — ок |
| Audit_DeviceActions §12 | Closure модели действий — код подтверждает |
| Audit_v08 / 08-14 | Health-pass закрыт; **не** использовать как список открытых багов |

Pawn-аудит 2026-08-17 в `Docs/Audits` проекта ссылается на старые пути (`Docs/DeviceInteractionModel.md`, аудиты в корне Docs/). Файлы переехали в `Additional/` и `Audits/`.

---

## 10. План по приоритету (только открытое)

Не включать пункты §5.

### P1 — контракт и целевая граница

1. **Feature Root** `UDIVEPlayerComponent` (managed UI, proxy Handle*, optional physical slot). QUICKSTART → один компонент; explicit stack = Advanced.  
2. Убрать Optional GRIP из DIVE `.uplugin`; поправить ARCHITECTURE, чтобы совпало с кодом.  
3. Детерминированный resolve `IDIVEPawnPhysicalDrive` (имя/слот/warning при N>1); на стороне Bridge — не `FindComponentByClass` для DIVE Input.  
4. `IsDataValid` / Scan: один обход якорей через `ForEachDeviceActor`.  
5. Late possession: re-bind presentation при смене контроллера, если сессия активна.

### P2 — гигиена и эффективность

6. Починить markdown-ссылки на `Docs/Additional/…`.  
7. Вынести Dump из зависимости Editor→RuntimeDev.  
8. Документировать инвариант shared catalog + local-only/no replication.  
9. `bSessionActive` shim; camera USTRUCT; inline `DIVEContextMenu`.  
10. Smoke: session lifecycle; multi-drive resolve.  
11. Оценить (не обязательно делать) Legacy coordinator при трёх плагинах — это **межплагинная** задача, не solo-DIVE.

### Не делать в ближайшем цикле

| Тема | Почему |
|------|--------|
| Logical mode | Нет UX |
| Слить Core+Runtime / убрать catalog / GAS | Эталон и v0.8 отвергли |
| Ops → отдельные подсистемы | Ложная чистота |
| Клон catalog per session | Одна сессия на world достаточна |
| GRIP Hand → UObject | Не DIVE |
| Mega-root ACTS+GRIP+DIVE | MUST NOT |
| Удалять Legacy / K2 / FindActionInstance без Content audit | Medium-confidence BP surface |
| Переписывать object-action model | Закрыта |

---

## 11. Scorecard

| Ось | Оценка |
|-----|--------|
| Модель действий v0.8 | Здоровая, не трогать |
| MUST input / modules / Bridge direction | ~90% |
| SHOULD Feature Root / managed internals | ~0% реализации; диагноз 100% актуален |
| Pawn-аудит §11 DIVE authoring | Не выполнен (и не обещался как уже сделанный) |
| Мёртвый production-код | Почти нет; есть BP-only и thin wrappers |
| Костыли | В основном честные UE-limits |
| «Можно проще» без потери функций | Да: 2→1 pawn component, меньше параллельного state, один anchor-validator, честный uplugin |
| Следующий агент | Не чинить B1–B4, CDSO, ExecuteResolvedAction, dual events choke, Hand picker Bridge, ExecutionWorld |

---

## 12. Итог одной страницей

DIVE — зрелый адаптер осмотра устройств. Каркас v0.8 выбран правильно и после health-pass **не требует смены архитектуры действий**.

Несоответствие эталону 2026-08-17 сосредоточено в **единице развёртывания**: автор всё ещё собирает Input + UI (+ Bridge + Legacy), хотя принципы и свежий Pawn-аудит требуют один plugin-owned Feature Root. Это главный ответ на «можно проще не теряя функциональности».

Сопутствующая грязь меньше по объёму, но конкретна: ложный Optional GRIP в `.uplugin`, silent first physical-drive, `IsDataValid` без child-акторов, late possession, битые пути документов, Editor→RuntimeDev для Dump.

God-objects Session/Inspectable — accepted. Legacy в RuntimeDev — правильный карантин, не рудимент. Sibling Bridge — правильное §7, его надо встроить в root как provider, а не тащить GRIP в DIVERuntime.

**Нового фреймворка не нужно.** Нужна граница `UDIVEPlayerComponent` и зачистка деклараций/resolve/валидации, которые сейчас врут относительно уже принятых правил.

---

## 13. Closure (2026-08-17, тот же день)

Код DIVE и sibling `DIVEGRIPBridge` изменён этим проходом. Health-pass B1–B4 **не переоткрывались**. GRIP и ACTS **не редактировались**.

### Closed в DIVE

| Пункт аудита | Что сделано |
|--------------|-------------|
| P-Root / П6 / §4.4 Feature Root | `UDIVEPlayerComponent`: Managed / Use Existing; имена `DIVE_Input` / `DIVE_ContextMenuUI`; proxy всех `Handle*`; chrome config; Legacy не создаётся |
| S1 Optional GRIP в `.uplugin` | Зависимость снята; RuntimeDev co-location через Build.cs |
| S2 silent `IDIVEPawnPhysicalDrive` | `FindOnPawn(Pawn, FName)`: unique или named; N>1 unnamed → Warning + `nullptr`. Имя на Player, затем Input |
| S2 Bridge `FindComponentByClass` DIVE Input | `ResolveDiveInput()`: named / DIVE Player managed Input / unique Input; N>1 unnamed → SharedComponentResolve warning + `nullptr` |
| S4 late possession | Input держит idle tick; gain local control + active session → presentation; lose local control → `ClearSessionPresentation` + `EndSession(Forced)` |
| D6 / D11 якоря | `DIVE::CollectDeviceComponents<UDIVEAnchorComponent>` + `ForEachDeviceActor` в `IsDataValid` и Scan |
| D4 `IsSessionActive` | Getter читает WorldSubsystem; `bSessionActive` — shim |
| D2 `DIVEContextMenu::BuildEntries` | Inlined; header/cpp удалены |
| D7 битые пути | README / ARCHITECTURE / QUICKSTART / Problem_* → `Docs/Additional/DeviceInteractionModel.md` |
| §6.1.7 / §6.3 player `IsDataValid` | Owner = `APawn` на Input и Player; Managed + authored leaves → error; два root → error; Use Existing без refs → error |
| Docs §8 | Recommended = один Root; Advanced = explicit Input+UI; owner = locally controlled Pawn; инвариант 1 session/world ⇒ shared catalog; Exit при открытом меню — intentional; таблица smoke |
| Smoke | `DIVE.PawnPhysicalDrive.Resolve` N>1 (live world); `DIVE.Player.IsDataValid` CDO / non-Pawn / mixed Managed+leaves |

### Won't this pass

| Тема | Почему |
|------|--------|
| Instanced Physical provider внутри Root | **Closed:** `UDIVEGRIPPhysicalDriveProvider` is a UObject on Player (`IDIVEPawnPhysicalDrive`); no Bridge ActorComponent |
| Межплагинный Legacy coordinator | ACTS+GRIP+DIVE, не solo-DIVE |
| Split Inspectable / BindingCatalog / PickPolicy | P2, не закрывали — второй AC / перенос Bindings сломает device BP |
| Camera USTRUCT (D5) | **Closed:** `FDIVECameraSettings` на Inspectable и Definition |
| Dump из RuntimeDev / Editor→Dev | **Closed:** dump в `DIVEUncooked`; Editor не линкует RuntimeDev; console регистрирует Dev |
| UBT-native plugin detect | **Closed:** `ProjectDescriptor` + `Plugins.ReadAvailablePlugins` (DIVE + Bridge) |
| Полный PIE `CreateWorld` session smoke | **Closed as live-world:** `DIVE.Session.Lifecycle` (skip без PC; без `CreateWorld`) |
| Logical mode, Core+Runtime merge, mega-root | Отклонено аудитом §10 |

### Scorecard после closure

| Ось | Было | Стало |
|-----|------|-------|
| SHOULD Feature Root | ~0% | Implemented (additive; Advanced stack жив) |
| Silent N>1 pawn drive | ❌ | Unique/named |
| Bridge silent DIVE Input | ❌ | Named / Player managed / unique |
| Optional GRIP в DIVE `.uplugin` | ❌ | Снят |
| Late possession | ❌ | Poll на Input |
| Player `IsDataValid` | ❌ | Player + Input |
| Dump Editor→RuntimeDev | ❌ | Dump in Uncooked; Editor does not link Dev |
| Bridge as provider in Root | ❌ | Instanced provider + managed Bridge |

---

## 14. 100% строк vs единица развёртывания (2026-08-17)

**P1 / Feature Root — закрыт.** Input, Context Menu UI и GRIP Bridge **сняты как ActorComponent**. На pawn из DIVE остаётся один `UDIVEPlayerComponent` (+ optional Legacy KBM). Physical — UObject provider на Player.

Health-pass B1–B4 не переоткрывались.

### ActorComponents на pawn

| Сценарий | Было | Сейчас |
|----------|------|--------|
| DIVE без Physical GRIP | 2: Input + Context Menu UI | **1:** `UDIVEPlayerComponent` |
| DIVE + Physical GRIP | 3: Input + UI + Bridge | **1:** Player (instanced `UDIVEGRIPPhysicalDriveProvider`, не AC) |
| PIE без EI | +1 Legacy KBM | +1 Legacy KBM (Player его не создаёт) |
| Device-актор | Inspectable ± Anchor | без изменений (не player-side) |

GRIP Aim / Input / Coordinator AC **удалены** (логика на Rig: `GRIPAimLogic` / `Handle*`). `UGRIPHandComponent` жив, но не spawnable — Rig создаёт или adopt'ит. DIVE этих классов не держит.

```text
Pawn
 └─ UDIVEPlayerComponent          ← единственный DIVE ActorComponent
      ├─ session input / chrome / context menu (внутри Player)
      └─ instanced GRIP provider (UObject, IDIVEPawnPhysicalDrive)
 └─ UGRIPRigComponent             ← GRIP; слоты Player + Dive для Physical
```

DIVE Input / Context Menu UI / GRIP Bridge AC **удалены** (нет `.h/.cpp`, нет hide-meta). Не спрятаны.

### Что не 100% при скоупе «каждая строка»

| Тема | Статус |
|------|--------|
| Camera USTRUCT / dual Inspectable↔Definition (D5) | **Closed:** `FDIVECameraSettings` на Inspectable и Definition |
| Split Inspectable (BindingCatalog / PickPolicy) | P2, не закрывали — второй AC / перенос Bindings сломает device BP |
| UBT-native plugin enable вместо Directory+regex | **Closed:** `ProjectDescriptor` + `Plugins.ReadAvailablePlugins` |
| Полный PIE session smoke (`CreateWorld`) | **Closed as live-world:** `DIVE.Session.Lifecycle` (skip без PC; без `CreateWorld`) |
| Межплагинный Legacy coordinator | не solo-DIVE |
| Logical mode / merge Core+Runtime / mega-root | отвергнуто §10 — закрывать не нужно |
| Provider как чистый UObject-drive без ActorComponent | **Closed:** GRIP provider is UObject; Player is the AC implementor that forwards |

Game module / `ASimpleCameraCharacter` DIVE не содержит: интеграция — компонент на pawn BP + `IA_*` в Content.

