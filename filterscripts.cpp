/*	Copyright 2022-2023 habi

	Copyright 2004 - 2005 SA:MP Team

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "Main.h"
extern HSQAPI sq; extern HSQUIRRELVM v; extern PluginFuncs* VCMP;

CFilterScripts::CFilterScripts()
{
	m_iFilterScriptCount = 0;
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
		m_pFilterScripts[i] = NULL;
}
CFilterScripts::~CFilterScripts()
{
	UnloadFilterScripts();
}
bool CFilterScripts::LoadFilterScript(char* pFileName)
{
	char szFilterScriptFile[255];
	sprintf(szFilterScriptFile, "filterscripts/%s.nut", pFileName);
	if (m_iFilterScriptCount >= MAX_FILTER_SCRIPTS)
		return false;
	FILE* f = fopen(&szFilterScriptFile[0], "rb");
	if (!f) return false;
	fclose(f);
	// Find a spare slot to load the script into
	int iSlot;
	for (iSlot = 0; iSlot < MAX_FILTER_SCRIPTS; iSlot++)
	{
		if (m_pFilterScripts[iSlot] == NULL) break;
		if (strcmp(pFileName, m_szFilterScriptName[iSlot]) == 0) return false;
	}
	if (iSlot == MAX_FILTER_SCRIPTS) return false;
	
	int top = sq->gettop(v);
	sq->pushroottable(v);
	sq->pushobject(v, *m_pCloneOrgRoottable);
	sq->clone(v, -1);//Clones the clone of roottable.
	sq->remove(v, -2);//Remove the pushed object.
	
	//Now at -1, there is the copy of clone of original roottable
	m_pFilterScripts[iSlot] = new HSQOBJECT;
	sq->getstackobj(v, -1, m_pFilterScripts[iSlot]);
	
	//Store the copy of clone in the dungeons of registrytable
	int top2 = sq->gettop(v);
	sq->pushregistrytable(v);
	sq->pushstring(v, "vcmp_filterscripts", -1);
	if (SQ_SUCCEEDED(sq->get(v, -2)))
	{
		sq->pushstring(v, "fs_roottables", -1);
		if (SQ_SUCCEEDED(sq->get(v, -2)))
		{
			sq->pushinteger(v, iSlot);
			sq->pushobject(v, *m_pFilterScripts[iSlot]);
			sq->set(v, -3);
		}
	}
	sq->settop(v, top2);

	sq->setroottable(v);

	char buffer[100];
	sprintf(buffer, "dofile( \"%s\")", szFilterScriptFile);
	SQRESULT a = sq->compilebuffer(v, buffer, strlen(buffer), szFilterScriptFile, SQTrue);
	
	//Now compiled buffer as a function at -1
	sq->pushroottable(v);
	a = sq->call(v, 1, 0, 1);
	if (SQ_FAILED(a))
	{
		VCMP->LogMessage("Failed to load '%s.nut' filter script.", pFileName);
		sq->push(v, top + 1);//the original roottable
		sq->setroottable(v);
		sq->settop(v, top);
		return false;
	}
	sq->remove(v, -1);//Removes the function formed from compiled buffer.
	
	sq->push(v, top + 1);//the original roottable
	sq->setroottable(v);
	
	//Call onFilterScriptLoad
	sq->pushobject(v, *m_pFilterScripts[iSlot]);
	sq->pushstring(v, "onFilterScriptLoad", -1);
	if (SQ_SUCCEEDED(sq->get(v, -2)))
	{
		sq->pushobject(v, *m_pFilterScripts[iSlot]);
		sq->call(v, 1, 1, 1);
	}
	strcpy(m_szFilterScriptName[iSlot], pFileName);
	m_iFilterScriptCount++;
	sq->settop(v, top);
	return true;
}
void CFilterScripts::UnloadFilterScripts()
{
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			RemoveFilterScript(i);
		}
	}

	m_iFilterScriptCount = 0;
}
bool CFilterScripts::UnloadOneFilterScript(char* pFilterScript)
{
	int i;
	for (i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (strcmp(pFilterScript, m_szFilterScriptName[i]) == 0) break;
	}
	if (i == MAX_FILTER_SCRIPTS) return false;
	if (m_pFilterScripts[i])
	{
		RemoveFilterScript(i);
		return true;
	}
	return false;
}
// Unloads the individual filterscript
void CFilterScripts::RemoveFilterScript(int iIndex)
{
	if (m_pFilterScripts[iIndex])
	{
		int top = sq->gettop(v);
		sq->pushobject(v, *m_pFilterScripts[iIndex]);
		sq->pushstring(v, "onFilterScriptUnload", -1);
		if (SQ_SUCCEEDED(sq->get(v, -2)))
		{
			sq->pushobject(v, *m_pFilterScripts[iIndex]);
			sq->call(v, 1, 1, 1);
		}
		sq->pushregistrytable(v);
		sq->pushstring(v, "vcmp_filterscripts", -1);
		if (SQ_SUCCEEDED(sq->get(v, -2)))
		{
			sq->pushstring(v, "fs_roottables", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushinteger(v, iIndex);
				sq->pushnull(v);
				sq->set(v, -3);
			}
		}
		sq->settop(v, top);
		m_iFilterScriptCount--;
	}
	SAFE_DELETE(m_pFilterScripts[iIndex]);
	m_szFilterScriptName[iIndex][0] = '\0';
}


bool CFilterScripts::FunctionExists(const char* name)
{
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, name, -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->settop(v, top); return true;
			}
		}
	}
	sq->settop(v, top);
	return false;
}
int CFilterScripts::GetSlotId(HSQUIRRELVM v, HSQOBJECT* obj)
{
	sq->pushobject(v, *obj);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			if (sq->cmp(v) == 0)
			{
				sq->pop(v, 2); return i;
			}
			sq->poptop(v);
		}
	}
	sq->poptop(v);
	return -1;
}
bool CFilterScripts::GetFilterScriptTable(int SlotId, HSQOBJECT*& table)
{
	if (m_pFilterScripts[SlotId] != NULL)
	{
		table = m_pFilterScripts[SlotId]; return true;
	}
	table = NULL; return false;
}
int CFilterScripts::onCheckpointEntered(HSQOBJECT* player, HSQOBJECT* checkpoint)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onCheckpointEntered", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushobject(v, *checkpoint);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onCheckpointExited(HSQOBJECT* player, HSQOBJECT* checkpoint)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onCheckpointExited", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushobject(v, *checkpoint);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onObjectShot(HSQOBJECT* object, HSQOBJECT* player, int32_t weaponId)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onObjectShot", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *object);
				sq->pushobject(v, *player);
				sq->pushinteger(v, weaponId);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onObjectBump(HSQOBJECT* object, HSQOBJECT* player)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onObjectBump", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *object);
				sq->pushobject(v, *player);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPickupClaimPicked(HSQOBJECT* player, HSQOBJECT* pickup)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPickupClaimPicked", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushobject(v, *pickup);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					return 0;
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;

}
int CFilterScripts::onPickupPickedUp(HSQOBJECT* player, HSQOBJECT* pickup)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPickupPickedUp", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushobject(v, *pickup);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPickupRespawn(HSQOBJECT* pickup)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPickupRespawn", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *pickup);
				if (SQ_FAILED(sq->call(v, 2, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;

}

int CFilterScripts::onPlayerRequestClass(HSQOBJECT* player, int32_t classId)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerRequestClass", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushinteger(v, classId);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					return 0;
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerRequestSpawn(HSQOBJECT* player)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerRequestSpawn", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				if (SQ_FAILED(sq->call(v, 2, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					return 0;
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerSpawn(HSQOBJECT* player)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerSpawn", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				if (SQ_FAILED(sq->call(v, 2, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerDeath(HSQOBJECT* player, int32_t reason)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerDeath", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushinteger(v, reason);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerTeamKill(HSQOBJECT* killer, HSQOBJECT* player, int32_t reason, int32_t bodyPart)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerTeamKill", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *killer);
				sq->pushobject(v, *player);
				sq->pushinteger(v, reason);
				sq->pushinteger(v, bodyPart);
				if (SQ_FAILED(sq->call(v, 5, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerKill(HSQOBJECT* killer, HSQOBJECT* player, int32_t reason, int32_t bodyPart)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerKill", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *killer);
				sq->pushobject(v, *player);
				sq->pushinteger(v, reason);
				sq->pushinteger(v, bodyPart);
				if (SQ_FAILED(sq->call(v, 5, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerChat(HSQOBJECT* player, const char* message)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerChat", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushstring(v, message, -1);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					return 0;
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
/*
int CFilterScripts::onPlayerCommand(HSQOBJECT* player, SQChar* szText, SQChar* szArguments)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerCommand", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushstring(v, _SC(szText), -1);
				if (szArguments == NULL || strlen(szArguments) <= 0)
					sq->pushnull(v);
				else
					sq->pushstring(v, _SC(szArguments), -1);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (ret) {//Callback returned 1
						sq->settop(v, top); return 1;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 0;
}
*/
int CFilterScripts::onPlayerPM(HSQOBJECT* player, HSQOBJECT* targetplayer, const char* message)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerPM", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushobject(v, *targetplayer);
				sq->pushstring(v, message, -1);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					return 0;
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerBeginTyping(HSQOBJECT* player)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerBeginTyping", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				if (SQ_FAILED(sq->call(v, 2, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerEndTyping(HSQOBJECT* player)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerEndTyping", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				if (SQ_FAILED(sq->call(v, 2, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onLoginAttempt(char* playerName, size_t nameBufferSize, const char* userPassword, const char* ipAddress)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onLoginAttempt", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushstring(v, (const char*)playerName, -1);
				sq->pushstring(v, userPassword, -1);
				sq->pushstring(v, ipAddress, -1);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					return 0;
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onKeyDown(HSQOBJECT* player, int32_t bindId)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onKeyDown", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushinteger(v, bindId);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onKeyUp(HSQOBJECT* player, int32_t bindId)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onKeyUp", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushinteger(v, bindId);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerAwayChange(HSQOBJECT* player, bool isAway)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerAwayChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				if (isAway)
					sq->pushbool(v, SQTrue);
				else
					sq->pushbool(v, SQFalse);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerSpectate(HSQOBJECT* player, HSQOBJECT* target)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerSpectate", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushobject(v, *target);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerCrashDump(HSQOBJECT* player, const char* report)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerCrashDump", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushstring(v, report, -1);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerNameChange(HSQOBJECT* player, const char* oldName, const char* newName)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerNameChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushstring(v, oldName, -1);
				sq->pushstring(v, newName, -1);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerActionChange(HSQOBJECT* player, int32_t oldAction, int32_t newAction)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerActionChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushinteger(v, oldAction);
				sq->pushinteger(v, newAction);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerStateChange(HSQOBJECT* player, int32_t oldState, int32_t newState)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerStateChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushinteger(v, oldState);
				sq->pushinteger(v, newState);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerOnFireChange(HSQOBJECT* player, bool isOnFireNow)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerOnFireChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				if (isOnFireNow)sq->pushbool(v, SQTrue);
				else sq->pushbool(v, SQFalse);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerCrouchChange(HSQOBJECT* player, bool isCrouchingNow)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerCrouchChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				if (isCrouchingNow)sq->pushbool(v, SQTrue);
				else sq->pushbool(v, SQFalse);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerGameKeysChange(HSQOBJECT* player, uint32_t oldKeys, uint32_t newKeys)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerGameKeysChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushinteger(v, oldKeys);
				sq->pushinteger(v, newKeys);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerEnteringVehicle(HSQOBJECT* player, HSQOBJECT* vehicle, int32_t slotIndex)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerEnteringVehicle", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushobject(v, *vehicle);
				sq->pushinteger(v, slotIndex);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					return 0;
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerEnterVehicle(HSQOBJECT* player, HSQOBJECT* vehicle, int32_t slotIndex)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerEnterVehicle", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushobject(v, *vehicle);
				sq->pushinteger(v, slotIndex);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerExitVehicle(HSQOBJECT* player, HSQOBJECT* vehicle)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerExitVehicle", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushobject(v, *vehicle);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{

				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onVehicleExplode(HSQOBJECT* vehicle)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onVehicleExplode", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *vehicle);
				if (SQ_FAILED(sq->call(v, 2, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onVehicleRespawn(HSQOBJECT* vehicle)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onVehicleRespawn", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *vehicle);
				if (SQ_FAILED(sq->call(v, 2, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerModuleList(HSQOBJECT* player, const char* list)
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerModuleList", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->pushobject(v, *player);
				sq->pushstring(v, list, -1);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerJoin()
{
	SQInteger ret; 
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerJoin", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 2, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerPart()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerPart", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top-1);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 3, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerMove()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerMove", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top - 6);
				sq->push(v, top - 5);
				sq->push(v, top - 4);
				sq->push(v, top - 3);
				sq->push(v, top - 2);
				sq->push(v, top - 1);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 8, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}

int CFilterScripts::onPlayerHealthChange()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerHealthChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top - 2);
				sq->push(v, top - 1);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;

}
int CFilterScripts::onPlayerArmourChange()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerArmourChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top - 2);
				sq->push(v, top - 1);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;

}
int CFilterScripts::onPlayerWeaponChange()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerWeaponChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top - 2);
				sq->push(v, top - 1);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onServerStart()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onServerStart", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				if (SQ_FAILED(sq->call(v, 1, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onServerStop()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onServerStop", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				if (SQ_FAILED(sq->call(v, 1, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onVehicleMove()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onVehicleMove", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top - 6);
				sq->push(v, top - 5);
				sq->push(v, top - 4);
				sq->push(v, top - 3);
				sq->push(v, top - 2);
				sq->push(v, top - 1);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 8, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onVehicleHealthChange()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onVehicleHealthChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top - 2);
				sq->push(v, top - 1);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onTimeChange()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onTimeChange", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top - 3);
				sq->push(v, top - 2);
				sq->push(v, top - 1);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 5, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onClientScriptData()
{
	SQInteger ret; char buffer[30];
	int top = sq->gettop(v);
	sprintf(buffer, "Stream.SetReadPosition(0)");
	if (SQ_FAILED(sq->compilebuffer(v, buffer, strlen(buffer), "onClientScriptData", 1)))
	{
		sq->settop(v, top);  return 0;
	}
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onClientScriptData", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				//Set the read position to 0.
				sq->push(v, top + 1);//function pushed to stack
				sq->pushroottable(v);//roottable pushed to stack
				if (SQ_FAILED(sq->call(v, 1, 0, 1)))
				{
					sq->settop(v, top); return 0;
				}
				sq->pop(v, 1);//that push of top + 1

				sq->pushobject(v, *m_pFilterScripts[i]);//filterscript table
				sq->push(v, top);//player
				if (SQ_FAILED(sq->call(v, 2, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (!ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 1;
}
int CFilterScripts::onPlayerCommand()
{
	SQInteger ret;
	int top = sq->gettop(v);
	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i] != NULL)
		{
			sq->pushobject(v, *m_pFilterScripts[i]);
			sq->pushstring(v, "onPlayerCommand", -1);
			if (SQ_SUCCEEDED(sq->get(v, -2)))
			{
				sq->pushobject(v, *m_pFilterScripts[i]);
				sq->push(v, top - 2);
				sq->push(v, top - 1);
				sq->push(v, top);
				if (SQ_FAILED(sq->call(v, 4, 1, SQTrue)))
					return -1;
				if (sq->gettype(v, -1) == OT_NULL)
				{
					//do nothing. user forgot to return anything.
				}
				else if (sq->gettype(v, -1) == OT_INTEGER)
				{
					sq->getinteger(v, -1, &ret);
					if (ret) {
						sq->settop(v, top); return ret;
					}
				}
			}
		}
	}
	sq->settop(v, top);
	return 0;
}
/*
// forward OnPlayerConnect(playerid);
int CFilterScripts::OnPlayerConnect(cell playerid)
{
	int idx;
	cell ret = 0;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerConnect", &idx))
			{
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				if (!ret) return (int)ret;
			}
		}
	}
	return (int)ret;
}
// forward OnPlayerDisconnect(playerid, reason);
int CFilterScripts::OnPlayerDisconnect(cell playerid, cell reason)
{
	int idx;
	cell ret = 0;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts && m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerDisconnect", &idx))
			{
				amx_Push(m_pFilterScripts[i], reason);
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				if (!ret) return (int)ret;
			}
		}
	}
	return (int)ret;
}
// forward OnPlayerSpawn(playerid);
int CFilterScripts::OnPlayerSpawn(cell playerid)
{
	int idx;
	cell ret = 0;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerSpawn", &idx))
			{
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				if (!ret) return (int)ret;
			}
		}
	}
	return (int)ret;
}
// forward OnPlayerDeath(playerid, killerid, reason, bodypart);
int CFilterScripts::OnPlayerDeath(cell playerid, cell killerid, cell reason, cell bodypart)
{
	int idx;
	cell ret = 0;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerDeath", &idx))
			{
				amx_Push(m_pFilterScripts[i], bodypart); 
				amx_Push(m_pFilterScripts[i], reason);
				amx_Push(m_pFilterScripts[i], killerid);
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				if (!ret) return ret;
			}
		}
	}
	return (int)ret;
}
// forward OnPlayerEnterVehicle(playerid, vehicleid, ispassenger);
int CFilterScripts::OnPlayerEnterVehicle(cell playerid, cell vehicleid, cell ispassenger)
{
	int idx;
	cell ret = 0;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerEnterVehicle", &idx))
			{
				amx_Push(m_pFilterScripts[i], ispassenger);
				amx_Push(m_pFilterScripts[i], vehicleid);
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				//if (!ret) return ret;
			}
		}
	}
	return (int)ret;
}
// forward OnPlayerExitVehicle(playerid, vehicleid);
int CFilterScripts::OnPlayerExitVehicle(cell playerid, cell vehicleid)
{
	int idx;
	cell ret = 0;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerExitVehicle", &idx))
			{
				amx_Push(m_pFilterScripts[i], vehicleid);
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				//if (!ret) return ret;
			}
		}
	}
	return (int)ret;
}
// forward OnPickedUp(pickupid, playerid);
int CFilterScripts::OnPickedUp(cell pickupid, cell playerid)
{
	int idx;
	cell ret = 0;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPickedUp", &idx))
			{

				amx_Push(m_pFilterScripts[i], playerid);
				amx_Push(m_pFilterScripts[i], pickupid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
			}
		}
	}
	return (int)ret;
}
// forward OnPlayerRequestClass(playerid, classid);
int CFilterScripts::OnPlayerRequestClass(cell playerid, cell classid)
{
	int idx;
	cell ret = 1;	// DEFAULT TO 1!

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerRequestClass", &idx))
			{
				amx_Push(m_pFilterScripts[i], classid);
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				//if (!ret) return ret;
			}
		}
	}
	return (int)ret;
}
// forward OnPlayerCommandText(playerid, cmdtext[]);
int CFilterScripts::OnPlayerCommandText(cell playerid, const char* szCommandText)
{
	int idx;
	cell ret = 0;

	int orig_strlen = strlen((char*)szCommandText);

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerCommandText", &idx))
			{
				cell* amx_addr;
				amx_PushString(m_pFilterScripts[i], &amx_addr, (char*)szCommandText, 0, 0);
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				amx_Release(m_pFilterScripts[i], amx_addr);
				if (ret) return 1; // Callback returned 1, so the command was accepted!
			}
		}
	}
	return (int)ret;
}
// forward OnPlayerText(playerid, text[]);
int CFilterScripts::OnPlayerText(cell playerid, const char* szText)
{
	int idx;
	cell ret = 1;	// DEFAULT TO 1!

	int orig_strlen = strlen((char*)szText) + 1;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerText", &idx))
			{
				cell *amx_addr;
				amx_PushString(m_pFilterScripts[i], &amx_addr,  (char*)szText, 0, 0);
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				amx_GetString((char*)szText, amx_addr, 0, orig_strlen);
				amx_Release(m_pFilterScripts[i], amx_addr);
				if (!ret) return 0; // Callback returned 0, so exit and don't display the text.
			}
		}
	}
	return (int)ret;
}
// forward OnPlayerPrivmsg(playerid, toplayerid, text[]);
int CFilterScripts::OnPlayerPrivmsg(cell playerid, cell toplayerid, const char* szText)
{
	int idx;
	cell ret = 1;	// DEFAULT TO 1!

	int orig_strlen = strlen((char*)szText) + 1;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnPlayerPrivmsg", &idx))
			{
				cell *amx_addr;
				amx_PushString(m_pFilterScripts[i], &amx_addr, (char*)szText, 0, 0);
				amx_Push(m_pFilterScripts[i], toplayerid);
				amx_Push(m_pFilterScripts[i], playerid);
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				amx_GetString((char*)szText, amx_addr, 0, orig_strlen);
				amx_Release(m_pFilterScripts[i], amx_addr);
				if (!ret) return 0; 
			}
		}
	}
	return (int)ret;
}
int CFilterScripts::OnGameModeExit()
{
	int idx;
	cell ret = 0;

	for (int i = 0; i < MAX_FILTER_SCRIPTS; i++)
	{
		if (m_pFilterScripts[i])
		{
			if (!amx_FindPublic(m_pFilterScripts[i], "OnGameModeExit", &idx))
			{
				amx_Exec(m_pFilterScripts[i], &ret, idx);
				if (!ret) return ret;
			}
		}
	}
	return (int)ret;
}*/