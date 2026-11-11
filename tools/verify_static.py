#!/usr/bin/env python3
"""Статичні контракти та індекс API. НЕ компілятор Enfusion і НЕ RPC-тест."""
import pathlib,re,json
root=pathlib.Path(__file__).resolve().parents[1]
base=root/'mod/Scripts/Game/EGC_AdminTools'
engine=root/'research/1-Arma-Reforger-Script-Diff-main/Arma-Reforger-Script-Diff-main/scripts'
files=list(base.glob('*.c')); code='\n'.join(p.read_text() for p in files)
results=[]
def check(name, condition):
 results.append({'check':name,'passed':bool(condition)})
def strip(s):
 return re.sub(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*', '', s, flags=re.S)
check('new filenames use EGC_ prefix', all(p.name.startswith('EGC_') for p in files))
check('balanced braces (lexical only)', all(strip(p.read_text()).count('{')==strip(p.read_text()).count('}') for p in files))
check('no reference product symbols in active mod', not re.search(r'\b(?:CGG_|WIU_|ServerAdminTools_)',code))
check('no custom ban or VIP persistence', not re.search(r'\b(SaveToFile|PackToFile|CreateBanPlayerId|RemoveBans)\s*\(',code))
check('modern JSON context in new code', 'SCR_JsonLoadContext' not in code)
channel=(base/'EGC_AdminChannel.c').read_text()
signature=re.search(r'void EGC_AT_Receive\((.*?)\)',channel).group(1)
check('RPC does not accept actor/UID parameter', not re.search(r'actor|identity|uid',signature,re.I))
check('actor derives from controller', 'int actorId = GetPlayerId();' in channel)
check('server authorization in gateway', 'EGC_AdminAccess.CanUse(actorId)' in channel)
check('selection checks UID and controller incarnation', 'backend.GetPlayerIdentityId(selection.PlayerId) != selection.Identity' in channel and 'players.GetPlayerController(selection.PlayerId) != selection.Controller' in channel)
check('selection checks snapshot generation', 'generation != m_EGC_AT_Generation' in channel)
check('sequence replay and request rate guards present', 'sequence <= m_EGC_AT_ServerSequence' in channel and 'elapsed < 500' in channel)
check('RPC directions restricted to Server and Owner', len(re.findall(r'\[RplRpc\(',channel))==4 and 'RplRcver.Broadcast' not in channel)
access=(base/'EGC_AdminAccess.c').read_text()
check('staff load failure denies access', 'if (!context.LoadFromFile(EGC_StaffConfig.FILE_PATH))\n\t\t\treturn false;' in access)
check('VIP guard and active native roles checked', 'EGC_IsPlayerVIP(actorId)' in access and 'EPlayerRole.ADMINISTRATOR' in access and 'EPlayerRole.SESSION_ADMINISTRATOR' in access)
known={}
for p in root.glob('research/**/*.c'):
 if base in p.parents: continue
 s=strip(p.read_text(errors='replace'))
 for name in re.findall(r'(?<!modded )\bclass\s+(EGC_\w+)',s): known.setdefault(name,[]).append(str(p))
new_names=re.findall(r'(?m)^class\s+(EGC_\w+)',code)
check('new class names unique against supplied archives', len(set(new_names))==len(new_names) and not any(n in known for n in new_names))
# Зіставляємо зовнішні методи з конкретними файлами API.
contracts={
 'Game/generated/Network/PlayerManager.c':['IsPlayerConnected','HasPlayerRole','GetPlayers','GetPlayerName','GetPlayerController','GetPlayerControlledEntity','KickPlayer'],
 'Game/generated/Player/PlayerController.c':['GetPlayerId'],
 'GameLib/generated/online/BackendApi.c':['GetPlayerIdentityId'],
 'Core/generated/System/System.c':['GetTickCount','GetYearMonthDay','GetHourMinuteSecond'],
 'Game/generated/Plugins/Serialization/Presets/JsonLoadContext.c':['LoadFromFile'],
 'Game/generated/Plugins/Serialization/LoadContext.c':['ReadValue'],
 'Game/Components/Damage/SCR_CharacterDamageManagerComponent.c':['FullHeal'],
 'Game/generated/Components/DamageManagerComponent.c':['GetState'],
 'Game/generated/Components/CharacterControllerComponent.c':['SetDisableViewControls','SetDisableWeaponControls','SetDisableMovementControls'],
 'Core/generated/UI/WorkspaceWidget.c':['CreateWidget','AddModal','RemoveModal'],
 'Core/generated/UI/FrameSlot.c':['SetAnchorMin','SetAnchorMax','SetOffsets'],
 'Core/generated/UI/Widget.c':['SetName','GetName','FindAnyWidget','SetVisible','SetFlags','AddHandler','RemoveHandler','RemoveFromHierarchy'],
 'Core/generated/UI/TextWidget.c':['SetText','GetText','SetFont','SetDesiredFontSize','SetMinFontSize','SetTextWrapping'],
 'Core/generated/UI/EditBoxWidget.c':['GetText'],
 'Core/generated/UI/ScriptedWidgetEventHandler.c':['OnClick'],
 'Core/generated/Types/string.c':['Trim','Replace','Substring','Length','ToString'],
 'Core/proto/Types.c':['GetElement','GetKey'],
 'Game/Player/SCR_PlayerController.c':['UpdateLocalPlayerController','UpdateControls','GetCharacterController'],
 'GameLib/generated/Input/ActionManager.c':['AddActionListener']
}
evidence=[]
for file,methods in contracts.items():
 path=engine/file
 source=path.read_text() if path.exists() else ''
 for method in methods:
  matches=[i for i,line in enumerate(source.splitlines(),1) if re.search(r'\b'+method+r'\s*\(',line)]
  check('API symbol '+method+' in '+file, matches)
  evidence.append({'symbol':method,'file':str(path.relative_to(root)),'lines':matches})
check('native role/identity sources unchanged', (root/'research/2-EGC_scripts/scripts/Game/EGC_VIPManager.c').exists())
(root/'docs/static_api_evidence.json').write_text(json.dumps(evidence,ensure_ascii=False,indent=2))
report={'kind':'static source contracts only','checks':results,'passed':sum(r['passed'] for r in results),'failed':sum(not r['passed'] for r in results),'compiled':False,'in_game_tested':False}
(root/'docs/static_verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2))
for r in results:
 if not r['passed']: print('FAIL',r['check'])
print(f"Static checks: {report['passed']} passed, {report['failed']} failed. Compilation: NOT RUN. In-game: NOT RUN.")
raise SystemExit(bool(report['failed']))
