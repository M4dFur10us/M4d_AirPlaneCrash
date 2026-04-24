// =====================================================================================
// M4D_WreckContainers.c - VERSÃO DEFINITIVA (WorldState API, CleanupRadius e Master Switch)
// Responsabilidade: Contentores do Evento, Caixas de Loot e Gestão de Ownership.
// =====================================================================================

// =====================================================================================
// FUNÇÃO GLOBAL: Identificação de Jogador para Logs
// =====================================================================================
string M4D_GetNearestPlayerLog(vector pos, float maxRadius = 10.0)
{
	string playerIDInfo = "Desconhecido";
	array<Man> players = new array<Man>();
	GetGame().GetPlayers(players);
	
	for (int pIdx = 0; pIdx < players.Count(); pIdx++) {
		Man p = players.Get(pIdx);
		if (p && p.IsAlive() && vector.Distance(pos, p.GetPosition()) <= maxRadius) {
			PlayerBase pb = PlayerBase.Cast(p);
			if (pb && pb.GetIdentity()) {
				playerIDInfo = pb.GetIdentity().GetName() + " (SteamID: " + pb.GetIdentity().GetId() + ")";
				break;
			}
		}
	}
	return playerIDInfo;
}

// =====================================================================================
// CONTENTOR VERMELHO
// =====================================================================================
class M4D_WreckContainerRed extends Land_ContainerLocked_Red_DE 
{
	protected Object m_CrashLootCrate; 
	protected bool   m_M4DIsUnlocked = false;
	protected int    m_M4DOwnerEventID = -1;
	protected int    m_M4DElapsedMs = 0;    
	protected bool   m_M4DTimerArmed = false; 

	static const int   M4D_MAX_CONTAINER_LIFETIME = 1800000; 
	static const int   M4D_CHECK_INTERVAL = 100000;  

