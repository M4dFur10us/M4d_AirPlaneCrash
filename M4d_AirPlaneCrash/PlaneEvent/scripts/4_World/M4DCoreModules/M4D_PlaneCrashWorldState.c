// =====================================================================================
// M4D_PlaneCrashWorldState.c - VERSÃO DEFINITIVA (Painel Vivo, Auditoria e MarkContainerDead)
// Responsabilidade: Índice centralizado em RAM, validação profunda e snapshot seguro.
// =====================================================================================

// =====================================================================================
// 1. ESTRUTURA DO ESTADO DO EVENTO (A "Ficha" de cada Crash)
// =====================================================================================
class M4D_EventState
{
	int EventID;

	vector WreckPosition;
	int Tier;
	bool HasGas;
	bool KeyDropped;

	bool WreckAlive;
	bool ContainerAlive;
	string ContainerType;
	bool ContainerUnlocked;

	int DetailAssetsCount;
	int ZombiesCount;
	int AnimalsCount;
	int GasAreasCount;

	bool WasRestored;
	bool IsConsistent;
	string LastProblem;

	int CreatedAtMs;
	int LastHeartbeatMs;
	int LastRestoreMs;

	void M4D_EventState(int id, vector pos, int tier, bool gas)
	{
		EventID = id;
		WreckPosition = pos;
		Tier = tier;
		HasGas = gas;
		KeyDropped = false;

		WreckAlive = false;
		ContainerAlive = false;
		ContainerType = "Unknown";
		ContainerUnlocked = false;

		DetailAssetsCount = 0;
		ZombiesCount = 0;
		AnimalsCount = 0;
		GasAreasCount = 0;

		WasRestored = false;
		IsConsistent = true;
		LastProblem = "Nenhum";

		if (GetGame()) {
			CreatedAtMs = GetGame().GetTime();
			LastHeartbeatMs = CreatedAtMs;
		}
		LastRestoreMs = 0;
	}

	void Heartbeat() {
		if (GetGame()) LastHeartbeatMs = GetGame().GetTime();
	}

	// MALHA DE CONSISTÊNCIA PROFUNDA
	void Validate()
	{
		IsConsistent = true;
		LastProblem = "Nenhum";

		int currentTime = 0;
		if (GetGame()) currentTime = GetGame().GetTime();
		
		int ageMs = currentTime - CreatedAtMs;

		// 1. Órfão Inverso: Contentor vivo mas Wreck apagado da memória.
		if (!WreckAlive && ContainerAlive) {
			IsConsistent = false; LastProblem = "Órfão: Contentor vivo sem Wreck."; return;
		}

		// 2. Fuga de Contentor: Wreck vivo há mais de 2 minutos, mas sem contentor associado.
		if (WreckAlive && !ContainerAlive && ageMs > 120000) {
			IsConsistent = false; LastProblem = "Desconexão: Wreck ativo há muito tempo sem gerar/reportar contentor."; return;
		}

		// 3. Restauro Vazio: Core leu do .bin, mas o mundo está limpo.
		if (WasRestored && WreckAlive && DetailAssetsCount == 0 && !ContainerAlive) {
			IsConsistent = false; LastProblem = "Restauro Fantasma: Wreck existe mas sem assets e sem contentor associado."; return;
		}

		// 4. Contadores Absurdos: Vazamento de memória ou subtração indevida em algum array.
		if (DetailAssetsCount < 0 || ZombiesCount < 0 || AnimalsCount < 0 || GasAreasCount < 0) {
			IsConsistent = false; LastProblem = "Corrupção Matemática: Contadores de assets reportam valores negativos."; return;
		}

		// 5. Morte Silenciosa: Wreck parou de enviar Heartbeats há mais de 10 minutos.
		if ((currentTime - LastHeartbeatMs) > 600000) {
			IsConsistent = false; LastProblem = "Morte Silenciosa: Wreck ativo não envia heartbeat há mais de 10 min."; return;
		}

		// 6. WARNING OPERACIONAL: Vida útil muito estendida (Não invalida o evento, apenas avisa).
		if (ageMs > 2400000) { // 40 minutos
			LastProblem = "WARNING: Evento em RAM há mais de 40 minutos (Pausa longa ou anomalia?).";
		}
	}
}

// =====================================================================================
// 2. CLASSE WRAPPER PARA SNAPSHOT JSON (Contorna a limitação do Enforce com Maps)
// =====================================================================================
class M4D_WorldStateSnapshot
{
	int ActiveValidEvents;
	int ActiveInconsistentEvents;
	string LastSaved;
	ref array<ref M4D_EventState> TrackedEvents;

