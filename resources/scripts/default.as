#include "base.as"

#include "races.as"
racesManager races;

void main()
{
	int numRaces = races.loadRacesFromTerrn2();
	if (numRaces < 0)
		game.log("Error loading races from terrn2 [Races]: " + numRaces);
	else
		game.log("Loaded " + numRaces + " race-defs");
		
	game.log("default terrain script loaded");
}