#include <new>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h> // sprintf

#include "scriptarray.h"

BEGIN_AS_NAMESPACE

static void RegisterScriptArray_Native(asIScriptEngine *engine);
static void RegisterScriptArray_Generic(asIScriptEngine *engine);

struct SArrayBuffer
{
	asDWORD numElements;
	asBYTE  data[1];
};

static CScriptArray* ScriptArrayFactory2(asIObjectType *ot, asUINT length)
{
	CScriptArray *a = new CScriptArray(length, ot);

	// It's possible the constructor raised a script exception, in which case we
	// need to free the memory and return null instead, else we get a memory leak.
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && ctx->GetState() == asEXECUTION_EXCEPTION )
	{
		delete a;
		return 0;
	}

	return a;
}

static CScriptArray* ScriptArrayFactoryDefVal(asIObjectType *ot, asUINT length, void *defVal)
{
	CScriptArray *a = new CScriptArray(length, defVal, ot);

	// It's possible the constructor raised a script exception, in which case we
	// need to free the memory and return null instead, else we get a memory leak.
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && ctx->GetState() == asEXECUTION_EXCEPTION )
	{
		delete a;
		return 0;
	}

	return a;	
}

static CScriptArray* ScriptArrayFactory(asIObjectType *ot)
{
	return ScriptArrayFactory2(ot, 0);
}

// This optional callback is called when the template type is first used by the compiler.
// It allows the application to validate if the template can be instanciated for the requested
// subtype at compile time, instead of at runtime.
static bool ScriptArrayTemplateCallback(asIObjectType *ot)
{
	// Make sure the subtype can be instanciated with a default factory/constructor,
	// otherwise we won't be able to instanciate the elements.
	int typeId = ot->GetSubTypeId();
	if( typeId == asTYPEID_VOID )
		return false;
	if( (typeId & asTYPEID_MASK_OBJECT) && !(typeId & asTYPEID_OBJHANDLE) )
	{
		asIObjectType *subtype = ot->GetEngine()->GetObjectTypeById(typeId);
		asDWORD flags = subtype->GetFlags();
		if( (flags & asOBJ_VALUE) && !(flags & asOBJ_POD) )
		{
			// Verify that there is a default constructor
			for( asUINT n = 0; n < subtype->GetBehaviourCount(); n++ )
			{
				asEBehaviours beh;
				int funcId = subtype->GetBehaviourByIndex(n, &beh);
				if( beh != asBEHAVE_CONSTRUCT ) continue;

				asIScriptFunction *func = ot->GetEngine()->GetFunctionById(funcId);
				if( func->GetParamCount() == 0 )
				{
					// Found the default constructor
					return true;
				}
			}

			// There is no default constructor
			return false;
		}
		else if( (flags & asOBJ_REF) )
		{
			// Verify that there is a default factory
			for( asUINT n = 0; n < subtype->GetFactoryCount(); n++ )
			{
				int funcId = subtype->GetFactoryIdByIndex(n);
				asIScriptFunction *func = ot->GetEngine()->GetFunctionById(funcId);
				if( func->GetParamCount() == 0 )
				{
					// Found the default factory
					return true;
				}
			}	

			// No default factory
			return false;
		}
	}

	// The type is ok
	return true;
}

// Registers the template array type
void RegisterScriptArray(asIScriptEngine *engine, bool defaultArray)
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") == 0 )
		RegisterScriptArray_Native(engine);
	else
		RegisterScriptArray_Generic(engine);

	if( defaultArray )
	{
		int r = engine->RegisterDefaultArrayType("array<T>"); assert( r >= 0 );
	}
}