	void M4D_WorldStateSnapshot() { TrackedEvents = new array<ref M4D_EventState>(); }
}

// =====================================================================================
// 3. O REGISTO CENTRAL (Dashboard)
// =====================================================================================
class M4D_PlaneCrashWorldState
{
	private static ref map<int, ref M4D_EventState> s_Events;
	private static int s_LastSnapshotMs = 0; // Trava Térmica de I/O
	static const string SNAPSHOT_PATH = "$profile:M4D_AirPlaneCrash/WorldStateSnapshot.json";

	static void EnsureInit() {
		if (!s_Events) s_Events = new map<int, ref M4D_EventState>();
	}

	// ---------------------------------------------------------------------------------
	// API DE LIFECYCLE E REIDRATAÇÃO
	// ---------------------------------------------------------------------------------
	
	static void OpenEvent(int id, vector pos, int tier, bool hasGas) {
		EnsureInit();
		if (!s_Events.Contains(id)) {
			s_Events.Insert(id, new M4D_EventState(id, pos, tier, hasGas));
			M4D_PlaneCrashLogger.Debug(string.Format("[WorldState] Ficha aberta (EventID: %1)", id));
		}
	}

	static void MarkWreckAlive(int id, vector pos, int realTier = -1, bool realGas = false) {
		EnsureInit();
		M4D_EventState state = s_Events.Get(id);
		if (state) {
			state.WreckAlive = true;
			state.WreckPosition = pos;
			if (realTier != -1) state.Tier = realTier;
			state.HasGas = realGas;
			state.Heartbeat();
		} else {
			// Fallback de inicialização de Restart (Correção de Sintaxe Enforce)
			int tierToUse = 0;
			if (realTier != -1) {
				tierToUse = realTier;
			}
			
			OpenEvent(id, pos, tierToUse, realGas);
			s_Events.Get(id).WreckAlive = true;
		}
	}

	static void Heartbeat(int id) {
		EnsureInit();
		M4D_EventState state = s_Events.Get(id);
		if (state) state.Heartbeat();
	}

	static void MarkContainerAlive(int id, string type) {
		EnsureInit();
		M4D_EventState state = s_Events.Get(id);
		if (state) { state.ContainerAlive = true; state.ContainerType = type; state.Heartbeat(); }
	}

	static void MarkContainerUnlocked(int id) {
		EnsureInit();
		M4D_EventState state = s_Events.Get(id);
		if (state) { state.ContainerUnlocked = true; state.Heartbeat(); }
	}

	// ETAPA 5: O AVISO DE MORTE DO CONTAINER
	static void MarkContainerDead(int id) {
		EnsureInit();
		M4D_EventState state = s_Events.Get(id);
		if (state) { 
			state.ContainerAlive = false; 
			state.Heartbeat(); 
			state.LastProblem = "INFO: Contentor removido antes do encerramento do Wreck.";
			M4D_PlaneCrashLogger.Debug(string.Format("[WorldState] Contentor declarado morto (EventID: %1)", id));
		}
	}

	static void UpdateCounts(int id, int assets, int zombies, int animals, int gas) {
		EnsureInit();
		M4D_EventState state = s_Events.Get(id);
		if (state) {
			state.DetailAssetsCount = assets; state.ZombiesCount = zombies;
			state.AnimalsCount = animals; state.GasAreasCount = gas;
			state.Heartbeat();
		}
	}

	static void MarkKeyDropped(int id) {
		EnsureInit();
		M4D_EventState state = s_Events.Get(id);
		if (state) { state.KeyDropped = true; state.Heartbeat(); }
	}

	static void MarkRestored(int id) {
		EnsureInit();
		M4D_EventState state = s_Events.Get(id);
		if (state) {
			state.WasRestored = true;
			if (GetGame()) state.LastRestoreMs = GetGame().GetTime();
			state.Heartbeat();
		}
	}

	static void CloseEvent(int id) {
		EnsureInit();
		if (s_Events.Contains(id)) {
			s_Events.Remove(id);
			M4D_PlaneCrashLogger.Debug(string.Format("[WorldState] Ficha encerrada (EventID: %1)", id));
			ForceSaveSnapshot(); // I/O Imediato em fechamentos
		}
	}

	// ---------------------------------------------------------------------------------
	// CONSULTA E AUDITORIA (Throttling Incluído)
	// ---------------------------------------------------------------------------------
	
