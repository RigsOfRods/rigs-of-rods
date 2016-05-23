#ifndef SCRIPTHELPER_H
#define SCRIPTHELPER_H

#ifndef ANGELSCRIPT_H
// Avoid having to inform include path if header is already include before
#include <angelscript.h>
#endif


BEGIN_AS_NAMESPACE

// Compare relation between two objects of the same type
int CompareRelation(asIScriptEngine *engine, void *lobj, void *robj, int typeId, int &result);

// Compare equality between two objects of the same type
int CompareEquality(asIScriptEngine *engine, void *lobj, void *robj, int typeId, bool &result);

// Compile and execute simple statements
// The module is optional. If given the statements can access the entities compiled in the module.
// The caller can optionally provide its own context, for example if a context should be reused.
int ExecuteString(asIScriptEngine *engine, const char *code, asIScriptModule *mod = 0, asIScriptContext *ctx = 0);

// Write the registered application interface to a file for an offline compiler.
// The format is compatible with the offline compiler in /sdk/samples/asbuild/.
int WriteConfigToFile(asIScriptEngine *engine, const char *filename);

// Print details of the script exception to the standard output
void PrintException(asIScriptContext *ctx, bool printStack = false);

// Determine traits of a type for registration of value types
// Relies on C++11 features so it can not be used with non-compliant compilers
#if !defined(_MSC_VER) || _MSC_VER >= 1700   // MSVC 2012
#if !defined(__GNUC__) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)  // gnuc 4.7
END_AS_NAMESPACE
#include <type_traits>
BEGIN_AS_NAMESPACE

template<typename T>
asUINT GetTypeTraits()
{
	bool hasConstructor =  std::is_default_constructible<T>::value && !std::has_trivial_default_constructor<T>::value;
	bool hasDestructor = std::is_destructible<T>::value && !std::has_trivial_destructor<T>::value;
	bool hasAssignmentOperator = std::is_copy_assignable<T>::value && !std::has_trivial_copy_assign<T>::value;
	bool hasCopyConstructor = std::is_copy_constructible<T>::value && !std::has_trivial_copy_constructor<T>::value;
	bool isFloat = std::is_floating_point<T>::value;
	bool isPrimitive = std::is_integral<T>::value || std::is_pointer<T>::value || std::is_enum<T>::value;

	if( isFloat )
		return asOBJ_APP_FLOAT;
	if( isPrimitive )
		return asOBJ_APP_PRIMITIVE;

	asDWORD flags = asOBJ_APP_CLASS;
	if( hasConstructor )
		flags |= asOBJ_APP_CLASS_CONSTRUCTOR;
	if( hasDestructor )
		flags |= asOBJ_APP_CLASS_DESTRUCTOR;
	if( hasAssignmentOperator )
		flags |= asOBJ_APP_CLASS_ASSIGNMENT;
	if( hasCopyConstructor )
		flags |= asOBJ_APP_CLASS_COPY_CONSTRUCTOR;
	return flags;
}
#endif // gnuc 4.7
#endif // msvc 2012

END_AS_NAMESPACE

#endif
