// =====================================================================================
// M4D_PlaneCrashLootManager.c - VERSÃO FINAL CORRIGIDA (Bug do Slot Resolvido)
// Responsabilidade: Gestão e distribuição de loot baseado em probabilidades e Tiers.
// =====================================================================================

class M4D_PlaneCrashLootManager
{
	// =====================================================================================
	// CONTENTOR PRINCIPAL (Injeta no Anexo)
	// =====================================================================================
	static void FillContainerWithRandomLoot(EntityAI container, int tier) 
	{
		if (!container || !container.GetInventory()) return;
		
		// RECUPERAÇÃO DO ANEXO (CORRIGIDO): Converte a string para Slot ID nativo da engine
		int slotId = InventorySlots.GetSlotIdFromString("Truck_01_WoodenCrate1");
		EntityAI crate = container.GetInventory().FindAttachment(slotId);

		if (!crate) return; // Segurança: Se não achou a caixa anexada, aborta

		ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
		ref M4D_PlaneCrashLoot l = M4D_PlaneCrashLoot.Get();
		if (!s || !l) return;
		
		ref array<ref M4D_PlaneCrashLootEntry> activeLoot = l.DefaultLootItems;
		
		if (s.EnableDefaultLootItems == 0) {
			if (tier == 1) activeLoot = l.Tier1_MainLoot;
			else if (tier == 2) activeLoot = l.Tier2_MainLoot;
			else if (tier == 3) activeLoot = l.Tier3_MainLoot;
		}

		ProcessLootSpawning(crate, activeLoot, s.MinLootItems, s.MaxLootItems);
	}

	// =====================================================================================
	// CAIXAS DE MUNIÇÃO (AMMOBOXES STANDALONE)
	// =====================================================================================
	static void FillAmmoBoxLoot(EntityAI box, int tier) 
	{
		if (!box) return;
		ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
		ref M4D_PlaneCrashLoot l = M4D_PlaneCrashLoot.Get();
		if (!s || !l) return;

		ref array<ref M4D_PlaneCrashLootEntry> activeLoot = l.DefaultAmmoBoxLoot;
		
		if (s.EnableDefaultAmmoBoxLoot == 0) {
			if (tier == 1) activeLoot = l.Tier1_AmmoBoxLoot;
			else if (tier == 2) activeLoot = l.Tier2_AmmoBoxLoot;
			else if (tier == 3) activeLoot = l.Tier3_AmmoBoxLoot;
		}

		ProcessLootSpawning(box, activeLoot, s.MinAmmoBoxItems, s.MaxAmmoBoxItems);
	}

	// =====================================================================================
	// CAIXAS DE FERRAMENTAS (TOOLBOXES STANDALONE)
	// =====================================================================================
	static void FillToolBoxLoot(EntityAI box, int tier) 
	{
		if (!box) return;
		ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
		ref M4D_PlaneCrashLoot l = M4D_PlaneCrashLoot.Get();
		if (!s || !l) return;

		ref array<ref M4D_PlaneCrashLootEntry> activeLoot = l.DefaultToolBoxLoot;
		
		if (s.EnableDefaultToolBoxLoot == 0) {
			if (tier == 1) activeLoot = l.Tier1_ToolBoxLoot;
			else if (tier == 2) activeLoot = l.Tier2_ToolBoxLoot;
			else if (tier == 3) activeLoot = l.Tier3_ToolBoxLoot;
		}

		ProcessLootSpawning(box, activeLoot, s.MinToolBoxItems, s.MaxToolBoxItems);
	}

	// =====================================================================================
	// MOTOR MATEMÁTICO DE SPAWN (Core do Loot com Bypass Integrado)
	// =====================================================================================
	protected static void ProcessLootSpawning(EntityAI target, array<ref M4D_PlaneCrashLootEntry> lootPool, int minItems, int maxItems)
	{
		if (!target || !target.GetInventory() || !lootPool || lootPool.Count() == 0) return;
		
		// 1. LIGA O BYPASS: Faz o Cast para a classe PAI (M4DCrashStorage). 
		// Isto funciona universalmente para a caixa principal e para todas as AmmoBoxes.
		M4DCrashStorage customBox = M4DCrashStorage.Cast(target);
		if (customBox) {
			customBox.SetSystemSpawningMode(true);
		}

		ref array<ref M4D_PlaneCrashLootEntry> tempPool = new array<ref M4D_PlaneCrashLootEntry>();
		for (int a = 0; a < lootPool.Count(); a++) { tempPool.Insert(lootPool.Get(a)); }
		
		// Embaralhamento (Shuffle)
		for (int i = 0; i < tempPool.Count(); i++) {
			int j = Math.RandomInt(i, tempPool.Count());
			M4D_PlaneCrashLootEntry t = tempPool[i]; 
			tempPool[i] = tempPool[j]; 
			tempPool[j] = t;
		}

		int spawnCount = Math.RandomIntInclusive(minItems, maxItems);
		array<ref M4D_PlaneCrashLootEntry> selected = new array<ref M4D_PlaneCrashLootEntry>();
		
		for (int b = 0; b < tempPool.Count(); b++) {
			M4D_PlaneCrashLootEntry ent = tempPool.Get(b);
			if (selected.Count() >= spawnCount) break;
			if (Math.RandomFloat(0.0, 1.0) <= ent.Chance) { selected.Insert(ent); }
		}

		bool forcePristine = false;
		ref M4D_PlaneCrashSettings st = M4D_PlaneCrashSettings.Get();
		if (st && st.PristineLoot == 1) forcePristine = true;
		
		for (int c = 0; c < selected.Count(); c++) 
		{
			M4D_PlaneCrashLootEntry finalLoot = selected.Get(c);
			EntityAI item = target.GetInventory().CreateInInventory(finalLoot.Item);
			if (!item) continue;
			
			float hp = Math.RandomFloat(0.4, 1.0);
			if (forcePristine) hp = 1.0;
			item.SetHealth01("", "", hp);

			if (item.GetInventory()) {
				for (int d = 0; d < finalLoot.Attachments.Count(); d++) {
					string attName = finalLoot.Attachments.Get(d);
					EntityAI att = item.GetInventory().CreateAttachment(attName);
					if (!att) att = item.GetInventory().CreateInInventory(attName);
					if (att) att.SetHealth01("", "", hp);
				}
				for (int e = 0; e < finalLoot.CargoItems.Count(); e++) {
					string cargoName = finalLoot.CargoItems.Get(e);
					EntityAI cargoItem = item.GetInventory().CreateInInventory(cargoName);
					if (!cargoItem) cargoItem = item.GetInventory().CreateEntityInCargo(cargoName);
					if (cargoItem) cargoItem.SetHealth01("", "", hp);
				}
			}
		}

		// 2. DESLIGA O BYPASS: Tranca a caixa novamente contra os jogadores
		if (customBox) {
			customBox.SetSystemSpawningMode(false);
		}
	}
}