#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mesh-helper.h"
#include <iostream>
#include <sstream>
#include <fstream>


using namespace ns3;


int 
main (int argc, char *argv[])
{
	bool verbose = true;

	double    m_randomStart = 0.1; 			///< random start
	// double    m_totalTime = 10.0;
	uint32_t  m_nIfaces = 1; 				///< number interfaces
	bool      m_chan = true; 				///< channel
	bool      m_pcap = false; 				///< PCAP
	std::string m_stack = "ns3::Dot11sStack"; 				///< stack
	std::string m_root = "ff:ff:ff:ff:ff:ff"; 				///< root

	// Time::SetResolution (Time::NS);
	CommandLine cmd;
	// cmd.AddValue ("nCsma", "Number of \"extra\" nodes/devices", nCsma);
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

	cmd.Parse (argc,argv);

	if (verbose)
	{
		LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
		LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
	}


	NodeContainer mesh;
	NodeContainer base_cluster;


	uint32_t grid_nodes = 9;

	mesh.Create(grid_nodes);

	// nodes arranged in mesh
	MobilityHelper mobility;
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		"MinX", DoubleValue (0.0),
		"MinY", DoubleValue (0.0),
		"DeltaX", DoubleValue (100.0),
		"DeltaY", DoubleValue (100.0),
		"GridWidth", UintegerValue ((grid_nodes/3)),
		"LayoutType", StringValue ("RowFirst"));
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (mesh);

	// assignment of the coordinates of base_station left

	// interchange the get reference 0 and 1

	base_cluster.Create(1);						// base-station

	MobilityHelper mob;							// adding the coordinates to the base station
	mob.SetPositionAllocator ("ns3::GridPositionAllocator",
		"MinX", DoubleValue (400.0),
		"MinY", DoubleValue (400.0),
		"DeltaX", DoubleValue (100.0),
		"DeltaY", DoubleValue (100.0),
		"GridWidth", UintegerValue ((grid_nodes/3)),
		"LayoutType", StringValue ("RowFirst"));
	mob.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mob.Install (base_cluster);

	base_cluster.Add(mesh.Get(grid_nodes-1));	// Cluster Head // base_cluster.get(1)

	
	// base_cluster
	PointToPointHelper pointToPoint;										// chanel for base to cluster connection
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));		// for time-being delay is hard-coded


	//Mesh
	// defination for channel for mesh left
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
	wifiPhy.SetChannel (wifiChannel.Create ());

	MeshHelper meshhelper = MeshHelper::Default ();
	if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
	{
		meshhelper.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
	}
	else
	{
      //If root is not set, we do not use "Root" attribute, because it
      //is specified only for 11s
		meshhelper.SetStackInstaller (m_stack);
	}
	if (m_chan)
	{
		meshhelper.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
	}
	else
	{
		meshhelper.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
	}
	meshhelper.SetMacType ("RandomStart", TimeValue (Seconds (m_randomStart)));  // Set number of interfaces - default is single-interface mesh point
	meshhelper.SetNumberOfInterfaces (m_nIfaces);	
								 // Install protocols and return container if MeshPointDevices
	NetDeviceContainer nodes_to_CH_devices = meshhelper.Install (wifiPhy, mesh);

	if (m_pcap)
		wifiPhy.EnablePcapAll (std::string ("mp-"));


	// installation of devices on mesh left
	NetDeviceContainer base_to_CH_devices;
	base_to_CH_devices = pointToPoint.Install (base_cluster);

	InternetStackHelper stack;
	stack.Install (base_cluster.Get(0));
	stack.Install (mesh);

	Ipv4AddressHelper address;
	address.SetBase ("10.1.172.0", "255.255.255.0");
	Ipv4InterfaceContainer nodes2CHInterfaces;
	nodes2CHInterfaces = address.Assign (nodes_to_CH_devices);

	address.SetBase ("10.1.131.0", "255.255.255.0");
	Ipv4InterfaceContainer base2CHInterfaces;
	base2CHInterfaces = address.Assign (base_to_CH_devices);

	

	
	uint32_t rec_pkts = 0;
	/*Nodes to Cluster Head*/

	// Node 0

	uint32_t pkt_tosend = 5;

	// Port no. given to cluster Head
	UdpEchoServerHelper echoServer1 (3589);

	ApplicationContainer serverApps = echoServer1.Install (mesh.Get (grid_nodes-1));
	serverApps.Start (Seconds (1.0));
	serverApps.Stop (Seconds (5.0));
	
	UdpEchoClientHelper echoClient1 (nodes2CHInterfaces.GetAddress (0), 3589);
	echoClient1.SetAttribute ("MaxPackets", UintegerValue (pkt_tosend));
	echoClient1.SetAttribute ("Interval", TimeValue (Seconds(1.0)));
	echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer clientApps = echoClient1.Install (mesh.Get (0));
	clientApps.Start (Seconds (2.0));
	clientApps.Stop (Seconds (5.0));

	rec_pkts += pkt_tosend;


	// Port no. given to base station
	// Cluster Head to Base Station
	UdpEchoServerHelper echoServer (4539);

	ApplicationContainer BaseServerApps = echoServer.Install (base_cluster.Get (0));
	BaseServerApps.Start (Seconds (6.0));
	BaseServerApps.Stop (Seconds (10.0));

	UdpEchoClientHelper echoClient (base2CHInterfaces.GetAddress (0), 4539);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer CHClientApps = echoClient.Install (base_cluster.Get (1));
	CHClientApps.Start (Seconds (7.0));
	CHClientApps.Stop (Seconds (10.0));



	AnimationInterface anim("Nodes.xml");
	for (uint32_t i = 0;i < grid_nodes;i++){
		anim.SetConstantPosition(mesh.Get(0),(i%3)*100.0,(uint32_t)(i/3)*100.0);
	}

	anim.SetConstantPosition(base_cluster.Get(1),400.0,400.0);
	


	Simulator::Run ();
	Simulator::Destroy ();
	return 0;
}
