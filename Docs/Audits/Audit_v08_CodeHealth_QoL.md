# Аудит DIVE v0.8 — баги, рудименты, архитектурные швы, QoL

**Дата:** 2026-08-11
**Охват:** все модули плагина (`DIVECore`, `DIVERuntime`, `DIVERuntimeDev`, `DIVEUnrealEditor`), сиблинг `DIVEGRIPBridge`, вся документация в `Docs/` и README.
**Контекст:** это второй аудит. Первый (`Audit_DeviceActions_Architecture.md`) задал переход от FName-каталога к объектным действиям и по чек-листу §12 закрыт. Настоящий аудит проверяет *получившуюся* реализацию v0.8: баги, мёртвый код, рудименты, разрывы между системами, QoL и вопрос «можно ли проще, не теряя важного».

---

## 0. Вердикт (сводка)

Ядро v0.8 — правильное и соответствует возможностям UE. Instanced-объекты действий (`EditInlineNew` + `DefaultToInstanced` + `Instanced`-массивы в структурах) — это ровно паттерн Enhanced Input / GAS, UE 5.x поддерживает его полностью, включая инстансинг в DataAsset и на компонентах. Откатывать или радикально перестраивать архитектуру **не нужно**.

Однако найдено:

- **1 критичный баг (B1):** действия, инстансированные в `UDIVEActionCatalogAsset`, не имеют мира (`GetWorld() == nullptr`) — встроенные действия из каталога молча не работают, а документация при этом рекомендует каталог как «основной дом». Это подрывает главный авторинг-путь v0.8.
- **3 заметных бага (B2–B4):** primary-клик исполняет мгновенные действия в обход `CanExecute`; изоляция прячет меши так, что они остаются кликабельными; hover затирает чужой overlay-материал.
- **Мёртвый код и рудименты:** недостижимые Anchor-ветки на пик-пути, «потерянный» `bChecked`, режим `Logical` только в доках, обёртка-форвардер `DIVEComponentResolve`.
- **Дубли:** логика начала proxy-drive написана дважды; валидация биндингов — трижды; admin-гейтинг — дважды.
- **QoL-дыры:** два из трёх виджетов не переопределяются из BP; `SectionId` — строковая ссылка без выпадашки; сид-дефолты оставляют мёртвые субобъекты.

Ничего из этого не требует смены архитектуры — только точечные исправления. План в §8.

---

## 1. Метод

Прочитан весь C++ обоих плагинов (заголовки и реализации), `.uplugin`, `Build.cs`, `Config/*.ini`, все пять документов `Docs/` и README. Каждая находка ниже проверена по конкретным строкам кода (файлы указаны); утверждения о поведении UE (инстансинг, `GetWorld`, ограничения details-панели) сверены с известными механизмами движка, а не только с документацией плагина.

---

## 2. Баги

### B1 (критично). Действия из Action Catalog живут без мира — встроенные действия из каталога не работают

**Где:**
- `DIVECore/Private/DIVEDeviceAction.cpp` → `UDIVEDeviceAction::GetWorld()` — идёт по цепочке Outer'ов до первого объекта с миром.
- `DIVERuntime/Private/Actions/DIVEBuiltInActions.cpp` → `ResolveSessionSubsystem(const UObject* WorldContext)` — берёт `WorldContext->GetWorld()`.

**Механика.** Для `Instanced`-действия, созданного **на компоненте** (`UDIVEInspectableComponent.Bindings`), Outer-цепочка ведёт через компонент к актору и миру — всё работает. Для действия, созданного **в каталоге** (`UDIVEActionCatalogAsset.Bindings`), Outer-цепочка ведёт к DataAsset'у и его пакету в Content Browser. Ни один из них мира не имеет. Это фундаментальное свойство UE: asset-hosted UObject не привязан к миру, и никакой Outer-трюк этого не изменит.

