//-----------------------------------------------------------------------------
// netStun.cpp
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "network/netStun.h"
#include "console/console.h"
#include "math/mRandom.h"

Vector<NetStun::Query> NetStun::smQueries;
U32  NetStun::smNextQueryId = 1; // 0 is InvalidQueryId
bool NetStun::smHooked = false;

static NetStun::StunSucceededEvent *smStunSucceeded = NULL;
static NetStun::StunFailedEvent    *smStunFailed = NULL;

// Seeded once in init() rather than constructed fresh per-query - MRandomR250
// carries meaningful internal state (250 words) and there's no need to pay
// that cost on every beginQuery() call. STUN transaction IDs are not part of
// gameplay-affecting state, so unlike NetInterface::initRandomData() (which
// journals its seed for deterministic demo playback), this does not need to
// hook Journal::Read/Write - a STUN query's outcome (an external address)
// is environment-dependent regardless of any recorded seed, so replaying a
// demo was never going to reproduce it deterministically anyway.
static MRandomR250 *smRng = NULL;

NetStun::StunSucceededEvent& NetStun::getStunSucceededEvent()
{
   return *smStunSucceeded;
}

NetStun::StunFailedEvent& NetStun::getStunFailedEvent()
{
   return *smStunFailed;
}

void NetStun::init()
{
   smStunSucceeded = new StunSucceededEvent();
   smStunFailed    = new StunFailedEvent();
   smRng           = new MRandomR250(Platform::getRealMilliseconds());

   if (!smHooked)
   {
      // Piggyback on the exact same packet-receive channel every other UDP
      // consumer uses - see the file header comment on why this can't be
      // a blocking recv() instead.
      Net::getPacketReceiveEvent().notify(NetStun::handlePacket);
      smHooked = true;
   }
}

void NetStun::shutdown()
{
   if (smHooked)
   {
      Net::getPacketReceiveEvent().remove(NetStun::handlePacket);
      smHooked = false;
   }

   smQueries.clear();

   delete smStunSucceeded;
   delete smStunFailed;
   delete smRng;
   smStunSucceeded = NULL;
   smStunFailed = NULL;
   smRng = NULL;
}

NetStun::Query* NetStun::findQueryById(U32 queryId)
{
   for (S32 i = 0; i < smQueries.size(); i++)
   {
      if (smQueries[i].queryId == queryId)
         return &smQueries[i];
   }
   return NULL;
}

//-----------------------------------------------------------------------------
// beginQuery / update
//-----------------------------------------------------------------------------

U32 NetStun::beginQuery(const NetAddress &stunServer)
{
   Query query;
   query.queryId = smNextQueryId++;
   query.stunServer = stunServer;
   query.state = Querying;
   query.startTime = Platform::getVirtualMilliseconds();
   query.lastSentTime = 0;
   query.retryCount = 0;
   dMemset(&query.result, 0, sizeof(NetAddress));

   // Random transaction ID - this is how we match a response to this
   // specific query, and (loosely) guards against an off-path attacker
   // spoofing a response without having seen the request.
   for (U32 i = 0; i < StunTransactionIdSize; i++)
      query.transactionId[i] = (U8)(smRng->randI() & 0xFF);

   smQueries.push_back(query);

   Con::printf("NetStun: starting query %u against STUN server", query.queryId);

   return query.queryId;
}

void NetStun::update()
{
   if (smQueries.size() == 0)
      return;

   const U32 now = Platform::getVirtualMilliseconds();
   Vector<S32> toRemove;

   for (S32 i = 0; i < smQueries.size(); i++)
   {
      Query &query = smQueries[i];

      if (query.state != Querying)
      {
         // Same deferred-removal pattern as NetPunchthrough - handlePacket
         // may fire from inside the same net-processing pass that calls
         // update(), so don't mutate smQueries from there directly.
         toRemove.push_back(i);
         continue;
      }

      if (now - query.startTime > RequestTimeoutMs)
      {
         completeQuery(query, false);
         toRemove.push_back(i);
         continue;
      }

      if (query.retryCount >= MaxRetries)
      {
         completeQuery(query, false);
         toRemove.push_back(i);
         continue;
      }

      if (now - query.lastSentTime >= RetryIntervalMs)
         sendRequest(query, now);
   }

   for (S32 i = 0; i < toRemove.size(); i++)
   {
      for (S32 j = i + 1; j < toRemove.size(); j++)
      {
         if (toRemove[j] > toRemove[i])
         {
            S32 tmp = toRemove[i];
            toRemove[i] = toRemove[j];
            toRemove[j] = tmp;
         }
      }
   }
   for (S32 i = 0; i < toRemove.size(); i++)
      smQueries.erase(toRemove[i]);
}

