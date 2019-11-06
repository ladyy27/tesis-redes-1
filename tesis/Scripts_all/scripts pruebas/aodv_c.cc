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
#include "ns3/mobility-module.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/point-to-point-module.h"
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
  uint32_t m_nodeSpeed = 10;
  /// Puerto
  uint16_t m_port = 654;

  

  // Contenedor de nodos
  NodeContainer nodes;
  // Contenedor de dispositivos
  NetDeviceContainer devices;
  // Contenedor de interfaces IPv4
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

  //cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);

  //Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
  //ns2.Instalr();

  nodes.Create (size);
  
  // Nombre de nodos
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
  // Random Waypoint static
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

  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  
  //Server
  UdpServerHelper server (m_port);
  ApplicationContainer apps = server.Install (nodes.Get (1));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

  //Client
  Time interPacketInterval = Seconds (0.05);
  UdpClientHelper client (interfaces.GetAddress (1), m_port);
  client.SetAttribute ("MaxPackets", UintegerValue (size));
  client.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  client.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  apps = client.Install (nodes.Get (80));
  //std::cout << "nodo destino", apps, << "nodo";
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (10.0));

  std::string rate ("8kbps");

  //

  OnOffHelper onoffHelper ("ns3::UdpSocketFactory", InetSocketAddress 
		            (test.i1.GetAddress(0), m_port));
  onoffHelper.SetAttribute ("DataRate", StringValue (rate));
  onoffHelper.SetAttribute ("PacketSize", StringValue ("1000"));

  uint16_t port = 3999;
  uint16_t numberOfUes = 1;
 
  OnOffHelper cbrgenhelper ("ns3::UdpSocketFactory", Address());

  cbrgenhelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  cbrgenhelper.SetAttribute ("ns3::ConstantRandomVariable[Constant=1.0]"));
  ApplicationContainer cbrgen_node_to_node;

  int aux = 0;

  for (int i = 0; i < numberOfUes-1; i++)
  {
    //Remote is the address of the destination
    cbrgenhelper.SetAttribute ("Remote", AddressValue(InetSocketAddress(test.aux1.GetAddress(aux), port)));
    cbrgen_node_to_node.Add(cbrgenhelper.Install(test.ueNodes.Get(i)));
  }

  //Start to generate traffic
  cbrgen_node_to_node.Start(Seconds(1.0));

  //Stop generating traffic
  cbrgen_node_to_node.Stop(Seconds(timeS));

  ApplicationContainer cbrrecv_node_to_node;

  for (int k = numberOfUes-1; k >= 0; k--)
  {
    PacketSinkHelper sink("ns3::UdpSocketFactory", Address(InetSocketAddress (test.aux1.GetAddress(k), port)));
    cbrrecv_node_to_node.Add(sink.Install (test.ueNodes.Get (k)));
  }

  cbrrecv_node_to_node.Start(Seconds(0));
  cbrrecv_node_to_node.Stop(Seconds(timeS));

}

  




