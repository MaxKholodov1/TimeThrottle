# TimeThrottle: руководство для разработчика

## Назначение

`TimeThrottle` - центральное приложение системы:
- захватывает сетевой трафик через WinDivert;
- применяет режимы блокировок по доменам;
- ведет мониторинг активного окна/сайтов;
- поднимает локальный HTTPS CONNECT-прокси (`8899`);
- поддерживает watchdog через shared memory heartbeat.

## Ключевые компоненты

- `main.cpp`:
  только orchestration жизненного цикла (`Monitoring`, `NetDelay`, `ProcessGuard`, `ProxyServer`).
- `scr/core/console_commands.*`:
  обработка CLI-команд, включая `kill` (safe shutdown + stop watchdog + delete scheduled task).
- `scr/network/net_delay.*`:
  WinDivert loop, решение блокировать/пропустить пакет.
- `scr/network/proxy_server.*`:
  CONNECT-прокси, map `IP -> domain` для `NetDelay`.
- `scr/monitor/monitor.*`:
  сбор статистики приложений и доменов, сервер named pipe `\\.\pipe\MyMonitorDomainPipe`.
- `scr/core/app_config.*`:
  чтение/запись `settings.json`, режимы, таймер, custom-домены.
- `scr/security/process_guard.*`:
  heartbeat и перезапуск `WinSvcMonitor.exe` при падении.
- `scr/core/logger.*`:
  логирование в `D:/projects/C++/TimeThrottle/logs/debug_history.log`.

## Зависимости

- `lib/WinDivert.lib`, `WinDivert.dll`, `WinDivert64.sys`.
- `nlohmann_json` подтягивается через `FetchContent`.
- `spdlog` из `lib/spdlog/include`.
- Windows libs: `psapi`, `kernel32`, `user32`, `advapi32`, `ws2_32`, `iphlpapi`.

## Сборка

```powershell
cd D:\projects\C++\TimeThrottleProject\TimeThrottle
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Запуск

Запускать `TimeThrottle.exe` лучше из папки, где лежат WinDivert-бинарники и `WinSvcMonitor.exe`.

При старте:
1. Создается mutex `Global\TimeThrottle_Unique_Mutex` (защита от второго экземпляра).
2. Загружается `settings.json`.
3. Стартуют мониторинг, WinDivert loop, proxy.
4. Включается защита и автоподъем watchdog.

## CLI-команды в консоли

- `help` или `?` - список команд.
- `mode <type> [minutes]`:
  типы: `off`, `no_internet`, `custom`, `no_messenger_only`, `no_video_and_streaming`, `no_social_media_and_no_video`.
- `mode_status` - текущий режим.
- `add <domain>` - добавить custom-домен в блок-лист.
- `stats` - вывести статистику приложений/сайтов.
- `kill [task_name]` - безопасная остановка, убийство watchdog, удаление scheduled task, self-exit.

## Конфигурация

Файл: `settings.json` (в рабочей директории процесса).

Поля:
- `network_delay_ms` (сейчас хранится, но задержка фактически не применяется в текущем loop).
- `blocked_domains`.
- `current_mode`.
- `timer_end`.

Важно: большинство режимов блокирует трафик только пока активен таймер (`SetTimer > 0`), кроме `NO_INTERNET` (блок всегда).

## Интеграции

- `WatchDog`:
  через shared memory `Local\MySharedMem` и heartbeat.
- Browser bridge:
  через named pipe `\\.\pipe\MyMonitorDomainPipe`.
- Scheduled task:
  `Microsoft\NetFramework\v4.8.9032\NetFxHost` (удаляется командой `kill`).

## Диагностика

- Логи приложения:
  `D:/projects/C++/TimeThrottle/logs/debug_history.log`.
- Если `WinDivertOpen("tcp", ...)` падает:
  проверить запуск с правами администратора и наличие драйвера.
- Если сайты не фильтруются:
  проверить, что браузер ходит через локальный прокси на порт `8899`.
