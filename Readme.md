1. Once the script is loaded, any insertion of events will hide that event in filterscript
eg. function onPlayerCommand(player, cmd, text)
	{
		if(cmd=..)
		{
			onVehicleRespawn<-function(vehid){...};//this will overload
			//the filterscript's onVehicleRespawn
		}
	}

But 
main.nut
function onVehicleRespawn(vehid)
{
	..
}
//is perfectly alright because this slot 'onVehicleRespawn' is created
before 'onScriptLoad'.