**Последствия:**
1. `UDIVEIsolateAction`, `UDIVESimulatePhysicsAction`, `UDIVEDeleteMeshAction`, положенные в каталог: `ResolveSessionSubsystem(this)` → `nullptr` → `CanExecute` = false → строки меню навсегда disabled. Без единого ворнинга.
2. `UDIVEFocusAction` из каталога: `CanExecute` проходит (не требует подсистемы), строка выглядит рабочей, но `Execute` молча возвращает false.
3. Blueprint-действия из каталога: любые world-context ноды (Spawn Sound, Spawn Emitter, Line Trace, Get Game Instance...) получают null world.

**Почему это критично.** `QUICKSTART.md` и `Audit_DeviceActions_Architecture.md` прямо рекомендуют каталог как предпочтительный дом для биндингов («Prefer Action Catalog»). То есть рекомендованный путь ломает и встроенные действия, и половину BP-возможностей. Тезис старого аудита «BP world-context ноды работают через GetOuter-цепочку до устройства» верен только для component-hosted инстансов.

**Исправление (простое, без ломки API):**
1. В `FDIVEActionContext` контекст уже несёт `DeviceHost` и `SourceComponent` — оба world-bound на момент исполнения. Встроенные действия должны резолвить подсистему из `Context.DeviceHost` (или `Context.SourceComponent`), а не из `this`.
2. Для BP-действий: перед каждым вызовом `CanExecute`/`GetDisplayState`/`Execute`/`Begin/Update/EndInteraction` исполнитель (Inspectable/Subsystem) выставляет на инстансе transient `TWeakObjectPtr<UWorld> ExecutionWorld`, а `UDIVEDeviceAction::GetWorld()` возвращает его в первую очередь, и только затем падает на Outer-цепочку. Это стандартный приём (так делает, например, `UBlueprintAsyncActionBase`/`UGameplayAbility` через world context).
3. Добавить в `IsDataValid` каталога **ошибку не добавлять**, а в доки — примечание, что после фикса каталог полностью равноправен компоненту.

### B2. Primary-клик исполняет мгновенные действия в обход `CanExecute` и `Condition`

**Где:** `DIVERuntime/Private/Session/DIVESessionPickOps.cpp` → `ExecutePrimaryActionAtScreenPosition()`.

Путь контекстного меню проверяет и `GetDisplayState().bVisible` (при построении меню), и `CanExecute` (в `UDIVESessionSubsystem::ExecuteContextMenuAction`, строка ~356). Путь continuous-действий тоже защищён — `TryBeginContinuousAction` сам вызывает `CanExecute`. Но для **мгновенного** primary-действия `ExecutePrimaryActionAtScreenPosition` заканчивается голым `PrimaryAction->Execute(Context)`.

Итог: действие, скрытое или заблокированное своим `Condition` (например, «крышка ещё прикручена»), нельзя выбрать в меню, но можно исполнить кликом ЛКМ. Встроенные действия спасаются повторной проверкой внутри `Execute_Implementation`, но контракт для авторских BP-действий такой обязанности не декларирует — `Execute` по QUICKSTART'у может сразу делать работу.

**Исправление:** перед веткой Cast/Execute добавить `if (!PrimaryAction->CanExecute(Context)) return false;` — три строки, полная симметрия с меню.

### B3. Изоляция прячет меши, но они остаются кликабельными

**Где:**
- `DIVERuntime/Private/Session/DIVESessionIsolationOps.cpp` — изоляция скрывает через `SetHiddenInGame(true)`.
- `DIVERuntime/Private/DIVEInspectableComponent.cpp` → `IsPrimitivePickable()` (строка ~169) — проверяет `Primitive->IsVisible()`.

`SetHiddenInGame` не меняет visibility-флаг, поэтому `IsVisible()` для спрятанного изоляцией меша остаётся true, коллизия — тоже. Результат: в режиме изоляции ПКМ/ЛКМ по «пустому месту» попадают в невидимый меш — открывается его контекстное меню, срабатывает его primary-действие, происходит фокус на невидимую деталь.

