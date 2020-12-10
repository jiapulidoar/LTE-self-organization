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
#include "ns3/opengym-module.h"
#include "ns3/node-list.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LteMulticell");
uint16_t numberOfNodes = 6;
NodeContainer ueNodes;
NodeContainer enbNodes;

std::vector<double> throughput;

float last_throughput = 0;
int name = 0;
Ptr<LteUeRrc> lteuerrc = CreateObject<LteUeRrc>();
Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

NetDeviceContainer enbLteDevs;
NetDeviceContainer ueLteDevs;
enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
EpsBearer bearer(q);

ApplicationContainer clientApps;
ApplicationContainer serverApps;

std::vector<uint16_t> ueIdxConnection;

void plotDevices();
void setThroughput();

template <typename T>
std::string to_string_with_precision(const T a_value, const int n = 6)
{
  std::ostringstream out;
  out.precision(n);
  out << std::fixed << a_value;
  return out.str();
}

void handler(int ue, int enb)
{

  NS_LOG_UNCOND("Handler");

  Ptr<LteUeNetDevice> ueLteDevice = ueLteDevs.Get(ue)->GetObject<LteUeNetDevice>();
  lteuerrc = ueLteDevice->GetRrc();

  lteuerrc->ReleaseRrcResource();

  lteHelper->Attach(ueLteDevs.Get(ue), enbLteDevs.Get(enb));
}

static const std::string g_ueRrcStateName[LteUeRrc::NUM_STATES] =
    {
        "IDLE_START",
        "IDLE_CELL_SEARCH",
        "IDLE_WAIT_MIB_SIB1",
        "IDLE_WAIT_MIB",
        "IDLE_WAIT_SIB1",
        "IDLE_CAMPED_NORMALLY",
        "IDLE_WAIT_SIB2",
        "IDLE_RANDOM_ACCESS",
        "IDLE_CONNECTING",
        "CONNECTED_NORMALLY",
        "CONNECTED_HANDOVER",
        "CONNECTED_PHY_PROBLEM",
        "CONNECTED_REESTABLISHING"};

static const std::string &ToString(LteUeRrc::State s)
{
  return g_ueRrcStateName[s];
}

static void StateTransitionCallback(std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti, LteUeRrc::State oldState, LteUeRrc::State newState)
{
  if(newState==9 || oldState == 9 )
  std::cout << " UE IMSI " << imsi
            << " CellId " << cellId
            << " RNTI " << rnti
            << " State changed from: " << ToString(oldState)
            << " to: " << ToString(newState)
            << std::endl;
}

Ptr<OpenGymSpace> MyGetObservationSpace(void)
{
  float low = 0.0;
  float high = 100.0;
  uint32_t nodeNum = numberOfNodes * 3;
  std::vector<uint32_t> shape = {
      nodeNum,
  };
  std::string dtype = TypeNameGet<uint32_t>();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace>(low, high, shape, dtype);
  NS_LOG_UNCOND("MyGetObservationSpace: " << space);
  return space;
}

/*
Define action space
*/
Ptr<OpenGymSpace> MyGetActionSpace(void)
{
  uint32_t nodeNum = numberOfNodes * 3;
  float low = 0.0;
  float high = 100.0;
  std::vector<uint32_t> shape = {
      nodeNum,
  };
  std::string dtype = TypeNameGet<uint32_t>();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace>(low, high, shape, dtype);
  NS_LOG_UNCOND("MyGetActionSpace: " << space);
  return space;
}

/*
Define game over condition
*/
bool MyGetGameOver(void)
{
  bool isGameOver = false;
  NS_LOG_UNCOND("MyGetGameOver: " << isGameOver);
  return isGameOver;
}

/*
Collect observations
*/
Ptr<OpenGymDataContainer> MyGetObservation(void)
{

  uint32_t nodeNum = numberOfNodes * 3;
  std::vector<uint32_t> shape = {
      nodeNum,
  };
  Ptr<OpenGymBoxContainer<uint32_t>> box = CreateObject<OpenGymBoxContainer<uint32_t>>(shape);

  for (uint32_t i = 0; i < nodeNum; i++)
    box->AddValue(ueIdxConnection[i]);
  NS_LOG_UNCOND("MyGetObservation: " << box);
  return box;
}

