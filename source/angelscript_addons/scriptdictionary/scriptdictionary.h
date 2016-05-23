#ifndef SCRIPTDICTIONARY_H
#define SCRIPTDICTIONARY_H

// The dictionary class relies on the script string object, thus the script
// string type must be registered with the engine before registering the
// dictionary type

#ifndef ANGELSCRIPT_H 
// Avoid having to inform include path if header is already include before
#include <angelscript.h>
#endif

#include <string>

#ifdef _MSC_VER
// Turn off annoying warnings about truncated symbol names
#pragma warning (disable:4786)
#endif

#include <map>

// Sometimes it may be desired to use the same method names as used by C++ STL.
// This may for example reduce time when converting code from script to C++ or
// back.
//
//  0 = off
//  1 = on

#ifndef AS_USE_STLNAMES
#define AS_USE_STLNAMES 0
#endif


BEGIN_AS_NAMESPACE

class CScriptArray;

class CScriptDictionary
{
public:
    // Memory management
    CScriptDictionary(asIScriptEngine *engine);
    void AddRef() const;
    void Release() const;

    CScriptDictionary &operator =(const CScriptDictionary &other);

    // Sets/Gets a variable type value for a key
    void Set(const std::string &key, void *value, int typeId);
    bool Get(const std::string &key, void *value, int typeId) const;

    // Sets/Gets an integer number value for a key
    void Set(const std::string &key, asINT64 &value);
    bool Get(const std::string &key, asINT64 &value) const;

    // Sets/Gets a real number value for a key
    void Set(const std::string &key, double &value);
    bool Get(const std::string &key, double &value) const;

    // Returns true if the key is set
    bool Exists(const std::string &key) const;
	bool IsEmpty() const;
	asUINT GetSize() const;

    // Deletes the key
    void Delete(const std::string &key);

    // Deletes all keys
    void DeleteAll();

	// Get an array of all keys
	CScriptArray *GetKeys() const;

	// Garbage collections behaviours
	int GetRefCount();
	void SetGCFlag();
	bool GetGCFlag();
	void EnumReferences(asIScriptEngine *engine);
	void ReleaseAllReferences(asIScriptEngine *engine);

	// The structure for holding the values
    struct valueStruct
    {
        union
        {
            asINT64 valueInt;
            double  valueFlt;
            void   *valueObj;
        };
        int   typeId;
    };
    
	// HACK so we can interate over the items in C++
	// TODO: optimize: Use C++11 std::unordered_map instead
    std::map<std::string, valueStruct> dict;
	
protected:
	// We don't want anyone to call the destructor directly, it should be called through the Release method
	virtual ~CScriptDictionary();

	// Helper methods
    void FreeValue(valueStruct &value);
	
	// Our properties
    asIScriptEngine *engine;
    mutable int refCount;
	mutable bool gcFlag;
};

// This function will determine the configuration of the engine
// and use one of the two functions below to register the dictionary object
void RegisterScriptDictionary(asIScriptEngine *engine);

// Call this function to register the math functions
// using native calling conventions
void RegisterScriptDictionary_Native(asIScriptEngine *engine);

// Use this one instead if native calling conventions
// are not supported on the target platform
void RegisterScriptDictionary_Generic(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
