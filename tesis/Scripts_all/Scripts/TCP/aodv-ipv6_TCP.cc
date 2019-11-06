/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include <iostream>
#include <cmath>
#include "ns3/ping6-helper.h"

#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

    //nodo keyboord
    int nodo;

    //time keyboard
    int tm;

class AodvEjemplo
{
    public:

        // Constructor
        AodvEjemplo ();

        // Configurar parametros del script
        bool Configurar (int argc, char *argv[]);

        // Ejecutar simulacion
        void Ejecutar ();

        // Reporte de resultados
        void Reporte (std::ostream & os);

    private:

        // Numero de nodos
        uint32_t numNodos;

        // Tiempo de simulacion (s)
        double tiempoTotal;

        // Escribir trazas PCAP por dispositivo
        bool pcap;

        // Imprimir rutas
        bool imprimirRutas;

        ///
        double stopOffset;        
        
        /// 
        bool enableTraffic;

        /// Conexiones
        uint32_t sinks =20;

        // Contenedor de nodos
        NodeContainer nodos;

        // Contenedor de dispositivos
        NetDeviceContainer dispositivos;

        // Contenedor de interfaces IPv6
        Ipv6InterfaceContainer interfaces;


    private:

        // Creacion de nodos
        void CrearNodos ();

        // Creacion de dispositivos
        void CrearDispositivos ();

        // Instalacion de pila de protocolos
        void InstalarProtocolos ();

        // Instalacion de aplicaciones de red
        void InstalarAplicaciones ();
};

AodvEjemplo::AodvEjemplo () :
  numNodos (nodo),
  tiempoTotal (tm),
  stopOffset (10.0),
  enableTraffic (true)
{
}

bool
AodvEjemplo::Configurar (int argc, char *argv[])
{
    // Habilitar log AODV
    // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

    SeedManager::SetSeed (12345);
    CommandLine cmd;

    cmd.AddValue ("pcap", "Escribir trazas PCAP.", pcap);
    cmd.AddValue ("imprimirRutas", "Imprimir tabla de enrutamiento.", imprimirRutas);
    cmd.AddValue ("numNodos", "Numero de nodos.", numNodos);
    cmd.AddValue ("tiempoTotal", "Tiempo de simulacion, s.", tiempoTotal);

    cmd.Parse (argc, argv);
    return true;
}

void
AodvEjemplo::Ejecutar ()
{
    CrearNodos ();
    CrearDispositivos ();
    InstalarProtocolos ();
    if (enableTraffic)
      {
    InstalarAplicaciones ();
      }

    std::cout << "Iniciando simulacion por " << tiempoTotal << " segundos...\n";
 
    //FlowMonitor
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowMonitorHelper;
    flowMonitor = flowMonitorHelper.InstallAll();

    Simulator::Stop (Seconds (tiempoTotal));
    Simulator::Run ();
    Simulator::Destroy ();

    flowMonitor->SetAttribute("DelayBinWidth", DoubleValue(0.01));
    flowMonitor->SetAttribute("JitterBinWidth", DoubleValue(0.01));
    flowMonitor->SetAttribute("PacketSizeBinWidth", DoubleValue(1));
    flowMonitor->CheckForLostPackets();
    flowMonitor->SerializeToXmlFile("graphs/TCP/100/flowMonNodes.xml", true, true);
}

void
AodvEjemplo::Reporte (std::ostream &)
{
}

void
AodvEjemplo::CrearNodos ()
{
    std::cout << "Creando " << numNodos << " nodos.\n";

    std::string traceFile = "src/mobility/examples/udptcp100.ns_movements";

    CommandLine cmd;

    nodos.Create (numNodos);

    for (uint32_t i = 0; i < numNodos; ++i)
    {
        std::ostringstream os;
        os << "nodo-" << i;
        Names::Add (os.str (), nodos.Get (i));
    }

    // Crear cuadricula estatica
    MobilityHelper mobility;

    // Random Waypoint
    double velocidadNodo = 10.0;
    double tiempoPausa = 0.0;

    ObjectFactory of;
    of.SetTypeId ("ns3::RandomRectanglePositionAllocator");
    of.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=150.0]"));
    of.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=150.0]"));

    Ptr<PositionAllocator> pa = of.Create () -> GetObject<PositionAllocator> ();

    // Variable aleatoria uniforme de velocidad
    std::ostringstream survs;
    survs << "ns3::UniformRandomVariable[Min=0.0|Max=" << velocidadNodo << "]";

    // Variable aleatoria constante de pausa
    std::ostringstream pcrvs;
    pcrvs << "ns3::ConstantRandomVariable[Constant=" << tiempoPausa << "]";

    mobility.SetMobilityModel (
        "ns3::RandomWaypointMobilityModel",
        "Speed", StringValue (survs.str ()),
        "Pause", StringValue (pcrvs.str ()),
        "PositionAllocator", PointerValue (pa)
    );

    mobility.Install (nodos);

    cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);

    Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
    ns2.Install();
}

