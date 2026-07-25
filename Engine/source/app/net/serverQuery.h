//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _SERVERQUERY_H_
#define _SERVERQUERY_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _BITSET_H_
#include "core/bitSet.h"
#endif

#include "platform/platformNet.h"

//-----------------------------------------------------------------------------
// Game Server Information

struct ServerInfo
{
   enum StatusFlags
   {
      // Info flags (0-7):
      Status_Dedicated  = BIT(0),
      Status_Passworded = BIT(1),
      Status_Linux      = BIT(2),

      // Status flags:
      Status_New         = 0,
      Status_Fake       = BIT(27),
      Status_Querying   = BIT(28),
      Status_Updating   = BIT(29),
      Status_Responded  = BIT(30),
      Status_TimedOut   = BIT(31),
   };

   U8          numPlayers;
   U8          maxPlayers;
   U8          numBots;
   char*       name;
   char*       gameType;
   char*       missionName;
   char*       missionType;
   char*       statusString;
   char*       infoString;
   NetAddress  address;
   U32         version;
   U32         ping;
   U32         cpuSpeed;
   bool        isFavorite;
   BitSet32    status;

   ServerInfo()
   {
      numPlayers = 0;
      maxPlayers = 0;
      numBots = 0;
      name = NULL;
      gameType = NULL;
      missionType = NULL;
      missionName = NULL;
      statusString = NULL;
      infoString = NULL;
      version = 0;
      dMemset(&address, '\0', sizeof(NetAddress));
      ping = 0;
      cpuSpeed = 0;
      isFavorite = false;
      status = Status_New;
   }

   ServerInfo(const ServerInfo& other)
   {
      numPlayers = other.numPlayers;
      maxPlayers = other.maxPlayers;
      numBots = other.numBots;
      name = other.name ? dStrdup(other.name) : NULL;
      gameType = other.gameType ? dStrdup(other.gameType) : NULL;
      missionName = other.missionName ? dStrdup(other.missionName) : NULL;
      missionType = other.missionType ? dStrdup(other.missionType) : NULL;
      statusString = other.statusString ? dStrdup(other.statusString) : NULL;
      infoString = other.infoString ? dStrdup(other.infoString) : NULL;
      address = other.address;
      version = other.version;
      ping = other.ping;
      cpuSpeed = other.cpuSpeed;
      isFavorite = other.isFavorite;
      status = other.status;
   }

   ServerInfo& operator=(const ServerInfo& other)
   {
      if (this == &other)
         return *this;

      dFree(name);
      dFree(gameType);
      dFree(missionName);
      dFree(missionType);
      dFree(statusString);
      dFree(infoString);

      numPlayers = other.numPlayers;
      maxPlayers = other.maxPlayers;
      numBots = other.numBots;
      name = other.name ? dStrdup(other.name) : NULL;
      gameType = other.gameType ? dStrdup(other.gameType) : NULL;
      missionName = other.missionName ? dStrdup(other.missionName) : NULL;
      missionType = other.missionType ? dStrdup(other.missionType) : NULL;
      statusString = other.statusString ? dStrdup(other.statusString) : NULL;
      infoString = other.infoString ? dStrdup(other.infoString) : NULL;
      address = other.address;
      version = other.version;
      ping = other.ping;
      cpuSpeed = other.cpuSpeed;
      isFavorite = other.isFavorite;
      status = other.status;

      return *this;
   }

   ~ServerInfo();

   bool isNew()            { return( status == Status_New ); }
   bool isQuerying()       { return( status.test( Status_Querying ) ); }
   bool isUpdating()       { return( status.test( Status_Updating ) ); }
   bool hasResponded()     { return( status.test( Status_Responded ) ); }
   bool isTimedOut()       { return( status.test( Status_TimedOut ) ); }

   bool isDedicated()      { return( status.test( Status_Dedicated ) ); }
   bool isPassworded()     { return( status.test( Status_Passworded ) ); }
   bool isFake()           { return(status.test(Status_Fake)); }
   bool isLinux()          { return( status.test( Status_Linux ) ); }

};

extern Vector<ServerInfo> gServerList;
extern bool gServerBrowserDirty;
extern void clearServerList();
extern void queryLanServers(U32 port, U8 flags, const char* gameType, const char* missionType,
      U8 minPlayers, U8 maxPlayers, U8 maxBots, U32 regionMask, U32 maxPing, U16 minCPU,
      U8 filterFlags);
extern void queryMasterGameTypes();
extern void queryMasterServer(U8 flags, const char* gameType, const char* missionType,
      U8 minPlayers, U8 maxPlayers, U8 maxBots, U32 regionMask, U32 maxPing, U16 minCPU,
      U8 filterFlags, U8 buddyCount, U32* buddyList );
extern void queryFavoriteServers( U8 flags );
extern void querySingleServer(const NetAddress* addr, U8 flags);
extern void startHeartbeat();
extern void sendHeartbeat( U8 flags );

#ifdef TORQUE_DEBUG
extern void addFakeServers( S32 howMany );
extern void addFakeP2PPeers(S32 howMany);
#endif // DEBUG


//-----------------------------------------------------------------------------
// Peer2Peer
struct P2PPeerInfo
{
   enum StatusFlags
   {
      Status_New        = 0,
      Status_Responded  = BIT(0),
      Status_TimedOut   = BIT(1),
      Status_Fake       = BIT(3),
   };

   U32      peerKey;         ///< stable identifier for this peer - NOT an address
   char* displayName;     ///< human-readable name shown in a peer browser UI
   U8       numPlayers;      ///< if this peer is itself hosting others (e.g. a P2P "host"), 0 otherwise
   U8       maxPlayers;
   U32      ping;
   BitSet32 status;

   P2PPeerInfo()
   {
      peerKey = 0;
      displayName = NULL;
      numPlayers = 0;
      maxPlayers = 0;
      ping = 0;
      status = Status_New;
   }

   P2PPeerInfo(const P2PPeerInfo& other)
   {
      peerKey = other.peerKey;
      displayName = other.displayName ? dStrdup(other.displayName) : NULL;
      numPlayers = other.numPlayers;
      maxPlayers = other.maxPlayers;
      ping = other.ping;
      status = other.status;
   }

   P2PPeerInfo& operator=(const P2PPeerInfo& other)
   {
      if (this == &other)
         return *this;

      dFree(displayName);
      peerKey = other.peerKey;
      displayName = other.displayName ? dStrdup(other.displayName) : NULL;
      numPlayers = other.numPlayers;
      maxPlayers = other.maxPlayers;
      ping = other.ping;
      status = other.status;

      return *this;
   }

   ~P2PPeerInfo();

   bool hasResponded()  { return status.test(Status_Responded); }
   bool isTimedOut()    { return status.test(Status_TimedOut); }
   bool isFake()        { return status.test(Status_Fake); }
};

extern Vector<P2PPeerInfo> gP2PPeerList;
extern bool gP2PListDirty;

extern void startP2PAdvertise(U32 peerKey, const char* displayName, U8 maxPlayers);
extern void stopP2PAdvertise();
extern void queryP2PPeerList();
extern void clearP2PPeerList();


#endif
