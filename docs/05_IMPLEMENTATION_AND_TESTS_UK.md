# Етапи 3–6 — фактичний стан

**Це вихідний прототип фундаменту. Повний кінцевий результат ТЗ ще не досягнуто.** Немає фактичної компіляції або ігрового тесту; не встановлювати на бойовий сервер як перевірений мод.

## Етап 3 — фундамент написано, runtime-перевірка відкрита

1. Реалізовано протокол, controller-bound Server/Owner RPC, авторизацію кожного запиту, replay/rate guard, серверний знімок цілей, UI та повідомлення.
2. Створено сім EGC_Admin*.c у mod/Scripts/Game/EGC_AdminTools та власний Configs/System/chimeraInputCommon.conf.
3. Існуючих файлів EGC не змінено. Після внутрішнього review до нових файлів додано перевірку актуального staff-файла, UID/controller цілі, покоління списку та фокус UI.
4. Використовуються EGC_StaffList/EGC_StaffConfig та GameMode.EGC_IsPlayerVIP; немає окремих ролей або автоматичної видачі admin.
5. Використано PlayerManager, BackendApi identity, RplRpc, System, JsonLoadContext, runtime widgets/modal/input і CharacterController controls. Точні файли й рядки — static_api_evidence.json.
6. Залежності вихідного EGC неповні; власний runtime UI/input потребує перевірки ресурсів/фокуса в Workbench. Усі native admin roles мають однаковий набір дозволів; GAME_MASTER не отримує адміністративний доступ. Різних custom staff-ролей у наданій EGC-моделі немає.
7. Виконано статичні контракти/індексацію API, лексичну перевірку дужок, перевірку нових class names та заборонених залежностей.
8. Компіляція, resource loader/input config, F8, кирилиця, 720p/1080p/1440p/ultrawide, pointer/gamepad, modal cleanup і respawn/reconnect у UI ще не перевірені. UI має закриватися F8/кнопкою; зміна контролера/світу потребує окремого lifecycle-тесту. Управління рухом/зброєю тимчасово блокується, повернення управління треба перевірити з EGC_ZipTie/SafeZone.
9. Наступний крок — Workbench build та багатоклієнтний тест фундаменту, потім наступні інструменти.

## Етап 4 — інтеграція лише доступних операцій читання

1. Реалізовано staff authorization, VIP-read через чинний метод і перегляд чинного EGC_Bans.json; JSON не використовується для enforcement.
2. Створено EGC_AdminAccess.c та EGC_AdminReadModel.c; пропозиції розширення — 03_VIP_CHANGE_PROPOSAL_UK.md.
3. BanSystem/VIPManager/StaffList/інші EGC залишені без змін, включно з виявленими помилками.
4. EGC_BanSystem — єдиний writer його експорту; VIP — єдиний чинний GameMode cache. Нова адмінка тільки читає.
5. JsonLoadContext.LoadFromFile/ReadValue, map.GetKey/GetElement; EGC_IsPlayerVIP.
6. Ban/Unban через сам EGC_BanSystem неможливі: таких methods немає. VIP write потребує погодженого bridge. Часові VIP потребують зміни моделі, заборонено вводити їх приховано.
7. Статично звірено формат ключа activeBans, UID-map і публічний VIP-read. SHA-256 усіх розпакованих оригіналів збігаються з ZIP.
8. Реальний backend, відсутній/зіпсований JSON, reload, >100 банів, кілька одночасних адмінів не тестовано у грі. Зміни VIP на диску відображаються за чинним watchdog, не негайно.
9. Наступний крок — рішення щодо BanService adapter і VIP bridge; вони не застосовані до відповіді користувача.

## Етап 5 — початкові функції, не завершений набір ТЗ