Показательно, что hover-путь эту проблему знает: `FDIVESessionPickOps::UpdatePickHover` отдельно проверяет `Primitive->bHiddenInGame` (строка ~134). То есть подсветки на невидимом меше нет, а меню — есть: пользователь кликает в «ничто» без подсветки и получает меню невидимой детали.

**Исправление:** перенести проверку в общее место — в `IsPrimitivePickable` добавить `bHiddenInGame` к условию невидимости (с сохранением исключения для pick-прокси, которые прячутся намеренно):

```cpp
const bool bVisible = Primitive->IsVisible() && !Primitive->bHiddenInGame;
```

и убрать теперь избыточную проверку из `UpdatePickHover`. Заодно это выровняет обещание QUICKSTART «hidden meshes need tag DIVE.PickProxy» с фактическим поведением.

### B4. Hover затирает чужой overlay-материал

**Где:** `DIVESessionPickOps.cpp` → `SetMeshOverlayMaterial` / `ClearPickHover`.

Снятие подсветки — безусловный `Mesh->SetOverlayMaterial(nullptr)`. Если устройство само использует overlay (индикация повреждения, выделение из VR-пути GRIP и т.п.), DIVE его уничтожит при первом же наведении мыши.

**Исправление:** при установке ховера запоминать `Mesh->GetOverlayMaterial()` рядом с `PickHoverPrimitive` и восстанавливать его в `ClearPickHover` (по образцу `FIsolatedPrimitiveRecord.bWasHiddenInGame` — паттерн в кодовой базе уже есть).

### B5. Shared-инстансы действий из каталога несут пер-жестовое состояние

**Где:** `UDIVEContinuousDeviceAction::bInteractionActive`, `UDIVEProxyDriveForwardAction::ActiveProxyObject`.

Каталог — общий ассет: сто устройств с одним каталогом разделяют **одни и те же** инстансы действий (UE не дублирует instanced-объекты ассета на каждого потребителя). Пер-жестовое состояние на таком инстансе безопасно только благодаря глобальному инварианту «одна DIVE-сессия и один активный continuous-слот на world». Инвариант реален (`UDIVESessionSubsystem` — `UWorldSubsystem`, слот — один), но нигде не зафиксирован как причина корректности.

**Исправление (дёшево):** комментарий у полей + `ensure(!bInteractionActive)` в `MarkInteractionActive`. Если когда-нибудь появится сплит-скрин или вторая сессия — падение будет громким, а не тихой порчей состояния.

### B6. Три разных контракта валидации секций

- `UDIVEActionCatalogAsset::IsDataValid` — неявно разрешает встроенные `Standard`/`Admin`, даже если они не объявлены.
- `UDIVEInspectableComponent::IsDataValid` — строит множество только из авторских секций; если автор очистил `Sections`, биндинг на `Standard` даёт ошибку валидации.
- Билдер меню (`AppendConfiguredContextMenuEntries`) — вообще толерантен: неизвестная секция просто уходит в конец без заголовка.

Автор получает ошибку там, где рантайм работает, и наоборот. Нужен один контракт (см. §4.2 — общий валидатор), рекомендация: билдер строг быть не должен, а оба валидатора должны неявно разрешать встроенные ID.

### B7 (мелочь). При открытом меню не подавляются зум и навигация

`ShouldSuppressSessionInput` гасит орбиту и primary, но `HandleZoom*`, `HandleNavigateBack`, `HandleExitSession` в `UDIVEInputComponent` работают при открытом меню. Колесо мыши двигает камеру за меню, Back меняет фокус — а меню продолжает висеть с пиком, сделанным из уже неактуального ракурса. Стоит либо закрывать меню при этих действиях, либо подавлять их.

---

## 3. Мёртвый код и рудименты

### 3.1. Anchor-ветки на пик-пути недостижимы

