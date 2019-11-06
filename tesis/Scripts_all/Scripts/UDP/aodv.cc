/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h" 
#include "ns3/v4ping-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/animation-interface.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include <iostream>
#include <cmath>

using namespace ns3;

  int nodos;
  int timeS;

class AodvExample 
{
public:
  AodvExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /// Report results
  void Report (std::ostream & os);

private:

  // parameters
  /// Number of nodes
  uint32_t size;
  /// Distance between nodes, meters
  double step;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;
  
  double stopOffset;

  bool enableTraffic;

  // network
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
  //void Create2Plot ();
};

int main (int argc, char **argv)
{
  std::cout << "Ingrese número de nodos: \n";
  std::cin >> nodos;

  std::cout << "Ingrese tiempo de simulación: \n";
  std::cin >> timeS;

  AodvExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
AodvExample::AodvExample () :
  size (nodos),
  step (40),
  totalTime (timeS),
  pcap (true),
  printRoutes (true),
  stopOffset (10.0),
  enableTraffic(true)
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("step", "Grid step, m", step);
  cmd.AddValue ("traffic", "Enable traffic", enableTraffic);

  cmd.Parse (argc, argv);
  return true;
}

void
AodvExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  //InstallApplications ();
  
  if (enableTraffic) 
   {
     InstallApplications();
   }
  //Create2Plot ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  //FlowMonitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowMonitorHelper;
  flowMonitor = flowMonitorHelper.InstallAll();

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
  
  flowMonitor->SetAttribute("DelayBinWidth", DoubleValue(0.01));
  flowMonitor->SetAttribute("JitterBinWidth", DoubleValue(0.01));
  flowMonitor->SetAttribute("PacketSizeBinWidth", DoubleValue(1));
  flowMonitor->CheckForLostPackets();
  flowMonitor->SerializeToXmlFile("graph/UDP/100/flowMonNodes.xml", true, true);
}

void
AodvExample::Report (std::ostream &)
{ 
}

void
AodvExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes " << step << " m apart.\n";

  std::string traceFile = "src/mobility/examples/udptcp100.ns_movements";
  CommandLine cmd;
  cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile); 

  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
  // Create static grid
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (step),
                                 "DeltaY", DoubleValue (0),
                                 "GridWidth", UintegerValue (size),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);

  Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
  ns2.Install();
}

void
AodvExample::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes); 

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("graph/UDP/100/aodv"));
    }
}

void
AodvExample::InstallInternetStack ()
{
  AodvHelper aodv;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("graph/UDP/100/aodv.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

void
AodvExample::InstallApplications ()
{ 
  uint16_t port = 9;

  //Habilita componentes UDP Client and Server
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);

  OnOffHelper onOff ("ns3::UdpSocketFactory",Address ());

  //Impresión de urls 1 & 80
  Ptr<Node> node1 = nodes.Get(1);
  Ptr<Node> node80 = nodes.Get(80);
  Ptr<Ipv4> ipv4_1 = node1->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv4_80 = node80->GetObject<Ipv4> ();
  Ipv4Address ip_1 = ipv4_1->GetAddress(1, 0).GetLocal();
  Ipv4Address ip_80 = ipv4_80->GetAddress(1, 0).GetLocal();
  std::cout << "\tnodo 1:Server\n";
  std::cout << ip_1; 
  std::cout << "\n-----------\n";
  std::cout << "\tnodo 80:Cliente\n";
  std::cout << ip_80;
  std::cout << "\n-----------\n";

  //SERVER
  UdpServerHelper server (port);
  ApplicationContainer p = server.Install (nodes.Get (1));
  p.Start (Seconds (19.0));
  p.Stop (Seconds (150.0));

  //CLIENTE
  UdpClientHelper client (interfaces.GetAddress (1), port);
  client.SetAttribute ("MaxPackets", UintegerValue (800));
  client.SetAttribute ("Interval", TimeValue (Seconds (0.05)));
  client.SetAttribute ("PacketSize", UintegerValue (1024));
  p = client.Install (nodes.Get (80));
  p.Start (Seconds (20.0));
  p.Stop (Seconds (150.0));

}

