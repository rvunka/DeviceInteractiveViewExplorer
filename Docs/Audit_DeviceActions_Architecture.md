# Аудит: модель действий DIVE — от FName-каталога к объектам действий

> **Аудитория:** мейнтейнеры DIVE, авторы устройств.
> **Статус:** **реализовано (v0.8)** — §12 = closure checklist (Bindings/Sections + Catalog; Content Browser factories for Catalog / Device Action / Continuous / Condition; no sample Unscrew module). §§1–7 и таблицы §10 — **исторический диагноз**, не текущий контракт.
> **Нормативные доки сейчас:** `QUICKSTART.md`, `ARCHITECTURE.md`, `DeviceInteractionModel.md`.
> **Движок проекта:** UE **5.8.1**.
> **Связано:** `Problem_DuplicatedCatalogActions.md` (historical), `../../../Docs/Plugin_Architecture_Principles.md`.
> **Прецеденты (на этапе проектирования):** Enhanced Input Instanced triggers/modifiers; ATSEP StateMachine `EditInlineNew`.

---

## 1. Резюме

> **Исторический снимок.** Ниже — проблемы модели ActionSets/Roles/`IDIVEDeviceActionHandler`. Текущая модель — `UDIVEDeviceAction` + bindings; см. §12.

Текущая система «ActionSets + Roles + `IDIVEDeviceActionHandler`» решила проблему дублирования
строк каталога (N болтов × M действий), но осталась **строково-типизированной сквозь весь стек**:
`SetId`, `ActionId`, `RoleId`, `MatchValues`, `CatalogKey`, `Is_{CatalogKey}_{ActionId}` — всё это
`FName`, которые автор вписывает руками и которые сверяются только валидацией и дампами.
Группировка в ActionSet существует **только на этапе авторинга**: в точке диспатча
(`HandleDeviceAction(CatalogKey, ActionId, Target, bActiveBefore)`) она сознательно вырождается в
пару имён. Параметры домена (витки, инструмент, шаг регулятора) объявлены «out of scope» каталога
и размазываются по Blueprint устройства. Обработка сведена в одну God-функцию с гигантским
Switch, которую нельзя декомпозировать (CollapsedGraph не живёт в функциях). Админ-действия
захардкожены и в подсистему, и в отдельную секцию меню. Мгновенные действия (каталог) и
континуальные (proxy drive) — два несвязанных мира, хотя домен (Unscrew, CircularSwitch) требует
именно континуальных действий из каталога.

**Предлагаемое направление:** действия — полиморфные инстансируемые объекты
(`EditInlineNew` / `DefaultToInstanced` `UObject`), как триггеры и модификаторы в Enhanced Input.
Один класс — один паттерн взаимодействия (Unscrew, CircularValue, Toggle…); один инстанс с
параметрами — на группу мешей; таргетинг группы — наследник сегодняшних Roles; секции меню —
данные, а не хардкод; встроенные и админские действия — те же объекты, поставляемые плагином как
классы (не Content). Континуальные действия получают lifecycle Begin/Update/End и со временем
поглощают `IDIVEProxyDrive` как частный случай.

Провал прошлой попытки с DataAsset — не приговор объектам. Провалился **выбранный механизм**
(наследование класса ассета под каждое действие), а не идея: `Instanced`-свойства с
`EditInlineNew`-классами дают ровно то, чего не хватало — интерактивное добавление
типизированных параметров в редакторе без создания нового класса ассета. Это штатный механизм UE,
на нём построен Enhanced Input, и его же вы уже использовали в ATSEP StateMachine.

---

## 2. Карта текущей системы

### 2.1. Данные (на `UDIVEInspectableComponent`)

```text
PickActionSets : TArray<FDIVEPickActionSet>
  SetId : FName                      ← вписывается руками
  PrimaryActionId : FName            ← должен совпасть с ActionId строки (валидация)
  Actions : TArray<FDIVEPickContextMenuAction>
    ActionId : FName                 ← вписывается руками
    DisplayName / bEnabled / bToggleActiveSuffix

PickContextMenuRoles : TArray<FDIVEPickContextMenuRole>
  RoleId : FName                     ← диагностика
  Priority : int32                   ← winner-take-all при нескольких совпадениях
  MatchMode : Tag | ComponentName | PartId
  MatchValues : TArray<FName>        ← вписывается руками
  ActionSetId : FName                ← строковая ссылка на SetId
```

### 2.2. Поток исполнения

```text
Клик по строке меню / primary action
 → UDIVESessionSubsystem::ExecuteContextMenuAction(QualifiedActionId)
     ├─ if kContextFocus / kContextIsolate            ← хардкод плагина
     ├─ if kContextToggleMeshPhysics / kContextDeleteMesh  ← хардкод админ-действий
     └─ else → Inspectable->NotifyPickContextMenuAction
          ├─ разбор QualifiedActionId = "{CatalogKey}_{ActionId}"   ← строковая склейка
          ├─ QueryActorBoolState("Is_{CatalogKey}_{ActionId}")      ← рефлексия по имени
          ├─ IDIVEDeviceActionHandler::HandleDeviceAction(CatalogKey, ActionId, Target, bActiveBefore)
          │       ← на устройстве: ОДНА функция, Switch по ActionId, потом по CatalogKey
          └─ TryToggleActorBoolProperty("Is_*") + поиск "set_*" сеттера ← рефлексия
```

Параллельный, не пересекающийся путь для континуальных взаимодействий:

```text
Physical mode: HandlePrimaryAction* → IDIVEProxyDrive::Begin/ApplyDelta/End (на компоненте устройства)
```

### 2.3. Зачем вообще ActionSet (ответ на вопрос)

Только для **дедупликации авторинга**: M строк действий пишутся один раз на сет, membership
(какие меши получают сет) — отдельным правилом Role. Это зафиксировано в
`Problem_DuplicatedCatalogActions.md`. В рантайме группировка **намеренно невидима**: контракт
диспатча — пара `(CatalogKey, ActionId)`. Ваше ощущение «имитации группировки» точное: сет — это
таблица авторинга, а не сущность модели. Побочный эффект — сет невозможно переиспользовать между
устройствами (он живёт в массиве конкретного компонента), а «настройки действия» в нём негде
хранить.

---

