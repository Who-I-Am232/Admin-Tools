// Власна оперативна консоль EGC: стандартні runtime-віджети, відносні anchors.
class EGC_AdminConsole : ScriptedWidgetEventHandler
{
	protected Widget m_Root;
	protected SCR_PlayerController m_Owner;
	protected TextWidget m_Output;
	protected TextWidget m_Header;
	protected EditBoxWidget m_Reason;
	protected ref array<Widget> m_RowButtons = {};
	protected ref array<TextWidget> m_RowLabels = {};
	protected int m_Generation;
	protected int m_Selected = -1;
	protected string m_SelectedName;
	protected int m_Page;
	protected int m_Total;
	protected int m_BanPage;
	protected bool m_ShowBans;
	protected bool m_ConfirmKick;
	protected int m_LastSend;
	protected bool m_HasSent;

	bool IsOpen() { return m_Root != null; }

	protected Widget Place(WidgetType type, string name, float left, float top, float right, float bottom, Color color, Widget parent)
	{
		Widget widget = GetGame().GetWorkspace().CreateWidget(type, WidgetFlags.VISIBLE, color, 0, parent);
		if (!widget)
			return null;
		widget.SetName(name);
		FrameSlot.SetAnchorMin(widget, left, top);
		FrameSlot.SetAnchorMax(widget, right, bottom);
		FrameSlot.SetOffsets(widget, 0, 0, 0, 0);
		return widget;
	}

	protected TextWidget Label(string name, string text, float left, float top, float right, float bottom, Widget parent)
	{
		TextWidget label = TextWidget.Cast(Place(WidgetType.TextWidgetTypeID, name, left, top, right, bottom, new Color(0.85, 0.88, 0.80, 1), parent));
		if (label)
		{
			label.SetFont("{3E7733BAC8C831F6}UI/Fonts/RobotoCondensed/RobotoCondensed_Regular.fnt");
			label.SetText(text);
			label.SetDesiredFontSize(18);
			label.SetMinFontSize(10);
			label.SetTextWrapping(true);
			label.SetFlags(WidgetFlags.IGNORE_CURSOR);
		}
		return label;
	}

	protected Widget Button(string name, string caption, float left, float top, float right, float bottom)
	{
		Widget button = Place(WidgetType.ButtonWidgetTypeID, name, left, top, right, bottom, new Color(0.25, 0.31, 0.21, 1), m_Root);
		if (button)
			Label(name + "_Caption", caption, 0.04, 0.05, 0.98, 0.95, button);
		return button;
	}

	void Open(SCR_PlayerController owner)
	{
		if (m_Root || !owner || !GetGame().GetWorkspace())
			return;
		m_Owner = owner;
		m_Selected = -1;
		m_ConfirmKick = false;
		m_HasSent = false;
		m_Root = GetGame().GetWorkspace().CreateWidget(WidgetType.FrameWidgetTypeID, WidgetFlags.VISIBLE, new Color(1, 1, 1, 1), 1000);
		if (!m_Root)
			return;
		m_Root.SetName("EGC_AT_OperationsSurface");
		FrameSlot.SetAnchorMin(m_Root, 0.08, 0.08);
		FrameSlot.SetAnchorMax(m_Root, 0.92, 0.92);
		FrameSlot.SetOffsets(m_Root, 0, 0, 0, 0);
		Place(WidgetType.PanelWidgetTypeID, "EGC_AT_Backdrop", 0, 0, 1, 1, new Color(0.055, 0.065, 0.055, 0.98), m_Root);
		m_Header = Label("EGC_AT_SessionHeading", "EGC / ОПЕРАТИВНА КОНСОЛЬ", 0.03, 0.02, 0.82, 0.08, m_Root);
		Button("EGC_AT_Close", "Закрити", 0.84, 0.02, 0.98, 0.08);
		for (int i = 0; i < 12; i++)
		{
			float top = 0.11 + i * 0.052;
			Widget row = Button("EGC_AT_Roster_" + i.ToString(), "", 0.03, top, 0.35, top + 0.045);
			m_RowButtons.Insert(row);
			TextWidget label;
			if (row)
				label = TextWidget.Cast(row.FindAnyWidget("EGC_AT_Roster_" + i.ToString() + "_Caption"));
			m_RowLabels.Insert(label);
			if (row)
				row.SetVisible(false);
		}
		Button("EGC_AT_Previous", "Назад", 0.03, 0.75, 0.13, 0.81);
		Button("EGC_AT_Refresh", "Оновити", 0.14, 0.75, 0.25, 0.81);
		Button("EGC_AT_Next", "Далі", 0.26, 0.75, 0.35, 0.81);
		m_Output = Label("EGC_AT_OperationReceipt", "Сервер перевіряє доступ…", 0.39, 0.11, 0.97, 0.58, m_Root);
		Label("EGC_AT_ReasonHeading", "Причина для журналу (до 240 символів)", 0.39, 0.59, 0.97, 0.64, m_Root);
		m_Reason = EditBoxWidget.Cast(Place(WidgetType.EditBoxWidgetTypeID, "EGC_AT_ReasonInput", 0.39, 0.65, 0.97, 0.72, new Color(0.75, 0.80, 0.70, 1), m_Root));
		Button("EGC_AT_Dossier", "Досьє / VIP", 0.39, 0.75, 0.57, 0.81);
		Button("EGC_AT_Heal", "Лікувати", 0.59, 0.75, 0.77, 0.81);
		Button("EGC_AT_Kick", "Kick → підтвердити", 0.79, 0.75, 0.97, 0.81);
		Button("EGC_AT_BanExport", "Експорт банів EGC (перегляд)", 0.39, 0.84, 0.78, 0.90);
		Label("EGC_AT_BuildNotice", "ПРОТОТИП / без перевірки у Workbench / F8", 0.03, 0.93, 0.97, 0.98, m_Root);
		m_Root.AddHandler(this);
		GetGame().GetWorkspace().AddModal(m_Root, m_Reason);
	}

