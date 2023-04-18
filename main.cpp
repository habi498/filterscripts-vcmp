#include "main.h"
#ifdef WIN32
#define EXPORT __declspec(dllexport)
#else 
#define EXPORT
#endif
PluginFuncs* VCMP;
HSQUIRRELVM v = NULL; HSQAPI sq=NULL;
CFilterScripts* m_pFilterScripts;
SQInteger FS_NewTimer(HSQUIRRELVM v);
uint8_t FS_OnPluginCommand (uint32_t commandIdentifier, const char* message)
{
	if (commandIdentifier == 0x7D6E22D8)
	{
		int32_t id=VCMP->FindPlugin("SQHost2");
		if (id >= 0)
		{
			size_t size;
			const void** exports=VCMP->GetPluginExports(id, &size);
			if (VCMP->GetLastError() != vcmpErrorNoSuchEntity && size > 0)
			{
				SquirrelImports** _imports =(SquirrelImports**) exports;
				SquirrelImports* imports = (SquirrelImports*)(*_imports);
				if (imports)
				{
					v = *imports->GetSquirrelVM();
					sq = *imports->GetSquirrelAPI();

					if (v == NULL || sq == NULL)return 1;

					int top = sq->gettop(v);
					//Get a clone of roottable with original functions.
					sq->pushroottable(v);
					sq->clone(v, -1);
					m_pFilterScripts->m_pCloneOrgRoottable = new HSQOBJECT;
					sq->getstackobj(v, -1, m_pFilterScripts->m_pCloneOrgRoottable);
					sq->remove(v, -2);//Remove the roottable.
					
					//Now clone of roottable at -1
					sq->pushnull(v);
					sq->setdelegate(v, -2);//Pops the null and set as delegate of table at -2
					sq->pushstring(v, "getstackinfos", -1);
					sq->rawdeleteslot(v, -2, SQFalse);//Pops the string and delete the slot from table at -2

					sq->pushstring(v, "NewTimer", -1);
					sq->rawdeleteslot(v, -2, SQFalse);
					sq->pushstring(v, "NewTimer", -1);
					sq->newclosure(v, FS_NewTimer, 0);
					sq->newslot(v, -3, SQFalse);
					//Now at -1 there is clone of original roottable
					
					//Create array FilterScripts in registry table
					sq->pushregistrytable(v);
					sq->pushstring(v, "vcmp_filterscripts", -1);
					
					sq->newtable(v);
					sq->pushstring(v, "fs_roottables", -1);
					sq->newarray(v, MAX_FILTER_SCRIPTS);
					sq->newslot(v, -3, SQFalse);//pops string and array and creates a slot in the table just pushed.
					
					/*Now at - 1 there is the new table
					-2 string 'vcmp_filterscripts'
					-3 registrytable
					-4 clone of original roottable */

					sq->pushstring(v, "clone_orgnl_roottable", -1);
					sq->push(v, -5);//push the clone
					sq->newslot(v, -3, SQFalse);

					sq->newslot(v, -3, SQFalse);//pops string and table and creates a slot in registry table
					/*
					A structure of what we have created 
					-------------------------------------------------------------------------------------
					|			The Registry Table														|
					|	Slot-"vcmp_filterscripts" Type-Table											|
					|								|	Slot-"fs_roottables"	Type-array				|
					|								|	Slot-"clone_orgnl_roottable"	Type- Table		|
					-------------------------------------------------------------------------------------
					*/
					sq->settop(v, top);
					

					HookOnScriptLoad();

				}
			}
		}
	}
	else if (commandIdentifier == 0x2A1A3C4D)
	{
		char* filterscript = new char[strlen(message)+1];
		if (filterscript)
		{
			strcpy(filterscript, message);
			//Load Filterscript
			if (sq && v)m_pFilterScripts->LoadFilterScript(filterscript);
			delete[] filterscript;
		}
	}
	else if (commandIdentifier == 0x2A1A3C4E)
	{
		char* filterscript = new char[strlen(message) + 1];
		if (filterscript)
		{
			strcpy(filterscript, message);
			//Unload Filterscript
			if (sq && v)m_pFilterScripts->UnloadOneFilterScript(filterscript);
			delete[] filterscript;
		}
	}
	else if (commandIdentifier == 0x2A1A3C4F)
	{
		char* filterscript = new char[strlen(message) + 1];
		if (filterscript)
		{
			strcpy(filterscript, message);
			//Reload Filterscript
			if (sq && v)
			{
				m_pFilterScripts->UnloadOneFilterScript(filterscript);
				m_pFilterScripts->LoadFilterScript(filterscript);
			}
			delete[] filterscript;
		}
	}
	return 1;
}
void FS_OnCheckpointEntered (int32_t checkPointId, int32_t playerId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top=sq->gettop(v);
	sprintf(buffer, "return FindCheckpoint(%d)", checkPointId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnCheckpointEntered", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* checkpoint = new HSQOBJECT;
		sq->getstackobj(v, -1, checkpoint);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindPlayer(%d)", playerId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnCheckpointEntered", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* player = new HSQOBJECT;
				sq->getstackobj(v, -1, player);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					m_pFilterScripts->onCheckpointEntered(player, checkpoint);
				}
				delete player;
			}
		}
		delete checkpoint;
	}
	sq->settop(v, top);
}

