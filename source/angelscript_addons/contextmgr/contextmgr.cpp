#include <assert.h>
#include <string>

#include "contextmgr.h"

using namespace std;

// TODO: Should have a pool of free asIScriptContext so that new contexts
//       won't be allocated every time. The application must not keep 
//       its own references, instead it must tell the context manager
//       that it is using the context. Otherwise the context manager may
//       think it can reuse the context too early.

// TODO: Need to have a callback for when scripts finishes, so that the 
//       application can receive return values.

BEGIN_AS_NAMESPACE

struct SContextInfo
{
    asUINT sleepUntil;
	vector<asIScriptContext*> coRoutines;
	asUINT currentCoRoutine;
};

static CContextMgr *g_ctxMgr = 0;
static void ScriptSleep(asUINT milliSeconds)
{
	// Get a pointer to the context that is currently being executed
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && g_ctxMgr )
	{
		// Suspend its execution. The VM will continue until the current 
		// statement is finished and then return from the Execute() method
		ctx->Suspend();

		// Tell the context manager when the context is to continue execution
		g_ctxMgr->SetSleeping(ctx, milliSeconds);
	}
}

static void ScriptYield()
{
	// Get a pointer to the context that is currently being executed
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && g_ctxMgr )
	{
		// Let the context manager know that it should run the next co-routine
		g_ctxMgr->NextCoRoutine();

		// The current context must be suspended so that VM will return from 
		// the Execute() method where the context manager will continue.
		ctx->Suspend();
	}
}

// TODO: Use function pointers to receive the function that should be called
void ScriptCreateCoRoutine(string &func, CScriptAny *arg)
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && g_ctxMgr )
	{
		asIScriptEngine *engine = ctx->GetEngine();
		string mod = ctx->GetFunction()->GetModuleName();

		// We need to find the function that will be created as the co-routine
		string decl = "void " + func + "(any @)"; 
		asIScriptFunction *func = engine->GetModule(mod.c_str())->GetFunctionByDecl(decl.c_str());
		if( func == 0 )
		{
			// No function could be found, raise an exception
			ctx->SetException(("Function '" + decl + "' doesn't exist").c_str());
			return;
		}

		// Create a new context for the co-routine
		asIScriptContext *coctx = g_ctxMgr->AddContextForCoRoutine(ctx, func);

		// Pass the argument to the context
		coctx->SetArgObject(0, arg);
	}
}

CContextMgr::CContextMgr()
{
    m_getTimeFunc   = 0;
	m_currentThread = 0;

	m_numExecutions         = 0;
	m_numGCObjectsCreated   = 0;
	m_numGCObjectsDestroyed = 0;
}

CContextMgr::~CContextMgr()
{
	asUINT n;

	// Free the memory
	for( n = 0; n < m_threads.size(); n++ )
	{
		if( m_threads[n] )
		{
			for( asUINT c = 0; c < m_threads[n]->coRoutines.size(); c++ )
			{
				if( m_threads[n]->coRoutines[c] )
					m_threads[n]->coRoutines[c]->Release();
			}

			delete m_threads[n];
		}
	}

	for( n = 0; n < m_freeThreads.size(); n++ )
	{
		if( m_freeThreads[n] )
		{
			assert( m_freeThreads[n]->coRoutines.size() == 0 );

			delete m_freeThreads[n];
		}
	}
}

void CContextMgr::ExecuteScripts()
{
	g_ctxMgr = this; 

	// TODO: Should have an optional time out for this function. If not all scripts executed before the 
	//       time out, the next time the function is called the loop should continue
	//       where it left off.

	// TODO: There should be a time out per thread as well. If a thread executes for too 
	//       long, it should be aborted. A group of co-routines count as a single thread.

	// Check if the system time is higher than the time set for the contexts
	asUINT time = m_getTimeFunc ? m_getTimeFunc() : asUINT(-1);
	for( m_currentThread = 0; m_currentThread < m_threads.size(); m_currentThread++ )
	{
		SContextInfo *thread = m_threads[m_currentThread];
		if( thread->sleepUntil < time )
		{
			int currentCoRoutine = thread->currentCoRoutine;

			// Gather some statistics from the GC
			asIScriptEngine *engine = thread->coRoutines[currentCoRoutine]->GetEngine();
			asUINT gcSize1, gcSize2, gcSize3;
			engine->GetGCStatistics(&gcSize1);

			// Execute the script for this thread and co-routine
			int r = thread->coRoutines[currentCoRoutine]->Execute();

			// Determine how many new objects were created in the GC
			engine->GetGCStatistics(&gcSize2);
			m_numGCObjectsCreated += gcSize2 - gcSize1;
			m_numExecutions++;

			if( r != asEXECUTION_SUSPENDED )
			{
				// TODO: It should be possible to retrieve the return value before the context is released.
				//       Maybe by calling a callback, or by storing the context somewhere until it has been
				//       accessed.

				// The context has terminated execution (for one reason or other)
				thread->coRoutines[currentCoRoutine]->Release();
				thread->coRoutines[currentCoRoutine] = 0;

				thread->coRoutines.erase(thread->coRoutines.begin() + thread->currentCoRoutine);
				if( thread->currentCoRoutine >= thread->coRoutines.size() )
					thread->currentCoRoutine = 0;

				// If this was the last co-routine terminate the thread
				if( thread->coRoutines.size() == 0 )
				{
					m_freeThreads.push_back(thread);
					m_threads.erase(m_threads.begin() + m_currentThread);
					m_currentThread--;
				}
			}

			// Destroy all known garbage if any new objects were created
			if( gcSize2 > gcSize1 )
			{
				engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE);

				// Determine how many objects were destroyed
 				engine->GetGCStatistics(&gcSize3);
				m_numGCObjectsDestroyed += gcSize3 - gcSize2;
			}

			// TODO: If more objects are created per execution than destroyed on average
			//       then it may be necessary to run more iterations of the detection of
			//       cyclic references. At the startup of an application there is usually
			//       a lot of objects created that will live on through out the application
			//       so the average number of objects created per execution will be higher
			//       than the number of destroyed objects in the beginning, but afterwards
			//       it usually levels out to be more or less equal.

			// Just run an incremental step for detecting cyclic references
			engine->GarbageCollect(asGC_ONE_STEP | asGC_DETECT_GARBAGE);
		}
	}

	g_ctxMgr = 0;
}

