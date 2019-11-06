/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include <iostream>
#include <cmath>
#include "ns3/ping6-helper.h"

using namespace ns3;

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

AodvEjemplo::AodvEjemplo () : numNodos (10), tiempoTotal (10)
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
	InstalarAplicaciones ();

	std::cout << "Iniciando simulacion por " << tiempoTotal << " segundos...\n";

	Simulator::Stop (Seconds (tiempoTotal));
	Simulator::Run ();
	Simulator::Destroy ();
}

void
AodvEjemplo::Reporte (std::ostream &)
{ 
}

void
AodvEjemplo::CrearNodos ()
{
	std::cout << "Creando " << numNodos << " nodos.\n";
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
		wifiPhy.EnablePcapAll (std::string ("aodv-ipv6"));
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
	direcciones.SetBase (Ipv6Address ("2001:db8::"), Ipv6Prefix (64));
	interfaces = direcciones.Assign (dispositivos);
	
	// Adicional
	// interfaces.SetForwarding (0, true);
	// interfaces.SetDefaultRouteInAllNodes (0);

	if (imprimirRutas)
	{
		Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv-ipv6.rutas", std::ios::out);
		aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
	}
}

void
AodvEjemplo::InstalarAplicaciones ()
{
	// Ping v6
	Ping6Helper ping6;
	ping6.SetLocal (interfaces.GetAddress (0, 1));
	ping6.SetRemote (interfaces.GetAddress (1, 1));
		
	ping6.SetAttribute ("MaxPackets", UintegerValue (100));
	ping6.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	ping6.SetAttribute ("PacketSize", UintegerValue (1024));
	
	ApplicationContainer apps = ping6.Install (nodos.Get(0));
	apps.Start (Seconds (1.0));
	apps.Stop (Seconds (tiempoTotal) - Seconds (0.001));
}

int main (int argc, char *argv[])
{
	AodvEjemplo ejemplo;
	
	if ( !ejemplo.Configurar (argc, argv) )
	{
		NS_FATAL_ERROR ("La configuracion fallo :(");
	}
	
	ejemplo.Ejecutar ();
	return 0;
}