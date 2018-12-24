// this is the default event handling routine
void defaultEventCallback(int trigger_type, string inst, string box, int nodeid)
{
	if(box == "shopairplane")
	{
		game.showChooser("airplane", inst, "spawnzone");
	} else if (box == "shopall")
	{
		game.showChooser("all", inst, "spawnzone");
	} else if (box == "shopboat")
	{
		game.showChooser("boat", inst, "spawnzone");
	} else if (box == "shopcar")
	{
		game.showChooser("car", inst, "spawnzone");
	} else if (box == "shopextension")
	{
		game.showChooser("extension", inst, "spawnzone");
	} else if (box == "shopheli")
	{
		game.showChooser("heli", inst, "spawnzone");
	} else if (box == "shopload")
	{
		game.showChooser("load", inst, "spawnzone");
	} else if (box == "shopplane") // deprecated
	{
		game.showChooser("airplane", inst, "spawnzone");
	} else if (box == "shoptrailer")
	{
		game.showChooser("trailer", inst, "spawnzone");
	} else if (box == "shoptrain")
	{
		game.showChooser("train", inst, "spawnzone");
	} else if (box == "shoptruck")
	{
		game.showChooser("truck", inst, "spawnzone");
	} else if (box == "shopvehicle")
	{
		game.showChooser("vehicle", inst, "spawnzone");
	} else if (box == "repair")
	{
		game.repairVehicle(inst, "spawnzone", false);
	} else if (box == "removebeam")
	{
		game.removeVehicle(inst, "spawnzone");
	}
}
