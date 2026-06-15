Utilizing OGRE resource groups (#UtilizingOgreResourceGroupsPage)
==============================

This largely duplicates the official doc: https://ogrecave.github.io/ogre/api/latest/_resource-_management.html but provides some insights specific to this project.

Jargon
------

In OGRE Jargon, "resource" is an in-memory object that serves as a catalogue entry for a piece of game data (mesh, material, texture etc.).
An OGRE resource usually has a file associated (.mesh, .material, .dds/png/jpg etc.), but you can manually create OGRE resource without a file.
This is useful for enlisting game data upfront but loading/unloading them on demand (i.e. materials get parsed upfront but textures only load with a mesh that uses the material).
To make sense of the API, it's important to remember that OGRE resources are only entries in memory, not the files on disk themselves.

OGRE resources are organized in "resource group"-s. One resource belongs exactly to one RG.
A RG always has a name and 'inGlobalPool' flag, and optionally any number of "resource location"-s.
A resource location is either a directory on disk (type="FileSystem") or a ZIP archive (type="Zip").
It must have a type and a 'readonly' flag, and can have filename filter associated.
This means disk files in one physical location can be divided into multiple distinct RGs, or a single location can be part of multiple RGs.
RGs are necessary to deal with mods - player may load and unload several mods during a session, and these mods may contain files with conflicting names. Thanks to RGs everything stays contained and organized.

The typical workflow is to set up RGs (either via OGRE's 'resources.cfg' or via code) and then auto-populate them based on files in resource locations. This process is called "initialization", see `initializeResourceGroup(rg)`.
The opposite process is called clearing, see `clearResourceGroup(rg)`.
Calling `createResource(filename,rg)` creates both "a resource" and a file on disk, handing you an open file stream for writing.
Calling `createManual(name, rg ...)` creates a "manually loaded resource" which has no associated file and doesn't unload/reload automatically.

Gotchas
-------

The 'inGlobalPool' flag isn't entirely intuitive.
You may assume the global pool is automatically visible to all other groups, regardless of their setting.
However, the global pool is a single 'logical' resource group where filenames must not clash.

If you specify the same resource location twice with a different value of 'readonly' flag, you get an error.
This is because under the hood the physical zipfiles/dirs on disk are represented as `Ogre::Archive` entries and unlike "resource location"-s which can overlap, these are unique for the given zipfile/dir and shared.
This may bite you because the `Ogre::Archive`-s persist even after all resource groups using them are destroyed, and they keep the readonly flag.

Built-in resource groups
------------------------

All are created on OGRE startup and have 'inGlobalPool=true' option regardless of any config.
    DEFAULT_RESOURCE_GROUP_NAME = RGN_DEFAULT;  = "General";  // the "General" group is synonymous to global pool
    INTERNAL_RESOURCE_GROUP_NAME = RGN_INTERNAL; = "OgreInternal"; 
    AUTODETECT_RESOURCE_GROUP_NAME = RGN_AUTODETECT; = "OgreAutodetect"; // autodetect includes the global pool
    ResourceGroupManager::mWorldGroupName = DEFAULT_RESOURCE_GROUP_NAME; // default world group to the default group

The OGRE_RESOURCEMANAGER_STRICT build option
--------------------------------------------

There are three modes for resource lookup:
    0 - LEGACY - search in all groups twice - for case sensitive and insensitive lookup [DEPRECATED]
    1 - PEDANTIC - require an explicit resource group. Case sensitive lookup.
    2 - STRICT - search in default group if not specified otherwise. Case sensitive lookup.
    
Difference in lookup: 
- with 1, you must always provide the RG as parameter: `getResourceByName(name, rg)` or `resourceExists(name, rg)` 
- with 2, you can leave out the rg parameter and OGRE will search in DEFAULT_RESOURCE_GROUP_NAME
- with 0, you can leave out the rg parameter and OGRE will search in all resource groups (via AUTODETECT_RESOURCE_GROUP_NAME)

Difference in global pool handling:
- you can always specify it by hand: `createResourceGroup(name, inGlobalPool)`
- with 0, default value of 'inGlobalPool' is true. otherwise, it's false.

OGRE_RESOURCEMANAGER_STRICT=0 also disables a lot of useful errors:
- void ResourceManager::remove(const String& name, const String& group) ~ "attempting to remove unknown resource: " + name + " in group " + group);
- void ResourceManager::remove(ResourceHandle handle) ~ "attempting to remove unknown resource"
- void ResourceManager::removeImpl(const ResourcePtr& res ) ~ "Resource '" + res->getName() + "' was not created by the '" + getResourceType() + "' ResourceManager"
- void ResourceManager::unload(const String& name, const String& group) ~ "attempting to unload unknown resource: " + name + " in group " + group
- void ResourceManager::unload(ResourceHandle handle) ~ "attempting to unload unknown resource"

- SPECIAL: `bool ZipArchive::exists(const String& filename) const` - here !STRICT seems to strip directory names from input path.

Rigs of Rods considerations
---------------------------

 -  The game must dynamically load and unload mods (terrains, vehicles, loads, dashboards, skins etc...) while assuring their data won't clash if having the same filenames or resource names.
 -  The game's own data must be accessible to all mods. If a clash occurs, the mod's resources should have precedence.
 -  The game must be able to load and unload files added during runtime (after startup), such as mods from repository and their thumbnails/gallery images.

Presently we use OGRE_RESOURCEMANAGER_STRICT=LEGACY (0) for backwards compatibility.
The early versions of the game used only the RGN_DEFAULT and early modders prefixed all mod filenames with unique IDs.
During the years, clashes appeared and all mods are now loaded into their own RGs.
To provide access to game files for each mod, game file locations are added to each individual mod RG.
This wastes some memory and processing time to repeatedly parse OGRE scripts, but simplifies usage - only one lookup covers all.
In contrast, using global pool for game data would require 2 lookups: in the per-actor RG first and then fallback to global pool - technically doable but would require more game code and testing.

