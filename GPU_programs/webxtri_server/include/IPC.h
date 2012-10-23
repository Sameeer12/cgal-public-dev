// ============================================================================
//
// Copyright (c) 2001-2006 Max-Planck-Institut Saarbruecken (Germany).
// All rights reserved.
//
// This file is part of EXACUS (http://www.mpi-inf.mpg.de/projects/EXACUS/);
// you may redistribute it under the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with EXACUS.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// ----------------------------------------------------------------------------
//
// Library       : AlciX
// File          : demos/webxalci/include/IPC.h
// SoX_release   : $Name:  $
// Revision      : $Revision: 1.14 $
// Revision_date : $Date: 2007/02/06 12:03:11 $
//
// Author(s)     : Pavel Emeliyanenko <asm@mpi-sb.mpg.de>
//
// ============================================================================

/*! \file IPC.h
 *  \brief defines main interprocess communications data structutes
 *  
 *  these data structures must be common for client and server applications  
 */

#ifndef XTRI_IPC_H
#define XTRI_IPC_H

#include <memory.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>

#define _THREAD_SAFE
//! a filename to be used to generate unique IPC keys
#define KEY_FILENAME        "/etc/security/time.conf"

// CAUTION!!! this parameter must be changed very carefully
// taking into account maximal size of client-server messages
#define MAX_SEG_SIZE        512*1024L    // max shared segment size
    
//! message type: (PID << 4) | MSG_TYPE - to identify a process
enum Message_type {
    ANALYSE = 1,        // analyse surface
    TRIANGULATE = 2,    // compute triangulation
    COMMENT = 3,       // send a comment
    PING = 4,          // check for server connection
    MSGS_SERVER = -5,      // all messages dedicated for server
    ANALYSE_ACK = 4,   // analyse request server response
    TRIANGULATE_ACK = 5, // triangulate server response
    COMMENT_ACK = 6,    // send comment server response
    PING_ACK = 7        // ping response
};

//! triangulation mode
enum Triangulate_mode {
    BOGUS = -1,
    TRIANGULATE_DEFAULT = 0,       // default
    TRIANGULATE_ABS_BOUNDS = 1     // triangulate using absolute bounds
};

enum Error_code {
    ERR_OK = 0,                   // no errors
    ERR_INVALID_POLYNOMIAL = -1,  // invalid polynomial format
    ERR_INVALID_DATA = -2,        // invalid client parameters
    ERR_TIMEOUT = -3,             // analyse/rasterize phase timeout
    ERR_INVALID_REFERRER = -6,    // an attempt to access the script from
                                  // outside the server
    ERR_SERVER_TIMEOUT = -7,      // no connection to the server
    ERR_REQUEST_PENDING = -8,     // a request from this IP is already being
                                  // processed by the server
    ERR_SERVER_OVERLOAD = -9,     // the server is overloaded (number of
                                  // requests processed >= MAX_CLIENTS
    ERR_TRIANGULATE_GENERIC = -10   // generic error during rasterize request
};

//! describes format of a message queue
struct IPC_Message {   
    uint m_type;
    union {
        key_t shm_key;         //! the key of a shared memory region
        Error_code err_code;   //! error code: 0 indicates no errors
                               //! (server messages only)
    };
    long shm_size;  //! size of a shared memory region
};

// shm format:
// MSG_ANALYSE:   PID (4), n_indices(4) == 0, < polynomial string >
// MSG_RASTERIZE: PID (4), n_indices(4) != 0, < array of indices >, 
//      < polynomial string >

//! describes the format of a shared memory region of the client request
struct SHM_Data {
    pid_t PID;              //! client's PID
    in_addr_t host_addr;    //! client's IP-address
    Triangulate_mode mode;    //! triangulation mode (obsolete ?)
    //! skeletonizer specific parameters
    double en_left, en_right; 
    double en_bottom, en_top;
    double z_below, z_above;
    unsigned sx, sy;
};

//! shm format of the server reply
struct Triangulation_info {
    // # of vertices / triangles / silhouette triangles
    uint n_verts, n_tris, n_sil_tris;
    uint n_z_patches;             // # of z-patches
    double left, right, btm, top; // estimated x/y bounds
    double cx, cy, cz;            // centroid coordinates of a surface
};


//! \brief defines MD5 checksum used to identify bivariate polynomials
//!
//! first polynomial is printed out in ASCII format, and then MD5 checksum
//! is computed out of the resulting string
struct MD5_digest {
    MD5_digest() { }
    //! this operators required by \c ::boost::multi_index container
    bool operator ==(const MD5_digest& md) const
    { return (memcmp(this, &md, sizeof(MD5_digest)) == 0); }
    
    bool operator !=(const MD5_digest& md) const
    { return (!operator ==(md)); }
    unsigned md[4];
};

//! comparison predicate for \c active_job set
struct MD5_compare {
    bool operator ()(const MD5_digest& md1, const MD5_digest& md2) const
    {
        return (memcmp(&md1, &md2, sizeof(MD5_digest)) < 0);
    }
};

//! describes a reply format for surface analysis request
struct SHM_Analysis_reply {
    MD5_digest surface_ID; // hash identifying the set of polynomials
};

//! this structure is used to client (thread) information to cancellation
//! handler
struct Thread_info {
    Message_type msg_type;      //! a type of request being processed
    int timeout;        //! request timeout
    MD5_digest md;      //! unique curve identifier
};

//! this structure is passed to thread's cleanup handler to cancel request
struct Thread_cleanup_info {
    IPC_Message *pmsg;      //! message to be sent to the client
    void *shm_addr;         //! shared memory region address 
    //! client's IP: required to remove IP the from active client list
    in_addr_t host_addr;

    Message_type msg_type;      //! a type of request being processed
    int timeout;        //! request timeout
    MD5_digest md;      //! unique curve identifier
};

//! \brief this structure describes clients whose requests are being
//! processed by the server to prevent a user to initiate several requests 
//! simultaneously
struct Active_client_info 
{
    Active_client_info() { }
    Active_client_info(in_addr_t addr_): host_addr(addr_) 
    { }
    in_addr_t host_addr;
};

//! \brief stores information about incoming client request
struct Request_info 
{
    char time[64];        //! time of request in human-readable format
    char ip_address[32];  //! ip-address of the client
    char hostname[256];   //! client's hostname
};

//! global functions declarations

void *multiplexer_thread_proxy(void *data);
void *main_request_thread_proxy(void *data);
void thread_cleanup_handler_proxy(void *data);

void sigint_handler(int);
void err_msg(const char *text);
void err_exit();

#endif // XTRI_IPC_H