	void SetOwnerEventID(int id) { 
		m_M4DOwnerEventID = id; 
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.MarkContainerAlive(id, this.GetType());
		}
	}
	int  GetOwnerEventID()       { return m_M4DOwnerEventID; }
	bool GetUnlockedState()      { return m_M4DIsUnlocked; }

	override void EEInit() {
		super.EEInit();
		if (GetGame().IsServer()) {
			ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
			if (s && s.EnableMod == 0) {
				GetGame().ObjectDelete(this);
				return;
			}

			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DelayedInit, 100, false);
			M4D_StartTimerIfNeeded();
		}
		if (!GetGame().IsDedicatedServer()) SpawnVisualCrate();
	}

	void DelayedInit() {
		if (GetInventory()) {
			if (!FindAttachmentBySlotName("Truck_01_WoodenCrate1")) GetInventory().CreateAttachment("M4DCrashStorage");
			if (!m_M4DIsUnlocked) GetInventory().LockInventory(LOCK_FROM_SCRIPT);
			else GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
		}
	}

	void SpawnVisualCrate() {
		m_CrashLootCrate = Object.Cast(GetGame().CreateObjectEx("StaticObj_Misc_SupplyBox2_DE", "0 0 0", ECE_LOCAL));
		if (m_CrashLootCrate) {
			AddChild(m_CrashLootCrate, -1);
			m_CrashLootCrate.SetPosition("0 -0.3 0");
			m_CrashLootCrate.SetOrientation("0 0 0");
			m_CrashLootCrate.SetFlags(EntityFlags.STATIC, false);
		}
	}

	override void EEDelete(EntityAI parent) {
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.MarkContainerDead(m_M4DOwnerEventID);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.M4D_TimerTick);
		}
		if (m_CrashLootCrate) GetGame().ObjectDelete(m_CrashLootCrate);
		super.EEDelete(parent);
	}

	override void OnStoreSave(ParamsWriteContext ctx) {
		super.OnStoreSave(ctx);
		ctx.Write(m_M4DElapsedMs); 
		ctx.Write(m_M4DTimerArmed); 
		ctx.Write(m_M4DIsUnlocked);
		ctx.Write(m_M4DOwnerEventID); 
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version) {
		if (!super.OnStoreLoad(ctx, version)) return false;
		ctx.Read(m_M4DElapsedMs); 
		ctx.Read(m_M4DTimerArmed); 
		ctx.Read(m_M4DIsUnlocked);
		if (!ctx.Read(m_M4DOwnerEventID)) m_M4DOwnerEventID = -1;
		if (GetGame().IsServer()) M4D_StartTimerIfNeeded();
		return true;
	}

	void M4D_StartTimerIfNeeded() {
		if (!m_M4DTimerArmed) {
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.M4D_TimerTick, M4D_CHECK_INTERVAL, true);
			m_M4DTimerArmed = true;
		}
	}

	protected void M4D_TimerTick() {
		if (!GetGame().IsServer()) return;
		
		ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
		if (!s) return;

		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		bool nearby = false;
		
		for (int i = 0; i < players.Count(); i++) {
			if (players.Get(i) && vector.Distance(GetPosition(), players.Get(i).GetPosition()) <= s.CleanupRadius) {
				nearby = true; break;
			}
		}
		
		if (!nearby) {
			m_M4DElapsedMs += M4D_CHECK_INTERVAL;
			if (m_M4DElapsedMs >= M4D_MAX_CONTAINER_LIFETIME) {
				GetGame().ObjectDelete(this);
			}
		}
	}

	override void OnDoorUnlocked(DoorLockParams params) {
		super.OnDoorUnlocked(params);
		int doorIdx = params.param1;
		m_LockedMask &= ~(1 << doorIdx);
		SetAnimationPhase(string.Format("side%1_lock", doorIdx + 1), 1);

		if (!m_M4DIsUnlocked) {
			m_M4DIsUnlocked = true;
			
			if (GetGame() && GetGame().IsServer()) {
				M4D_PlaneCrashWorldState.MarkContainerUnlocked(m_M4DOwnerEventID);
			}

			if (GetInventory()) GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
			
			if (!GetGame().IsDedicatedServer()) {
				SEffectManager.PlaySoundEnviroment("Land_ContainerLocked_lock_SoundSet", GetPosition());
				ref M4D_PlaneCrashSettings settings = M4D_PlaneCrashSettings.Get();
				if (!settings || settings.EnableOpeningAlarm) SEffectManager.PlaySoundEnviroment("PlaneCrash_UnlockAlarm_SoundSet", GetPosition());
			}

			if (GetGame().IsServer()) {
				M4D_PlaneCrashLogger.Info(string.Format("[EventID: %1] Contentor VERMELHO destrancado por: %2", m_M4DOwnerEventID, M4D_GetNearestPlayerLog(GetPosition())));
			}
		}
	}

	override bool CanReleaseAttachment(EntityAI attachment) { return false; }
	override bool CanReceiveAttachment(EntityAI attachment, int slotId) { return true; }
	override bool IsInventoryVisible() { return !GetInventory().IsInventoryLockedForLockType(LOCK_FROM_SCRIPT); }
}

// =====================================================================================
// CONTENTOR AZUL
// =====================================================================================
class M4D_WreckContainerBlue extends Land_ContainerLocked_Blue_DE 
{
	protected Object m_CrashLootCrate; 
	protected bool   m_M4DIsUnlocked = false;
	protected int    m_M4DOwnerEventID = -1;
	protected int    m_M4DElapsedMs = 0;    
	protected bool   m_M4DTimerArmed = false; 

	static const int   M4D_MAX_CONTAINER_LIFETIME = 1800000; 
	static const int   M4D_CHECK_INTERVAL = 100000;  

