#include "wsyncdownload.h"

#include <wx/thread.h>
#include <wx/event.h>
#include <string.h>
#include "Timer.h"
#include "installerlog.h"
#include "tokenize.h"
#include "utils.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <string>

#include <iostream>
#include <fstream>

using namespace boost::filesystem;
using namespace std;


std::map < std::string, boost::uintmax_t > WsyncDownload::traffic_stats;

WsyncDownload::WsyncDownload(wxEvtHandler* parent) : parent(parent)
{
	mDownloadMessage = _T("downloading file ...");
}

int progress_callback(void *data, double dltotal, double dlnow, double ultotal, double ulnow)
{
	job_info_t *jinfo = (job_info_t *)data;
	if(!jinfo) return 1;
	
	jinfo->realDownloadSize = dltotal;
	jinfo->downloadDoneSize = dlnow;

	jinfo->filesize_left = jinfo->realDownloadSize - jinfo->downloadDoneSize;
	
	double tdiff = (jinfo->speedTimer->elapsed() - jinfo->lastTime);
	if(tdiff > 0.1f)
	{
		// slow down the GUI updates
		if(jinfo->wxp)
		{
			jinfo->wxp->Update(1000*(((float)jinfo->downloadDoneSize)/((float)jinfo->realDownloadSize)), _T("downloading..."));
			::wxSafeYield(jinfo->wxp);
			::wxYield();
			jinfo->wxp->Refresh();
		}
		jinfo->parent->reportDownloadProgress(jinfo->jobID, *jinfo->dlStartTime, jinfo->realDownloadSize, jinfo->downloadDoneSize);
		jinfo->lastTime = jinfo->speedTimer->elapsed();
	}
	return 0;
}