void CContextMgr::NextCoRoutine()
{
	m_threads[m_currentThread]->currentCoRoutine++;
	if( m_threads[m_currentThread]->currentCoRoutine >= m_threads[m_currentThread]->coRoutines.size() )
		m_threads[m_currentThread]->currentCoRoutine = 0;
}

void CContextMgr::AbortAll()
{
	// Abort all contexts and release them. The script engine will make 
	// sure that all resources held by the scripts are properly released.

	for( asUINT n = 0; n < m_threads.size(); n++ )
	{
		for( asUINT c = 0; c < m_threads[n]->coRoutines.size(); c++ )
		{
			if( m_threads[n]->coRoutines[c] )
			{
				m_threads[n]->coRoutines[c]->Abort();
				m_threads[n]->coRoutines[c]->Release();
				m_threads[n]->coRoutines[c] = 0;
			}
		}
		m_threads[n]->coRoutines.resize(0);

		m_freeThreads.push_back(m_threads[n]);
	}

	m_threads.resize(0);

	m_currentThread = 0;
}

asIScriptContext *CContextMgr::AddContext(asIScriptEngine *engine, asIScriptFunction *func)
{
	// Create the new context
	asIScriptContext *ctx = engine->CreateContext();
	if( ctx == 0 )
		return 0;

	// Prepare it to execute the function
	int r = ctx->Prepare(func);
	if( r < 0 )
	{
		ctx->Release();
		return 0;
	}

	// Add the context to the list for execution
	SContextInfo *info = 0;
	if( m_freeThreads.size() > 0 )
	{
		info = *m_freeThreads.rbegin();
		m_freeThreads.pop_back();
	}
	else
	{
		info = new SContextInfo;
	}

    info->coRoutines.push_back(ctx);
	info->currentCoRoutine = 0;
    info->sleepUntil = 0;
	m_threads.push_back(info);

	return ctx;
}

asIScriptContext *CContextMgr::AddContextForCoRoutine(asIScriptContext *currCtx, asIScriptFunction *func)
{
	asIScriptEngine *engine = currCtx->GetEngine();
	asIScriptContext *coctx = engine->CreateContext();
	if( coctx == 0 )
	{
		return 0;
	}

	// Prepare the context
	int r = coctx->Prepare(func);
	if( r < 0 )
	{
		// Couldn't prepare the context
		coctx->Release();
		return 0;
	}

	// Find the current context thread info
	// TODO: Start with the current thread so that we can find the group faster
	for( asUINT n = 0; n < m_threads.size(); n++ )
	{
		if( m_threads[n]->coRoutines[m_threads[n]->currentCoRoutine] == currCtx )
		{
			// Add the coRoutine to the list
			m_threads[n]->coRoutines.push_back(coctx);
		}
	}

	return coctx; 
}

void CContextMgr::SetSleeping(asIScriptContext *ctx, asUINT milliSeconds)
{
    assert( m_getTimeFunc != 0 );
    
	// Find the context and update the timeStamp  
	// for when the context is to be continued

	// TODO: Start with the current thread

	for( asUINT n = 0; n < m_threads.size(); n++ )
	{
		if( m_threads[n]->coRoutines[m_threads[n]->currentCoRoutine] == ctx )
		{
			m_threads[n]->sleepUntil = (m_getTimeFunc ? m_getTimeFunc() : 0) + milliSeconds;
		}
	}
}

void CContextMgr::RegisterThreadSupport(asIScriptEngine *engine)
{
	int r;

    // Must set the get time callback function for this to work
    assert( m_getTimeFunc != 0 );
    
    // Register the sleep function
    r = engine->RegisterGlobalFunction("void sleep(uint)", asFUNCTION(ScriptSleep), asCALL_CDECL); assert( r >= 0 );

	// TODO: Add support for spawning new threads, waiting for signals, etc
}

void CContextMgr::RegisterCoRoutineSupport(asIScriptEngine *engine)
{
	int r;

	r = engine->RegisterGlobalFunction("void yield()", asFUNCTION(ScriptYield), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void createCoRoutine(const string &in, any @+)", asFUNCTION(ScriptCreateCoRoutine), asCALL_CDECL); assert( r >= 0 );
}

void CContextMgr::SetGetTimeCallback(TIMEFUNC_t func)
{
	m_getTimeFunc = func;
}

END_AS_NAMESPACE
