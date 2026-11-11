// Авторизація за чинними правилами EGC, без другої бази персоналу.
class EGC_AdminAccess
{
	static bool CanUse(int actorId)
	{
		if (!Replication.IsServer() || actorId <= 0)
			return false;
		PlayerManager players = GetGame().GetPlayerManager();
		BackendApi backend = GetGame().GetBackendApi();
		SCR_BaseGameMode mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!players || !backend || !mode || !players.IsPlayerConnected(actorId))
			return false;
		string identity = backend.GetPlayerIdentityId(actorId);
		if (identity.IsEmpty())
			return false;
		// Читаємо той самий EGC-файл без створення власної бази.
		// Оригінальний Reload не сигналізує про помилку і може лишити старий кеш.
		JsonLoadContext context = new JsonLoadContext();
		map<string, string> currentStaff = new map<string, string>();
		if (!context.LoadFromFile(EGC_StaffConfig.FILE_PATH))
			return false;
		if (!context.ReadValue("staff", currentStaff) || !currentStaff.Contains(identity))
			return false;
		EGC_StaffList.Reload();
		if (!EGC_StaffList.HasEntries() || !EGC_StaffList.IsStaff(identity))
			return false;
		if (mode.EGC_IsPlayerVIP(actorId))
			return false;
		return players.HasPlayerRole(actorId, EPlayerRole.ADMINISTRATOR) || players.HasPlayerRole(actorId, EPlayerRole.SESSION_ADMINISTRATOR);
	}
}
