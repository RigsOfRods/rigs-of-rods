#include "wthread.h"
#include "wizard.h"
#include "cevent.h"
#include "SHA1.h"
#include "installerlog.h"
#include "utils.h"
#include "wsyncdownload.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#include <windows.h>
	#include <conio.h> // for getch
#endif // OGRE_PLATFORM

#include "Timer.h"
#include <boost/algorithm/string.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace boost::filesystem;
using namespace std;

BEGIN_EVENT_TABLE(WsyncThread, wxEvtHandler)
	EVT_MYSTATUS(wxID_ANY, WsyncThread::onDownloadStatusUpdate )
END_EVENT_TABLE()



WsyncThread::WsyncThread(wxEvtHandler *parent, wxString _ipath, std::vector < stream_desc_t > _streams) :
	wxThread(wxTHREAD_DETACHED),
	parent(parent),
	ipath(conv(_ipath)),
	streams(_streams),
	predDownloadSize(0),
	dlNum(0),
	dlm(new WsyncDownloadManager(this))
{
	// main server
	mainserver = server = WSYNC_MAIN_SERVER;
	mainserverdir = serverdir = WSYNC_MAIN_DIR;
}

WsyncThread::~WsyncThread()
{
}

void WsyncThread::onDownloadStatusUpdate(MyStatusEvent &ev)
{
	// first, record the new status of that download job
	int jobID = ev.GetInt();

	if(dlStatus.find(jobID) == dlStatus.end()) return; // invalid job

	switch(ev.GetId())
	{
	case MSE_DOWNLOAD_START:
	{
		dlStatus[jobID].status = 1;
		//LOG("DLFile-TP-%04d| MSE_DOWNLOAD_START\n", jobID );

		// we do this here, since its getting overwritten on the start only
		updateCallback(MSE_UPDATE_TEXT, dlStatus[jobID].path + " (" + formatFilesize(dlStatus[jobID].filesize) + ") ...");
		break;
	}
	case MSE_DOWNLOAD_PROGRESS:
		//LOG("DLFile-TP-%04d| MSE_DOWNLOAD_PROGRESS\n", jobID );
		dlStatus[jobID].status = 1;
		dlStatus[jobID].percent = ev.GetProgress();
		updateCallback(MSE_DOWNLOAD_PROGRESS, dlStatus[jobID].path, ev.GetProgress());
		break;
	case MSE_DOWNLOAD_TIME:
		//LOG("DLFile-TP-%04d| MSE_DOWNLOAD_TIME\n", jobID );
		dlStatus[jobID].time = ev.GetProgress();
		break;
	case MSE_DOWNLOAD_TIME_LEFT:
		//LOG("DLFile-TP-%04d| MSE_DOWNLOAD_TIME_LEFT\n", jobID );
		dlStatus[jobID].time_remaining = ev.GetProgress();
		break;
	case MSE_DOWNLOAD_SPEED:
		//LOG("DLFile-TP-%04d| MSE_DOWNLOAD_SPEED\n", jobID );
		dlStatus[jobID].speed = ev.GetProgress();
		break;
	case MSE_DOWNLOAD_DOWNLOADED:
		//LOG("DLFile-TP-%04d| MSE_DOWNLOAD_DOWNLOADED\n", jobID);
		dlStatus[jobID].downloaded = (boost::uintmax_t)ev.GetProgress();
		break;
	case MSE_DOWNLOAD_DONE:
		//LOG("DLFile-TP-%04d| MSE_DOWNLOAD_DONE\n", jobID );
		updateCallback(MSE_DOWNLOAD_DONE, dlStatus[jobID].path);
		dlStatus[jobID].status = 3;
		break;
	}

	if(updateTimer.elapsed() > 0.5f)
	{
		// then report the progress to the GUI
		reportProgress();
		updateTimer.restart();
	}
}

