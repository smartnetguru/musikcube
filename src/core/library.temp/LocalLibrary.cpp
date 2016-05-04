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

#include <core/Library/LocalLibrary.h>
#include <core/library/query/QueryBase.h>
#include <core/Preferences.h>

#include <boost/bind.hpp>

using namespace musik::core;
using namespace musik::core::library;

//////////////////////////////////////////
///\brief
///Constructor.
///
///The constructor will not start the Library.
///
///\see
///Startup
//////////////////////////////////////////
LocalLibrary::LocalLibrary(std::string name,int id)
: LibraryBase(name, id)
{
}

//////////////////////////////////////////
///\brief
///Create a LocalLibrary library
//////////////////////////////////////////
LibraryPtr LocalLibrary::Create(std::string name,int id){
	LibraryPtr lib(new LocalLibrary(name,id));
	lib->self	= lib;
	return lib;
}

//////////////////////////////////////////
///\brief
///Destructor that exits and joins all threads
//////////////////////////////////////////
LocalLibrary::~LocalLibrary() {
    this->Exit();
    this->threads.join_all();
}

//////////////////////////////////////////
///\brief
///Get a short status string of what is going on in the Library.
///
///\returns
///Information string.
///
///The information is mostly used to get the information
///about the Indexer.
//////////////////////////////////////////
std::string LocalLibrary::GetInfo(){
    return this->indexer.GetStatus();
}


//////////////////////////////////////////
///\brief
///Startup the library threads.
///
///\returns
///True if successfully started. This should always be true. Nothing else is expected.
///
///Start up the Library like this:
///\code
/// // Create a library
/// musik::core::library::LocalLibrary library;
/// // Start the library (and indexer that is included)
/// library.Startup();
/// // The library is now ready to receive queries
///\endcode
//////////////////////////////////////////
bool LocalLibrary::Startup(){

    // Start the library thread
    try{
        this->threads.create_thread(boost::bind(&LocalLibrary::ThreadLoop,this));
    }
    catch(...){
        return false;
    }

    return true;
}


//////////////////////////////////////////
///\brief
///Main loop the library thread is running in.
///
///The loop will run until Exit() has been called.
//////////////////////////////////////////
void LocalLibrary::ThreadLoop(){

    Preferences prefs("Library");

    std::string database(this->GetDBPath());
    this->db.Open(database.c_str(),0,prefs.GetInt("DatabaseCache",4096));

    LibraryBase::CreateDatabase(this->db);

    // Startup the indexer
    this->indexer.database    = database;
    this->indexer.Startup(this->GetLibraryDirectory());

    while(!this->Exited()){
        query::Ptr query(this->GetNextQuery());

        if(query){    // No empty query

            ////////////////////////////////////////////////////////////
            // Add to the finished queries
            {
                boost::mutex::scoped_lock lock(this->libraryMutex);
                this->runningQuery    = query;
                this->outgoingQueries.push_back(query);

                // Set query as started
                query->status |= query::QueryBase::Started;
            }

            ////////////////////////////////////////////////////////////
            // Lets parse the query
            if(!this->QueryCanceled(query.get())){
                query->ParseQuery(this,this->db);
            }

            {
                boost::mutex::scoped_lock lock(this->libraryMutex);
                this->runningQuery.reset();
                // And set it as finished
                query->status |= query::QueryBase::Ended;
            }

            ////////////////////////////////////////////////////////////
            // Notify that the Query is finished.
            this->waitCondition.notify_all();

        }else{

            ////////////////////////////////////////////////////////////
            // Tricky part, waiting for queries to be added.
            // Not sure I'm doing this the right way.
            // Could this part lead to a deadlock???
            boost::mutex::scoped_lock lock(this->libraryMutex);
            if(!this->exit && this->incomingQueries.size()==0 ){

                // This is a likely place where a thread will wait
                this->waitCondition.wait(lock);

            }
        }
    }

}

//////////////////////////////////////////
///\brief
///Cancel the current running query
///
///This method will also send a sqlite3_interrupt to cancel the
///current running SQL Query
//////////////////////////////////////////
void LocalLibrary::CancelCurrentQuery( ){
    this->db.Interrupt();
}

//////////////////////////////////////////
///\brief
///Get a pointer to the librarys Indexer (NULL if none)
//////////////////////////////////////////
musik::core::Indexer* LocalLibrary::Indexer(){
    return &this->indexer;
}

