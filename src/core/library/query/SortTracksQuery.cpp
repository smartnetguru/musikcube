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

#include "pch.hpp"

#include <core/library/query/SortTracksQuery.h>
#include <core/library/LibraryBase.h>
#include <core/config.h>
#include <boost/algorithm/string.hpp>

using namespace musik::core;
using namespace musik::core::query;

//////////////////////////////////////////
///\brief
///Constructor
//////////////////////////////////////////
SortTracksQuery::SortTracksQuery(void){
}

//////////////////////////////////////////
///\brief
///Destructor
//////////////////////////////////////////
SortTracksQuery::~SortTracksQuery(void){
}


bool SortTracksQuery::ParseQuery(library::LibraryBase *library,db::Connection &db){

    std::vector<int> sortFieldsMetakeyId;

    // Create smart SQL statment
    std::string selectSQL("SELECT temp_track_sort.track_id,tracks.duration,tracks.filesize ");
    std::string selectSQLTables("temp_track_sort LEFT OUTER JOIN tracks ON tracks.id=temp_track_sort.track_id ");
//    std::string selectSQLWhere(" WHERE 1=1 ");
    std::string selectSQLWhere(" ");
//    std::string selectSQLGroup(" GROUP BY tt.id ");
    std::string selectSQLGroup(" ");
    std::string selectSQLSort;

    std::string insertFields;
    std::string insertValues;
    std::string createTableStatement("CREATE TEMPORARY TABLE temp_track_sort (id INTEGER PRIMARY KEY AUTOINCREMENT,track_id INTEGER NOT NULL default 0");

    db::CachedStatement selectMetaKeyId("SELECT id FROM meta_keys WHERE name=?",db);

    while(!this->sortMetaKeys.empty()){
        std::string metakey   = this->sortMetaKeys.front();

        // Check if it's a fixed field
        if(musik::core::library::LibraryBase::IsStaticMetaKey(metakey)){
//            selectSQL     += ",tracks."+metakey;
            selectSQLSort += (selectSQLSort.empty()?" ORDER BY tracks.":",tracks.") + metakey;

        // Check if it's a special MTO field
        }else if(musik::core::library::LibraryBase::IsSpecialMTOMetaKey(metakey) || musik::core::library::LibraryBase::IsSpecialMTMMetaKey(metakey)){
            if(metakey=="album"){
                selectSQLTables += " LEFT OUTER JOIN albums ON albums.id=tracks.album_id ";
//                selectSQLTables += " albums al ";
//                selectSQLWhere  += "al.id=t.album_id";
//                selectSQL     += ",albums.sort_order";
                selectSQLSort += (selectSQLSort.empty()?" ORDER BY albums.sort_order":",albums.sort_order");
            }
            if(metakey=="visual_genre" || metakey=="genre"){
                selectSQLTables += " LEFT OUTER JOIN genres ON genres.id=tracks.visual_genre_id ";
//                selectSQLTables += ",genres g";
//                selectSQLWhere  += "g.id=t.visual_genre_id";
//                selectSQL     += ",genres.sort_order";
                selectSQLSort += (selectSQLSort.empty()?" ORDER BY genres.sort_order":",genres.sort_order");
            }
            if(metakey=="visual_artist" || metakey=="artist"){
                selectSQLTables += " LEFT OUTER JOIN artists ON artists.id=tracks.visual_artist_id";
//                selectSQLTables += ",artists ar";
//                selectSQLWhere  += "ar.id=t.visual_artist_id";
//                selectSQL     += ",artists.sort_order";
                selectSQLSort += (selectSQLSort.empty()?" ORDER BY artists.sort_order":",artists.sort_order");
            }
        } else {
            // Sort by metakeys table
            selectMetaKeyId.BindText(0,metakey);
            if(selectMetaKeyId.Step()==db::Row){
                sortFieldsMetakeyId.push_back(selectMetaKeyId.ColumnInt(0));

                std::string sortField = boost::str( boost::format("ef%1%")%(sortFieldsMetakeyId.size()-1) );

                selectSQLSort += (selectSQLSort.empty()?" ORDER BY temp_track_sort.":",temp_track_sort.")+sortField;
                createTableStatement += ","+sortField+" INTEGER";
                insertFields    += ","+sortField;
                insertValues    += ",?";

            }
            selectMetaKeyId.Reset();

        }

        this->sortMetaKeys.pop_front();
    }


    // First lets start by inserting all tracks in a temporary table
    db.Execute("DROP TABLE IF EXISTS temp_track_sort");

    createTableStatement+=")";
    db.Execute(createTableStatement.c_str());

    {
        insertValues    = "INSERT INTO temp_track_sort (track_id"+insertFields+") VALUES (?"+insertValues+")";
        db::Statement insertTracks(insertValues.c_str(),db);

        db::Statement selectMetaValue("SELECT mv.sort_order FROM meta_values mv,track_meta tm WHERE tm.meta_value_id=mv.id AND tm.track_id=? AND mv.meta_key_id=? LIMIT 1",db);
		for(std::size_t i(0);i<this->tracksToSort.size();++i){
            DBID track(this->tracksToSort[i]);
            insertTracks.BindInt(0,track);

            // Lets find the meta values
            //sortFieldsMetakeyId
            for(std::size_t field(0);field<sortFieldsMetakeyId.size();++field){
                int metakeyId(sortFieldsMetakeyId[field]);
                selectMetaValue.BindInt(0,track);
                selectMetaValue.BindInt(1,metakeyId);
                if(selectMetaValue.Step()==db::Row){
                    insertTracks.BindInt(field+1,selectMetaValue.ColumnInt(0));
                }
            }

            insertTracks.Step();
            insertTracks.Reset();
            insertTracks.UnBindAll();
        }
    }

    // Finaly keep sort order of inserted order.
    selectSQLSort   += ",temp_track_sort.id";

    std::string sql=selectSQL+" FROM "+selectSQLTables+selectSQLWhere+selectSQLGroup+selectSQLSort;

    return this->ParseTracksSQL(sql,library,db);
}

//////////////////////////////////////////
///\brief
///Copy a query
///
///\returns
///A shared_ptr to the Base
//////////////////////////////////////////
Ptr SortTracksQuery::copy() const{
    Ptr queryCopy(new SortTracksQuery(*this));
    queryCopy->PostCopy();
    return queryCopy;
}

void SortTracksQuery::AddTrack(DBID trackId){
    this->tracksToSort.push_back(trackId);
}

void SortTracksQuery::AddTracks(std::vector<DBID> &tracks){
    this->tracksToSort.reserve(this->tracksToSort.size()+tracks.size());
    for(std::vector<DBID>::iterator track=tracks.begin();track!=tracks.end();++track){
        this->tracksToSort.push_back(*track);
    }
}

void SortTracksQuery::AddTracks(musik::core::tracklist::LibraryTrackListQuery &tracks){
    this->tracksToSort.reserve(this->tracksToSort.size()+tracks.Size());
    for(int i(0);i<tracks.Size();++i){
        this->tracksToSort.push_back(tracks[i]->Id());
    }

}

void SortTracksQuery::SortByMetaKeys(std::list<std::string> metaKeys){
    this->sortMetaKeys  = metaKeys;
}

void SortTracksQuery::ClearTracks(){
    this->tracksToSort.clear();
}

std::string SortTracksQuery::Name(){
    return "SortTracks";
}