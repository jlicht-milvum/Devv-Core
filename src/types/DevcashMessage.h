/*
 * DevcashMessage.h
 *
 *  Created on: Mar 15, 2018
 */

#ifndef TYPES_DEVCASHMESSAGE_H_
#define TYPES_DEVCASHMESSAGE_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#include "common/logger.h"

namespace Devcash {

enum eMessageType {
  FINAL_BLOCK = 0,
  PROPOSAL_BLOCK = 1,
  TRANSACTION_ANNOUNCEMENT = 2,
  VALID = 3,
  REQUEST_BLOCK = 4,
  NUM_TYPES = 5
};

typedef std::string URI;

struct DevcashMessage {
  URI uri;
  eMessageType message_type;
  std::vector<uint8_t> data;
  uint32_t index;

  DevcashMessage() : uri(""), message_type(eMessageType::VALID), data(), index(0) {}
  DevcashMessage(URI uri, eMessageType msgType, std::vector<uint8_t>& data, int index=0) :
    uri(uri), message_type(msgType), data(data), index(index) {}

  /**
   * Constructor. Takes a string to initialize data vector
   */
  DevcashMessage(const URI& uri, eMessageType msgType, const std::string& data, int index=0) :
    uri(uri), message_type(msgType), data(data.begin(), data.end()), index(index) {}
};

typedef std::unique_ptr<DevcashMessage> DevcashMessageUniquePtr;

/**
 * Append to serialized buffer
 */
template <typename T>
inline void append(std::vector<uint8_t>& buf, const T& obj) {
  unsigned int pos = buf.size();
  buf.resize(pos + sizeof(obj));
  memcpy(&buf[pos], &obj, sizeof(obj));
}

/**
 * Append to serialized buffer. Specialize for string
 */
template <>
inline void append(std::vector<uint8_t>& buf, const std::string& obj) {
  // Serialize the size of the string
  unsigned int size = obj.size();
  append(buf, size);
  // Serialize the string
  std::copy(obj.begin(), obj.end(), std::back_inserter(buf));
}

/**
 * Append to serialized buffer. Specialize for data buf
 */
template <>
inline void append(std::vector<uint8_t>& buf, const std::vector<uint8_t>& obj) {
  // Serialize the size of the vector
  unsigned int size = obj.size();
  append(buf, size);
  // Serialize the vector
  buf.insert(buf.end(), obj.begin(), obj.end());
}

/**
 * Serialize a messag into a buffer
 */
static std::vector<uint8_t> serialize(const DevcashMessage& msg) {
  // Arbitrary header
  uint8_t header_version = 41;
  // Serialized buffer
  std::vector<uint8_t> bytes;

  // Serialize a header. Stripped during deserialization
  append(bytes, header_version);
  // Serialize the index
  append(bytes, msg.index);
  // Serialize the message type
  append(bytes, msg.message_type);
  // Serialize the URI
  append(bytes, msg.uri);
  // Serialize the data buffer
  append(bytes, msg.data);

  return bytes;
}

/**
 * Extract data from serialized buffer.
 * @return Index of buffer after extraction
 */
template <typename T>
inline unsigned int extract(T& dest, const std::vector<uint8_t>& buffer, unsigned int index) {
  assert(buffer.size() >= index + sizeof(dest));
  dest = static_cast<T>(buffer[index]);
  return(index + sizeof(dest));
}

/**
 * Extract data from serialized buffer. Specialized for std::string
 * @return Index of buffer after extraction
 */
template <>
inline unsigned int extract(std::string& dest,
                            const std::vector<uint8_t>& buffer,
                            unsigned int index) {
  unsigned int size;
  unsigned int new_index = extract(size, buffer, index);
  assert(buffer.size() >= new_index + size);
  dest.append(reinterpret_cast<const char*>(&buffer[new_index]), size);
  return(new_index + size);
}


/**
 * Extract data from serialized buffer. Specialized for std::vector<uint8_t>
 * @return Index of buffer after extraction
 */
template <>
inline unsigned int extract(std::vector<uint8_t>& dest,
                            const std::vector<uint8_t>& buffer,
                            unsigned int index) {
  unsigned int size;
  unsigned int new_index = extract(size, buffer, index);
  assert(buffer.size() >= new_index + size);
  dest.insert(dest.end(), buffer.begin() + new_index, buffer.begin() + new_index + size);
 return(new_index + size);
}

/**
 * Deserialize a a buffer into a devcash message
 */
static DevcashMessageUniquePtr deserialize(const std::vector<uint8_t>& bytes) {
  unsigned int buffer_index = 0;
  uint8_t header_version = 0;

  // Create the devcash message
  auto message = std::make_unique<DevcashMessage>();

  // Get the header_version
  buffer_index = extract(header_version, bytes, buffer_index);
  assert(header_version == 41);
  LOG_DEBUG << "header_version: " << static_cast<int>(header_version) << " buffer_index " << buffer_index;
  // index
  buffer_index = extract(message->index, bytes, buffer_index);
  LOG_DEBUG << "index: " <<  message->index << " buffer_index " << buffer_index;
  // message type
  buffer_index = extract(message->message_type, bytes, buffer_index);
  LOG_DEBUG << "message_type: " <<  message->message_type << " buffer_index " << buffer_index;
  // URI
  buffer_index = extract(message->uri, bytes, buffer_index);
  LOG_DEBUG << "uri: " <<  message->uri << " buffer_index " << buffer_index;
  // data
  buffer_index = extract(message->data, bytes, buffer_index);

  return(message);
}

/**
 * Stream the message to the logger
 */
static void LogDevcashMessageSummary(const DevcashMessage& message) {

  std::string message_type_string;
  switch (message.message_type) {
  case(eMessageType::FINAL_BLOCK):
    message_type_string = "FINAL_BLOCK";
    break;
  case(eMessageType::PROPOSAL_BLOCK):
    message_type_string = "PROPOSAL_BLOCK";
    break;
  case(eMessageType::TRANSACTION_ANNOUNCEMENT):
    message_type_string = "TRANSACTION_ANNOUNCEMENT";
    break;
  case(eMessageType::VALID):
    message_type_string = "VALID";
    break;
  case(eMessageType::REQUEST_BLOCK):
    message_type_string = "REQUEST_BLOCK";
    break;
  default:
    message_type_string = "ERROR_DEFAULT";
    break;
  }

  LOG_INFO << "DevcashMessage: " <<
    "URI: " << message.uri << " | " <<
    "TYPE: " << message_type_string << " | " <<
    "SIZE: " << message.data.size() << " | " <<
    "INDEX: " << message.index;
}

} /* namespace Devcash */

#endif /* TYPES_DEVCASHMESSAGE_H_ */