1. Написано online roster (12 на сторінку), досьє UID/стан/VIP, бан-експорт із навігацією, Heal живого персонажа і Kick із причиною/локальним підтвердженням. Запит не може діяти на playerId, якого не було в серверному знімку. Повторний вхід перевіряється посиланням на controller та backend UID.
2. Створено EGC_AdminOperations.c; UI та channel з'єднані з ним.
3. Змінювалися лише власні нові файли прототипу.
4. Дії поважають EGC staff/VIP guard; EGC damage-системи не переписані.
5. SCR_CharacterDamageManagerComponent.FullHeal(), EDamageState.DESTROYED; PlayerManager.KickPlayer(...KICK,0).
6. KICK_REQUESTED означає переданий рушію запит, не підтверджене відключення; reason зберігається в audit, бо KickPlayer не має текстового reason argument. HEAL_APPLIED — виклик методу, не ігровий доказ усунення всіх damage effects. Досьє не є розширеною телеметрією здоров'я. Rate-limit запити тихо відкидає до читання файлів; UI додатково обмежує частоту.
7. Фактично перевірено сигнатури та наявність серверної валідації в source, не виконання функцій.
8. Потрібні окремі Heal/Kick tests, гонки disconnect/reconnect/respawn, відкликання ролі між refresh та click, failed backend identity, EGC damage conflicts.
9. Teleport/bring/goto, God, Revive, Spectator, невидимість, позиція, server/match info та admin announcements **не реалізовані**. Teleport API підтверджено, але vehicle/placement/replication policy та тести не завершені. Наявність EnableDamageHandling/FullHeal не є достатнім доказом повноцінного God/Revive.

## Етап 6 — лише статичні перевірки

1. Виконано audit_archives.py та verify_static.py в цьому середовищі.
2. Створено archive_manifest.json, egc_dependencies.json, EGC_INDEX.md, static_api_evidence.json, static_verification.json.
3. Файли EGC не змінено; новий код уточнено після review.
4. Оригінали порівняно побайтово з архівами; індексовано class names для конфліктів нових класів.
5. Перевірено наявність використовуваних API symbols за конкретними файлами snapshot. Перевірка не виводить типи, не компілює override та не перевіряє native linkage.
6. У середовищі не виявлено ArmaReforgerWorkbench/ArmaReforgerServer у PATH та перевірених /opt,/usr/local/bin,/workspace. Не інсталювалися і не запускалися сторонні бінарники гри. Snapshot не доводить номер встановленої 1.9 збірки. Критичний startup-loop EGC описано окремо в 04_EGC_STARTUP_RISK_UK.md.
7. Результат verify_static.py: **68 статичних перевірок пройдено, 0 провалено**. Це source contracts та symbol checks, не 68 ігрових тестів. Повна незмінність розпакованих матеріалів підтверджена.
8. **Компіляція НЕ виконана; RPC у мережі НЕ перевірені; dedicated/listen НЕ запущено; багатоклієнтні/рольові/ban/VIP/performance сценарії НЕ виконані.**
9. Далі потрібні робочий addon-проєкт і залежності EGC, відповідний Workbench, погоджений спосіб безпечного старту EGC, компіляція, усунення діагностик і тестова матриця нижче.

## Обов'язкова матриця приймання (усі пункти ще не виконані)

| Сценарій | Очікування |
|---|---|
| звичайний гравець викликає кожен verb напряму | відмова без приватних даних і зміни стану |
| staff без admin login / лише GAME_MASTER | відмова |
| admin поза staff / порожній, зіпсований, видалений staff-файл | відмова |
| staff admin із VIP | відмова; оригінальний VIP watchdog зберігає свою поведінку |
| адміністратор і session admin без VIP | дозволений однаковий базовий набір |
| двоє адміністраторів з різними сторінками | незалежні selection snapshots |
| відкликання staff/ролі перед дією | дія відхилена наступною перевіркою |
| підміна generation/row/sequence/verb/page, довга причина | відмова без виконання |
| replay тієї самої sequence | не більше одного виконання |
| відключення/повторний вхід із тим самим ID | старий snapshot не діє на нового controller |
| спроба Server RPC на чужому controller | transport drop, відсутність ефекту |
| rapid requests / wrap tick clock | bounded replies/робота rate guard без зависання |
| Heal живого/мертвого/без персонажа | тільки припустимий живий персонаж, без воскресіння |
| Kick себе/без причини/ціль зникла | відхилення; для чинної цілі audit + запит рушію |
| бан-експорт відсутній/порожній/>100 | чесне повідомлення про snapshot, не «немає банів» |
| UI F8/закриття/respawn/зміна світу/інші меню/ZipTie | без завислого modal і втрати керування |
| 128 гравців, кілька адмінів, тривала сесія | виміряти frame time, file IO, RPC/log обсяг; норматив узгодити |

Для кожного runtime-тесту зберігати game build, addon GUID/versions, server/client logs, кроки, очікуваний/фактичний результат. Після появи Ban/VIP-write додати success/error/timeout/одночасний запит/невдале збереження та read-back тести. Без цих доказів статус production-ready не присвоюється.
