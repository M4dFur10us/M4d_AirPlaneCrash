// =====================================================================================
// M4D_AirPlaneCrash_Core.c - VERSÃO DEFINITIVA (Reidratação Simétrica do WorldState)
// Responsabilidade: Entidade âncora, Estado de Runtime, Snapshot Persistente e Ownership.
// =====================================================================================

class M4d_AirPlaneCrash extends CrashBase
{
	// ==========================================================
	// 1. SNAPSHOT MÍNIMO (Persistido no .bin)
	// ==========================================================
	protected int  m_M4DEventID = 0;       
	protected int  m_CrashTier = 0;        
	protected bool m_M4DHasGas = false;    
	protected int  m_M4DElapsedMs = 0;     
	protected bool m_M4DKeyDropped = false; 
	
	// Flags persistidas para Reidratação de Roamers
	protected bool m_HadWolves = false;
	protected bool m_HadBears = false;

	// ==========================================================
	// 2. CONTEXTO OPERACIONAL (Persistido via serialização de array)
	// ==========================================================
	protected ref array<string> m_ZombieTypes;
	protected ref PlaneCrashAnimalSpawn m_WolfSpawn;
	protected ref PlaneCrashAnimalSpawn m_BearSpawn;

	// ==========================================================
	// 3. ARRAYS DE TRACKING (Reconstruídos no Restore / RAM)
	// ==========================================================
	protected ref array<EntityAI> m_DetailAssets;
	protected ref array<DayZInfected> m_Zombies;
	protected ref array<AnimalBase> m_Animals;
	protected ref array<EffectArea> m_GasAreas;
	protected EntityAI m_MainContainer;

	// Controle de Pausa e Timers
	protected bool m_M4DTimerArmed = false;
	protected bool m_M4DIsTimerPaused = false;
	protected int  m_M4DPauseStartTime = 0;

	// Visuais do Cliente
	protected Particle m_PlaneCrash_FireEfx;

	static const int M4D_MAX_CONTAINER_LIFETIME = 1800000; // 30 Minutos
	static const int M4D_CHECK_INTERVAL         = 100000;  // 100 Segundos

	void M4d_AirPlaneCrash()
	{
		m_DetailAssets = new array<EntityAI>();
		m_Zombies = new array<DayZInfected>();
		m_Animals = new array<AnimalBase>();
		m_GasAreas = new array<EffectArea>();
		
		if (GetGame() && GetGame().IsServer()) {
			m_M4DEventID = Math.RandomIntInclusive(10000000, 99999999);
		}
	}

	// ==========================================================
	// 4. API DE ENCAPSULAMENTO E OWNERSHIP
	// ==========================================================
	int GetEventID() { return m_M4DEventID; }
	int GetOwnerEventID() { return m_M4DEventID; }

	int GetDetailAssetsCount() { return m_DetailAssets.Count(); }
	int GetZombiesCount() { return m_Zombies.Count(); }
	int GetAnimalsCount() { return m_Animals.Count(); }
	int GetGasAreasCount() { return m_GasAreas.Count(); }

	bool HasKeyDropped() { return m_M4DKeyDropped; }
	void SetKeyDropped() { m_M4DKeyDropped = true; }

