/*
    This source file is part of Rigs of Rods
    Copyright 2013-2021 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#if USE_EFSW

#include "ProjectFileWatcher.h"

#include "Application.h"
#include "Console.h"
#include "GameContext.h"
#include "PlatformUtils.h"
#include "ProjectManager.h"

#include <fmt/format.h>

using namespace RoR;

void ProjectFileWatcher::StartWatching(std::string const& dirname, std::string const& filename)
{
    ROR_ASSERT(dirname != "");
    ROR_ASSERT(filename != "");

    if (!m_watcher)
        m_watcher = std::make_unique<efsw::FileWatcher>();

    this->StopWatching();

    std::string path = PathCombine(PathCombine(App::sys_projects_dir->GetStr(), dirname), filename);
    m_watch_id = m_watcher->addWatch(path, this);
    if (m_watch_id > 0)
    {
        m_watcher->watch();
    }
    else
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_PROJECT, Console::CONSOLE_SYSTEM_ERROR,
                                      fmt::format(_L("Could not set up file monitoring for project '{}/{}', error code '{}'"),
                                                  dirname, filename, m_watch_id));
        m_watch_id = 0;
    }
}

void ProjectFileWatcher::StopWatching()
{
    if (m_watcher && m_watch_id != 0)
    {
        m_watcher->removeWatch(m_watch_id);
        m_watch_id = 0;
    }
}

void ProjectFileWatcher::handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename )
{
    if (action == efsw::Action::Modified)
    {
        Project* project = App::GetProjectManager()->GetActiveProject();
        if (project && App::edi_file_watch_respawn->GetBool())
        {
            for (Actor* actor: App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor->GetProject() == project)
                {
                    ActorModifyRequest* request = new ActorModifyRequest();
                    request->amr_type = ActorModifyRequest::Type::RELOAD;
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)request));
                }
            }
        }
    }
}

#endif // USE_EFSW

