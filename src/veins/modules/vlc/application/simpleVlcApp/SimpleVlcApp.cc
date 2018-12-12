#include <omnetpp.h>
#include <veins/modules/vlc/application/simpleVlcApp/SimpleVlcApp.h>

using namespace Veins;

Define_Module(Veins::SimpleVlcApp);

SimpleVlcApp::SimpleVlcApp()
    : traciManager(NULL)
    , mobility(NULL)
    , annotations(NULL)
{
}

SimpleVlcApp::~SimpleVlcApp()
{
}

void SimpleVlcApp::initialize(int stage)
{
    if (stage == 0) {
        toLower = findGate("lowerLayerOut");
        fromLower = findGate("lowerLayerIn");

        // Pointers to simulation modules
        traciManager = TraCIScenarioManagerAccess().get();
        ASSERT(traciManager);

        cModule* tmpMobility = getParentModule()->getSubmodule("veinsmobility");
        mobility = dynamic_cast<TraCIMobility*>(tmpMobility);
        ASSERT(mobility);

        annotations = AnnotationManagerAccess().getIfExists();
        ASSERT(annotations);

        sumoId = mobility->getExternalId();
        debug = par("debug").boolValue();
        byteLength = par("packetByteLength");
        transmissionPeriod = 1 / par("beaconingFrequency").doubleValue();
    }
    if (stage == 3) {
        auto dsrc = [this]() {
            VlcMessage* vlcMsg = new VlcMessage();
            vlcMsg->setChannelNumber(Channels::CCH);
            vlcMsg->setAccessTechnology(DSRC);
            send(vlcMsg, toLower);
        };
        int vlcModule = BOTH_LIGHTS;
        auto vlc = [this, vlcModule]() {
            EV_INFO << "Sending VLC message of type: " << vlcModule << std::endl;
            VlcMessage* vlcMsg = generateVlcMessage(VLC, vlcModule);
            send(vlcMsg, toLower);
        };
        timerManager.create(Veins::TimerSpecification(vlc).oneshotAt(SimTime(20, SIMTIME_S)));
    }
}

void SimpleVlcApp::handleMessage(cMessage* msg)
{
    if (timerManager.handleMessage(msg)) return;

    if (msg->isSelfMessage()) {
        throw cRuntimeError("This module does not use custom self messages");
        return;
    }
    else {
        VlcMessage* vlcMsg = check_and_cast<VlcMessage*>(msg);
        int accessTech = vlcMsg->getAccessTechnology();
        switch (accessTech) {
        case DSRC: {
            EV_INFO << "DSRC message received!" << std::endl;
            delete vlcMsg;
            break;
        }
        case VLC: {
            EV_INFO << "VLC message received from: " << vlcMsg->getSourceNode() << std::endl;
            delete vlcMsg;
            break;
        }
        default:
            error("message neither from DSRC nor VLC");
            break;
        }
    }
}

VlcMessage* SimpleVlcApp::generateVlcMessage(int accessTechnology, int sendingModule)
{
    VlcMessage* vlcMsg = new VlcMessage();

    // OMNeT-specific
    vlcMsg->setName("vlcMessage");

    // WSM fields
    vlcMsg->setChannelNumber(Channels::CCH);

    // HeterogeneousMessage specific
    vlcMsg->setSourceNode(this->sumoId.c_str());
    vlcMsg->setDestinationNode("BROADCAST");
    vlcMsg->setAccessTechnology(accessTechnology);
    vlcMsg->setHeadOrTail(sendingModule);
    vlcMsg->setSentAt(simTime()); // There is timestamp field in WSM too

    // Set application layer packet length
    vlcMsg->setByteLength(byteLength);

    return vlcMsg;
}
