/*
 * transaction.h defines the structure of the transaction section of a block.
 *
 *  Created on: Dec 11, 2017
 *  Author: Nick Williams
 *
 *  **Transaction Structure**
 *  oper - coin operation (one of create, modify, exchange, delete)
 *  type - the coin type
 *  xfer - array of participants
 *
 *  xfer params:
 *  addr - wallet address
 *  amount - postive implies giving, negative implies taking, 0 implies neither
 *  nonce - generated by wallet (required iff amount positive)
 *  sig - generated by wallet (required iff amount positive)
 *
 *
 */

#ifndef DEVCASH_PRIMITIVES_TRANSACTION_H
#define DEVCASH_PRIMITIVES_TRANSACTION_H

#include <string>
#include <vector>
#include <stdint.h>

#include "Transfer.h"
#include "Summary.h"
#include "consensus/KeyRing.h"
#include "consensus/chainstate.h"
#include "Validation.h"

using namespace Devcash;

namespace Devcash
{
static const std::string kXFER_COUNT_TAG = "xfer_count";
static const std::string kOPER_TAG = "oper";
static const std::string kXFER_TAG = "xfer";
static const std::string kNONCE_TAG = "nonce";
static const std::string kSIG_TAG = "sig";

enum eOpType : byte {
  Create     = 0,
  Modify     = 1,
  Exchange   = 2,
  Delete     = 3};

class Transaction {
 public:
  uint64_t xfer_count_;

/** Constructors */
  Transaction() : xfer_count_(0), canonical_(), is_sound_(false) {}
  explicit Transaction(const std::vector<byte>& serial, const KeyRing& keys)
    : xfer_count_(0), canonical_(), is_sound_(false) {
    if (serial.size() < MinSize()) {
      LOG_WARNING << "Invalid serialized transaction, too small!";
      return;
    }
    xfer_count_ = BinToUint64(serial, 0);
    size_t tx_size = MinSize()+(Transfer::Size()*xfer_count_);
    if (serial.size() < tx_size) {
      LOG_WARNING << "Invalid serialized transaction, wrong size!";
    }
    canonical_.insert(canonical_.end(), serial.begin()
        , serial.begin()+tx_size);
    if (getOperation() > 3) {
      LOG_WARNING << "Invalid serialized transaction, invalid operation!";
      return;
    }
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized transaction, not sound!";
    }
  }