void NetStun::sendRequest(Query &query, U32 now)
{
   // RFC 5389 Binding Request: 20-byte fixed header, no attributes required.
   //
   //  0                   1                   2                   3
   //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   // |0 0|     STUN Message Type     |         Message Length        |
   // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   // |                         Magic Cookie                          |
   // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   // |                                                                |
   // |                     Transaction ID (96 bits)                  |
   // |                                                                |
   // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   U8 packet[StunHeaderSize];

   packet[0] = (StunBindingRequest >> 8) & 0xFF;
   packet[1] = StunBindingRequest & 0xFF;
   packet[2] = 0; // message length high byte - 0 attributes, so length is 0
   packet[3] = 0; // message length low byte

   packet[4] = (StunMagicCookie >> 24) & 0xFF;
   packet[5] = (StunMagicCookie >> 16) & 0xFF;
   packet[6] = (StunMagicCookie >> 8) & 0xFF;
   packet[7] = StunMagicCookie & 0xFF;

   dMemcpy(&packet[8], query.transactionId, StunTransactionIdSize);

   Net::Error err = Net::sendto(&query.stunServer, packet, StunHeaderSize);
   if (err != Net::NoError)
   {
      Con::warnf("NetStun: sendto failed for query %u (error %d)", query.queryId, err);
   }

   query.lastSentTime = now;
   query.retryCount++;
}

//-----------------------------------------------------------------------------
// handlePacket - hooked to Net::getPacketReceiveEvent(); every UDP packet
// that arrives on the game socket passes through here, same as it passes
// through NetPunchthrough's probe/ack handling and ordinary game traffic.
// We only act on packets that look like a STUN response matching a query
// we actually sent; everything else is ignored and left for the rest of
// the engine's packet pump to handle as it already does.
//-----------------------------------------------------------------------------

void NetStun::handlePacket(NetAddress from, RawData packetData)
{
   if (smQueries.size() == 0)
      return; // fast out - don't even parse if nobody's waiting on a STUN reply

   if (packetData.size < StunHeaderSize)
      return; // too short to be a STUN message at all

   const U8 *data = (const U8*)packetData.data;

   // Verify magic cookie before doing anything else - this is what lets us
   // safely ignore ordinary game packets without misinterpreting them.
   U32 cookie = (U32(data[4]) << 24) | (U32(data[5]) << 16) | (U32(data[6]) << 8) | U32(data[7]);
   if (cookie != StunMagicCookie)
      return;

   U16 msgType = (U16(data[0]) << 8) | U16(data[1]);
   if (msgType != StunBindingResponse && msgType != StunBindingErrorResp)
      return;

   const U8 *transactionId = &data[8];

   for (S32 i = 0; i < smQueries.size(); i++)
   {
      Query &query = smQueries[i];
      if (query.state != Querying)
         continue;
      if (!Net::compareAddresses(&query.stunServer, &from))
         continue;
      if (dMemcmp(transactionId, query.transactionId, StunTransactionIdSize) != 0)
         continue;

      // Found the matching in-flight query.
      if (msgType == StunBindingErrorResp)
      {
         completeQuery(query, false);
         return;
      }

      NetAddress external;
      if (tryParseResponse(data, packetData.size, transactionId, &external))
      {
         query.result = external;
         completeQuery(query, true);
      }
      else
      {
         Con::warnf("NetStun: query %u got a Binding Response with no parseable "
            "(XOR-)MAPPED-ADDRESS attribute", query.queryId);
         completeQuery(query, false);
      }
      return;
   }

   // No matching query - either a stale/duplicate response after we already
   // gave up, or a response to something else entirely. Ignore it.
}

