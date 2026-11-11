// Незалежний префікс у серверному журналі; ротація — засобами хостингу.
class EGC_AdminAudit
{
	static string Clean(string value, int limit = 240)
	{
		value.Replace("\n", " ");
		value.Replace("\r", " ");
		value.Replace("\t", " ");
		value.Replace("|", "/");
		if (value.Length() > limit)
			return value.Substring(0, limit);
		return value;
	}

	static void Write(int actorId, int sequence, int verb, string targetIdentity, string result, string reason)
	{
		if (!Replication.IsServer())
			return;
		string actorIdentity;
		BackendApi backend = GetGame().GetBackendApi();
		if (backend)
			actorIdentity = backend.GetPlayerIdentityId(actorId);
		int year, month, day, hour, minute, second;
		System.GetYearMonthDay(year, month, day);
		System.GetHourMinuteSecond(hour, minute, second);
		string stamp = string.Format("%1-%2-%3T%4:%5:%6", year, month, day, hour, minute, second);
		Print(string.Format("[EGC_AT_AUDIT] time=%1|actor=%2|uid=%3|seq=%4|verb=%5|targetUid=%6|result=%7|reason=%8", stamp, actorId, Clean(actorIdentity), sequence, verb, Clean(targetIdentity), Clean(result), Clean(reason)));
	}
}