  explicit Transaction(const std::vector<byte>& serial, size_t& offset
    , const KeyRing& keys)
    : xfer_count_(0), canonical_(), is_sound_(false) {
    if (serial.size() < offset+MinSize()) {
      LOG_WARNING << "Invalid serialized transaction, too small!";
      return;
    }
    xfer_count_ = BinToUint64(serial, offset);
    size_t tx_size = MinSize()+(Transfer::Size()*xfer_count_);
    if (serial.size() < offset+tx_size) {
      LOG_WARNING << "Invalid serialized transaction, wrong size!";
    }
    canonical_.insert(canonical_.end(), serial.begin()+offset
        , serial.begin()+(offset+tx_size));
    offset += tx_size;
    if (getOperation() > 3) {
      LOG_WARNING << "Invalid serialized transaction, invalid operation!";
      return;
    }
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized transaction, not sound!";
    }
  }

  Transaction(uint64_t xfer_count, byte oper
      , const std::vector<Transfer>& xfers, uint64_t nonce, Signature sig
      , const KeyRing& keys)
      : xfer_count_(xfer_count), canonical_(), is_sound_(false) {
    canonical_.reserve(MinSize()+(Transfer::Size()*xfer_count_));

    Uint64ToBin(xfer_count_, canonical_);
    canonical_.push_back(oper);
    for (auto it = xfers.begin(); it != xfers.end(); ++it) {
      std::vector<byte> xfer_canon(it->getCanonical());
      canonical_.insert(std::end(canonical_), std::begin(xfer_canon)
        , std::end(xfer_canon));
    }
    Uint64ToBin(nonce, canonical_);
    canonical_.insert(std::end(canonical_), std::begin(sig), std::end(sig));
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized transaction, not sound!";
    }
  }

  Transaction(const Transaction& other) : xfer_count_(other.xfer_count_)
    , canonical_(other.canonical_), is_sound_(other.is_sound_) {}

  Transaction(byte oper, const std::vector<Transfer>& xfers, uint64_t nonce
      , EC_KEY* eckey, const KeyRing& keys)
      : xfer_count_(xfers.size()), canonical_(), is_sound_(false) {
    canonical_.reserve(MinSize()+(Transfer::Size()*xfer_count_));

    Uint64ToBin(xfer_count_, canonical_);
    canonical_.push_back(oper);
    for (auto it = xfers.begin(); it != xfers.end(); ++it) {
      std::vector<byte> xfer_canon(it->getCanonical());
      canonical_.insert(std::end(canonical_), std::begin(xfer_canon)
        , std::end(xfer_canon));
    }
    Uint64ToBin(nonce, canonical_);
    std::vector<byte> msg(getMessageDigest());
    Signature sig;
    SignBinary(eckey, dcHash(msg), sig);
    canonical_.insert(std::end(canonical_), std::begin(sig), std::end(sig));
    is_sound_ = isSound(keys);
    if (!is_sound_) {
      LOG_WARNING << "Invalid serialized transaction, not sound!";
    }
  }

  static const size_t MinSize() {
    return 89;
  }

  static const size_t EnvelopeSize() {
    return 17;
  }

  /** Comparison Operators */
  friend bool operator==(const Transaction& a, const Transaction& b)
  {
    return a.canonical_ == b.canonical_;
  }

  friend bool operator!=(const Transaction& a, const Transaction& b)
  {
    return a.canonical_ != b.canonical_;
  }

  /** Assignment Operators */
  Transaction* operator=(Transaction&& other)
  {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return this;
  }

  Transaction* operator=(const Transaction& other)
  {
    if (this != &other) {
      this->canonical_ = other.canonical_;
    }
    return this;
  }

  byte getOperation() const {
    return canonical_[8];
  }

  std::vector<Transfer> getTransfers() const {
    std::vector<Transfer> out;
    for (size_t i=0; i<xfer_count_; ++i) {
      size_t offset = 9+(Transfer::Size()*i);
      Transfer* t = new Transfer(canonical_, offset);
      out.push_back(*t);
    }
    return out;
  }

  uint64_t getNonce() const {
    return BinToUint64(canonical_, 9+(Transfer::Size()*xfer_count_));
  }

  Signature getSignature() const {
    Signature sig;
    std::copy_n(canonical_.begin()+(17+(Transfer::Size()*xfer_count_))
        , kSIG_SIZE, sig.begin());
    return sig;
  }

  /** Checks if this transaction is sound, meaning potentially valid.
   *  If any portion of the transaction is invalid,
   *  the entire transaction is also unsound.
   * @params keys a KeyRing that provides keys for signature verification
   * @return true iff the transaction is sound
   * @return false otherwise
   */
    bool isSound(const KeyRing& keys) const
    {
      CASH_TRY {
        if (is_sound_) return(is_sound_);
        long total = 0;
        byte oper = getOperation();

        if (getNonce() < 1) {
          LOG_WARNING << "Error: nonce is required";
          return(false);
        }

        bool sender_set = false;
        Address sender;

        std::vector<Transfer> xfers = getTransfers();
        for (auto it = xfers.begin(); it != xfers.end(); ++it) {
          int64_t amount = it->getAmount();
          total += amount;
          if ((oper == eOpType::Delete && amount > 0) ||
            (oper != eOpType::Delete && amount < 0) || oper == eOpType::Modify) {
            if (it->getDelay() < 0) {
              LOG_WARNING << "Error: A negative delay is not allowed.";
              return false;
            }
          }
            if (amount < 0) {
              if (sender_set) {
                LOG_WARNING << "Multiple senders in transaction!";
                return false;
              }
              sender = it->getAddress();
              sender_set = true;
            }
        }
        if (total != 0) {
          LOG_WARNING << "Error: transaction amounts are asymmetric. (sum="
              +std::to_string(total)+")";
          return false;
        }

        if ((oper != Exchange) && (!keys.isINN(sender))) {
          LOG_WARNING << "INN transaction not performed by INN!";
          return false;
        }

        EC_KEY* eckey(keys.getKey(sender));
        std::vector<byte> msg(getMessageDigest());
        Signature sig = getSignature();

        if (!VerifyByteSig(eckey, dcHash(msg), sig)) {
          LOG_WARNING << "Error: transaction signature did not validate.\n";
          LOG_DEBUG << "Transaction state is: "+getJSON();
          LOG_DEBUG << "Sender addr is: "
            +toHex(std::vector<byte>(std::begin(sender), std::end(sender)));
          LOG_DEBUG << "Signature is: "
            +toHex(std::vector<byte>(std::begin(sig), std::end(sig)));
          return false;
        }
        return true;
      } CASH_CATCH (const std::exception& e) {
        LOG_WARNING << FormatException(&e, "transaction");
      }
      return false;
    }