`DIVEPick::PickAtScreenPosition` (`Utils/DIVEPick.cpp`) возвращает только `FromPrimitive(...)` либо `MakeDeviceRoot()` — kind `Anchor` из пика не выходит **никогда** (якорь — `USceneComponent` без коллизии, трейс его не видит). При этом:

- `DIVEInputComponent.cpp` (~341–347): fallback «primary → фокус на якорь под курсором» — мёртвая ветка;
- `DIVESessionSubsystem.cpp` (~285): guard `OutPickTarget.Kind == EDIVEFocusKind::Anchor` в построении меню — мёртвая ветка;
- `DeviceInteractionModel.md` и README описывают «anchor focus on primary» и «no menu on anchor pick» — поведение, которого не существует.

Важно: `Anchor`-kind сам по себе жив — он приходит из `TryResolveStartFocusTarget` (StartFocusPartId → семантический реестр) и корректно обрабатывается в `DIVESessionFocusOps`/`IsolationOps`. Мёртв только пик-путь. Решение: либо убрать обе ветки и поправить доки, либо (если поведение хотелось) сделать честный маппинг «pick-proxy примитив → якорь» в `PickAtScreenPosition`. Рекомендация — убрать: явный фокус на PartId уже покрывает сценарий.

### 3.2. `FDIVEActionDisplayState.bChecked` теряется по дороге к виджету

Действия честно выставляют `bChecked` (`UDIVEIsolateAction`, `UDIVESimulatePhysicsAction`), но `FDIVEContextMenuEntry` этого поля не имеет, и виджет его не рисует. Реальную работу делает костыль `DIVEContextMenu::FormatActiveLabelSuffix` — суффикс `*` прямо в `DisplayName`. Итог: поле-фантом в публичном API (`BlueprintReadWrite`!) плюс дублирующий текстовый механизм. Либо пронести `bChecked` в `FDIVEContextMenuEntry` и рисовать чек-марк в виджете (лучше), либо удалить поле и оставить суффикс осознанным решением.

### 3.3. Режим `Logical` существует только в документации

`DeviceInteractionModel.md` трижды описывает поведение режима `Logical`, но в `EDIVESessionInteractionMode` его нет. Пометить в доке как «планируемый, не реализован» или убрать — сейчас читатель ищет несуществующий enum.

### 3.4. `DIVEComponentResolve` — обёртка ради обёртки

`Utils/DIVEComponentResolve.h` — шаблон, единственное содержимое которого — вызов `SharedComponentResolve::FindComponentByNameOrClass` с теми же аргументами. Два call-site'а могут звать shared-версию напрямую; файл удалить.

### 3.5. Мелочи

- `DIVE::kActionOpenDIVE` в коде плагина не используется — это контрактная константа для ACTS-стороны, что легально, но заслуживает комментария «consumed by game code via ACTS» прямо у объявления. **Closed Remaining DIVE Pass:** `TryRequestSessionFromActionId` + QUICKSTART §3.
- `FDIVEContextMenuEntry` — чисто рантаймовая структура, но её поля помечены `EditAnywhere`; редактировать их негде и незачем.
- Четыре `CreateDefaultSubobject`-действия (`DefaultFocusAction` и др.) на `UDIVEInspectableComponent` живут на **каждом** инстансе компонента, даже если автор очистил `Bindings` и они ни в чём не участвуют. Сид-механика (`PostInitProperties` + проверка пустоты) корректна с точки зрения дельта-сериализации, но мёртвые субобъекты остаются. Дешёвая альтернатива — создавать их в `SeedDefaultBindingsIfNeeded` через `NewObject` только когда сид реально происходит.

---

## 4. Архитектурные швы и дубли

### 4.1. Логика начала proxy-drive написана дважды

Один и тот же код «`FindProxyDriveForHit` → `CanProxyDrive` → собрать `FDIVEProxyDriveContext` → `BeginProxyDrive`» существует в:

- `Session/DIVESessionPhysicalDriveOps.cpp` → `TryBeginProxyDriveAtScreenPosition` (Physical-режим, прямой путь);
- `Actions/DIVEBuiltInActions.cpp` → `UDIVEProxyDriveForwardAction::BeginInteraction` (объектный путь).

Это тот самый шов, который старый аудит пометил как «unify later»: Physical-режим ходит мимо системы действий, а действие-мост дублирует его логику. Пока обе копии совпадают — работает; первое же изменение контракта (например, передача модификаторов в context) разъедет их молча.

**Рекомендация:** сделать `UDIVEProxyDriveForwardAction` единственным исполнителем. Physical-режим при нажатии primary конструирует контекст и запускает **внутренний** инстанс ForwardAction через тот же `TryBeginContinuousAction`-слот. `PhysicalDriveOps` худеет до маршрутизации pawn-bridge, `DeviceProxy`-kind в `EDIVEActivePhysicalDriveKind` со временем отмирает. Это единственное *структурное* изменение, которое стоит делать в ближайший цикл.

### 4.2. Валидация биндингов размазана на три копии

Проверки «дубликат SectionId», «пустой Match Value», «null action», «PrimaryActionIndex вне диапазона» повторяются в `UDIVEActionCatalogAsset::IsDataValid`, `UDIVEInspectableComponent::IsDataValid` и `DIVEUnrealEditor/DIVEDeviceScan.cpp`, с расхождениями (см. B6). Вынести в один статический валидатор в `DIVECore` (вход: sections + bindings + опциональный контекст устройства; выход: массив сообщений), звать из всех трёх мест. Заодно уйдёт квадратичность компонентного `IsDataValid` (сбор примитивов устройства выполняется внутри цикла по биндингам — собрать один раз).

### 4.3. Двойной admin-гейтинг

`UDIVESessionSubsystem::ToggleMeshPhysicsForTarget`/`DeleteMeshForTarget` проверяют `AreAdminContextMenuEntriesAllowed()`, и действия `UDIVESimulatePhysicsAction`/`UDIVEDeleteMeshAction` проверяют то же в `CanExecute`/`GetDisplayState`. Дубль не опасен, но субсистемная проверка делает эти публичные методы неработоспособными в Shipping даже для легитимных не-admin сценариев (обучающий скрипт хочет уронить деталь — не сможет). Оставить гейт **только** в действиях; методы подсистемы — обычный API.

### 4.4. `UDIVESessionSubsystem` как GameInstanceSubsystem

~~Подсистема жила на GameInstance…~~ **Закрыто:** мигрирована на `UWorldSubsystem` (Game/PIE only) + сохранён `OnWorldBeginTearDown` → `EndSession`. Одна сессия на world.

### 4.5. Session*Ops — честная оценка

Разбиение толстой подсистемы на `Focus/Isolation/Pick/PhysicalDriveOps` (friend-структуры со static-методами) — это разбиение по файлам, а не по ответственностям: каждый Ops имеет полный доступ к приваткам `Session&`. Как читабельность — работает; как инкапсуляция — нет. Менять **не рекомендую** (выгода не окупит переделку), просто фиксирую, что это компромисс, а не архитектура.

### 4.6. Асимметрия семантического реестра

`BuildSemanticRegistry` сканирует `UDIVEAnchorComponent`'ы только на owner-акторе, тогда как пик и `TryResolveStartFocusTarget` ходят по всей иерархии attached-акторов (`DIVE::ForEachDeviceActor`). Якорь на дочернем акторе (модульное устройство из нескольких BP) молча не попадёт в реестр — PartId не зарезолвится, привязки по PartId не сработают. Исправление — один и тот же обход `ForEachDeviceActor` в реестре.

---

## 5. QoL / удобство в редакторе

### 5.1. Виджеты: одна точка расширения из трёх