	static int GetValidActiveCrashCount() {
		EnsureInit();
		int validCount = 0;
		for (int i = 0; i < s_Events.Count(); i++) {
			M4D_EventState state = s_Events.GetElement(i);
			if (state && state.IsConsistent) validCount++;
		}
		return validCount;
	}

	static void AuditAndClean() {
		EnsureInit();
		int total = s_Events.Count();
		int valid = 0;
		int broken = 0;
		array<int> toRemove = new array<int>();

		for (int i = 0; i < total; i++) {
			int id = s_Events.GetKey(i);
			M4D_EventState state = s_Events.GetElement(i);
			
			if (state) {
				state.Validate();
				if (state.IsConsistent) {
					valid++;
					if (state.LastProblem.Contains("WARNING")) {
						M4D_PlaneCrashLogger.Warn(string.Format("[WorldState] %1 (EventID: %2)", state.LastProblem, id));
					}
				} else {
					broken++;
					M4D_PlaneCrashLogger.Error(string.Format("[WorldState] INCONSISTÊNCIA GRAVE no EventID %1: %2", id, state.LastProblem));
					
					if (!state.WreckAlive) { toRemove.Insert(id); }
				}
			}
		}

		for (int j = 0; j < toRemove.Count(); j++) {
			s_Events.Remove(toRemove.Get(j));
			M4D_PlaneCrashLogger.Error(string.Format("[WorldState] EventID %1 expurgado da RAM para desobstrução de limite.", toRemove.Get(j)));
		}

		if (total > 0) { SafeSaveSnapshot(); }
	}

	// ---------------------------------------------------------------------------------
	// CONSULTA RÁPIDA E DEBUG
	// ---------------------------------------------------------------------------------
	
	static bool HasEvent(int id) {
		EnsureInit();
		return s_Events.Contains(id);
	}

	static M4D_EventState GetEvent(int id) {
		EnsureInit();
		return s_Events.Get(id);
	}

	static string BuildSummary() {
		EnsureInit();
		string summary = string.Format("=== M4D WORLDSTATE SUMMARY (%1 Eventos) ===\n", s_Events.Count());
		for (int i = 0; i < s_Events.Count(); i++) {
			M4D_EventState state = s_Events.GetElement(i);
			if (state) {
				string status = "SAUDÁVEL";
				if (!state.IsConsistent) status = "QUEBRADO";
				
				summary += string.Format("-> EventID: %1 | Status: %2 | Wreck: %3 | Cont: %4 | Prob: %5\n", 
					state.EventID, status, state.WreckAlive.ToString(), state.ContainerAlive.ToString(), state.LastProblem);
			}
		}
		summary += "=================================================";
		return summary;
	}

	// ---------------------------------------------------------------------------------
	// SNAPSHOT (I/O e JSON)
	// ---------------------------------------------------------------------------------

	// Salva apenas se passou mais de 1 minuto desde o último save (Proteção de I/O)
	static void SafeSaveSnapshot() {
		EnsureInit();
		if (!GetGame()) return;
		int currentMs = GetGame().GetTime();
		if (currentMs - s_LastSnapshotMs < 60000) return; // Trava térmica de 60 segundos
		
		ForceSaveSnapshot();
	}

	// Ignora a trava térmica (usado no CloseEvent)
	static void ForceSaveSnapshot() {
		EnsureInit();
		M4D_WorldStateSnapshot snap = new M4D_WorldStateSnapshot();
		snap.ActiveValidEvents = 0; snap.ActiveInconsistentEvents = 0;

		int y, mo, d, hh, mi, ss;
		GetYearMonthDay(y, mo, d); GetHourMinuteSecond(hh, mi, ss);
		snap.LastSaved = y.ToStringLen(4) + "-" + mo.ToStringLen(2) + "-" + d.ToStringLen(2) + " " + hh.ToStringLen(2) + ":" + mi.ToStringLen(2) + ":" + ss.ToStringLen(2);

		for (int i = 0; i < s_Events.Count(); i++) {
			M4D_EventState state = s_Events.GetElement(i);
			if (state) {
				snap.TrackedEvents.Insert(state);
				if (state.IsConsistent) snap.ActiveValidEvents++;
				else snap.ActiveInconsistentEvents++;
			}
		}

		JsonFileLoader<M4D_WorldStateSnapshot>.JsonSaveFile(SNAPSHOT_PATH, snap);
		if (GetGame()) s_LastSnapshotMs = GetGame().GetTime();
	}
}