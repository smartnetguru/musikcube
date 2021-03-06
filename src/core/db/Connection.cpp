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

#include "pch.hpp"

#include <core/db/Connection.h>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <sqlite/sqlite3.h>

static boost::mutex globalMutex;

using namespace musik::core::db;

Connection::Connection()
: connection(nullptr)
, transactionCounter(0) {
    this->UpdateReferenceCount(true);
}

Connection::~Connection(){
    this->Close();
    this->UpdateReferenceCount(false);
}

//////////////////////////////////////////
///\brief
///Open a connection to the database
///
///\param database
///Connection string. In SQLite this is the filename
///
///\param options
///Bit options. Unused at the moment
///
///\param cache
///Cachesize in KB
///
///\returns
///Error code returned by SQLite
//////////////////////////////////////////
int Connection::Open(const char *database, unsigned int options, unsigned int cache) {
    int error;

#ifdef UTF_WIDECHAR
    error = sqlite3_open16(database,&this->connection);
#else
    error = sqlite3_open(database,&this->connection);
#endif

    if (error==SQLITE_OK) {
        this->Initialize(cache);
    }

    return error;
}

//////////////////////////////////////////
///\brief
///Open a connection to the database
///
///\param database
///Connection string. In SQLite this is the filename
///
///\param options
///Bit options. Unused at the moment
///
///\param cache
///Cachesize in KB
///
///\returns
///Error code returned by SQLite
//////////////////////////////////////////
int Connection::Open(const std::string &database,unsigned int options,unsigned int cache){
    int error;
    #ifdef WIN32
        std::wstring wdatabase = u8to16(database);
        error = sqlite3_open16(wdatabase.c_str(),&this->connection);
    #else
        error = sqlite3_open(database.c_str(),&this->connection);
    #endif

    if (error==SQLITE_OK) {
        this->Initialize(cache);
    }

    return error;
}

//////////////////////////////////////////
///\brief
///Close connection to the database
///
///\returns
///Errorcode ( musik::core::db::ReturnCode )
//////////////////////////////////////////
int Connection::Close() {
    if (sqlite3_close(this->connection) == SQLITE_OK) {
        this->connection = 0;
        return musik::core::db::Okay;
    }

    return musik::core::db::Error;
}

//////////////////////////////////////////
///\brief
///Execute a SQL string
///
///\param sql
///SQL to execute
///
///\returns
///Errorcode musik::core::db::ReturnCode
///
///\see
///musik::core::db::ReturnCode
//////////////////////////////////////////
int Connection::Execute(const char* sql) {
    sqlite3_stmt *stmt  = NULL;

    // Prepaire seems to give errors when interrupted
    {
        boost::mutex::scoped_lock lock(this->mutex);
        if(sqlite3_prepare_v2(this->connection,sql,-1,&stmt,NULL)!=SQLITE_OK){
            sqlite3_finalize(stmt);
            return db::Error;
        }
    }

    // Execute the statement
    int error   = this->StepStatement(stmt);
    if(error!=SQLITE_OK && error!=SQLITE_DONE){
        sqlite3_finalize(stmt);
        return db::Error;
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return musik::core::db::Okay;
}


//////////////////////////////////////////
///\brief
///Execute a SQL string
///
///\param sql
///SQL to execute
///
///\returns
///Errorcode musik::core::db::ReturnCode
///
///\see
///musik::core::db::ReturnCode
//////////////////////////////////////////
int Connection::Execute(const wchar_t* sql) {
    sqlite3_stmt *stmt  = NULL;
    {
        boost::mutex::scoped_lock lock(this->mutex);
        int err = sqlite3_prepare16_v2(this->connection,sql,-1,&stmt,NULL);
        if(err!=SQLITE_OK){
            sqlite3_finalize(stmt);
            return db::Error;
        }
    }

    // Execute the statement
    int error = this->StepStatement(stmt);
    if(error!=SQLITE_OK && error!=SQLITE_DONE){
        sqlite3_finalize(stmt);
        return db::Error;
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    return musik::core::db::Okay;
}

void Connection::Checkpoint() {
    sqlite3_wal_checkpoint(this->connection, nullptr);
}

//////////////////////////////////////////
///\brief
///Get the last inserted row ID
///
///\returns
///Last inserted row ID
///
///\see
///http://www.sqlite.org/c3ref/last_insert_rowid.html
//////////////////////////////////////////
int Connection::LastInsertedId(){
    return (int) sqlite3_last_insert_rowid(this->connection);
}

//////////////////////////////////////////
///\brief
///Initializes the database.
///
///\param cache
///Size of the cache to use in kilobytes
///
///This will set all the initial PRAGMAS
//////////////////////////////////////////
void Connection::Initialize(unsigned int cache) {
//    sqlite3_enable_shared_cache(1);
    sqlite3_busy_timeout(this->connection, 10000);

    sqlite3_exec(this->connection, "PRAGMA synchronous=OFF", NULL, NULL, NULL);	    // Not a critical DB. Sync set to OFF
    sqlite3_exec(this->connection, "PRAGMA page_size=4096", NULL, NULL, NULL);	    // According to windows standard page size
    sqlite3_exec(this->connection, "PRAGMA auto_vacuum=0", NULL, NULL, NULL);	    // No autovaccum.
    sqlite3_exec(this->connection, "PRAGMA auto_vacuum=0", NULL, NULL, NULL);	    // No autovaccum.
    sqlite3_exec(this->connection, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);    // Allow reading while writing (write-ahead-logging)

    if (cache != 0) {
        // Divide by 4 to since the page_size is 4096
        // Total cache is the same as page_size*cache_size
        cache = cache / 4;
        std::string cacheSize("PRAGMA cache_size=" + boost::lexical_cast<std::string>(cache));
        sqlite3_exec(this->connection,cacheSize.c_str(), NULL, NULL, NULL); // size * 1.5kb = 6Mb cache
    }

    sqlite3_exec(this->connection, "PRAGMA case_sensitive_like=0", NULL, NULL, NULL);   // More speed if case insensitive
    sqlite3_exec(this->connection, "PRAGMA count_changes=0", NULL, NULL, NULL);         // If set it counts changes on SQL UPDATE. More speed when not.
    sqlite3_exec(this->connection, "PRAGMA legacy_file_format=OFF", NULL, NULL, NULL);  // No reason to be backwards compatible :)
    sqlite3_exec(this->connection, "PRAGMA temp_store=MEMORY", NULL, NULL, NULL);       // MEMORY, not file. More speed.
}

void Connection::Interrupt(){
    boost::mutex::scoped_lock lock(this->mutex);
    sqlite3_interrupt(this->connection);
}

void Connection::UpdateReferenceCount(bool init) {
    boost::mutex::scoped_lock lock(globalMutex);

    static int count = 0;

    if (init) {
        if (count == 0) {
            sqlite3_initialize();
        }

        ++count;
    }
    else {
        --count;
        if (count <= 0){
            sqlite3_shutdown();
            count = 0;
        }
    }
}

int Connection::StepStatement(sqlite3_stmt *stmt) {
    return sqlite3_step(stmt);
}