void
AodvEjemplo::CrearDispositivos ()
{
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    wifiMac.SetType ("ns3::AdhocWifiMac");

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();

    wifiPhy.SetChannel (wifiChannel.Create ());
    WifiHelper wifi;
    wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

    dispositivos = wifi.Install (wifiPhy, wifiMac, nodos);

    if (pcap)
    {
        //std::string pathbase = "nodos_pcap/ipv6/";
        //std::string dir = system(mkdir(pathbase + str(nodo)));
        wifiPhy.EnablePcapAll ("graphs/TCP/100/aodv-ipv6");

        //system("pwd");
    }
}

void
AodvEjemplo::InstalarProtocolos ()
{
    Aodv6Helper aodv;

    Ipv6ListRoutingHelper lrh;
    lrh.Add (aodv, 0);

    InternetStackHelper pila;
    // pila.SetRoutingHelper (aodv);
    pila.SetIpv4StackInstall (false);
    pila.SetRoutingHelper (lrh);
    pila.Install (nodos);

    Ipv6AddressHelper direcciones;
    direcciones.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
    interfaces = direcciones.Assign (dispositivos);

    // Adicional
    // interfaces.SetForwarding (0, true);
    // interfaces.SetDefaultRouteInAllNodes (0);

    if (imprimirRutas)
    {
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("graphs/TCP/100/aodv-ipv6.rutas", std::ios::out);
        aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

void
AodvEjemplo::InstalarAplicaciones ()
{
 
    Ptr<Node> node = nodos.Get(1);
    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
    Ipv6Address ip = ipv6->GetAddress(1, 0).GetAddress();
    std::cout << "\tnodo 1:Server\n";
    std::cout << ip; 
    std::cout << "\n-----------\n";

    Ptr<Node> nodex = nodos.Get(80);
    Ptr<Ipv6> ipv6x = nodex->GetObject<Ipv6> ();
    Ipv6Address ip2 = ipv6x->GetAddress(1, 0).GetAddress();
    std::cout << "\tnodo 80:Cliente\n";
    std::cout << ip2;
    std::cout << "\n-----------\n";

    Address server;
    server = Address(interfaces.GetAddress(1, 0));
    uint16_t port = 8080;
    
    Address sinkLocal (Inet6SocketAddress(Ipv6Address::GetAny(), port));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocal);
    ApplicationContainer sinkApp = sinkHelper.Install(nodos.Get(1));
    sinkApp.Start (Seconds (19.0));
    sinkApp.Stop (Seconds (150.0));
     
    OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
    clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
    clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
 
    ApplicationContainer clientApp;
   
    AddressValue remoteAddress (Inet6SocketAddress (interfaces.GetAddress (1, 0), port));
    clientHelper.SetAttribute ("Remote", remoteAddress);
    clientApp.Add (clientHelper.Install (nodos.Get(80)));
    
    clientApp.Start (Seconds (20.0));
    clientApp.Stop (Seconds (150.0)); 

    OnOffHelper onOff ("ns3::TcpSocketFactory", Address ());
    //Start app
    std::cout << "Clients are connecting to Servers" << "\n";

    OnOffHelper onOffHelper ("ns3::TcpSocketFactory", Address ());
    onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
    onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
    PacketSinkHelper sink("ns3::TcpSocketFactory",Address(Inet6SocketAddress(ns3::Ipv6Address::GetAny(),port)));
  
}

int main (int argc, char *argv[])
{
    std::cout << "Ingrese número de nodos: \n";
    std::cin >> nodo;

    std::cout << "Ingrese tiempo total: \n";
    std::cin >> tm;

    AodvEjemplo ejemplo;

    if ( !ejemplo.Configurar (argc, argv) )
    {
        NS_FATAL_ERROR ("La configuracion fallo :(");
    }

    ejemplo.Ejecutar ();
    // Ejecutar script gráficas
    //system("python nodos_json/ipv6/Script/graficas.py");
    return 0;

}
