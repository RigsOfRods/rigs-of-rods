#ifndef SCRIPTBUILDER_H
#define SCRIPTBUILDER_H

//---------------------------
// Compilation settings
//

// Set this flag to turn on/off metadata processing
//  0 = off
//  1 = on
#ifndef AS_PROCESS_METADATA
#define AS_PROCESS_METADATA 1
#endif

// TODO: Implement flags for turning on/off include directives and conditional programming



//---------------------------
// Declaration
//

#include <angelscript.h>

#if defined(_MSC_VER) && _MSC_VER <= 1200
// disable the annoying warnings on MSVC 6
#pragma warning (disable:4786)
#endif

#include <string>
#include <map>
#include <set>
#include <vector>

BEGIN_AS_NAMESPACE

class CScriptBuilder;

// This callback will be called for each #include directive encountered by the
// builder. The callback should call the AddSectionFromFile or AddSectionFromMemory
// to add the included section to the script. If the include cannot be resolved
// then the function should return a negative value to abort the compilation.
typedef int (*INCLUDECALLBACK_t)(const char *include, const char *from, CScriptBuilder *builder, void *userParam);

// Helper class for loading and pre-processing script files to
// support include directives and metadata declarations
class CScriptBuilder
{
public:
	CScriptBuilder();

	// Start a new module
	int StartNewModule(asIScriptEngine *engine, const char *moduleName);

	// Load a script section from a file on disk
	int AddSectionFromFile(const char *filename);

	// Load a script section from memory
	int AddSectionFromMemory(const char *scriptCode,
							 const char *sectionName = "");

	// Build the added script sections
	int BuildModule();

	// Returns the current module
	asIScriptModule *GetModule();

	// Register the callback for resolving include directive
	void SetIncludeCallback(INCLUDECALLBACK_t callback, void *userParam);

	// Add a pre-processor define for conditional compilation
	void DefineWord(const char *word);

#if AS_PROCESS_METADATA == 1
	// Get metadata declared for class types and interfaces
	const char *GetMetadataStringForType(int typeId);

	// Get metadata declared for functions
	const char *GetMetadataStringForFunc(int funcId);

	// Get metadata declared for global variables
	const char *GetMetadataStringForVar(int varIdx);

	// Get metadata declared for class variables
	const char *GetMetadataStringForTypeProperty(int typeId, int varIdx);

	// Get metadata declared for class functions
	const char *GetMetadataStringForTypeMethod(int typeId, int methodIdx);
#endif

protected:
	void ClearAll();
	int  Build();
	int  ProcessScriptSection(const char *script, const char *sectionname);
	virtual int  LoadScriptSection(const char *filename) = 0;
	bool IncludeIfNotAlreadyIncluded(const char *filename);

	int  SkipStatement(int pos);

	int  ExcludeCode(int start);
	void OverwriteCode(int start, int len);

	asIScriptEngine           *engine;
	asIScriptModule           *module;
	std::string                modifiedScript;

	INCLUDECALLBACK_t  includeCallback;
	void              *callbackParam;

#if AS_PROCESS_METADATA == 1
	int  ExtractMetadataString(int pos, std::string &outMetadata);
	int  ExtractDeclaration(int pos, std::string &outDeclaration, int &outType);

	// Temporary structure for storing metadata and declaration
	struct SMetadataDecl
	{
		SMetadataDecl(std::string m, std::string d, int t, std::string c) : metadata(m), declaration(d), type(t), parentClass(c) {}
		std::string metadata;
		std::string declaration;
		int         type;
		std::string parentClass;
	};

	struct SClassMetadata
	{
		SClassMetadata(const std::string& aName) : className(aName) {}
		std::string className;
		std::map<int, std::string> funcMetadataMap;
		std::map<int, std::string> varMetadataMap;
	};

	std::string currentClass;

	std::vector<SMetadataDecl> foundDeclarations;

	std::map<int, std::string> typeMetadataMap;
	std::map<int, std::string> funcMetadataMap;
	std::map<int, std::string> varMetadataMap;
	std::map<int, SClassMetadata> classMetadataMap;
#endif

	std::set<std::string>      includedScripts;

	std::set<std::string>      definedWords;
};

END_AS_NAMESPACE

#endif
