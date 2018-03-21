/*
 * devnode.h manages this node in the devcash network.
 *
 *  Created on: Mar 20, 2018
 *      Author: Nick Williams
 */

#ifndef DEVCASHNODE_H_
#define DEVCASHNODE_H_

#ifndef DEVCASH_INIT_H
#define DEVCASH_INIT_H

#include <string>
#include <vector>

#include "common/devcash_context.h"

namespace Devcash
{

class DevcashNode {
 public:
  DevcashContext appContext;
  /** Begin stopping threads and shutting down. */
  void StartShutdown();

  /** Check if system is shutting down.
   *  @return true iff shutdown process has begun
   *  @return false otherwise
  */
  bool ShutdownRequested();

  /** Shut down immediately. */
  void Shutdown();

  /** Construct a context object for this node.
   *  @param mode either T1, T2, or scan
   *  @param nodeIndex the index of this node in the set of shard peers
   *  @return the context for this node.
   */
  DevcashNode(std::string mode, int nodeIndex);

  /** Initialize devcoin core: Basic context setup.
   *  @note Do not call Shutdown() if this function fails.
   *  @pre Parameters should be parsed and config file should be read.
   */
  bool Init();

  /**
   * Initialization sanity checks: ecc init, sanity checks, dir lock.
   * @note Do not call Shutdown() if this function fails.
   * @pre Parameters should be parsed and config file should be read.
   */
  bool SanityChecks();

  /**
   * Runs the scanner over the input.
   * @pre Parameters should be parsed and config file should be read.
   */
  std::string RunScanner(std::string inStr);

  /**
   * Devcash core main initialization.
   * @note Call Shutdown() if this function fails.
   * @param inStr raw string of input transaction or block
   * @param mode either T1 or T2
   */
  std::string RunNode(std::string inStr);
};

} //end namespace Devcash

#endif // DEVCASH_INIT_H


#endif /* DEVCASHNODE_H_ */
