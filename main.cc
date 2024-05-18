#include "ns3/log.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/olsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/batmand-helper.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MANET_EXPERIMENT");

static inline void PrintPacketInfo(Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress);

class Experiment {
public:
    Experiment(std::string routingProtocol);
    void Run();
private:
    NodeContainer c;
    NetDeviceContainer devices;
    Ipv4InterfaceContainer interfaces;
    int nNodes;
    int nSinks;
    int totalBytes;
    int totalPackets;

    void SetPosition(Ptr<Node> node, Vector position);
    Vector GetPosition(Ptr<Node> node);
    void DisplayNodesPosition();
    void CheckTransferredData();

    void ReceivePacket(Ptr<Socket> socket);
    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);
};

void Experiment::Run() {
    AnimationInterface anim("animation.xml");

    for (uint32_t i = 0; i < this->c.GetN(); ++i) {
        anim.UpdateNodeSize(i, 4, 4);
    }

    anim.EnablePacketMetadata(true);
    anim.EnableIpv4RouteTracking("routingtable.xml",
        Seconds(0),
        Seconds(200),
        Seconds(0.25));
    anim.EnableWifiMacCounters(Seconds(0), Seconds(200));
    anim.EnableWifiPhyCounters(Seconds(0), Seconds(200));

    // Simulator::Schedule(Seconds(1.0), &Experiment::DisplayNodesPosition, this);

    // Opening receive sockets in every node

    // for (int i = 0; i < this->nNodes; i++) {
    //     SetupPacketReceive(this->interfaces.GetAddress(i), this->c.Get(i));
    // }

    OnOffHelper onoff1("ns3::UdpSocketFactory", Address());
    onoff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

    for (int i = 0; i < this->nSinks; i++) {
        Ptr<Socket> sink = SetupPacketReceive(this->interfaces.GetAddress(i), this->c.Get(i));
        
        AddressValue remoteAddress(InetSocketAddress(this->interfaces.GetAddress(i), 9));
        onoff1.SetAttribute("Remote", remoteAddress);

        Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();
        ApplicationContainer temp = onoff1.Install(this->c.Get(i + this->nSinks));
        temp.Start(Seconds(100.0));
        temp.Stop(Seconds(200.0));
    }

    // AddressValue remoteAddress(InetSocketAddress(this->interfaces.GetAddress(12), 9));
    // onoff1.SetAttribute("Remote", remoteAddress);

    // Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();
    // ApplicationContainer temp = onoff1.Install(this->c.Get(14));
    // temp.Start(Seconds(120.0));
    // temp.Stop(Seconds(160.0));




    CheckTransferredData();

    Simulator::Stop(Seconds(200.0));
    Simulator::Run();
    Simulator::Destroy();
}

