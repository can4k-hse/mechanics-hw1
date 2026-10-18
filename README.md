# Домашнее задание №1 — основы механики

Яковлев Александр Александрович, БПИ244.

Проект на основе **Third Person Template**, Unreal Engine **5.7.4**. Реализованы
спринт, взаимодействие через делегат, состояние анимации Sprint и выносливость.

## Управление

| Клавиша | Действие |
| --- | --- |
| W, A, S, D | Перемещение |
| Мышь | Поворот камеры |
| Space | Прыжок |
| Left Shift, удерживать | Спринт |
| E | Взаимодействие: сообщение `Interaction triggered!` |

## Соответствие заданию

1. **Наследование.** `AHWCharacter` наследуется от `ACharacter`.
   Родитель стандартного `BP_ThirdPersonCharacter` изменён на `HWCharacter`.
   Стандартные перемещение, камера и прыжок сохранены.
2. **Enhanced Input.** Ресурсы `IA_Sprint` и `IA_Interact` добавлены в
   `Content/Input/Actions`; Left Shift и E назначены в существующем `IMC_Default`.
   Новые действия привязаны в C++ через `BindAction` в `SetupPlayerInputComponent`.
3. **Делегат.** Обработчик E вызывает `OnInteraction.Broadcast()`.
   В Event Graph персонажа `BeginPlay` подписывает событие `ShowInteraction`
   на Dynamic Multicast Delegate `OnInteraction`. Обработчик Blueprint вызывает
   `Print String` с точным текстом `Interaction triggered!`.
4. **Спринт и анимация.** При удержании Shift `MaxWalkSpeed` увеличивается до 900;
   при отпускании восстанавливается исходная скорость из шаблона.
   `bIsSprinting` объявлена с `UPROPERTY(BlueprintReadOnly)`.
   Стандартный `ABP_Unarmed` читает её в Event Graph в переменную `IsSprinting`.
   В `AnimGraph → Locomotion` добавлено состояние `Sprint`, переход
   `Walk / Run → Sprint` при `IsSprinting` и обратный при `NOT IsSprinting`.
   Используется стандартная анимация `MF_Unarmed_Jog_Fwd` с коэффициентом скорости 1.5.
   Дополнительный переход в Idle при остановке сохраняет поведение шаблона.
   Машина `Main States`, отвечающая за прыжки и падение, сохранена.
5. **Выносливость, дополнительный пункт на 10 баллов.** Начальное и максимальное
   значение — 100. Во время спринта в C++ `Tick` расходуется 20 единиц в секунду
   с учётом `DeltaSeconds`. При нуле спринт прекращается, даже если Shift зажат.
   После отпускания Shift выносливость восстанавливается по 25 единиц в секунду.
   Для нового спринта нужно снова нажать Shift, имея ненулевой запас.
   Значение и шкала выносливости отображаются внизу слева через `AHWStaminaHUD`.

Выносливость расходуется, пока активен режим спринта, в том числе при удержании
Shift на месте. В покое персонаж при этом сохраняет анимацию Idle.

## Открытие и сборка

1. Установить Unreal Engine 5.7 и Visual Studio с нагрузкой **Game development
   with C++**, MSVC и Windows SDK. Проект проверяется на UE 5.7.4.
2. Склонировать репозиторий в короткий путь на диске Windows.
3. При необходимости выбрать для `MechanicsHW1.uproject` через контекстное меню
   **Switch Unreal Engine version** установленную UE 5.7. Ассоциация установленного
   движка может отличаться на другом компьютере.
4. Выполнить **Generate Visual Studio project files**, открыть решение и собрать
   проект `MechanicsHW1` в конфигурации **Development Editor / Win64** при закрытом
   Unreal Editor. Файлы решения и результаты сборки генерируются локально.
5. Открыть `MechanicsHW1.uproject`. Карта `ThirdPerson/Lvl_ThirdPerson` назначена
   стартовой. Нажать **Play**, затем щёлкнуть в игровом окне.

В Visual Studio при ошибке `SetEnv: Environment variable name or value is too long`
следует открыть **Project → Configure Tools for Unreal Engine**, установить
**Max Include Paths Length (chars) = 30000** и пересоздать решение.
UE 5.7 рекомендует MSVC 14.44.35207; сборка также проверялась с установленным
MSVC 14.51.36256, который выдаёт предупреждение о нерекомендуемой версии.

## Основные файлы

- `Source/MechanicsHW1/Public/HWCharacter.h` — свойства, действия ввода и делегат.
- `Source/MechanicsHW1/Private/HWCharacter.cpp` — обработчики, скорость и выносливость.
- `Source/MechanicsHW1/Private/HWStaminaHUD.cpp` — отображение выносливости.
- `Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset` — персонаж
  и подписка Blueprint на делегат.
- `Content/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.uasset` — анимации.
- `Content/Input/IMC_Default.uasset` — привязки клавиш.

В `Source/HW1Tools` находится вспомогательный модуль **Editor** для воспроизводимой
настройки и проверки ресурсов. В игровой сборке он не используется. Все настроенные
ресурсы уже сохранены в `Content`; запускать настройку перед игрой не требуется.

## Автоматическая проверка

После сборки `MechanicsHW1Editor` выполнить из PowerShell в папке проекта,
подставив свой путь к движку:

```powershell
& 'D:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' "$PWD\MechanicsHW1.uproject" -run=HW1Setup validate -unattended -nop4 -NullRHI -nosound -stdout
```

Проверка загружает сохранённые ресурсы, компилирует Blueprint и создаёт тестовый
игровой мир. Она проверяет привязки ввода, изменение скорости, расход и восстановление
выносливости, истощение при удерживаемом Shift, вывод сообщения через делегат,
передачу состояния в Animation Blueprint и переходы состояний анимации.
Флаг `validate` проверяет проект без сохранения изменений ресурсов.

Результат выполненной проверки: **37 проверок, 0 ошибок**. C++ сборка
`MechanicsHW1Editor` завершилась с `Result: Succeeded`; персонаж и Animation
Blueprint скомпилированы без ошибок и предупреждений. Проверка выполнялась без
отрисовки (`NullRHI`); внешний вид HUD и плавность движения следует оценить в Play.

## Ручная проверка

1. Проверить перемещение, мышь и прыжок.
2. Во время движения зажать Shift: скорость и анимация должны измениться,
   выносливость — уменьшаться.
3. Отпустить Shift: скорость и анимация возвращаются к обычным,
   выносливость восстанавливается.
4. Удерживать Shift до нуля: персонаж прекращает спринт, пока клавиша ещё зажата.
5. Нажать E: появляется `Interaction triggered!`.
6. Проверить прыжок во время спринта и отображение шкалы выносливости.

`.gitignore` исключает `Binaries`, `DerivedDataCache`, `Intermediate`, `Saved`,
локальные настройки IDE и резервные копии. Для воспроизведения сохранены исходники,
настройки и ресурсы проекта.
