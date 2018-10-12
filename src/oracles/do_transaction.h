/*
 * hex_transaction.h is a testnet oracle that reconstructs a (Tier2) transaction and signature
 * from a hexadecimal representation and announces that transaction.
 *
 * Proposal format is hex transaction
 *
 * @copywrite  2018 Devvio Inc
 *
 */

#ifndef ORACLES_HEX_TRANSACTION_H_
#define ORACLES_HEX_TRANSACTION_H_

#include <string>

#include "oracleInterface.h"
#include "consensus/chainstate.h"
#include "consensus/KeyRing.h"

using namespace Devv;

class DoTransaction : public oracleInterface {

 public:


DoTransaction(std::string data) : oracleInterface(data) {};

/**
 *  @return the string name that invokes this oracle
 */
  static std::string getOracleName() {
    return("io.devv.hex_transaction");
  }

/**
 *  @return the shard used by this oracle
 */
  static uint64_t getShardIndex() {
    return(1);
  }

/**
 *  @return the coin type used by this oracle
 */
  static uint64_t getCoinIndex() {
    return(0);
  }

/** Checks if this proposal is objectively sound according to this oracle.
 *  When this function returns false, the proposal is syntactically unsound
 *  and will be invalid for all chain states.
 * @return true iff the proposal can be valid according to this oracle
 * @return false otherwise
 */
  bool isSound() override {
    std::string hex = Bin2Str(getCanonical());
    std::vector<byte> serial = Hex2Bin(hex);
    InputBuffer buffer(serial);
    Tier2Transaction tx = Tier2Transaction::QuickCreate(buffer);
    return true;
  }

/** Checks if this proposal is valid according to this oracle
 *  given a specific blockchain.
 * @params context the blockchain to check against
 * @return true iff the proposal is valid according to this oracle
 * @return false otherwise
 */
  bool isValid(const Blockchain& context) override {
    if (!isSound()) {
		error_msg_ = "Transaction is not sound: hex data: "+Bin2Str(getCanonical());
		return false;
	}
	return true;
  }

/**
 *  @return if not valid or not sound, return an error message
 */
  std::string getErrorMessage() override {
    return(error_msg_);
  }

  //always empty for now
  std::map<uint64_t, std::vector<Tier2Transaction>>
      getTrace(const Blockchain& context) override {
    std::map<uint64_t, std::vector<Tier2Transaction>> out;
    return out;
  }

  uint64_t getCurrentDepth(const Blockchain& context) override {
    //@TODO(nick) scan pre-existing chain for this oracle instance.
    return(0);
  }

  std::map<uint64_t, std::vector<Tier2Transaction>>
      getNextTransactions(const Blockchain& context, const KeyRing& keys) override {
    std::map<uint64_t, std::vector<Tier2Transaction>> out;
    if (!isValid(context)) return out;
    std::vector<byte> serial = Hex2Bin(raw_data);
    InputBuffer buffer(serial);
    Tier2Transaction the_tx = Tier2Transaction::Create(buffer, keys, true);
    std::vector<Tier2Transaction> txs;
    txs.push_back(std::move(the_tx));
    std::pair<uint64_t, std::vector<Tier2Transaction>> p(getShardIndex(), std::move(txs));
    out.insert(std::move(p));
    return out;
  }

/** Recursively generate the state of this oracle and all dependent oracles.
 *
 * @pre This proposal must be valid.
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a map of oracles to data
 */
  std::map<std::string, std::vector<byte>>
      getDecompositionMap(const Blockchain& context) override {
    std::map<std::string, std::vector<byte>> out;
    std::vector<byte> data(Hex2Bin(raw_data_));
    std::pair<std::string, std::vector<byte>> p(getOracleName(), data);
    out.insert(p);
    return out;
  }

/** Recursively generate the state of this oracle and all dependent oracles.
 *
 * @pre This proposal must be valid.
 * @params context the blockchain of the shard that provides context for this oracle
 * @return a map of oracles to data encoded in JSON
 */
  std::map<std::string, std::string>
      getDecompositionMapJSON(const Blockchain& context) override {
    std::map<std::string, std::string> out;
    std::pair<std::string, std::string> p(getOracleName(), getJSON());
    out.insert(p);
    return out;
  }

/** Generate the appropriate signature(s) for this proposal.
 *
 * Oracle data to sign.
 */
  std::vector<byte> Sign() override {
    return Str2Bin(raw_data_);
  }

/**
 * @return the internal state of this oracle in JSON.
 */
  std::string getJSON() override {
    std::string json("{\"hex\":\""+raw_data_+"\"}";
    return json;
  }

private:
 std::string error_msg_;

};

#endif /* ORACLES_DO_TRANSACTION_H_ */
