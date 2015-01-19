/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include <memory>
#include <vector>

#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/rudp/managed_connections.h"

#include "maidsafe/routing/client_routing_table.h"
#include "maidsafe/routing/network_statistics.h"
#include "maidsafe/routing/network.h"
#include "maidsafe/routing/parameters.h"
#include "maidsafe/routing/routing.pb.h"
#include "maidsafe/routing/rpcs.h"
#include "maidsafe/routing/service.h"
#include "maidsafe/routing/tests/test_utils.h"
#include "maidsafe/routing/acknowledgement.h"


namespace maidsafe {

namespace routing {

namespace test {

namespace {

typedef boost::asio::ip::udp::endpoint Endpoint;

}  // unnamed namespace

TEST(ServicesTest, BEH_Ping) {
  Address Address(RandomString(Address::kSize));
  RoutingTable routing_table(false, Address, asymm::GenerateKeyPair());
  ClientRoutingTable client_routing_table(routing_table.kAddress());
  AsioService asio_service(1);
  Acknowledgement acknowledgement(Address, asio_service);
  Network network(routing_table, client_routing_table, acknowledgement);
  PublicKeyHolder public_key_holder(asio_service, network);
  Service service(routing_table, client_routing_table, network, public_key_holder);
  NodeInfo node;
  rudp::ManagedConnections rudp;
  protobuf::PingRequest ping_request;
  // somebody pings us
  protobuf::Message message = rpcs::Ping(routing_table.kAddress(), "me");
  EXPECT_EQ(message.DestinationAddress(), routing_table.kAddress().string());
  EXPECT_TRUE(ping_request.ParseFromString(message.data(0)));  // us
  EXPECT_TRUE(ping_request.IsInitialized());
  // run message through Service
  service.Ping(message);
  EXPECT_EQ(1, message.type());
  EXPECT_EQ(message.request(), false);
  EXPECT_NE(message.data_size(), 0);
  EXPECT_EQ(message.SourceAddress(), routing_table.kAddress().string());
  EXPECT_EQ(message.replication(), 1);
  EXPECT_EQ(message.type(), 1);
  EXPECT_EQ(message.request(), false);
  EXPECT_EQ(message.id(), 0);
  EXPECT_FALSE(message.client_node());
  // EXPECT_FALSE(message.has_relay());
}

TEST(ServicesTest, BEH_FindNodes) {
  Address Address(RandomString(Address::kSize));
  RoutingTable routing_table(false, Address, asymm::GenerateKeyPair());
  Address this_Address(routing_table.kNodeId());
  ClientRoutingTable client_routing_table(routing_table.kAddress());
  AsioService asio_service(1);
  Acknowledgement acknowledgement(Address, asio_service);
  Network network(routing_table, client_routing_table, acknowledgement);
  PublicKeyHolder public_key_holder(asio_service, network);
  Service service(routing_table, client_routing_table, network, public_key_holder);
  protobuf::Message message = rpcs::FindNodes(this_Address, this_node_id, 8);
  service.FindNodes(message);
  protobuf::FindNodesResponse find_nodes_respose;
  EXPECT_TRUE(find_nodes_respose.ParseFromString(message.data(0)));
  //  EXPECT_TRUE(find_nodes_respose.nodes().size() > 0);  // will only have us
  //  EXPECT_EQ(find_nodes_respose.nodes().Get(1), us.Address.string());
  EXPECT_TRUE(find_nodes_respose.has_timestamp());
  EXPECT_TRUE(find_nodes_respose.timestamp() > GetTimeStamp() - 2000);
  EXPECT_TRUE(find_nodes_respose.timestamp() < GetTimeStamp() + 1000);
  EXPECT_EQ(message.DestinationAddress(), this_Address.string());
  EXPECT_EQ(message.SourceAddress(), this_Address.string());
  EXPECT_NE(message.data_size(), 0);
  EXPECT_TRUE(message.direct());
  EXPECT_EQ(message.replication(), 1);
  EXPECT_EQ(message.type(), 3);
  EXPECT_EQ(message.request(), false);
  //  EXPECT_EQ(message.id(), 0);
  EXPECT_FALSE(message.client_node());
  // EXPECT_FALSE(message.has_relay());
}

// TEST(ServicesTest, BEH_ProxyConnect) {
//   asymm::Keys my_keys;
//   my_keys.identity = RandomString(64);
//   asymm::Keys keys;
//   keys.identity = RandomString(64);
//   RoutingTable routing_table(keys, false);
//   ClientRoutingTable client_routing_table(keys);
//   AsioService asio_service(0);
//   Timer timer(asio_service);
//   NodeInfo node;
//   Network network(routing_table, client_routing_table, timer);
//   protobuf::ProxyConnectRequest proxy_connect_request;
//   // they send us an proxy connect rpc
//   rudp::EndpointPair endpoint_pair;
//   endpoint_pair.external =  Endpoint(boost::asio::ip::address_v4::loopback(), GetRandomPort());
//   endpoint_pair.local =  Endpoint(boost::asio::ip::address_v4::loopback(), GetRandomPort());
//   protobuf::Message message = rpcs::ProxyConnect(Address(keys.identity),
//   Address(my_keys.identity),
//                                                  endpoint_pair);
//   EXPECT_TRUE(message.DestinationAddress() == keys.identity);
//   EXPECT_TRUE(proxy_connect_request.ParseFromString(message.data(0)));  // us
//   EXPECT_TRUE(proxy_connect_request.IsInitialized());
//   // run message through Service
//   service::ProxyConnect(routing_table, network, message);
//   protobuf::ProxyConnectResponse proxy_connect_respose;
//   EXPECT_TRUE(proxy_connect_respose.ParseFromString(message.data(0)));
//   EXPECT_EQ(protobuf::kFailure, proxy_connect_respose.result());
//   EXPECT_NE(message.data_size(), 0);
//   EXPECT_TRUE(message.direct());
//   EXPECT_TRUE(message.SourceAddress() == keys.identity);
//   EXPECT_EQ(1, message.replication());
//   EXPECT_EQ(4, message.type());
//   EXPECT_EQ(message.request(), false);
//   EXPECT_EQ(0, message.id());
//   EXPECT_FALSE(message.client_node());
//   // EXPECT_FALSE(message.has_relay());
//   // TODO(Prakash): Need to add peer to connect and test for kSuccess & kAlreadyConnected.
// }

}  // namespace test

}  // namespace routing

}  // namespace maidsafe