## 3. Проблемы

| # | Проблема | Где проявляется |
|---|----------|-----------------|
| P1 | **Строковая типизация сквозь стек.** 6 видов ручных `FName` (SetId, ActionId, RoleId, MatchValues, PrimaryActionId, CatalogKey) + склейка `{CatalogKey}_{ActionId}` как транспорт. Ошибки ловятся только `IsDataValid` и `DIVE.DumpDevice`. | `DIVETypes.h`, `DIVEConvention.h::MakeQualifiedPickContextMenuActionId` |
| P2 | **Группировка исчезает в диспатче.** ActionSet — только авторинг; интерфейс получает голую пару имён. Сущность «действие» в рантайме не существует. | `IDIVEDeviceActionHandler` |
| P3 | **Параметрам действия негде жить.** Витки Unscrew, требуемая отвёртка, шаг/диапазон регулятора — «out of scope, храните в BP устройства». Итог: параметры одного действия размазаны между каталогом (имя, лейбл) и графом устройства (данные, логика). | `Problem_DuplicatedCatalogActions.md` §Out of scope |
| P4 | **`Is_*` — рефлексивная магия.** Поиск bool-свойства по конкатенированному имени, обход `TFieldIterator`, вызов `set_*`-сеттера через `ProcessEvent`. Хрупко (переименование переменной BP молча ломает toggle), невидимо для рефакторинга, двойной источник правды (флаг на акторе + суффикс `*` в меню). | `DIVEInspectableComponent.cpp::FindActorBoolProperty`, `TryToggleActorBoolProperty` |
| P5 | **Хардкод встроенных и админ-действий.** Focus/Isolate/SimulatePhysics/DeleteMesh — ветки `if` в подсистеме + фиксированная секция в билдере меню. Автор не может их перегруппировать, переименовать, добавить свои «админские» действия в ту же секцию или убрать разделитель. | `DIVESessionSubsystem::ExecuteContextMenuAction`, `DIVEContextMenu.cpp::AppendStandardMeshEntries` |
| P6 | **God-switch в обработчике.** `HandleDeviceAction` — единственная точка входа для всех действий устройства. Идея разнести по CollapsedGraph провалилась (в функции их не добавить). QUICKSTART честно советует «Switch on ActionId, выносите в функции» — то есть проблема признана и переложена на автора. | `QUICKSTART.md` §1 |
| P7 | **Два мира: instant vs continuous.** Каталог умеет только «клик → выполнить». Proxy drive умеет Begin/Delta/End, но живёт в другом режиме (Physical), с другим авторингом (интерфейс/регистри на компоненте) и не появляется в меню. Домен же требует континуальных действий из каталога: Unscrew (держать и крутить), CircularSwitch (держать, крутить, видеть значение). | `IDIVEProxyDrive` vs `PickActionSets` |
| P8 | **Секции меню не авторятся.** Единственный механизм — автоматический разделитель между «встроенными» и «кастомными». Задача «сгруппировать действия как хочу, с прочерками» нерешаема данными. | `DIVEContextMenu.cpp::AppendSeparator` |

Отдельно: сегодняшний winner-take-all по Roles (одна выигравшая роль → один сет) означает, что
часть **не может получить действия из двух сетов** (например, «болт» + «маркированная деталь»).
С секциями это станет заметно быстрее.

---

## 4. Почему DataAsset-попытка провалилась и почему Instanced-объекты — не то же самое

Попытка «действие = DataAsset» упёрлась в два ограничения:

1. Чтобы добавить действию новый параметр (витки для Unscrew), нужен **новый класс** ассета —
   наследование `UDataAsset` с новыми `UPROPERTY`. Интерактивно, из редактора, поля не добавить.
2. Ассет — глобальный объект Content: на каждую группу болтов со своими параметрами пришлось бы
   плодить ассеты, и всё равно оставались N строковых ссылок membership.

Оба ограничения снимаются другим штатным механизмом — **инстансируемые полиморфные субобъекты**:

```cpp
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class UDIVEDeviceAction : public UObject { ... };

// на компоненте / в ассете:
UPROPERTY(EditAnywhere, Instanced, Category="DIVE|Actions")
TArray<TObjectPtr<UDIVEDeviceAction>> Actions;
```

В деталях компонента это выглядит как выпадающий список классов действий; выбрал
`DIVE Unscrew Action` — раскрылись **его** свойства (Turns, RequiredTool, DegreesPerPixel),
прямо inline, со значениями конкретного инстанса. Новый параметр = новый `UPROPERTY` в классе
действия (C++ или BP-подкласс) — не новый класс ассета и не новая строка каталога.

Это не экзотика, а тот самый паттерн, на который вы ориентируетесь:

- **Enhanced Input**: `UInputAction` держит `Instanced`-массивы `UInputTrigger` / `UInputModifier`;
  каждый триггер — `EditInlineNew`-объект со своими параметрами (Hold Time и т.п.).
- **Ваш ATSEP StateMachine**: `UDeviceStateBase` и `UDeviceScenarioDefinition` — `Abstract,
  Blueprintable, EditInlineNew`. Состояния и сценарии-предикаты добавлялись инстансами в деталях.
  Идея была правильной; DIVE может забрать её в зрелом виде.

---

## 5. Целевая архитектура

### 5.1. Ядро: объект действия

