#include <iostream>
#include <fstream>
#include <sstream>

#include "ns3/log.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/energy-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/olsr-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
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
    DeviceEnergyModelContainer energies;
    
    int nNodes;
    int nSinks;
    int totalBytes;
    int totalPackets;
    int packetSize;

    void SetPosition(Ptr<Node> node, Vector position);
    Vector GetPosition(Ptr<Node> node);
    void DisplayNodesPosition();
    void CheckTransferredData();
    void CheckRemainingEnergy();

    void ReceivePacket(Ptr<Socket> socket);
    void SendPacket(Ptr<Socket> socket, InetSocketAddress to, int packetSize);
    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);
};

Experiment::Experiment(std::string routingProtocol) {
    this->nNodes = 25;
    this->nSinks = 100;
    this->totalBytes = 0;
    this->totalPackets = 0;
    this->packetSize = 64;
    std::string fieldSize = "ns3::UniformRandomVariable[Min=0.0|Max=120.0]";
    std::string nodeSpeed = "ns3::UniformRandomVariable[Min=0.0|Max=5]";
    std::string nodePause = "ns3::ConstantRandomVariable[Constant=5]"; 
    std::string dataRate = "2048bps";
    std::string dataMode = "DsssRate11Mbps";
    double wifiRadius = 100.0;

    this->c.Create(this->nNodes);

    // Setting MobilityModel
    for (int i = 1; i <= nNodes; ++i) {
        std::stringstream ss;
        ss << "out/agent_" << i;
        std::ifstream mm(ss.str().c_str());

        int r, g, b;
        mm >> r >> g >> b;

        ns3::Ptr<ns3::WaypointMobilityModel> mobility = CreateObject<ns3::WaypointMobilityModel>();
        ns3::ConstantVelocityHelper constantVelocityHelper;

        int last_time = -1;
        while(!mm.eof()){
            int x, y, time;
            mm >> x >> y >> time;

            if (time == last_time) break;

            //std::cout << x << " " << y << " " << time << "\n";

            ns3::Waypoint waypoint(ns3::Seconds(time), ns3::Vector(x, y, 0)); 
            mobility->AddWaypoint(waypoint);

            last_time = time;
        }

        this->c.Get(i-1)->AggregateObject(mobility);
    }

    // Global configs
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(dataMode));

    // Wifi and Channel
    WifiHelper wifi;
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();

    wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
    wifiPhy.Set("RxGain", DoubleValue(5));
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    // wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel", "Rss", DoubleValue(-40));
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(wifiRadius));
    wifiPhy.SetChannel(wifiChannel.Create());

    // MAC address and disable rate control
    WifiMacHelper wifiMac;

    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
        "DataMode", StringValue(dataMode),
        "ControlMode", StringValue(dataMode));
    wifiPhy.Set("TxPowerStart", DoubleValue(5));
    wifiPhy.Set("TxPowerEnd", DoubleValue(5));

    wifiMac.SetType("ns3::AdhocWifiMac");
    this->devices = wifi.Install(wifiPhy, wifiMac, this->c);

    // Energy

    BasicEnergySourceHelper basicSourceHelper;
    basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(10000.0));
    basicSourceHelper.Set("PeriodicEnergyUpdateInterval", TimeValue(Seconds(1.0)));

    EnergySourceContainer sources = basicSourceHelper.Install(this->c);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    radioEnergyHelper.Set("TxCurrentA", DoubleValue(5.0174));
    radioEnergyHelper.Set("RxCurrentA", DoubleValue(5.0174));

    this->energies = radioEnergyHelper.Install(this->devices, sources);

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

void Experiment::Run() {
    AnimationInterface anim("animation.xml");

    for (uint32_t i = 0; i < this->c.GetN(); ++i) {
        anim.UpdateNodeSize(i, 15, 15);

        std::stringstream ss;
        ss << "out/agent_" << (i+1);
        std::ifstream mm(ss.str().c_str());

        int r, g, b;
        if (mm >> r >> g >> b) {
            anim.UpdateNodeColor(i, r, g, b);
        }
    }

    anim.EnablePacketMetadata(true);
    anim.EnableIpv4RouteTracking("routingtable.xml",
        Seconds(0),
        Seconds(200),
        Seconds(0.25));
    anim.EnableWifiMacCounters(Seconds(0), Seconds(200));
    anim.EnableWifiPhyCounters(Seconds(0), Seconds(200));

    // Simulator::Schedule(Seconds(1.0), &Experiment::DisplayNodesPosition, this);

    std::vector<Ptr<Socket> > sendSockets;

    // Opening receive and send sockets in every node

    for (int i = 0; i < this->nNodes; i++) {
        SetupPacketReceive(this->interfaces.GetAddress(i), this->c.Get(i));

        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        Ptr<Socket> sendSocket = Socket::CreateSocket(this->c.Get(i), tid);
        InetSocketAddress local = InetSocketAddress(this->interfaces.GetAddress(i), 15);
        sendSocket->Bind(local);
        sendSockets.push_back(sendSocket);  
    }


    for (int i = 0; i < this->nSinks; i++) {
        Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();

        int from = var->GetInteger(0, this->nNodes - 1);
        int to = var->GetInteger(0, this->nNodes - 1);

        while (to == from) {
            to = var->GetInteger(0, this->nNodes - 1);
        }

        Ptr<Socket> fromSocket = sendSockets[from];
        InetSocketAddress addressTo(this->interfaces.GetAddress(to), 9);

        int time = var->GetInteger(5, 195);
        Simulator::Schedule(Seconds(time), &Experiment::SendPacket, this, fromSocket, addressTo, this->packetSize);
        
        // AddressValue remoteAddress(InetSocketAddress(this->interfaces.GetAddress(i), 9));
        // onoff1.SetAttribute("Remote", remoteAddress);

        // Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();
        // ApplicationContainer temp = onoff1.Install(this->c.Get(i + this->nSinks));
        // temp.Start(Seconds(100.0));
        // temp.Stop(Seconds(200.0));
    }

    Simulator::Schedule(Seconds(0.0), &Experiment::CheckRemainingEnergy, this);


    Simulator::Schedule(Seconds(200.0), &Experiment::CheckRemainingEnergy, this);


    CheckTransferredData();

    // sockets[0]->SendTo(new Packet(64), 0, InetSocketAddress(this->interfaces.GetAddress(15), 9));

    Simulator::Stop(Seconds(200.0));
    Simulator::Run();
    Simulator::Destroy();
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

void Experiment::SendPacket(Ptr<Socket> socket, InetSocketAddress to, int packetSize) {
    socket->SendTo(new Packet(packetSize), 0, to);
}

void Experiment::CheckRemainingEnergy() {
    for (int i = 0; i < this->nNodes; i++) {
        Ptr<DeviceEnergyModel> model = this->energies.Get(i);
        std::cout << "Node [" << i << "] Energy consumption: " << model->GetTotalEnergyConsumption() << std::endl;
    }
}

int main(int argc, char *argv[]) {

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    CommandLine cmd;
    cmd.Parse(argc, argv);

    Experiment expB = Experiment("BATMAN");
    expB.Run();
    

    // Experiment expO = Experiment("OLSR");
    // expO.Run();
    
    return 0;
}