void WsyncThread::reportProgress()
{
	//LOG("DLFile-TP| reportProgress\n");
	boost::uintmax_t downloadedSize=0;
	boost::uintmax_t speedSum=0;
	float timerunning = dlStartTime.elapsed();
	std::string text;
	char tmp[1024]="";
	int job_existing=0, job_done=0, job_running=0;
	std::map<int, dlstatus_t>::iterator it;
	for(it=dlStatus.begin(); it!=dlStatus.end(); it++)
	{
		downloadedSize += it->second.downloaded;
		speedSum += it->second.speed;

		job_existing++;
		if(it->second.status == 1)
		{
			job_running++;
		}else if(it->second.status == 3)
		{
			job_done++;
		}
	}
	boost::uintmax_t size_left = predDownloadSize - downloadedSize;
	float speed = (float)(downloadedSize / timerunning);

	float eta = size_left / speed;

	// update the progress bar
	float progress = ((float)downloadedSize) /((float)predDownloadSize);
	updateCallback(MSE_UPDATE_PROGRESS, "", progress);

	string sizeDone = formatFilesize(downloadedSize);
	string sizePredicted = formatFilesize(predDownloadSize);

	string timestr = formatSeconds(timerunning);
	updateCallback(MSE_UPDATE_TIME, timestr);

	if(eta > 10)
	{
		timestr = formatSeconds(eta);
		updateCallback(MSE_UPDATE_TIME_LEFT, timestr);
	} else if (eta != 0)
	{
		timestr = std::string("less than 10 seconds");
		updateCallback(MSE_UPDATE_TIME_LEFT, timestr);
	}

	//speedsum is behaving strange ...
	if(timerunning > 0)
	{
		string speedstr = formatFilesize((int)speed) + "/s";
		updateCallback(MSE_UPDATE_SPEED, speedstr);
	}

	char trafstr[256] = "";
	sprintf(trafstr, "%s / %s (%0.0f%%)", sizeDone.c_str(), sizePredicted.c_str(), progress * 100);
	updateCallback(MSE_UPDATE_TRAFFIC, string(trafstr));


	//progressOutputShort(float(changeCounter)/float(changeMax));
	sprintf(tmp, "%d done, %d running, %d remaining", job_done, job_running, job_existing-job_done-job_running);
	updateCallback(MSE_UPDATE_CONCURR, string(tmp));

	if(job_existing-job_done == 0)
	{
		// update the traffic label a last time
		/*
		string sizeOverhead = formatFilesize(downloadedSize-predDownloadSize);
		char trafstr[256] = "";
		sprintf(trafstr, "%s (%s overhead)", sizeDone.c_str(), sizeOverhead.c_str());
		updateCallback(MSE_UPDATE_TRAFFIC, string(trafstr));
		*/
		updateCallback(MSE_UPDATE_TRAFFIC, formatFilesize(downloadedSize));

		// we are done :D
		//updateCallback(MSE_DONE);
	}
}


WsyncThread::ExitCode WsyncThread::Entry()
{
	// update path target
	updateCallback(MSE_UPDATE_PATH, ipath.string());


	//LOG("DLFile-TP| getSyncData\n");
	getSyncData();

	updateCallback(MSE_UPDATE_TITLE, "Downloading ...");
	//LOG("DLFile-TP| sync\n");
	sync();

	//LOG("DLFile-TP| recordDataUsage\n");
	recordDataUsage();

	updateCallback(MSE_UPDATE_TITLE, "Finished!");

	//LOG("DLFile-TP| DONE\n");
	return (WsyncThread::ExitCode)0;     // success
}

void WsyncThread::updateCallback(int type, std::string txt, float percent)
{
	// send event
	MyStatusEvent ev(MyStatusCommandEvent, type);
	ev.SetString(conv(txt));
	ev.SetProgress(percent);
	parent->AddPendingEvent(ev);
}

void WsyncThread::listFiles(const path &dir_path, std::vector<std::string> &files)
{
	if (!exists(dir_path))
		return;
	directory_iterator end_itr; // default construction yields past-the-end
	for (directory_iterator itr(dir_path); itr != end_itr; ++itr)
	{
		if (is_directory(itr->status()))
		{
			listFiles(itr->path(), files);
			continue;
		}
		string entry = itr->path().string();
		files.push_back(entry);
	}
}

