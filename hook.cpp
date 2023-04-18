#include "main.h"
extern HSQAPI sq; extern HSQUIRRELVM v; 
extern PluginFuncs* VCMP;
extern CFilterScripts* m_pFilterScripts;
SQInteger FS_NewTimer(HSQUIRRELVM v)
{
	int top = sq->gettop(v);
	if (top < 4)
		return sq->throwerror(v, "Unexpected number of parameters for NewTimer [string, float, float]");
	if (sq->gettype(v, 2) != OT_STRING)
		return sq->throwerror(v, "The function name must be given as a string.");
	//Check if function exist in that filterscript.
	sq->push(v, 2);
	if (SQ_FAILED(sq->get(v, 1)))
		return sq->throwerror(v, "The given timer callback does not exist.");
	sq->poptop(v);
	HSQOBJECT* fs_table = new HSQOBJECT;
	sq->getstackobj(v, 1, fs_table);
	int SlotId=m_pFilterScripts->GetSlotId(v, fs_table);
	delete fs_table;
	if (SlotId == -1)
		return sq->throwerror(v, "Unexpected error in retrieving id of filterscript table");
	sq->pushroottable(v);
	sq->pushstring(v, "NewTimer", -1);
	if (SQ_SUCCEEDED(sq->get(v, -2)))
	{
		sq->pushroottable(v);
		sq->pushstring(v, "_FS_TimerHandle", -1);
		sq->push(v, 3); sq->push(v, 4);
		sq->pushinteger(v, SlotId);
		sq->push(v, 2);//the function name.
		for (int i = 5; i <= top; i++)
		{
			sq->push(v, i);
		}
		if (SQ_SUCCEEDED(sq->call(v, top + 2, 1, 1)))//top + this(filterscript.) + funcname
		{
			//printf("%d", sq->gettype(v, -1));
			return 1;
		}
		else return -1;
	}
	else return sq->throwerror(v, "Function NewTimer not found in roottable");;
}
SQInteger FS_onScriptLoad(HSQUIRRELVM v)
{
	//Load filterscripts here

	//Read the filterscripts
	char* szFilterScript;
	cfg filterscripts_cfg; int iScriptCount = 0;
	filterscripts_cfg.read("server.cfg", "filterscripts");
	for (int i = 0; i < filterscripts_cfg.argc; i++)
	{
		szFilterScript = filterscripts_cfg.ptr[i];
		VCMP->LogMessage("  Loading filter script '%s.nut'...", szFilterScript);
		if (m_pFilterScripts->LoadFilterScript(szFilterScript)) //File))
		{
			iScriptCount++;
		}
		else {
			VCMP->LogMessage("  Unable to load filter script '%s.nut'.", szFilterScript);
		}
	}
	filterscripts_cfg.freememory();
	VCMP->LogMessage("  Loaded %d filter scripts.\n", iScriptCount);

	return 0;
}
SQInteger FS_onScriptUnload(HSQUIRRELVM v)
{
	//Unload filterscripts
	m_pFilterScripts->UnloadFilterScripts();
	return 0;
}
SQInteger FS_onServerStart(HSQUIRRELVM v)
{
	return m_pFilterScripts->onServerStart();
}
SQInteger FS_onServerStop(HSQUIRRELVM v)
{
	return m_pFilterScripts->onServerStop();
}
SQInteger FS_onPlayerJoin(HSQUIRRELVM v)
{
	return m_pFilterScripts->onPlayerJoin();
}
SQInteger FS_onPlayerPart(HSQUIRRELVM v)
{
	return m_pFilterScripts->onPlayerPart();
}
SQInteger FS_onPlayerMove(HSQUIRRELVM v)
{
	return m_pFilterScripts->onPlayerMove();
}
SQInteger FS_onPlayerHealthChange(HSQUIRRELVM v)
{
	return m_pFilterScripts->onPlayerHealthChange();
}
SQInteger FS_onPlayerArmourChange(HSQUIRRELVM v)
{
	return m_pFilterScripts->onPlayerArmourChange();
}
SQInteger FS_onPlayerWeaponChange(HSQUIRRELVM v)
{
	return m_pFilterScripts->onPlayerWeaponChange();
}
SQInteger FS_onVehicleMove(HSQUIRRELVM v)
{
	return m_pFilterScripts->onVehicleMove();
}
SQInteger FS_onVehicleHealthChange(HSQUIRRELVM v)
{
	return m_pFilterScripts->onVehicleHealthChange();
}
SQInteger FS_onTimeChange(HSQUIRRELVM v)
{
	return m_pFilterScripts->onTimeChange();
}
SQInteger FS_onClientScriptData(HSQUIRRELVM v)
{
	return m_pFilterScripts->onClientScriptData();
}
SQInteger FS_onPlayerCommand(HSQUIRRELVM v)
{
	return m_pFilterScripts->onPlayerCommand();
}
SQInteger mm_get(HSQUIRRELVM v)
{
	const SQChar* idx;
	sq->getstring(v, 2, &idx);
	if (strcmp(idx, "onScriptLoad") == 0)
	{
		//Squirrel Game mode is looking for onScriptLoad (to call),
		//but not found
		FS_onScriptLoad(v);
	}else if (strcmp(idx, "onScriptUnload") == 0)
	{
		//Squirrel Game mode is looking for onScriptUnload (to call),
		//but not found
		FS_onScriptUnload(v);
	}
	else if (strcmp(idx, "onPlayerJoin") == 0)
	{
		char buffer[128];
		sprintf(buffer, "onPlayerJoin<-function(player){};return onPlayerJoin;");
		sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
		sq->pushroottable(v);
		sq->call(v, 1, 1, 1);
		sq->remove(v, -2);//the compiled buffer
		//the function is returned..
		return 1;
	}
	else if (strcmp(idx, "onPlayerPart") == 0)
	{
		char buffer[128];
		sprintf(buffer, "onPlayerPart<-function(player, reason){};return onPlayerPart;");
		sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
		sq->pushroottable(v);
		sq->call(v, 1, 1, 1);
		sq->remove(v, -2);//the compiled buffer
		//the function is returned..
		return 1;
	}
	else if (strcmp(idx, "onPlayerMove") == 0)
	{
		if (m_pFilterScripts->FunctionExists("onPlayerMove"))
		{
			char buffer[128];
			sprintf(buffer, "onPlayerMove<-function(player, oldX, oldY, oldZ, newX, newY, newZ){};return onPlayerMove;");
			sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
			sq->pushroottable(v);
			sq->call(v, 1, 1, 1);
			sq->remove(v, -2);//the compiled buffer
			//the function is returned..
			return 1;
		}
	}
	else if (strcmp(idx, "onPlayerHealthChange") == 0)
	{
		if (m_pFilterScripts->FunctionExists("onPlayerHealthChange"))
		{
			char buffer[128];
			sprintf(buffer, "onPlayerHealthChange<-function(player, lastHP, newHP ){};return onPlayerHealthChange;");
			sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
			sq->pushroottable(v);
			sq->call(v, 1, 1, 1);
			sq->remove(v, -2);//the compiled buffer
			//the function is returned..
			return 1;
		}
	}
	else if (strcmp(idx, "onPlayerArmourChange") == 0)
	{
		if (m_pFilterScripts->FunctionExists("onPlayerArmourChange"))
		{
			char buffer[128];
			sprintf(buffer, "onPlayerArmourChange<-function(player, lastArmour, newArmour ){};return onPlayerArmourChange;");
			sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
			sq->pushroottable(v);
			sq->call(v, 1, 1, 1);
			sq->remove(v, -2);//the compiled buffer
			//the function is returned..
			return 1;
		}
	}
	else if (strcmp(idx, "onPlayerWeaponChange") == 0)
	{
		if (m_pFilterScripts->FunctionExists("onPlayerWeaponChange"))
		{
			char buffer[128];
			sprintf(buffer, "onPlayerWeaponChange<-function(player, oldWep, newWep ){};return onPlayerWeaponChange;");
			sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
			sq->pushroottable(v);
			sq->call(v, 1, 1, 1);
			sq->remove(v, -2);//the compiled buffer
			//the function is returned..
			return 1;
		}
	}
	else if (strcmp(idx, "onServerStart") == 0)
	{
		char buffer[128];
		sprintf(buffer, "onServerStart<-function(){};return onServerStart;");
		sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
		sq->pushroottable(v);
		sq->call(v, 1, 1, 1);
		sq->remove(v, -2);//the compiled buffer
		//the function is returned..
		return 1;
	}
	else if (strcmp(idx, "onServerStop") == 0)
	{
		char buffer[128];
		sprintf(buffer, "onServerStop<-function(){};return onServerStop;");
		sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
		sq->pushroottable(v);
		sq->call(v, 1, 1, 1);
		sq->remove(v, -2);//the compiled buffer
		//the function is returned..
		return 1;
	}
	else if (strcmp(idx, "onVehicleMove") == 0)
	{
		if (m_pFilterScripts->FunctionExists("onVehicleMove"))
		{
			char buffer[128];
			sprintf(buffer, "onVehicleMove<-function(vehicle, oldX, oldY, oldZ, newX, newY, newZ){};return onVehicleMove;");
			sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
			sq->pushroottable(v);
			sq->call(v, 1, 1, 1);
			sq->remove(v, -2);//the compiled buffer
			//the function is returned..
			return 1;
		}
	}
	else if (strcmp(idx, "onVehicleHealthChange") == 0)
	{
		if (m_pFilterScripts->FunctionExists("onVehicleHealthChange"))
		{
			char buffer[128];
			sprintf(buffer, "onVehicleHealthChange<-function(vehicle, lastHP, newHP ){};return onVehicleHealthChange;");
			sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
			sq->pushroottable(v);
			sq->call(v, 1, 1, 1);
			sq->remove(v, -2);//the compiled buffer
			//the function is returned..
			return 1;
		}
	}
	else if (strcmp(idx, "onTimeChange") == 0)
	{
		if (m_pFilterScripts->FunctionExists("onTimeChange"))
		{
			char buffer[128];
			sprintf(buffer, "onTimeChange<-function(lasthour, lastminute, hour, minute ){};return onTimeChange;");
			sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
			sq->pushroottable(v);
			sq->call(v, 1, 1, 1);
			sq->remove(v, -2);//the compiled buffer
			//the function is returned..
			return 1;
		}
	}
	else if (strcmp(idx, "onClientScriptData") == 0)
	{
		if (m_pFilterScripts->FunctionExists("onClientScriptData"))
		{
			char buffer[128];
			sprintf(buffer, "onClientScriptData<-function(player){};return onClientScriptData;");
			sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
			sq->pushroottable(v);
			sq->call(v, 1, 1, 1);
			sq->remove(v, -2);//the compiled buffer
			//the function is returned..
			return 1;
		}
	}
	else if (strcmp(idx, "onPlayerCommand") == 0)
	{
	if (m_pFilterScripts->FunctionExists("onPlayerCommand"))
	{
		char buffer[128];
		sprintf(buffer, "onPlayerCommand<-function(player, cmd, text){};return onPlayerCommand;");
		sq->compilebuffer(v, buffer, strlen(buffer), "", SQTrue);
		sq->pushroottable(v);
		sq->call(v, 1, 1, 1);
		sq->remove(v, -2);//the compiled buffer
		//the function is returned..
		return 1;
	}
	}
	/*else if (strcmp(idx, "getregtbl") == 0)
	{
		sq->pushregistrytable(v);
		return 1;
	}*/
	return -1;//important
}
SQInteger mm_newslot(HSQUIRRELVM v)
{

	int top = sq->gettop(v);
	const SQChar* key;
	sq->getstring(v, 2, &key); 
	if (strcmp(key, "onScriptLoad") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onFilterScriptLoad", -1);
			sq->newclosure(v, FS_onScriptLoad, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer,"local gamemode_event=rawget(\"onScriptLoad\");\
				local filterscript_event=rawget(\"_FS_onFilterScriptLoad\");\
				onScriptLoad<-function(){\
				local ret=gamemode_event();\
				if(ret==null||ret!=0)filterscript_event();\
				};\
				rawdelete(\"_FS_onFilterScriptLoad\");\
				");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			sq->call(v, 1, 0, 1);
			sq->settop(v, top);
			return 0;
		}
		
	}else if (strcmp(key, "onScriptUnload") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onFilterScriptUnload", -1);
			sq->newclosure(v, FS_onScriptUnload, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onScriptUnload\");\
				local filterscript_event=rawget(\"_FS_onFilterScriptUnload\");\
				onScriptUnload<-function(){\
				filterscript_event();\
				gamemode_event();\
				};\
				rawdelete(\"_FS_onFilterScriptUnload\");\
				");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			sq->call(v, 1, 0, 1);
			sq->settop(v, top);
			return 0;
		}

	}
	else if (strcmp(key, "onPlayerJoin") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onPlayerJoin", -1);
			sq->newclosure(v, FS_onPlayerJoin, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onPlayerJoin\");\
				local filterscript_event=rawget(\"_FS_onPlayerJoin\");\
				onPlayerJoin<-function(player){\
				local ret=filterscript_event(player);\
				if(ret==null || ret!=0 )return gamemode_event(player);\
				};\
				rawdelete(\"_FS_onPlayerJoin\");\
				");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}

	}
	else if (strcmp(key, "onPlayerPart") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onPlayerPart", -1);
			sq->newclosure(v, FS_onPlayerPart, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onPlayerPart\");\
				local filterscript_event=rawget(\"_FS_onPlayerPart\");\
				onPlayerPart<-function(player, reason){\
				local ret=filterscript_event(player, reason);\
				if(ret==null || ret!=0 )return gamemode_event(player, reason);\
				};\
				rawdelete(\"_FS_onPlayerPart\");\
				");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onPlayerMove") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onPlayerMove", -1);
			sq->newclosure(v, FS_onPlayerMove, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onPlayerMove\");\
				local filterscript_event=rawget(\"_FS_onPlayerMove\");\
				onPlayerMove<-function(player, oldX, oldY, oldZ, newX, newY, newZ){\
				local ret=filterscript_event(player, oldX, oldY, oldZ, newX, newY, newZ);\
				if(ret==null || ret!=0 )return gamemode_event(player, oldX, oldY, oldZ, newX, newY, newZ);\
				};\
				rawdelete(\"_FS_onPlayerMove\");\
				");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onPlayerHealthChange") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onPlayerHealthChange", -1);
			sq->newclosure(v, FS_onPlayerHealthChange, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onPlayerHealthChange\");\
					local filterscript_event=rawget(\"_FS_onPlayerHealthChange\");\
					onPlayerHealthChange<-function( player, lastHP, newHP){\
					local ret=filterscript_event( player, lastHP, newHP);\
					if(ret==null || ret!=0 )return gamemode_event( player, lastHP, newHP);\
					};\
					rawdelete(\"_FS_onPlayerHealthChange\");\
					");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onPlayerArmourChange") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onPlayerArmourChange", -1);
			sq->newclosure(v, FS_onPlayerArmourChange, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onPlayerArmourChange\");\
						local filterscript_event=rawget(\"_FS_onPlayerArmourChange\");\
						onPlayerArmourChange<-function( player, lastArmour, newArmour){\
						local ret=filterscript_event(  player, lastArmour, newArmour);\
						if(ret==null || ret!=0 )return gamemode_event(  player, lastArmour, newArmour);\
						};\
						rawdelete(\"_FS_onPlayerArmourChange\");\
						");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onPlayerWeaponChange") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onPlayerWeaponChange", -1);
			sq->newclosure(v, FS_onPlayerWeaponChange, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onPlayerWeaponChange\");\
							local filterscript_event=rawget(\"_FS_onPlayerWeaponChange\");\
							onPlayerWeaponChange<-function( player, oldWep, newWep){\
							local ret=filterscript_event(  player, oldWep, newWep);\
							if(ret==null || ret!=0 )return gamemode_event(  player, oldWep, newWep);\
							};\
							rawdelete(\"_FS_onPlayerWeaponChange\");\
							");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onServerStart") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onServerStart", -1);
			sq->newclosure(v, FS_onServerStart, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onServerStart\");\
								local filterscript_event=rawget(\"_FS_onServerStart\");\
								onServerStart<-function(){\
								local ret=filterscript_event();\
								if(ret==null || ret!=0 )return gamemode_event( );\
								};\
								rawdelete(\"_FS_onServerStart\");\
								");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onServerStop") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onServerStop", -1);
			sq->newclosure(v, FS_onServerStop, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onServerStop\");\
									local filterscript_event=rawget(\"_FS_onServerStop\");\
									onServerStop<-function(){\
									local ret=filterscript_event();\
									if(ret==null || ret!=0 )return gamemode_event( );\
									};\
									rawdelete(\"_FS_onServerStop\");\
									");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onVehicleMove") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onVehicleMove", -1);
			sq->newclosure(v, FS_onVehicleMove, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onVehicleMove\");\
					local filterscript_event=rawget(\"_FS_onVehicleMove\");\
					onVehicleMove<-function(vehicle, oldX, oldY, oldZ, newX, newY, newZ){\
					local ret=filterscript_event(vehicle, oldX, oldY, oldZ, newX, newY, newZ);\
					if(ret==null || ret!=0 )return gamemode_event(vehicle, oldX, oldY, oldZ, newX, newY, newZ);\
					};\
					rawdelete(\"_FS_onVehicleMove\");\
					");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onVehicleHealthChange") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onVehicleHealthChange", -1);
			sq->newclosure(v, FS_onVehicleHealthChange, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onVehicleHealthChange\");\
						local filterscript_event=rawget(\"_FS_onVehicleHealthChange\");\
						onVehicleHealthChange<-function( vehicle, lastHP, newHP){\
						local ret=filterscript_event( vehicle, lastHP, newHP);\
						if(ret==null || ret!=0 )return gamemode_event( vehicle, lastHP, newHP);\
						};\
						rawdelete(\"_FS_onVehicleHealthChange\");\
						");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onTimeChange") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onTimeChange", -1);
			sq->newclosure(v, FS_onTimeChange, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onTimeChange\");\
							local filterscript_event=rawget(\"_FS_onTimeChange\");\
							onTimeChange<-function( lasthour, lastminute, hour, minute){\
							local ret=filterscript_event( lasthour, lastminute, hour, minute);\
							if(ret==null || ret!=0 )return gamemode_event( lasthour, lastminute, hour, minute);\
							};\
							rawdelete(\"_FS_onTimeChange\");\
							");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onClientScriptData") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onClientScriptData", -1);
			sq->newclosure(v, FS_onClientScriptData, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onClientScriptData\");\
								local filterscript_event=rawget(\"_FS_onClientScriptData\");\
								onClientScriptData<-function(player){\
								local ret=filterscript_event(player);\
								Stream.SetReadPosition(0);\
								if(ret==null || ret!=0 )return gamemode_event(player);\
								};\
								rawdelete(\"_FS_onClientScriptData\");\
								");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	else if (strcmp(key, "onPlayerCommand") == 0)
	{
		if (sq->gettype(v, 3) == OT_CLOSURE)
		{
			sq->rawset(v, 1);
			char buffer[640];
			sq->pushroottable(v);
			sq->pushstring(v, "_FS_onPlayerCommand", -1);
			sq->newclosure(v, FS_onPlayerCommand, 0);
			sq->newslot(v, -3, SQFalse);
			sprintf(buffer, "local gamemode_event=rawget(\"onPlayerCommand\");\
									local filterscript_event=rawget(\"_FS_onPlayerCommand\");\
									onPlayerCommand<-function(player,cmd,text){\
									local ret=filterscript_event(player,cmd,text);\
									if(!ret )return gamemode_event(player,cmd,text);\
									};\
									rawdelete(\"_FS_onPlayerCommand\");\
									");
			sq->compilebuffer(v, buffer, strlen(buffer), "", 1);
			sq->pushroottable(v);
			if (SQ_FAILED(sq->call(v, 1, 0, 1)))
			{
				return -1;
			}
			sq->settop(v, top);
			return 0;
		}
	}
	sq->rawset(v, 1);
	return 0;
}
SQInteger _FS_TimerHandle(HSQUIRRELVM v)
{
	int top = sq->gettop(v);
	SQInteger SlotId;
	sq->getinteger(v, 2, &SlotId);
	HSQOBJECT* table;
	if (m_pFilterScripts->GetFilterScriptTable(SlotId, table) == false)
		return sq->throwerror(v, "Error in getting filterscript table");
	sq->pushobject(v, *table);
	sq->push(v, 3);//the function name as string
	if (SQ_SUCCEEDED(sq->get(v, -2))) 
	{
		//Function exist in filterscript. Call it
		sq->pushobject(v, *table);//the filterscript table.
		for (int i = 4; i <= top; i++)
		{
			sq->push(v, i);
		}
		sq->call(v, top - 2, 1, 1);
	}
	else return sq->throwerror(v, "Function not found in filterscript");
	sq->settop(v, top);
	return 0;
}
bool HookOnScriptLoad()
{
	int top = sq->gettop(v);
	sq->pushroottable(v);
	sq->newtable(v);
	m_pFilterScripts->m_pDelegateTable = new HSQOBJECT;
	sq->getstackobj(v, -1, m_pFilterScripts->m_pDelegateTable);
	sq->pushstring(v, "_newslot", -1);
	sq->newclosure(v, mm_newslot, 0);
	sq->setparamscheck(v, 3, "ts.");
	sq->newslot(v, -3, SQFalse);

	sq->pushstring(v, "_get", -1);
	sq->newclosure(v, mm_get, 0);//mm = meta method
	sq->setparamscheck(v, 2, "ts");
	sq->newslot(v, -3, SQFalse);
	sq->setdelegate(v, -2);

	sq->pushstring(v, "_FS_TimerHandle", -1);
	sq->newclosure(v, _FS_TimerHandle, 0);
	sq->setparamscheck(v, -3, "tis");
	sq->newslot(v, -3, SQFalse);

	sq->settop(v, top);
	return true;
}
