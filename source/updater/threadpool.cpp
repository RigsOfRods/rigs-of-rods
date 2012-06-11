#include "threadpool.h"
#include "wsyncdownload.h"
#include "utils.h"
#include "installerlog.h"
#include "wthread.h"
#include <wx/msgdlg.h>

using namespace std;

//// WsyncJob
WsyncJob::WsyncJob() : mCmd(eID_THREAD_NULL)
{
}

WsyncJob::WsyncJob(
	int num,
	job_commands cmd,
	const wxString& localFile,
	const wxString& server,
	const wxString& remoteDir,
	const wxString& remoteFile,
	const wxString& hashRemoteFile) :
		mNum(num),
		mCmd(cmd),
		mLocalFile(localFile),
		mServer(server),
		mRemoteDir(remoteDir),
		mRemoteFile(remoteFile),
		mHashRemoteFile(hashRemoteFile)
{
}


//// ThreadQueue

ThreadQueue::ThreadQueue(wxEvtHandler* pParent) : m_pParent(pParent)
{
}

void ThreadQueue::addJob(const WsyncJob& job, const ThreadQueue::job_priority& priority) // push a job with given priority class onto the FIFO
{
	wxMutexLocker lock(m_MutexQueue); // lock the queue
	m_Jobs.insert(std::pair<job_priority, WsyncJob>(priority, job)); // insert the prioritized entry into the multimap
		m_QueueCount.Post(); // new job has arrived: increment semaphore counter
}

WsyncJob ThreadQueue::Pop()
{
	WsyncJob element;
	m_QueueCount.Wait(); // wait for semaphore (=queue count to become positive)
	m_MutexQueue.Lock(); // lock queue
	element=(m_Jobs.begin())->second; // get the first entry from queue (higher priority classes come first)
	m_Jobs.erase(m_Jobs.begin()); // erase it
	m_MutexQueue.Unlock(); // unlock queue
	return element; // return job entry
}

void ThreadQueue::Report(const WsyncJob::job_commands& cmd, const wxString& sArg, int iArg) // report back to parent
{
	wxCommandEvent evt(wxEVT_THREAD, cmd); // create command event object
	evt.SetString(sArg); // associate string with it
	evt.SetInt(iArg);
	m_pParent->AddPendingEvent(evt); // and add it to parent's event queue
}

size_t ThreadQueue::Stacksize() // helper function to return no of pending jobs
{
	wxMutexLocker lock(m_MutexQueue); // lock queue until the size has been read
	return m_Jobs.size();
}

//// WsyncWorkerThread
WsyncWorkerThread::WsyncWorkerThread(ThreadQueue* pQueue, int id, wxEvtHandler *handler) : m_pQueue(pQueue), m_ID(id), m_handler(handler)
{
	assert(pQueue);
	wxThread::Create();
}


wxThread::ExitCode WsyncWorkerThread::Entry()
{
	WsyncJob::job_commands iErr;
	m_pQueue->Report(WsyncJob::eID_THREAD_STARTED, wxEmptyString, m_ID); // tell main thread that worker thread has successfully started
	try
	{
		// this is the main loop: process jobs until a job handler throws
		while(true)
		{
			OnJob();
		}

	} catch(WsyncJob::job_commands& i)
	{
		// catch return value from error condition
		m_pQueue->Report(iErr=i, wxEmptyString, m_ID);
	}
	m_pQueue->Report(WsyncJob::eID_THREAD_EXIT, wxEmptyString, m_ID); // tell main thread that worker thread has successfully exited
	return (wxThread::ExitCode)iErr; // and return exit code
}

void WsyncWorkerThread::updateCallback(int jobID, int type, std::string txt, float percent)
{
	if(!m_handler) return;

	// send event
	MyStatusEvent ev(MyStatusCommandEvent, type);
	ev.SetInt(jobID);
	ev.SetString(wxString(txt.c_str(), wxConvUTF8));
	ev.SetProgress(percent);
	m_handler->AddPendingEvent(ev);
}