int WsyncThread::buildFileIndex(boost::filesystem::path &outfilename, boost::filesystem::path &path, boost::filesystem::path &rootpath, FileHashMap &hashMap, bool writeFile, int mode)
{
	vector<string> files;
	listFiles(path, files);
	char tmp[256] = "";
	updateCallback(MSE_UPDATE_TITLE, "Indexing local files ..");
	sprintf(tmp, "indexing %d files ...", (int)files.size());
	LOG("%s\n", tmp);
	updateCallback(MSE_STARTING, string(tmp));
	int counter = 0, counterMax = files.size();
	for(vector<string>::iterator it=files.begin(); it!=files.end(); it++, counter++)
	{
		string respath = *it;

		boost::uintmax_t fileSize = 0;
		try
		{
			fileSize = file_size(respath);
		} catch(...)
		{
		}

		// cut out root path
		if(respath.substr(0, rootpath.string().size()) == rootpath.string())
		{
			// this ensures that all paths start with /
			int start = rootpath.string().size();
			if(respath.substr(rootpath.string().size()-1,1) == "/")
				start -= 1;
			respath = respath.substr(start);
		}

		// do not use the file index in itself!
		if(respath == (string("/") + string(INDEXFILENAME)))
			continue;

		size_t found = respath.find(".svn");
		if (found != string::npos)
			continue;

		found = respath.find(".temp.");
		if (found != string::npos)
			continue;


		sprintf(tmp, "%s (%s) ...", respath.c_str(), formatFilesize(fileSize).c_str());
		updateCallback(MSE_UPDATE_PROGRESS, string(tmp), float(counter)/float(counterMax));
		updateCallback(MSE_UPDATE_TEXT, string(tmp));

		string resultHash = generateFileHash(it->c_str());

		Hashentry entry(resultHash, file_size(*it));
		hashMap[respath] = entry;
	}
	if(writeFile)
		return saveHashMapToFile(outfilename, hashMap, mode);
	return 0;
}

int WsyncThread::saveHashMapToFile(boost::filesystem::path &filename, FileHashMap &hashMap, int mode)
{
	ensurePathExist(filename);
	const char *cfile = filename.string().c_str();
	FILE *f = fopen(cfile, "wb");
	if(!f)
	{
		LOG("error opening file: %s\n", cfile);
		return -1;
	}

	for(FileHashMap::iterator it=hashMap.begin(); it!=hashMap.end(); it++)
	{
		int fsize = (int)it->second.filesize;
		fprintf(f, "%s : %d : %s\n", it->first.c_str(), fsize, it->second.hash.c_str());
	}
	// add custom keys
	fprintf(f, "|MODE : %d : 0\n", mode);
	fclose(f);
	return 0;
}

std::string WsyncThread::generateFileHash(boost::filesystem::path file)
{
	CSHA1 sha1;
	bool res = sha1.HashFile(const_cast<char*>(file.string().c_str()));
	if(!res) return string("");
	sha1.Final();
	char resultHash[256] = "";
	sha1.ReportHash(resultHash, CSHA1::REPORT_HEX_SHORT);
	return string(resultHash);
}

void WsyncThread::debugOutputHashMap(FileHashMap &hashMap)
{
	for(FileHashMap::iterator it=hashMap.begin(); it!=hashMap.end(); it++)
	{
		int fsize = (int)it->second.filesize;
		LOG(" - %s : %d : %s\n", it->first.c_str(), fsize, it->second.hash.c_str());
	}
}