static void RegisterScriptArray_Native(asIScriptEngine *engine)
{
	int r;
	
	// Register the array type as a template
	r = engine->RegisterObjectType("array<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE); assert( r >= 0 );

	// Register a callback for validating the subtype before it is used
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in)", asFUNCTION(ScriptArrayTemplateCallback), asCALL_CDECL); assert( r >= 0 );

	// Templates receive the object type as the first parameter. To the script writer this is hidden
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in)", asFUNCTIONPR(ScriptArrayFactory, (asIObjectType*), CScriptArray*), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, uint)", asFUNCTIONPR(ScriptArrayFactory2, (asIObjectType*, asUINT), CScriptArray*), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, uint, const T &in)", asFUNCTIONPR(ScriptArrayFactoryDefVal, (asIObjectType*, asUINT, void *), CScriptArray*), asCALL_CDECL); assert( r >= 0 );

	// Register the factory that will be used for initialization lists
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_LIST_FACTORY, "array<T>@ f(int&in, uint)", asFUNCTIONPR(ScriptArrayFactory2, (asIObjectType*, asUINT), CScriptArray*), asCALL_CDECL); assert( r >= 0 );

	// The memory management methods
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptArray,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptArray,Release), asCALL_THISCALL); assert( r >= 0 );

	// The index operator returns the template subtype
	r = engine->RegisterObjectMethod("array<T>", "T &opIndex(uint)", asMETHOD(CScriptArray, At), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "const T &opIndex(uint) const", asMETHOD(CScriptArray, At), asCALL_THISCALL); assert( r >= 0 );
	
	// The assignment operator
	r = engine->RegisterObjectMethod("array<T>", "array<T> &opAssign(const array<T>&in)", asMETHOD(CScriptArray, operator=), asCALL_THISCALL); assert( r >= 0 );

	// Other methods
	r = engine->RegisterObjectMethod("array<T>", "void insertAt(uint, const T&in)", asMETHOD(CScriptArray, InsertAt), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void removeAt(uint)", asMETHOD(CScriptArray, RemoveAt), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void insertLast(const T&in)", asMETHOD(CScriptArray, InsertLast), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void removeLast()", asMETHOD(CScriptArray, RemoveLast), asCALL_THISCALL); assert( r >= 0 );
	// TODO: Should length() and resize() be deprecated as the property accessors do the same thing?
	r = engine->RegisterObjectMethod("array<T>", "uint length() const", asMETHOD(CScriptArray, GetSize), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void resize(uint)", asMETHODPR(CScriptArray, Resize, (asUINT), void), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void sortAsc()", asMETHODPR(CScriptArray, SortAsc, (), void), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void sortAsc(uint, uint)", asMETHODPR(CScriptArray, SortAsc, (asUINT, asUINT), void), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void sortDesc()", asMETHODPR(CScriptArray, SortDesc, (), void), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void sortDesc(uint, uint)", asMETHODPR(CScriptArray, SortDesc, (asUINT, asUINT), void), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void reverse()", asMETHOD(CScriptArray, Reverse), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "int find(const T&in) const", asMETHODPR(CScriptArray, Find, (void*), int), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "int find(uint, const T&in) const", asMETHODPR(CScriptArray, Find, (asUINT, void*), int), asCALL_THISCALL); assert( r >= 0 );

	// Register virtual properties
	r = engine->RegisterObjectMethod("array<T>", "uint get_length() const", asMETHOD(CScriptArray, GetSize), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void set_length(uint)", asMETHODPR(CScriptArray, Resize, (asUINT), void), asCALL_THISCALL); assert( r >= 0 );

	// Register GC behaviours in case the array needs to be garbage collected
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(CScriptArray, GetRefCount), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(CScriptArray, SetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(CScriptArray, GetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(CScriptArray, EnumReferences), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(CScriptArray, ReleaseAllHandles), asCALL_THISCALL); assert( r >= 0 );
}

CScriptArray &CScriptArray::operator=(const CScriptArray &other)
{
	// Only perform the copy if the array types are the same
	if( &other != this &&
		other.GetArrayObjectType() == GetArrayObjectType() )
	{
		if( buffer )
		{
			DeleteBuffer(buffer);
			buffer = 0;
		}

		// Copy all elements from the other array
		CreateBuffer(&buffer, other.buffer->numElements);
		CopyBuffer(buffer, other.buffer);
	}

	return *this;
}

CScriptArray::CScriptArray(asUINT length, asIObjectType *ot)
{
	refCount = 1;
	gcFlag = false;
	objType = ot;
	objType->AddRef();
	buffer = 0;

	Precache();

	// Determine element size
	if( subTypeId & asTYPEID_MASK_OBJECT )
	{
		elementSize = sizeof(asPWORD);
	}
	else
	{
		elementSize = objType->GetEngine()->GetSizeOfPrimitiveType(subTypeId);
	}

	// Make sure the array size isn't too large for us to handle
	if( !CheckMaxSize(length) )
	{
		// Don't continue with the initialization
		return;
	}

	CreateBuffer(&buffer, length);

	// Notify the GC of the successful creation
	if( objType->GetFlags() & asOBJ_GC )
		objType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, objType->GetTypeId());
}

CScriptArray::CScriptArray(asUINT length, void *defVal, asIObjectType *ot)
{
	refCount = 1;
	gcFlag = false;
	objType = ot;
	objType->AddRef();
	buffer = 0;

	Precache();

	// Determine element size
	if( subTypeId & asTYPEID_MASK_OBJECT )
	{
		elementSize = sizeof(asPWORD);
	}
	else
	{
		elementSize = objType->GetEngine()->GetSizeOfPrimitiveType(subTypeId);
	}

	// Make sure the array size isn't too large for us to handle
	if( !CheckMaxSize(length) )
	{
		// Don't continue with the initialization
		return;
	}

	CreateBuffer(&buffer, length);

	// Notify the GC of the successful creation
	if( objType->GetFlags() & asOBJ_GC )
		objType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, objType->GetTypeId());

	// Initialize the elements with the default value
	for( asUINT n = 0; n < GetSize(); n++ )
		SetValue(n, defVal);
}

