/*
 * validation.h defines an interface for primitize Devcash oracles.
 * Devcash oracles further specify validation logic for particular
 * transaction types beyond the 'abstract' smartcoin transactions.
 * Note that smartcoin is not abstract as a C++ class,
 * rather it is a logical abstraction for all tokenized transactions.
 *
 *  Created on: Feb 23, 2018
 *  Author: Nick Williams
 *
 */

#ifndef ORACLE_INTERFACE_H_
#define ORACLE_INTERFACE_H_

#include <string>
#include "../consensus/chainstate.h"
#include "../primitives/transaction.h"

class oracleInterface {

 public:

  /** Constructor/Destructor */
  oracleInterface() {};
  virtual ~oracleInterface() {};

/**
 *  @return string that invokes this oracle in a T2 transaction
 */
  static std::string getCoinType() {
    return("SmartCoin");
  };

/** Checks if a transaction is objectively valid according to this oracle.
 *  When this function returns false, a transaction is syntactically invalid
 *  and will be invalid for all chain states.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 * @params checkTx the transaction to (in)validate
 * @return true iff the transaction can be valid according to this oracle
 * @return false otherwise
 */
  virtual bool isValid(Devcash::DCTransaction checkTx) = 0;

/** Checks if a transaction is valid according to this oracle
 *  given a specific chain state.
 *  Transactions are atomic, so if any portion of the transaction is invalid,
 *  the entire transaction is also invalid.
 * @params checkTx the transaction to (in)validate
 * @params context the chain state to check against
 * @return true iff the transaction is valid according to this oracle
 * @return false otherwise
 */
  virtual bool isValid(Devcash::DCTransaction checkTx,
      Devcash::DCState& context) = 0;

/** Generate a tier 1 smartcoin transaction based on this tier 2 transaction.
 *
 * @pre This transaction must be valid.
 * @params checkTx the transaction to (in)validate
 * @return a tier 1 transaction to implement this tier 2 logic.
 */
  virtual Devcash::DCTransaction getT1Syntax(Devcash::DCTransaction theTx) = 0;

/**
 * End-to-end tier2 process that takes a string, parses it into a transaction,
 * validates it against the provided chain state,
 * and returns a tier 1 transaction if it is valid.
 * Returns null if the transaction is invalid.
 *
 * @params rawTx the raw transaction to process
 * @params context the chain state to check against
 * @return a tier 1 transaction to implement this tier 2 logic
 * @return nullptr if the transaction is invalid
 */
  virtual Devcash::DCTransaction Tier2Process(std::string rawTx,
      Devcash::DCState context) = 0;

};

#endif /* ORACLE_INTERFACE_H_ */