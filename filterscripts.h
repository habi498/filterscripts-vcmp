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
#ifndef FILTERSCRIPTS_H
#define FILTERSCRIPTS_H

class CFilterScripts
{
private:
	HSQOBJECT* m_pFilterScripts[MAX_FILTER_SCRIPTS];
	char m_szFilterScriptName[MAX_FILTER_SCRIPTS][255];
	int m_iFilterScriptCount;
	
	
public:
	CFilterScripts();
	~CFilterScripts();
	bool LoadFilterScript(char* pFileName);
	void UnloadFilterScripts();
	bool UnloadOneFilterScript(char* pFilterScript);
	void RemoveFilterScript(int iIndex);
	HSQOBJECT* m_pDelegateTable;
	HSQOBJECT* m_pCloneOrgRoottable;
	bool FunctionExists(const char* name);
	int GetSlotId(HSQUIRRELVM v, HSQOBJECT* obj);
	bool GetFilterScriptTable(int SlotId, HSQOBJECT* &table);
	int onCheckpointEntered(HSQOBJECT* player, HSQOBJECT* checkpoint);
	int onCheckpointExited(HSQOBJECT* player, HSQOBJECT* checkpoint);
	int onObjectShot(HSQOBJECT* object, HSQOBJECT* player, int32_t weaponId);
	int onObjectBump(HSQOBJECT* object, HSQOBJECT* player);
	int onPickupClaimPicked(HSQOBJECT* player, HSQOBJECT* pickup);
	int onPickupPickedUp(HSQOBJECT* player, HSQOBJECT* pickup);
	int onPickupRespawn(HSQOBJECT* pickup);
	int onPlayerRequestClass(HSQOBJECT* player, int32_t classId);
	int onPlayerRequestSpawn(HSQOBJECT* player);
	int onPlayerSpawn(HSQOBJECT* player);
	int onPlayerDeath(HSQOBJECT* player, int32_t reason);
	int onPlayerTeamKill(HSQOBJECT* killer, HSQOBJECT* player, int32_t reason, int32_t bodyPart);
	int onPlayerKill(HSQOBJECT* killer, HSQOBJECT* player, int32_t reason, int32_t bodyPart);
	int onPlayerChat(HSQOBJECT* player, const char*message);
	//int onPlayerCommand(HSQOBJECT* player, SQChar* szText, SQChar* szArguments);
	int onPlayerPM(HSQOBJECT* player, HSQOBJECT* targetplayer, const char* message);
	int onPlayerBeginTyping(HSQOBJECT* player);
	int onPlayerEndTyping(HSQOBJECT* player);
	int onLoginAttempt(char* playerName, size_t nameBufferSize, const char* userPassword, const char* ipAddress);
	int onKeyDown(HSQOBJECT* player, int32_t bindId);
	int onKeyUp(HSQOBJECT* player, int32_t bindId);
	int onPlayerAwayChange(HSQOBJECT* player, bool isAway);
	int onPlayerSpectate(HSQOBJECT* player, HSQOBJECT* target);
	int onPlayerCrashDump(HSQOBJECT* player, const char* report);
	int onPlayerNameChange(HSQOBJECT* player, const char* oldName, const char* newName);
	int onPlayerActionChange(HSQOBJECT* player, int32_t oldAction, int32_t newAction);
	int onPlayerStateChange(HSQOBJECT* player, int32_t oldState, int32_t newState);
	int onPlayerOnFireChange(HSQOBJECT* player, bool isOnFireNow);
	int onPlayerCrouchChange(HSQOBJECT* player, bool isCrouchingNow);
	int onPlayerGameKeysChange(HSQOBJECT* player, uint32_t oldKeys, uint32_t newKeys);
	int onPlayerEnteringVehicle(HSQOBJECT* player, HSQOBJECT* vehicle, int32_t slotIndex);
	int onPlayerEnterVehicle(HSQOBJECT* player, HSQOBJECT* vehicle, int32_t slotIndex);
	int onPlayerExitVehicle(HSQOBJECT* player, HSQOBJECT* vehicle);
	int onVehicleExplode(HSQOBJECT* vehicle);
	int onVehicleRespawn(HSQOBJECT* vehicle);
	int onPlayerModuleList(HSQOBJECT* player, const char* list);
	int onPlayerJoin();
	int onPlayerPart();
	int onPlayerMove();
	int onPlayerHealthChange();
	int onPlayerArmourChange();
	int onPlayerWeaponChange();
	int onServerStart();
	int onServerStop();
	int onVehicleMove();
	int onVehicleHealthChange();
	int onTimeChange();
	int onClientScriptData();
	int onPlayerCommand();
};
#endif