| Виджет | Переопределяем из BP? |
|---|---|
| `UDIVEValueReadoutWidget` | да (`ValueReadoutWidgetClass`) |
| `UDIVEContextMenuWidget` | нет — `StaticClass()` захардкожен (`DIVEContextMenuUIComponent.cpp` ~193) |
| `UDIVESessionChromeWidget` | нет — `StaticClass()` захардкожен (`DIVEInputComponent.cpp` ~820) |

Стили (`FDIVEContextMenuStyle`) закрывают покраску, но не структуру (иконки в строках, другой layout секций). Добавить `TSubclassOf<...>` для обоих — по образцу уже существующего `ValueReadoutWidgetClass`, ~10 строк на виджет.

### 5.2. `SectionId` — строковая ссылка без подсказки

Биндинг ссылается на секцию голым `FName`. Опечатка = молчаливый провал в «хвост меню» (см. B6). UE решает это штатно: `meta = (GetOptions = "FuncName")` на FName-свойстве + UFUNCTION на владельце, возвращающая доступные ID (авторские секции + встроенные). Работает и в ассете, и на компоненте. Аналогично можно дать выпадашку для `PrimaryActionIndex` (details customization в `DIVEUnrealEditor`, показывающая display-имена действий биндинга вместо голого int) — сам индекс остаётся правильным компромиссом, так как UE не даёт property-picker на sibling-субобъект.

### 5.3. Легаси-KBM и dev-инструменты

`UDIVELegacyKbmInputComponent` (12 пар bool+FKey), `DIVE.DumpDevice`, редакторский Scan — в хорошем состоянии для dev-инструментов, трогать не нужно. Единственное: Scan после появления общего валидатора (§4.2) должен звать его же.

---

## 6. «Можно ли проще?» — проверка против возможностей UE

Прямой ответ на вопрос запроса: **упрощать структуру дальше особо некуда, а вот дочищать швы — есть куда.** Разбор по слоям:

**Что UE реально не позволяет (и плагин правильно обошёл):**
1. Наследуемые DataAsset'ы с интерактивным добавлением параметров — невозможно; instanced-объекты действий это и решили. Подтверждаю: выбранный механизм — штатный, стабильный, тот же, что в Enhanced Input.
2. Property-picker на соседний инстанс-субобъект в details — нет; отсюда `PrimaryActionIndex` (int). Правильный компромисс, смягчается кастомизацией панели (§5.2).
3. Латентные ноды (Delay/Timeline) в BP-переопределениях функций — нельзя; контракт «Execute синхронен, длительное — через continuous-действия или события устройства» верен и задокументирован.
4. Мир для asset-hosted UObject — UE его **не даёт и не даст**; это не «ограничение обошли», а дыра B1, которую нужно закрыть через контекст исполнения.

**Что можно было бы упростить, но не стоит:**
- Слить `DIVECore` и `DIVERuntime` — нет: разделение «типы/интерфейсы vs сессия» реально позволяет девайсам зависеть только от Core.
- Убрать двойной дом биндингов (компонент + каталог) — нет: компонентные биндинги нужны для сид-дефолтов и одноразовых устройств, каталог — для массовых (100 болтов). Правила слияния простые (union + dedupe секций).
- Отказаться от `FDIVEMenuSection` в пользу неявных групп — нет: явные секции с порядком массива `Sections` (поля `SortOrder` нет) — это ровно то, что просил исходный запрос («группировать как захочется»).

**Что упростить стоит (сводится к §2–§4):**
- один код-путь proxy-drive вместо двух (§4.1) — *минус* целый класс расхождений;
- один валидатор вместо трёх (§4.2);
- один гейт admin вместо двух (§4.3);
- минус мёртвые Anchor-ветки, минус `DIVEComponentResolve`, минус (или донести) `bChecked`.

После этого плагин станет *проще*, чем сейчас, без потери единой функции.

---

## 7. Расхождения документации (сводно)

