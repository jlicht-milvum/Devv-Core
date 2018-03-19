/*
 * DevcashWorkerPool.h acts as a wrapper around DevcashRingQueue.
 * This wrapper provides a pattern to use DevcashRingQueue safely.
 * Consumers of DevcashMessages can use this worker pool to
 * spawn threads that callback with an arbitrary DevcashMessage
 * to a member function of the Consumer class that can call on
 * other Consumer members as needed.
 * Producers of Devcash messages need post() DevcashMessage, so
 * this can wrap them in a unique_ptr and allocate them via the
 * ring queue.
 *
 * How to create a consumer:
 * class ExampleConsumer {
 *  public:
 *   int someMemberState_;
 *   ExampleConsumer(int myVal) : someMemberState_(myVal) {}
 *   void run(DevcashMessage message) {
 *     someMemberState_++;
 *   }
 * };
 *
 * From the main thread:
 * ExampleConsumer consumer;
 * std::function<void(DevcashMessage)> callback =
 *   boost::bind(&ExampleConsumer::run, consumer, _1);
 * DevcashWorkerPool consumerPool(callback);
 * DevcashMessage m("uri", eMessageType::VALID, std::vector<uint8_t>());
 * consumerPool.post(m); //sends m to an ExampleConsumer
 *
 *  Created on: Mar 14, 2018
 *      Author: Nick Williams
 */

#ifndef QUEUES_DEVCASHWORKERPOOL_H_
#define QUEUES_DEVCASHWORKERPOOL_H_

#include <functional>
#include <thread>

#include <boost/thread/thread.hpp>

#include "types/DevcashMessage.h"
#include "common/logger.h"
#include "common/util.h"

#include "DevcashRingQueue.h"

namespace Devcash {

//exception toggling capability
#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && not defined(DEVCASH_NOEXCEPTION)
    #define CASH_THROW(exception) throw exception
    #define CASH_TRY try
    #define CASH_CATCH(exception) catch(exception)
#else
    #define CASH_THROW(exception) std::abort()
    #define CASH_TRY if(true)
    #define CASH_CATCH(exception) if(false)
#endif

static const int kDEFAULT_WORKERS = 10;

class DevcashWorkerPool {
 public:

  /** The internal callback from boost.
   * Pops unique_ptr from the queue and passes the DevcashMessage to
   * the registered consumer callback function.
   * @note consumer is now fully responsible for the DevcashMessage.
   */
  void theLoop() {
    CASH_TRY {
      while (continue_) {
        std::unique_ptr<DevcashMessage> nextMsg(trigger_.pop());
        callback_(*nextMsg.get());
      }
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "DevcashWorkerPool.theLoop");
    }
  }

  /* Constructors/Destructors */
  DevcashWorkerPool(std::function<void(DevcashMessage)> callback,
      const int workers=kDEFAULT_WORKERS) :
      callback_(callback), continue_(true) {
    for (int w = 0; w < workers; w++) {
      pool_.create_thread(boost::bind(&Devcash::DevcashWorkerPool::theLoop, this));
    }
  }

  virtual ~DevcashWorkerPool() {
    continue_ = false;
    pool_.join_all();
  }

  /* Disallow copy and assign */
  DevcashWorkerPool(DevcashWorkerPool const&) = delete;
  DevcashWorkerPool& operator=(DevcashWorkerPool const&) = delete;

  /** Stops all threads in this pool.
   * @note This function may block.
   * @return true iff all threads in this pool joined.
   * @return false if some error occurred.
   */
  bool stopAll() {
	CASH_TRY {
	  continue_ = false;
      pool_.join_all();
      return true;
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "DevcashWorkerPool.stopAll");
      return false;
    }
  }

  /** Post a DevcashMessage to the ring queue.
   * DevcashWorkerPool will create a unique_ptr for the queue safely.
   * @note This function blocks when the ring queue is full!
   * @param message the DevcashMessage to push.
   */
  void push(DevcashMessage message) {
    CASH_TRY {
      DevcashMessage* msg = &message;
      std::unique_ptr<DevcashMessage> ptr(msg);
      trigger_.push(std::move(ptr));
    } CASH_CATCH (const std::exception& e) {
      LOG_WARNING << FormatException(&e, "DevcashWorkerPool.push");
    }
  }

 private:
  boost::thread_group pool_;
  DevcashRingQueue trigger_; //the ring queue triggers callbacks
  std::function<void(DevcashMessage)> callback_; //the callback for this pool
  std::atomic<bool> continue_;  //signals all threads to stop gracefully

};

} /* namespace Devcash */

#endif /* QUEUES_DEVCASHWORKERPOOL_H_ */