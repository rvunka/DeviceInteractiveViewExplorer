# Аудит DIVE — рудименты, разрывы, расчёты, тесты, QoL + стратегия интерактивных органов управления

> **Дата:** 2026-08-19
> **Плагин:** `DeviceInteractiveViewExplorer` v0.8-dev
> **Охват:** все модули (`DIVECore`, `DIVERuntime`, `DIVERuntimeDev`, `DIVEUncooked`, `DIVEUnrealEditor`), `.uplugin` / `Build.cs` / `Config/Engine.ini`, вся документация плагина, сиблинг `DIVEGRIPBridge`, стыки с ACTS / GRIP / хостом.
> **Эталон:** [`Docs/Plugin_Architecture_Principles.md`](../../../../Docs/Plugin_Architecture_Principles.md).
> **Предыдущие аудиты (не переписывать, сверять статус):** [`Audit_DeviceActions_Architecture.md`](Audit_DeviceActions_Architecture.md), [`Audit_v08_CodeHealth_QoL.md`](Audit_v08_CodeHealth_QoL.md), [`Audit_2026-08-14.md`](Audit_2026-08-14.md), [`Audit_DIVE_Compliance_Deep_Architecture_2026-08-17.md`](Audit_DIVE_Compliance_Deep_Architecture_2026-08-17.md).
> **Смежный контекст:** GRIP-аудит [`Audit_GRIP_Deep_Modularity_NAnchor_2026-08-19.md`](../../../GraspRigidbodyInertialPhysics/Docs/Audits/Audit_GRIP_Deep_Modularity_NAnchor_2026-08-19.md).
> **Код не менялся.** Доказательный снимок + стратегическое решение по интерактивным частям (§10).

Приоритеты: **P0** — ломает авторинг/валидацию без обхода; **P1** — контракт врёт или разъедется; **P2** — гигиена, QoL, эффективность; **Info** — зафиксировать, не действие.
Confidence: **High** — проверено по конкретным строкам этим проходом; **Medium** — отчёт субагента с цитатой кода, повторно не перечитывалось.

---

## 0. Вердикт

Каркас v0.8 после закрытий 2026-08-17 (Feature Root `UDIVEPlayerComponent`, dump в `DIVEUncooked`, UBT-native plugin detect, `FDIVECameraSettings`, `DIVE.Session.Lifecycle`) — **зрелый и не требует переделки**. Все закрытия предыдущих аудитов подтверждены кодом, регрессий нет (§2).

Найдено новое:

- **1 P0 (B1):** единый валидатор секций считает встроенные `Standard`/`Admin` «уже известными» **до** обхода авторских секций — а сид `SeedDefaultBindingsIfNeeded` сам создаёт секции с этими ID. Итог: **каждый Inspectable с дефолтным сидом проваливает `IsDataValid` и Scan** с ошибками «Duplicate SectionId 'Standard' / 'Admin'».
- **1 P1 (B2):** value-HUD мёртв на встроенных путях — `OnValueChanged.Broadcast` не вызывается ни из одного C++ места плагина, у `IDIVEProxyDrive` вообще нет канала значения, а QUICKSTART/ARCHITECTURE/DeviceInteractionModel утверждают «device `IDIVEProxyDrive` feeds it».
- **Ключевой факт для стратегии (§10):** `IDIVEProxyDrive` / `IDIVEDeviceControlRegistry` имеют **ноль имплементоров** во всём workspace. Контракт Physical-режима для органов управления существует три версии подряд, но ни разу не был отработан — и в текущем виде недостаточен для крутилок/гаек (нет значения, нет контекста в `CanProxyDrive`, нет camera-basis для маппинга дельты, резолв не ходит по иерархии устройства).

**Ответ на стратегический вопрос (крутилки, гайки), ревизия 2026-08-20 (§10.8):** двухъярусная стратегия. **Ярус 1 (сейчас):** «меш + унифицированный Action» — стандартизированные continuous-действия (C++ в DIVERuntime рядом с Focus/Isolate) в Default-режиме; состояние — transform самого меша и/или устройство, **не** инстанс действия. Механика жеста для этого уже готова и отработана. **Ярус 2 (при VR-паритете / реальной физике):** control-компоненты на устройстве + доращенный `IDIVEProxyDrive`; инфраструктура для этого перечислена в §10.8.3. «Отдельный ассет вторым слоем» отвергнут в обоих ярусах. Первоначальная версия рекомендации (§10.2–10.7) сохранена как ход анализа; итог — §10.8.

---

## 1. Метод

Прочитаны все исходники пяти модулей и `DIVEGRIPBridge`, `.uplugin`, `Build.cs`, `Config/Engine.ini`, все документы `Docs/`, README, предыдущие аудиты. Использование каждого подозрительного символа проверено grep'ом по всему workspace (оба корня). P0/P1-находки перепроверены вручную по полной цепочке вызовов (сид → gather → валидатор; delegate → broadcast sites; Physical routing → resolve). Git-история недоступна — репозиторий в `KDP` не инициализирован (отмечено в §8).

---

## 2. Закрытое ранее — подтверждено кодом, не переоткрывать

