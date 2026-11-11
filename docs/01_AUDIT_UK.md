# Етап 1 — аудит вихідних матеріалів

Дата: 2026-11-11. Статус: інвентаризацію всіх архівів і дослідження критичних інтеграцій виконано. Це статичний аудит, не сертифікація сумісності гри. Повні SHA-256, шляхи, класи та розміри кожного файла: `archive_manifest.json`. Посилання між EGC-символами: `egc_dependencies.json`; читабельний перелік: `EGC_INDEX.md`. Автоматичний індекс не є доказом правильності всіх алгоритмів; глибокий аналіз зосереджено на правах, ідентифікації, банах, VIP, мережі та збереженні.

## Матеріали та ліцензії

| Архів | Файли (без директорій) | Скрипти .c | Структура |
|---|---:|---:|---|
| Arma-Reforger-Script-Diff | 6721 | 6719 | scripts/Core, GameLib, Game, WorkbenchGame; generated API, gameplay, UI |
| EGC_scripts | 92 | 84 | scripts/Game, Components, SafeZone, DFF, Editor, UserActions, UI, GameMode, Ranks, Campaign |
| CasualAdminTools | 159 | 38 | Scripts/Game/CGG_AdminTools/Client, UI/layouts, Configs/System, матеріали та префаби |
| WIU_AdminTools | 130 | 118 | Scripts/Game/Admin/Core, Integration, UI/Panel2; Configs, UI |

У базової гри є LICENSE та README з Arma Public License: атрибуція, некомерційне використання, лише Arma. В архівах CasualAdminTools, WIU_AdminTools та EGC немає окремого LICENSE/NOTICE/README. Право копіювати код референсів не підтверджене. Їх код, ресурси та макети не включаються до нового моду. Архів EGC надано як основу інтеграції; це не встановлює права поширювати сторонні залежності.

Архів гри не містить підтвердженого номера збірки 1.9 у README. Тому коректне формулювання: API підтверджено в наданому snapshot, а не перевірено виконанням на 1.9. Для остаточної перевірки потрібен Workbench відповідної збірки.

## EGC: джерела істини

### EGC_BanSystem.c — прочитано повністю

`SCR_BaseGameMode.OnGameStart` запускає `EGC_BanSystem.Init` тільки для Dedicated/Listen. `StartAutoExport` планує перший Export через 10 секунд, наступні кожні 300000 мс. Export бере `BackendApi.GetBanServiceApi`, створює збережені callback і параметри, запитує `RequestServerBanList` з limit=100, offset=0.

`EGC_BanExporterCallback.OnSuccess` читає поточну сторінку через `GetPageItems`, очищує map і записує identityId -> текст `Name | Reason | Created | Expires` до `$profile:EGC_config/EGC_Bans.json`, ключ `activeBans`. `expiresAt=0` позначає безстроковий бан. UID походить із `OnlineBanListData.identityId`, а не з імені гравця.

**У цьому файлі немає Ban, Unban, перевірки входу, власного enforcement, завантаження JSON як бази банів або інформації про адміністратора.** Фактичним джерелом банів є серверний BanService; JSON — знімок першої сторінки, а не повний авторитетний реєстр. Не можна використовувати JSON для дозволу входу або стверджувати, що він містить усі бани.

Виявлені обмеження: немає пагінації понад 100; немає OnError/OnTimeout; немає захисту від паралельного Export та повторного Init; callback використовує спільну сторінку BanService; Save не перевіряє результат. Новий мод не виправляє це в оригіналі та не створює другого списку блокування.

Варіанти інтеграції: (1) лише читати експорт із явним попередженням про неповноту; (2) окремий погоджений адаптер запису до того самого BanService, з callback результатами, без власного enforcement. API запису присутні в snapshot: `CreateBanPlayerId`, `CreateBanIdentityId`, `RemoveBans`; успіх повернення bool означає прийняття запиту, не завершення операції. Розширення контракту за межі наявного EGC_BanSystem винесено на уточнення користувача.

### EGC_VIPManager.c — прочитано повністю

`EGC_VIPConfig.vips` — map UID -> опис; Load/Save використовують `$profile:EGC_config/EGC_Vips.json`. Полів часу, тарифу, подовження або типу VIP немає. GameMode зберігає protected `m_EGC_VIPConfig`, завантажує на серверному OnGameStart. `EGC_IsPlayerVIP(playerId)` резолвить UID через BackendApi і перевіряє Contains. Watchdog працює щосекунди, кожні 300 викликів перечитує конфіг.

VIP + ADMINISTRATOR/SESSION_ADMINISTRATOR: знищення персонажа, зняття ADMINISTRATOR/SESSION_ADMINISTRATOR/GAME_MASTER, kick. Окремий RPC на PlayerController, без actor-параметра, викликає `EGC_CheckAndPunishVIP(GetPlayerId())` при input action `CGG_AdminToggle`. Це карає будь-якого VIP, який викликає таку дію, навіть без адмін-ролі. Новий інтерфейс не повинен використовувати цю дію.

