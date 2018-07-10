/*
 * Tier1Transaction.cpp
 *
 *  Created on: Jun 20, 2018
 *      Author: Shawn McKenney
 */
#include "primitives/Tier1Transaction.h"
#include "common/devcash_exceptions.h"

namespace Devcash {

void Tier1Transaction::Fill(Tier1Transaction& tx,
                    InputBuffer &buffer,
                    const KeyRing &keys,
                    bool calculate_soundness) {
  MTR_SCOPE_FUNC();
  int trace_int = 124;
  MTR_START("Transaction", "Transaction", &trace_int);
  MTR_STEP("Transaction", "Transaction", &trace_int, "step1");

  tx.sum_size_ = buffer.getNextUint64(false);

  MTR_STEP("Transaction", "Transaction", &trace_int, "step2");
  if (buffer.size() < tx.sum_size_ + kSIG_SIZE + uint64Size()*2) {
    LOG_WARNING << "Invalid serialized T1 transaction, too small!";
    return;
  }

  buffer.copy(std::back_inserter(canonical_)
      , tx.sum_size_ + kSIG_SIZE + uint64Size()*2);

  MTR_STEP("Transaction", "Transaction", &trace_int, "sound");
  if (calculate_soundness) {
    tx.is_sound_ = tx.isSound(keys);
    if (!tx.is_sound_) {
      LOG_WARNING << "Invalid serialized T1 transaction, not sound!";
    }
  }
  MTR_FINISH("Transaction", "Transaction", &trace_int);
}

Tier1Transaction Tier1Transaction::Create(InputBuffer& buffer,
                                 const KeyRing& keys,
                                 bool calculate_soundness) {
  Tier1Transaction tx;
  Tier1Transaction::Fill(tx, buffer, keys, calculate_soundness);
  return tx;
}

Tier1TransactionPtr Tier1Transaction::CreateUniquePtr(InputBuffer& buffer,
                                 const KeyRing& keys,
                                 bool calculate_soundness) {
  auto tx = std::make_unique<Tier1Transaction>();
  Tier1Transaction::Fill(*tx, buffer, keys, calculate_soundness);
  return tx;
}

} // namespace Devcash