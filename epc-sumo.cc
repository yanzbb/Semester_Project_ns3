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
#include "ns3/netanim-module.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

int
main (int argc, char *argv[])
{

  std::string testScenario   = "scratch/bologna.time.tcl";


  uint16_t nEnbs = 1;
  uint16_t nUesPerEnb = 3;
  double simTime = 100.1;
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

   //Log info of simulations
  // LogComponentEnable ("LteUePhy", LOG_LEVEL_ALL);

  //LogComponentEnable ("UdpClient", LOG_ALL);
  //LogComponentEnable ("UdpServer", LOG_ALL);
   LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
 //LogComponentEnable ("EpcFirstExample", LOG_LEVEL_ALL);
  //LogComponentEnable ("LteEnbRrc", LOG_LEVEL_ALL);

 // let's speed things up, we don't need these details for this scenario
  Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue (false));  

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);
/*
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<EmuEpcHelper>  epcHelper = CreateObject<EmuEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  epcHelper->Initialize ();
*/

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

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
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (5)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);  

  /* //adding a rate error model inside device on channels
   Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
   em->SetAttribute ("ErrorRate", DoubleValue (0.1));
   internetDevices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
*/


                                                                      
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);


  // Install Mobility Model


  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(nEnbs);
  //MobilityHelper mobility;
  //mobility.Install (enbNodes);
  //ueNodes.Create(nEnbs*nUesPerEnb);
  ueNodes.Create (1);
  Ns2MobilityHelper mobHelper = Ns2MobilityHelper(testScenario);
  mobHelper.Install(ueNodes.Begin(), ueNodes.End());

  // Install Mobility Model
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  for (uint16_t i = 0; i < nUesPerEnb; i++)
    {
      enbPositionAlloc->Add (Vector(20 * i, 0, 0));
    }
  enbmobility.SetPositionAllocator(enbPositionAlloc);
  enbmobility.Install (enbNodes); 



for (int i=0;i < 1;i++)
  {
  Ptr<MobilityModel> ue1 = ueNodes.Get (i)->GetObject<MobilityModel> ();
  Vector pos = ue1->GetPosition ();
  std::cout << "Node position is at (" << pos.x << ", " <<pos.y << "," <<pos.z << ")\n"; 
 

 /*  Ptr<ConstantPositionMobilityModel> sn0 = enbNodes.Get (0)->GetObject<ConstantPositionMobilityModel> ();
    sn0 ->SetPosition (Vector (2600.0, 2400.0, 0.0));
*/  
}





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
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
      dlClient.SetAttribute ("PacketSize", UintegerValue (1024));


      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
      ulClient.SetAttribute ("PacketSize", UintegerValue (1024));

      clientApps.Add (ulClient.Install (remoteHost));
      clientApps.Add (dlClient.Install (ueNodes.Get(u)));

      serverApps.Start (Seconds (startTimeSeconds->GetValue ()));
      clientApps.Start (Seconds (startTimeSeconds->GetValue ()));  
    }



  lteHelper->EnableTraces ();
  AnimationInterface anim ("animation2.xml");

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  Simulator::Destroy();
  return 0;
}