//-----------------------------------------------------------------------------
// tryParseResponse - walk the STUN attribute TLVs looking for
// XOR-MAPPED-ADDRESS (preferred) or MAPPED-ADDRESS (RFC 3489 fallback).
//-----------------------------------------------------------------------------

bool NetStun::tryParseResponse(const U8 *data, U32 dataSize, const U8 *expectedTransactionId, NetAddress *outAddr)
{
   U16 msgLength = (U16(data[2]) << 8) | U16(data[3]);

   U32 offset = StunHeaderSize;
   U32 end = StunHeaderSize + msgLength;
   if (end > dataSize)
      end = dataSize; // defensive - don't trust a header claiming more than we actually received

   while (offset + 4 <= end)
   {
      U16 attrType = (U16(data[offset]) << 8) | U16(data[offset + 1]);
      U16 attrLen  = (U16(data[offset + 2]) << 8) | U16(data[offset + 3]);

      U32 valueOffset = offset + 4;
      if (valueOffset + attrLen > end)
         break; // malformed/truncated attribute - stop rather than read out of bounds

      if (attrType == StunAttrXorMappedAddress && attrLen >= 8)
      {
         U8 family = data[valueOffset + 1];
         U16 xport = (U16(data[valueOffset + 2]) << 8) | U16(data[valueOffset + 3]);
         U16 port = xport ^ (U16)(StunMagicCookie >> 16);

         if (family == 0x01) // IPv4
         {
            U32 xaddr = (U32(data[valueOffset + 4]) << 24) | (U32(data[valueOffset + 5]) << 16) |
                        (U32(data[valueOffset + 6]) << 8)  |  U32(data[valueOffset + 7]);
            U32 addr = xaddr ^ StunMagicCookie;

            dMemset(outAddr, 0, sizeof(NetAddress));
            outAddr->type = NetAddress::IPAddress;
            outAddr->address.ipv4.netNum[0] = (addr >> 24) & 0xFF;
            outAddr->address.ipv4.netNum[1] = (addr >> 16) & 0xFF;
            outAddr->address.ipv4.netNum[2] = (addr >> 8) & 0xFF;
            outAddr->address.ipv4.netNum[3] = addr & 0xFF;
            outAddr->port = port;
            return true;
         }
         // IPv6 XOR-MAPPED-ADDRESS (family 0x02) intentionally not handled
         // yet - punchthrough candidate gathering currently only calls this
         // for the IPv4 game socket. Extend here if/when IPv6 punchthrough
         // candidates are needed.
      }
      else if (attrType == StunAttrMappedAddress && attrLen >= 8)
      {
         // RFC 3489 fallback - not XOR'd.
         U8 family = data[valueOffset + 1];
         U16 port = (U16(data[valueOffset + 2]) << 8) | U16(data[valueOffset + 3]);

         if (family == 0x01)
         {
            dMemset(outAddr, 0, sizeof(NetAddress));
            outAddr->type = NetAddress::IPAddress;
            outAddr->address.ipv4.netNum[0] = data[valueOffset + 4];
            outAddr->address.ipv4.netNum[1] = data[valueOffset + 5];
            outAddr->address.ipv4.netNum[2] = data[valueOffset + 6];
            outAddr->address.ipv4.netNum[3] = data[valueOffset + 7];
            outAddr->port = port;
            return true;
         }
      }

      // Attributes are padded to a 4-byte boundary.
      U32 paddedLen = (attrLen + 3) & ~U32(3);
      offset = valueOffset + paddedLen;
   }

   return false;
}

//-----------------------------------------------------------------------------
// completeQuery
//-----------------------------------------------------------------------------

void NetStun::completeQuery(Query &query, bool succeeded)
{
   query.state = succeeded ? Succeeded : Failed;

   if (succeeded)
   {
      char addrStr[256];
      Net::addressToString(&query.result, addrStr);
      Con::printf("NetStun: query %u succeeded - external address %s", query.queryId, addrStr);
      smStunSucceeded->trigger(query.queryId, query.result);
   }
   else
   {
      Con::printf("NetStun: query %u failed (no response from STUN server)", query.queryId);
      smStunFailed->trigger(query.queryId);
   }
}