/*
Define reward function
*/
float MyGetReward(void)
{
  float avg = 0;
  for (int i = 0; i < (int)throughput.size(); i++)
  {
    avg += throughput[i];
  }
  avg /= throughput.size();

  float reward = avg - last_throughput;
  NS_LOG_UNCOND("Reward: " << reward);

  if (reward > 0)
  {
    last_throughput = avg;
  }
  return reward;
}

/*
Define extra info. Optional
*/
std::string MyGetExtraInfo(void)
{
  std::string myInfo = "taller2 ";
  myInfo += "|123";
  NS_LOG_UNCOND("MyGetExtraInfo: " << myInfo);
  return myInfo;
}

/*
Execute received actions
*/

MobilityHelper mobility;
bool MyExecuteActions(Ptr<OpenGymDataContainer> action)
{
  plotDevices();
  //setThroughput();
  NS_LOG_UNCOND("MyExecuteActions: " << action);

  Ptr<OpenGymBoxContainer<uint32_t>> box = DynamicCast<OpenGymBoxContainer<uint32_t>>(action);
  std::vector<uint32_t> actionVector = box->GetData();

  for (int i = 0; i < (int)ueIdxConnection.size(); i++)
  {
    if (ueIdxConnection[i] != actionVector[i])
    {
      handler(i, actionVector[i]);
      ueIdxConnection[i] = actionVector[i];
    }
  }

  return true;
}

void ScheduleNextStateRead(double envStepTime, Ptr<OpenGymInterface> openGym)
{
  Simulator::Schedule(Seconds(envStepTime), &ScheduleNextStateRead, envStepTime, openGym);

  openGym->NotifyCurrentState();
}

void setThroughput()
{

  for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
  {
    Ptr<PacketSink> sink = serverApps.Get(u)->GetObject<PacketSink>();
    NS_LOG_UNCOND("UE" << u + 1 << " throughput :" << (sink->GetTotalRx() * 8.0 / 1000000.0) - throughput[u] << " Mbps ");

    throughput[u] = (sink->GetTotalRx() * 8.0 / 1000000.0) - throughput[u];
  }

   NS_LOG_UNCOND(std::to_string(name)  + ',' +  std::to_string(last_throughput) );

}

void plotDevices()
{

  setThroughput();

  std::string fileNameWithNoExtension = "nodes" + std::to_string(name++);
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
  plot.AppendExtra("set yrange [-1.2:+1.2]");
  plot.AppendExtra("set pointsize 3");
  plot.AppendExtra("set terminal png size 700, 700 font ',9'");

  // Instantiate the dataset, set its title, and make the points be
  // plotted along with connecting lines.
  Gnuplot2dDataset dataset;
  dataset.SetTitle("eNodeB");
  dataset.SetStyle(Gnuplot2dDataset::POINTS);
  dataset.SetExtra("pointtype 3");

  for (uint32_t u = 0; u < enbNodes.GetN(); ++u)
  {
    Ptr<Node> ueNode = enbNodes.Get(u);
    Ptr<MobilityModel> mob = ueNode->GetObject<MobilityModel>();

    dataset.Add(mob->GetPosition().x, mob->GetPosition().y);
  }

  // Add the dataset to the plot.
  plot.AddDataset(dataset);

  // Edges  dataset

  Gnuplot2dDataset dataset_graph;
  dataset_graph.SetStyle(Gnuplot2dDataset::VECTORS); // update gnuplot.h gnuplot.cc
  dataset_graph.SetErrorBars(Gnuplot2dDataset::XY);
  dataset_graph.SetTitle("Conexión");

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

    plot.AppendExtra("set label 'UE" + std::to_string(u + 1) + "' at " + std::to_string(mob->GetPosition().x) + "," + std::to_string(mob->GetPosition().y - 0.05));

    plot.AppendExtra("set label '" + to_string_with_precision(throughput[u], 3) + " mbps' at " + std::to_string(mob->GetPosition().x) + "," + std::to_string(mob->GetPosition().y - 0.1));

    // Extract edges

    Ptr<NetDevice> ueDevice = ueNodes.Get(u)->GetDevice(0);
    Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice>();
    lteuerrc = ueLteDevice->GetRrc();
    int cellid = ueIdxConnection[u];

    Ptr<Node> enbNode = enbNodes.Get(cellid);
    Ptr<MobilityModel> mob_enb = enbNode->GetObject<MobilityModel>();

    double x = mob->GetPosition().x;
    double y = mob->GetPosition().y;
    double xErrorDelta = mob_enb->GetPosition().x - x;
    double yErrorDelta = mob_enb->GetPosition().y - y;

    dataset_graph.Add(x, y, xErrorDelta, yErrorDelta);

    NS_LOG_UNCOND("ENB" << cellid << ",UE" << u + 1);
  }

  plot.AppendExtra("set label 'avg = " + std::to_string(last_throughput) + "' at  -1,1");

  // Add the dataset to the plot.
  plot.AddDataset(dataset_graph);
  plot.AddDataset(dataset2);

  // Open the plot file.
  std::ofstream plotFile(plotFileName.c_str());

  // Write the plot file.
  plot.GenerateOutput(plotFile);

  // Close the plot file.
  plotFile.close();
}