void FS_OnCheckpointExited(int32_t checkPointId, int32_t playerId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top=sq->gettop(v);
	sprintf(buffer, "return FindCheckpoint(%d)", checkPointId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnCheckpointExited", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* checkpoint = new HSQOBJECT;
		sq->getstackobj(v, -1, checkpoint);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindPlayer(%d)", playerId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnCheckpointExited", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* player = new HSQOBJECT;
				sq->getstackobj(v, -1, player);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					m_pFilterScripts->onCheckpointExited(player, checkpoint);
				}
				delete player;
			}
		}
		delete checkpoint;
	}
	sq->settop(v, top);
}

void FS_OnObjectShot(int32_t objectId, int32_t playerId, int32_t weaponId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top=sq->gettop(v);
	sprintf(buffer, "return FindObject(%d)", objectId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnObjectShot", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* object = new HSQOBJECT;
		sq->getstackobj(v, -1, object);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindPlayer(%d)", playerId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnObjectShot", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* player = new HSQOBJECT;
				sq->getstackobj(v, -1, player);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					m_pFilterScripts->onObjectShot(object, player, weaponId);
				}
				delete player;
			}
		}
		delete object;
	}
	sq->settop(v, top);
}
void FS_OnObjectBump(int32_t objectId, int32_t playerId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top=sq->gettop(v);
	sprintf(buffer, "return FindObject(%d)", objectId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnObjectBump", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* object = new HSQOBJECT;
		sq->getstackobj(v, -1, object);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindPlayer(%d)", playerId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnObjectBump", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* player = new HSQOBJECT;
				sq->getstackobj(v, -1, player);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					m_pFilterScripts->onObjectBump(object, player);
				}
				delete player;
			}
		}
		delete object;
	}
	sq->settop(v, top);
}
uint8_t FS_onPickupClaimPicked(int32_t pickupId, int32_t playerId)
{
	if(v==NULL||sq==NULL)return 1; char buffer[512]; int top=sq->gettop(v); uint8_t ret = 1;
	sprintf(buffer, "return FindPickup(%d)", pickupId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_onPickupClaimPicked", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* pickup = new HSQOBJECT;
		sq->getstackobj(v, -1, pickup);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindPlayer(%d)", playerId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_onPickupClaimPicked", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* player = new HSQOBJECT;
				sq->getstackobj(v, -1, player);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					ret = (uint8_t)m_pFilterScripts->onPickupClaimPicked(player, pickup);
				}
				delete player;
			}
		}
		delete pickup;
	}
	sq->settop(v, top);
	return ret;
}
void FS_onPickupPickedUp(int32_t pickupId, int32_t playerId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top=sq->gettop(v);
	sprintf(buffer, "return FindPickup(%d)", pickupId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_onPickupPickedUp", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* pickup = new HSQOBJECT;
		sq->getstackobj(v, -1, pickup);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindPlayer(%d)", playerId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_onPickupPickedUp", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* player = new HSQOBJECT;
				sq->getstackobj(v, -1, player);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					m_pFilterScripts->onPickupPickedUp(player, pickup);
				}
				delete player;
			}
		}
		delete pickup;
	}
	sq->settop(v, top);
}
void FS_onPickupRespawn(int32_t pickupId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top=sq->gettop(v);
	sprintf(buffer, "return FindPickup(%d)", pickupId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_onPickupRespawn", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* pickup = new HSQOBJECT;
		sq->getstackobj(v, -1, pickup);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPickupRespawn(pickup);
		}
		delete pickup;
	}
	sq->settop(v, top);
}
uint8_t FS_OnPlayerRequestClass(int32_t playerId, int32_t offset)
{
	if(v==NULL||sq==NULL)return 1; char buffer[512]; int top=sq->gettop(v); uint8_t ret = 1;
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerRequestClass", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			ret=(uint8_t)m_pFilterScripts->onPlayerRequestClass(player, offset);
		}
		delete player;
	}
	sq->settop(v, top);
	return ret;
}