// Internal
void CScriptArray::SetValue(asUINT index, void *value)
{
	if( (subTypeId & ~asTYPEID_MASK_SEQNBR) && !(subTypeId & asTYPEID_OBJHANDLE) )
		objType->GetEngine()->CopyScriptObject(At(index), value, subTypeId);
	else if( subTypeId & asTYPEID_OBJHANDLE )
	{
		*(void**)At(index) = *(void**)value;
		objType->GetEngine()->AddRefScriptObject(*(void**)value, objType->GetSubType());
	}
	else if( subTypeId == asTYPEID_BOOL ||
			 subTypeId == asTYPEID_INT8 ||
			 subTypeId == asTYPEID_UINT8 )
		*(char*)At(index) = *(char*)value;
	else if( subTypeId == asTYPEID_INT16 ||
			 subTypeId == asTYPEID_UINT16 )
		*(short*)At(index) = *(short*)value;
	else if( subTypeId == asTYPEID_INT32 ||
			 subTypeId == asTYPEID_UINT32 ||
			 subTypeId == asTYPEID_FLOAT )
		*(int*)At(index) = *(int*)value;
	else if( subTypeId == asTYPEID_INT64 ||
			 subTypeId == asTYPEID_UINT64 ||
			 subTypeId == asTYPEID_DOUBLE )
		*(double*)At(index) = *(double*)value;
}

CScriptArray::~CScriptArray()
{
	if( buffer )
	{
		DeleteBuffer(buffer);
		buffer = 0;
	}
	if( objType ) objType->Release();
}

asUINT CScriptArray::GetSize() const
{
	return buffer->numElements;
}

void CScriptArray::Resize(asUINT numElements)
{
	if( numElements & 0x80000000 )
	{
		CheckMaxSize(numElements);
		return;
	}

	Resize((int)numElements - (int)buffer->numElements, (asUINT)-1);
}

// Internal
void CScriptArray::Resize(int delta, asUINT at)
{
	if( delta < 0 )
	{
		if( -delta > (int)buffer->numElements )
			delta = -(int)buffer->numElements;
		if( at > buffer->numElements + delta )
			at = buffer->numElements + delta;
	}
	else if( delta > 0 )
	{
		// Make sure the array size isn't too large for us to handle
		if( delta > 0 && !CheckMaxSize(buffer->numElements + delta) )
			return;

		if( at > buffer->numElements )
			at = buffer->numElements;
	}

	if( delta == 0 ) return;

	// Allocate memory for the buffer
	SArrayBuffer *newBuffer;
	newBuffer = (SArrayBuffer*)new asBYTE[sizeof(SArrayBuffer)-1 + elementSize*(buffer->numElements + delta)];
	newBuffer->numElements = buffer->numElements + delta;

	memcpy(newBuffer->data, buffer->data, at*elementSize);
	if( delta > 0 && at < buffer->numElements )
		memcpy(newBuffer->data + (at+delta)*elementSize, buffer->data + at*elementSize, (buffer->numElements-at)*elementSize);
	else if( delta < 0 && at < buffer->numElements )
		memcpy(newBuffer->data + at*elementSize, buffer->data + (at-delta)*elementSize, (buffer->numElements-at+delta)*elementSize);

	if( subTypeId & asTYPEID_MASK_OBJECT )
	{
		if( delta > 0 )
			Construct(newBuffer, at, at+delta);
		else if( delta < 0 )
			Destruct(buffer, at, at-delta);
	}

	// Release the old buffer
	delete[] (asBYTE*)buffer;

	buffer = newBuffer;
}

