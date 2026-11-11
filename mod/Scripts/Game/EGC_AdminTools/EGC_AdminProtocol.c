// Закритий протокол: клієнт не передає UID адміністратора або довільний код.
enum EGC_AdminVerb
{
	ROSTER = 1,
	DOSSIER,
	BAN_EXPORT,
	HEAL,
	KICK
}

// Знімок цілі зберігається виключно на серверному контролері.
class EGC_AdminSelection
{
	int PlayerId;
	PlayerController Controller;
	string Identity;
}