int WsyncThread::getSyncData()
{
	// generating local FileIndex
	path myFileIndex;
	if(getTempFilename(myFileIndex))
	{
		LOG("error creating tempfile\n");
		updateCallback(MSE_ERROR, "error creating tempfile!");
		return -1;
	}

	hashMapLocal.clear();
	if(buildFileIndex(myFileIndex, ipath, ipath, hashMapLocal, true, 1))
	{
		LOG("error while generating local FileIndex\n");
		updateCallback(MSE_ERROR, "error while generating local FileIndex");
		WsyncDownload::tryRemoveFile(myFileIndex);
		return -1;
	}
	LOG("local hashmap building done:\n");
	debugOutputHashMap(hashMapLocal);

	updateCallback(MSE_UPDATE_TITLE, "Getting remote streams ...");

	// now fetch remote file-indexes
	for(std::vector < stream_desc_t >::iterator it = streams.begin(); it!=streams.end(); it++)
	{
		// only use selected streams
		if(!it->checked) continue;

		path remoteFileIndex;
		if(getTempFilename(remoteFileIndex))
		{
			LOG("error creating tempfile\n");
			updateCallback(MSE_ERROR, "error creating tempfile!");
			WsyncDownload::tryRemoveFile(myFileIndex);
			return -1;
		}

		updateCallback(MSE_UPDATE_TEXT, "downloading file list for stream " + conv(it->path) + " ...");
		LOG("downloading file list to file %s ...\n", remoteFileIndex.string().c_str());
		string url = "/" + conv(it->path) + "/" + INDEXFILENAME;
		WsyncDownload *dl = new WsyncDownload(this);
		if(dl->downloadFile(0, remoteFileIndex.string(), mainserver, url))
		{
			delete(dl);
			updateCallback(MSE_ERROR, "error downloading file index from http://" + mainserver + url);
			LOG("error downloading file index from http://%s%s\n", mainserver.c_str(), url.c_str());
			WsyncDownload::tryRemoveFile(myFileIndex);
			return -1;
		}
		delete(dl);


		string hashMyFileIndex = generateFileHash(myFileIndex);
		string hashRemoteFileIndex = generateFileHash(remoteFileIndex);

		WsyncDownload::tryRemoveFile(INDEXFILENAME);
		if(hashMyFileIndex == hashRemoteFileIndex)
		{
			updateCallback(MSE_DONE, "Files are up to date, no sync needed");
			return 0;
		}

		// now read in the remote index
		FileHashMap temp_hashMapRemote;
		int modeNumber = 0; // pseudo, not used, to be improved
		int res = loadHashMapFromFile(remoteFileIndex, temp_hashMapRemote, modeNumber);
		// remove that temp file
		WsyncDownload::tryRemoveFile(remoteFileIndex);
		if(res)
		{
			updateCallback(MSE_ERROR, "error reading remote file index");
			LOG("error reading remote file index\n");
			WsyncDownload::tryRemoveFile(myFileIndex);
			return -2;
		}

		if(temp_hashMapRemote.size() == 0)
		{
			updateCallback(MSE_ERROR, "remote file index is invalid\nConnection Problems / Server down?");
			LOG("remote file index is invalid\nConnection Problems / Server down?\n");
			WsyncDownload::tryRemoveFile(myFileIndex);
			return -3;
		}
		LOG("remote hashmap reading done [%s]:\n", conv(it->path).c_str());
		debugOutputHashMap(temp_hashMapRemote);

		// add it to our map
		hashMapRemote[conv(it->path)] = temp_hashMapRemote;
	}
	WsyncDownload::tryRemoveFile(myFileIndex);
	LOG("all sync data received\n");
	return 0;
}



int WsyncThread::loadHashMapFromFile(boost::filesystem::path &filename, FileHashMap &hashMap, int &mode)
{
	FILE *f = fopen(filename.string().c_str(), "r");
	if (!f)
	{
		LOG("error opening file '%s'\n", filename.string().c_str());
		return -1;
	}
	while(!feof(f))
	{
		char file[2048]="";
		char filehash[256]="";
		int scansize=0;
		boost::uintmax_t filesize = 0;
		int res = fscanf(f, "%s : %d : %s\n", file, &scansize, filehash);
		if(res < 2)
			continue;
		filesize = scansize;
		if(file[0] == '|')
		{
			// its actually an option
			if(!strncmp(file, "|MODE", 5))
				mode = (int)filesize;
			continue;
		}
		Hashentry entry(filehash, filesize);

		hashMap[file] = entry;
	}
	fclose (f);
	return 0;
}

