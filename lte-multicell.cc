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
#include "ns3/config-store.h"
#include "ns3/gnuplot.h"
#include <math.h>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LteMulticell");

void plotDevices(NodeContainer Nodes, NodeContainer ueNodes, Gnuplot2dDataset dataset_graph)
{

    std::string fileNameWithNoExtension = "nodes";
    std::string graphicsFileName = fileNameWithNoExtension + ".png";
    std::string plotFileName = fileNameWithNoExtension + ".plt";
    std::string plotTitle = "2-D Plot";
    std::string dataTitle = "2-D Data";

    // Instantiate the plot and set its title.
    Gnuplot plot(graphicsFileName);
    plot.SetTitle(plotTitle);

    // Make the graphics file, which the plot file will create when it
    // is used with Gnuplot, be a PNG file.
    plot.SetTerminal("png");

    // Set the labels for each axis.
    plot.SetLegend("X", "Y");

    // Set the range for the x axis.
    plot.AppendExtra("set xrange [-1.5:+1.5]");
    plot.AppendExtra("set pointsize 3");

    // Instantiate the dataset, set its title, and make the points be
    // plotted along with connecting lines.
    Gnuplot2dDataset dataset;
    dataset.SetTitle("eNodeB");
    dataset.SetStyle(Gnuplot2dDataset::POINTS);
    dataset.SetExtra("pointtype 3");

    for (uint32_t u = 0; u < Nodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = Nodes.Get(u);
        Ptr<MobilityModel> mob = ueNode->GetObject<MobilityModel>();

        dataset.Add(mob->GetPosition().x, mob->GetPosition().y);
    }

    // Add the dataset to the plot.
    plot.AddDataset(dataset);

    // ueNodes Dataset
    Gnuplot2dDataset dataset2;
    dataset2.SetTitle("UE");
    dataset2.SetStyle(Gnuplot2dDataset::POINTS);
    dataset2.SetExtra("pointtype 26");

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        Ptr<MobilityModel> mob = ueNode->GetObject<MobilityModel>();

        dataset2.Add(mob->GetPosition().x, mob->GetPosition().y);
    }

    // Add the dataset to the plot.
    plot.AddDataset(dataset2);

    // Add the dataset to the plot.
    plot.AddDataset(dataset_graph);

    

        // Open the plot file.
        std::ofstream plotFile(plotFileName.c_str());

    // Write the plot file.
    plot.GenerateOutput(plotFile);

    // Close the plot file.
    plotFile.close();
}

