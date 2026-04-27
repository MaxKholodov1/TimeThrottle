# bridge_time_throttle: руководство для разработчика

## Назначение

`bridge_time_throttle` - мост между браузером (native messaging/stdin) и `TimeThrottle`.
Получает сообщения с доменом и пересылает их в named pipe:
`\\.\pipe\MyMonitorDomainPipe`.

## Поток обработки

1. Инициализация логгера.
2. Перевод `stdin/stdout` в бинарный режим.
3. Чтение сообщения:
   сначала 4 байта длины (`unsigned int`), затем тело JSON.
4. Валидация размера (лимит 1 MB).
5. Попытка открыть pipe и отправить тело сообщения в `TimeThrottle`.

Если pipe недоступен (основное приложение не запущено), мост не падает и пишет trace.

## Ключевые файлы

- `main.cpp` - вся логика моста.
- `CMakeLists.txt` - сборка единичного exe.

## Сборка

```powershell
cd D:\projects\C++\TimeThrottleProject\bridge_time_throttle
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Логи

- `D:/projects/C++/bridge_time_throttle/logs/bridge.log` (rotating).

## Контракт сообщений

- Транспорт: бинарный протокол native messaging.
- Payload: строка JSON (как есть, без дополнительного парсинга в bridge).
- Parse в `TimeThrottle` делается функцией `ParseDomain(...)`.

## Типичные проблемы

- `Incomplete message body received`:
  источник неверно передает длину или обрывает канал.
- `Message too large`:
  payload > 1 MB.
- `Main app Pipe not available`:
  `TimeThrottle` не запущен либо pipe имя не совпадает.