```cpp
/** Контекст исполнения — всё, что раньше передавалось именами, теперь объектами. */
USTRUCT(BlueprintType)
struct FDIVEActionContext
{
    TObjectPtr<AActor> DeviceHost;
    TObjectPtr<UDIVEInspectableComponent> Inspectable;
    TObjectPtr<UPrimitiveComponent> Target;   // конкретный болт из сотни
    FName TargetKey;                          // нормализованный ключ части (диагностика, сейвы)
    FDIVEFocusTarget PickTarget;
    FVector2D ScreenPosition;
    FHitResult PickHit;
};

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class UDIVEDeviceAction : public UObject
{
    /** Лейбл строки меню. Пустой = имя класса. */
    UPROPERTY(EditAnywhere) FText DisplayName;

    /** Опциональный предикат видимости/доступности (сам EditInlineNew-объект). */
    UPROPERTY(EditAnywhere, Instanced) TObjectPtr<UDIVEActionCondition> Condition;

    UFUNCTION(BlueprintNativeEvent) bool CanExecute(const FDIVEActionContext& Ctx) const;

    /** Динамическое состояние строки: лейбл-суффикс, checked, enabled. Замена Is_*-рефлексии. */
    UFUNCTION(BlueprintNativeEvent) FDIVEActionDisplayState GetDisplayState(const FDIVEActionContext& Ctx) const;

    /** Мгновенное действие. */
    UFUNCTION(BlueprintNativeEvent) bool Execute(const FDIVEActionContext& Ctx);

    /** Хук устройства: биндится в BeginPlay устройства или слушается системой обучения. */
    UPROPERTY(BlueprintAssignable) FOnDIVEActionExecuted OnExecuted;
};

/** Континуальное действие: держать и вести. Общий lifecycle с proxy drive. */
UCLASS(Abstract, Blueprintable)
class UDIVEContinuousDeviceAction : public UDIVEDeviceAction
{
    UFUNCTION(BlueprintNativeEvent) bool BeginInteraction(const FDIVEActionContext& Ctx);
    UFUNCTION(BlueprintNativeEvent) void UpdateInteraction(FVector2D ScreenDelta, float DeltaTime);
    UFUNCTION(BlueprintNativeEvent) void EndInteraction(bool bCommit);

    /** Для value-HUD (регулятор: текущее значение) и звуков устройства. */
    UPROPERTY(BlueprintAssignable) FOnDIVEActionValueChanged OnValueChanged;
};
```

Размещение: базовые типы и контекст — `DIVECore`; исполнение, меню, встроенные действия —
`DIVERuntime`. Классы — код, не Content: принцип 5 (`Plugin_Architecture_Principles.md`) не
нарушается, deployment без C++ сохраняется (проект может наследовать действия в Blueprint).

### 5.2. Таргетинг и биндинги (наследники Roles)

Механика membership из Roles — работающая и остаётся; меняется то, **на что** она ссылается:

```cpp
USTRUCT(BlueprintType)
struct FDIVETargetQuery      // бывшая Role без строковой ссылки на сет
{
    UPROPERTY(EditAnywhere) EDIVEPickRoleMatchMode MatchMode; // Tag | ComponentName | PartId
    UPROPERTY(EditAnywhere) TArray<FName> MatchValues;
    UPROPERTY(EditAnywhere) int32 Priority = 0;
};

USTRUCT(BlueprintType)
struct FDIVEActionBinding
{
    UPROPERTY(EditAnywhere) FName BindingId;      // диагностика/дампы (аналог RoleId)
    UPROPERTY(EditAnywhere) FDIVETargetQuery Targets;
    UPROPERTY(EditAnywhere, Instanced) TArray<TObjectPtr<UDIVEDeviceAction>> Actions;
    /**
     * Primary — индексом, НЕ объект-ссылкой: у details-панели UE нет пикера
     * «соседний инстансированный субобъект»; обычный TObjectPtr-проперти дал бы
     * бесполезный asset picker. Валидация: 0 <= Index < Actions.Num().
     */
    UPROPERTY(EditAnywhere) int32 PrimaryActionIndex = INDEX_NONE;
    UPROPERTY(EditAnywhere) FName SectionId;      // в какую секцию меню попадают строки
};
```

Сценарий «100 болтов»: тег `DIVE.Bolt` на мешах → один биндинг → внутри один инстанс
`UDIVEUnscrewAction {Turns=5, Tool=PH2}`. Вторая группа болтов с другой резьбой → второй биндинг
с **другим инстансом того же класса** `{Turns=8, Tool=T10}`. Конкретный болт приходит в
`Ctx.Target` — «один универсальный обработчик, просто передаём нужный болт», как и хотелось.

Строковыми остаются только `MatchValues` (тег/имя/PartId — это неустранимо: связь с level-данными)
и косметические `BindingId`/`SectionId`. Исчезают: `SetId`, `ActionSetId`, `ActionId`,
`PrimaryActionId`-строка, `CatalogKey` как транспорт, `Is_*`, `{Key}_{Action}`-склейка.

Разрешение конфликтов: вместо winner-take-all предлагается **объединение** биндингов,
совпавших по части, с сортировкой по секциям; Priority разруливает дубликаты одного действия
внутри секции. Это снимает ограничение «часть не может состоять в двух группах».

### 5.3. Секции меню — данные

```cpp
USTRUCT(BlueprintType)
struct FDIVEMenuSection
{
    UPROPERTY(EditAnywhere) FName SectionId;
    UPROPERTY(EditAnywhere) FText Header;     // опциональный заголовок
    UPROPERTY(EditAnywhere) int32 SortOrder;  // целевой снимок: в v0.8 поля нет; порядок = индекс в Sections
};
```

Билдер меню: собрать все совпавшие биндинги → сгруппировать строки по `SectionId` →
отсортировать секции → разделители между непустыми секциями. Хардкод-разделитель между
«встроенными» и «кастомными» умирает как частный случай.

### 5.4. Встроенные и админ-действия — те же объекты

Плагин поставляет **классы** (не Content): `UDIVEFocusAction`, `UDIVEIsolateAction`,
`UDIVESimulatePhysicsAction`, `UDIVEDeleteMeshAction`. Подсистема больше не диспатчит `if
ActionId == kContextFocus` — она исполняет объект, а объект зовёт публичный API подсистемы
(`FocusTarget`, `ToggleIsolationForTarget`, …).

Дефолтные Focus / Isolate / Admin — обычные `BuiltInBindings` на компоненте (те же
`FDIVEActionBinding` + классы действий), засеянные на CDO. Автор убирает Isolate из массива
Actions, переносит Focus в другую секцию или чистит массив целиком. Admin-строки остаются в
данных, но `GetDisplayState.bVisible` зависит от
`!UE_BUILD_SHIPPING && bEnableAdminContextMenuEntries` — политика показа, не отдельный
код-путь сборки меню. Тот же гейт в `CanExecute` админских классов.

### 5.5. Куда девается логика устройства (решение P6 / CollapsedGraph)

Три яруса вместо одного Switch:

