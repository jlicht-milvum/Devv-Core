
/*
 * devcash.cpp the main class.  Checks args and hands of to init.
 *
 *  Created on: Dec 8, 2017
 *  Author: Nick Williams
 */

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <functional>
#include <thread>

#include "common/argument_parser.h"
#include "common/devcash_context.h"
#include "concurrency/ValidatorController.h"
#include "modules/BlockchainModule.h"
#include "io/message_service.h"
#include "modules/ThreadedController.h"

using namespace Devcash;

std::unique_ptr<io::TransactionClient> create_transaction_client(const devcash_options& options,
                                                                 zmq::context_t& context) {
  std::unique_ptr<io::TransactionClient> client(new io::TransactionClient(context));
  for (auto i : options.host_vector) {
    client->addConnection(i);
  }
  return client;
}

std::unique_ptr<io::TransactionServer> create_transaction_server(const devcash_options& options,
                                                                 zmq::context_t& context) {
  std::unique_ptr<io::TransactionServer> server(new io::TransactionServer(context,
                                                                          options.bind_endpoint));
  return server;
}

int main(int argc, char* argv[])
{

  init_log();

  try {
    std::unique_ptr<devcash_options> options = parse_options(argc, argv);

    if (!options) {
      exit(-1);
    }

    zmq::context_t context(1);

    DevcashContext this_context(options->node_index
                                , options->shard_index
                                , options->mode
                                , options->inn_keys
                                , options->node_keys
                                , options->wallet_keys
                                , options->sync_port
                                , options->sync_host);
    KeyRing keys(this_context);
    ChainState prior;

    std::unique_ptr<io::TransactionServer> server = create_transaction_server(*options, context);
    std::unique_ptr<io::TransactionClient> peer_client = create_transaction_client(*options, context);

    /*
    // Create loopback client to subscribe to simulator transactions
    std::unique_ptr<io::TransactionClient> loopback_client(new io::TransactionClient(context));
    auto be = options->bind_endpoint;
    std::string this_uri = "";
    try {
      this_uri = "tcp://localhost" + be.substr(be.rfind(":"));
    } catch (std::range_error& e) {
      LOG_ERROR << "Extracting bind number failed: " << be;
    }
    loopback_client->addConnection(this_uri);
*/

    /*
        std::signal(SIGINT, signal_handler);
        std::signal(SIGABRT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        shutdown_handler = [&](int signal) {
          LOG_INFO << "Received signal ("+std::to_string(signal)+").";
          shutdown();
        };
     */

    /*
    ValidatorController controller(*server,
                                 *peer_client,
                                 options->num_validator_threads,
                                 options->num_consensus_threads,
                                 options->tx_batch_size,
                                 keys,
                                 this_context,
                                 prior,
                                 options->mode,
                                 options->stop_file);
*/

    UnrecordedTransactionPool utx_pool(prior, options->mode, 10);

    //auto bcm = BlockchainModule::Create(*server, *peer_client, keys, prior, options->mode

    //ThreadedController<BlockchainModule> devcash_module(validator_module, this_context);

    /**
     * Chrome tracing setup
     */
    if (options->trace_file.empty()) {
      LOG_FATAL << "Trace file is required.";
      exit(-1);
	}
    mtr_init(options->trace_file.c_str());
    mtr_register_sigint_handler();

    MTR_META_PROCESS_NAME("minitrace_test");
    MTR_META_THREAD_NAME("main thread");

    MTR_BEGIN("main", "outer");

    /*
    if (!validator_module.init()) {
      LOG_FATAL << "Basic setup failed";
      return false;
    }
    LOG_INFO << "Basic Setup complete";

    if (!validator_module.performSanityChecks()) {
      LOG_FATAL << "Sanity checks failed";
      return false;
    }
    LOG_INFO << "Sanity checks passed";

    validator_module.RunNode();
*/
    MTR_END("main", "outer");
    mtr_flush();
    mtr_shutdown();

    LOG_INFO << "DevCash Shutting Down";

    return(true);
  } catch (...) {
    std::exception_ptr p = std::current_exception();
    std::string err("");
    err += (p ? p.__cxa_exception_type()->name() : "null");
    LOG_FATAL << "Error: "+err <<  std::endl;
    std::cerr << err << std::endl;
    return(false);
  }
}
