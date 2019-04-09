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
#include "ns3/dsdv-module.h"
#include "ns3/simple-wireless-tdma-module.h"
#include <iostream>
#include <sstream>
#include <fstream>


using namespace ns3;

int 
main (int argc, char *argv[])
{
	bool verbose = true;

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

	NetDeviceContainer nodes_to_CH_devices;
	Config::SetDefault ("ns3::SimpleWirelessChannel::MaxRange", DoubleValue (10.0));
	TdmaHelper tdma = TdmaHelper (mesh.GetN (),mesh.GetN ()); // in this case selected, numSlots = nodes
	TdmaControllerHelper controller;
	controller.Set ("SlotTime", TimeValue (MicroSeconds (1100)));
	controller.Set ("GaurdTime", TimeValue (MicroSeconds (100)));
	controller.Set ("InterFrameTime", TimeValue (MicroSeconds (0)));
	tdma.SetTdmaControllerHelper (controller);
	nodes_to_CH_devices = tdma.Install (mesh);

	  // AsciiTraceHelper ascii;
	  // Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (tr_name + ".tr");
	  // tdma.EnableAsciiAll (stream);

	// installation of devices on mesh left
	NetDeviceContainer base_to_CH_devices;
	base_to_CH_devices = pointToPoint.Install (base_cluster);


	DsdvHelper dsdv;
	dsdv.Set ("PeriodicUpdateInterval", TimeValue (Seconds (15)));
	dsdv.Set ("SettlingTime", TimeValue (Seconds (6)));

	InternetStackHelper stack;
	stack.SetRoutingHelper (dsdv);
	stack.Install (base_cluster.Get(0));
	stack.Install (mesh);

	Ipv4AddressHelper address;
	address.SetBase ("10.1.172.0", "255.255.255.0");
	Ipv4InterfaceContainer nodes2CHInterfaces;
	nodes2CHInterfaces = address.Assign (nodes_to_CH_devices);

	address.SetBase ("10.1.131.0", "255.255.255.0");
	Ipv4InterfaceContainer base2CHInterfaces;
	base2CHInterfaces = address.Assign (base_to_CH_devices);


	// Nodes to cllusterhead communication
	OnOffHelper onoff1 ("ns3::UdpSocketFactory", Address (InetSocketAddress (nodes2CHInterfaces.GetAddress (8), 6000))); //clusterhead as sink
    onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));




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
