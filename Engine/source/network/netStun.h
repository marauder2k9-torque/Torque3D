//-----------------------------------------------------------------------------
// netStun.h
//
// Minimal STUN (RFC 5389) Binding Request/Response client, used to discover
// this client's external (NAT-mapped) address for use as a punchthrough
// candidate.
//-----------------------------------------------------------------------------

#ifndef _NETSTUN_H_
#define _NETSTUN_H_

#ifndef _PLATFORM_PLATFORMNET_H_
#include "platform/platformNet.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _JOURNALEDSIGNAL_H_
#include "core/util/journal/journaledSignal.h"
#endif

class NetStun
{
public:
   enum Constants
   {
      MaxRetries       = 4,
      RetryIntervalMs  = 500,
      RequestTimeoutMs = 3000,
      InvalidQueryId   = 0,

      // RFC 5389 fixed values.
      StunMagicCookie       = 0x2112A442,
      StunBindingRequest    = 0x0001,
      StunBindingResponse   = 0x0101,
      StunBindingErrorResp  = 0x0111,
      StunAttrMappedAddress = 0x0001,
      StunAttrXorMappedAddress = 0x0020,
      StunHeaderSize        = 20,
      StunTransactionIdSize = 12,
   };

   enum State
   {
      Querying,
      Succeeded,
      Failed
   };

   struct Query
   {
      U32        queryId;
      NetAddress stunServer;
      U8         transactionId[StunTransactionIdSize];
      State      state;
      U32        startTime;
      U32        lastSentTime;
      U16        retryCount;
      NetAddress result;
   };

   /// void event(U32 queryId, NetAddress externalAddress)
   typedef JournaledSignal<void(U32,NetAddress)> StunSucceededEvent;
   /// void event(U32 queryId)
   typedef JournaledSignal<void(U32)> StunFailedEvent;

   static StunSucceededEvent& getStunSucceededEvent();
   static StunFailedEvent&    getStunFailedEvent();

   static void init();
   static void shutdown();

   /// Kick off a STUN query against the given server, using whatever socket
   /// Net::getPort() currently returns (i.e. the game's own UDP socket - the
   /// external mapping only matches what you'll actually punch/connect from
   /// if queried from the same local port).
   static U32  beginQuery(const NetAddress &stunServer);

   /// Tick - call once per frame/net-update, alongside NetPunchthrough::update().
   static void update();

   /// Wire this into Net::getPacketReceiveEvent() - see netStun.cpp's init()
   /// for how this is done; exposed here in case a caller needs to feed it
   /// packets from somewhere other than the default hookup.
   static void handlePacket(NetAddress from, RawData packetData);

private:
   static Vector<Query> smQueries;
   static U32            smNextQueryId;
   static bool           smHooked;

   static Query* findQueryById(U32 queryId);
   static void   sendRequest(Query &query, U32 now);
   static void   completeQuery(Query &query, bool succeeded);
   static bool   tryParseResponse(const U8 *data, U32 dataSize, const U8 *expectedTransactionId, NetAddress *outAddr);
};

#endif // _NETSTUN_H_