int WsyncThread::sync()
{
	int res=0;
	char tmp[512] = "";
	memset(tmp, 0, 512);

	LOG("==== starting sync\n");

	std::vector<Fileentry> deletedFiles;
	std::vector<Fileentry> changedFiles;
	std::vector<Fileentry> newFiles;

	FileHashMap::iterator it;
	std::map<string, FileHashMap>::iterator itr;

	// walk all remote hashmaps
	for(itr = hashMapRemote.begin(); itr != hashMapRemote.end(); itr++)
	{
		//first, detect deleted or changed files
		LOG("* detecting deleted or changed files for stream %s...\n", itr->first.c_str());
		for(it = hashMapLocal.begin(); it != hashMapLocal.end(); it++)
		{
			// filter some utility files
			if(it->first == string("/update.temp.exe")) continue;
			if(it->first == string("/stream.info")) continue;
			if(it->first == string("/version.txt")) continue;
			if(it->first == string("/config.cfg")) continue;
			if(it->first == string("/uninst.exe")) continue;
			if(it->first == string("/wizard.log")) continue;
			if(it->first == string("/forums.url")) continue;

			//if(hashMapRemote[it->first] == it->second)
				//printf("same: %s %s==%s\n", it->first.c_str(), hashMapRemote[it->first].c_str(), it->second.c_str());

			// old deletion method
			if(itr->second.find(it->first) == itr->second.end())
			{
				// this file is not in this stream, ignore it for now
				// check if the file is deleted already
				path localfile = ipath / it->first;
				if(exists(localfile))
					deletedFiles.push_back(Fileentry(itr->first, it->first, it->second.filesize));
				continue;
			}
			else if(itr->second[it->first].hash != it->second.hash)
				changedFiles.push_back(Fileentry(itr->first, it->first, itr->second[it->first].filesize));
		}
		// second, detect new files
		LOG("* detecting new files for stream %s...\n", itr->first.c_str());
		for(it = itr->second.begin(); it != itr->second.end(); it++)
		{
			// filter some files
			if(it->first == string("/stream.info")) continue;
			if(it->first == string("/version.txt")) continue;
			if(it->first == string("/config.cfg")) continue;
			if(it->first == string("/uninst.exe")) continue;
			if(it->first == string("/forums.url")) continue;

			if(hashMapLocal.find(it->first) == hashMapLocal.end())
				newFiles.push_back(Fileentry(itr->first, it->first, it->second.filesize));
		}
	}

	// rename the installer if required:
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	//specific things to rename the installer on the fly in order to allow its update
	bool updateInstaller = false;
	for(std::vector<Fileentry>::iterator itf=changedFiles.begin();itf!=changedFiles.end();itf++)
	{
		if(itf->filename == "/updater.exe")
		{
			LOG("* will be updating the installer\n");
			updateInstaller=true;
			break;
		}
	}
	if(updateInstaller)
	{
		path installer_from = ipath / "updater.exe";
		path installer_to = ipath / "updater.exe.old";
		if(exists(installer_from))
		{
			try
			{
				if(boost::filesystem::exists(installer_to))
				{
					// we dont need the old updater, remove it.
					WsyncDownload::tryRemoveFile(installer_to);
				}
				LOG("trying to rename updater from %s to %s\n", installer_from.string().c_str(), installer_to.string().c_str());
				boost::filesystem::rename(installer_from, installer_to);
			} catch (std::exception& e)
			{
				LOG("error while renaming updater: %s\n", std::string(e.what()).c_str());
			}
		}
	}
#endif // OGRE_PLATFORM

	// done comparing, now summarize the changes
	std::vector<Fileentry>::iterator itf;
	int changeCounter = 0, changeMax = changedFiles.size() + newFiles.size() + deletedFiles.size();
	int filesToDownload = newFiles.size() + changedFiles.size();

	LOG("==== Changes:\n");
	predDownloadSize = 0; // predicted download size
	for(itf=newFiles.begin(); itf!=newFiles.end(); itf++)
	{
		predDownloadSize += (int)itf->filesize;
		LOG("> A path:%s, file: %s, size:%d\n", itf->stream_path.c_str(), itf->filename.c_str(), itf->filesize);
	}
	if(!newFiles.size()) LOG("> no files added\n");


	for(itf=changedFiles.begin(); itf!=changedFiles.end(); itf++)
	{
		predDownloadSize += (int)itf->filesize;
		LOG("> U path:%s, file: %s, size:%d\n", itf->stream_path.c_str(), itf->filename.c_str(), itf->filesize);
	}
	if(!changedFiles.size()) LOG("> no files changed\n");

	for(itf=deletedFiles.begin(); itf!=deletedFiles.end(); itf++)
	{
		LOG("> D path:%s, file: %s, size:%d\n", itf->stream_path.c_str(), itf->filename.c_str(), itf->filesize);
	}
	if(!deletedFiles.size()) LOG("> no files deleted\n");


	if(predDownloadSize > 0)
	{
		LOG("==== finding mirror\n");
		bool measureMirrorSpeed = (predDownloadSize > 10485760); // 10 MB
		if(findMirror(measureMirrorSpeed) && measureMirrorSpeed)
		{
			// try to find a mirror without measuring the speed
			findMirror();
		}
	}
	// reset progress bar
	updateCallback(MSE_UPDATE_PROGRESS, "", 0);

	updateCallback(MSE_UPDATE_TEXT, "starting download", 0);
	LOG("==== starting download\n");

	// security check in order not to delete the entire harddrive
	if(deletedFiles.size() > 1000)
	{
		LOG("would delete more than 1000 files, aborting\n");
		updateCallback(MSE_ERROR, "would delete more than 1000 files, aborting");
		return -1;
	}

	dlStartTime = Timer(); // start download counter

	// if there are files to be updated
	if(changeMax)
	{
		string server_use = server, dir_use = serverdir;
		// do things now!
		if(newFiles.size())
		{
			for(itf=newFiles.begin();itf!=newFiles.end();itf++, changeCounter++)
			{
				path localfile = ipath / itf->filename;
				string hash_remote = findHashInHashmap(hashMapRemote, itf->filename);

				this->addJob(conv(localfile.string()), conv(dir_use), conv(server_use), conv(itf->stream_path + itf->filename), conv(hash_remote), conv(itf->filename), itf->filesize);
			}
		}

		if(changedFiles.size())
		{
			for(itf=changedFiles.begin();itf!=changedFiles.end();itf++, changeCounter++)
			{
				path localfile = ipath / itf->filename;
				string hash_remote = findHashInHashmap(hashMapRemote, itf->filename);

				this->addJob(conv(localfile.string()), conv(dir_use), conv(server_use), conv(itf->stream_path + itf->filename), conv(hash_remote), conv(itf->filename), itf->filesize);
			}
		}

		if(deletedFiles.size())
		{
			for(itf=deletedFiles.begin();itf!=deletedFiles.end();itf++, changeCounter++)
			{
				string filename = itf->filename;

				//progressOutputShort(float(changeCounter)/float(changeMax));
				sprintf(tmp, "deleting file: %s\n", filename.c_str()); //, formatFilesize(itf->filesize).c_str());
				updateCallback(MSE_UPDATE_TEXT, string(tmp));
				LOG("%s\n", tmp);
				path localfile = ipath / filename;
				try
				{
					WsyncDownload::tryRemoveFile(localfile);
					//if(exists(localfile))
					//	printf("unable to delete file: %s\n", localfile.string().c_str());
				} catch(...)
				{
					//printf("unable to delete file: %s\n", localfile.string().c_str());
				}
			}
		}
		//sprintf(tmp, "sync complete, downloaded %s\n", formatFilesize(getDownloadSize()).c_str());
		res = 1;

		while(!dlm->isDone())
		{
			Sleep(100);
		}

		// be aware that if you destory this thread before destorying all child threads,
		// the child threads will crash upon event sending

	} else
	{
		sprintf(tmp, "sync complete (already up to date)\n");
	}

	// update progress for the last time
	//downloadProgress();
	// user may proceed
	updateCallback(MSE_DONE, string(tmp), 1);
	LOG("%s\n", tmp);

	return res;
}

