#ifndef SCRIPTHANDLE_H
#define SCRIPTHANDLE_H

#include <angelscript.h>

BEGIN_AS_NAMESPACE

class CScriptHandle
{
public:
	// Constructors
	CScriptHandle();
	CScriptHandle(const CScriptHandle &other);
	CScriptHandle(void *ref, int typeId);
	~CScriptHandle();

	// Copy the stored value from another any object
	CScriptHandle &operator=(const CScriptHandle &other);
	CScriptHandle &opAssign(void *ref, int typeId);

	// Compare equalness
	bool opEquals(const CScriptHandle &o) const;
	bool opEquals(void *ref, int typeId) const;

	// Dynamic cast to desired handle type
	void opCast(void **outRef, int typeId);

protected:
	void ReleaseHandle();
	void AddRefHandle();

	void          *m_ref;
	asIObjectType *m_type;
};

void RegisterScriptHandle(asIScriptEngine *engine);

END_AS_NAMESPACE

#endif
