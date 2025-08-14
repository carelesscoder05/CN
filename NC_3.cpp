#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

// Use the ns-3 namespace
using namespace ns3;

// Enable logging for the UdpEchoClient and UdpEchoServer applications
NS_LOG_COMPONENT_DEFINE("FirstScriptExample");

int main(int argc, char* argv[]) {
    // Set the time resolution to nanoseconds
    Time::SetResolution(Time::NS);

    // Enable logging for our applications
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // 1. Create Nodes
    // Create a NodeContainer to hold our two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // 2. Setup Channel and NetDevices
    // Create a PointToPointHelper to configure the channel and devices
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    // Install the NetDevices on our nodes
    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // 3. Install the Internet Stack
    // This helper installs the TCP/IP stack on the nodes
    InternetStackHelper stack;
    stack.Install(nodes);

    // 4. Assign IP Addresses
    // This helper assigns IP addresses to the NetDevices
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 5. Create and Install Applications
    // Create a UDP echo server application on the second node (node 1)
    UdpEchoServerHelper echoServer(9); // Port 9
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    // Create a UDP echo client application on the first node (node 0)
    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    // 6. Run the Simulation
    Simulator::Run();

    // 7. Clean up
    Simulator::Destroy();
    return 0;
}