	void SetupEventState(int tier, bool hasGas) {
		m_CrashTier = tier;
		m_M4DHasGas = hasGas;
		
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.MarkWreckAlive(m_M4DEventID, GetPosition(), m_CrashTier, m_M4DHasGas);
		}
	}

	void SetupThreatsData(array<string> zTypes, PlaneCrashAnimalSpawn wolf, PlaneCrashAnimalSpawn bear) 
	{
		if (zTypes) {
			m_ZombieTypes = new array<string>();
			m_ZombieTypes.Copy(zTypes);
		}
		
		if (wolf && wolf.Enabled == 1) m_HadWolves = true;
		if (bear && bear.Enabled == 1) m_HadBears = true;
		
		m_WolfSpawn = wolf;
		m_BearSpawn = bear;
	}

	array<string> GetZombieTypes() { return m_ZombieTypes; }
	PlaneCrashAnimalSpawn GetWolfSpawn() { return m_WolfSpawn; }
	PlaneCrashAnimalSpawn GetBearSpawn() { return m_BearSpawn; }

	void SetMainContainer(EntityAI container) { m_MainContainer = container; }
	void AddDetailAsset(EntityAI asset) { if(asset) m_DetailAssets.Insert(asset); }
	void AddZombie(DayZInfected z) { if(z) m_Zombies.Insert(z); }
	void AddAnimal(AnimalBase a) { if(a) m_Animals.Insert(a); }
	void AddGasArea(EffectArea gas) { if(gas) m_GasAreas.Insert(gas); }

	// ---------------------------------------------------------------------------------
	// INICIALIZAÇÃO
	// ---------------------------------------------------------------------------------
	override void EEInit()
	{
		super.EEInit();
		if (GetGame() && GetGame().IsServer()) {
			if (!m_M4DTimerArmed) { 
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.M4D_TimerTick, M4D_CHECK_INTERVAL, true); 
				m_M4DTimerArmed = true; 
			}
			M4D_PlaneCrashLogger.Info(string.Format("[EventID: %1] Wreck inicializado.", m_M4DEventID));
		}
		
		if (GetGame() && !GetGame().IsDedicatedServer()) {
			m_ParticleEfx = Particle.PlayOnObject(ParticleList.SMOKING_HELI_WRECK, this, Vector(4.7, -2.4, -2));
			m_PlaneCrash_FireEfx = Particle.PlayOnObject(ParticleList.BONFIRE_FIRE, this, Vector(4.7, -2.4, -2));
		}
	}

	// ---------------------------------------------------------------------------------
	// PERSISTÊNCIA AVANÇADA (Serialização do Snapshot)
	// ---------------------------------------------------------------------------------
	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(m_M4DEventID);
		ctx.Write(m_CrashTier); 
		ctx.Write(m_M4DHasGas);
		ctx.Write(m_M4DElapsedMs); 
		ctx.Write(m_M4DKeyDropped); 
		
		ctx.Write(m_HadWolves);
		ctx.Write(m_HadBears);
		
		if (m_ZombieTypes) {
			ctx.Write(m_ZombieTypes.Count());
			for (int i = 0; i < m_ZombieTypes.Count(); i++) {
				ctx.Write(m_ZombieTypes.Get(i));
			}
		} else {
			ctx.Write(0); 
		}
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version)) return false;
		
		if (!ctx.Read(m_M4DEventID) || m_M4DEventID == 0) m_M4DEventID = Math.RandomIntInclusive(10000000, 99999999);
		if (!ctx.Read(m_CrashTier)) m_CrashTier = 0;
		if (!ctx.Read(m_M4DHasGas)) m_M4DHasGas = false;
		if (!ctx.Read(m_M4DElapsedMs)) m_M4DElapsedMs = 0;
		if (!ctx.Read(m_M4DKeyDropped)) m_M4DKeyDropped = false; 
		
		if (!ctx.Read(m_HadWolves)) m_HadWolves = false;
		if (!ctx.Read(m_HadBears)) m_HadBears = false;

		int zCount = 0;
		if (ctx.Read(zCount) && zCount > 0) {
			m_ZombieTypes = new array<string>();
			for (int j = 0; j < zCount; j++) {
				string zType;
				if (ctx.Read(zType)) m_ZombieTypes.Insert(zType);
			}
		}

		#ifdef SERVER
			if (GetGame()) {
				M4D_PlaneCrashWorldState.MarkWreckAlive(m_M4DEventID, GetPosition(), m_CrashTier, m_M4DHasGas);

				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RehydrateEvent, 1000, false);
				if (!m_M4DTimerArmed) { 
					GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.M4D_TimerTick, M4D_CHECK_INTERVAL, true); 
					m_M4DTimerArmed = true; 
				}
			}
		#endif
		return true;
	}

	// ---------------------------------------------------------------------------------
	// REIDRATAÇÃO E INVERSÃO DE CONTROLE
	// ---------------------------------------------------------------------------------
	protected bool IsEventZombieType(string type)
	{
		if (type.Contains("ZmbM_NBC_")) return true;
		if (type.Contains("ZmbM_PatrolNormal_")) return true;
		if (type.Contains("ZmbM_SoldierNormal")) return true;
		if (type.Contains("ZmbM_usSoldier_")) return true;
		if (type.Contains("ZmbM_eastSoldier_")) return true;
		return false;
	}

	protected void RehydrateEvent()
	{
		if (!GetGame() || !GetGame().IsServer()) return;

		m_DetailAssets.Clear();
		m_Zombies.Clear();
		m_Animals.Clear();
		m_GasAreas.Clear();
		m_MainContainer = null;

		array<Object> objs = new array<Object>();
		GetGame().GetObjectsAtPosition(GetPosition(), 120.0, objs, null);

		for (int i = 0; i < objs.Count(); i++) 
		{
			Object obj = objs.Get(i);
			if (!obj) continue;
			
			string type = obj.GetType();

			if (type.Contains("M4D_WreckContainer") || type.Contains("M4DCrashStorage")) {
				int objID = -1;
				GetGame().GameScript.CallFunction(obj, "GetOwnerEventID", objID, null);
				
				if (objID == m_M4DEventID) {
					if (type.Contains("M4D_WreckContainer")) {
						m_MainContainer = EntityAI.Cast(obj);
					} else {
						m_DetailAssets.Insert(EntityAI.Cast(obj));
					}
				}
				continue;
			}

			if (type == "CraterLong" || type == "StaticObj_ammoboxes_big") {
				if (vector.Distance(GetPosition(), obj.GetPosition()) <= 35.0) {
					m_DetailAssets.Insert(EntityAI.Cast(obj));
				}
				continue;
			}

			if (DayZInfected.Cast(obj)) {
				if (vector.Distance(GetPosition(), obj.GetPosition()) <= 45.0 && IsEventZombieType(type)) {
					m_Zombies.Insert(DayZInfected.Cast(obj));
				}
				continue;
			}

			if (AnimalBase.Cast(obj)) {
				string aType = type; aType.ToLower();
				if (m_HadWolves && aType.Contains("wolf")) {
					m_Animals.Insert(AnimalBase.Cast(obj));
				} else if (m_HadBears && aType.Contains("bear")) {
					m_Animals.Insert(AnimalBase.Cast(obj));
				}
				continue;
			}

			if (type == "ContaminatedArea_Static" || type == "ContaminatedArea_Dynamic") {
				if (vector.Distance(GetPosition(), obj.GetPosition()) <= 5.0) {
					m_GasAreas.Insert(EffectArea.Cast(obj));
				}
				continue;
			}
		}

		// MASTER SWITCH CHECK: Se o mod estiver desligado, o avião se auto-deleta e limpa o mapa
		ref M4D_PlaneCrashSettings settings = M4D_PlaneCrashSettings.Get();
		if (settings && settings.EnableMod == 0) {
			M4D_PlaneCrashLogger.Warn(string.Format("[EventID: %1] Mod desligado no JSON. Removendo evento do mapa.", m_M4DEventID));
			GetGame().ObjectDelete(this);
			return;
		}

		if (m_MainContainer) {
			// INTEGRAÇÃO WORLDSTATE (Restart): Informa que o contentor foi encontrado na RAM
			M4D_PlaneCrashWorldState.MarkContainerAlive(m_M4DEventID, m_MainContainer.GetType());

			bool isUnlocked = false;
			GetGame().GameScript.CallFunction(m_MainContainer, "GetUnlockedState", isUnlocked, null);
			
			if (isUnlocked) {
				// INTEGRAÇÃO WORLDSTATE (Restart): Restaura o status de destrancado caso os jogadores já tenham aberto antes do wipe
				M4D_PlaneCrashWorldState.MarkContainerUnlocked(m_M4DEventID);
			} else {
				string key = "ShippingContainerKeys_Blue";
				string cType = m_MainContainer.GetType();
				if (cType.Contains("Red")) key = "ShippingContainerKeys_Red";
				else if (cType.Contains("Yellow")) key = "ShippingContainerKeys_Yellow";
				else if (cType.Contains("Orange")) key = "ShippingContainerKeys_Orange";

				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(M4D_PlaneCrashThreats.M4D_SpawnZombiesWithKey, 2000, false, this, m_MainContainer.GetPosition(), key);
			}
		}

		vector wMat[3];
		M4D_PlaneCrashSpawner.BuildWreckMatrix(GetPosition(), GetDirection(), wMat);
		M4D_PlaneCrashSpawner.SpawnDetailAssets(this, wMat, m_CrashTier, true); 

		M4D_PlaneCrashWorldState.UpdateCounts(m_M4DEventID, m_DetailAssets.Count(), m_Zombies.Count(), m_Animals.Count(), m_GasAreas.Count());
		M4D_PlaneCrashWorldState.MarkRestored(m_M4DEventID);
	}

	// ---------------------------------------------------------------------------------
	// CLEANUP E TIMERS
	// ---------------------------------------------------------------------------------
	override void EEDelete(EntityAI parent)
	{
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.CloseEvent(m_M4DEventID);
			
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.M4D_TimerTick);
			
			if (m_MainContainer) { 
				int cID = -1; GetGame().GameScript.CallFunction(m_MainContainer, "GetOwnerEventID", cID, null);
				if (cID == m_M4DEventID || cID == -1) GetGame().ObjectDelete(m_MainContainer);
			}
			
			foreach (EntityAI asset : m_DetailAssets) {
				if (asset) { 
					int aID = -1; GetGame().GameScript.CallFunction(asset, "GetOwnerEventID", aID, null);
					if (aID == m_M4DEventID || aID == -1) GetGame().ObjectDelete(asset);
				}
			}
			
			foreach (DayZInfected z : m_Zombies) { if (z) GetGame().ObjectDelete(z); }
			foreach (AnimalBase a : m_Animals) { if (a) GetGame().ObjectDelete(a); }
			foreach (EffectArea gas : m_GasAreas) { if (gas) GetGame().ObjectDelete(gas); }

			m_DetailAssets.Clear(); m_Zombies.Clear(); m_Animals.Clear(); m_GasAreas.Clear(); m_MainContainer = null;
		}
		
		if (GetGame() && !GetGame().IsDedicatedServer()) {
			if (m_ParticleEfx) m_ParticleEfx.Stop();
			if (m_PlaneCrash_FireEfx) m_PlaneCrash_FireEfx.Stop();
		}
		
		super.EEDelete(parent);
	}

	protected void M4D_TimerTick() {
		if (!GetGame() || !GetGame().IsServer()) return;

		M4D_PlaneCrashWorldState.Heartbeat(m_M4DEventID);

		ref M4D_PlaneCrashSettings settings = M4D_PlaneCrashSettings.Get();
		if (!settings) return;

		vector pos = GetPosition();
		array<Man> players = new array<Man>(); GetGame().GetPlayers(players);
		bool anyPlayerNearby = false;

		for (int i = 0; i < players.Count(); i++) {
			Man p = players.Get(i);
			if (p && vector.Distance(pos, p.GetPosition()) <= settings.CleanupRadius) { anyPlayerNearby = true; break; }
		}

		if (anyPlayerNearby) {
			if (!m_M4DIsTimerPaused) {
				m_M4DIsTimerPaused = true; m_M4DPauseStartTime = GetGame().GetTime() / 1000;
			}
		} else {
			if (m_M4DIsTimerPaused) m_M4DIsTimerPaused = false;
			
			m_M4DElapsedMs += M4D_CHECK_INTERVAL;
			if (m_M4DElapsedMs >= M4D_MAX_CONTAINER_LIFETIME) {
				GetGame().ObjectDelete(this);
			}
		}
	}

	static void M4D_UpdateNavmesh(Object obj) { if (obj && GetGame() && GetGame().IsServer()) GetGame().UpdatePathgraphRegionByObject(obj); }
}