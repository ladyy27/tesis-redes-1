/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 
#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include <iostream>
#include <cmath>
#include "ns3/ping6-helper.h"
 
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
    stopOffset (20.0),
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
    flowMonitor->SerializeToXmlFile("graphs/UDP/100/flowMonNodes.xml", true, true);
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
    cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);

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
    double velocidadNodo = 20.0;
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
        wifiPhy.EnablePcapAll (std::string ("graphs/UDP/100/aodv-ipv6"));
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
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("graphs/UDP/100/aodv-ipv6.rutas", std::ios::out);
        aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}
 
void
AodvEjemplo::InstalarAplicaciones ()
{

    uint16_t port = 576;

    LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);

    Ptr<Node> nodeServer = nodos.Get(1);
    Ptr<Node> nodeCliente = nodos.Get(80);
    Ptr<Ipv6> ipv6Server = nodeServer->GetObject<Ipv6> ();
    Ptr<Ipv6> ipv6Cliente = nodeCliente->GetObject<Ipv6> ();
    Ipv6Address ipServer = ipv6Server->GetAddress( 1, 0 ).GetAddress();
    Ipv6Address ipCliente = ipv6Cliente->GetAddress( 1, 0 ).GetAddress();
    std::cout << "\tnodo 1:Server\n";
    std::cout << ipServer; 
    std::cout << "\n-----------\n";
    std::cout << "\tnodo 80:Cliente\n";
    std::cout << ipCliente;
    std::cout << "\n-----------\n";
  
    UdpServerHelper server (port);
    ApplicationContainer apps = server.Install (nodos.Get(1));
    apps.Start (Seconds (19.0));
    apps.Stop (Seconds (150.0));

    UdpClientHelper client (Address(interfaces.GetAddress (1, 0)), port);
    apps = client.Install (nodos.Get(80));
    apps.Start (Seconds (20.0));
    apps.Stop (Seconds (150.0));


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
    return 0;
}