| Ярус | Что | Где живёт граф |
|------|-----|----------------|
| A. Библиотечные действия | Unscrew, CircularValue, Toggle, PressButton — параметризованные классы, работают с `Ctx.Target` напрямую | C++ плагина / проекта (или BP-класс) — **у действия свой граф**, по определению |
| B. Реакция устройства | «болт открутился → снять крышку разрешено», звук, MESS, обучение | Устройство в BeginPlay биндится на `OnExecuted` / `OnValueChanged` инстансов — по одному аккуратному событию на действие вместо веток Switch |
| C. Уникальные действия | Одноразовая специфичная операция конкретного устройства | **BP-подкласс `UDIVEDeviceAction`** в Content проекта: граф лежит в классе действия, а не в EventGraph устройства |

Ограничение «CollapsedGraph нельзя положить в функцию» перестаёт быть релевантным: единицей
декомпозиции становится не граф внутри устройства, а **класс действия**. `IDIVEDeviceActionHandler`
как единая точка входа больше не нужен (остаётся только как legacy-мост на время миграции, §7).

**Честная оговорка про латентность.** `Execute` с возвращаемым `bool` в BP-подклассе — это
function graph: латентные ноды (Delay) и Timeline там недоступны — ровно то же ограничение, что
сегодня у `HandleDeviceAction`. Выигрыш ярусов A/C — декомпозиция, типизированные параметры и
инстансы, **а не** латентность. Длительное поведение реализуется двумя штатными путями:
континуальное действие (тикается сессией через `UpdateInteraction`) либо ярус B — устройство
реагирует на `OnExecuted` в своём EventGraph, где латентные ноды и Timeline доступны. Плюс
BP-подклассы `UObject` требуют `GetWorld()` через Outer, чтобы world-context-ноды (звук, spawn)
работали — базовый класс обязан это реализовать
(`GetOuter()`-цепочка до устройства).

**Доступ устройства к инстансам (недостающее звено яруса B).** Чтобы биндить `OnExecuted` в
BeginPlay, устройству нужен способ найти инстансы действий. Обязательный API:
`UDIVEInspectableComponent::GetActionInstances(FName BindingId)` и
`FindActionInstance(TSubclassOf<UDIVEDeviceAction>, FName BindingId = NAME_None)` — без них
ярус B превращается в ручной обход массивов.

### 5.6. Состояние: одна тонкость, о которую легко споткнуться

Один инстанс `UnscrewAction` обслуживает 100 болтов ⇒ **прогресс откручивания нельзя хранить в
полях инстанса**. Правило для библиотечных действий: инстанс = параметры (immutable в рантайме) +
стратегия; состояние per-target — либо `TMap<TWeakObjectPtr<UPrimitiveComponent>, FState>` внутри
действия (Transient, не сохраняется), либо (предпочтительно для симулятора) на компоненте
устройства, куда действие пишет через `OnValueChanged`/интерфейс. Второе согласуется с принципом
`DeviceInteractionModel.md` §1: DIVE не является источником правды о состоянии устройства.

Toggle-состояние (замена `Is_*`): `GetDisplayState` действия спрашивает устройство (делегат или
интерфейсный getter) либо читает своё per-target состояние. Никакой рефлексии по именам.

### 5.7. Унификация instant / continuous / proxy drive (ответ на «стоит ли унифицировать»)

Стоит. Unscrew на двух группах болтов и CircularSwitch на колонке и осциллографе — это **один
класс на паттерн взаимодействия, разные инстансы на параметры**. Ровно это и даёт объектная
модель; никакого специального механизма не нужно.

Интеграция с сессией:

```text
HandlePrimaryActionPressed (Default mode)
 → pick → PrimaryAction биндинга
     ├─ UDIVEDeviceAction          → Execute (по клику, как сейчас)
     └─ UDIVEContinuousDeviceAction → session занимает слот "active interaction":
          BeginInteraction → UpdateInteraction(ScreenDelta) пока держат → EndInteraction(commit)
          (+ опциональный value-HUD по OnValueChanged)
```

Слот «активного взаимодействия» в подсистеме уже фактически существует для proxy drive
(`bProxyDriving`, `ActiveProxyDrive`); он обобщается на континуальные действия. `IDIVEProxyDrive`
остаётся низкоуровневым бэкендом Physical-режима (его lifecycle побуквенно совпадает с
`UDIVEContinuousDeviceAction`), плюс плагин даёт `UDIVEProxyDriveForwardAction` — континуальное
действие, форвардящее в `IDIVEProxyDrive` цели. Долгосрочно proxy drive становится частным
случаем континуального действия, а Physical mode — политикой «primary сразу начинает
континуальное взаимодействие, минуя меню».

Важно сохранить принцип (`DeviceInteractionModel.md` §1): континуальное действие **двигает
состояние устройства**, DIVE лишь транслирует ввод. Отсюда правило разделения параметров:

- **на действии** — только параметры адаптации ввода (чувствительность, градусов-на-пиксель,
  требуемый инструмент как условие доступности);
- **на компоненте устройства** — доменные параметры и состояние (диапазон/шаг регулятора,
  текущее значение, угол, звук). `CircularValueAction` не владеет значением — он зовёт контрол
  устройства (тот же компонент, что крутит GRIP в VR).

Пограничный случай Unscrew: «число витков до откручивания» — данные взаимодействия, допустимо
на инстансе действия; но **прогресс** конкретного болта — состояние устройства (§5.6). VR/GRIP
путь к тем же контролам не затрагивается.

### 5.8. Каталог-ассет — основной дом авторинга (ревизия v2: было «опционально»)

```cpp
UCLASS(BlueprintType)
class UDIVEActionCatalogAsset : public UDataAsset
{
    UPROPERTY(EditAnywhere) TArray<FDIVEMenuSection> Sections;
    UPROPERTY(EditAnywhere) TArray<FDIVEActionBinding> Bindings; // Instanced-объекты внутри ассета — легально
};
```

**Почему ассет — primary, а inline на компоненте — secondary.** Аргументация двухслойная:

1. **Архитектурная (постоянная).** У DataAsset нет пары «CDO Blueprint → инстанс в уровне» —
   нечему рассинхронизироваться; один источник правды переиспользуется всеми инстансами
   устройства. Enhanced Input держит свои Instanced-триггеры/модификаторы именно в ассетах
   (`UInputAction`, `UInputMappingContext`) — по той же причине.
