/*
 * primitives/block_tools.h
 *
 *  Created on: July 2, 2018
 *      Author: Shawn McKenney
 */
#include "block_tools.h"

namespace Devcash {

bool IsBlockData(const std::vector<byte>& raw) {
  //check if big enough
  if (raw.size() < FinalBlock::MinSize()) { return false; }
  //check version
  if (raw[0] != 0x00) { return false; }
  size_t offset = 9;
  uint64_t block_time = BinToUint64(raw, offset);
  // check blocktime is from 2018 or newer.
  if (block_time < 1514764800) { return false; }
  // check blocktime is in past
  if (block_time > GetMillisecondsSinceEpoch()) { return false; }
  return true;
}

bool IsTxData(const std::vector<byte>& raw) {
  // check if big enough
  if (raw.size() < Transaction::MinSize()) { return false; }
  // check transfer count
  uint64_t xfer_count = BinToUint64(raw, 0);
  size_t tx_size = Transaction::MinSize() + (Transfer::Size() * xfer_count);
  if (raw.size() < tx_size) { return false; }
  // check operation
  if (raw[8] >= 4) { return false; }
  return true;
}

bool CompareChainStateMaps(const std::map<Address, std::map<uint64_t, int64_t>>& first,
                           const std::map<Address, std::map<uint64_t, int64_t>>& second) {
  if (first.size() != second.size()) { return false; }
  for (auto i = first.begin(), j = second.begin(); i != first.end(); ++i, ++j) {
    if (i->first != j->first) { return false; }
    if (i->second.size() != j->second.size()) { return false; }
    for (auto x = i->second.begin(), y = j->second.begin(); x != i->second.end(); ++x, ++y) {
      if (x->first != y->first) { return false; }
      if (x->second != y->second) { return false; }
    }
  }
  return true;
}

std::string WriteChainStateMap(const std::map<Address, std::map<uint64_t, int64_t>>& map) {
  std::string out("{");
  bool first_addr = true;
  for (auto e : map) {
    if (first_addr) {
      first_addr = false;
    } else {
      out += ",";
    }
    Address a = e.first;
    out += "\"" + ToHex(std::vector<byte>(std::begin(a), std::end(a))) + "\":[";
    bool is_first = true;
    for (auto f : e.second) {
      if (is_first) {
        is_first = false;
      } else {
        out += ",";
      }
      out += std::to_string(f.first) + ":" + std::to_string(f.second);
    }
    out += "]";
  }
  out += "}";
  return out;
}

} // namespace Devcash