int WsyncWorkerThread::downloadFile(WsyncJob job)
{
	WsyncDownload *wsdl = new WsyncDownload(m_handler);
	int retrycount=0;
	std::string server_use = conv(job.getServer());
	std::string dir_use = conv(job.getRemoteDir());
	std::string localFile = conv(job.getLocalFile());
	std::string remoteFile = conv(job.getRemoteFile());
	std::string hashRemote = conv(job.getHashRemoteFile());
	int jobID = job.getJobNumber();
retry:
	updateCallback(jobID, MSE_DOWNLOAD_START);
	std::string url = dir_use + remoteFile;
	int stat = wsdl->downloadFile(jobID, localFile, server_use, url);
	if(stat != 0 && retrycount < retrylimit)
	{
		LOG("DLFile-%04d|  result: %d, retrycount: %d, falling back to main server\n", jobID, stat, retrycount);
		// fallback to main server (for this single file only!)
		//printf("falling back to main server.\n");
		server_use = WSYNC_MAIN_SERVER;
		dir_use = WSYNC_MAIN_DIR;
		retrycount++;
		goto retry;
	}
	if(stat)
	{
		LOG("DLFile-%04d|  unable to create file: %s\n", jobID, localFile.c_str());
		updateCallback(jobID, MSE_ERROR, "unable to create file: " + localFile + "\nPlease ensure that you have the permissions to modify the file and\nthat it is not used by another process.");
	} else
	{
		string checkHash = WsyncThread::generateFileHash(localFile);
		if(hashRemote == checkHash)
		{
			LOG("DLFile-%04d| file hash ok\n", jobID);
		} else
		{
			LOG("DLFile-%04d| file hash wrong, is: '%s', should be: '%s'\n", jobID, checkHash.c_str(), hashRemote.c_str());
			WsyncDownload::tryRemoveFile(localFile);
			if(retrycount < retrylimit)
			{
				// fallback to main server!
				//printf(" hash wrong, falling back to main server.\n");
				//printf(" probably the mirror is not in sync yet\n");
				server_use = WSYNC_MAIN_SERVER;
				dir_use = WSYNC_MAIN_DIR;
				retrycount++;
				goto retry;
			}
		}
	}
	if(retrycount >= retrylimit)
	{
		wxString msg = wxString::Format(_T("failed to download file: \n%s\nPlease ensure that you have internet access."),  conv(remoteFile));
		updateCallback(jobID, MSE_ERROR, conv(msg), 0);
		wxMessageBox(msg, _T("Error"), wxICON_ERROR | wxOK);
		exit(1);
	}
	updateCallback(jobID, MSE_DOWNLOAD_DONE);
	LOG("DLFile-%04d| file ok\n", jobID);
	delete(wsdl);
	updateCallback(jobID, MSE_DOWNLOAD_DONE);
	return 0;
}

void WsyncWorkerThread::OnJob()
{
	if(m_pQueue->Stacksize() == 0)
	{
		// done
		throw WsyncJob::eID_THREAD_EXIT; // report exit of worker thread
	}
	WsyncJob job = m_pQueue->Pop(); // pop a job from the queue. this will block the worker thread if queue is empty
	switch(job.getCommand())
	{
	case WsyncJob::eID_THREAD_EXIT: // thread should exit
		throw WsyncJob::eID_THREAD_EXIT; // confirm exit command
	case WsyncJob::eID_THREAD_JOB:
	{
		// process a standard job
		int res = this->downloadFile(job);
		//m_pQueue->Report(WsyncJob::eID_THREAD_JOB, wxString::Format(wxT("Job #%s done."), job.getRemoteFile().c_str()), m_ID); // report successful completion
		break;
	}
	case WsyncJob::eID_THREAD_JOBERR: // process a job that terminates with an error
		m_pQueue->Report(WsyncJob::eID_THREAD_JOB, wxString::Format(wxT("Job #%s errorneous."), job.getRemoteFile().c_str()), m_ID);
		Sleep(1000);
		throw WsyncJob::eID_THREAD_EXIT; // report exit of worker thread
		break;
	case WsyncJob::eID_THREAD_NULL: // dummy command
	default:
		break;
	}
}

