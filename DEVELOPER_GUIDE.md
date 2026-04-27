# TimeThrottleProject: руководства для разработчика

В проекте 4 отдельных приложения. Для каждого есть отдельный гайд:

1. `TimeThrottle`:
   `D:\projects\C++\TimeThrottleProject\TimeThrottle\DEVELOPER_GUIDE.md`
2. `WatchDog`:
   `D:\projects\C++\TimeThrottleProject\WatchDog\DEVELOPER_GUIDE.md`
3. `bridge_time_throttle`:
   `D:\projects\C++\TimeThrottleProject\bridge_time_throttle\DEVELOPER_GUIDE.md`
4. `Launcher`:
   `D:\projects\C++\TimeThrottleProject\Launcher\DEVELOPER_GUIDE.md`

## Как читать

1. Начать с `Launcher` (как разворачивается и стартует система).
2. Затем `TimeThrottle` (основная логика блокировок/мониторинга).
3. Затем `WatchDog` (взаимный контроль процессов).
4. Затем `bridge_time_throttle` (мост от браузера к named pipe).

## Общая среда разработки

- ОС: Windows.
- Компилятор: MSVC (Visual Studio 2022).
- CMake: 3.30+ (для `TimeThrottle`, `WatchDog`, `bridge_time_throttle`), 3.30 указан и для `Launcher`.
- Стандарт C++: C++20.
- Для функций с WinDivert и задачами Планировщика обычно нужны повышенные права.