`OnPlayerRoleChange` -> `EGC_ValidateAdmin`: до 5 повторів отримання UID; VIP обробляється власним watchdog; інші адміністратори перевіряються через EGC_StaffList. При порожньому staff guard пропускає перевірку. Нова адмінка повинна відмовляти в доступі при відсутності позитивного запису staff; це політика нового інструменту, не зміна guard.

Публічного методу зміни живої VIP-конфігурації немає. Створення нового EGC_VIPConfig і Save використає той самий файл, але залишить живий кеш старим до reload та створить ризик перезапису. Це не надійна інтеграція запису. Для негайного Grant/Revoke потрібен узгоджений додатковий modded-фрагмент GameMode з доступом до існуючого protected config, Save із перевіркою результату та відновленням при помилці. Оригінальний файл можна фізично не редагувати, однак поведінка існуючого класу розшириться — це окреме рішення, не прихована зміна.

Терміни/продовження VIP неможливі за поточною моделлю даних без зміни її принципів. Альтернатива без зміни — постійне членство та опис, керовані чинним EGC-процесом; нова панель читає `EGC_IsPlayerVIP`.

### Ідентифікація та права

`EGC_StaffList.c`: EGC_StaffConfig.staff UID -> опис, `$profile:EGC_config/EGC_Staff.json`; Reload/IsStaff/HasEntries — серверна основа. SeedDefaults існує, але Load його не викликає. Не переносити захардкоджені UID у нову систему. `EGC_SetStaff` передає UI-прапорець через Owner RPC; `IsLocalPlayerStaff` — лише відображення, не серверна авторизація.

`GameMode/SCR_BaseGameModeInternal.c`: через 5 секунд після OnPlayerAuditSuccess перечитує staff, надсилає прапорець контролеру, дає GAME_MASTER staff-гравцеві, якщо SCR_Global.IsAdmin=false. GAME_MASTER окремо не означає повний адмін-доступ. Для нового інструменту: підключений playerId, непорожній backend UID, staff.Contains(uid), не VIP, наявна ADMINISTRATOR або SESSION_ADMINISTRATOR. Нова база ролей не потрібна. Розширення permissions можливе через окремий evaluator, але первинне джерело членства — EGC.

`EGC_PlayerRecord.c`: UID/нік/FirstSeen/LastSeen, історія у EGC_PlayersDatabase.json; на OnPlayerAuditSuccess UID вже доступний; fallback INTERNAL_<id> призначений для локального логування, його заборонено використовувати як адмін-ідентичність. EOnInit/логування не мають явного серверного guard у всіх місцях — це ризик вихідного коду, не виправлено.

`EGC_PlayerOnline.c`: окрема проєкція онлайн-гравців у EGC_PlayersOnline.json; очищення при старті, додавання/видалення через події. Новий список онлайн бере PlayerManager, не дискову проєкцію. `EGC_SaveSystem.c` пише wipe-маркер при OnGameModeEnd, не є універсальним persistence сервісом адмінки. `EGC_AdminSpy.c` приховує GM-іконку і реплікує глобальний прапорець; не дорівнює невидимості персонажа.

### Решта EGC та конфлікти

Усі 84 скрипти індексовано з визначеннями класів та EGC-посиланнями. Групи: safezone/захист HQ; очищення трупів/сутностей; AFK і logout; faction/team/rank/budget; транспорт/зброя; HUD/GPS/map; friendly fire; запис матчів; серверні правила. Численні modded SCR_BaseGameMode, SCR_PlayerController і damage-класи залежать від коректних super-викликів та порядку завантаження addon. `.c.bk` — резервна копія, не встановлювати як активний скрипт.

`EGC_AdminToolsLogout.c` розширює **ServerAdminTools_PauseMenuButtonComponent**, якого немає серед визначень усіх чотирьох архівів. Потрібен встановлений addon, який його визначає. Не можна вилучити залежність, не змінюючи EGC або його комплектування. `ACE_Trenches_OutlineGarbageComponent` — ще одна зовнішня основа. Немає .gproj і повного manifest залежностей EGC, тому неможливо назвати Workshop GUID цих залежностей без додаткових матеріалів.

Heal/Revive/God взаємодіють із EGC_ConsciousnessSystem, damage-модифікаціями DFF, HQ/SafeZone/BushCamping; teleport — із faction/vehicle locks і safezones; автоматичне очищення — зі spawn інструментами; AdminSpy — із spectator/GM. Потрібні тести саме в повному EGC-наборі.

## Референси

### CasualAdminTools

Класи CGG_AdminAccessService, CGG_AdminRolePermissions — конфіг access.json, іменовані ролі, granular permissions, пріоритети; root визначається поза JSON через backend/власника. CGG_AdminBanService використовує **власний** bans.json та reconnect enforcement, а не EGC. Його модель банів непридатна для інтеграції за ТЗ.