// internal
bool CScriptArray::CheckMaxSize(asUINT numElements)
{
	// This code makes sure the size of the buffer that is allocated
	// for the array doesn't overflow and becomes smaller than requested

	asUINT maxSize = 0xFFFFFFFFul - sizeof(SArrayBuffer) + 1;
	if( subTypeId & asTYPEID_MASK_OBJECT )
		maxSize /= sizeof(void*);
	else if( elementSize > 0 )
		maxSize /= elementSize;

	if( numElements > maxSize )
	{
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
		{
			// Set a script exception
			ctx->SetException("Too large array size");
		}

		return false;
	}

	// OK
	return true;
}

asIObjectType *CScriptArray::GetArrayObjectType() const
{
	return objType;
}

int CScriptArray::GetArrayTypeId() const
{
	return objType->GetTypeId();
}

int CScriptArray::GetElementTypeId() const
{
	return subTypeId;
}

void CScriptArray::InsertAt(asUINT index, void *value)
{
	if( index > buffer->numElements )
	{
		// If this is called from a script we raise a script exception
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException("Index out of bounds");
		return;
	}

	// Make room for the new element
	Resize(1, index);

	// Set the value of the new element
	SetValue(index, value);
}

void CScriptArray::InsertLast(void *value)
{
	InsertAt(buffer->numElements, value);
}

void CScriptArray::RemoveAt(asUINT index)
{
	if( index >= buffer->numElements )
	{
		// If this is called from a script we raise a script exception
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException("Index out of bounds");
		return;
	}

	// Remove the element
	Resize(-1, index);
}

void CScriptArray::RemoveLast()
{
	RemoveAt(buffer->numElements-1);
}

// Return a pointer to the array element. Returns 0 if the index is out of bounds
void *CScriptArray::At(asUINT index)
{
	if( index >= buffer->numElements )
	{
		// If this is called from a script we raise a script exception
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException("Index out of bounds");
		return 0;
	}
	else
	{
		if( (subTypeId & asTYPEID_MASK_OBJECT) && !(subTypeId & asTYPEID_OBJHANDLE) )
			return (void*)((size_t*)buffer->data)[index];
		else
			return buffer->data + elementSize*index;
	}
}

// internal
void CScriptArray::CreateBuffer(SArrayBuffer **buf, asUINT numElements)
{
	if( subTypeId & asTYPEID_MASK_OBJECT )
	{
		*buf = (SArrayBuffer*)new asBYTE[sizeof(SArrayBuffer)-1+sizeof(void*)*numElements];
		(*buf)->numElements = numElements;
	}
	else
	{
		*buf = (SArrayBuffer*)new asBYTE[sizeof(SArrayBuffer)-1+elementSize*numElements];
		(*buf)->numElements = numElements;
	}

	Construct(*buf, 0, numElements);
}

// internal
void CScriptArray::DeleteBuffer(SArrayBuffer *buf)
{
	Destruct(buf, 0, buf->numElements);

	// Free the buffer
	delete[] (asBYTE*)buf;
}

// internal
void CScriptArray::Construct(SArrayBuffer *buf, asUINT start, asUINT end)
{
	if( subTypeId & asTYPEID_OBJHANDLE )
	{
		// Set all object handles to null
		void *d = (void*)(buf->data + start * sizeof(void*));
		memset(d, 0, (end-start)*sizeof(void*));
	}
	else if( subTypeId & asTYPEID_MASK_OBJECT )
	{
		void **max = (void**)(buf->data + end * sizeof(void*));
		void **d = (void**)(buf->data + start * sizeof(void*));

		asIScriptEngine *engine = objType->GetEngine();

		for( ; d < max; d++ )
			*d = (void*)engine->CreateScriptObject(subTypeId);
	}
}

// internal
void CScriptArray::Destruct(SArrayBuffer *buf, asUINT start, asUINT end)
{
	if( subTypeId & asTYPEID_MASK_OBJECT )
	{
		asIScriptEngine *engine = objType->GetEngine();

		void **max = (void**)(buf->data + end * sizeof(void*));
		void **d   = (void**)(buf->data + start * sizeof(void*));

		for( ; d < max; d++ )
		{
			if( *d )
				engine->ReleaseScriptObject(*d, objType->GetSubType());
		}
	}
}


