//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007-2016 musikcube team
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the author nor the names of other contributors may
//      be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Hotkeys.h"
#include <core/support/Preferences.h>
#include <app/util/Playback.h>
#include <unordered_map>
#include <unordered_set>

using namespace musik::box;
using namespace musik::core;

using Id = Hotkeys::Id;

/* sigh: http://stackoverflow.com/a/24847480 */
struct EnumHasher {
    template <typename T>
    std::size_t operator()(T t) const {
        return static_cast<std::size_t>(t);
    }
};

/* map from internal ID to user-friendly JSON key name */
static std::unordered_map<std::string, Id> NAME_TO_ID = {
    { "navigate_library", Id::NavigateLibrary },
    { "navigate_library_browse", Id::NavigateLibraryBrowse },
    { "navigate_library_browse_artists", Id::NavigateLibraryBrowseArtists },
    { "navigate_library_browse_albums", Id::NavigateLibraryBrowseAlbums },
    { "navigate_library_browse_genres", Id::NavigateLibraryBrowseGenres },
    { "navigate_library_filter", Id::NavigateLibraryFilter },
    { "navigate_library_tracks", Id::NavigateLibraryTracks },
    { "navigate_library_play_queue", Id::NavigateLibraryPlayQueue },
    { "navigate_settings", Id::NavigateSettings },
    { "navigate_console", Id::NavigateConsole },
    { "navigate_jump_to_playing", Id::NavigateJumpToPlaying },

    { "playback_toggle_pause", Id::TogglePause },
    { "playback_next", Id::Next },
    { "playback_previous", Id::Previous },
    { "playback_volume_up", Id::VolumeUp },
    { "playback_volume_down", Id::VolumeDown },
    { "playback_seek_forward", Id::SeekForward },
    { "playback_seek_back", Id::SeekBack },
    { "playback_toggle_repeat", Id::ToggleRepeat },
    { "playback_toggle_shuffle", Id::ToggleShuffle },
    { "playback_stop", Id::Stop },

    { "view_refresh", Id::ViewRefresh },

    { "metadata_rescan", Id::RescanMetadata }
};

/* default hotkeys */
static std::unordered_map<Id, std::string, EnumHasher> ID_TO_DEFAULT = {
    { Id::NavigateLibrary, "M-a" },
    { Id::NavigateLibraryBrowse, "M-b" },
    { Id::NavigateLibraryBrowseArtists, "M-1" },
    { Id::NavigateLibraryBrowseAlbums, "M-2" },
    { Id::NavigateLibraryBrowseGenres, "M-3" },
    { Id::NavigateLibraryFilter, "M-f" },
    { Id::NavigateLibraryTracks, "M-t" },
    { Id::NavigateLibraryPlayQueue, "M-n" },
    { Id::NavigateSettings, "M-s" },
    { Id::NavigateConsole, "M-`" },
    { Id::NavigateJumpToPlaying, "M-m" },

    { Id::TogglePause, "^P" },
    { Id::Next, "M-l" },
    { Id::Previous, "M-j" },
    { Id::VolumeUp, "M-i" },
    { Id::VolumeDown, "M-k" },
    { Id::SeekForward, "M-o" },
    { Id::SeekBack, "M-u" },
    { Id::ToggleRepeat, "M-," },
    { Id::ToggleShuffle, "M-." },
    { Id::Stop, "^X" },

    { Id::ViewRefresh, "KEY_F(5)" },

    { Id::RescanMetadata, "^R"}
};

/* custom keys */
static std::unordered_set<std::string> customKeys;
static std::unordered_map<Id, std::string, EnumHasher> customIdToKey;

/* preferences file */
static std::shared_ptr<Preferences> prefs;

static void savePreferences() {
    for (const std::pair<std::string, Id>& pair : NAME_TO_ID) {
        prefs->SetString(
            pair.first.c_str(),
            Hotkeys::Get(pair.second).c_str());
    }

    prefs->Save();
}

static void loadPreferences() {
    prefs = Preferences::ForComponent("hotkeys", Preferences::ModeReadWrite);

    try {
        /* load all of the custom key mappings into customKeys and 
        customIdToKey structures for quick lookup. */
        if (prefs) {
            customKeys.clear();
            std::vector<std::string> names;
            prefs->GetKeys(names);
            for (auto n : names) {
                auto it = NAME_TO_ID.find(n);
                if (it != NAME_TO_ID.end()) {
                    customKeys.insert(prefs->GetString(n));
                    customIdToKey[it->second] = prefs->GetString(n);
                }
            }
        }

        /* write back to disk; this way any new hotkey defaults are picked
        up and saved so the user can edit them easily. */
        savePreferences();
    }
    catch (...) {
        std::cerr << "failed to load hotkeys.json! default hotkeys selected.";
        customKeys.clear();
        customIdToKey.clear();
    }
}

Hotkeys::Hotkeys() {
}

bool Hotkeys::Is(Id id, const std::string& kn) {
    if (!prefs) {
        loadPreferences();
    }

    /* see if the user has specified a custom value for this hotkey. if
    they have, compare it to the custom value. */
    auto custom = customIdToKey.find(id);
    if (custom != customIdToKey.end()) {
        return (custom->second == kn);
    }

    /* otherwise, let's compare against the default key, assuming the
    input key doesn't match ANY that the user has customized */
    if (customKeys.find(kn) == customKeys.end()) {
        auto it = ID_TO_DEFAULT.find(id);
        if (it != ID_TO_DEFAULT.end() && it->second == kn) {
            return true;
        }
    }

    return false;
}

std::string Hotkeys::Get(Id id) {
    if (!prefs) {
        loadPreferences();
    }

    auto custom = customIdToKey.find(id);
    if (custom != customIdToKey.end()) {
        return custom->second;
    }

    auto it = ID_TO_DEFAULT.find(id);
    if (it != ID_TO_DEFAULT.end()) {
        return it->second;
    }

    return "";
}
