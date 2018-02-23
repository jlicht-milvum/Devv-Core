/*
 * chainstate.h holds the state of the chain for each validating fork
 *
 *  Created on: Jan 12, 2018
 *  Author: Nick Williams
 */

#ifndef SRC_CONSENSUS_STATESTUB_H_
#define SRC_CONSENSUS_STATESTUB_H_

#include "../primitives/smartcoin.h"
#include <map>

namespace Devcash
{

class DCState {
public:
  std::map<std::string, std::map<std::string, long>> stateMap_;

/** Constructor */
  DCState();

/** Adds a coin to the state.
 *  @param reference to the coin to add
 *  @return true if the coin was added successfully
 *  @return false otherwise
*/
  bool addCoin(SmartCoin& coin);

/** Gets the number of coins at a particular location.
 *  @param type the coin type to check
 *  @param the address to check
 *  @return the number of this type of coins at this address
*/
  long getAmount(std::string type, std::string addr);

/** Moves a coin from one address to another
 *  @param start references where the coins will be removed
 *  @param end references where the coins will be added
 *  @return true if the coins were moved successfully
 *  @return false otherwise
*/
  bool moveCoin(SmartCoin& start, SmartCoin& end);

/** Deletes a coin from the state.
 *  @param reference to the coin to delete
 *  @return true if the coin was deleted successfully
 *  @return false otherwise
*/
  bool delCoin(SmartCoin& coin);

/** Clears this chain state.
 *  @return true if the state cleared successfully
 *  @return false otherwise
*/
  bool clear();
};

} //namespace Devcash

#endif /* SRC_CONSENSUS_STATESTUB_H_ */