/*
----------------------------------- main ---------------------------------------------
*/
int main(int argc, char *argv[])
{
  //LogComponentEnable("EpcTftClassifier",LOG_LEVEL_INFO);

  uint32_t openGymPort = 5555;
  int f_RandomPositionAllocator = 1;
  double envStepTime = 0.5;   //seconds, ns3gym env step time interval
  RngSeedManager::SetSeed(2); // Changes seed from default of 1 to 3
  RngSeedManager::SetRun(7);  // Changes run number from default of 1 to 7

  double simTime = 25.5;
  double interPacketInterval = 1;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);

  //float p = 1 / numberOfNodes;

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

  enbNodes.Create(numberOfNodes);
  ueNodes.Create(3 * numberOfNodes);

  for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    throughput.push_back(0);

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
  enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
  ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

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

  for (uint16_t i = 0; i < ueNodes.GetN(); i++)
  {
    Ptr<UniformRandomVariable> v = CreateObject<UniformRandomVariable>();

    uint16_t enbid = ceil(v->GetValue() * numberOfNodes) - 1;

    lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(enbid));

    ueIdxConnection.push_back(enbid);
  }

  lteHelper->ActivateDedicatedEpsBearer(ueLteDevs, bearer, EpcTft::Default());

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;

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

  Ptr<OpenGymInterface> openGymInterface = CreateObject<OpenGymInterface>(openGymPort);
  openGymInterface->SetGetActionSpaceCb(MakeCallback(&MyGetActionSpace));
  openGymInterface->SetGetObservationSpaceCb(MakeCallback(&MyGetObservationSpace));
  openGymInterface->SetGetGameOverCb(MakeCallback(&MyGetGameOver));
  openGymInterface->SetGetObservationCb(MakeCallback(&MyGetObservation));
  openGymInterface->SetGetRewardCb(MakeCallback(&MyGetReward));
  openGymInterface->SetGetExtraInfoCb(MakeCallback(&MyGetExtraInfo));
  openGymInterface->SetExecuteActionsCb(MakeCallback(&MyExecuteActions));

  Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/StateTransition",
                  MakeCallback(&StateTransitionCallback));
  Simulator::Schedule(Seconds(0.5), &ScheduleNextStateRead, envStepTime, openGymInterface);

  //Simulator::Schedule(Seconds(1), &plotDevices);
  //Simulator::Schedule(Seconds(1), &handler);

  NS_LOG_UNCOND("Simulation start");
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  float avg = 0;
  for (int i = 0; i < (int)throughput.size(); i++)
  {
    avg += throughput[i];
  }
  avg /= throughput.size();

  last_throughput = avg;

  plotDevices();

  NS_LOG_UNCOND("Simulation stop");

  openGymInterface->NotifySimulationEnd();
  Simulator::Destroy();

  return 0;
}