	void SetOwnerEventID(int id) { 
		m_M4DOwnerEventID = id; 
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.MarkContainerAlive(id, this.GetType());
		}
	}
	int  GetOwnerEventID()       { return m_M4DOwnerEventID; }
	bool GetUnlockedState()      { return m_M4DIsUnlocked; }

	override void EEInit() {
		super.EEInit();
		if (GetGame().IsServer()) {
			ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
			if (s && s.EnableMod == 0) {
				GetGame().ObjectDelete(this);
				return;
			}

			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DelayedInit, 100, false);
			M4D_StartTimerIfNeeded();
		}
		if (!GetGame().IsDedicatedServer()) SpawnVisualCrate();
	}

	void DelayedInit() {
		if (GetInventory()) {
			if (!FindAttachmentBySlotName("Truck_01_WoodenCrate1")) GetInventory().CreateAttachment("M4DCrashStorage");
			if (!m_M4DIsUnlocked) GetInventory().LockInventory(LOCK_FROM_SCRIPT);
			else GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
		}
	}

	void SpawnVisualCrate() {
		m_CrashLootCrate = Object.Cast(GetGame().CreateObjectEx("StaticObj_Misc_SupplyBox2_DE", "0 0 0", ECE_LOCAL));
		if (m_CrashLootCrate) {
			AddChild(m_CrashLootCrate, -1);
			m_CrashLootCrate.SetPosition("0 -0.3 0");
			m_CrashLootCrate.SetOrientation("0 0 0");
			m_CrashLootCrate.SetFlags(EntityFlags.STATIC, false);
		}
	}

	override void EEDelete(EntityAI parent) {
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.MarkContainerDead(m_M4DOwnerEventID);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.M4D_TimerTick);
		}
		if (m_CrashLootCrate) GetGame().ObjectDelete(m_CrashLootCrate);
		super.EEDelete(parent);
	}

	override void OnStoreSave(ParamsWriteContext ctx) {
		super.OnStoreSave(ctx);
		ctx.Write(m_M4DElapsedMs); 
		ctx.Write(m_M4DTimerArmed); 
		ctx.Write(m_M4DIsUnlocked);
		ctx.Write(m_M4DOwnerEventID); 
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version) {
		if (!super.OnStoreLoad(ctx, version)) return false;
		ctx.Read(m_M4DElapsedMs); 
		ctx.Read(m_M4DTimerArmed); 
		ctx.Read(m_M4DIsUnlocked);
		if (!ctx.Read(m_M4DOwnerEventID)) m_M4DOwnerEventID = -1;
		if (GetGame().IsServer()) M4D_StartTimerIfNeeded();
		return true;
	}

	void M4D_StartTimerIfNeeded() {
		if (!m_M4DTimerArmed) {
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.M4D_TimerTick, M4D_CHECK_INTERVAL, true);
			m_M4DTimerArmed = true;
		}
	}

	protected void M4D_TimerTick() {
		if (!GetGame().IsServer()) return;
		
		ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
		if (!s) return;

		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		bool nearby = false;
		
		for (int i = 0; i < players.Count(); i++) {
			if (players.Get(i) && vector.Distance(GetPosition(), players.Get(i).GetPosition()) <= s.CleanupRadius) {
				nearby = true; break;
			}
		}
		
		if (!nearby) {
			m_M4DElapsedMs += M4D_CHECK_INTERVAL;
			if (m_M4DElapsedMs >= M4D_MAX_CONTAINER_LIFETIME) {
				GetGame().ObjectDelete(this);
			}
		}
	}

	override void OnDoorUnlocked(DoorLockParams params) {
		super.OnDoorUnlocked(params);
		int doorIdx = params.param1;
		m_LockedMask &= ~(1 << doorIdx);
		SetAnimationPhase(string.Format("side%1_lock", doorIdx + 1), 1);

		if (!m_M4DIsUnlocked) {
			m_M4DIsUnlocked = true;
			
			if (GetGame() && GetGame().IsServer()) {
				M4D_PlaneCrashWorldState.MarkContainerUnlocked(m_M4DOwnerEventID);
			}

			if (GetInventory()) GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
			
			if (!GetGame().IsDedicatedServer()) {
				SEffectManager.PlaySoundEnviroment("Land_ContainerLocked_lock_SoundSet", GetPosition());
				ref M4D_PlaneCrashSettings settings = M4D_PlaneCrashSettings.Get();
				if (!settings || settings.EnableOpeningAlarm) SEffectManager.PlaySoundEnviroment("PlaneCrash_UnlockAlarm_SoundSet", GetPosition());
			}

			if (GetGame().IsServer()) {
				M4D_PlaneCrashLogger.Info(string.Format("[EventID: %1] Contentor AZUL destrancado por: %2", m_M4DOwnerEventID, M4D_GetNearestPlayerLog(GetPosition())));
			}
		}
	}

	override bool CanReleaseAttachment(EntityAI attachment) { return false; }
	override bool CanReceiveAttachment(EntityAI attachment, int slotId) { return true; }
	override bool IsInventoryVisible() { return !GetInventory().IsInventoryLockedForLockType(LOCK_FROM_SCRIPT); }
}

// =====================================================================================
// CONTENTOR AMARELO
// =====================================================================================
class M4D_WreckContainerYellow extends Land_ContainerLocked_Yellow_DE 
{
	protected Object m_CrashLootCrate; 
	protected bool   m_M4DIsUnlocked = false;
	protected int    m_M4DOwnerEventID = -1;
	protected int    m_M4DElapsedMs = 0;    
	protected bool   m_M4DTimerArmed = false; 