| Документ | Проблема |
|---|---|
| `DeviceInteractionModel.md` | Режим `Logical` описан как существующий (§3.3); «anchor focus on primary» — недостижимо (§3.1) |
| `README.md` | «anchor focus» в описании primary — недостижимо |
| `QUICKSTART.md` | «hidden meshes need tag DIVE.PickProxy» не соответствует коду до фикса B3; «Prefer Action Catalog» без оговорки о B1 |
| `Audit_DeviceActions_Architecture.md` | Тезис о работающем BP world-context верен только для component-hosted действий (B1) |

---

## 8. План действий по приоритетам

**P0 — до любого нового авторинга через каталог:**
1. B1: world-context для действий через `FDIVEActionContext`/`ExecutionWorld`; встроенные действия резолвят подсистему из `Context.DeviceHost`.
2. B2: `CanExecute` в primary-пути (3 строки).
3. B3: `bHiddenInGame` в `IsPrimitivePickable`.

**P1 — ближайший цикл:**
4. B4: сохранение/восстановление overlay-материала.
5. §4.1: единый код-путь proxy-drive через `UDIVEProxyDriveForwardAction`.
6. §4.2 + B6: общий валидатор биндингов в `DIVECore`, один контракт по встроенным секциям.
7. §3.1: удалить мёртвые Anchor-ветки, поправить README/DeviceInteractionModel.
8. §5.1: `TSubclassOf` для контекстного меню и session chrome.

**P2 — по мере необходимости:**
9. §5.2: `GetOptions` для `SectionId`; details customization для `PrimaryActionIndex`.
10. §4.3: убрать admin-гейт из методов подсистемы.
11. §4.4: подписка на `OnWorldBeginTearDown` (или миграция на `UWorldSubsystem`, если появится стриминг уровней).
12. §4.6: реестр якорей по `ForEachDeviceActor`.
13. §3.2–3.5, B5, B7: мелочи (bChecked, Logical в доках, `DIVEComponentResolve`, ensure на continuous-состоянии, подавление зума при меню).

---

## 9. Что в v0.8 сделано хорошо (чтобы не потерять при правках)

- Объектная модель действий и её редакторская обвязка (фабрики BP-классов действий/условий, категории ассетов) — соответствует изначальному запросу и лучшим паттернам UE.
- Дельта-корректный сид дефолтных биндингов через `PostInitProperties` — редко у кого сделан правильно; здесь сделан (очистка автором переживает перезагрузку).
- `IsDataValid` + `DIVE.DumpDevice` + редакторский Scan — три уровня диагностики; после консолидации валидатора станут согласованными.
- Восстановление состояния (`bWasHiddenInGame`, input mode, view target) везде через записи «что было», а не через сброс в дефолт — правильный паттерн; B4 (overlay) — единственное место, где он нарушен.
- Разделение «DIVE — адаптер, устройство — источник истины» выдержано по всему коду: подсистема нигде не пишет в состояние устройства напрямую, только через действия/интерфейсы.

---

## 10. Статус закрытия — Health Pass (2026-08-11)

Все пункты плана `dive_v0.8_health_pass` реализованы за один проход:

| ID | Суть | Статус |
|----|------|--------|
| B1 | `ExecutionWorld` + `FDIVEActionWorldScope`; `ResolveSessionSubsystem` из `Context.DeviceHost` | ✅ |
| B2 | `CanExecute` в primary-пути (`DIVESessionPickOps`) | ✅ |
| B3 | `IsPrimitivePickable`: добавлена проверка `!bHiddenInGame` | ✅ |
| B4 | Сохранение/восстановление `OverlayMaterial` (`PickHoverPreviousOverlay`) | ✅ |
| Physical | Device-proxy путь переведён на `InternalProxyDriveAction` / continuous-slot; `EDIVEActivePhysicalDriveKind::DeviceProxy` удалён | ✅ |
| Validator | `DIVEActionBindingValidation` — единый util для CatalogAsset, Inspectable, DeviceScan | ✅ |
| Anchors | Мёртвые `EDIVEFocusKind::Anchor`-ветки удалены; `BuildSemanticRegistry` расширен на `ForEachDeviceActor` | ✅ |
| QoL widgets | `SessionChromeWidgetClass` на `DIVEInputComponent`; `ContextMenuWidgetClass` на `DIVEContextMenuUIComponent` | ✅ |
| QoL sections | `GetAvailableSectionIds` UFUNCTION + `GetOptions` мета на `SectionId` | ✅ |
| Admin gate | Убран из `ToggleMeshPhysicsForTarget` / `DeleteMeshForTarget`; остался только в `CanExecute` действий | ✅ |
| TearDown | `OnWorldBeginTearDown` → `EndSession` в `UDIVESessionSubsystem` | ✅ |
| bChecked | Проброшен в `FDIVEContextMenuEntry`; суффикс убран из actions; **UI:** префикс `✓`; helper `FormatActiveLabelSuffix` удалён (verify pass) | ✅ |
| ComponentResolve | `DIVEComponentResolve.h` удалён; call-sites переведены на `SharedComponentResolve` напрямую | ✅ |
| ensure | `ensure(!bInteractionActive)` в `MarkInteractionActive` | ✅ |
| Suppress zoom | `HandleZoomIn/Out/NavigateBack` подавляются при открытом меню (`ShouldSuppressSessionInput`) | ✅ |
| Docs | `DeviceInteractionModel.md` — Logical = planned, anchor-primary убран; `QUICKSTART.md` — pickability + hover overlay | ✅ |

**Follow-up (post–Health Pass):** отрисовка `bChecked` в контекстном меню закрыта; `KDPEditor` Win64 Development собирается (исправлены UHT: `BlueprintNativeEvent` в public-секции `UDIVEDeviceAction`; include `DIVEInspectableComponent` в PhysicalDriveOps; `FDataValidationContext::GetIssues()` в DeviceScan).

---

## 11. Закрытие оставшихся долгов (post–Health Pass)

| Долг | Суть | Статус |
|------|------|--------|
| PrimaryActionIndex UI | `FDIVEActionBindingCustomization` — combo display-имён Actions (`DIVEUnrealEditor`) | ✅ |
| WorldSubsystem | `UDIVESessionSubsystem` → `UWorldSubsystem` + все GetSubsystem call-sites | ✅ |
| NewObject defaults | CDSOs убраны; сид через `NewObject` только в `SeedDefaultBindingsIfNeeded` | ✅ Health Pass 2 (2026-08-14): конструктор больше не `CreateDefaultSubobject`; сид `NewObject` с теми же именами субобъектов |
| Auto smoke | `DIVE.Actions.ExecutionWorld`, `DIVE.Actions.CanExecuteGate` (+ прежние DefaultBindings/BindingResolve) | ✅ |

**Audit_v08 code/doc debts = 100%.** Runtime PIE smoke остаётся operational checklist (не code debt):

- [ ] Catalog Focus/Isolate в PIE
- [ ] Primary + Condition=false не срабатывает
- [ ] Isolation + pick по «пустому» (hidden не кликается)
- [ ] Hover не затирает device overlay
- [ ] Physical device `IDIVEProxyDrive` + GRIP pawn bridge
- [ ] Widget class override в Details
- [ ] Clear Bindings → save/reload → empty
- [ ] PrimaryActionIndex combo в Details (Catalog + Inspectable)
- [ ] Session start после миграции на World Subsystem (BP: Get World Subsystem, не Game Instance)

### Verify/cleanup pass (post-debts)

- Removed dead `FormatActiveLabelSuffix`
- Docs: remaining «anchor focus on primary» lines cleared (`ARCHITECTURE`, `DeviceInteractionModel`); B5 text → world subsystem; QUICKSTART notes World Subsystem + ExecutionWorld
- `FDIVEActionBindingCustomization`: AccessRawData fallback + `SetSelectedItem` after Actions rebuild
- Dropped unused `Engine/GameInstance.h` includes in Input/DebugDump