| Закрытие (аудиты 08-11…08-17) | Подтверждение 2026-08-19 |
|---|---|
| B1–B4 health pass (ExecutionWorld, CanExecute на primary, `bHiddenInGame` в pick, overlay restore) | `ExecuteResolvedAction` + `FDIVEActionWorldScope`; `DIVEInspectableComponent.cpp:207-209`; `PickHoverPreviousOverlay` |
| Единый валидатор `DIVEActionBindingValidation` | Catalog / Inspectable / Scan зовут его (но см. **B1 этого аудита** — в самом валидаторе баг) |
| `UWorldSubsystem` + `OnWorldBeginTearDown` | `DIVESessionSubsystem.cpp:23-44` |
| `InternalProxyDriveAction` — единый continuous-слот | `DIVESessionPhysicalDriveOps.cpp:91-107` |
| Feature Root: один `UDIVEPlayerComponent` на pawn | `DIVEPlayerComponent.h:34-38`; старых `UDIVEInputComponent` / `UDIVEContextMenuUIComponent` / имён `DIVE_Input` в Source нет |
| GRIP provider — instanced UObject на Player | `DIVEPlayerComponent.h:117-124` |
| `FindOnPawn`: named / unique / N>1 → Warning + nullptr | `DIVEPawnPhysicalDriveResolve.cpp:46-91` |
| `FDIVECameraSettings` общий (Inspectable + Definition) | `DIVETypes.h`; `DIVEDeviceDefinitionAsset.h:21-22` |
| Dump в `DIVEUncooked`; Editor не линкует RuntimeDev | `DIVEUnrealEditor.Build.cs` |
| UBT-native plugin detect (`ProjectDescriptor` + `ReadAvailablePlugins`) | `DIVERuntimeDev.Build.cs:34-91` |
| Late possession poll; `IsSessionActive` → subsystem; якоря через `ForEachDeviceActor` | `DIVEPlayerComponent.cpp:110-114`; `DIVEInspectableComponent.cpp:485-495`, `:105-137` |
| bChecked в меню; widget class overrides; K2 без `LoadObject` всех BP | подтверждено |

Камерная математика (см. §6) — здоровая; переоткрывать «проверку расчётов камеры» в следующих аудитах не нужно, если код не менялся.

---

## 3. Новые баги

### B1 (P0, High). Валидатор секций ложно флагует дефолтный сид — каждый Inspectable Invalid

**Цепочка (проверена построчно):**

1. `SeedDefaultBindingsIfNeeded` при пустых `Sections` **авторит** две секции: `Standard` и `Admin` (с Header «Admin») — `DIVEInspectableComponent.cpp:40-50`. Сид срабатывает на CDO → значения копируются в каждый инстанс.
2. `GatherAuthoredSections` отдаёт их валидатору как авторские (`:533-563`, дедуп только компонент↔каталог).
3. `ValidateSections` **сначала** кладёт `kSectionStandard`/`kSectionAdmin` в `OutKnownSectionIds` («Built-in IDs are always valid», `DIVEActionBindingValidation.cpp:18-20`), а затем для каждой авторской секции проверяет `Contains` → обе дефолтные секции получают **ошибку** `Duplicate SectionId` (`:34-39`).
4. `UDIVEInspectableComponent::IsDataValid` (`:1409-1416`) → **Invalid**; Scan (`DIVEDeviceScan.cpp:128-138`) → те же 2 ошибки на каждом устройстве.

То же ударит по каталогу, где автор объявил секцию `Standard`/`Admin` ради Header — легальный по QUICKSTART приём (сид сам так делает).

**Почему не замечено:** smoke `DIVE.Player.IsDataValid` тестирует только Player; `IsDataValid` Inspectable'а автотестом не покрыт; Data Validation, видимо, не гоняется регулярно.

**Исправление:** built-in ID не пре-сидить в известные **до** обхода. Дубликат ловить только внутри авторского массива: пройти секции, поймать повторы между собой, после — добавить built-ins в `OutKnownSectionIds` для `ValidateBindings`. Плюс smoke на `IsDataValid` свежесозданного Inspectable (fixture с owner-актором) — сейчас дефолтное состояние компонента валидацией не покрыто вообще.

### B2 (P1, High). Value-HUD не питается ни одним встроенным путём; документация утверждает обратное

- `FOnDIVEActionValueChanged OnValueChanged` объявлен (`DIVEDeviceAction.h:174-175`), сессия на него подписывается (`DIVESessionSubsystem.cpp:598`) и пробрасывает в `OnInteractionValueChanged` → Player → `UDIVEValueReadoutWidget`.
- **`OnValueChanged.Broadcast` не вызывается нигде в C++** (grep по всему workspace: только Add/Remove). `UDIVEProxyDriveForwardAction` — единственный мост device-proxy → action-система — значений не эмитит; у самого `IDIVEProxyDrive` **нет канала значения** (интерфейс: `CanProxyDrive` / `Begin` / `ApplyDelta` / `End` — `DIVEProxyDrive.h:22-32`).
- Доки заявляют: QUICKSTART:101 «Device `IDIVEProxyDrive` / continuous actions feed it», ARCHITECTURE:160 и DeviceInteractionModel:291 «device `IDIVEProxyDrive` still is [fed]». **Код это не подтверждает.**
- **Уточнение (ревизия 2026-08-20):** хуже, чем казалось. `OnValueChanged` объявлен только `BlueprintAssignable` (`DIVEDeviceAction.h:174-175`) — без `BlueprintCallable` **BP-подкласс действия не может его бродкастить** (нода Call недоступна). `BlueprintCallable`-хелпера (`NotifyValueChanged`) в базовом классе нет — единственный BlueprintCallable там `NotifyInteractionCompleted`. Итог: value-HUD недостижим **ни из C++ плагина, ни из BP-действий** — только из внешнего C++-подкласса действия. Канал полностью мёртв.

**Исправление (связано с §10):** добавить `UFUNCTION(BlueprintCallable) NotifyValueChanged(float NormalizedValue)` на `UDIVEContinuousDeviceAction` (бродкастит `OnValueChanged` с текущим контекстом) — это оживляет HUD и для Action-пути, и для proxy-пути (§10.5, V1); поправить три документа. Минимум на сейчас — убрать из доков «device IDIVEProxyDrive feeds it».

### B3 (P2, High). Колесо мыши во время drag: заблокировано и никуда не форвардится

`HandleZoomIn/Out` ранний выход при `IsProxyDriving()` (`DIVEPlayerComponent.cpp:729, 745`). `HandlePawnPhysicalGrabHoldDistanceScroll` существует (`:1292-1298`), но в production его зовёт **только** Legacy Dev wheel-glue (`DIVEGripLegacyDevQuery.cpp:80`). В таблицах `IA_DIVE_*` (ARCHITECTURE §Enhanced Input, QUICKSTART §4) этого хэндлера нет — EI-хост без чтения исходников не узнает, что дистанцию хвата надо вешать на отдельный IA. Либо форвардить Zoom → GrabHoldDistanceScroll при активном pawn-drive, либо задокументировать IA в обеих таблицах.

