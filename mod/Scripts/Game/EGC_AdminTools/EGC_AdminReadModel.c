// Проєкції для UI: жоден із цих методів не змінює банів або VIP.
class EGC_AdminReadModel
{
	static string Dossier(int targetId)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		BackendApi backend = GetGame().GetBackendApi();
		SCR_BaseGameMode mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!Replication.IsServer() || !players || !backend || !mode || !players.IsPlayerConnected(targetId))
			return "Гравець недоступний";
		string status = "без персонажа";
		IEntity entity = players.GetPlayerControlledEntity(targetId);
		if (entity)
		{
			status = "персонаж активний";
			DamageManagerComponent damage = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
			if (damage && damage.GetState() == EDamageState.DESTROYED)
				status = "персонаж загинув";
		}
		string vip = "ні";
		if (mode.EGC_IsPlayerVIP(targetId))
			vip = "так (строки не підтримуються EGC)";
		return string.Format("%1\nID: %2\nUID: %3\nСтан: %4\nVIP: %5", EGC_AdminAudit.Clean(players.GetPlayerName(targetId), 80), targetId, backend.GetPlayerIdentityId(targetId), status, vip);
	}

	static string BanExport(int page)
	{
		if (!Replication.IsServer() || page < 0 || page > 100)
			return "Некоректна сторінка";
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile("$profile:EGC_config/EGC_Bans.json"))
			return "Експорт EGC недоступний. Це не означає відсутність банів.";
		map<string, string> entries = new map<string, string>();
		if (!context.ReadValue("activeBans", entries))
			return "Неправильний формат експорту EGC";
		string output = string.Format("Знімок EGC / стор. %1 / записів %2. Максимум 100; може бути застарілим. Навігація: Назад/Далі.\n", page + 1, entries.Count());
		int first = page * 12;
		int last = Math.Min(first + 12, entries.Count());
		for (int i = first; i < last; i++)
			output += entries.GetKey(i) + " | " + EGC_AdminAudit.Clean(entries.GetElement(i), 160) + "\n";
		return output;
	}
}