// internal
bool CScriptArray::Less(const void *a, const void *b, bool asc, asIScriptContext *ctx)
{
	if( !asc )
	{
		// Swap items
		const void *TEMP = a;
		a = b;
		b = TEMP;
	}

	if( subTypeId <= asTYPEID_DOUBLE )
	{
		// Simple compare of values
		switch( subTypeId )
		{
			#define COMPARE(T) *((T*)a) < *((T*)b)
			case asTYPEID_BOOL: return COMPARE(bool);
			case asTYPEID_INT8: return COMPARE(signed char);
			case asTYPEID_UINT8: return COMPARE(unsigned char);
			case asTYPEID_INT16: return COMPARE(signed short);
			case asTYPEID_UINT16: return COMPARE(unsigned short);
			case asTYPEID_INT32: return COMPARE(signed int);
			case asTYPEID_UINT32: return COMPARE(unsigned int);
			case asTYPEID_FLOAT: return COMPARE(float);
			case asTYPEID_DOUBLE: return COMPARE(double);
			#undef COMPARE
		}
	}
	else
	{
		int r = 0;

		// Execute object opCmp
		// TODO: Add proper error handling
		r = ctx->Prepare(cmpFuncId); assert(r >= 0);
		r = ctx->SetObject((void*)a); assert(r >= 0);
		r = ctx->SetArgAddress(0, (void*)b); assert(r >= 0);
		r = ctx->Execute();

		if( r == asEXECUTION_FINISHED )
		{
			return (int)ctx->GetReturnDWord() < 0;
		}
	}

	return false;
}

void CScriptArray::Reverse()
{
	asUINT size = GetSize();

	if( size >= 2 )
	{
		asBYTE TEMP[16];

		for( asUINT i = 0; i < size / 2; i++ )
		{
			Copy(TEMP, GetArrayItemPointer(i));
			Copy(GetArrayItemPointer(i), GetArrayItemPointer(size - i - 1));
			Copy(GetArrayItemPointer(size - i - 1), TEMP);
		}
	}
}

// internal
bool CScriptArray::Equals(const void *a, const void *b, asIScriptContext *ctx)
{
	if( subTypeId <= asTYPEID_DOUBLE )
	{
		// Simple compare of values
		switch( subTypeId )
		{
			#define COMPARE(T) *((T*)a) == *((T*)b)
			case asTYPEID_BOOL: return COMPARE(bool);
			case asTYPEID_INT8: return COMPARE(signed char);
			case asTYPEID_UINT8: return COMPARE(unsigned char);
			case asTYPEID_INT16: return COMPARE(signed short);
			case asTYPEID_UINT16: return COMPARE(unsigned short);
			case asTYPEID_INT32: return COMPARE(signed int);
			case asTYPEID_UINT32: return COMPARE(unsigned int);
			case asTYPEID_FLOAT: return COMPARE(float);
			case asTYPEID_DOUBLE: return COMPARE(double);
			#undef COMPARE
		}
	}
	else
	{
		int r = 0;

		// Execute object opEquals if available
		if( eqFuncId >= 0 )
		{
			// TODO: Add proper error handling
			r = ctx->Prepare(eqFuncId); assert(r >= 0);
			r = ctx->SetObject((void*)a); assert(r >= 0);
			r = ctx->SetArgAddress(0, (void*)b); assert(r >= 0);
			r = ctx->Execute();

			if( r == asEXECUTION_FINISHED )
			{
				return ctx->GetReturnByte() != 0;
			}
		}

		// Execute object opCmp if available
		if( cmpFuncId >= 0 )
		{
			// TODO: Add proper error handling
			r = ctx->Prepare(cmpFuncId); assert(r >= 0);
			r = ctx->SetObject((void*)a); assert(r >= 0);
			r = ctx->SetArgAddress(0, (void*)b); assert(r >= 0);
			r = ctx->Execute();

			if( r == asEXECUTION_FINISHED )
			{
				return (int)ctx->GetReturnDWord() == 0;
			}
		}
	}

	return false;
}

int CScriptArray::Find(void *value)
{
	return Find(0, value);
}

