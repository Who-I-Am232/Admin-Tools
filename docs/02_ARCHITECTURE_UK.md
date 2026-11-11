# Етап 2 — архітектура EGC Admin Tools

Статус: незалежний проєкт; реалізація фундаменту обмежена можливостями наявних контрактів EGC. Розширення Ban/VIP не вважати погодженим лише через наявність цього документа.

## Межі та дерево

```
EGC_AdminTools/
  research/                    незмінні розпаковані архіви, не встановлювати
  docs/                        аудит, архітектура, встановлення, перевірки
  tools/                       відтворювані статичні перевірки
  mod/
    Configs/System/chimeraInputCommon.conf
    Scripts/Game/EGC_AdminTools/
      EGC_AdminProtocol.c      команди та серверний запис вибору цілі
      EGC_AdminAccess.c        наявні staff + game roles + VIP guard
      EGC_AdminAudit.c         окремий структурований audit у console.log
      EGC_AdminReadModel.c     списки онлайн, VIP, експорт банів
      EGC_AdminOperations.c    вузькі серверні команди
      EGC_AdminChannel.c       owner-controller gateway, replay/rate guard
      EGC_AdminConsole.c       власний runtime UI без ресурсів референсів
```

Модуль не містить копій оригінальних EGC класів. `modded SCR_PlayerController` — стандартна точка розширення рушія; всі нові члени мають EGC_AT-префікс. Немає власної таблиці staff/roles, VIP-файла або ban enforcement.

## Класи та контракти

- EGC_AdminVerb: закритий перелік команд; невідоме значення завжди відхиляється.
- EGC_AdminSelection: серверні playerId та UID, прив'язані до номера рядка поточного знімка. UID не приймається від клієнта як доказ ідентичності.
- EGC_AdminAccess: CanUse отримує actor тільки з контролера. Кожна команда перевіряється заново; клієнтський staff flag не використовується. EGC_StaffList є джерелом членства; ролі рушія підтверджують активний admin login; VIP не має доступу.
- EGC_AdminAudit: час сервера, UID/ID actor та target, verb, sequence, причина, результат. Рядки очищаються від переносів, обмежуються за довжиною. Окремий префікс, без перезапису журналів EGC. Архівація/ротація console.log — серверна інфраструктура, не нескінченний масив у пам'яті.
- EGC_AdminReadModel: серверні online page та деталі, EGC_IsPlayerVIP, read-only EGC_Bans.json. Експорт банів явно позначається неповним і потенційно застарілим.
- EGC_AdminOperations: диспетчер тільки відомих команд, валідація цілі та аргументів. Перша версія: roster/details/VIP-read/ban-export-read/kick/heal. Teleport та інші інструменти розширюють окремі модулі після runtime-перевірки.
- EGC_AdminConsole: лише UI та надсилання запитів. Жодних backend-запитів/файлових записів/рішень про доступ на клієнті.
- SCR_PlayerController.EGC_AT_*: транспортний шлюз і стан сесії. Reliable Server RPC для запитів, Owner RPC для обмежених результатів. Глобального broadcast персональних UID немає.

## Потік

```mermaid
sequenceDiagram
  participant U as EGC_AdminConsole (клієнт)
  participant C as власний PlayerController (сервер)
  participant A as EGC_AdminAccess
  participant E as наявні EGC / PlayerManager
  U->>C: sequence, verb, row, reason (без actor UID)
  C->>C: rate limit, replay check, bounds
  C->>A: GetPlayerId()
  A->>E: backend UID, staff, ролі, VIP
  A-->>C: дозвіл або відмова
  C->>C: row -> збережені playerId/UID; повторна перевірка UID
  C->>E: виконати відому операцію
  C->>C: audit спроби та результату
  C-->>U: Owner RPC, результат і повідомлення
```

Списки обмежені сторінкою 12 рядків. Номери рядків чинні лише до refresh; зміна UID/відключення роблять ціль недійсною. Запити послідовні на одному контролері; monotonically increasing sequence захищає від повторного виконання надісланого номера. Rate limit передує файловим перевіркам та розсилкам. Це не захист від усіх мережевих DoS і не заміна BattlEye.

## Ban/VIP інтеграція

EGC_BanSystem -> BanService -> EGC_Bans.json -> read model -> Owner UI. У першому фундаменті жодного запису. Якщо погоджено адаптер BanService, він потребує власної callback-життєздатності, pending/result, timeout, online identity revalidation та координації із спільною сторінкою EGC exporter. Текст успішного запиту не означає успішний бан. Видавець покарання зберігається в audit, не в неіснуючому полі оригінального бан-запису.

VIP-read -> чинний GameMode.EGC_IsPlayerVIP(playerId). Grant/Revoke/опис вимагають розширення доступу до живого EGC_VIPConfig. Причина: окремий config object не оновлює watchdog cache, а наступний reload/Save може дати розбіжність. Точна ділянка: EGC_VIPManager.c:40 (protected config), :54 (read method), :64–75 (watchdog/reload), :19–27 (Save). Ризики: UID typo, відмова Save, одночасне ручне редагування, VIP-адмін буде покараний чинним watchdog. Альтернатива без розширення класу — read-only панель і наявне ручне керування EGC_Vips.json з reload до 300 секунд. Жодних змін оригіналу чи нового mutator до окремого погодження.

## UI концепція

Власна «оперативна консоль»: графітове тло, приглушений оливковий акцент, зверху EGC/статус; зліва сторінка онлайн-складу; справа досьє та дії; знизу причина, окреме підтвердження kick і результат. Розкладка через відносні anchors; власна input action EGC_AT_ConsoleToggle (F8), жодного CGG_AdminToggle. Інтерфейс побудований стандартними runtime widgets, без залежності від чужих layouts або GUID. Це початкова реалізація, не затверджений у грі дизайн.

## Залежності та незмінні системи

Arma Reforger target 1.9, повний чинний EGC addon із EGC_StaffList/EGC_VIPManager/EGC_BanSystem та його власними зовнішніми залежностями. Новий мод не залежить від Casual/WIU. Весь EGC, зокрема StaffList, VIPManager, BanSystem, PlayerRecord, PlayerOnline, AdminSpy, SaveSystem, залишається побайтово без змін. Якщо повний EGC уже вимагає ServerAdminTools/ACE, новий addon не усуває цих вихідних залежностей.

## Звіт етапу

1. Спроєктовано потоки, модулі, UI, політику прав та безпечний вибір цілі.
2. Створено цей документ.
3. EGC не змінено.
4. Використовуються StaffList/IsPlayerVIP/Ban exporter, без дублювання реєстрів.
5. API з аудиту; для кожної реалізованої операції потрібна перевірка сигнатур.
6. Повний CRUD Ban/VIP не може чесно заявлятися як інтеграція через відсутні public methods.
7. Фактично звірено архітектуру з прочитаними контрактами.
8. Мережеві гарантії owner RPC, UI/input та gameplay потребують багатоклієнтного тесту.
9. Наступний етап: фундамент і доступні read-only інтеграції; непогоджені розширення відкладені.