/** Checks if this transaction is valid with respect to a chain state.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 * @params state the chain state to validate against
 * @params keys a KeyRing that provides keys for signature verification
 * @params summary the Summary to update
 * @return true iff the transaction is valid
 * @return false otherwise
 */
  bool isValid(ChainState& state, const KeyRing& keys, Summary& summary) const
  {
    CASH_TRY {
      if (!isSound(keys)) return false;
      byte oper = getOperation();
      std::vector<Transfer> xfers = getTransfers();
      for (auto it = xfers.begin(); it != xfers.end(); ++it) {
        int64_t amount = it->getAmount();
        uint64_t coin = it->getCoin();
        Address addr = it->getAddress();
        if (amount < 0) {
          if ((oper == Exchange) && (amount > state.getAmount(coin, addr))) {
            LOG_WARNING << "Coins not available at addr.";
            return false;
          }
        }
        SmartCoin next_flow(addr, coin, amount);
        state.addCoin(next_flow);
        summary.addItem(addr, coin, amount, it->getDelay());
      }
      return true;
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "transaction");
    }
    return false;
  }

/** Returns a canonical bytestring representation of this transaction.
 * @return a canonical bytestring representation of this transaction.
*/
  std::vector<byte> getCanonical() const {
    return canonical_;
  }

  /** Returns the message digest bytestring for this transaction.
   * @return the message digest bytestring for this transaction.
  */
  std::vector<byte> getMessageDigest() const {
    std::vector<byte> md(canonical_.begin()
        , canonical_.begin()+(EnvelopeSize()+Transfer::Size()*xfer_count_));
    return md;
  }

  /** Returns the transaction size in bytes.
   * @return the transaction size in bytes.
  */
  size_t getByteSize() const {
    return(canonical_.size());
  }

/** Returns a JSON string representing this transaction.
 * @return a JSON string representing this transaction.
*/
  std::string getJSON() const {
    std::string json("{\""+kXFER_COUNT_TAG+"\":");
    json += std::to_string(xfer_count_)+",";
    json += "\""+kOPER_TAG+"\":"+std::to_string(getOperation())+",";
    json += "\""+kXFER_TAG+"\":[";
    bool isFirst = true;
    std::vector<Transfer> xfers = getTransfers();
    for (auto it = xfers.begin(); it != xfers.end(); ++it) {
      if (isFirst) {
        isFirst = false;
      } else {
        json += ",";
      }
      json += it->getJSON();
    }
    json += "],\""+kNONCE_TAG+"\":"+std::to_string(getNonce())+",";
    Signature sig = getSignature();
    json += "\""+kSIG_TAG+"\":\""
      +toHex(std::vector<byte>(std::begin(sig), std::end(sig)))+"\"}";
    return json;
  }

/** Returns a CBOR byte vector representing this transaction.
 * @return a CBOR byte vector representing this transaction.
*/
  //std::vector<uint8_t> ToCBOR() const;

 private:
  std::vector<byte> canonical_;
  bool is_sound_ = false;
};

} //end namespace Devcash

#endif // DEVCASH_PRIMITIVES_TRANSACTION_H
