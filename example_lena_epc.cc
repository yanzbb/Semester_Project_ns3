/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011, 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include <ns3/config-store-module.h>
#include "ns3/flow-monitor-module.h"
using namespace ns3;

/*
 * Simple simulation program using the emulated EPC.
 * For the LTE radio part, it simulates a simple linear topology with
 * a fixed number of eNBs spaced at equal distance, and a fixed number
 * of UEs per each eNB, located at the same position of the eNB. 
 * For the EPC, it uses EmuEpcHelper to realize the S1-U connection
 * via a real link. 
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

int
main (int argc, char *argv[])
{

  uint16_t nEnbs = 1;
  uint16_t nUesPerEnb = 1;
  double simTime = 30.1;
  double distance = 1000.0;
  double interPacketInterval = 100;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("nEnbs", "Number of eNBs", nEnbs);
  cmd.AddValue("nUesPerEnb", "Number of UEs per eNB", nUesPerEnb);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);

   //***Log info of simulations
  // LogComponentEnable ("LteUePhy", LOG_LEVEL_ALL);

  //LogComponentEnable ("UdpClient", LOG_ALL);
  //LogComponentEnable ("UdpServer", LOG_ALL);
  // LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
 //LogComponentEnable ("EpcFirstExample", LOG_LEVEL_ALL);


   LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);

   // LogComponentEnable ("PacketSink", logLevel); 
 // LogComponentEnable ("EpcSgwPgwApplication", logLevel); 
    //LogComponentEnable ("PhyTxStatsCalculator", logLevel);
   LogComponentEnable ("LteHelper", logLevel);
   LogComponentEnable ("EpcHelper", logLevel);

   LogComponentEnable ("EmuEpcHelper", logLevel);
   //LogComponentEnable ("RadioBearerStatsCalculator", logLevel);

   //LogComponentEnable ("LteEnbRrc", logLevel);
  //LogComponentEnable ("PacketLossCounter", logLevel);
   //LogComponentEnable ("LteUeRrc", logLevel);
  // LogComponentEnable ("LteUeNetDevice", logLevel);




  // let's go in real time
  // NOTE: if you go in real time I strongly advise to use
  // --ns3::RealtimeSimulatorImpl::SynchronizationMode=HardLimit
  // I've seen that if BestEffort is used things can break
  // (even simple stuff such as ARP)
  //GlobalValue::Bind ("SimulatorImplementationType", 
  //                 StringValue ("ns3::RealtimeSimulatorImpl"));

 // let's speed things up, we don't need these details for this scenario
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue (false));  

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);



  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  //Ptr<EmuEpcHelper>  epcHelper = CreateObject<EmuEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  //epcHelper->Initialize ();



  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet


  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Mb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1000));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.01)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(nEnbs);
  ueNodes.Create(nEnbs*nUesPerEnb);

/*//This part is to test change of EpsBearer settings effection,Not working...
  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs;
  // Default scheduler is PF, uncomment to use RR
  //lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);
// Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::NGBR_VOICE_VIDEO_GAMING ;
  EpsBearer bearer (q);
  lteHelper->ActivateDataRadioBearer (ueDevs, bearer);

*/




  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < nEnbs; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  lteHelper->Attach (ueLteDevs); 
  // side effects: 1) use idle mode cell selection, 2) activate default EPS bearer

  // randomize a bit start times to avoid simulation artifacts
  // (e.g., buffer overflows due to packet transmissions happening
  // exactly at the same time) 
  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (interPacketInterval/1000.0));


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++ulPort;
      ApplicationContainer clientApps;
      ApplicationContainer serverApps;


      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      serverApps.Add (ulPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (dlPacketSinkHelper.Install (remoteHost));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(10000));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(10000));

      clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get(u)));

      serverApps.Start (Seconds (startTimeSeconds->GetValue ()));
      clientApps.Start (Seconds (startTimeSeconds->GetValue ()));  
    }


  //Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

 monitor->CheckForLostPackets ();
 monitor->SerializeToXmlFile ("results.xml" , true, true );
 Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
 std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

 for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
  {
  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);
  
          NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
          NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
          NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
          NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
 NS_LOG_UNCOND("Packet loss= " << ((iter->second.txPackets-iter->second.rxPackets)*1.0)/iter->second.txPackets);
  }


  lteHelper->EnableTraces ();

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  Simulator::Destroy();
  return 0;
}

