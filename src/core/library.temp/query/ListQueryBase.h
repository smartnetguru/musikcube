//////////////////////////////////////////////////////////////////////////////
//
// License Agreement:
//
// The following are Copyright � 2008, Daniel �nnerby
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

#pragma once

#include <core/library/query/QueryBase.h>
#include <core/MetadataValue.h>
#include <core/LibraryTrack.h>

#include <sigslot/sigslot.h>
#include <vector>
#include <map>
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

namespace musik { namespace core { namespace query {

    class ListQueryBase : public query::QueryBase {
        public:
            typedef std::map<std::string, MetadataValueVector> MetadataResults;
            typedef sigslot::signal2<MetadataValueVector*, bool> MetadataSignal;
            typedef std::map<std::string, MetadataSignal> MetadataSignals;
            typedef sigslot::signal2<TrackVector*, bool> TrackSignal;
            typedef sigslot::signal3<UINT64, UINT64, UINT64> TrackInfoSignal;

            MetadataSignal& OnMetadataEvent(const char* metatag);
            MetadataSignal& OnMetadataEvent(const wchar_t* metatag);
            TrackSignal& OnTrackEvent();
            TrackInfoSignal& OnTrackInfoEvent();

            ListQueryBase();
            virtual ~ListQueryBase();

        protected:
            friend class library::LibraryBase;
            friend class library::LocalLibrary;

            bool RunCallbacks(library::LibraryBase *library);
            bool ParseTracksSQL(std::string sql, library::LibraryBase *library, db::Connection &db);

            MetadataResults metadataResults;
            TrackVector trackResults;
            std::set<std::string> clearedMetadataResults;
            bool clearedTrackResults;

            MetadataSignals metadataEvent;
            TrackSignal trackEvent;
            TrackInfoSignal trackInfoEvent;

            UINT64 trackInfoTracks;
            UINT64 trackInfoDuration;
            UINT64 trackInfoSize;
    };

} } }