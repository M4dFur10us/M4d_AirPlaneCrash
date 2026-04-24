// =====================================================================================
// M4D_PlayerBase.c
// Responsabilidade: Receber os metadados de propagação do servidor e agendar a execução
// do áudio para simular a física de uma onda sonora (Etapa 3).
// =====================================================================================

modded class PlayerBase
{
	// Identificador único do RPC definido no Spawner
	static const int M4D_RPC_CRASH_SOUND = 4857392;

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		
		// Verifica se o RPC é o comando de som e se estamos no Cliente
		if (rpc_type == M4D_RPC_CRASH_SOUND && !GetGame().IsDedicatedServer()) 
		{
			// Lê os metadados enviados pelo servidor na Etapa 2
			Param2<vector, int> data;
			if (ctx.Read(data)) 
			{
				vector crashPosition = data.param1;
				int delayMs = data.param2;
				
				// ETAPA 3: O cliente agenda a execução. 
				// O som não toca imediatamente; ele espera o tempo que a onda levaria para viajar.
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.M4D_PlayDelayedWave, delayMs, false, crashPosition);
				
				// Log de debug opcional para confirmar a recepção (visível apenas no script log do cliente)
				// Print("[M4D_Sound] Onda sonora recebida. Delay: " + delayMs + "ms");
			}
		}
	}

	// Função acionada após o término do delayMs
	void M4D_PlayDelayedWave(vector pos)
	{
		// ETAPA 4: A engine do som assume o controle.
		// O volume e a atenuação serão resolvidos pelo 'PlaneCrash_SoundSet' definido no config.cpp
		SEffectManager.PlaySoundEnviroment("PlaneCrash_SoundSet", pos);
	}
}