std::string WsyncThread::findHashInHashmap(FileHashMap hashMap, std::string filename)
{
	FileHashMap::iterator it;
	for(it = hashMap.begin(); it != hashMap.end(); it++)
	{
		if(it->first == filename)
			return it->second.hash;
	}
	return "";
}

std::string WsyncThread::findHashInHashmap(std::map < std::string, FileHashMap> hashMap, std::string filename)
{
	FileHashMap::iterator it;
	std::map<std::string, FileHashMap >::iterator itm;
	for(itm = hashMap.begin(); itm != hashMap.end(); itm++)
	{
		for(it = itm->second.begin(); it != itm->second.end(); it++)
		{
			if(it->first == filename)
				return it->second.hash;
		}
	}
	return "";
}

void WsyncThread::recordDataUsage()
{
	LOG("==== reporting used up bandwidth\n");
	if(traffic_stats.size() == 0) return;
	std::map < std::string, unsigned int >::iterator itt;
	for(itt=traffic_stats.begin();itt!=traffic_stats.end(); itt++)
	{
		std::vector< std::vector< std::string > > list;
		char tmp[256]="";
		sprintf(tmp, API_RECORDTRAFFIC, itt->first.c_str(), itt->second);
		//responseLessRequest(API_SERVER, string(tmp));

		string sizeStr = formatFilesize(itt->second);
		LOG("%s : %d bytes / %s\n", itt->first.c_str(), itt->second, sizeStr.c_str());
	}
}

