AngelScript: Generic file format tokenizer                        {#GenericFileFormatPage}
==========================================

We have multiple formats with very similar syntax: rig def (truck), odef, tobj, race, addonpart, dashboard. This parser supports all these formats and adds new features:
 - **Support for "quoted strings with spaces"**. I first implemented them in the new .character fileformat and I decided I want them everywhere, notably .tobj so that procedural roads can have custom names and descriptions for a GPS-like navigation system.
 - **Simple editing in memory**. The document is a vector of Tokens (types: linebreak, comment, keyword, string, number, boolean.) which keeps the exact sequence as in the file. Tokens can be easily added/modified/removed without writing any extra custom code.
 - **Serializability** - saving the (possibly modified) file is as simple as looping through the Tokens array and writing them to a file. No custom code needs to be written for any file format.
 - **Ease of binding to AngelScript**: a single API can modify any fileformat. All the operations the user needs is 1. insert token 2. modify token 3. delete token.

The bundled 'demo_script.as' showcases them. There's a button "View document" (changes to "Close document" after pressing) which opens separate window with a syntax highlighted truck file.

**script API:**
* class GenericDocumentClass - loads/saves a document from OGRE resource system
* class GenericDocContextClass - traverses/adds/deletes document tokens
* enum TokenType (TOKEN_TYPE_*)
* enum Script2Game::GenericDocumentOptions (`GENERIC_DOCUMENT_OPTION_*`) - This is important to cover the many quirks that our pre-existing formats have, see subsections below.

Tips to retrieve live documents:
* TerrainClass::getTerrainFileName()
* TerrainClass::getTerrainFileResourceGroup()
* BeamClass::getTruckFileName()
* BeamClass::getTruckFileResourceGroup()

To test, open game console and say `loadscript demo_script.as`.
![obrazek](https://user-images.githubusercontent.com/491088/206774416-1ab7ce40-7d1e-46d1-9d6e-41f57ecdf068.png)

Parsing rig-def format
----------------------

This is the most frequented file type, also known as 'truck file format', see [reference documentation](https://docs.rigsofrods.org/vehicle-creation/fileformat-truck/).

Note the option 'ALLOW_NAKED_STRINGS' must be on at all times, because otherwise the parser requires all strings to be in quotes. Also see these special cases:
1. The first line is the actor name, with spaces and special characters, possibly including multiple quotes. I added an option "FIRST_LINE_IS_TITLE" which parses it as one string.
![obrazek](https://user-images.githubusercontent.com/491088/207772463-953f72c0-9a94-4d18-ac13-fa32f78f6be6.png)
2. in `forset`, single node numbers are parsed as NUMBER. Node ranges are not valid numbers so they decay to STRINGs. 
![obrazek](https://user-images.githubusercontent.com/491088/207771609-2aab6f24-abb7-4d1a-adf6-d0e6b81315f2.png)
3. The `axle` and `interaxle` keyword have a quirky `abc(1 2)` syntax. I added an option 'PARENTHESES_CAPTURE_SPACES' which make the whole expression parse as one string. 
![obrazek](https://user-images.githubusercontent.com/491088/207771935-aeb9bad9-419d-4f69-9f33-8bf428ad38b9.png)

Originally I only planned to cover fileformats which are similar to each other (truck, odef, tobj) but now I wanted the demo script to display TOBJ files too, but I didn't want to clog the Terrain API with `getFileWhatever()` funcs. I wanted to use the `GenericDocument` to parse the TERRN2 file, get TOBJ filenames from there and parse those as well. It turned out to be pretty straightforward, the tokenizer is already robust so it could take a few extra options.

Parsing TERRN2 format
---------------------

This is an INI-like format so it needs these special options: "ALLOW_HASH_COMMENTS", "ALLOW_SEPARATOR_EQUALS", "ALLOW_BRACED_KEYWORDS" and of course "ALLOW_NAKED_STRINGS".
![obrazek](https://user-images.githubusercontent.com/491088/207946139-f9d4598f-e911-4e81-b373-0355c7a56068.png)

Parsing ODEF/TOBJ formats
-------------------------

Needs just "ALLOW_NAKED_STRINGS" and "ALLOW_SLASH_COMMENTS".
![obrazek](https://user-images.githubusercontent.com/491088/207946561-79be9c22-dee8-4ce2-9699-07f1c3f00b50.png)


This page was archived from GitHub PR [#2953](https://github.com/RigsOfRods/rigs-of-rods/pull/2953).