Client-директорія містить і серверні RPC на modded SCR_PlayerController: назва папки не означає клієнтський авторитет. Патерн: Server RPC -> GetPlayerId -> access-check -> дія -> Owner RPC результату. CGG_AdminClientInput, AdminMenuUI/UIComponent, QuickActions, SpectatorTool, ServerActions, ExtensionRegistry, support-сервіси дають масштабний набір функцій. UI має 48 layout, текстури, overlays. Вивчені стандартні механізми, а не перенесені класи чи оформлення. Наявність access-check в окремих обробниках не є аудитом безпеки всіх RPC референсу.

### WIU_AdminTools

Core/WIU_AdminAccessService: admins.json, role/permission, priority, root; Permissions — відображення action -> permission; ActionRouter — авторизація й dispatch; DutyService — стан duty; Audit/DeferredWriter/Retention — журнал і обслуговування. Integration/WIU_AdminPlayerController — запити Server, відповіді Owner, actor із GetPlayerId, обмеження частоти деяких запитів. PlayerService/Vitals/World/Observation — приклад поділу відповідальності.

WIU_AdminBanService також зберігає власні bans.json, викликає NotifyAndKick і перевіряє reconnect. Не використовувати цю модель для EGC. UI/Core/Integration та Panel2 — структура самого референсу, не шаблон нового моду. Автоматизація, jail, loadout, tickets, Discord, ESP є окремими модулями; вони не стають залежностями EGC Admin Tools.

## API, підтверджені в snapshot

| Механізм | Джерело під scripts/ |
|---|---|
| GetPlayers/GetPlayerController/IsPlayerConnected/HasPlayerRole/KickPlayer | Game/generated/Network/PlayerManager.c |
| GetPlayerId | Game/generated/Player/PlayerController.c |
| backend identity | GameLib/generated/online/BackendApi.c; Game/Utilities/SCR_PlayerIdentityUtils.c |
| Ban CRUD/page/callback | GameLib/generated/online/BanServiceApi.c; BackendCallback.c |
| RplRpc/RplRcver.Owner/Server | Core/proto/EnNetwork.c; Game/Player/SCR_PlayerController.c та стандартні gameplay RPC |
| UI widgets/handler/modal/FrameSlot | Core/generated/UI; Core/proto/EnWidgets.c |
| local input hook | Game/Player/SCR_PlayerController.c: UpdateLocalPlayerController |
| FullHeal(bool) | Game/Components/Damage/SCR_CharacterDamageManagerComponent.c |
| SCR_Global.TeleportPlayer | Game/Global/Functions.c |
| clock | Core/generated/System/System.c |
| JSON context | Game/Serialization (шлях уточнює індекс API при реалізації) |

TeleportPlayer також телепортує транспорт, якщо гравець у ньому. Першій версії потрібна відмова для seated-гравців або окрема явна політика. Просте SetOrigin не гарантує коректної синхронізації. FullHeal не слід рекламувати як resurrect мертвого персонажа. Для spectator/невидимості/god потрібна окрема оцінка серверної реплікації та конфліктів; самі назви функцій референсу не доводять сумісність.

## Звіт етапу

1. Досліджено архіви, ліцензії, критичні EGC-файли та мережеві/адміністративні механізми референсів.
2. Створено цей звіт, manifest, індекс класів/залежностей та audit_archives.py.
3. Оригінальні файли та архіви не змінено; перевірено побайтово за SHA-256.
4. Основа інтеграції: EGC_BanSystem, EGC_VIPManager, EGC_StaffList, GameMode audit hook.
5. API наведено вище; наявність сигнатури не дорівнює runtime-перевірці.
6. Проблеми: BanSystem не надає CRUD, VIP не має строків/публічних setters, відсутня зовнішня UI-основа та addon manifests, непідтверджений build snapshot.
7. Фактично виконано розпакування, читання/пошук, інвентаризацію й перевірку незмінності.
8. Компіляція, RPC ownership, backend, dedicated/listen та повний EGC-набір ще потребують Workbench/гри.
9. Наступний етап: незалежна архітектура з явними межами підтримки та погодженням інтеграцій, які розширюють існуючі контракти.

## Додаткова перевірка застарілих API

Оригінальний EGC exporter перевизначає `BackendCallback.OnSuccess(int)`: він присутній у snapshot, але позначений Obsolete на користь SetOnSuccess. Оригінальні SCR_JsonLoadContext/SCR_JsonSaveContext також позначені Obsolete у Game/Plugins/Serialization/BackwardsCompatiblity.c. Це не причина самовільно редагувати EGC. Новий read model використовує сучасний JsonLoadContext. Правила owner RPC підтверджено таблицею в Core/generated/Replication/RplRcver.c: Client + not owner + Server -> dropped. Це документація snapshot; фактичний hostile-client тест ще не виконано.
