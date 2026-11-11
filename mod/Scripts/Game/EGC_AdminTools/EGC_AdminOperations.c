// Вузькі серверні дії. Авторизацію та прив'язку UID виконує канал перед викликом.
class EGC_AdminOperations
{
	static string Execute(int actorId, int targetId, int verb, string reason)
	{
		if (!EGC_AdminAccess.CanUse(actorId))
			return "DENIED";
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || !players.IsPlayerConnected(targetId))
			return "TARGET_GONE";
		if (verb == EGC_AdminVerb.DOSSIER)
			return EGC_AdminReadModel.Dossier(targetId);
		if (verb == EGC_AdminVerb.KICK)
		{
			if (targetId == actorId || reason.IsEmpty())
				return "INVALID_ARGUMENT";
			// KickPlayer не має reason string або callback підтвердження відключення.
			players.KickPlayer(targetId, PlayerManagerKickReason.KICK, 0);
			return "KICK_REQUESTED";
		}
		if (verb == EGC_AdminVerb.HEAL)
		{
			IEntity entity = players.GetPlayerControlledEntity(targetId);
			if (!entity)
				return "NO_CHARACTER";
			SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(entity.FindComponent(SCR_CharacterDamageManagerComponent));
			if (!damage || damage.GetState() == EDamageState.DESTROYED)
				return "NOT_ALIVE";
			damage.FullHeal();
			return "HEAL_APPLIED";
		}
		return "UNKNOWN_COMMAND";
	}
}
