#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

using namespace ns3;

NodeContainer stas;
NodeContainer ap;

double PLE = 3.0;
int sum = 0;
void Position(std::string context, Ptr<const MobilityModel> model)
{
    // std::cout << context << std::endl;
    // Vector position = model->GetPosition();
    // std::cout << " x: " << position.x
    //            << " y: " << position.y
    //            << " z: " << position.z << std::endl;
}

void Monitor(std::string context, Ptr<const Packet> packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, SignalNoiseDbm signalNoise, uint16_t staId)
{
    Ptr<MobilityModel> apMobility = ap.Get(0)->GetObject<MobilityModel>();
    Vector position_ap = apMobility->GetPosition();
    // std::cout << " x: " << position_ap.x
    //           << " y: " << position_ap.y
    //           << " z: " << position_ap.z << std::endl;

    Ptr<MobilityModel> stasMobility = stas.Get(0)->GetObject<MobilityModel>();
    Vector position_sta = stasMobility->GetPosition();
    // std::cout << " x: " << position_sta.x
    //            << " y: " << position_sta.y
    //            << " z: " << position_sta.z << std::endl;
    
    double distance = CalculateDistance(position_sta, position_ap);
    std::cout << "distance:" << distance << std::endl;
    std::cout << "signal:" << signalNoise.signal << std::endl;

    // sum += packet->GetSize();
}


int main(int argc, char *argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    WifiHelper wifi;
    // NodeContainer stas;
    // NodeContainer ap;
    NetDeviceContainer staDevs;
    NetDeviceContainer apDevs;

    // TODO 1: create one ap node and one wifi node
    ap.Create(1);
    stas.Create(1);
    // TODO 1

    // TODO 2: change propagation model here
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    
    wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                   "Exponent", DoubleValue(3.0),
                                   "ReferenceDistance", DoubleValue(1.0),
                                   "ReferenceLoss", DoubleValue(46.677));
                                    // spec
    // wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
    //                                 "m0", DoubleValue(1.0),
    //                                 "m1", DoubleValue(1.0),
    //                                 "m2", DoubleValue(1.0));
                                    // Rayleigh
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    // TODO 2

    wifiPhy.SetChannel(wifiChannel.Create());

    Ssid ssid = Ssid("wifi-default");
    wifi.SetRemoteStationManager("ns3::ArfWifiManager");

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::StaWifiMac",
                    "ActiveProbing", BooleanValue(true),
                    "Ssid", SsidValue(ssid));
    staDevs = wifi.Install(wifiPhy, wifiMac, stas);

    wifiMac.SetType("ns3::ApWifiMac",
                    "Ssid", SsidValue(ssid));
    apDevs = wifi.Install(wifiPhy, wifiMac, ap);
    
    /**Install mobility model**/
    // TODO 3: Initialize AP & wifi node position and specify movement
    MobilityHelper mobility;

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));

    // mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
    //                             "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"),
    //                             "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
    // mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
    //                         "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    //                         // spec

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");

    mobility.Install(stas);

    Ptr<ConstantVelocityMobilityModel> cvmm = stas.Get(0)->GetObject<ConstantVelocityMobilityModel>();
    cvmm->SetVelocity(Vector(20, 0, 0));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ap);
    // TODO 3

    /**Install IP stack**/
    InternetStackHelper stack;
    stack.Install(ap);
    stack.Install(stas);
    Ipv4AddressHelper address;
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer stainterface = address.Assign(staDevs);
    Ipv4InterfaceContainer apinterface = address.Assign(apDevs);
    address.Assign(apDevs);

    /**Install application**/
    UdpEchoServerHelper serv(9);
    ApplicationContainer servApps = serv.Install(ap.Get(0));
    servApps.Start(Seconds(0.0));
    servApps.Stop(Seconds(30.0));

    UdpEchoClientHelper client(apinterface.GetAddress(0), 9);
    client.SetAttribute("MaxPackets", UintegerValue(1000));
    client.SetAttribute("Interval", TimeValue(Seconds(1)));
    client.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = client.Install(stas.Get(0));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(30.0));

    /**Tracing source**/
    // EXAMPLE: tracing node position
    Config::Connect("/NodeList/1/$ns3::MobilityModel/CourseChange", MakeCallback(&Position));
    
    // TODO 4: hook SNR and throughput related trace source
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx", MakeCallback(&Monitor));
    // TODO 4

    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    Simulator::Destroy();

    // double throughput = sum * 8 / 30 / 1000000;
    // std::cout << "PLE:" << PLE << std::endl;
    // std::cout << "throughput:" << throughput << std::endl;

    return 0;
}