Experiment::Experiment(std::string routingProtocol) {
    this->nNodes = 25;
    this->nSinks = 10;
    this->totalBytes = 0;
    this->totalPackets = 0;
    std::string fieldSize = "ns3::UniformRandomVariable[Min=0.0|Max=120.0]";
    std::string nodeSpeed = "ns3::UniformRandomVariable[Min=0.0|Max=5]";
    std::string nodePause = "ns3::ConstantRandomVariable[Constant=5]"; 
    std::string packetSize = "64";
    std::string dataRate = "2048bps";
    std::string dataMode = "DsssRate11Mbps";
    double wifiRadius = 20.0;

    this->c.Create(this->nNodes);

    // Setting MobilityModel
    MobilityHelper mobility;
    ObjectFactory pos;

    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue(fieldSize));
    pos.Set("Y", StringValue(fieldSize));
    Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();

    mobility.SetPositionAllocator(taPositionAlloc);
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
        "Speed", StringValue(nodeSpeed),
        "Pause", StringValue(nodePause),
        "PositionAllocator", PointerValue(taPositionAlloc));
    mobility.Install(this->c);

    // Global configs
    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue(packetSize));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(dataRate));
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(dataMode));

    // Wifi and Channel
    WifiHelper wifi;
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();

    wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
    wifiPhy.Set("RxGain", DoubleValue(0));
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    // wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel", "Rss", DoubleValue(-40));
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(wifiRadius));
    wifiPhy.SetChannel(wifiChannel.Create());

    // MAC address and disable rate control
    WifiMacHelper wifiMac;

    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
        "DataMode", StringValue(dataMode),
        "ControlMode", StringValue(dataMode));
    // wifiPhy.Set("TxPowerStart", DoubleValue(2));
    // wifiPhy.Set("TxPowerEnd", DoubleValue(2));

    wifiMac.SetType("ns3::AdhocWifiMac");
    this->devices = wifi.Install(wifiPhy, wifiMac, this->c);

    // Routing protocol
    OlsrHelper olsr;
    BatmandHelper batman;
    Ipv4ListRoutingHelper list;
    InternetStackHelper internet;

    if (routingProtocol == "OLSR") {
        list.Add(olsr, 100);
    }
    else if (routingProtocol == "BATMAN") {
        list.Add(batman, 100);
    }

    internet.SetRoutingHelper(list);
    internet.Install(this->c);

    // Address assigning
    Ipv4AddressHelper ipAddresses;
    Ipv4InterfaceContainer ipInterface;

    ipAddresses.SetBase("10.0.0.0", "255.255.255.0");
    this->interfaces = ipAddresses.Assign(this->devices);
}

void Experiment::CheckTransferredData() {
    std::cout << "Time [" << Simulator::Now().GetSeconds() << "] Packets: " << this->totalPackets <<
        " Bytes: " << totalBytes << std::endl;
    
    Simulator::Schedule(Seconds(1.0), &Experiment::CheckTransferredData, this);
}

void Experiment::SetPosition(Ptr<Node> node, Vector position) {
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    mobility->SetPosition(position);
}

Vector Experiment::GetPosition(Ptr<Node> node) {
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    return mobility->GetPosition();
}

void Experiment::DisplayNodesPosition() {
    std::cout << "Displaying nodes position at " << Simulator::Now().GetSeconds()  << "s" << std::endl;
    for (int i = 0; i < this->nNodes; i++) {
        std::cout << "[" << i << "] " << GetPosition(c.Get(i)) << std::endl; 
    }
}

void Experiment::ReceivePacket(Ptr<Socket> socket) {
    Ptr<Packet> packet;
    Address senderAddress;

    while ((packet = socket->RecvFrom(senderAddress))) {
        this->totalBytes += packet->GetSize();
        this->totalPackets += 1;
        PrintPacketInfo(socket, packet, senderAddress);
    }
}

static inline void PrintPacketInfo(Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress) {
    std::cout << "[" << Simulator::Now().GetSeconds() << "] " << socket->GetNode()->GetId();

    if (InetSocketAddress::IsMatchingType(senderAddress)) { 
        InetSocketAddress addr = InetSocketAddress::ConvertFrom(senderAddress);
        std::cout << " received one packet from " << addr.GetIpv4() << std::endl;
    }
    else {
        std::cout << " received one packet" << std::endl;
    }
}

Ptr<Socket> Experiment::SetupPacketReceive(Ipv4Address addr, Ptr<Node> node) {
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> sink = Socket::CreateSocket(node, tid);
    InetSocketAddress local = InetSocketAddress(addr, 9);
    sink->Bind(local);
    sink->SetRecvCallback(MakeCallback(&Experiment::ReceivePacket, this));
    
    return sink;
}

int main(int argc, char *argv[]) {

    CommandLine cmd;
    cmd.Parse(argc, argv);

    //Experiment expB = Experiment("BATMAN");
    //expB.Run();
    

    Experiment expO = Experiment("OLSR");
    expO.Run();
    
    return 0;
}