int WsyncThread::findMirror(bool probeForBest)
{
	std::vector< std::vector< std::string > > list;
	updateCallback(MSE_STARTING, "finding suitable mirror ...");
	LOG("finding suitable mirror ...\n");
	if(!probeForBest)
	{
		updateCallback(MSE_STARTING, "getting random mirror ...");
		LOG("getting random mirror ...\n");
		// just collect a best fitting server by geolocating this client's IP
		WsyncDownload *wsdl = new WsyncDownload(this);
		int res = wsdl->downloadConfigFile(0, API_SERVER, API_MIRROR, list);
		delete(wsdl);
		if(!res)
		{
			if(list.size()>0 && list[0].size() > 2)
			{
				if(list[0][0] == "failed")
				{
					//printf("failed to get mirror, using main server\n");
					return -1;
				} else
				{
					server = list[0][0];
					serverdir = list[0][1];
				}
			}
		}
		string stxt = server + serverdir + " (random)";
		updateCallback(MSE_UPDATE_SERVER, stxt);
		return 0;
	} else
	{
		// probe servers :D
		// get some random servers and test their speeds
		LOG("getting fastest mirror ...\n");
		WsyncDownload *wsdl = new WsyncDownload(this);
		int res = wsdl->downloadConfigFile(0, API_SERVER, API_MIRROR_NOGEO, list);
		delete(wsdl);

		if(!res)
		{
			if(list.size() > 0)
			{
				if(list[0][0] == "failed")
					return -1;

				double bestTime = 99999;
				int bestServer = -1;
				for(int i=0; i<(int)list.size(); i++)
				{
					updateCallback(MSE_STARTING, "testing speed of mirror: " + list[i][0]);
					updateCallback(MSE_UPDATE_PROGRESS, "", float(i)/float(list.size()));

					double tdiff = measureDownloadSpeed(list[i][0], list[i][1]+"../speedtest.bin");
					if(tdiff < 0)
					{
						// error, skip this server
						//LOG("mirror speed: % 30s : error: %s\n", list[i][0].c_str(), w->getLastError().c_str());
						continue;
					}
					if(tdiff >=0 && tdiff < bestTime)
					{
						bestTime = tdiff;
						bestServer = i;
					}

					char tmp[255]="";
					sprintf(tmp, "%6.2f: kB/s", (10240.0f / tdiff) / 1024.0f);
					updateCallback(MSE_STARTING, "speed of " + list[i][0]+ ": " + std::string(tmp));
					LOG("mirror speed: % 30s : %s\n", list[i][0].c_str(), tmp);
				}
				if(bestServer != -1)
				{
					server = list[bestServer][0];
					serverdir = list[bestServer][1];
					string stxt = server + serverdir + " (fastest)";
					updateCallback(MSE_UPDATE_SERVER, stxt);
				} else
					return -1;
			}
		}

	}
	return 0;
}

double WsyncThread::measureDownloadSpeed(std::string server, std::string url)
{
	path tempfile;
	if(getTempFilename(tempfile))
	{
		LOG("error creating tempfile!\n");
		return -1;
	}

	boost::uintmax_t filesize=0;
	Timer timer = Timer();
	WsyncDownload *dl = new WsyncDownload(this);
	if(dl->downloadFile(0, tempfile, server, url, 0, &filesize))
	{
		delete(dl);
		return -2;
	}
	delete(dl);
	double tdiff = timer.elapsed();
	//printf("mirror speed: %s : %dkB in %0.2f seconds = %0.2f kB/s\n", server.c_str(), (int)(filesize/1024.0f), tdiff, (filesize/1024.0f)/(float)tdiff);
	WsyncDownload::tryRemoveFile(tempfile);
	return tdiff;
}

void WsyncThread::addJob(wxString localFile, wxString remoteDir, wxString remoteServer, wxString remoteFile, wxString hashRemoteFile, wxString localFilename, boost::uintmax_t filesize)
{
	//LOG("DLFile-TP| addJob %d = %s, %s\n", dlNum, conv(localFile).c_str(), conv(remoteFile).c_str());
	dlStatus[dlNum].downloaded=0;
	dlStatus[dlNum].percent=0;
	dlStatus[dlNum].speed=0;
	dlStatus[dlNum].status=0;
	dlStatus[dlNum].text=string();
	dlStatus[dlNum].time=0;
	dlStatus[dlNum].time_remaining=0;
	dlStatus[dlNum].filesize=filesize;
	dlStatus[dlNum].path = conv(localFilename);
	dlm->addJob(dlNum, localFile, remoteDir, remoteServer, remoteFile, hashRemoteFile);
	dlNum++;
}

