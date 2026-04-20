# WatchDog: руководство для разработчика

## Назначение

`WatchDog` следит за живостью `TimeThrottle` и перезапускает его при падении/зависании.
Также участвует в двустороннем heartbeat через общую память.

## Ключевые компоненты

- `main.cpp`:
  singleton через mutex `Global\WatchDog_Unique_Mutex`, инициализация логов, запуск защиты.
- `process_guard.*`:
  логика heartbeat + проверка mutex `Global\TimeThrottle_Unique_Mutex` + перезапуск `TimeThrottle.exe`.

## Механика контроля

1. Shared memory: `Local\MySharedMem`.
2. `watchDogTick` обновляется каждые 500 мс.
3. Если `mainAppTick` не обновлялся > 2000 мс:
   `TimeThrottle.exe` считается зависшим и перезапускается.
4. Если mutex `TimeThrottle` отсутствует:
   `TimeThrottle` считается не запущенным и стартует заново.

## Зависимости

- `spdlog` (`lib/spdlog/include`).
- Windows API: `CreateMutexW`, `CreateFileMappingW`, `CreateProcessW`, `Toolhelp32`.

## Сборка

```powershell
cd D:\projects\C++\TimeThrottleProject\WatchDog
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Запуск

Ожидает, что рядом лежит `SecurityHealthService.exe` (маска под основное приложение).
Этот путь собирается относительно директории текущего exe.

## Логи

- `D:/projects/C++/WatchDog/logs/watchDog.log` (rotating).
- Плюс вывод в консоль.

## Типичные проблемы

- `TimeThrottle` не поднимается:
  проверить реальное имя/расположение целевого exe.
- Постоянные ложные рестарты:
  проверить, что обе стороны heartbeat используют одну структуру shared memory.