2. **Багозащитная (историческая, ревизия v3).** Инстансированные субобъекты **на компонентах в
   BP** — исторически самое багоопасное место движка: регрессия
   **[UE-282655](https://issues.unrealengine.com/issue/UE-282655)** (5.4–5.7: правки дефолтов
   `Instanced`-свойств не доходили до level-инстансов; целевой фикс — 5.8), UE-219732 /
   UE-222390 (тихие сбросы свойств при рекомпиляции, починены в 5.5), UE-164500 (синхронизация
   только после переоткрытия уровня). Проект теперь на **5.8.1**: смежные фиксы серии
   (CL 47597728, CL 46175874) подтверждены Epic как вошедшие в 5.8, но статус «Fixed» именно
   UE-282655 в публичном трекере не подтверждён — **перед тем как полагаться на inline-авторинг,
   выполнить 2-минутный репро** (BP-актор с Instanced-свойством → инстансы в уровне → правка
   дефолта в BP → проверить инстансы).

Итог: компонент ссылается на `UDIVEActionCatalogAsset` (паттерн уже есть —
`bUseDeviceDefinitionSettings` / `DeviceDefinition`); inline-биндинги на компоненте — удобство
для одноразовых устройств, полноценно допустимое после положительного репро UE-282655 на 5.8.1.
Прошлая проблема DataAsset не возвращается: класс ассета один и никогда не наследуется; вся
вариативность — в инстансах действий внутри.

### 5.9. Слой адаптации редактора

Минимум (фаза 1–2):
- `IsDataValid` на компоненте/ассете: пустые биндинги, null-действия в массиве, `PrimaryAction`
  не из списка, дубли `SectionId`, теги без совпадений — то же, что сейчас, но проверок меньше,
  потому что строковых ссылок меньше.
- **DIVE Scan Device** и `DIVE.DumpDevice` — перевести на биндинги (какая часть → какие биндинги
  → какие действия; cross-check остаётся).

Желательно (фаза 3+):
- Details customization: у `FDIVEActionBinding` — превью совпавших примитивов прямо в панели
  («Targets: 14 components»), кнопка подсветки во вьюпорте.
- Палитра классов действий в выпадашке с категориями (`meta = (Category)`).

Стандартный inline-редактор Instanced-массивов рабочий, хоть и не роскошный — Enhanced Input
живёт с ним же. Кастомизация — полировка, не пререквизит.

---

## 6. Что взять из ATSEP StateMachine, что — нет

