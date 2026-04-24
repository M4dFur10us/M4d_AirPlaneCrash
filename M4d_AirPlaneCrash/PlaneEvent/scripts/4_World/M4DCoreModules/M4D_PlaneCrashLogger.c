class M4D_PlaneCrashLogger
{
	static int s_LastCheckMs = 0;
	static bool s_DebugEnabled = false;
	static bool s_FileEnabled = true;
	static bool s_RPTEnabled = true;
	static bool s_CleanupDone = false;

	static string GetCleanServerName()
	{
		string raw = "DayZServer";
		if (GetGame()) {
			string sn = GetGame().GetServerName();
			if (sn != "") { 
				raw = sn; 
			}
		}

		string clean = "";
		// Lista de caracteres permitidos para nomes de arquivos no Windows/Linux
		string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-[]()";
		for (int i = 0; i < raw.Length(); i++) {
			string chr = raw.Substring(i, 1);
			if (allowed.Contains(chr)) { 
				clean = clean + chr; 
			} else { 
				clean = clean + "_"; 
			}
		}
		return clean;
	}

	static string GetLogFilePath()
	{
		string folderPath = "$profile:M4D_AirPlaneCrash/Logs";
		if (!FileExist(folderPath)) { 
			MakeDirectory(folderPath); 
		}

		int y, mo, d;
		GetYearMonthDay(y, mo, d);
		string dateStr = d.ToStringLen(2) + "-" + mo.ToStringLen(2) + "-" + y.ToStringLen(4);
		
		return folderPath + "/" + dateStr + "_" + GetCleanServerName() + ".log";
	}

	static void CleanOldLogs()
	{
		string folderPath = "$profile:M4D_AirPlaneCrash/Logs";
		if (!FileExist(folderPath)) return;
		
		int y, mo, d;
		GetYearMonthDay(y, mo, d);
		string todayStr = d.ToStringLen(2) + "-" + mo.ToStringLen(2) + "-" + y.ToStringLen(4);
		
		string fileName;
		int fileAttr;
		FindFileHandle handle = FindFile(folderPath + "/*.log", fileName, fileAttr, 0);
		
		if (handle) {
			array<string> filesToDelete = new array<string>();
			bool found = true;
			
			while (found) {
				// Se o ficheiro não contiver a data de HOJE, é marcado para ser apagado (Regra das 24h)
				if (!fileName.Contains(todayStr)) { 
					filesToDelete.Insert(folderPath + "/" + fileName); 
				}
				found = FindNextFile(handle, fileName, fileAttr);
			}
			CloseFindFile(handle);
			
			// Apaga os ficheiros marcados
			for (int i = 0; i < filesToDelete.Count(); i++) { 
				DeleteFile(filesToDelete.Get(i)); 
			}
		}
	}

	static void RefreshFlags()
	{
		// Executa a auto-limpeza apenas uma vez no arranque da missão
		if (!s_CleanupDone) { 
			CleanOldLogs(); 
			s_CleanupDone = true; 
		}

		int now = 0;
		if (GetGame()) { 
			now = GetGame().GetTime(); 
		}

		// Atualiza as configurações de log a cada 5 segundos
		if (s_LastCheckMs != 0 && now != 0) {
			if (now - s_LastCheckMs < 5000) { 
				return; 
			}
		}
		
		s_LastCheckMs = now;
		
		// Valores de segurança por omissão
		s_DebugEnabled = false; 
		s_FileEnabled = true; 
		s_RPTEnabled = true;
		
		ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
		if (!s) return;
		
		s_FileEnabled = s.EnableFileLogging;
		s_DebugEnabled = s.EnableDebugLogging;
		s_RPTEnabled = s.AlsoLogToRPT;
	}

	static void Debug(string msg)
	{
		RefreshFlags();
		if (!s_DebugEnabled) return;
		
		Write("DEBUG", msg);
		
		if (s_RPTEnabled) { 
			Print("[M4D_PlaneCrash] [DEBUG] " + msg); 
		}
	}

	static void Info(string msg)
	{
		RefreshFlags();
		
		Write("INFO", msg);
		
		if (s_RPTEnabled) { 
			Print("[M4D_PlaneCrash] [INFO] " + msg); 
		}
	}

	static void Warn(string msg)
	{
		RefreshFlags();
		
		Write("WARN", msg);
		
		if (s_RPTEnabled) { 
			Print("[M4D_PlaneCrash] [WARN] " + msg); 
		}
	}

	static void Error(string msg)
	{
		RefreshFlags();
		
		Write("ERROR", msg);
		
		if (s_RPTEnabled) { 
			Print("[M4D_PlaneCrash] [ERROR] " + msg); 
		}
	}

	protected static void Write(string level, string msg)
	{
		if (!s_FileEnabled) return;
		
		int y, mo, d, hh, mi, ss;
		GetYearMonthDay(y, mo, d); 
		GetHourMinuteSecond(hh, mi, ss);
		
		string stamp = y.ToStringLen(4) + "-" + mo.ToStringLen(2) + "-" + d.ToStringLen(2) + " " + hh.ToStringLen(2) + ":" + mi.ToStringLen(2) + ":" + ss.ToStringLen(2);
		string line = "[" + stamp + "][" + level + "] " + msg + "\n";
		
		FileHandle fh = OpenFile(GetLogFilePath(), FileMode.APPEND);
		if (fh != 0) { 
			FPrint(fh, line); 
			CloseFile(fh); 
		}
	}
}