### B4 (P2, High — Medium по последствиям). K2 AssetRegistry-хуки не снимаются

`EnsureActionClassMenuRefreshHooks` (`K2Node_DIVEActionEvent.cpp:229-261`): `static bool bHooksInstalled` + четыре `AddLambda` на `OnFilesLoaded/OnAssetAdded/Removed/Updated` без хранения handle и без unregister на shutdown модуля. Hot-reload / выгрузка `DIVEUncooked` → колбэки в мёртвый код. Типичный editor-хук, но фикс дешёвый: хранить `FDelegateHandle`, снимать в `ShutdownModule`.

### B5 (P2, Medium). Контекстное меню и DPI

Позиционирование виджета мешает пиксельные координаты курсора (`GetMousePosition`) и slate-координаты `SetPosition` со снапом по viewport scale (`DIVEContextMenuWidget.cpp:30-43, 216-294`). При DPI ≠ 1.0 возможен дрейф позиции меню. Проверить на 125–150% Windows scale; если дрейф есть — конвертировать через `USlateBlueprintLibrary::AbsoluteToViewport` / единый scale.

---

## 4. Мёртвый код и рудименты

| ID | Что | Где | Вердикт |
|----|-----|-----|---------|
| D1 | **Именованный pawn-drive resolve не используется в production** | Сессия зовёт `FindOnPlayerController(PC)` без имени (`DIVESessionPhysicalDriveOps.cpp:117`); default `NAME_None` (`DIVEPawnPhysicalDriveResolve.h:18-21`); имя передают только smoke-тесты. На Player **нет** UPROPERTY имени | P2. Санкционированный стек (Player = единственный имплементор) работает; но при N>1 у хоста **нет способа** указать нужный — Physical-путь падает с Warning. Либо добавить `PawnPhysicalDriveComponentName` на Player/сессию, либо задокументировать «ровно один имплементор — контракт» |
| D2 | `OnValueChanged` — C++-канал без единого эмиттера | `DIVEDeviceAction.h:175` | Не удалять (BP-surface + §10), но это рудимент обещания — см. B2 |
| D3 | 9× `GetEffective*` тонких обёрток | `DIVEInspectableComponent.h` | Info. BP-surface; C++ давно на `GetEffectiveCameraSettings()`. Кандидат на deprecate после Reference Viewer |
| D4 | Устаревший комментарий «or GRIP Input» | `DIVEGripLegacyDevQuery.h:9` | GRIP Input-компонента больше не существует (Rig `Handle*`). Поправить строку |
| D5 | `InputCore` в `DIVEUnrealEditor.Build.cs:30`; `EditorFramework` вероятно транзитивен | Build.cs | P2/Low. Снять `InputCore`, проверить сборку |
| D6 | Противоречие внутри одного дока: sole-Hand fallback | `DeviceInteractionModel.md:287` («no sole-Hand fallback») vs `:288` («Single-Hand fallback: … uses that Hand once») | Код fail-closed, фолбэка **нет** (`DIVEGRIPPhysicalDriveProvider.cpp:245-253`). Удалить строку 288 |
| D7 | Пустая папка `Docs/Archive` | плагин | Удалить |
| D8 | `Problem_DuplicatedCatalogActions.md`, K2 `BoundEvent`, `kActionOpenDIVE`, `FindActionInstance`, Legacy KBM | — | **Не мёртвое** (historical / ExpandNode / контракт хоста / BP-surface / dev PIE) — подтверждено, не трогать |

TODO/FIXME/HACK в Source — по-прежнему 0. Legacy KBM: все 10 биндов сходятся с существующими `Handle*` Player'а (проверено потаблично), тиков нет.

---

## 5. Разрывы систем и декларативные противоречия

### S1 (P1, High). Optional GRIP в `.uplugin`: три источника говорят разное

Факт: `DeviceInteractiveViewExplorer.uplugin:25-29` **содержит** `GraspRigidbodyInertialPhysics` c `"Enabled": false, "Optional": true`.

| Источник | Утверждение |
|---|---|
| `ARCHITECTURE.md:158` | DIVE `.uplugin` **must not** depend on Optional GRIP |
| `README.md:17` | `.uplugin` lists GRIP as optional **so DIVERuntimeDev can link GRIPRuntime** — т.е. это осознанно |
| Аудит 08-17 §13 | «S1 Optional GRIP: **зависимость снята**» — не соответствует текущему файлу |

Либо декларация нужна (тогда поправить ARCHITECTURE и закрытие 08-17 неточно), либо нет (тогда убрать из `.uplugin` — RuntimeDev находит GRIP через `ReadAvailablePlugins`, а не через эту запись). Сейчас три нормативных текста несовместимы — это хуже любого из двух вариантов.

### S2 (P1, High). Proxy-резолв не ходит по иерархии устройства — разрыв с pick

`FindProxyDriveForHit` смотрит **только** `HitComponent->GetOwner()`: registry-компоненты этого актора → сам актор → первый `IDIVEProxyDrive`-компонент (`DIVEProxyDriveResolve.cpp:13-58`). Pick же собирает примитивы по **всей** иерархии `ForEachDeviceActor` (модульные устройства из child-акторов — поддерживаемый сценарий, ради него чинили B6/D6/D11 прошлых аудитов). Итог: registry на корне устройства **не найдётся** для хита по примитиву child-актора. Плюс: при нескольких registry — silent first-wins; сам owner-актор проверяется на `IDIVEProxyDrive`, но не на registry (асимметрия). Пока имплементоров ноль — латентно; при первом же реальном органе управления на модульном устройстве выстрелит. Исправить до/вместе с §10.

### S3 (P2, High). Physical-режим без affordance