int main(int argc, char *argv[])
{
    int f_RandomPositionAllocator = 1;

    RngSeedManager::SetSeed(3); // Changes seed from default of 1 to 3
    RngSeedManager::SetRun(7);  // Changes run number from default of 1 to 7

    uint16_t numberOfNodes = 3;
    double simTime = 1.1;
    double interPacketInterval = 1;

    // Command line arguments
    CommandLine cmd;
    cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
    cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
    cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
    cmd.Parse(argc, argv);

    //float p = 1 / numberOfNodes;

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfNodes);
    ueNodes.Create(3 * numberOfNodes);

    // Install Mobility Model
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(-1.0, 0.0, 0.0)); //for enb 1
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));  //for enb 2
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));  //for enb 2

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    MobilityHelper mobility2;
    mobility2.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    if (!f_RandomPositionAllocator)
    {
        positionAlloc->Add(Vector(-1.0, 1.0, 0.0));  //for ue 1
        positionAlloc->Add(Vector(1.0, 1.0, 0.0));   //for ue 2
        positionAlloc->Add(Vector(-1.0, -1.0, 0.0)); //for ue 3
        positionAlloc->Add(Vector(1.0, -1.0, 0.0));  //for ue 4
        positionAlloc->Add(Vector(-1., 0.0, 0.0));   //for ue 5
        positionAlloc->Add(Vector(1.0, 0.0, 0.0));   //for ue 6

        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(enbNodes);
        mobility.Install(ueNodes);
    }
    else
    {
        Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
        x->SetAttribute("Min", DoubleValue(-0.1));
        x->SetAttribute("Max", DoubleValue(0.1));

        //NS_LOG_UNCOND(x->GetValue());

        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(enbNodes);

        mobility2.SetPositionAllocator("ns3::RandomDiscPositionAllocator", "X", DoubleValue(x->GetValue()), "Rho", StringValue("ns3::UniformRandomVariable[Min=0.9|Max=1]"));
        mobility2.Install(ueNodes);
    }

    // Print position


    lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");

    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));
    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Attach one UE per eNodeB

    Gnuplot2dDataset dataset_graph;
    dataset_graph.SetStyle(Gnuplot2dDataset::VECTORS);  // update gnuplot.h gnuplot.cc 
    dataset_graph.SetErrorBars(Gnuplot2dDataset::XY);
    dataset_graph.SetTitle("Conexión");

    for (uint16_t i = 0; i < ueNodes.GetN(); i++)
    {
        Ptr<UniformRandomVariable> v = CreateObject<UniformRandomVariable>();

        uint16_t enbid = ceil(v->GetValue() * numberOfNodes) - 1;

        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(enbid));

        // get points
        Ptr<Node> ueNode = ueNodes.Get(i);
        Ptr<MobilityModel> mob_ue = ueNode->GetObject<MobilityModel>();

        Ptr<Node> enbNode = enbNodes.Get(enbid);
        Ptr<MobilityModel> mob_enb = enbNode->GetObject<MobilityModel>();

        double x = mob_ue->GetPosition().x;
        double y = mob_ue->GetPosition().y;
        double xErrorDelta = mob_enb->GetPosition().x - x;
        double yErrorDelta = mob_enb->GetPosition().y  - y;

        dataset_graph.Add(x, y, xErrorDelta, yErrorDelta);

        //NS_LOG_UNCOND("ENB" << enbid + 1 << ",UE" << i + 1);
    }

    plotDevices(enbNodes, ueNodes, dataset_graph);

    // Install and start applications on UEs and remote host
    uint16_t dlPort = 1234;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        PacketSinkHelper dlPacketSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort + u));
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));

        UdpClientHelper dlClient(ueIpIface.GetAddress(u), dlPort + u);
        dlClient.SetAttribute("Interval", TimeValue(MilliSeconds(interPacketInterval)));
        dlClient.SetAttribute("MaxPackets", UintegerValue(1000000));

        clientApps.Add(dlClient.Install(remoteHost));
    }
    serverApps.Start(Seconds(0.01));
    clientApps.Start(Seconds(0.01));
    lteHelper->EnableTraces();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    Ptr<PacketSink> sink = serverApps.Get(0)->GetObject<PacketSink>();
    NS_LOG_UNCOND("UE1 throughput :" << sink->GetTotalRx() * 8.0 / 1000000.0 << " Mbps");

    Ptr<PacketSink> sink1 = serverApps.Get(1)->GetObject<PacketSink>();
    NS_LOG_UNCOND("UE2 throughput :" << sink1->GetTotalRx() * 8.0 / 1000000.0 << " Mbps");

    Ptr<PacketSink> sink2 = serverApps.Get(2)->GetObject<PacketSink>();
    NS_LOG_UNCOND("UE3 throughput :" << sink2->GetTotalRx() * 8.0 / 1000000.0 << " Mbps");

    Ptr<PacketSink> sink3 = serverApps.Get(3)->GetObject<PacketSink>();
    NS_LOG_UNCOND("UE4 throughput :" << sink3->GetTotalRx() * 8.0 / 1000000.0 << " Mbps");

    Ptr<PacketSink> sink4 = serverApps.Get(4)->GetObject<PacketSink>();
    NS_LOG_UNCOND("UE5 throughput :" << sink4->GetTotalRx() * 8.0 / 1000000.0 << " Mbps");

    Ptr<PacketSink> sink5 = serverApps.Get(5)->GetObject<PacketSink>();
    NS_LOG_UNCOND("UE6 throughput :" << sink5->GetTotalRx() * 8.0 / 1000000.0 << " Mbps");

    Simulator::Destroy();
    return 0;
}
