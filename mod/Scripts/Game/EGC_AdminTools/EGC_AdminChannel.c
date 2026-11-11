// Мережевий шлюз живе на контролері власника; actor не є аргументом RPC.
modded class SCR_PlayerController
{
	protected ref EGC_AdminConsole m_EGC_AT_Console;
	protected bool m_EGC_AT_HadFocus;
	protected int m_EGC_AT_ClientSequence;
	protected int m_EGC_AT_ServerSequence;
	protected int m_EGC_AT_LastRequest;
	protected bool m_EGC_AT_HasRequest;
	protected int m_EGC_AT_Generation;
	protected ref array<ref EGC_AdminSelection> m_EGC_AT_Selections = {};

	override protected void UpdateLocalPlayerController()
	{
		super.UpdateLocalPlayerController();
		if (this != GetGame().GetPlayerController())
			return;
		InputManager input = GetGame().GetInputManager();
		if (input)
			input.AddActionListener("EGC_AT_ConsoleToggle", EActionTrigger.DOWN, EGC_AT_Toggle);
	}

	// Відкритий runtime modal не зареєстрований у MenuManager.
	// На виході повертаємо стандартний стан за наявністю меню.
	override protected void UpdateControls()
	{
		super.UpdateControls();
		bool open = m_EGC_AT_Console && m_EGC_AT_Console.IsOpen();
		if (!open && !m_EGC_AT_HadFocus)
			return;
		SCR_CharacterControllerComponent character = GetCharacterController();
		if (character)
		{
			bool disabled = open || GetGame().GetMenuManager().IsAnyMenuOpen();
			character.SetDisableViewControls(disabled);
			character.SetDisableWeaponControls(disabled);
			character.SetDisableMovementControls(disabled);
		}
		m_EGC_AT_HadFocus = open;
	}

	void EGC_AT_Toggle()
	{
		if (!m_EGC_AT_Console)
			m_EGC_AT_Console = new EGC_AdminConsole();
		if (m_EGC_AT_Console.IsOpen())
			m_EGC_AT_Console.Close();
		else
		{
			m_EGC_AT_Console.Open(this);
			EGC_AT_Send(EGC_AdminVerb.ROSTER, 0, -1, 0, "");
		}
	}

	void EGC_AT_Send(int verb, int generation, int row, int page, string reason)
	{
		if (this != GetGame().GetPlayerController() || m_EGC_AT_ClientSequence >= 2000000000)
			return;
		m_EGC_AT_ClientSequence++;
		Rpc(EGC_AT_Receive, m_EGC_AT_ClientSequence, verb, generation, row, page, reason);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void EGC_AT_Receive(int sequence, int verb, int generation, int row, int page, string reason)
	{
		if (!Replication.IsServer())
			return;
		int actorId = GetPlayerId();
		int now = System.GetTickCount();
		// До будь-якого дискового читання. Переповнення tick не блокує назавжди.
		int elapsed = now - m_EGC_AT_LastRequest;
		if (m_EGC_AT_HasRequest && elapsed >= 0 && elapsed < 500)
			return;
		m_EGC_AT_LastRequest = now;
		m_EGC_AT_HasRequest = true;
		if (sequence <= m_EGC_AT_ServerSequence || sequence <= 0 || sequence > 2000000000)
		{
			EGC_AdminAudit.Write(actorId, sequence, verb, "", "REPLAY_REJECTED", "");
			Rpc(EGC_AT_Result, "REPLAY_REJECTED");
			return;
		}
		m_EGC_AT_ServerSequence = sequence;
		if (!EGC_AdminAccess.CanUse(actorId))
		{
			m_EGC_AT_Selections.Clear();
			EGC_AdminAudit.Write(actorId, sequence, verb, "", "DENIED", "");
			Rpc(EGC_AT_Result, "DENIED: потрібні staff, admin login та відсутність VIP");
			return;
		}
		if (reason.Length() > 240 || page < 0 || page > 100 || verb < EGC_AdminVerb.ROSTER || verb > EGC_AdminVerb.KICK)
		{
			EGC_AdminAudit.Write(actorId, sequence, verb, "", "INVALID_ARGUMENT", "");
			Rpc(EGC_AT_Result, "INVALID_ARGUMENT");
			return;
		}
		reason = EGC_AdminAudit.Clean(reason).Trim();
		if (verb == EGC_AdminVerb.ROSTER)
		{
			EGC_AT_Roster(page);
			EGC_AdminAudit.Write(actorId, sequence, verb, "", "ROSTER_SENT", "");
			return;
		}
		if (verb == EGC_AdminVerb.BAN_EXPORT)
		{
			Rpc(EGC_AT_Result, EGC_AdminReadModel.BanExport(page));
			EGC_AdminAudit.Write(actorId, sequence, verb, "", "EXPORT_READ_ATTEMPT", "");
			return;
		}
		if (generation != m_EGC_AT_Generation || row < 0 || row >= m_EGC_AT_Selections.Count())
		{
			EGC_AdminAudit.Write(actorId, sequence, verb, "", "STALE_SELECTION", reason);
			Rpc(EGC_AT_Result, "STALE_SELECTION: оновіть список");
			return;
		}
		EGC_AdminSelection selection = m_EGC_AT_Selections[row];
		PlayerManager players = GetGame().GetPlayerManager();
		BackendApi backend = GetGame().GetBackendApi();
		if (!selection || !selection.Controller || !players || !backend || !players.IsPlayerConnected(selection.PlayerId) || players.GetPlayerController(selection.PlayerId) != selection.Controller || selection.Identity.IsEmpty() || backend.GetPlayerIdentityId(selection.PlayerId) != selection.Identity)
		{
			EGC_AdminAudit.Write(actorId, sequence, verb, "", "TARGET_GONE", reason);
			Rpc(EGC_AT_Result, "TARGET_GONE: оновіть список");
			return;
		}
		EGC_AdminAudit.Write(actorId, sequence, verb, selection.Identity, "ATTEMPT", reason);
		string result = EGC_AdminOperations.Execute(actorId, selection.PlayerId, verb, reason);
		// Досьє містить переноси рядків: в аудиті залишається код, не дубль персональних даних.
		string auditResult = result;
		if (verb == EGC_AdminVerb.DOSSIER)
			auditResult = "DOSSIER_RETURNED";
		EGC_AdminAudit.Write(actorId, sequence, verb, selection.Identity, auditResult, reason);
		Rpc(EGC_AT_Result, result);
	}

	protected void EGC_AT_Roster(int page)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		BackendApi backend = GetGame().GetBackendApi();
		if (!players || !backend)
			return;
		array<int> ids = {};
		players.GetPlayers(ids);
		m_EGC_AT_Selections.Clear();
		m_EGC_AT_Generation++;
		Rpc(EGC_AT_BeginRoster, m_EGC_AT_Generation, page, ids.Count());
		int first = page * 12;
		int last = Math.Min(first + 12, ids.Count());
		for (int i = first; i < last; i++)
		{
			EGC_AdminSelection selection = new EGC_AdminSelection();
			selection.PlayerId = ids[i];
			selection.Controller = players.GetPlayerController(ids[i]);
			selection.Identity = backend.GetPlayerIdentityId(ids[i]);
			m_EGC_AT_Selections.Insert(selection);
			Rpc(EGC_AT_RosterRow, m_EGC_AT_Generation, i - first, EGC_AdminAudit.Clean(players.GetPlayerName(ids[i]), 80));
		}
		Rpc(EGC_AT_Result, "Оберіть гравця. Мінімальний інтервал запитів — 0,5 с.");
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void EGC_AT_BeginRoster(int generation, int page, int total)
	{
		if (m_EGC_AT_Console)
			m_EGC_AT_Console.BeginRoster(generation, page, total);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void EGC_AT_RosterRow(int generation, int row, string label)
	{
		if (m_EGC_AT_Console)
			m_EGC_AT_Console.SetRow(generation, row, label);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void EGC_AT_Result(string message)
	{
		if (m_EGC_AT_Console)
			m_EGC_AT_Console.SetResult(message);
	}
}