//// WsyncDownloadManager
WsyncDownloadManager::WsyncDownloadManager() : m_pQueue(NULL), m_parent(NULL)
{
}

WsyncDownloadManager::WsyncDownloadManager(wxEvtHandler *parent) : m_pQueue(new ThreadQueue(this)), m_parent(parent)
{
}

WsyncDownloadManager::~WsyncDownloadManager()
{
	destroyThreads();
	delete m_pQueue;
}

void WsyncDownloadManager::checkThreads()
{
	if(m_Threads.size() < threadCounter && m_pQueue->Stacksize() > m_Threads.size())
	{
		// too less threads, start one ...
		int id = m_Threads.empty()?1:m_Threads.back() + 1;
		m_Threads.push_back(id);
		WsyncWorkerThread* pThread = new WsyncWorkerThread(m_pQueue, id, m_parent); // create a new worker thread, increment thread counter (this implies, thread will start OK)
		pThread->Run();
	}
}

void WsyncDownloadManager::addJob(int num, wxString localFile, wxString remoteDir, wxString remoteServer, wxString remoteFile, wxString hashRemoteFile)
{
	m_pQueue->addJob(WsyncJob(num, WsyncJob::eID_THREAD_JOB, localFile, remoteServer, remoteDir, remoteFile, hashRemoteFile));

	// start the threads if not already done
	checkThreads();
}

void WsyncDownloadManager::onThread(wxCommandEvent& event) // handler for thread notifications
{
	int jobID = event.GetInt();
	switch(event.GetId())
	{
		case WsyncJob::eID_THREAD_JOB:
			LOG("DLFile-WSDLM| eID_THREAD_JOB %d\n", jobID);
			//wxMessageBox(wxString::Format(wxT("[%i]: %s"), event.GetInt(), event.GetString()));
			break;
		case WsyncJob::eID_THREAD_EXIT:
			LOG("DLFile-WSDLM| eID_THREAD_EXIT %d\n", jobID);
			//wxMessageBox(wxString::Format(wxT("[%i]: Stopped."), event.GetInt()));
			m_Threads.remove(jobID); // thread has exited: remove thread ID from list
			break;
		case WsyncJob::eID_THREAD_STARTED:
			LOG("DLFile-WSDLM| eID_THREAD_STARTED %d\n", jobID);
			//wxMessageBox(wxString::Format(wxT("[%i]: Ready."), event.GetInt()));
			break;
		default:
			LOG("DLFile-WSDLM| IDK %d - %d\n", event.GetId(), jobID);
			//wxMessageBox(wxString::Format(wxT("[%i]: IDK."), event.GetInt()));
			event.Skip();
	}
}

bool WsyncDownloadManager::isDone()
{
	return m_Threads.empty();
}

void WsyncDownloadManager::destroyThreads()
{
	if(m_Threads.empty())
		return;
	for(size_t t=0; t<m_Threads.size(); ++t)
	{
		m_pQueue->addJob(WsyncJob(0, WsyncJob::eID_THREAD_EXIT, wxEmptyString, wxEmptyString, wxEmptyString, wxEmptyString, wxEmptyString), ThreadQueue::eHIGHEST); // send all running threads the "EXIT" signal
	}
}

//// EVENTS
IMPLEMENT_DYNAMIC_CLASS(WsyncDownloadManager, wxEvtHandler)

DEFINE_EVENT_TYPE(wxEVT_THREAD)

//// EVENT TABLE

BEGIN_EVENT_TABLE(WsyncDownloadManager, wxEvtHandler)
  EVT_COMMAND(wxID_ANY, wxEVT_THREAD, WsyncDownloadManager::onThread)
END_EVENT_TABLE()
