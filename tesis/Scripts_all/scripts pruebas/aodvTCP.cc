/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
 * Copyright (c) 2009 IITP RAS
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
 * This is an example script for AODV manet routing protocol.
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/v4ping-helper.h"
#include <iostream>
#include <cmath>


using namespace ns3;

  int nodos;
  int timeS;
  int sinks;

class AodvExample
{
public:
  AodvExample ();
  /// Script de configuraciones parametros
  bool Configure (int argc, char **argv);
  /// Simulación
  void Run ();
  /// Reporte de resultados
  void Report (std::ostream & os);

private:

  // PARAMETROS
  /// Numero de nodos
  uint32_t size;
  /// Distancia entre nodos, metros
  double step;
  /// Tiempo de simulación, segundos
  double totalTime;
  /// Escribe por-dispositivo PCAP traces
  bool pcap;
  /// Imprime rutas
  bool printRoutes;
  /// Tamaño de paquetes
  uint32_t m_packetSize = 1024;
  /// Velocidad de nodos
  uint32_t m_nodeSpeed = 0;
  /// Puerto
  uint16_t m_port = 8080;

  

  // red
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
};

int main (int argc, char **argv)
{
  
  std::cout << "Ingrese número de nodos: \n";
  std::cin >> nodos;

  std::cout << "Ingrese tiempo de simulación: \n";
  std::cin >> timeS;

  //std::cout << "Ingrese número de conexiones: \n";
  //std::cin >> sinks;

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
  step (100),
  totalTime (timeS),
  pcap (true),
  printRoutes (true)
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
  InstallApplications ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
AodvExample::Report (std::ostream &)
{
}

void
AodvExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes " << step << " m apart.\n";

  //std::string traceFile = "src/mobility/examples/simulator.ns_movements";
  //CommandLine cmd;

  nodes.Create (size);
  
  // Nombre de nodos
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
  // Crear mobility Random Waypoint
  MobilityHelper mobility;
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));

  std::ostringstream speedConstantRandomVariableStream;
  speedConstantRandomVariableStream << "ns3::ConstantRandomVariable[Constant="
                                    << m_nodeSpeed
                                   << "]";

  Ptr <PositionAllocator> taPositionAlloc = pos.Create ()->GetObject <PositionAllocator> ();
  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel", "Speed", StringValue (speedConstantRandomVariableStream.str ()),
                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"), "PositionAllocator", PointerValue (taPositionAlloc));
  mobility.SetPositionAllocator (taPositionAlloc);
  mobility.Install (nodes);

  //cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);

  //Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
  //ns2.Instalr();
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
      wifiPhy.EnablePcapAll (std::string ("graph/ipv4/aodv"));
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
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("graph/ipv4/aodv.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

void
AodvExample::InstallApplications ()
{

 // V4PingHelper ping (interfaces.GetAddress (size - 1));
 // ping.SetAttribute ("Verbose", BooleanValue (true));

 // ApplicationContainer p = ping.Install (nodes.Get (0));
 // p.Start (Seconds (0));
 // p.Stop (Seconds (totalTime) - Seconds (0.001));

  // move node away
 // Ptr<Node> node = nodes.Get (size/2);
 // Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
 // Simulator::Schedule (Seconds (totalTime/3), &MobilityModel::SetPosition, mob, Vector (1e5, 1e5, 1e5));
 
  NS_LOG_INFO("Create TCP Applications");
  
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (4096));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("6Mbps"));

  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), m_port));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (size-1));
  sinkApp.Start (Seconds (totalTime/2));
  sinkApp.Stop (Seconds (totalTime));

  //Create the OnOff applications to send TCP
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute 
	  ("OnTime", RandomVariableValue (ConstantVariable (10.3, 40.7)));
  clientHelper.SetAttribute 
	  ("OffTime", RandomVariableValue (ConstantVariable (0.01)));

  ApplicationContainer clientApp;
  AddressValue remoteAddress (InetSocketAddress (interfaces.GetAddress (size - 1), m_port));
  clientHelper.SetAttribute ("Remote", remoteAddress);
  clientApp = clientHelper.Install (nodes.Get (0));
  clientApp.Start (Seconds (totalTime/2));
  clientApp.Stop (Seconds (totalTime));

}