int WsyncDownload::downloadFile(int jobID, boost::filesystem::path localFile, string server, string path, boost::uintmax_t predDownloadSize, boost::uintmax_t *fileSizeArg, bool showProgress)
{
	LOG("DLFile-%04d| starting: http://%s%s -> %s ... \n", jobID, server.c_str(), path.c_str(), localFile.string().c_str());

	// remove '//' and '///' from url
	cleanURL(path);

	job_info jinfo;
	jinfo.parent = this;
	jinfo.jobID = jobID;
	jinfo.showProgress = showProgress;
	jinfo.realDownloadSize = predDownloadSize;
	jinfo.predDownloadSize = predDownloadSize;
	jinfo.downloadDoneSize=0;
	jinfo.datacounter=0;
	jinfo.dataspeed=0;
	jinfo.filesize_left=0;
	jinfo.lastTime=-1;
	jinfo.dlStartTime = new Timer();
	jinfo.speedTimer = new Timer();
	jinfo.wxp = 0;
	if(showProgress)
	{
		jinfo.wxp = new wxProgressDialog(mDownloadMessage, conv(string("downloading http://") + server + path), 1000,0, wxPD_APP_MODAL|wxPD_AUTO_HIDE|wxPD_SMOOTH|wxPD_ELAPSED_TIME|wxPD_ESTIMATED_TIME|wxPD_REMAINING_TIME|wxPD_CAN_ABORT);
	}

	ensurePathExist(localFile);

	CURL *curl = curl_easy_init();
	if(!curl)
	{
		LOG("DLFile-%04d| error creating curl: %s\n", jobID, localFile.string().c_str());
		LOG("DLFile-%04d| download URL: http://%s%s\n", jobID, server.c_str(), path.c_str());
		if(jinfo.wxp) jinfo.wxp->Update(1000, wxT("error"));
		return 1;
	}

	string url = "http://" + server + path;
	FILE *outFile = fopen(localFile.string().c_str(), "wb");
	if(!outFile)
	{
		perror("error opening file");
		LOG("DLFile-%04d| error opening local file: %s.\n", jobID, localFile.string().c_str());
		LOG("DLFile-%04d| download URL: http://%s%s\n", jobID, server.c_str(), path.c_str());
		if(jinfo.wxp) jinfo.wxp->Update(1000, wxT("error"));	
	}

	char *curl_err_str[CURL_ERROR_SIZE];
	memset(curl_err_str, 0, CURL_ERROR_SIZE);

	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL,              url.c_str());
	
	// logging stuff
	curl_easy_setopt(curl, CURLOPT_STDERR,           InstallerLog::getSingleton()->getLogFilePtr());
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,      curl_err_str[0]);

	// progess stuff
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS,       0);
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA,     &jinfo);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &progress_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,        outFile);

	// http related settings
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,   1); // follow redirects
	curl_easy_setopt(curl, CURLOPT_AUTOREFERER,      1); // set the Referer: field in requests where it follows a Location: redirect.
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS,        20);
	curl_easy_setopt(curl, CURLOPT_USERAGENT,        "RoRInstaller");
	curl_easy_setopt(curl, CURLOPT_FILETIME,         1);
	

	// important for us later:
	//curl_easy_setopt(curl, CURLOPT_RANGE,        "...");
	
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	fclose(outFile);

	// print curl error if existing
	if(res != CURLE_OK)
	{
		LOG("DLFile-%04d| CURL returned: %d\n", jobID, res);
		LOG("DLFile-%04d| CURL error: %s\n", jobID, curl_err_str);
	}

	// check results
	boost::uintmax_t fileSize = file_size(localFile);
	if(jinfo.predDownloadSize != 0 && fileSize != jinfo.predDownloadSize)
	{
		LOG("DLFile-%04d| Error: file size is different: should be %d, is %d. removing file.\n", jobID, jinfo.realDownloadSize, (int)fileSize);
		LOG("DLFile-%04d| download URL: http://%s%s\n", jobID, server.c_str(), path.c_str());
		if(jinfo.wxp) jinfo.wxp->Update(1000, wxT("error"));

		tryRemoveFile(localFile, jobID);
		return 1;
	}

	// close dialog
	if(jinfo.wxp) jinfo.wxp->Update(1000, wxT("done"));

	// delete timers
	delete (jinfo.dlStartTime); jinfo.dlStartTime = 0;
	delete (jinfo.speedTimer); jinfo.speedTimer = 0;
	if(jinfo.wxp) { delete (jinfo.wxp); jinfo.wxp = 0; }

	// traffic stats tracking
	increaseServerStats(server, fileSize);

	if(fileSizeArg)
		*fileSizeArg = (int)fileSize;
	LOG("DLFile-%04d| download ok, %d bytes received\n", jobID, fileSize);

	return 0;
}


void WsyncDownload::reportDownloadProgress(int jobID, Timer dlStartTime, boost::uintmax_t predDownloadSize, boost::uintmax_t size_done)
{
	// this function will format and send some info to the gui
	unsigned int size_left = predDownloadSize - size_done;
	double tdiff = dlStartTime.elapsed();
	float speed = (float)(size_done / tdiff);
	float eta = size_left / speed;
	if(predDownloadSize == 0) eta = 0;
	float progress = ((float)size_done) /((float)predDownloadSize);

	// update parent
	updateCallback(jobID, MSE_DOWNLOAD_PROGRESS, "", progress);
	updateCallback(jobID, MSE_DOWNLOAD_TIME, "", tdiff);
	updateCallback(jobID, MSE_DOWNLOAD_TIME_LEFT, "", eta);
	updateCallback(jobID, MSE_DOWNLOAD_SPEED, "", speed);
	updateCallback(jobID, MSE_DOWNLOAD_DOWNLOADED, "", size_done);
}