| Из ATSEP | Вердикт |
|----------|---------|
| `EditInlineNew`-объекты как единица поведения (`UDeviceStateBase`) | **Взять** — ядро предложения |
| Сценарии-предикаты (`UDeviceScenarioDefinition::EvaluateScenario`) | **Взять** как `UDIVEActionCondition` (видимость/доступность действия: «Unscrew виден, пока крышка закрыта») |
| Делегаты «запросить событие/состояние» вместо жёстких вызовов | **Взять** — `OnExecuted`/`OnValueChanged` как канал устройство←действие |
| Приоритеты состояний через отдельный DataAsset (`UStatePriorityAsset`) | **Не брать** — приоритет лежит инлайн в биндинге; отдельный ассет — лишняя сущность |
| Машина состояний устройства целиком | **Не в DIVE и не свой плагин.** Состояния/последствия — доменная логика устройства (game layer); DIVE поставляет действия-глаголы, их последствия — подписки устройства. Если FSM устройств понадобится — сначала оценить движковый **StateTree** (штатный плагин Epic: инстансированные ноды-состояния/задачи/условия, ровно этот паттерн, поддерживается Epic'ом), а не портировать самописный StateMachine из ATSEP. Ценность ATSEP-кода уже исчерпана этим документом: паттерн `EditInlineNew` и предикаты-условия впитаны в предложение |

---

## 7. План миграции (фазы) — статус

**Статус (2026-08-10):** фазы 1–4 **выполнены одним чистым срезом** (без legacy-моста) на UE 5.8.1. См. реализацию в `DIVECore` (`UDIVEDeviceAction`, `UDIVEActionCatalogAsset`), `DIVERuntime` (built-ins, меню, active interaction), sibling `DeviceActionsLibrary`.

Исторический план фаз сохранён ниже для контекста.

Проект в pre-release, breaking changes допустимы (прецедент: миграция map → ActionSets была
breaking). Тем не менее фазить стоит — рефактор затрагивает подсистему, меню, валидацию, дампы и
smoke-тесты.

**Фаза 1 — ядро.**
`UDIVEDeviceAction` + `FDIVEActionContext` + `FDIVEActionBinding`/`FDIVETargetQuery` +
**`UDIVEActionCatalogAsset` как primary-дом авторинга (§5.8)** + сборка
меню по биндингам + accessor API для яруса B (§5.5). Совместимость: `UDIVELegacyHandlerAction` —
действие с полями `ActionId`/`CatalogKey`-override, чей `Execute` зовёт старый
`IDIVEDeviceActionHandler` на владельце. Автоконверсия: каждый (Role × строка ActionSet) →
биндинг с legacy-действиями. `PickActionSets`/`PickContextMenuRoles` помечаются deprecated.

**Фаза 2 — встроенные действия и секции.**
Focus/Isolate/Admin как классы действий в `BuiltInBindings`; `FDIVEMenuSection`;
удаление хардкод-веток из `ExecuteContextMenuAction` и `AppendStandardMeshEntries`.
Обновить smoke-тест `DIVE.ContextMenu.BuiltInEntries`, Scan, Dump, QUICKSTART.

**Фаза 3 — континуальные действия.**
`UDIVEContinuousDeviceAction`; обобщение слота proxy drive в подсистеме до «active interaction»;
`UDIVEProxyDriveForwardAction`; простенький value-HUD в session chrome по `OnValueChanged`.
**Доменная библиотека (`UnscrewAction`, `CircularValueAction`) — НЕ в DIVE** (ревизия v2):
по `DeviceInteractionModel.md` §1/§3 плагин остаётся тонким и не знает про болты, резьбу и
отвёртки. Дом библиотеки — **sibling-плагин `DeviceActionsLibrary`** (решение №6 в §10):
Runtime-модуль, зависит от DIVECore, но не наоборот. В самом DIVE — только фреймворк +
сессионные built-ins (Focus/Isolate/Admin) + generic forward-действия.

**Фаза 4 — зачистка.**
Удалить `Is_*`-рефлексию (`FindActorBoolProperty` и весь блок), `MakeQualifiedPickContextMenuActionId`,
старые структуры, `IDIVEDeviceActionHandler` **полностью, вместе с `UDIVELegacyHandlerAction`**
(решение №3 в §10 — без escape-hatch), обновить все доки, прогнать `DIVE Scan` по устройствам
проекта.

---

## 8. Риски и trade-offs

| Риск | Оценка / смягчение |
|------|--------------------|
| UX inline-редактирования Instanced-массивов средненький (свёрнутые элементы, нет drag-n-drop между массивами) | Так живёт Enhanced Input; при боли — details customization (фаза 3+) |
| Баги UE с Instanced-субобъектами **на компонентах в BP**: непропагация дефолтов на level-инстансы (**UE-282655**, 5.4–5.7; целевой фикс 5.8 — проект уже на 5.8.1, смежные CL подтверждены, прямое подтверждение фикса ждёт репро), тихие сбросы при рекомпиляции (UE-219732/UE-222390, починены в 5.5), синхронизация после переоткрытия уровня (UE-164500) | **Смягчение — архитектурное (§5.8): primary-дом авторинга — DataAsset-каталог**, где пары «CDO → level-инстанс» нет — устойчиво к текущим и будущим багам этой серии независимо от версии движка. Inline на компоненте — после положительного репро UE-282655. Библиотечные действия — C++, BP-подклассы — листья без глубоких иерархий |
| Состояние на инстансе действия при 100 целях | Правило §5.6: инстанс = параметры; per-target состояние — map или устройство. Зафиксировать в доке и code review |
| Репликация инстансированных UObject | Не актуально: DIVE-сессия локальная, single-player policy уже зафиксирована в ARCHITECTURE |
| Порог входа: «вписать FName» проще, чем «создать класс» | Для типовых случаев классы уже есть (библиотека + встроенные) — автор только инстансирует и заполняет параметры; классы пишутся один раз на паттерн |
| Объём рефактора: подсистема, меню, валидация, дампы, тесты, доки | Фазирование §7; legacy-мост держит устройства рабочими между фазами |
| Больше сущностей в плагине (Action, Condition, Binding, Section) | Взамен умирают: Set, Role, Qualified-ID, `Is_*`-конвенция, hardcode-ветки. Чистый баланс сущностей ≈ нулевой, но все оставшиеся — типизированные |

---

## 9. Сводная таблица: боль → решение

| Боль (из §3) | Решение |
|--------------|---------|
| P1 строки везде | Ссылки объектами (`PrimaryAction` — указатель; действие — инстанс); строки остаются только на границе с level-данными (теги/имена) |
| P2 группировка-фикция | Действие — рантайм-объект; биндинг — явная сущность «группа целей × набор действий» |
| P3 параметры негде хранить | `UPROPERTY` на классе действия; инстанс на группу целей |
| P4 `Is_*` рефлексия | `GetDisplayState` + делегаты/интерфейс к устройству |
| P5 хардкод админки | Встроенные действия = классы в `BuiltInBindings` (данные); admin visibility = `bVisible` |
| P6 God-switch, CollapsedGraph | Граф живёт в классе действия (ярус C) или в C++ (ярус A); устройство — только подписки (ярус B) |
| P7 instant vs continuous | `UDIVEContinuousDeviceAction` + общий слот взаимодействия; proxy drive — частный случай |
| P8 секции не авторятся | `FDIVEMenuSection` + `SectionId` на биндинге |

---

## 10. Открытые вопросы → рекомендованные решения (v3)

Критерий выбора во всех пунктах: долгосрочная чистота/гибкость, без промежуточных решений,
которые станут рудиментами. Breaking changes допустимы.

1. **Модуль для базовых классов → DIVECore.** В Core: `UDIVEDeviceAction`,
   `UDIVEContinuousDeviceAction`, `UDIVEActionCondition`, `FDIVEActionContext`,
   `FDIVEActionBinding`/`FDIVETargetQuery`/`FDIVEMenuSection`, `UDIVEActionCatalogAsset`.
   В Runtime: сборка меню, слот взаимодействия, built-ins (им нужен `UDIVESessionSubsystem` —
   берут из World, Core его не линкует). Жёсткое условие: контекст без типов Runtime —
   `AActor* DeviceHost` + `UActorComponent* SourceComponent` вместо
   `UDIVEInspectableComponent*`. Отдельный плагин фреймворка **сейчас не создавать**: при такой
   дисциплине Core выделение позже — механический перенос модуля (не переписывание, т.е. не
   рудимент), а триггер выделения объективный — второй потребитель (ACTS-меню / VR).
2. **Union, без флагов-исключений.** Winner-take-all выражается поверх union (условия на
   действиях, приоритеты), обратное — нет: union строго более выразителен, второй механизм не
   нужен. Детерминизм: сортировка строк (в цели — `Section.SortOrder` → Priority; **в v0.8 поля SortOrder нет**, порядок = индекс массивов `Sections` / Bindings); одинаковый resolved-DisplayName в одной секции — **warning валидации**, не тихий
   дедуп (тихая магия хуже дубликата в меню). Никаких `bExclusive` — это возвращение
   winner-take-all через чёрный ход.
3. **`IDIVEDeviceActionHandler` — удалить полностью в фазе 4.** Escape-hatch не нужен: ярус C
   (BP-подкласс действия: `Ctx.DeviceHost` → Cast → вызов) покрывает «просто дёрнуть устройство»
   одной нодой без глобального интерфейса. Оставить интерфейс = навсегда второй путь диспатча —
   ровно тот рудимент, которого избегаем. `UDIVELegacyHandlerAction` живёт только фазы 1–3 и
   удаляется вместе с интерфейсом.
4. **Value-HUD — дефолтный виджет в плагине + переопределяемый класс + всегда-делегаты.**
   Зеркалит уже принятый в DIVE паттерн (`UDIVEContextMenuWidget` + UIComponent +
   `*Style`-ассет): плагин поставляет минимальный readout-виджет, хост может подменить класс
   виджета или игнорировать его и строить свой UI по `OnValueChanged`/`OnInteraction*` —
   делегаты вещают всегда, независимо от того, чей виджет активен. Это консистентность с
   существующей UI-моделью плагина, а не промежуточный вариант.
5. ~~`UDIVEActionCatalogAsset` — в фазу 1 или отложить?~~ **Решено: фаза 1.** В ревизии v2 —
   как обход UE-282655; в ревизии v3 (проект на 5.8.1) мотивация смещается к архитектурной
   (§5.8, слой 1), решение не меняется.
6. **Дом доменной библиотеки → sibling-плагин `DeviceActionsLibrary` сразу.** Вариант «модуль в
   хост-проекте» отклонён: game module KDP — демо-исключение принципов (§11
   `Plugin_Architecture_Principles.md`), наращивать его доменным C++ — значит осознанно создать
   рудимент с последующим переездом. Прецедент sibling-плагинов в репо устоявшийся
   (SharedPluginUtils, DIVEGRIPBridge). Состав: Runtime-модуль, зависимости — `DIVECore` +
   Engine, без DIVERuntime; DIVE о библиотеке не знает.

---

## 11. Ревизия v2 — итоги перепроверки

### 11.1. Что подтвердилось

- Диагноз P1–P8 и вывод «группировка ActionSet — фикция рантайма» — подтверждены кодом
  (`MakeQualifiedPickContextMenuActionId`, `FindActorBoolProperty`, hardcode-ветки
  `ExecuteContextMenuAction`).
- Механизм `EditInlineNew`/`DefaultToInstanced` как таковой — рабочий и штатный; прецедент
  Enhanced Input точен вплоть до деталей: `FEnhancedActionKeyMapping` держит
  `UPROPERTY(Instanced) TArray<TObjectPtr<UInputTrigger/UInputModifier>>` **внутри USTRUCT внутри
  DataAsset** — ровно предлагаемая структура `FDIVEActionBinding` в `UDIVEActionCatalogAsset`.
- Объяснение провала DataAsset-попытки (наследование класса ассета vs инстансирование объектов) —
  верно.

### 11.2. Что исправлено по итогам перепроверки

| Было (v1) | Стало (v2) | Причина |
|-----------|-----------|---------|
| `PrimaryAction` — `TObjectPtr` на соседний инстанс | `PrimaryActionIndex` | В details-панели UE нет пикера «соседний субобъект»; объект-ссылка была нереализуема |
| Каталог-ассет — «опционально, отложить» | **Primary-дом авторинга, фаза 1** | **UE-282655**: на 5.4–5.7 (наша 5.7) правки Instanced-дефолтов в BP не доходят до level-инстансов; DataAsset свободен от проблемы по построению |
| Библиотека Unscrew/CircularValue в DIVE (фаза 3) | В хост-проекте / sibling `DeviceActionsLibrary` | Противоречило собственному принципу плагина (`DeviceInteractionModel.md` §1: DIVE тонкий, доменной семантики не знает) |
| Умолчание о латентности BP-`Execute` | Явная оговорка §5.5 | `Execute` с return — function graph, без Delay/Timeline; выигрыш — декомпозиция, не латентность |
| Не было accessor API для яруса B | `GetActionInstances` / `FindActionInstance` | Без него биндинг делегатов устройством — ручной обход массивов |
| Риск instanced-багов «в основном закрыт» | Риск актуален на 5.7, смягчение архитектурное | Проверено по трекеру Epic |

### 11.3. Рассмотренные и отвергнутые альтернативы

| Альтернатива | Почему нет |
|--------------|-----------|
| **GAS (Gameplay Ability System)** | «Действия как объекты» из коробки, но: тяжёлая зависимость, заточен под репликацию/атрибуты/эффекты, конфликтует с принципом self-contained плагинов проекта. Заимствуем идею instancing policy, не систему |
| **GameplayTags вместо FName** | Лечит только опечатки в идентификаторах, не даёт ни параметров, ни lifecycle, ни объектов. `ComponentTags` всё равно `FName` — таргетинг не улучшится. Можно добавить поверх позже, отдельным решением |
| **Ассет на каждое действие (буквальная калька EI `UInputAction`)** | Возвращает проблему прошлой попытки: N ассетов на N групп параметров + строковые membership-ссылки. Инстансы в биндинге дешевле |
| **Действие = UActorComponent** | Компонент на каждую пару «группа × действие» на каждом акторе: тяжело, шумно в иерархии, та же проблема шаринга параметров |
| **StateTree для действий** | StateTree — про состояния/задачи во времени, не про каталог команд контекстного меню; но он — правильный кандидат для FSM устройств, если та понадобится (§6) |

### 11.4. Судьба смежных документов

- `Problem_DuplicatedCatalogActions.md` — **historical** (помечен superseded): описывает v0.7 ActionSets + Roles;
  нормативная модель — этот аудит + `QUICKSTART.md` §1.
- `QUICKSTART.md` §1, `ARCHITECTURE.md`, `DeviceInteractionModel.md` — обновлены под объектную модель v0.8.

### 11.5. Ревизия v3 — переход проекта на UE 5.8.1 (2026-08-10)

Факты перехода 5.7 → 5.8.1, релевантные этому документу:

- **Код не менялся**: весь C++ (проект + DIVE, DIVEGRIPBridge, GRIP, ACTS, MESS,
  SharedPluginUtils) собрался под 5.8 без правок, уже с V7-настройками
  (`BuildSettingsVersion.V7`, `IncludeOrderVersion.Unreal5_8`, warning-as-error для
  unreachable/return-type/dangling). Единственная правка — два `Target.cs`.
- **Смоук-тесты зелёные** на 5.8.1: `DIVE.ContextMenu.DefaultBindings` (ex-BuiltInEntries),
  `DIVE.PawnPhysicalDrive.Resolve`, `ACTS.DiveLegacyDevQuery.NullOwner` — Success (headless
  `UnrealEditor-Cmd`, NullRHI).
- **UE-282655**: смежные фиксы серии instanced-пропагации подтверждены Epic как вошедшие в 5.8
  (CL 47597728, CL 46175874); статус самого тикета в публичном трекере не обновлён. До
  положительного 2-минутного репро §5.8 считать inline-авторинг на компонентах «условно
  разрешённым». Приоритет каталога-ассета это не меняет (аргумент №1 в §5.8 — архитектурный).
- 5.8 — последний мажорный релиз UE5 (дальше UE6): версия долгоживущая, целевая для багфиксов.
  Реализацию фаз §7 можно вести на ней без ожидания следующего мажора.
- Изменений API, затрагивающих предложение (Instanced-свойства, `EditInlineNew`,
  `IsDataValid(FDataValidationContext&)`, details-панель), в 5.8 не обнаружено — сигнатуры и
  паттерны раздела §5 остаются валидными.

---

## 12. Closure checklist (post-implementation review)

Статус после cut-over на объектную модель и повторной проверки кода (2026-08-10).

### 12.1. Проблемы P1–P8

| # | Статус | Примечание |
|---|--------|------------|
| P1 | **Закрыт** | Транспорт — `UDIVEDeviceAction*`; строки только на границе таргетинга (теги/имена/`SectionId`) |
| P2 | **Закрыт** | Действие и биндинг — рантайм-сущности |
| P3 | **Закрыт** | Параметры на классе/инстансе действия; DAL Unscrew/Circular/… |
| P4 | **Закрыт** | `Is_*` / `FindActorBoolProperty` удалены; `GetDisplayState` |
| P5 | **Закрыт** | Built-ins = те же `BuiltInBindings` / классы действий; выборочное удаление/перенос; admin visibility через `bVisible` + флаг |
| P6 | **Закрыт** | `IDIVEDeviceActionHandler` удалён; логика в классе действия |
| P7 | **Закрыт** | `UDIVEContinuousDeviceAction` + общий слот; `UDIVEProxyDriveForwardAction`; menu/primary begin; self-complete → `EndProxyDrive` |
| P8 | **Закрыт** | `FDIVEMenuSection` + `SectionId` на биндинге; separators между секциями |

### 12.2. Решения §10

| # | Статус |
|---|--------|
| 1 Types in DIVECore | **Закрыт** |
| 2 Union bindings | **Закрыт** (+ warning на дубликат DisplayName) |
| 3 Delete handler | **Закрыт** |
| 4 Value-HUD | **Закрыт** (delegates + default widget path) |
| 5 Catalog asset phase 1 | **Закрыт** |
| 6 DeviceActionsLibrary sibling | **Закрыт** |

### 12.3. Известные остатки (не блокеры аудита)

- `const_cast` в `MakeActionContext` / `BuildEntries` — типичный UE pattern (const API → non-const `SourceComponent`).
- Continuous из меню = modal drag (не hold-LMB) — продуктовое UX, зафиксировано в QUICKSTART.
- `Problem_DuplicatedCatalogActions.md` — historical reference only.

### 12.4. Ревизия после BuiltInBindings + bugfix (2026-08-10)

Повторный аудит нашёл и закрыл в коде:

| Баг | Фикс |
|-----|------|
| Continuous из меню убивался тем же LMB-up | `bIgnoreNextPrimaryActionRelease` + seed cursor pos |
| `AnyPrimitive` + empty MatchValues → IsDataValid/Scan error | валидация пропускает AnyPrimitive |
| Menu context без `PickHit` | `ContextMenuPickHit` → `MakeActionContext` |
| CircularValue копил значение между жестами | reset AccumulatedValue в Begin |
| Condition только `bEnabled` | `bVisible` от Condition |
| Stale editor tooltips ActionSets/Roles/`Is_*` | обновлены |

### 12.6. Re-audit pass (2026-08-10, evening)

| Issue | Fix |
|-------|-----|
| Menu execute lost binding `TargetKey` | `FDIVEContextMenuEntry::TargetKey` → UI → `ExecuteContextMenuAction` |
| Catalog `SectionId` Standard/Admin invalid without local Sections | known Built-In ids allowed in catalog validation |
| Sticky ignore-release edge | clear ignore on menu open + release while menu open |
| Header first-wins blocked Catalog Header on Standard | empty Header may be filled by later merge source |
| `BuildEntries` unused Subsystem/`const_cast` | removed |
| Audit §§1–7 read as “current” | banner: historical diagnosis; §12 = status |

### 12.7. Authoring simplify (2026-08-10)

| Change | Why |
|--------|-----|
| Deleted sibling / module sample libraries (`DeviceActionsLibrary`, `DIVEActionsLibrary`) | Domain examples ≠ DIVE; author creates BP via factory |
| `BuiltIn*` + `Inline*` → single `Bindings` / `Sections` | Same component, same data; Catalog remains the shared Data Asset home |
| Removed `bEnableAdminContextMenuEntries` | Admin = rows in Bindings; Shipping hides via `GetDisplayState` |
| Content Browser **DIVE** factories | Catalog, Device Action, Continuous Device Action, Action Condition |
| Handler path = action BP graph (+ optional device interface) | Avoid BeginPlay subscription dump |
| Docs: Condition = optional visibility predicate | Concrete Evaluate example in QUICKSTART |

### 12.8. Re-audit after factory pass (2026-08-11)

| Check | Result |
|-------|--------|
| Source / `.uplugin` / `.uproject` free of `DIVEActionsLibrary` / `DeviceActionsLibrary` / `UDAL*` | OK (stale Intermediate cleaned) |
| CoreRedirects BuiltIn→Bindings, Inline→DEPRECATED | Present |
| PostLoad does not re-seed empty Bindings | OK (`PostInitProperties` seed only) |
| Content Browser DIVE factories (Catalog / Action / Continuous / Condition) | Present |
| Continuous BP: `bInteractionActive` never set from BP Begin | **Fixed** — session `MarkInteractionActive` after successful Begin; BP self-finish via `NotifyInteractionCompleted` |
| Admin Shipping hide via `GetDisplayState` | OK |
| Catalog Details nested `DIVE\|Action` | Flattened to Sections / Bindings |

### 12.9. Drop Inline* migration (2026-08-11)

| Change | Notes |
|--------|-------|
| Removed `InlineBindings_DEPRECATED` / `InlineSections_DEPRECATED` + `PostLoad` migrate | Content had no remaining refs |
| CoreRedirects: Inline* / Inline*_DEPRECATED → Bindings / Sections | Same pattern as BuiltIn* |
