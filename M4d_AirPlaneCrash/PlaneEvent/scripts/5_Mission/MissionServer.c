// =====================================================================================
// M4D_MissionServer.c - VERSÃO DEFINITIVA (Maestro do Evento e Integração WorldState)
// Responsabilidade: Inicialização do Mod, Gestão de Timers e Tomada de Decisão de Spawn.
// =====================================================================================

modded class MissionServer
{
	// Variáveis de controlo de tempo (em Segundos)
	protected int m_M4D_NextSpawnTime = 0;
	protected bool m_M4D_IsSystemInitialized = false;

	override void OnInit()
	{
		super.OnInit();

		// Garante que a inicialização do M4D ocorre apenas uma vez
		if (!m_M4D_IsSystemInitialized) 
		{
			M4D_PlaneCrashLogger.Info("========== INICIANDO M4D AIRPLANE CRASH SYSTEM ==========");
			
			// 1. Carrega todas as configurações e bases de dados do disco (JSONs)
			ref M4D_PlaneCrashSettings settings = M4D_PlaneCrashSettings.Get();
			M4D_PlaneCrashSites.Get();
			M4D_PlaneCrashLoot.Get();
			
			if (settings) {
				// MESTRE: SE O MOD ESTIVER DESLIGADO, ABORTA A INICIALIZAÇÃO E EXECUTA A VARREDURA!
				if (settings.EnableMod == 0) {
					M4D_PlaneCrashLogger.Warn("[MissionServer] Mod M4D Airplane Crash esta DESLIGADO no JSON (EnableMod = 0).");
					M4D_PlaneCrashLogger.Warn("[MissionServer] O agendador foi suspenso. Iniciando protocolo de Limpeza Centralizada...");
					
					// Agenda a limpeza centralizada por Ownership para 15 segundos após o boot
					// Isso garante que a Engine do DayZ já carregou os arquivos .bin da memória para o mapa.
					GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.M4D_ExecuteMasterCleanup, 15000, false);
					
					m_M4D_IsSystemInitialized = true;
					return; // Impede que o Tick de 5 segundos seja iniciado
				}

				// 2. Define o tempo para o PRIMEIRO evento pós-restart
				// CADÊNCIA INICIAL: 5s para agendar + 55s para surgir = 60 segundos do start
				m_M4D_NextSpawnTime = (GetGame().GetTime() / 1000) + 60;
				
				M4D_PlaneCrashLogger.Info("[MissionServer] Mod carregado com sucesso. Primeiro evento agendado para surgir em 60 segundos.");
			}

			// 3. Inicia o Relógio do Evento (Tick reduzido para 5 segundos para suportar a cadência com precisão)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.M4D_EventTick, 5000, true);
			
			m_M4D_IsSystemInitialized = true;
		}
	}

	// =====================================================================================
	// PROTOCOLO DE LIMPEZA CENTRALIZADA POR OWNERSHIP (Master Switch OFF)
	// =====================================================================================
	void M4D_ExecuteMasterCleanup()
	{
		if (!GetGame() || !GetGame().IsServer()) return;

		M4D_PlaneCrashLogger.Warn("[MissionServer] [Master Switch] Executando varredura centralizada cirurgica por OWNERSHIP...");

		string snapPath = "$profile:M4D_AirPlaneCrash/WorldStateSnapshot.json";
		
		if (FileExist(snapPath)) 
		{
			ref M4D_WorldStateSnapshot snap = new M4D_WorldStateSnapshot();
			string errorMessage;
			
			if (JsonFileLoader<M4D_WorldStateSnapshot>.LoadFile(snapPath, snap, errorMessage)) 
			{
				if (snap.TrackedEvents) 
				{
					for (int i = 0; i < snap.TrackedEvents.Count(); i++) 
					{
						M4D_EventState state = snap.TrackedEvents.Get(i);
						if (state) 
						{
							int targetEventID = state.EventID;
							vector pos = state.WreckPosition;
							array<Object> objs = new array<Object>();
							
							// Lança a rede de busca na região do evento (200 metros de margem segura)
							GetGame().GetObjectsAtPosition(pos, 200.0, objs, null);
							
							int deletedObjs = 0;
							for (int j = 0; j < objs.Count(); j++) 
							{
								Object obj = objs.Get(j);
								if (!obj) continue;
								
								string type = obj.GetType();
								int foundID = -1;

								// Extrai o Ownership de acordo com a classe do objeto
								if (type == "M4d_AirPlaneCrash") {
									GetGame().GameScript.CallFunction(obj, "GetEventID", foundID, null);
								} 
								else if (type.Contains("M4D_WreckContainer") || type.Contains("M4DCrashStorage")) {
									GetGame().GameScript.CallFunction(obj, "GetOwnerEventID", foundID, null);
								}
								
								// A MÁGICA DA PRECISÃO: Só deleta se o ID pertencer a este evento exato!
								if (foundID == targetEventID) 
								{
									GetGame().ObjectDelete(obj);
									deletedObjs++;
								}
								// Fallback de limpeza para lixo visual nativo (Crateras) que não suportam Ownership Scriptado
								else if (foundID == -1 && (type == "CraterLong" || type == "StaticObj_ammoboxes_big"))
								{
									if (vector.Distance(pos, obj.GetPosition()) <= 40.0) {
										GetGame().ObjectDelete(obj);
										deletedObjs++;
									}
								}
							}
							M4D_PlaneCrashLogger.Info(string.Format("[MissionServer] Varredura no EventID %1 concluída: %2 entidades deletadas.", targetEventID, deletedObjs));
						}
					}
				}
			}
			
			// Após realizar a caçada física baseada nos IDs, extermina a persistência.
			DeleteFile(snapPath);
			M4D_PlaneCrashLogger.Info("[MissionServer] [Master Switch] Arquivo de persistencia (WorldStateSnapshot) apagado com sucesso.");
		} 
		else 
		{
			M4D_PlaneCrashLogger.Info("[MissionServer] [Master Switch] Nenhum snapshot anterior encontrado. O mapa ja esta limpo.");
		}
	}

	// =====================================================================================
	// O CORAÇÃO DO AGENDADOR (Executado a cada 5 Segundos)
	// =====================================================================================
	void M4D_EventTick()
	{
		if (!GetGame() || !GetGame().IsServer()) return;

		// 1. MANUTENÇÃO DO PAINEL VIVO
		// Obriga o WorldState a limpar fantasmas, validar os ativos e gravar a auditoria no disco
		M4D_PlaneCrashWorldState.AuditAndClean();

		int currentUnixTime = GetGame().GetTime() / 1000; 
		
		// 2. VERIFICAÇÃO DE TEMPO PARA SPAWN
		if (currentUnixTime >= m_M4D_NextSpawnTime) 
		{
			ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
			if (!s) return;

			// Pergunta ao Painel APENAS pelos eventos íntegros
			int activeValidEvents = M4D_PlaneCrashWorldState.GetValidActiveCrashCount();
			
			if (activeValidEvents < s.MaxActivePlaneEvents) 
			{
				M4D_PlaneCrashLogger.Info(string.Format("[MissionServer] Autorizando Spawn. Ocupação da RAM: %1/%2", activeValidEvents, s.MaxActivePlaneEvents));
				
				// Dispara o Evento (A mágica acontece aqui)
				M4D_PlaneCrashSpawner.SpawnSite();
				
				// Calcula o tempo para o PRÓXIMO evento
				// CADÊNCIA SUBSEQUENTE: 5s para agendar + 25s para surgir = 30 segundos após o spawn anterior
				m_M4D_NextSpawnTime = currentUnixTime + 30;
				
				M4D_PlaneCrashLogger.Info("[MissionServer] Próximo evento agendado para surgir em 30 segundos.");
			} 
			else 
			{
				// MAPA CHEIO: Continua verificando a cada 5 segundos (tick) até abrir um slot
				// O timer não é empurrado para a frente e não "flooda" o log. 
				// Assim que um avião sumir e abrir vaga, o sistema dispara quase imediatamente no próximo tick.
			}
		}
	}
}