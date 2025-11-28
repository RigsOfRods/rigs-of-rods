/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
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

/// @file   GUI_RepositorySelector.h
/// @author Rafael Galvan, 04/2021
/// @author Petr Ohlidal, 2022
/// @author tritonas00, 2022

#pragma once

#include "Application.h"
#include "BBDocument.h"
#include "OgreImGui.h" // ImVec4

#include <future>
#include <memory>
#include <thread>
#include <vector>
#ifdef USE_CURL
#   include <curl/curl.h>
#endif //USE_CURL

namespace RoR {

    struct RepoImageDownloadRequest
    {
        // Only one should be set!
        int thumb_resourceitem_idx = -1; //!< fetch thumbnail
        int attachment_id = -1; //!< download attachment
        // attachment extras
        std::string attachment_ext;
        // thumb extras
        int thumb_resource_id = 0;
        std::string thumb_url;
    };

    // This will be removed during OGRE14 migration
    struct RepoImageRequestHandler: public Ogre::WorkQueue::RequestHandler
    {
        Ogre::WorkQueue::Response* handleRequest(const Ogre::WorkQueue::Request* req, const Ogre::WorkQueue* srcQ) override;
    };

    // `Ogre::Any` holder requires the `<<` operator to be implemented, otherwise it won't compile. ~ This will also be removed during OGRE14 migration
    inline std::ostream& operator<<(std::ostream& os, RepoImageDownloadRequest& val)
    {
        return os;
    }

namespace GUI {

struct ResourceCategories
{
    int                 resource_category_id;
    std::string         title;
    std::string         description;
    int                 resource_count;
    int                 display_order;
};

struct ResourceItem
{
    int                 resource_id;
    std::string         title;
    std::string         tag_line;
    std::string         icon_url;
    std::string         authors;
    std::string         version;
    bbcpp::BBDocumentPtr description;
    int                 download_count;
    int                 last_update;
    int                 resource_category_id;
    float               rating_avg;
    int                 rating_count;
    std::string         view_url;
    int                 resource_date;
    int                 view_count;
    Ogre::TexturePtr    preview_tex;

    // Repo UI state:
    bool                thumbnail_dl_queued = false;
    bool                attachments_dl_queued = false; //!< Attachments are requested in bulk on first display
};

typedef int ResourceItemArrayPos_t;
const int RESOURCEITEMARRAYPOS_INVALID = -1;

struct ResourceFiles
{
    int                 id;
    std::string         filename;
    int                 size;
};

struct ResourcesCollection
{
    std::vector<ResourceItem>           items;
    std::vector<ResourceCategories>     categories;
    std::vector<ResourceFiles>          files;
};

typedef std::map<int, Ogre::TexturePtr> RepoAttachmentsMap; //!< Maps attachment ID to Ogre::TexturePtr

class BBCodeDrawingContext;

class RepositorySelector
{
public:
    const float                         ATTACH_MAX_WIDTH = 160.f;
    const float                         ATTACH_MAX_HEIGHT = 90.f;
    const float                         ATTACH_SPINNER_RADIUS = 20.f;
    const ImVec2                        ATTACH_SPINNER_PADDING = ImVec2(55.f, 25.f);
    const ImVec4                        RESOURCE_TITLE_COLOR = ImVec4(1.f, 1.f, 0.7f, 1.f);
    const ImVec4                        RESOURCE_INSTALL_BTN_COLOR = ImVec4(0.830, 0.655, 0.174, 1.f);

    RepositorySelector();
    ~RepositorySelector();

    void                                SetVisible(bool visible);
    bool                                IsVisible() const { return m_is_visible; }
    void                                Draw();
    void                                DrawGalleryView();
    void                                DrawResourceView(float searchbox_x);
    void                                DrawResourceViewRightColumn();
    void                                OpenResource(int resource_id);
    void                                Download(int resource_id, std::string filename, int id);
    void                                DownloadFinished();
    void                                Refresh();
    void                                UpdateResources(ResourcesCollection* data);
    void                                UpdateResourceFilesAndDescription(ResourcesCollection* data);
    void                                ShowError(CurlFailInfo* failinfo);
    void                                DrawThumbnail(ResourceItemArrayPos_t resource_arraypos, ImVec2 image_size, float spinner_size, ImVec2 spinner_cursor);
    static void                         DownloadImage(RepoImageDownloadRequest* request); //!< To be run on background via Ogre WorkQueue
    void                                LoadDownloadedImage(RepoImageDownloadRequest* request); //!< To be run on main thread
    void                                DrawResourceDescriptionBBCode(const ResourceItem& item, ImVec2 panel_screenpos, ImVec2 panel_size);
    void                                DrawAttachment(BBCodeDrawingContext* context, int attachment_id);
    void                                DownloadAttachment(int attachment_id, std::string const& attachment_ext);
    void                                DownloadBBCodeAttachmentsRecursive(const bbcpp::BBNode& parent);

private:
    bool                                m_is_visible = false;
    bool                                m_draw = false;
    ResourcesCollection                 m_data;
    Str<500>                            m_search_input;
    std::string                         m_current_category;
    int                                 m_current_category_id = 1;
    std::string                         m_all_category_label;
    std::string                         m_current_category_label;
    int                                 m_gallery_mode_attachment_id = -1;
    bool                                m_update_cache = false;
    bool                                m_show_spinner = false;
    std::string                         m_current_sort = "Last Update";
    std::string                         m_view_mode = "List";
    ResourceItemArrayPos_t              m_resourceview_item_arraypos = RESOURCEITEMARRAYPOS_INVALID;
    Ogre::TexturePtr                    m_fallback_thumbnail;
    RepoAttachmentsMap                  m_repo_attachments; //!< Fully loaded images in memory.

    // This will be removed during OGRE14 migration
    Ogre::uint16                        m_ogre_workqueue_channel = 0;
    RepoImageRequestHandler             m_repo_image_request_handler;

    // status or error messages
    std::string                         m_repofiles_msg;
    std::string                         m_repolist_msg;
    ImVec4                              m_repolist_msg_color;
    std::string                         m_repolist_curlmsg; //!< Displayed as dimmed text
    std::string                         m_repolist_httpmsg; //!< Displayed as dimmed text
};

}// namespace GUI
}// namespace RoR
