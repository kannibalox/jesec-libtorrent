// libTorrent - BitTorrent library
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

// Fucked up ugly piece of hack, this code.

#include <algorithm>
#include <cinttypes>

#include "protocol/peer_chunks.h"
#include "torrent/bitfield.h"
#include "torrent/data/block.h"
#include "torrent/data/block_list.h"
#include "torrent/data/block_transfer.h"
#include "torrent/exceptions.h"

#include "download/delegator.h"

namespace torrent {

std::vector<BlockTransfer*>
Delegator::delegate(PeerChunks* peerChunks,
                    uint32_t    affinity,
                    uint32_t    maxPieces) {
  // TODO: Make sure we don't queue the same piece several time on the same peer
  // when it timeout cancels them.
  std::vector<BlockTransfer*> new_transfers;
  PeerInfo*                   peerInfo = peerChunks->peer_info();

  // Find piece with same index as affinity. This affinity should ensure that we
  // never start another piece while the chunk this peer used to download is
  // still in progress.

  // TODO: What if the hash failed? Don't want data from that peer again.
  if (affinity >= 0) {
    for (BlockList* itr : m_transfers) {
      if (new_transfers.size() >= maxPieces)
        return new_transfers;
      if (affinity == itr->index())
        delegate_from_blocklist(itr, peerInfo, new_transfers, maxPieces);
    }
  }

  // Prioritize full seeders
  if (peerChunks->is_seeder()) {
    for (BlockList* itr : m_transfers) {
      if (new_transfers.size() >= maxPieces)
        return new_transfers;
      if (itr->by_seeder())
        delegate_from_blocklist(itr, peerInfo, new_transfers, maxPieces);
    }
    // Create new high priority pieces.
    delegate_new_chunks(peerChunks, new_transfers, maxPieces, true);
    // Create new normal priority pieces.
    delegate_new_chunks(peerChunks, new_transfers, maxPieces, false);
  }
  if (new_transfers.size() >= maxPieces)
    return new_transfers;

  // Find existing high priority pieces.
  for (BlockList* itr : m_transfers) {
    if (new_transfers.size() >= maxPieces)
      return new_transfers;
    if (itr->priority() == PRIORITY_HIGH &&
        peerChunks->bitfield()->get(itr->index()))
      delegate_from_blocklist(itr, peerInfo, new_transfers, maxPieces);
  }

  // Create new high priority pieces.
  delegate_new_chunks(peerChunks, new_transfers, maxPieces, true);

  // Find existing normal priority pieces.
  for (BlockList* itr : m_transfers) {
    if (new_transfers.size() >= maxPieces)
      return new_transfers;
    if (itr->priority() == PRIORITY_NORMAL &&
        peerChunks->bitfield()->get(itr->index()))
      delegate_from_blocklist(itr, peerInfo, new_transfers, maxPieces);
  }

  // Create new normal priority pieces.
  delegate_new_chunks(peerChunks, new_transfers, maxPieces, false);

  if (!m_aggressive)
    return new_transfers;

  // Aggressive mode, look for possible downloads that already have
  // one or more queued.

  // No more than 4 per piece.
  uint16_t overlapped = 5;
  for (BlockList* itr : m_transfers) {
    if (new_transfers.size() >= maxPieces)
      return new_transfers;
    if (peerChunks->bitfield()->get(itr->index()) &&
        itr->priority() != PRIORITY_OFF) {
      for (auto bl_itr = itr->begin(); bl_itr != itr->end() && overlapped != 0;
           bl_itr++) {
        if (new_transfers.size() >= maxPieces ||
            bl_itr->size_not_stalled() >= overlapped)
          break;
        if (!bl_itr->is_finished() && bl_itr->find(peerInfo) == NULL) {
          new_transfers.push_back(bl_itr->insert(peerInfo));
          overlapped = bl_itr->size_not_stalled();
        }
      }
    }
  }

  return new_transfers;
}

void
Delegator::delegate_new_chunks(PeerChunks*                  pc,
                               std::vector<BlockTransfer*>& transfers,
                               uint32_t                     maxPieces,
                               bool                         highPriority) {
  // Find new chunks and if successfull, add all possible pieces into
  // `transfers`
  while (transfers.size() < maxPieces) {
    uint32_t index = m_slot_chunk_find(pc, highPriority);

    if (index == ~(uint32_t)0)
      return;

    TransferList::iterator itr =
      m_transfers.insert(Piece(index, 0, m_slot_chunk_size(index)), block_size);

    (*itr)->set_by_seeder(pc->is_seeder());

    if (highPriority)
      (*itr)->set_priority(PRIORITY_HIGH);
    else
      (*itr)->set_priority(PRIORITY_NORMAL);
    delegate_from_blocklist(*itr, pc->peer_info(), transfers, maxPieces);
  }
}

BlockList*
Delegator::new_chunklist(PeerChunks* pc, bool highPriority) {
  uint32_t index = m_slot_chunk_find(pc, highPriority);

  if (index == ~(uint32_t)0)
    return NULL;

  TransferList::iterator itr =
    m_transfers.insert(Piece(index, 0, m_slot_chunk_size(index)), block_size);

  (*itr)->set_by_seeder(pc->is_seeder());

  if (highPriority)
    (*itr)->set_priority(PRIORITY_HIGH);
  else
    (*itr)->set_priority(PRIORITY_NORMAL);

  return *itr;
}

void
Delegator::delegate_from_blocklist(BlockList*                   c,
                                   PeerInfo*                    peerInfo,
                                   std::vector<BlockTransfer*>& transfers,
                                   uint32_t                     maxPieces) {
  std::vector<Block*> blocks;

  for (auto i = c->begin(); i != c->end(); ++i) {
    if (transfers.size() >= maxPieces)
      return;
    // If not finished or stalled, and no one is downloading this, then assign
    if (!(i->is_finished() || !i->is_stalled()) && i->size_all() == 0)
      transfers.push_back(i->insert(peerInfo));
  }
  if (transfers.size() >= maxPieces)
    return;

  // Fill any remaining slots with potentially stalled pieces.
  // Use a reverse iterator to emulate the previous behavior of the singular
  // `delegate_piece`
  for (auto i = c->rbegin(); i != c->rend() && transfers.size() < maxPieces;
       ++i) {
    if (!(i->is_finished() || !i->is_stalled()) && i->find(peerInfo) == NULL) {
      transfers.push_back(i->insert(peerInfo));
    }
  }
}

} // namespace torrent