int CScriptArray::Find(asUINT index, void *value)
{
	// Subtype isn't primitive and doesn't have opEquals / opCmp
	if( subTypeId > asTYPEID_DOUBLE && (cmpFuncId <= 0 && eqFuncId <= 0) )
	{
		asIScriptContext *ctx = asGetActiveContext();
		asIObjectType* subType = objType->GetEngine()->GetObjectTypeById(subTypeId);

		// Throw an exception
		if( ctx )
		{
			char tmp[512];
#if defined(_MSC_VER) && _MSC_VER >= 1500
			sprintf_s(tmp, 512, "Type '%s' does not have opEquals / opCmp", subType->GetName());
#else
			sprintf(tmp, "Type '%s' does not have opEquals / opCmp", subType->GetName());
#endif
			ctx->SetException(tmp);
		}

		return -1;
	}

	asIScriptContext *cmpContext = 0;

	if( subTypeId > asTYPEID_DOUBLE )
	{
		// TODO: Ideally this context would be retrieved from a pool, so we don't have to
		//       create a new one everytime. We could keep a context with the array object
		//       but that would consume a lot of resources as each context is quite heavy.
		cmpContext = objType->GetEngine()->CreateContext();
	}

	int ret = -1;
	asUINT size = GetSize();

	if( index < size )
	{
		for( asUINT i = index; i < size; i++ )
		{
			// value passed by reference
			if( Equals(At(i), (value), cmpContext) )
			{
				ret = (int)i;
				break;
			}
		}
	}

	if( cmpContext )
		cmpContext->Release();

	return ret;
}



// internal
// Copy object handle or primitive value
void CScriptArray::Copy(void *dst, void *src)
{
	memcpy(dst, src, elementSize);
}


// internal
// Return pointer to array item (object handle or primitive value)
void *CScriptArray::GetArrayItemPointer(int index)
{
	return buffer->data + index * elementSize;
}

// internal
// Return pointer to data in buffer (object or primitive)
void *CScriptArray::GetDataPointer(void *buffer)
{
	if ((subTypeId & asTYPEID_MASK_OBJECT) && !(subTypeId & asTYPEID_OBJHANDLE) )
	{
		// Real address of object
		return reinterpret_cast<void*>(*(size_t*)buffer);
	}
	else
	{
		// Primitive is just a raw data
		return buffer;
	}
}


// Sort ascending
void CScriptArray::SortAsc()
{
	Sort(0, GetSize(), true);
}

// Sort ascending
void CScriptArray::SortAsc(asUINT index, asUINT count)
{
	Sort(index, count, true);
}

// Sort descending
void CScriptArray::SortDesc()
{
	Sort(0, GetSize(), false);
}

// Sort descending
void CScriptArray::SortDesc(asUINT index, asUINT count)
{
	Sort(index, count, false);
}


// internal
void CScriptArray::Sort(asUINT index, asUINT count, bool asc)
{
	// Subtype isn't primitive and doesn't have opCmp
	if( subTypeId > asTYPEID_DOUBLE && cmpFuncId <= 0 )
	{
		asIScriptContext *ctx = asGetActiveContext();
		asIObjectType* subType = objType->GetEngine()->GetObjectTypeById(subTypeId);

		// Throw an exception
		if( ctx )
		{
			char tmp[512];
#if defined(_MSC_VER) && _MSC_VER >= 1500
			sprintf_s(tmp, 512, "Type '%s' does not have opCmp", subType->GetName());
#else
			sprintf(tmp, "Type '%s' does not have opCmp", subType->GetName());
#endif
			ctx->SetException(tmp);
		}

		return;
	}

	// No need to sort
	if( count < 2 )
	{
		return;
	}

	int start = index;
	int end = index + count;

	// Check if we could access invalid item while sorting
	if( start >= (int)buffer->numElements || end > (int)buffer->numElements )
	{
		asIScriptContext *ctx = asGetActiveContext();

		// Throw an exception
		if( ctx )
		{
			ctx->SetException("Index out of bounds");
		}

		return;
	}

	asBYTE tmp[16];
	asIScriptContext *cmpContext = 0;

	if( subTypeId > asTYPEID_DOUBLE )
	{
		// TODO: Ideally this context would be retrieved from a pool, so we don't have to
		//       create a new one everytime. We could keep a context with the array object
		//       but that would consume a lot of resources as each context is quite heavy.
		cmpContext = objType->GetEngine()->CreateContext();
	}

	// Insertion sort
	for( int i = start + 1; i < end; i++ )
	{
		Copy(tmp, GetArrayItemPointer(i));

		int j = i - 1;

		while( j >= start && Less(GetDataPointer(tmp), At(j), asc, cmpContext) )
		{
			Copy(GetArrayItemPointer(j + 1), GetArrayItemPointer(j));
			j--;
		}

		Copy(GetArrayItemPointer(j + 1), tmp);
	}

	if( cmpContext )
		cmpContext->Release();
}

