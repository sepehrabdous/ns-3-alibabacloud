/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#include "rdma-client-helper.h"
#include "ns3/rdma-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

// Default constructor — creates an unconfigured helper.
// Attributes must be set manually via SetAttribute() before calling Install().
RdmaClientHelper::RdmaClientHelper ()
{
}

// Full constructor: configures an RDMA flow from sip:sport -> dip:dport.
//
// Parameters:
//   pg         — priority group (QoS class) for the flow
//   sip/dip    — source and destination IPv4 addresses
//   sport/dport— source and destination UDP ports
//   size       — total number of bytes to write (RDMA message size)
//   win        — congestion-control window size (bytes)
//   baseRtt    — base round-trip time used by the CC algorithm (ns)
//   msg_handler— callback invoked when the message transfer completes
//   fun_arg    — opaque argument forwarded to msg_handler
//   tag        — collective tag used to match sender/receiver in AstraSim
//   src/dest   — logical AstraSim node IDs for the sender and receiver
RdmaClientHelper::RdmaClientHelper (uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint64_t size, uint32_t win, uint64_t baseRtt,
        void (*msg_handler)(void* fun_arg), void* fun_arg, int tag, int src, int dest)
{
  // Debug print so we can trace which flows are being registered
  std::cout << "RdmaClientHelper::RdmaClientHelper called:" << std::endl
          << "\t pg=" << pg << std::endl
          << "\t sip=";
  sip.Print(std::cout);
  std::cout << std::endl
            << "\t dip=";
  dip.Print(std::cout);
  std::cout << std::endl
            << "\t sport=" << sport << std::endl
            << "\t dport=" << dport << std::endl;

  // Bind the factory to the RdmaClient application type, then populate
  // all ns-3 attributes so that each Install() call creates a fully
  // configured client without extra per-node setup.
	m_factory.SetTypeId (RdmaClient::GetTypeId ());
	SetAttribute ("PriorityGroup", UintegerValue (pg));
	SetAttribute ("SourceIP", Ipv4AddressValue (sip));
	SetAttribute ("DestIP", Ipv4AddressValue (dip));
	SetAttribute ("SourcePort", UintegerValue (sport));
	SetAttribute ("DestPort", UintegerValue (dport));
	SetAttribute ("WriteSize", UintegerValue (size));
	SetAttribute ("Window", UintegerValue (win));
	SetAttribute ("BaseRtt", UintegerValue (baseRtt));
  SetAttribute ("Tag", UintegerValue (tag));
  SetAttribute ("Src", UintegerValue (src));
  SetAttribute ("Dest", UintegerValue (dest));
  // NVLS (NVLink Switch) collective offload disabled by default
  SetAttribute ("NVLS_enable", UintegerValue (0));
  // Store the completion callback and its argument so Install() can
  // forward them to each created RdmaClient via SetFn().
  msg_handler = msg_handler;
  fun_arg = fun_arg;
}

// Forwards an attribute key/value pair to the internal object factory.
// Can be called before Install() to override any default or constructor-set value.
void
RdmaClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

// Creates and installs one RdmaClient application on every node in the container.
// Each client is wired with the completion callback, then added to the node's
// application list. Returns a container of all created application objects.
ApplicationContainer
RdmaClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      // Instantiate a new RdmaClient using the pre-configured factory
      Ptr<RdmaClient> client = m_factory.Create<RdmaClient> ();
      // Register the completion callback so AstraSim is notified when the
      // RDMA write finishes
      client->SetFn(msg_handler, fun_arg);
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

} // namespace ns3