Hover overlay — только Default (осознанно), но в Physical нет **никакой** подсветки «это можно крутить»: игрок тычет вслепую, промах — Warning в лог, который он не видит. DeviceInteractionModel §4.2 обещает «Physical may prefer grabbable primitives» как policy-пример — не реализовано и не помечено planned. Для тренажёра это UX-дыра: добавить (позже, вместе с §10) hover-фильтр в Physical: подсвечивать только примитивы, у которых резолвится drive/грабабельность.

### S4 (P2, Medium). Bridge: двойной `GrabHoldDistance` и одноразовый tick-order

Bridge хранит свой `GrabHoldDistance` отдельно от Hand'овского (скролл меняет только бриджевый; `DIVEGRIPPhysicalDriveProvider.cpp:591-611`); tick-prerequisites ставятся один раз в `InitializeOnPawn` — если Hand не зарезолвился к этому моменту, порядок не выставится никогда (`.cpp:62-78`). Оба пункта уже зафиксированы GRIP-аудитом 08-19 (GRIP-NEW-29) — здесь только сослаться, чинить на стороне Bridge.

### S5 (Info). ACTS ↔ DIVE

Без изменений с 08-14: production ACTS не знает DIVE; dev-suspend — tick-poll `IsSessionActive` в `ACTSRuntimeDev` вместо подписки на `OnSessionStarted/Ended`. Межплагинная тема, не solo-DIVE.

### S6 (Info). `GetDisplayState` vs `CanExecute`

`bEnabled = CanExecute(Context)`, `bVisible = !Condition || Evaluate` (`DIVEDeviceAction.cpp:49-56`): переопределённый `CanExecute` даёт «видимо, но серо», Condition — «скрыто». Логично, но неочевидно; одного абзаца в QUICKSTART §1 (после списка Condition) хватит.

---

## 6. Корректность расчётов — снимок

Проверено по коду (High): **математика камеры и пика здоровая, багов не найдено.**

| Область | Вердикт |
|---|---|
| Orbit yaw/pitch (`DIVECameraRig.cpp:184-224`) | Корректно: yaw вокруг world-up, pitch вокруг local-right, кламп ±89° |
| Zoom (`:85-94`) | Линейное масштабирование шага от `OrbitDistance/Ref`, кламп 0.08–4 — соответствует tooltip'ам |
| Focus fit / near floor (`DIVEInspectableComponent.cpp:343-376`) | `SphereRadius × FitMultiplier`, мягкий пол `Radius × NearPadding`, клампы — корректно |
| Blend (`DIVECameraRig.cpp:142-157`) | SmoothStep; `duration ≤ KINDA_SMALL_NUMBER` → снап, деления на ноль нет |
| Pick deprojection (`DIVEPick.cpp:59-116`) | Multi-hit + предпочтение pick-proxy — корректно |
| Frame-rate | Мышиные дельты по кадру (правильно для абсолютного ввода), blend на DeltaTime; зависимость от FPS не найдена |
| Bridge grab-depth ray (`SeedHoldDistanceAlongRay`) | Проекция на луч через Dot, фолбэк на дистанцию, клампы — корректно; ротация frame-compensated на стороне GRIP |
| Мелочь (Info) | Pitch-клامп через FRotator у полюсов может дать след roll — ограничено 89°, не чинить. DPI контекстного меню — см. B5 |

Один настоящий пробел расчётного уровня — **не в DIVE, а в контракте**: `ApplyProxyDriveDelta(FVector2D ScreenDelta)` отдаёт сырые пиксели без camera-basis; вся геометрия «экранная дельта → угол ручки» ложится на автора устройства без хелперов (§10.5, V3).

---

## 7. Тесты: лишнее и пробелы

10 smoke-тестов в `DIVESmokeTests.cpp` (Medium — построчный разбор субагента, выборочно перепроверен):

| Тест | Вердикт |
|------|---------|
| `DIVE.ContextMenu.DefaultBindings` | Оставить ядро (CDO-сид, Focus, AnyPrimitive, Admin header). **Выкинуть trivia-asserts:** пустой массив = 0, `enum ≠ enum`, константа `0.35f` |
| `DIVE.Actions.BindingResolve` | Оставить (специфичность, порядок, continuous flag). Убрать asserts на FName-константы секций; влить сюда `CanExecuteGate` |
| `DIVE.Actions.CanExecuteGate` | **Удалить** после слияния — один assert, дублирует Notify-путь BindingResolve |
| `DIVE.Camera.DefinitionBlendDuration` | **Удалить** — чистая проверка «UPROPERTY хранит присвоенное» |
| `DIVE.Actions.CollectMatchingPrimitives` | **Переписать или удалить** — CDO без owner всегда даёт 0; тест ничего не защищает. Нужна fixture с актором и примитивами |
| `DIVE.Actions.ComponentNameMatch` | Оставить — реальное покрытие Normalize/name-match |
| `DIVE.Actions.ExecutionWorld` | Оставить (skip-prone часть без мира — терпимо) |
| `DIVE.PawnPhysicalDrive.Resolve` | Оставить; N>1-ветка молча скипается без live world — false-green риск |
| `DIVE.Player.IsDataValid` | Оставить |
| `DIVE.Session.Lifecycle` | Оставить как intent; сильно skip-prone (нет PIE/PC → зелёный) — считать operational, не unit |

**Пробелы (важнее лишнего):** (1) `IsDataValid` **Inspectable** с owner-fixture — поймал бы B1 этого аудита; (2) session end → restore input mode / view target; (3) isolation + pick по скрытому; (4) menu-continuous жест (Begin → ignore-next-release → End). Пункты 2–4 — operational чеклист из Audit_v08 §11, всё ещё не автоматизированы.

---

## 8. Лишние файлы и папки

| Что | Вердикт |
|-----|---------|
| `Binaries/`, `Intermediate/` на диске | Норма (артефакты сборки); `.gitignore` их исключает. **Но git-репозитория в `KDP` нет вообще** — история изменений плагина не ведётся. Рекомендация вне скоупа плагина: инициализировать VCS |
| `Docs/Archive/` | Пустая — удалить |
| `Docs/Audits/` ×4 + этот | Оставить; предыдущие имеют перекрёстные статусы, служат «карта закрытого» |
| `Problem_DuplicatedCatalogActions.md` | Historical, помечен superseded — оставить |
| `Config/Engine.ini` (CoreRedirects K2 Editor→Uncooked) | Оставить, пока все BP-графы не пересохранены |
| Структура Source | Чистая: лишних пустых папок в модулях нет; тестовые типы корректно в `Private/Tests` |

