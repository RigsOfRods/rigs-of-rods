/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
// created on 21th of May 2008 by Thomas

#ifndef CACHESYSTEMTABLE_H__
#define CACHESYSTEMTABLE_H__

char *cachesystem_create_table_settings_sql = "CREATE TABLE IF NOT EXISTS settings (name TEXT PRIMARY KEY, value TEXT NOT NULL);";

char *cachesystem_create_table_mods_sql     = "CREATE TABLE IF NOT EXISTS mods ("\
		"id INTEGER PRIMARY KEY AUTOINCREMENT, " \
		"deleted INTEGER, " \
		"usagecounter INTEGER, " \
		"addtimestamp INTEGER, " \
		"minitype VARCHAR(1024), " \
		"type VARCHAR(255), " \
		"dirname VARCHAR(1024), " \
		"fname VARCHAR(1024), " \
		"fname_without_uid VARCHAR(1024), " \
		"fext VARCHAR(25), " \
		"filetime VARCHAR(128), " \
		"dname TEXT NOT NULL, "\
		"hash VARCHAR(41), " \
		"categoryid INTEGER, " \
		"uniqueid VARCHAR(41), " \
		"guid VARCHAR(41), " \
		"version INTEGER, " \
		"filecachename VARCHAR(1024)" \
	");";

char *cachesystem_create_table_categories_sql  = "CREATE TABLE IF NOT EXISTS categories ("\
		"id INTEGER PRIMARY KEY, " \
		"name TEXT" \
	");";
		
#endif //CACHESYSTEMTABLE_H__