	static const int   M4D_MAX_CONTAINER_LIFETIME = 1800000; 
	static const int   M4D_CHECK_INTERVAL = 100000;  

	void SetOwnerEventID(int id) { 
		m_M4DOwnerEventID = id; 
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.MarkContainerAlive(id, this.GetType());
		}
	}
	int  GetOwnerEventID()       { return m_M4DOwnerEventID; }
	bool GetUnlockedState()      { return m_M4DIsUnlocked; }

	override void EEInit() {
		super.EEInit();
		if (GetGame().IsServer()) {
			ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
			if (s && s.EnableMod == 0) {
				GetGame().ObjectDelete(this);
				return;
			}

			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DelayedInit, 100, false);
			M4D_StartTimerIfNeeded();
		}
		if (!GetGame().IsDedicatedServer()) SpawnVisualCrate();
	}

	void DelayedInit() {
		if (GetInventory()) {
			if (!FindAttachmentBySlotName("Truck_01_WoodenCrate1")) GetInventory().CreateAttachment("M4DCrashStorage");
			if (!m_M4DIsUnlocked) GetInventory().LockInventory(LOCK_FROM_SCRIPT);
			else GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
		}
	}

	void SpawnVisualCrate() {
		m_CrashLootCrate = Object.Cast(GetGame().CreateObjectEx("StaticObj_Misc_SupplyBox2_DE", "0 0 0", ECE_LOCAL));
		if (m_CrashLootCrate) {
			AddChild(m_CrashLootCrate, -1);
			m_CrashLootCrate.SetPosition("0 -0.3 0");
			m_CrashLootCrate.SetOrientation("0 0 0");
			m_CrashLootCrate.SetFlags(EntityFlags.STATIC, false);
		}
	}

	override void EEDelete(EntityAI parent) {
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.MarkContainerDead(m_M4DOwnerEventID);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.M4D_TimerTick);
		}
		if (m_CrashLootCrate) GetGame().ObjectDelete(m_CrashLootCrate);
		super.EEDelete(parent);
	}

	override void OnStoreSave(ParamsWriteContext ctx) {
		super.OnStoreSave(ctx);
		ctx.Write(m_M4DElapsedMs); 
		ctx.Write(m_M4DTimerArmed); 
		ctx.Write(m_M4DIsUnlocked);
		ctx.Write(m_M4DOwnerEventID); 
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version) {
		if (!super.OnStoreLoad(ctx, version)) return false;
		ctx.Read(m_M4DElapsedMs); 
		ctx.Read(m_M4DTimerArmed); 
		ctx.Read(m_M4DIsUnlocked);
		if (!ctx.Read(m_M4DOwnerEventID)) m_M4DOwnerEventID = -1;
		if (GetGame().IsServer()) M4D_StartTimerIfNeeded();
		return true;
	}

	void M4D_StartTimerIfNeeded() {
		if (!m_M4DTimerArmed) {
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.M4D_TimerTick, M4D_CHECK_INTERVAL, true);
			m_M4DTimerArmed = true;
		}
	}

	protected void M4D_TimerTick() {
		if (!GetGame().IsServer()) return;
		
		ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
		if (!s) return;

		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		bool nearby = false;
		
		for (int i = 0; i < players.Count(); i++) {
			if (players.Get(i) && vector.Distance(GetPosition(), players.Get(i).GetPosition()) <= s.CleanupRadius) {
				nearby = true; break;
			}
		}
		
		if (!nearby) {
			m_M4DElapsedMs += M4D_CHECK_INTERVAL;
			if (m_M4DElapsedMs >= M4D_MAX_CONTAINER_LIFETIME) {
				GetGame().ObjectDelete(this);
			}
		}
	}

	override void OnDoorUnlocked(DoorLockParams params) {
		super.OnDoorUnlocked(params);
		int doorIdx = params.param1;
		m_LockedMask &= ~(1 << doorIdx);
		SetAnimationPhase(string.Format("side%1_lock", doorIdx + 1), 1);

		if (!m_M4DIsUnlocked) {
			m_M4DIsUnlocked = true;
			
			if (GetGame() && GetGame().IsServer()) {
				M4D_PlaneCrashWorldState.MarkContainerUnlocked(m_M4DOwnerEventID);
			}

			if (GetInventory()) GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
			
			if (!GetGame().IsDedicatedServer()) {
				SEffectManager.PlaySoundEnviroment("Land_ContainerLocked_lock_SoundSet", GetPosition());
				ref M4D_PlaneCrashSettings settings = M4D_PlaneCrashSettings.Get();
				if (!settings || settings.EnableOpeningAlarm) SEffectManager.PlaySoundEnviroment("PlaneCrash_UnlockAlarm_SoundSet", GetPosition());
			}

			if (GetGame().IsServer()) {
				M4D_PlaneCrashLogger.Info(string.Format("[EventID: %1] Contentor AMARELO destrancado por: %2", m_M4DOwnerEventID, M4D_GetNearestPlayerLog(GetPosition())));
			}
		}
	}

	override bool CanReleaseAttachment(EntityAI attachment) { return false; }
	override bool CanReceiveAttachment(EntityAI attachment, int slotId) { return true; }
	override bool IsInventoryVisible() { return !GetInventory().IsInventoryLockedForLockType(LOCK_FROM_SCRIPT); }
}