void WsyncDownload::tryRemoveFile(boost::filesystem::path filename, int jobID)
{
	LOG("tryRemoveFile-%04d| removing file: %s ... TRYING ...\n", jobID, filename.string().c_str());
	try
	{
		remove(filename);
		LOG("tryRemoveFile-%04d| removing file: %s ... SUCCESSFULLY\n", jobID, filename.string().c_str());
	} catch(exception& e)
	{
		LOG("tryRemoveFile-%04d| removing file: %s ... ERROR: %s\n", jobID, filename.string().c_str(), e.what());
		wxMessageBox(_T("Unable to delete file: \n") + conv(filename.string()) + _T("\n\nError: ") + wxString(e.what()) + _T("\n\nPlease close RoR and all associated applications and try again later with admin priviledges."), _T("Update Error"),0);
		exit(1);
	}
}


void WsyncDownload::increaseServerStats(std::string server, boost::uintmax_t bytes)
{
	if(traffic_stats.find(server) == traffic_stats.end())
		traffic_stats[server] = 0;

	// add filesize to traffic stats
	traffic_stats[server] += bytes;
}

void WsyncDownload::updateCallback(int jobID, int type, std::string txt, float percent)
{
	if(!parent) return;

	// send event
	MyStatusEvent ev(MyStatusCommandEvent, type);
	ev.SetInt(jobID);
	ev.SetString(wxString(txt.c_str(), wxConvUTF8));
	ev.SetProgress(percent);
	parent->AddPendingEvent(ev);
}




int WsyncDownload::downloadAdvancedConfigFile(int jobID, std::string server, std::string url, std::vector< std::map< std::string, std::string > > &list, bool showProgress)
{
	path tempfile;
	if(getTempFilename(tempfile))
	{
		LOG("DLACFile-%04d| error creating tempfile!\n", jobID);
		return -1;
	}

	if(downloadFile(jobID, tempfile, server, url, 0, 0, showProgress))
	{
		LOG("DLACFile-%04d| error downloading file from %s, %s\n", jobID, server.c_str(), url.c_str());
		return -2;
	}
	ifstream fin(tempfile.string().c_str());
	if (fin.is_open() == false)
	{
		LOG("DLACFile-%04d| unable to open file for reading: %s\n", jobID, tempfile.string().c_str());
		return -3;
	}
	bool complete = false;
	string line = string();
	std::map < std::string, std::string > obj;
	while(getline(fin, line))
	{
		if(!line.size()) continue;
		if(line == "#end")
		{
			complete = true;
			continue;
		}
		if(line[0] == '#') continue;
		if(line[0] == '=')
		{
			// new stream
			if(obj.size() > 0)
				list.push_back(obj);
			obj.clear();
			continue;
		}
		std::vector<std::string> args = tokenize_str(line, "=");
		if(args.size() == 2)
		{
			boost::trim(args[0]);
			boost::trim(args[1]);
			obj[args[0]] = args[1];
		}
	}
	fin.close();
	tryRemoveFile(tempfile, jobID);

	if(!complete)
	{
		LOG("DLACFile-%04d| error downloading file from %s, %s\n", jobID, server.c_str(), url.c_str());
		return -4;
	}
	return 0;
}


int WsyncDownload::downloadConfigFile(int jobID, std::string server, std::string url, std::vector< std::vector< std::string > > &list, bool showProgress)
{
	list.clear();

	path tempfile;
	if(getTempFilename(tempfile))
	{
		LOG("DLCFile-%04d| error creating tempfile!\n", jobID);
		return -1;
	}

	if(downloadFile(0, tempfile, server, url, 0, 0, showProgress))
	{
		LOG("DLCFile-%04d| error downloading file from %s, %s\n", jobID, server.c_str(), url.c_str());
		return -1;
	}
	ifstream fin(tempfile.string().c_str());
	if (fin.is_open() == false)
	{
		LOG("DLCFile-%04d| unable to open file for reading: %s\n", jobID, tempfile.string().c_str());
		return -1;
	}
	string line = string();
	while(getline(fin, line))
	{
		std::vector<std::string> args = tokenize_str(line, " ");
		list.push_back(args);
	}
	fin.close();
	WsyncDownload::tryRemoveFile(tempfile, jobID);
	return 0;
}
