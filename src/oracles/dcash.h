/*
 * dcash.h is an oracle to validate basic dcash transactions.
 *
 *  Created on: Feb 23, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLES_DCASH_H_
#define ORACLES_DCASH_H_

#include <string>

#include "dnerowallet.h"
#include "oracleInterface.h"
#include "common/logger.h"
#include "consensus/chainstate.h"

using namespace Devcash;

class dcash : public oracleInterface {

 public:

  /**
   *  @return string that invokes this oracle in a T2 transaction
  */
  static std::string getCoinType() {
    return("dcash");
  }

  /**
   *  @return int internal index of this coin type
  */
  static int getCoinIndex() {
    return(0);
  }

  /** Checks if a transaction is objectively valid according to this oracle.
   *  When this function returns false, a transaction is syntactically invalid
   *  and will be invalid for all chain states.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  dcash transactions must be immediate with a delay of 0.
   *
   * @params checkTx the transaction to (in)validate
   * @return true iff the transaction can be valid according to this oracle
   * @return false otherwise
   */
  bool isSound(Transaction checkTx) {
    std::vector<Transfer> xfers = checkTx.getTransfers();
    for (auto it=xfers.begin(); it != xfers.end(); ++it) {
      if (it->getDelay() != 0) {
        LOG_WARNING << "Error: Delays are not allowed for dcash.";
        return false;
      }
    } //end for each transfer
    return true;
  }

  /** Checks if a transaction is valid according to this oracle
   *  given a specific chain state.
   *  Transactions are atomic, so if any portion of the transaction is invalid,
   *  the entire transaction is also invalid.
   *
   *  A dnerowallet token indicates that this address may not send dcash.
   *
   * @params checkTx the transaction to (in)validate
   * @params context the chain state to check against
   * @return true iff the transaction is valid according to this oracle
   * @return false otherwise
   */
  bool isValid(Transaction checkTx, ChainState& context) {
    if (!isSound(checkTx)) return false;
    std::vector<Transfer> xfers = checkTx.getTransfers();
    for (auto it=xfers.begin(); it != xfers.end(); ++it) {
      if (it->getAmount() < 0) {
        Address addr = it->getAddress();
        if (context.getAmount(dnerowallet::getCoinIndex(), addr) > 0) {
          LOG_WARNING << "Error: Dnerowallets may not send dcash.";
          return false;
        } //endif has dnerowallet
      } //endif sender
    } //end transfer loop
    return true;
  }

/** Generate a tier 1 smartcoin transaction based on this tier 2 transaction.
 *
 * @pre This transaction must be valid.
 * @params checkTx the transaction to (in)validate
 * @return a tier 1 transaction to implement this tier 2 logic.
 */
  Transaction getT1Syntax(Transaction theTx) {
    return theTx;
  }

  /**
   * End-to-end tier2 process that takes a string, parses it into a transaction,
   * validates it against the provided chain state,
   * and returns a tier 1 transaction if it is valid.
   * Returns null if the transaction is invalid.
   *
   * @params rawTx the raw transaction to process
   * @params context the chain state to check against
   * @return a tier 1 transaction to implement this tier 2 logic.
   * @return empty/null transaction if the transaction is invalid
   */
  Tier2Transaction Tier2Process(std::vector<byte> rawTx,
        ChainState context, const KeyRing& keys) {
      Tier2Transaction tx(rawTx, keys);
      if (!isValid(tx, context)) {
        return tx;
      }
      return tx;
    }

};

#endif /* ORACLES_DCASH_H_ */