// internal
void CScriptArray::CopyBuffer(SArrayBuffer *dst, SArrayBuffer *src)
{
	asIScriptEngine *engine = objType->GetEngine();
	if( subTypeId & asTYPEID_OBJHANDLE )
	{
		// Copy the references and increase the reference counters
		if( dst->numElements > 0 && src->numElements > 0 )
		{
			int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;

			void **max = (void**)(dst->data + count * sizeof(void*));
			void **d   = (void**)dst->data;
			void **s   = (void**)src->data;
			
			for( ; d < max; d++, s++ )
			{
				*d = *s;
				if( *d )
					engine->AddRefScriptObject(*d, objType->GetSubType());
			}
		}
	}
	else
	{
		if( dst->numElements > 0 && src->numElements > 0 )
		{
			int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;
			if( subTypeId & asTYPEID_MASK_OBJECT )
			{
				// Call the assignment operator on all of the objects
				void **max = (void**)(dst->data + count * sizeof(void*));
				void **d   = (void**)dst->data;
				void **s   = (void**)src->data;

				for( ; d < max; d++, s++ )
					engine->CopyScriptObject(*d, *s, subTypeId);
			}
			else
			{
				// Primitives are copied byte for byte
				memcpy(dst->data, src->data, count*elementSize);
			}
		}
	}
}

// internal
// Precache some info
void CScriptArray::Precache()
{
	// TODO: optimize: This information could be stored in the object type as user data,
	//                 then it wouldn't be necessary to look for this for each array initialization

	subTypeId = objType->GetSubTypeId();

	cmpFuncId = -1;
	eqFuncId = -1;

	// Object - search for opCmp / opEquals
	if( subTypeId > asTYPEID_DOUBLE )
	{
		asIObjectType *subType = objType->GetEngine()->GetObjectTypeById(subTypeId);

		if( subType )
		{
			for( asUINT i = 0; i < subType->GetMethodCount(); i++ )
			{
				asIScriptFunction *func = subType->GetMethodByIndex(i);

				if( func->GetParamCount() == 1 /* && func->IsReadOnly() */ )
				{
					asDWORD flags = 0;
					int returnTypeId = func->GetReturnTypeId();
					int paramTypeId = func->GetParamTypeId(0, &flags);
				
					if( flags == asTM_INREF && paramTypeId == subTypeId )
					{
						if( returnTypeId == asTYPEID_INT32 && strcmp(func->GetName(), "opCmp") == 0 )
						{
							cmpFuncId = subType->GetMethodIdByIndex(i);
						}

						if( returnTypeId == asTYPEID_BOOL && strcmp(func->GetName(), "opEquals") == 0 )
						{
							eqFuncId = subType->GetMethodIdByIndex(i);
						}

						if( cmpFuncId >= 0 && eqFuncId >= 0 )
						{
							break;
						}
					}
				}
			}
		}
	}
}

// GC behaviour
void CScriptArray::EnumReferences(asIScriptEngine *engine)
{
	// If the array is holding handles, then we need to notify the GC of them
	if( subTypeId & asTYPEID_MASK_OBJECT )
	{
		void **d = (void**)buffer->data;
		for( asUINT n = 0; n < buffer->numElements; n++ )
		{
			if( d[n] )
				engine->GCEnumCallback(d[n]);
		}
	}
}

// GC behaviour
void CScriptArray::ReleaseAllHandles(asIScriptEngine *engine)
{
	// Resizing to zero will release everything
	Resize(0);
}

void CScriptArray::AddRef() const
{
	// Clear the GC flag then increase the counter
	gcFlag = false;
	refCount++;
}

void CScriptArray::Release() const
{
	// Now do the actual releasing (clearing the flag set by GC)
	gcFlag = false;
	if( --refCount == 0 )
	{
		delete this;
	}
}

// GC behaviour
int CScriptArray::GetRefCount()
{
	return refCount;
}

// GC behaviour
void CScriptArray::SetFlag()
{
	gcFlag = true;
}

