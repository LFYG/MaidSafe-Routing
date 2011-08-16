/* Copyright (c) 2009 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MAIDSAFE_DHT_KADEMLIA_NODE_IMPL_STRUCTS_H_
#define MAIDSAFE_DHT_KADEMLIA_NODE_IMPL_STRUCTS_H_

#include <memory>
#include <string>

#include "boost/thread/mutex.hpp"
#ifdef __MSVC__
#  pragma warning(push)
#  pragma warning(disable: 4244)
#endif
#include "boost/multi_index_container.hpp"
#include "boost/multi_index/composite_key.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/identity.hpp"
#include "boost/multi_index/member.hpp"
#ifdef __MSVC__
#  pragma warning(pop)
#endif

#include "maidsafe/dht/kademlia/config.h"
#include "maidsafe/dht/kademlia/contact.h"
#include "maidsafe/dht/kademlia/node_id.h"
#include "maidsafe/dht/kademlia/rpcs.h"
#include "maidsafe/dht/kademlia/utils.h"

namespace maidsafe {

namespace dht {

namespace kademlia {

struct ContactInfo {
  enum RpcState { kNotSent, kSent, kDelayed, kRepliedOK };
  ContactInfo() : providers(), rpc_state(kNotSent) {}
  ContactInfo(const Contact &provider) : providers(1, provider),
                                         rpc_state(kNotSent) {}
  std::vector<Contact> providers;
  RpcState rpc_state;
};

typedef std::map<Contact,
                 ContactInfo,
                 std::function<bool(const Contact&,  // NOLINT (Fraser)
                                    const Contact&)>> LookupContacts;
typedef std::map<Contact, ContactInfo> Downlist;

struct LookupArgs {
  enum OperationType {
    kFindNodes,
    kFindValue,
    kStore,
    kDelete,
    kUpdate,
    kGetContact
  };
  LookupArgs(OperationType operation_type,
             const NodeId &target,
             const OrderedContacts &close_contacts,
             const uint16_t &num_contacts_requested,
             SecurifierPtr securifier_in)
      : lookup_contacts(std::bind(static_cast<bool(*)(const Contact&,
            const Contact&, const NodeId&)>(&CloserToTarget),
            arg::_1, arg::_2, target)),
        downlist(),
        cache_candidate(),
        mutex(),
        total_lookup_rpcs_in_flight(0),
        rpcs_in_flight_for_current_iteration(0),
        lookup_phase_complete(false),
        kOperationType(operation_type),
        kTarget(target),
        kNumContactsRequested(num_contacts_requested),
        securifier(securifier_in) {
    auto insert_itr(lookup_contacts.end());
    for (auto it(close_contacts.begin()); it != close_contacts.end(); ++it) {
      insert_itr = lookup_contacts.insert(insert_itr,
                                          std::make_pair((*it), ContactInfo()));
    }
  }
  virtual ~LookupArgs() {}
  LookupContacts lookup_contacts;
  Downlist downlist;
  Contact cache_candidate;
  boost::mutex mutex;
  int total_lookup_rpcs_in_flight, rpcs_in_flight_for_current_iteration;
  bool lookup_phase_complete;
  const OperationType kOperationType;
  const NodeId kTarget;
  const uint16_t kNumContactsRequested;
  SecurifierPtr securifier;
};

struct FindNodesArgs : public LookupArgs {
  FindNodesArgs(const NodeId &target,
                const uint16_t &num_contacts_requested,
                const OrderedContacts &close_contacts,
                SecurifierPtr securifier,
                FindNodesFunctor callback_in)
      : LookupArgs(kFindNodes, target, close_contacts, num_contacts_requested,
                   securifier),
        callback(callback_in) {}
  FindNodesFunctor callback;
};

struct FindValueArgs : public LookupArgs {
  FindValueArgs(const NodeId &target,
                const uint16_t &num_contacts_requested,
                const OrderedContacts &close_contacts,
                SecurifierPtr securifier,
                FindValueFunctor callback_in)
      : LookupArgs(kFindValue, target, close_contacts, num_contacts_requested,
                   securifier),
        callback(callback_in) {}
  FindValueFunctor callback;
};

struct StoreArgs : public LookupArgs {
  StoreArgs(const NodeId &target,
            const uint16_t &num_contacts_requested,
            const OrderedContacts &close_contacts,
            const size_t &success_threshold_in,
            const std::string &value_in,
            const std::string &signature_in,
            const bptime::time_duration &ttl_in,
            SecurifierPtr securifier,
            StoreFunctor callback_in)
      : LookupArgs(kStore, target, close_contacts, num_contacts_requested,
                   securifier),
        success_threshold(success_threshold_in),
        value(value_in),
        signature(signature_in),
        ttl(ttl_in),
        callback(callback_in) {}
  size_t success_threshold;
  std::string value, signature;
  bptime::time_duration ttl;
  StoreFunctor callback;
};

struct DeleteArgs : public LookupArgs {
  DeleteArgs(const NodeId &target,
             const uint16_t &num_contacts_requested,
             const OrderedContacts &close_contacts,
             const size_t &success_threshold_in,
             const std::string &value_in,
             const std::string &signature_in,
             SecurifierPtr securifier,
             DeleteFunctor callback_in)
      : LookupArgs(kDelete, target, close_contacts, num_contacts_requested,
                   securifier),
        success_threshold(success_threshold_in),
        value(value_in),
        signature(signature_in),
        callback(callback_in) {}
  size_t success_threshold;
  std::string value, signature;
  DeleteFunctor callback;
};

struct UpdateArgs : public LookupArgs {
  UpdateArgs(const NodeId &target,
             const uint16_t &num_contacts_requested,
             const OrderedContacts &close_contacts,
             const size_t &success_threshold_in,
             const std::string &old_value_in,
             const std::string &old_signature_in,
             const std::string &new_value_in,
             const std::string &new_signature_in,
             const bptime::time_duration &ttl_in,
             SecurifierPtr securifier,
             UpdateFunctor callback_in)
      : LookupArgs(kUpdate, target, close_contacts, num_contacts_requested,
                   securifier),
        success_threshold(success_threshold_in),
        old_value(old_value_in),
        old_signature(old_signature_in),
        new_value(new_value_in),
        new_signature(new_signature_in),
        ttl(ttl_in),
        callback(callback_in) {}
  size_t success_threshold;
  std::string old_value;
  std::string old_signature;
  std::string new_value;
  std::string new_signature;
  bptime::time_duration ttl;
  UpdateFunctor callback;
};

struct GetContactArgs : public LookupArgs {
  GetContactArgs(const NodeId &target,
                 const uint16_t &num_contacts_requested,
                 const OrderedContacts &close_contacts,
                 SecurifierPtr securifier,
                 GetContactFunctor callback_in)
      : LookupArgs(kGetContact, target, close_contacts, num_contacts_requested,
                   securifier),
        callback(callback_in) {}
  GetContactFunctor callback;
};

typedef std::shared_ptr<LookupArgs> LookupArgsPtr;
typedef std::shared_ptr<FindNodesArgs> FindNodesArgsPtr;
typedef std::shared_ptr<FindValueArgs> FindValueArgsPtr;
typedef std::shared_ptr<StoreArgs> StoreArgsPtr;
typedef std::shared_ptr<DeleteArgs> DeleteArgsPtr;
typedef std::shared_ptr<UpdateArgs> UpdateArgsPtr;
typedef std::shared_ptr<GetContactArgs> GetContactArgsPtr;

}  // namespace kademlia

}  // namespace dht

}  // namespace maidsafe

#endif  // MAIDSAFE_DHT_KADEMLIA_NODE_IMPL_STRUCTS_H_
