#include <omnetpp.h>
#include <veins/modules/vlc/application/simpleVlcApp/SimpleVlcApp.h>

Define_Module(SimpleVlcApp);

using Veins::TraCIColor;

SimpleVlcApp::SimpleVlcApp():
    traciManager(NULL),
    mobility(NULL),
    timeoutManager(NULL),
    annotations(NULL)
{
}

SimpleVlcApp::~SimpleVlcApp()
{}

void SimpleVlcApp::initialize(int stage)
{
    if (stage == 0){
        toLower = findGate("lowerLayerOut");
        fromLower = findGate("lowerLayerIn");

        // Pointers to simulation modules
        traciManager = TraCIScenarioManagerAccess().get();
        ASSERT(traciManager);

        cModule *tmpMobility = getParentModule()->getSubmodule("veinsmobility");
        mobility = dynamic_cast<TraCIMobility*>(tmpMobility);
        ASSERT(mobility);

        cModule* tmp = getParentModule()->getSubmodule("timeoutManager");
        timeoutManager = dynamic_cast<TimeoutManager*>(tmp);
        ASSERT(timeoutManager);

        annotations = AnnotationManagerAccess().getIfExists();
        ASSERT(annotations);

        sumoId = mobility->getExternalId();
        debug = par("debug").boolValue();
        byteLength = par("packetByteLength").doubleValue();
        transmissionPeriod = 1/par("beaconingFrequency").doubleValue();
    }
    if(stage == 3){
        timeoutManager->setTimeout(this, SEND_VLC_BOTH, 1.0 + uniform(0.0,0.1) );
    }
}

void SimpleVlcApp::handleMessage(cMessage *msg){
    if (msg->isSelfMessage()){
        VOUT("This is wrong! There should be no SelfMessage!");
        delete msg;
    }
    else{
        VlcMessage *vlcMsg = check_and_cast<VlcMessage *>(msg);
        int accessTech = vlcMsg->getAccessTechnology();
        switch (accessTech) {
            case DSRC:
            {
                VOUT("DSRC message received!")
                delete vlcMsg;
                break;
            }
            case VLC:
            {
                VOUT("VLC message received!")
                delete vlcMsg;
                break;
            }
            default:
                error("message neither from DSRC nor VLC");
                break;
        }
    }
}

void SimpleVlcApp::onTimeout(std::string name, short int kind){
    Enter_Method("onTimeout");
    switch(kind)
    {
        case SEND_DSRC:
        {
            VlcMessage* vlcMsg = new VlcMessage();
            vlcMsg->setChannelNumber(Channels::CCH);
            vlcMsg->setAccessTechnology(DSRC);
            send(vlcMsg, toLower);
            break;
        }
        case SEND_VLC_BOTH:
        {
            VOUT("Broadcasting with both lighting modules")
            VlcMessage* vlcMsg = generateVlcMessage(VLC, BOTH_LIGHTS);
            send(vlcMsg, toLower);
            timeoutManager->setTimeout(this, SEND_VLC_BOTH, transmissionPeriod + uniform(0.0,0.1) );
            break;
        }
        case SEND_VLC_HEAD:
        {
            VlcMessage* vlcMsg = generateVlcMessage(VLC, HEADLIGHT);
            send(vlcMsg, toLower);
            break;
        }
        case SEND_VLC_TAIL:
        {
            VlcMessage* vlcMsg = generateVlcMessage(VLC, TAILLIGHT);
            send(vlcMsg, toLower);
            break;
        }
    }
}

VlcMessage* SimpleVlcApp::generateVlcMessage(int accessTechnology, int sendingModule){
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
    vlcMsg->setSentAt(simTime());    // There is timestamp field in WSM too

    // Set application layer packet length
    vlcMsg->setByteLength(byteLength);

    return vlcMsg;
}