// GC behaviour
bool CScriptArray::GetFlag()
{
	return gcFlag;
}

//--------------------------------------------
// Generic calling conventions

static void ScriptArrayFactory_Generic(asIScriptGeneric *gen)
{
	asIObjectType *ot = *(asIObjectType**)gen->GetAddressOfArg(0);

	*(CScriptArray**)gen->GetAddressOfReturnLocation() = ScriptArrayFactory(ot);
}

static void ScriptArrayFactory2_Generic(asIScriptGeneric *gen)
{
	asIObjectType *ot = *(asIObjectType**)gen->GetAddressOfArg(0);
	asUINT length = gen->GetArgDWord(1);

	*(CScriptArray**)gen->GetAddressOfReturnLocation() = ScriptArrayFactory2(ot, length);
}

static void ScriptArrayFactoryDefVal_Generic(asIScriptGeneric *gen)
{
	asIObjectType *ot = *(asIObjectType**)gen->GetAddressOfArg(0);
	asUINT length = gen->GetArgDWord(1);
	void *defVal = gen->GetArgAddress(2);

	*(CScriptArray**)gen->GetAddressOfReturnLocation() = ScriptArrayFactoryDefVal(ot, length, defVal);
}

static void ScriptArrayTemplateCallback_Generic(asIScriptGeneric *gen)
{
	asIObjectType *ot = *(asIObjectType**)gen->GetAddressOfArg(0);
	*(bool*)gen->GetAddressOfReturnLocation() = ScriptArrayTemplateCallback(ot);
}

static void ScriptArrayAssignment_Generic(asIScriptGeneric *gen)
{
	CScriptArray *other = (CScriptArray*)gen->GetArgObject(0);
	CScriptArray *self = (CScriptArray*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

static void ScriptArrayAt_Generic(asIScriptGeneric *gen)
{
	asUINT index = gen->GetArgDWord(0);
	CScriptArray *self = (CScriptArray*)gen->GetObject();

	gen->SetReturnAddress(self->At(index));
}

static void ScriptArrayLength_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();

	gen->SetReturnDWord(self->GetSize());
}

static void ScriptArrayResize_Generic(asIScriptGeneric *gen)
{
	asUINT size = gen->GetArgDWord(0);
	CScriptArray *self = (CScriptArray*)gen->GetObject();

	self->Resize(size);
}

static void ScriptArrayAddRef_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	self->AddRef();
}

static void ScriptArrayRelease_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	self->Release();
}

static void ScriptArrayGetRefCount_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	*(int*)gen->GetAddressOfReturnLocation() = self->GetRefCount();
}

static void ScriptArraySetFlag_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	self->SetFlag();
}

static void ScriptArrayGetFlag_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	*(bool*)gen->GetAddressOfReturnLocation() = self->GetFlag();
}

static void ScriptArrayEnumReferences_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->EnumReferences(engine);
}

static void ScriptArrayReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->ReleaseAllHandles(engine);
}

static void RegisterScriptArray_Generic(asIScriptEngine *engine)
{
	int r;
	
	r = engine->RegisterObjectType("array<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in)", asFUNCTION(ScriptArrayFactory_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in)", asFUNCTION(ScriptArrayTemplateCallback_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, uint)", asFUNCTION(ScriptArrayFactory2_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, uint, const T &in)", asFUNCTION(ScriptArrayFactoryDefVal_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_LIST_FACTORY, "array<T>@ f(int&in, uint)", asFUNCTION(ScriptArrayFactory2_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptArrayAddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptArrayRelease_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "T &opIndex(uint)", asFUNCTION(ScriptArrayAt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "const T &opIndex(uint) const", asFUNCTION(ScriptArrayAt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "array<T> &opAssign(const array<T>&in)", asFUNCTION(ScriptArrayAssignment_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "uint length() const", asFUNCTION(ScriptArrayLength_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void resize(uint)", asFUNCTION(ScriptArrayResize_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "uint get_length() const", asFUNCTION(ScriptArrayLength_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void set_length(uint)", asFUNCTION(ScriptArrayResize_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ScriptArrayGetRefCount_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ScriptArraySetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ScriptArrayGetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ScriptArrayEnumReferences_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ScriptArrayReleaseAllHandles_Generic), asCALL_GENERIC); assert( r >= 0 );
}

END_AS_NAMESPACE