// =====================================================================================
// CONTENTOR LARANJA
// =====================================================================================
class M4D_WreckContainerOrange extends Land_ContainerLocked_Orange_DE 
{
	protected Object m_CrashLootCrate; 
	protected bool   m_M4DIsUnlocked = false;
	protected int    m_M4DOwnerEventID = -1;
	protected int    m_M4DElapsedMs = 0;    
	protected bool   m_M4DTimerArmed = false; 

	static const int   M4D_MAX_CONTAINER_LIFETIME = 1800000; 
	static const int   M4D_CHECK_INTERVAL = 100000;  

	void SetOwnerEventID(int id) { 
		m_M4DOwnerEventID = id; 
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.MarkContainerAlive(id, this.GetType());
		}
	}
	int  GetOwnerEventID()       { return m_M4DOwnerEventID; }
	bool GetUnlockedState()      { return m_M4DIsUnlocked; }

	override void EEInit() {
		super.EEInit();
		if (GetGame().IsServer()) {
			ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
			if (s && s.EnableMod == 0) {
				GetGame().ObjectDelete(this);
				return;
			}

			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DelayedInit, 100, false);
			M4D_StartTimerIfNeeded();
		}
		if (!GetGame().IsDedicatedServer()) SpawnVisualCrate();
	}

	void DelayedInit() {
		if (GetInventory()) {
			if (!FindAttachmentBySlotName("Truck_01_WoodenCrate1")) GetInventory().CreateAttachment("M4DCrashStorage");
			if (!m_M4DIsUnlocked) GetInventory().LockInventory(LOCK_FROM_SCRIPT);
			else GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
		}
	}

	void SpawnVisualCrate() {
		m_CrashLootCrate = Object.Cast(GetGame().CreateObjectEx("StaticObj_Misc_SupplyBox2_DE", "0 0 0", ECE_LOCAL));
		if (m_CrashLootCrate) {
			AddChild(m_CrashLootCrate, -1);
			m_CrashLootCrate.SetPosition("0 -0.3 0");
			m_CrashLootCrate.SetOrientation("0 0 0");
			m_CrashLootCrate.SetFlags(EntityFlags.STATIC, false);
		}
	}

	override void EEDelete(EntityAI parent) {
		if (GetGame() && GetGame().IsServer()) {
			M4D_PlaneCrashWorldState.MarkContainerDead(m_M4DOwnerEventID);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.M4D_TimerTick);
		}
		if (m_CrashLootCrate) GetGame().ObjectDelete(m_CrashLootCrate);
		super.EEDelete(parent);
	}

	override void OnStoreSave(ParamsWriteContext ctx) {
		super.OnStoreSave(ctx);
		ctx.Write(m_M4DElapsedMs); 
		ctx.Write(m_M4DTimerArmed); 
		ctx.Write(m_M4DIsUnlocked);
		ctx.Write(m_M4DOwnerEventID); 
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version) {
		if (!super.OnStoreLoad(ctx, version)) return false;
		ctx.Read(m_M4DElapsedMs); 
		ctx.Read(m_M4DTimerArmed); 
		ctx.Read(m_M4DIsUnlocked);
		if (!ctx.Read(m_M4DOwnerEventID)) m_M4DOwnerEventID = -1;
		if (GetGame().IsServer()) M4D_StartTimerIfNeeded();
		return true;
	}

	void M4D_StartTimerIfNeeded() {
		if (!m_M4DTimerArmed) {
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.M4D_TimerTick, M4D_CHECK_INTERVAL, true);
			m_M4DTimerArmed = true;
		}
	}

	protected void M4D_TimerTick() {
		if (!GetGame().IsServer()) return;
		
		ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
		if (!s) return;

		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		bool nearby = false;
		
		for (int i = 0; i < players.Count(); i++) {
			if (players.Get(i) && vector.Distance(GetPosition(), players.Get(i).GetPosition()) <= s.CleanupRadius) {
				nearby = true; break;
			}
		}
		
		if (!nearby) {
			m_M4DElapsedMs += M4D_CHECK_INTERVAL;
			if (m_M4DElapsedMs >= M4D_MAX_CONTAINER_LIFETIME) {
				GetGame().ObjectDelete(this);
			}
		}
	}

	override void OnDoorUnlocked(DoorLockParams params) {
		super.OnDoorUnlocked(params);
		int doorIdx = params.param1;
		m_LockedMask &= ~(1 << doorIdx);
		SetAnimationPhase(string.Format("side%1_lock", doorIdx + 1), 1);

		if (!m_M4DIsUnlocked) {
			m_M4DIsUnlocked = true;
			
			if (GetGame() && GetGame().IsServer()) {
				M4D_PlaneCrashWorldState.MarkContainerUnlocked(m_M4DOwnerEventID);
			}

			if (GetInventory()) GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
			
			if (!GetGame().IsDedicatedServer()) {
				SEffectManager.PlaySoundEnviroment("Land_ContainerLocked_lock_SoundSet", GetPosition());
				ref M4D_PlaneCrashSettings settings = M4D_PlaneCrashSettings.Get();
				if (!settings || settings.EnableOpeningAlarm) SEffectManager.PlaySoundEnviroment("PlaneCrash_UnlockAlarm_SoundSet", GetPosition());
			}

			if (GetGame().IsServer()) {
				M4D_PlaneCrashLogger.Info(string.Format("[EventID: %1] Contentor LARANJA destrancado por: %2", m_M4DOwnerEventID, M4D_GetNearestPlayerLog(GetPosition())));
			}
		}
	}

	override bool CanReleaseAttachment(EntityAI attachment) { return false; }
	override bool CanReceiveAttachment(EntityAI attachment, int slotId) { return true; }
	override bool IsInventoryVisible() { return !GetInventory().IsInventoryLockedForLockType(LOCK_FROM_SCRIPT); }
}

