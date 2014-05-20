
//
// CSerializer
//
// This code was based on the CScriptReloader written by FDsagizi
// http://www.gamedev.net/topic/604890-dynamic-reloading-script/
//

#include <assert.h>
#include <string.h> // strstr
#include <stdio.h>  // sprintf
#include "serializer.h"

using namespace std;

BEGIN_AS_NAMESPACE

///////////////////////////////////////////////////////////////////////////////////

CSerializer::CSerializer()
{
	m_engine = 0;
}

CSerializer::~CSerializer()
{
	// Clean the serialized values before we remove the user types
	m_root.Uninit();

	// Delete the user types
	std::map<std::string, CUserType*>::iterator it;
	for( it = m_userTypes.begin(); it != m_userTypes.end(); it++  )
		delete it->second;

	if( m_engine )
		m_engine->Release();
}

void CSerializer::AddUserType(CUserType *ref, const std::string &name)
{
	m_userTypes[name] = ref;
}

int CSerializer::Store(asIScriptModule *mod)
{
	m_mod = mod;

	// The engine must not be destroyed before we're completed, so we'll hold on to a reference
	mod->GetEngine()->AddRef();
	if( m_engine ) m_engine->Release();
	m_engine = mod->GetEngine();

	m_root.m_serializer = this;

	// First store global variables
	for( asUINT i = 0; i < mod->GetGlobalVarCount(); i++ )
	{
		const char *name;
		int typeId;
		mod->GetGlobalVar(i, &name, &typeId);
		m_root.m_children.push_back(new CSerializedValue(&m_root, name, mod->GetAddressOfGlobalVar(i), typeId));
	}

	// For the handles that were stored, we need to substitute the stored pointer
	// that is still pointing to the original object to an internal reference so
	// it can be restored later on.
	m_root.ReplaceHandles();

	return 0;
}