	void Close()
	{
		if (!m_Root)
			return;
		GetGame().GetWorkspace().RemoveModal(m_Root);
		m_Root.RemoveHandler(this);
		m_Root.RemoveFromHierarchy();
		m_Root = null;
		m_Output = null;
		m_Header = null;
		m_Reason = null;
		m_RowButtons.Clear();
		m_RowLabels.Clear();
		m_Selected = -1;
		m_ConfirmKick = false;
	}

	void BeginRoster(int generation, int page, int total)
	{
		if (!m_Root)
			return;
		m_Generation = generation;
		m_Page = page;
		m_Total = total;
		m_Selected = -1;
		m_ConfirmKick = false;
		if (m_Header)
			m_Header.SetText(string.Format("EGC / ОНЛАЙН %1 / СТОРІНКА %2", total, page + 1));
		foreach (Widget row : m_RowButtons)
		{
			if (row)
				row.SetVisible(false);
		}
	}

	void SetRow(int generation, int row, string text)
	{
		if (!m_Root || generation != m_Generation || row < 0 || row >= m_RowButtons.Count())
			return;
		if (m_RowButtons[row])
			m_RowButtons[row].SetVisible(true);
		if (m_RowLabels[row])
			m_RowLabels[row].SetText(text);
	}

	void SetResult(string message)
	{
		if (m_Output)
			m_Output.SetText(message);
	}

	protected void Send(int verb, int page = 0)
	{
		int now = System.GetTickCount();
		if (m_HasSent && now - m_LastSend >= 0 && now - m_LastSend < 650)
		{
			SetResult("Зачекайте 0,65 с між запитами та повторіть дію.");
			return;
		}
		if (verb != EGC_AdminVerb.ROSTER && verb != EGC_AdminVerb.BAN_EXPORT && m_Selected < 0)
		{
			SetResult("Спочатку оберіть гравця зліва.");
			return;
		}
		m_LastSend = now;
		m_HasSent = true;
		string reason;
		if (m_Reason)
			reason = m_Reason.GetText();
		if (m_Owner)
			m_Owner.EGC_AT_Send(verb, m_Generation, m_Selected, page, reason);
	}

	override bool OnClick(Widget widget, int x, int y, int button)
	{
		if (!m_Root || !widget)
			return false;
		for (int i = 0; i < m_RowButtons.Count(); i++)
		{
			if (widget != m_RowButtons[i])
				continue;
			m_ShowBans = false;
			m_Selected = i;
			if (m_RowLabels[i])
				m_SelectedName = m_RowLabels[i].GetText();
			if (m_Header)
				m_Header.SetText("EGC / ЦІЛЬ: " + m_SelectedName);
			m_ConfirmKick = false;
			Send(EGC_AdminVerb.DOSSIER);
			return true;
		}
		string name = widget.GetName();
		if (name != "EGC_AT_Kick")
			m_ConfirmKick = false;
		switch (name)
		{
			case "EGC_AT_Close": Close(); return true;
			case "EGC_AT_Refresh": m_ShowBans = false; Send(EGC_AdminVerb.ROSTER, m_Page); return true;
			case "EGC_AT_Previous":
				if (m_ShowBans)
				{
					if (m_BanPage > 0) m_BanPage--;
					Send(EGC_AdminVerb.BAN_EXPORT, m_BanPage);
				}
				else if (m_Page > 0) Send(EGC_AdminVerb.ROSTER, m_Page - 1);
				return true;
			case "EGC_AT_Next":
				if (m_ShowBans)
				{
					if (m_BanPage < 8) m_BanPage++;
					Send(EGC_AdminVerb.BAN_EXPORT, m_BanPage);
				}
				else if ((m_Page + 1) * 12 < m_Total) Send(EGC_AdminVerb.ROSTER, m_Page + 1);
				return true;
			case "EGC_AT_Dossier": Send(EGC_AdminVerb.DOSSIER); return true;
			case "EGC_AT_Heal": Send(EGC_AdminVerb.HEAL); return true;
			case "EGC_AT_BanExport":
				m_ShowBans = true;
				m_BanPage = 0;
				Send(EGC_AdminVerb.BAN_EXPORT, m_BanPage);
				return true;
			case "EGC_AT_Kick":
				if (m_Selected < 0 || !m_Reason || m_Reason.GetText().IsEmpty())
				{
					SetResult("Оберіть ціль та введіть причину.");
					return true;
				}
				if (!m_ConfirmKick)
				{
					m_ConfirmKick = true;
					SetResult("Підтвердьте Kick повторним натисканням. Ціль: " + m_SelectedName);
					return true;
				}
				m_ConfirmKick = false;
				Send(EGC_AdminVerb.KICK);
				return true;
		}
		return false;
	}
}