// =====================================================================================
// CLASSES DE STORAGE (Loot, Persistência via SeaChest e Bypass de Sistema)
// =====================================================================================
class M4DCrashStorage extends SeaChest 
{
	protected int m_M4DOwnerEventID = -1;
	protected bool m_M4D_IsSystemSpawningLoot = false;
	
	void SetOwnerEventID(int id) { m_M4DOwnerEventID = id; }
	int  GetOwnerEventID()       { return m_M4DOwnerEventID; }

	// Método invocado pelo LootManager para destrancar a permissão de inserção de itens
	void SetSystemSpawningMode(bool state) { m_M4D_IsSystemSpawningLoot = state; }

	// BLOQUEIO: Impede que os jogadores coloquem itens. Liberta apenas quando o sistema gera o loot.
	override bool CanReceiveItemIntoCargo(EntityAI item) { return m_M4D_IsSystemSpawningLoot; }
	
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }

	override void OnStoreSave(ParamsWriteContext ctx) {
		super.OnStoreSave(ctx);
		ctx.Write(m_M4DOwnerEventID);
	}
	override bool OnStoreLoad(ParamsReadContext ctx, int version) { 
		if (!super.OnStoreLoad(ctx, version)) return false;
		if (!ctx.Read(m_M4DOwnerEventID)) m_M4DOwnerEventID = -1;
		return true;
	}
}

class M4DCrashStorage_AmmoBox extends M4DCrashStorage {}
class M4DCrashStorage_ToolBox extends M4DCrashStorage_AmmoBox {}
class M4D_BoatCrashLoot extends M4DCrashStorage {}
class M4D_IndustrialCrashLoot extends M4DCrashStorage {}