// Retrieve all global variables after reload script.
int CSerializer::Restore(asIScriptModule *mod)
{
	m_mod = mod;

	// The engine must not be destroyed before we're completed, so we'll hold on to a reference
	mod->GetEngine()->AddRef();
	if( m_engine ) m_engine->Release();
	m_engine = mod->GetEngine();

	// First restore the global variables
	asUINT varCount = mod->GetGlobalVarCount();
	for( asUINT i = 0; i < varCount; i++ )
	{
		const char *name;
		int typeId;
		mod->GetGlobalVar(i, &name, &typeId);

		CSerializedValue *v = m_root.FindByName(name);
		if( v )
			v->Restore(mod->GetAddressOfGlobalVar(i), typeId);
	}

	// The handles that were restored needs to be
	// updated to point to their final objects.
	m_root.RestoreHandles();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////

CSerializedValue::CSerializedValue()
{
	Init();
}

CSerializedValue::CSerializedValue(CSerializedValue *parent, const std::string &name, void *ref, int typeId)
{
	Init();

	m_name       = name;
	m_serializer = parent->m_serializer;
	Store(ref, typeId);
}

void CSerializedValue::Init()
{
	m_handlePtr  = 0;
	m_restorePtr = 0;
	m_typeId     = 0;
	m_isInit     = false;
	m_serializer = 0;
	m_userData   = 0;
}

void CSerializedValue::Uninit()
{
	m_isInit = false;

	ClearChildren();

	if( m_userData )
	{
		CUserType *type = m_serializer->m_userTypes[m_typeName];
		if( type )
			type->CleanupUserData(this);
		m_userData = 0;
	}
}

void CSerializedValue::ClearChildren()
{
	// If this value is for an object handle that created an object during the restore
	// then it is necessary to release the handle here, so we won't get a memory leak
	if( (m_typeId & asTYPEID_OBJHANDLE) && m_children.size() == 1 && m_children[0]->m_restorePtr )
	{
		m_serializer->m_engine->ReleaseScriptObject(m_children[0]->m_restorePtr, m_children[0]->m_typeId);
	}

	for( size_t n = 0; n < m_children.size(); n++ )
		delete m_children[n];
	m_children.clear();
}

CSerializedValue::~CSerializedValue()
{
	Uninit();
}

CSerializedValue *CSerializedValue::FindByName(const std::string &name)
{
	for( size_t i = 0; i < m_children.size(); i++ )
		if( m_children[i]->m_name == name )
			return m_children[i];

	return 0;
}

void  CSerializedValue::GetAllPointersOfChildren(std::vector<void*> *ptrs)
{
	ptrs->push_back(m_ptr);

	for( size_t i = 0; i < m_children.size(); ++i )
		m_children[i]->GetAllPointersOfChildren(ptrs);
}

CSerializedValue *CSerializedValue::FindByPtr(void *ptr)
{
	if( m_ptr == ptr )
		return this;

	for( size_t i = 0; i < m_children.size(); i++ )
	{
		CSerializedValue *find = m_children[i]->FindByPtr(ptr);
		if( find )
			return find;
	}

	return 0;
}

void *CSerializedValue::GetPointerToRestoredObject(void *ptr)
{
	if( m_ptr == ptr )
		return m_restorePtr;

	for( size_t i = 0; i < m_children.size(); ++i )
	{
		void *ret = m_children[i]->GetPointerToRestoredObject(ptr);
		if( ret )
			return ret;
	}

	return 0;
}

// find variable by ptr but looking only at those in the references, which will create a new object
CSerializedValue *CSerializedValue::FindByPtrInHandles(void *ptr)
{
	// if this handle created object
	if( (m_typeId & asTYPEID_OBJHANDLE) && m_children.size() == 1 )
	{
		if( m_children[0]->m_ptr == ptr )
			return this;
	}

	if( !(m_typeId & asTYPEID_OBJHANDLE) )
	{
		for( size_t i = 0; i < m_children.size(); i++ )
		{
			CSerializedValue *find = m_children[i]->FindByPtrInHandles(ptr);
			if( find )
				return find;
		}
	}

	return 0;
}

void CSerializedValue::Store(void *ref, int typeId)
{
	m_isInit = true;
	SetType(typeId);
	m_ptr = ref;

	if( m_typeId & asTYPEID_OBJHANDLE )
	{
		m_handlePtr = *(void**)ref;
	}
	else if( m_typeId & asTYPEID_SCRIPTOBJECT )
	{
		asIScriptObject *obj = (asIScriptObject *)ref;
		asIObjectType *type = obj->GetObjectType();
		SetType(type->GetTypeId());

		// Store children
		for( asUINT i = 0; i < type->GetPropertyCount(); i++ )
		{	
			int childId;
			const char *childName;
			type->GetProperty(i, &childName, &childId);

			m_children.push_back(new CSerializedValue(this, childName, obj->GetAddressOfProperty(i), childId));
		}	
	}
	else
	{
		int size = m_serializer->m_engine->GetSizeOfPrimitiveType(m_typeId);
		
		if( size == 0 )
		{			
			// if it is user type( string, array, etc ... )
			if( m_serializer->m_userTypes[m_typeName] )
				m_serializer->m_userTypes[m_typeName]->Store(this, m_ptr);
			
			// it is script class
			else if( GetType() )
				size = GetType()->GetSize();	
		}

		if( size )
		{
			m_mem.resize(size);
			memcpy(&m_mem[0], ref, size);
		}
	}
}

void CSerializedValue::Restore(void *ref, int typeId)
{
	if( !this || !m_isInit || !ref )
		return;

	// Verify that the stored type matched the new type of the value being restored
	if( typeId <= asTYPEID_DOUBLE && typeId != m_typeId ) return; // TODO: We may try to do a type conversion for primitives
	if( (typeId & ~asTYPEID_MASK_SEQNBR) ^ (m_typeId & ~asTYPEID_MASK_SEQNBR) ) return;
	asIObjectType *type = m_serializer->m_engine->GetObjectTypeById(typeId);
	if( type && m_typeName != type->GetName() ) return;

	// Set the new pointer and type
	m_restorePtr = ref;
	SetType(typeId);

	// Restore the value
	if( m_typeId & asTYPEID_OBJHANDLE )
	{
		// if need create objects
		if( m_children.size() == 1 )
		{
			asIObjectType *type = m_children[0]->GetType();

			void *newObject = m_serializer->m_engine->CreateScriptObject(type->GetTypeId());

			m_children[0]->Restore(newObject, type->GetTypeId());	
		}
	}
	else if( m_typeId & asTYPEID_SCRIPTOBJECT )
	{
		asIScriptObject *obj = (asIScriptObject *)ref;
		asIObjectType *type = GetType();

		// Retrieve children
		for( asUINT i = 0; i < type->GetPropertyCount() ; i++ )
		{	
			const char *nameProperty;
			int typeId;
			type->GetProperty(i, &nameProperty, &typeId);
			
			CSerializedValue *var = FindByName(nameProperty);
			if( var )
				var->Restore(obj->GetAddressOfProperty(i), typeId);
		}
	}
	else
	{
		if( m_mem.size() )
			memcpy(ref, &m_mem[0], m_mem.size());
		
		// user type restore
		else if( m_serializer->m_userTypes[m_typeName] )
			m_serializer->m_userTypes[m_typeName]->Restore(this, m_restorePtr);
	}
}

void CSerializedValue::CancelDuplicates(CSerializedValue *from)
{
	std::vector<void*> ptrs;
	from->GetAllPointersOfChildren(&ptrs);

	for( size_t i = 0; i < ptrs.size(); ++i )
	{
		CSerializedValue *find = m_serializer->m_root.FindByPtrInHandles(ptrs[i]);

		while( find )
		{
			// cancel create object
			find->ClearChildren();

			// Find next link to this ptr
			find = m_serializer->m_root.FindByPtrInHandles(ptrs[i]);
		}
	}
}

void CSerializedValue::ReplaceHandles()
{
	if( m_handlePtr )
	{
		// Find the object that the handle is referring to
		CSerializedValue *handle_to = m_serializer->m_root.FindByPtr(m_handlePtr);
		
		// If the object hasn't been stored yet...
		if( handle_to == 0 )
		{
			// Store the object now
			asIObjectType *type = GetType();
			CSerializedValue *need_create = new CSerializedValue(this, m_name, m_handlePtr, type->GetTypeId());

			// Make sure all other handles that point to the same object
			// are updated, so we don't end up creating duplicates
			CancelDuplicates(need_create);

			m_children.push_back(need_create);
		}
	}

	// Replace the handles in the children too
	for( size_t i = 0; i < m_children.size(); ++i )
		m_children[i]->ReplaceHandles();
}

void CSerializedValue::RestoreHandles()
{
	if( m_typeId & asTYPEID_OBJHANDLE )
	{
		if( m_handlePtr )
		{
			// Find the object the handle is supposed to point to
			CSerializedValue *handleTo = m_serializer->m_root.FindByPtr(m_handlePtr);

			if( m_restorePtr && handleTo && handleTo->m_restorePtr )
			{
				// If the handle is already pointing to something it must be released first
				if( *(void**)m_restorePtr )
					m_serializer->m_engine->ReleaseScriptObject(*(void**)m_restorePtr, m_typeId);

				// Update the internal pointer
				*(void**)m_restorePtr = handleTo->m_restorePtr;

				// Increase the reference
				m_serializer->m_engine->AddRefScriptObject(handleTo->m_restorePtr, m_typeId);
			}
		}
		else
		{
			// If the handle is pointing to something, we must release it to restore the null pointer
			if( m_restorePtr && *(void**)m_restorePtr )
			{
				m_serializer->m_engine->ReleaseScriptObject(*(void**)m_restorePtr, m_typeId);
				*(void**)m_restorePtr = 0;
			}
		}
	}

	// Do the same for the children
	for( size_t i = 0; i < m_children.size(); ++i )
		m_children[i]->RestoreHandles();
}

void CSerializedValue::SetType(int typeId)
{
	m_typeId = typeId;

	asIObjectType *type = m_serializer->m_engine->GetObjectTypeById(typeId);

	if( type )
		m_typeName = type->GetName();
}

asIObjectType *CSerializedValue::GetType()
{
	if( !m_typeName.empty() )
	{
		int newTypeId = m_serializer->m_mod->GetTypeIdByDecl(m_typeName.c_str());
		return m_serializer->m_engine->GetObjectTypeById(newTypeId);
	}	

	return 0;
}

void CSerializedValue::SetUserData(void *data)
{
	m_userData = data;
}

void *CSerializedValue::GetUserData()
{
	return m_userData;
}

END_AS_NAMESPACE
