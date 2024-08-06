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

void Position(std::string context, Ptr<const MobilityModel> model)
{
  std::cout << context << std::endl;
  Vector position = model->GetPosition();
  std::cout << " x: " << position.x
            << " y: " << position.y
            << " z: " << position.z << std::endl;
}

void Monitor()
{
}

int main(int argc, char *argv[])
{
  CommandLine cmd(__FILE__);
  cmd.Parse(argc, argv);

  WifiHelper wifi;
  NodeContainer stas;
  NodeContainer ap;
  NetDeviceContainer staDevs;
  NetDeviceContainer apDevs;

  // TODO 1: create one ap node and one wifi node

  // TODO 2: change propagation model here
  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel;

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

  /**Install IP stack**/
  InternetStackHelper stack;
  stack.Install(ap);
  stack.Install(stas);
  Ipv4AddressHelper address;
  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer stainterface = address.Assign(staDevs);
  address.Assign(apDevs);

  /**Install application**/
  UdpEchoServerHelper serv(9);
  ApplicationContainer servApps = serv.Install(ap.Get(0));
  servApps.Start(Seconds(0.0));
  servApps.Stop(Seconds(30.0));

  UdpEchoClientHelper client(stainterface.GetAddress(0), 9);
  client.SetAttribute("MaxPackets", UintegerValue(1000));
  client.SetAttribute("Interval", TimeValue(Seconds(1)));
  client.SetAttribute("PacketSize", UintegerValue(1024));
  ApplicationContainer clientApps = client.Install(stas.Get(0));
  clientApps.Start(Seconds(1.0));
  clientApps.Stop(Seconds(30.0));

  /**Tracing source**/
  // EXAMPLE: tracing node position
  Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&Position));

  // TODO 4: hook SNR and throughput related trace source
  Config::Connect("", MakeCallback(&Monitor));

  Simulator::Stop(Seconds(30.0));
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}