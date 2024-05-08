#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MANET_EXPERIMENT");


class Experiment {
public:
    Experiment (int nNodes);
    void Run();
private:
    NodeContainer c;
    int nNodes;

    void SetPosition(Ptr<Node> node, Vector position);
    Vector GetPosition(Ptr<Node> node);
    void DisplayNodesPosition();
};

void Experiment::Run() {
    Simulator::Schedule(Seconds(1.0), &Experiment::DisplayNodesPosition, this);

    Simulator::Schedule(Seconds(3.0), &Experiment::DisplayNodesPosition, this);

    Simulator::Stop(Seconds(5));
    Simulator::Run();
    Simulator::Destroy();
}

Experiment::Experiment(int nNodes) {
    this->nNodes = nNodes;

    c.Create(this->nNodes);

    // Setting MobilityModel
    MobilityHelper mobility;
    ObjectFactory pos;

    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
    Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();

    mobility.SetPositionAllocator(taPositionAlloc);
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
        "Speed", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=20]"),
        "Pause", StringValue("ns3::ConstantRandomVariable[Constant=0]"),
        "PositionAllocator", PointerValue(taPositionAlloc));
    mobility.Install(c);
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


int main(int argc, char *argv[]) {
    int nNodes = 10;

    CommandLine cmd;
    cmd.AddValue ("nNodes", "number of nodes", nNodes);
    cmd.Parse(argc, argv);

    Experiment exp = Experiment(nNodes);
    exp.Run();
    
    return 0;
}