// this is the default event handling routine
void defaultEventCallback(int trigger_type, string inst, string box, int nodeid)
{
	if(box == "shopairplane")
	{
		game.showChooser("airplane", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shopall")
	{
		game.showChooser("all", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shopboat")
	{
		game.showChooser("boat", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shopcar")
	{
		game.showChooser("car", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shopextension")
	{
		game.showChooser("extension", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shopheli")
	{
		game.showChooser("heli", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shopload")
	{
		game.showChooser("load", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shopplane") // deprecated
	{
		game.showChooser("airplane", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shoptrailer")
	{
		game.showChooser("trailer", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shoptrain")
	{
		game.showChooser("train", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shoptruck")
	{
		game.showChooser("truck", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "shopvehicle")
	{
		game.showChooser("vehicle", inst, "spawnzone");
		game.clearEventCache();
	} else if (box == "repair")
	{
		game.repairVehicle(inst, "spawnzone", false);
		game.clearEventCache();
	} else if (box == "removebeam")
	{
		game.removeVehicle(inst, "spawnzone");
		game.clearEventCache();
	}
}