uint8_t FS_OnPlayerRequestSpawn(int32_t playerId)
{
	if(v==NULL||sq==NULL)return 1; char buffer[512]; int top=sq->gettop(v); uint8_t ret = 1;
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerRequestSpawn", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			ret=m_pFilterScripts->onPlayerRequestSpawn(player);
		}
		delete player;
	}
	sq->settop(v, top);
	return ret;
}
void FS_OnPlayerSpawn (int32_t playerId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top=sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerSpawn", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerSpawn(player);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerDeath (int32_t playerId, int32_t killerId, int32_t reason, vcmpBodyPart bodyPart)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top=sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerDeath", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			if (!VCMP->IsPlayerConnected(killerId))
			{
				if (reason == 43 || reason == 50)
					reason = 43; // drowned

				else if (reason == 39 && bodyPart == 7)
					reason = 39; // car crash

				else if (reason == 39 || reason == 40 || reason == 44)
					reason = 44; // fell
				
				m_pFilterScripts->onPlayerDeath(player, reason);
			}
			else
			{
				sprintf(buffer, "return FindPlayer(%d)", killerId);
				sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerDeath", 1);
				sq->pushroottable(v);
				if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
				{
					if (sq->gettype(v, -1) != OT_NULL)
					{
						HSQOBJECT* killer = new HSQOBJECT;
						sq->getstackobj(v, -1, killer);
						if (VCMP->GetPlayerTeam(playerId) == VCMP->GetPlayerTeam(killerId))
							m_pFilterScripts->onPlayerTeamKill(killer, player, reason, bodyPart);
						else 
							m_pFilterScripts->onPlayerKill(killer, player, reason, bodyPart);
						delete killer;
					}

				}
			}
		}
		delete player;
	}
	sq->settop(v, top);
}
uint8_t FS_OnPlayerChat(int32_t playerId, const char* message)
{
	if(v==NULL||sq==NULL)return 1; char buffer[512]; int top=sq->gettop(v); uint8_t ret = 1;
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerChat", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			ret=m_pFilterScripts->onPlayerChat(player, message);
		}
		delete player;
	}
	sq->settop(v, top); 
	return ret;
}
/*
uint8_t FS_OnPlayerCommand(int32_t playerId, const char* message)
{
	if(v==NULL||sq==NULL)return 1; char buffer[512]; int top=sq->gettop(v); uint8_t ret = 1;
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerCommand", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			SQChar* szText = strdup(message);
			SQChar* szSpacePos = strchr(szText, ' ');

			if (szSpacePos) {
				szSpacePos[0] = '\0';
			}

			SQChar* szArguments = szSpacePos ? &szSpacePos[1] : NULL;

			ret = m_pFilterScripts->onPlayerCommand(player, szText, szArguments);
			free(szText);
		}
		delete player;
	}
	sq->settop(v, top);
	return ret;
}*/
uint8_t FS_OnPlayerPM(int32_t playerId, int32_t targetPlayerId, const char* message)
{
	if(v==NULL||sq==NULL)return 1; char buffer[512]; int top=sq->gettop(v); uint8_t ret = 1;
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerPM", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindPlayer(%d)", targetPlayerId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerPM", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* targetplayer = new HSQOBJECT;
				sq->getstackobj(v, -1, targetplayer);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					ret = m_pFilterScripts->onPlayerPM(player, targetplayer, message);
				}
				delete targetplayer;
			}
		}
		delete player;
	}
	sq->settop(v, top);
	return ret;
}
void FS_OnPlayerBeginTyping(int32_t playerId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerBeginTyping", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerBeginTyping(player);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerEndTyping(int32_t playerId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerEndTyping", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerEndTyping(player);
		}
		delete player;
	}
	sq->settop(v, top);
}
uint8_t FS_onLoginAttempt(char* playerName, size_t nameBufferSize, const char* userPassword, const char* ipAddress)
{
	if (v == NULL || sq == NULL)return 1;
	return (uint8_t)m_pFilterScripts->onLoginAttempt(playerName, nameBufferSize, userPassword, ipAddress);
}
void FS_onKeyDown(int32_t playerId, int32_t bindId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_onKeyDown", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onKeyDown(player, bindId);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_onKeyUp(int32_t playerId, int32_t bindId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_onKeyUp", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onKeyUp(player, bindId);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerAwayChange(int32_t playerId, uint8_t isAway)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerAwayChange", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerAwayChange(player, isAway==1);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerSpectate(int32_t playerId, int32_t targetPlayerId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerSpectate", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindPlayer(%d)", targetPlayerId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerSpectate", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				if (sq->gettype(v, -1) != OT_NULL)
				{
					HSQOBJECT* target = new HSQOBJECT;
					sq->getstackobj(v, -1, target);
					m_pFilterScripts->onPlayerSpectate(player, target);
					delete target;
				}
			}
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_onPlayerCrashDump(int32_t playerId, const char* report)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerCrashDump", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerCrashDump(player, report);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerNameChange(int32_t playerId, const char* oldName, const char* newName)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerNameChange", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerNameChange(player, oldName, newName);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerActionChange(int32_t playerId, int32_t oldAction, int32_t newAction)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerActionChange", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerActionChange(player, oldAction, newAction);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerStateChange(int32_t playerId, vcmpPlayerState oldState, vcmpPlayerState newState)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerStateChange", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerStateChange(player, oldState, newState);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerOnFireChange(int32_t playerId, uint8_t isOnFire)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerOnFireChange", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerOnFireChange(player, isOnFire==1);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerCrouchChange(int32_t playerId, uint8_t isCrouching)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerCrouchChange", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerCrouchChange(player, isCrouching == 1);
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerGameKeysChange(int32_t playerId, uint32_t oldKeys, uint32_t newKeys)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerGameKeysChange", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerGameKeysChange(player, oldKeys, newKeys);
		}
		delete player;
	}
	sq->settop(v, top);
}
uint8_t FS_OnPlayerRequestEnterVehicle(int32_t playerId, int32_t vehicleId, int32_t slotIndex)
{
	if(v==NULL||sq==NULL)return 1; char buffer[512]; int top = sq->gettop(v); uint8_t ret = 1;
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerRequestEnterVehicle", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindVehicle(%d)", vehicleId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerRequestEnterVehicle", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* vehicle = new HSQOBJECT;
				sq->getstackobj(v, -1, vehicle);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					ret = m_pFilterScripts->onPlayerEnteringVehicle(player, vehicle, slotIndex);
				}
				delete vehicle;
			}
		}
		delete player;
	}
	sq->settop(v, top);
	return ret;
}
void FS_OnPlayerEnterVehicle(int32_t playerId, int32_t vehicleId, int32_t slotIndex)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerEnterVehicle", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindVehicle(%d)", vehicleId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerEnterVehicle", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* vehicle = new HSQOBJECT;
				sq->getstackobj(v, -1, vehicle);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					m_pFilterScripts->onPlayerEnterVehicle(player, vehicle, slotIndex);
				}
				delete vehicle;
			}
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnPlayerExitVehicle(int32_t playerId, int32_t vehicleId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerExitVehicle", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			sprintf(buffer, "return FindVehicle(%d)", vehicleId);
			sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerExitVehicle", 1);
			sq->pushroottable(v);
			if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
			{
				HSQOBJECT* vehicle = new HSQOBJECT;
				sq->getstackobj(v, -1, vehicle);
				if (sq->gettype(v, -1) != OT_NULL)
				{
					m_pFilterScripts->onPlayerExitVehicle(player, vehicle);
				}
				delete vehicle;
			}
		}
		delete player;
	}
	sq->settop(v, top);
}
void FS_OnVehicleExplode(int32_t vehicleId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindVehicle(%d)", vehicleId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnVehicleExplode", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* vehicle = new HSQOBJECT;
		sq->getstackobj(v, -1, vehicle);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onVehicleExplode(vehicle);
		}
		delete vehicle;
	}
	sq->settop(v, top);
}
void FS_OnVehicleRespawn(int32_t vehicleId)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindVehicle(%d)", vehicleId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnVehicleRespawn", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* vehicle = new HSQOBJECT;
		sq->getstackobj(v, -1, vehicle);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onVehicleRespawn(vehicle);
		}
		delete vehicle;
	}
	sq->settop(v, top);
}
void FS_OnPlayerModuleList(int32_t playerId, const char* list)
{
	if(v==NULL||sq==NULL)return; char buffer[512]; int top = sq->gettop(v);
	sprintf(buffer, "return FindPlayer(%d)", playerId);
	sq->compilebuffer(v, buffer, strlen(buffer), "FS_OnPlayerModuleList", 1);
	sq->pushroottable(v);
	if (SQ_SUCCEEDED(sq->call(v, 1, 1, 1)))
	{
		HSQOBJECT* player = new HSQOBJECT;
		sq->getstackobj(v, -1, player);
		if (sq->gettype(v, -1) != OT_NULL)
		{
			m_pFilterScripts->onPlayerModuleList(player, list);
		}
		delete player;
	}
	sq->settop(v, top);
}
extern "C" EXPORT unsigned int VcmpPluginInit(PluginFuncs * Funcs, PluginCallbacks * Calls, PluginInfo * Info)
{
	VCMP = Funcs;
	Info->pluginVersion = 0x1;
	memcpy(Info->name, "FilterScripts", 14);
	Info->apiMinorVersion = PLUGIN_API_MINOR;
	Info->apiMajorVersion = PLUGIN_API_MAJOR;
	Calls->OnPluginCommand = FS_OnPluginCommand;
	Calls->OnCheckpointEntered = FS_OnCheckpointEntered;
	Calls->OnCheckpointExited = FS_OnCheckpointExited;
	Calls->OnObjectShot = FS_OnObjectShot;
	Calls->OnObjectTouched = FS_OnObjectBump;
	Calls->OnPickupPickAttempt = FS_onPickupClaimPicked;
	Calls->OnPickupPicked = FS_onPickupPickedUp;
	Calls->OnPickupRespawn = FS_onPickupRespawn;
	Calls->OnPlayerRequestClass = FS_OnPlayerRequestClass;
	Calls->OnPlayerRequestSpawn = FS_OnPlayerRequestSpawn;
	Calls->OnPlayerSpawn = FS_OnPlayerSpawn;
	Calls->OnPlayerDeath = FS_OnPlayerDeath;
	Calls->OnPlayerMessage = FS_OnPlayerChat;
	//Calls->OnPlayerCommand = FS_OnPlayerCommand;
	Calls->OnPlayerPrivateMessage = FS_OnPlayerPM;
	Calls->OnPlayerBeginTyping = FS_OnPlayerBeginTyping;
	Calls->OnPlayerEndTyping = FS_OnPlayerEndTyping;
	Calls->OnIncomingConnection = FS_onLoginAttempt;
	Calls->OnPlayerKeyBindDown = FS_onKeyDown;
	Calls->OnPlayerKeyBindUp = FS_onKeyUp;
	Calls->OnPlayerAwayChange = FS_OnPlayerAwayChange;
	Calls->OnPlayerSpectate = FS_OnPlayerSpectate;
	Calls->OnPlayerCrashReport = FS_onPlayerCrashDump;
	Calls->OnPlayerNameChange = FS_OnPlayerNameChange;
	Calls->OnPlayerActionChange = FS_OnPlayerActionChange;
	Calls->OnPlayerStateChange = FS_OnPlayerStateChange;
	Calls->OnPlayerOnFireChange = FS_OnPlayerOnFireChange;
	Calls->OnPlayerCrouchChange = FS_OnPlayerCrouchChange;
	Calls->OnPlayerGameKeysChange = FS_OnPlayerGameKeysChange;
	Calls->OnPlayerRequestEnterVehicle = FS_OnPlayerRequestEnterVehicle;
	Calls->OnPlayerEnterVehicle = FS_OnPlayerEnterVehicle;
	Calls->OnPlayerExitVehicle = FS_OnPlayerExitVehicle;
	Calls->OnVehicleExplode = FS_OnVehicleExplode;
	Calls->OnVehicleRespawn = FS_OnVehicleRespawn;
	Calls->OnPlayerModuleList = FS_OnPlayerModuleList;
	m_pFilterScripts = new CFilterScripts();
	if (!m_pFilterScripts)
	{
		printf("Error when creating pointer.\n"); return 0;
	}
	return 1;
}