---

## 9. «Можно проще, гибче, удобнее» — QoL-сводка

Каркас упрощать не надо (подтверждаю выводы 08-14/08-17: object actions, catalog+bindings union, WorldSubsystem, Ops-friend'ы, sibling Bridge — сохранять). Точечные улучшения, по убыванию ROI:

1. **B1-фикс + smoke на Inspectable `IsDataValid`** — сейчас валидация кричит на каждое здоровое устройство; авторы научатся игнорировать ошибки, и валидация умрёт как институт.
2. **Value-канал для proxy** (§10.5 V1) — закрывает B2 и открывает HUD для органов управления.
3. **Camera-basis в proxy-контексте** (§10.5 V3) — без этого каждый автор крутилки пишет деprojection сам.
4. `CanProxyDrive(Context)` — гейт по хиту на этапе Can (§10.5 V2).
5. `FindProxyDriveForHit` по `ForEachDeviceActor` + warning при N>1 registry (S2).
6. Wheel при pawn-drive: форвард в GrabHoldDistanceScroll или строка в таблицах IA (B3).
7. K2-хуки: хранить handle, снимать в Shutdown (B4).
8. Admin-сид: сейчас Simulate/Delete сидятся всегда, Shipping прячет только отображение — опция «Seed Admin defaults» (bool, default true) сняла бы вопрос «почему у меня в меню Delete Mesh» у новых авторов (Medium).
9. Доки: три value-HUD-строки (B2), sole-Hand строка (D6), GRIP-декларация (S1), таблицы IA (B3).
10. Тесты: −3 trivia-теста, +1 fixture-тест `IsDataValid` (§7).

Что чувствует автор устройства сегодня: путь «100 болтов → Unscrew BP → Catalog» — по-прежнему лучший в проекте (фабрики, GetOptions, `Targets: N`, Scan, Dump, K2 events). Реальная боль одна: **Physical-режим — контракт без ни одной реализации и без хелперов** (§10).

---

## 10. Стратегия: крутилки, откручиваемые гайки, регулируемые части

### 10.1. Постановка

Вопрос: реализовывать интерактивные двигаемые/регулируемые части (крутилки, гайки, слайдеры, тумблеры) как
**(A)** меш + унифицированный/стандартизированный Action;
**(B)** отдельный ассет, интегрируемый «вторым слоем» в сессию DIVE;
**(C)** иной вариант.

Факты, от которых отталкиваемся (все проверены этим аудитом):

- Контракт `DeviceInteractionModel.md` §1: «Interactive parts are not built for DIVE. They are device prefabs… DIVE must not become the source of truth for knob position». Контракт правильный — и **ни разу не реализованный**: имплементоров `IDIVEProxyDrive`/`IDIVEDeviceControlRegistry` в workspace ноль.
- Инстансы каталожных действий **общие** на все устройства (один DataAsset — сто устройств); per-target состояние на инстансе действия запрещено (`DIVEDeviceAction.h:97` + `ensure(!bInteractionActive)`).
- GRIP (по аудиту NAnchor 08-19) — grab-движок: руки, PD-силы, drive-lease. **DOF-констрейнтов, значений, детентов в GRIP нет и не будет** — констрейнты на устройстве.
- Physical-роутинг сессии готов: pick → device proxy (через continuous-слот) → pawn GRIP → Warning (`DIVESessionPhysicalDriveOps.cpp:63-139`).

### 10.2. Вариант A — «меш + унифицированный Action»: не годится как источник истины

Стандартизированный `UDIVEContinuousDeviceAction` («UnscrewAction», «RotaryAction»), навешенный каталогом на тегированные меши, разбивается о три стены:

1. **Состояние.** Угол ручки / прогресс откручивания гайки должен жить между сессиями и жестами. На инстансе действия нельзя (shared по контракту). Значит, действию всё равно нужен компонент состояния на устройстве — и тогда действие лишь адаптер, а «источник» уже вариант C.
2. **VR/GRIP-путь идёт мимо действий.** Та же гайка в VR крутится рукой через констрейнт — каталожное действие в этом пути не участвует. Два пути к одному состоянию = состояние обязано жить на устройстве (DeviceInteractionModel §2 ровно об этом).
3. **Домен.** Звук трещотки, MESS-события, тренировочная логика «крышку можно снять после 4 гаек» — это устройство, не DIVE-Action.

**Но:** унифицированный Action — правильная **половина** ответа: как стандартный входной адаптер «screen delta → значение контрола» в Default-режиме и в меню (см. 10.4, слой 3). Просто он не хранит и не определяет состояние.

### 10.3. Вариант B — «отдельный ассет вторым слоем»: хуже

Отдельный asset-тип («ControlDefinition» DataAsset / оверлей интерактивных частей поверх сессии) означает:

- **второй параллельный реестр** рядом с Catalog+Bindings — вторая точка истины про «какие части интерактивны», расползание с pick/exclusions/hover;
- контролы становятся **DIVE-only**: VR/GRIP этот слой не видит, миссия «одно состояние — два входа» проваливается;
- команда уже проходила этот урок: «Action Definition DataAsset was tried and **rejected**» (`Problem_DuplicatedCatalogActions.md`) — второй источник истины не масштабируется.

Единственный случай, где ассет уместен, — **пресеты параметров** контрола (кривая детентов, лимиты, звуки) как DataAsset, на который ссылается компонент. Это не «слой в сессии», а обычные данные компонента.

### 10.4. Рекомендация (вариант C): control-компонент на устройстве + доращенный DIVE-адаптер

Три слоя, каждый уже предусмотрен архитектурой — не хватает только реализации и 4 доработок контракта (10.5):

```text
Слой 1 — ИСТИНА (device prefab, переиспользуемый компонент)
  U<Host>RotaryControlComponent   — крутилка: MinAngle/MaxAngle, детенты, NormalizedValue,
                                    OnValueChanged(device event), звук
  U<Host>LinearControlComponent  — слайдер/рычаг: ось, лимиты, снапы
  U<Host>ThreadedControlComponent — гайка/болт: TurnsToRelease, Progress,
                                    OnReleased → detach + SimulatePhysics (+grabbable)
  Владеет: значением, DOF-маппингом, лимитами, событиями, звуком, MESS-хуками.
  Регистрирует свои примитивы (какие меши крутят этот контрол).

Слой 2 — ВХОДЫ (адаптеры к одному и тому же состоянию)
  Монитор/DIVE Physical: компонент реализует IDIVEProxyDrive (Begin/Delta/End → значение)
                         или отдаётся через IDIVEDeviceControlRegistry на корне устройства
  VR/мир:                GRIP grabbable + Chaos constraint на движущейся части;
                         constraint ограничивает, компонент читает результат
  Гайка после срыва:     свободное тело → уже работающий pawn GRIP drag (Bridge)

Слой 3 — СЕМАНТИКА (Catalog/Bindings, как сейчас)
  Дискретные операции: «Открутить» как continuous-действие в меню/primary для
  Default-режима (адаптер зовёт тот же компонент), Condition «крышка после гаек»,
  K2 DIVE Action Event на устройстве. Ничего в модели действий менять не надо.
```

Где живут control-компоненты слоя 1: **не в `DIVERuntime`** (устройства не должны зависеть от DIVE ради своих контролов — «controls first, DIVE second») и **не в GRIP** (он grab-движок). Два честных варианта:

- **старт:** game module / host-слой KDP — быстро, без нового плагина; DIVE-адаптер — реализация `IDIVEProxyDrive` прямо на компоненте (интерфейс из `DIVECore`, это легальная зависимость устройства от Core);
- **целевой (при ≥2–3 типах контролов):** отдельный маленький плагин уровня устройств (например `DeviceControlKit`), зависящий только от `DIVECore` (интерфейс) и опционально дающий GRIP-маркеры — по образцу существующей сетки плагинов (ACTS/GRIP/MESS/DIVE + мосты).

Правило выбора входа на мониторе (уже в доке, подтверждаю): **детерминированный кинематический контрол (тренажёр, снапы) — device proxy (Option A из §7); свободные тела — pawn GRIP (Option B)**. Гайка использует оба последовательно: proxy пока на резьбе, GRIP после срыва.

### 10.5. Что дорастить в контракте DIVE (без этого слой 2 останется голым)

| ID | Доработка | Зачем | Объём |
|----|-----------|-------|-------|
| **V1** | Канал значения: `GetProxyDriveState(float& OutNormalized, FText& OutLabel) → bool` в `IDIVEProxyDrive` (или optional-интерфейс); `UDIVEProxyDriveForwardAction` опрашивает после каждого `ApplyDelta` и бродкастит `OnValueChanged` | Закрывает B2: HUD оживает для всех устройств бесплатно; доки перестают врать | ~40 строк + доки |
| **V2** | `CanProxyDrive(const FDIVEProxyDriveContext&)` — передать контекст | Гейт «этот примитив — мой» на этапе Can; сейчас Can слепой (`DIVEProxyDrive.h:22`) | Сигнатура + call-sites |
| **V3** | В `FDIVEProxyDriveContext` добавить camera-basis: `FVector ViewLocation; FRotator ViewRotation; FVector PickRayDir` (на Begin), и вместе с `ApplyProxyDriveDelta` давать хелпер `DIVE::MapScreenDeltaToAxisAngle(Delta, ViewBasis, AxisWS, Sensitivity)` в Core | Сейчас «экранная дельта → угол вокруг оси ручки» каждый автор выводит сам, с ручным deprojection. Это главный QoL-барьер крутилок | Struct + 1 хелпер |
| **V4** | `FindProxyDriveForHit`: обход по `ForEachDeviceActor` + Warning при N>1 registry (S2) | Модульные устройства; симметрия с pick | ~20 строк |
| V5 (опц.) | Physical-hover: подсветка примитивов, у которых резолвится drive (S3) | Affordance в тренажёре | Позже, после первого контрола |

Ничего из этого не ломает существующий API: V1/V2 — расширение BlueprintNativeEvent-интерфейса с дефолтами, V3 — новые поля struct.

### 10.6. Гайка — сквозной сценарий (проверка решения)

1. На устройстве: `ThreadedControlComponent` (TurnsToRelease=4, ось, звук трещотки), зарегистрированы меши гайки. Прогресс — на компоненте, переживает сессию.
2. Монитор: Physical-режим → LMB по гайке → сессия находит proxy (V4) → `Begin` (компонент запоминает ось в camera-basis из V3) → drag → `MapScreenDeltaToAxisAngle` → прогресс + `GetProxyDriveState` → HUD «2.5 / 4 об.» (V1) → на 4-м обороте компонент: detach, `SetSimulatePhysics(true)`, event `OnNutReleased`.
3. Та же сессия: гайка теперь свободное тело — следующий LMB-drag уходит в pawn GRIP drive (роутинг уже так работает), гайку можно снять и отложить.
4. VR (позже): те же меши grabbable, констрейнт-резьба на устройстве, компонент читает угол — прогресс общий.
5. Тренировочная логика: Condition «RemoveCover» проверяет `AllNutsReleased` на устройстве; строка меню «Снять крышку» появляется сама. Уже работает сегодня.

Ни один шаг не потребовал ни нового asset-слоя, ни изменений модели действий.

### 10.7. Фазовый план — **superseded ревизией §10.8 (2026-08-20)**

| Фаза | Содержимое | Критерий |
|------|-----------|----------|
| 0 | B1-фикс валидатора (не блокирует стратегию, но чистит авторинг) | Data Validation зелёная на дефолтном устройстве |
| 1 | Контракт: V1–V4 + правка трёх доков (B2, S1, D6) | Compile + smoke на `FindProxyDriveForHit` иерархию |
| 2 | **Первый референс-контрол** `RotaryControlComponent` (host-слой) + демо-девайс с крутилкой; PIE-чеклист Physical | Крутилка живёт: HUD, детенты, звук |
| 3 | `ThreadedControlComponent` (гайка: резьба → срыв → GRIP handoff) | Сценарий 10.6 целиком в PIE |
| 4 | Решение о выносе в `DeviceControlKit`-плагин (если контролов ≥3 и/или нужен второй проект); V5 Physical-hover | — |

### 10.8. Ревизия 2026-08-20 — переоценка Action-пути (после challenge)

Повторная построчная проверка механики действий показала, что §10.2 отверг вариант A слишком широко. Смешаны два разных утверждения: «Action не может быть **источником истины**» (верно) и «Action-путь не годится для крутилок/гаек» (**неверно** для монитора). Разбор заново.

#### 10.8.1. Что умеет Action-путь на самом деле (проверено по коду)

| Возможность | Подтверждение |
|---|---|
| **Полный drag-цикл в Default-режиме**, Physical не нужен | `ExecuteResolvedAction` (меню и primary) → `TryBeginContinuousAction` ставит `bProxyDriving=true`, kind=ContinuousAction (`DIVESessionSubsystem.cpp:414-426, 576-606`); tick Player'а гонит `UpdateInteraction(ScreenDelta, DeltaTime)` **каждый кадр в любом режиме** (`DIVEPlayerComponent.cpp:128-131`) |
| Два жеста из коробки | Hold-drag (primary) и модальный drag из меню (`bIgnoreNextPrimaryActionRelease`) |
| BP-класс с графом | Фабрики Content Browser; Event Graph, свои переменные/функции; per-binding параметры инстанса (Sensitivity/Min/Max/детенты — `EditAnywhere` на инстансе прямо в Details биндинга) |
| Прямой доступ к части | `Context.Target` (`UPrimitiveComponent`) — BP крутит `SetRelativeRotation` без каких-либо интерфейсов |
| Per-device инстансы | Component-hosted `Instanced`-массивы дублируются на каждый инстанс компонента; **shared только catalog-hosted** |
| Affordance | Hover overlay работает именно в Default (в Physical — нет, S3) |
| Вся авторская обвязка | Scan, `Targets: N`, Condition, K2 DIVE Action Event, специфичность primary |

Единственное, что в Action-пути реально сломано — value-HUD, и это баг B2 (уточнённый: `OnValueChanged` без `BlueprintCallable` — BP бродкастить не может). Фикс V1 (BlueprintCallable `NotifyValueChanged`) нужен **в первую очередь этому пути**.

#### 10.8.2. Паттерн «stateless mapper + transform-as-state»

Снимает главный аргумент §10.2 про состояние для большинства кинематических контролов:

- **Действие — бесстатусный маппер:** `UpdateInteraction` переводит ScreenDelta в дельту угла/хода и пишет её в **transform `Context.Target`**. На инстансе — только параметры (ось, чувствительность, лимиты, шаг детента), никакого рантайм-состояния → безопасно даже для shared catalog-инстансов и tag-биндинга на 40 гаек (у каждой гайки своё состояние = её transform).
- **Состояние = сам меш:** угол крутилки — relative rotation; прогресс гайки — трансляция по резьбе (pitch × обороты), т.е. многооборотный прогресс кодируется позицией. Переживает сессии (ничто в DIVE трансформы устройств не сбрасывает — проверено по EndSession/restore-путям).
- **Реакция устройства:** порог достигнут → действие зовёт `NotifyInteractionCompleted` + устройство слушает **DIVE Action Event** (K2) / `OnActionExecuted` — звук, MESS, «крышку можно снять». Устройство остаётся источником доменной истины; DIVE-действие — только глагол манипуляции.
- Ограничение паттерна: неограниченный многооборотный контрол **без** трансляции (энкодер) — состояние в transform не влезает; тогда map на устройстве (одна функция в device BP) или ярус 2.

Итого исправленная оценка варианта A: **пригоден как основной путь для монитора** — при условиях: kinematic (без Chaos-физики), Default-режим, состояние в transform/на устройстве. Не пригоден по-прежнему: как хранилище per-target состояния на инстансе и как путь для VR.

#### 10.8.3. Ответ на вопрос об инфраструктуре компонентного пути

Да, ярус 2 — это заметная инфраструктура, и это главный аргумент **не** начинать с него:

| # | Что строить | Оценка |
|---|-------------|--------|
| 1 | Контракт DIVECore V1–V4 (значение, Can(Context), camera-basis, резолв по иерархии) | ~1 день |
| 2 | Дом: host game module (быстро) или плагин `DeviceControlKit` (`.uplugin`, Build.cs, соответствие принципам) | 0.5–1 день |
| 3 | Компоненты Rotary/Linear/Threaded: DOF-маппинг, лимиты/детенты, регистрация «своих» примитивов, `IsDataValid`, editor-визуализация оси (стрелка как у Anchor) | 2–4 дня |
| 4 | VR-слой: GRIP grabbable-маркеры + Chaos-констрейнты на префабах + компонент читает результат | только при VR |
| 5 | Обвязка: расширение Scan (регистрация примитивов), Dump, smoke на резолв/значение | 1–2 дня |
| 6 | Референс-девайс + документация | 1 день |

≈1.5–2 недели против «сегодня» для яруса 1. Ярус 2 становится **обязательным** при любом из: (a) VR-паритет (одно состояние — рука и монитор); (b) настоящая физика/констрейнты (дверь с инерцией, пружинный тумблер); (c) контрол должен жить и без DIVE (мир, ACTS); (d) непрерывная телеметрия контрола в MESS. Миграция ярус 1 → 2 естественная: тот же `RotaryDriveAction` вместо записи в transform начинает звать компонент — авторинг каталога не меняется.

#### 10.8.4. Итоговая рекомендация (заменяет 10.4/10.7)

| Ярус | Что | Когда |
|------|-----|-------|
| **1 — сейчас** | Встроенные C++ continuous-действия в `DIVERuntime` рядом с Focus/Isolate: `UDIVERotaryDriveAction` (ось, лимиты, детенты, `NotifyValueChanged`) и `UDIVEThreadedDriveAction` (резьба: обороты → трансляция → на пороге detach + `SetSimulatePhysics` → дальше готовый GRIP pawn-drag). C++ — чтобы бродкастить value до фикса V1 и дать точные дефолты; авторы наследуют BP при необходимости | Первое же устройство с крутилкой |
| **Пререквизиты яруса 1** | B2/V1 (`NotifyValueChanged` BlueprintCallable), хелпер `MapScreenDeltaToAxis` (V3 — нужен обоим ярусам), B1-фикс валидатора | До/вместе с первым контролом |
| **2 — по триггеру** | Control-компоненты + V2/V4 + Physical-режим как вход (10.4–10.6 остаются верным описанием целевой архитектуры яруса 2) | VR / физика / контролы вне DIVE |
| **Никогда** | Отдельный ассет-слой контролов в сессии | — |

Примечание к доктрине: `DeviceInteractionModel.md` §3 относит knob/slider к «Physical mode + proxy». С ярусом 1 кинематические панельные контролы легально живут в **Default** (меню/primary + hover), Physical остаётся за свободными телами (GRIP) и будущим ярусом 2. При реализации яруса 1 обновить таблицу §3 дока, иначе она продолжит отправлять авторов в контракт с нулём реализаций.

---

## 11. План по приоритетам (только открытое)

**P0:**
1. B1 — валидатор секций: дубликаты ловить внутри авторского массива, built-ins добавлять после; smoke `IsDataValid` Inspectable с fixture.

**P1:**
2. B2 + V1 — `NotifyValueChanged` (BlueprintCallable) на continuous-действии + правка QUICKSTART:101 / ARCHITECTURE:160 / DeviceInteractionModel:291. Нужно обоим ярусам §10.8.
3. S1 — привести `.uplugin` ↔ ARCHITECTURE ↔ README к одному ответу про Optional GRIP.
4. S2/V4 — `FindProxyDriveForHit` по `ForEachDeviceActor`, warning при N>1 registry (актуально для яруса 2; дешёво — сделать сразу).
5. §10.8 ярус 1 — `MapScreenDeltaToAxis` хелпер + встроенные `UDIVERotaryDriveAction` / `UDIVEThreadedDriveAction` + правка таблицы DeviceInteractionModel §3 (это ответ на главный вопрос запроса; компонентный ярус 2 — по триггеру VR/физика, см. §10.8.3–10.8.4).

**P2:**
6. B3 — wheel при pawn-drive (форвард или док-таблицы IA).
7. B4 — K2 delegate handles + unregister.
8. B5 — проверить меню на DPI ≠ 1.0.
9. D1 — имя pawn-drive для N>1 или задокументированный контракт «ровно один».
10. D4/D5/D6/D7 — комментарий Grip query, InputCore dep, sole-Hand строка, пустой `Docs/Archive`.
11. §7 — минус 3 trivia-теста, влить CanExecuteGate, fixture для CollectMatchingPrimitives.
12. Admin-сид под опцией (QoL).

### Won't-fix this pass

| Тема | Почему |
|------|--------|
| God-фасады Session/Inspectable, Ops-friend'ы | Accepted всеми аудитами; Inspectable похудел до ~1260 LOC |
| Logical mode / merge Core+Runtime / GAS / клон каталога / mega-root | Отклонено 08-14 и 08-17, подтверждаю |
| Split Inspectable (BindingCatalog/PickPolicy) | P2 maintainability; трогать после §10 фазы 2, не раньше |
| PIE-автоматизация session restore | Operational чеклист, не code debt |
| Bridge dual GrabHoldDistance / tick-order | Зафиксировано GRIP-аудитом 08-19 — чинить на стороне Bridge |

---

## 12. Итог одной страницей

DIVE v0.8-dev после прохода 08-17 — **здоровый, самый вылизанный плагин связки**: закрытия подтверждены, камера и pick математически корректны, мёртвого production-кода почти нет, Legacy/K2/доки в порядке.

Новое существенное: **(1)** валидатор секций конфликтует с собственным дефолтным сидом — каждое устройство формально Invalid (P0, фикс на полчаса); **(2)** value-HUD-контракт обещан тремя документами и не реализован ни одним кодовым путём; **(3)** декларация Optional GRIP живёт в трёх несовместимых версиях; **(4)** smoke-набор содержит тривию и скрывает пробел, который и пропустил P0.

Главный стратегический вывод (ревизия 2026-08-20, §10.8): **Physical-контракт для органов управления — чертёж без единого построенного здания, а вот Action-машинерия — работающая и отработанная.** Ответ на вопрос «крутилки и гайки» двухъярусный. **Ярус 1 (сейчас):** унифицированные continuous-действия в Default-режиме — drag-цикл, два жеста, hover, Condition, K2-события уже готовы; действие — бесстатусный маппер, состояние — transform самого меша (угол крутилки, трансляция гайки по резьбе) и/или устройство; для гайки: резьба → drag с HUD → на пороге detach + физика → готовый GRIP pawn-drag. Пререквизиты: V1 (`NotifyValueChanged` — без него HUD недостижим даже из BP), хелпер camera-basis, фикс B1. **Ярус 2 (по триггеру VR-паритета / реальной физики / контролов вне DIVE):** переиспользуемые control-компоненты на устройстве + доращенный `IDIVEProxyDrive` (V2/V4) — инфраструктура ~1.5–2 недели, перечислена в §10.8.3. «Отдельный ассет-слой в сессии» отвергнут в обоих ярусах (второй источник истины, DIVE-only).

Следующий агент: не переоткрывать §2; чинить B1 до любого нового авторинга; ярус 1 из §10.8.4 — прежде чем строить первое реальное устройство с крутилками; §10.2–10.7 читать только как ход анализа, итог — §10.8.
