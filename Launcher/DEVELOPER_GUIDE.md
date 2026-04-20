# Launcher: руководство для разработчика

## Назначение

`Launcher` подготавливает файловое окружение в `ProgramData`, разворачивает бинарники под маскированными именами, регистрирует задачу в Планировщике и запускает основное приложение.

## Что делает при запуске

1. Получает путь `%ProgramData%`.
2. Создает каталоги:
   - `%ProgramData%\WinNetDiagnostic` (основной набор),
   - `%ProgramData%\Microsoft\NetFramework` (набор для task/launcher).
3. Проверяет наличие критичных файлов в обоих каталогах.
4. Если файлов нет, копирует из текущей рабочей директории.
5. Регистрирует scheduled task через XML:
   `Microsoft\NetFramework\v4.8.9032\NetFxHost`.
6. Запускает `%ProgramData%\WinNetDiagnostic\SecurityHealthService.exe`.

## Имена и маппинг файлов

Исходники (в текущей директории запуска):
- `tt_core.dat` (бинарь main),
- `service_fixer.res` (бинарь watchdog),
- `tt_config.res` (бинарь launcher),
- `WinDivert.dll`,
- `WinDivert64.sys`.

Целевые имена задаются в `functions.h`:
- для app-dir: `SecurityHealthService.exe`, `WinSvcMonitor.exe`, `WinDivert.dll`, `WinDivert64.sys`;
- для launcher-dir: `mscorsvw_x64.exe`, `NetFxPerfAsset.exe`, `ClrShimHost.exe`, `SystemRuntimeSerialization.dll`, `MicrosoftFrameworkNative.sys`.

## Ключевые файлы

- `main.cpp` - сценарий развертывания.
- `functions.cpp` - файловые операции, запуск процессов, регистрация задачи.
- `functions.h` - константы путей/имен.

Примечание: `main.cpp` и `functions.cpp` в репозитории могут быть помечены hidden-атрибутом на Windows, но это обычные исходники.

## Сборка

```powershell
cd D:\projects\C++\TimeThrottleProject\Launcher
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Права и ограничения

- Для `schtasks /Create` обычно нужны повышенные права.
- Для записи в `%ProgramData%` также может потребоваться запуск от администратора.

## Типичные проблемы

- `not deployed`:
  в текущей директории нет одного из source-файлов (`tt_core.dat` и т.д.).
- Задача не регистрируется:
  проверить права и корректность пути к exe в XML.
- Приложение после deploy не стартует:
  проверить, что файл по ожидаемому имени реально существует и не заблокирован защитником.
