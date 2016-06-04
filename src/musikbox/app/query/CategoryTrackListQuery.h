#pragma once

#include <core/library/query/QueryBase.h>
#include <core/db/Connection.h>
#include <core/library/track/Track.h>

#include "TrackListQueryBase.h"

namespace musik {
    namespace box {
        class CategoryTrackListQuery : public TrackListQueryBase {
            public:
                CategoryTrackListQuery(
                    musik::core::LibraryPtr library,
                    const std::string& column, 
                    DBID id);

                virtual ~CategoryTrackListQuery();

                virtual std::string Name() { return "CategoryTrackListQuery"; }

                virtual Result GetResult();
                virtual Headers GetHeaders();
                virtual size_t GetQueryHash();

            protected:
                virtual bool OnRun(musik::core::db::Connection &db);

            private:
                musik::core::LibraryPtr library;
                Result result;
                Headers headers;
                std::string column;
                DBID id;
                std::string query;
                size_t hash;
        };
    }
}