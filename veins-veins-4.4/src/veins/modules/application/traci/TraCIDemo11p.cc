
#include "veins/modules/application/traci/TraCIDemo11p.h"

using Veins::TraCIMobilityAccess;
using Veins::AnnotationManagerAccess;

const simsignalwrap_t TraCIDemo11p::parkingStateChangedSignal = simsignalwrap_t(TRACI_SIGNAL_PARKING_CHANGE_NAME);

Define_Module(TraCIDemo11p);

void TraCIDemo11p::initialize(int stage) {
	BaseWaveApplLayer::initialize(stage);
	if (stage == 0) {

		mobility = TraCIMobilityAccess().get(getParentModule());
		traci = mobility->getCommandInterface();
		traciVehicle = mobility->getVehicleCommandInterface();
		annotations = AnnotationManagerAccess().getIfExists();
		ASSERT(annotations);

		sentMessage = false;
		lastDroveAt = simTime();
		findHost()->subscribe(parkingStateChangedSignal, this);
		isParking = false;
		sendWhileParking = par("sendWhileParking").boolValue();

        wait1_done = 0;
        wait2_done = 0;
        discard = 0;

		//std::cout << "Vehicle : " << mobility->getExternalId() << " is born at " << simTime().dbl() << endl;
	}
}

void TraCIDemo11p::finish()
{
    //std::cout << "Vehicle : " << mobility->getExternalId() << "   Number of neighbors : " << neighbors.size() << endl;


    recordScalar("NumberOfBeacons", numBeacons);
    recordScalar("NumberOfWSMs", numWSMs);
    recordScalar("ForcedBroadcastMessages", numForcedMsgs);

    recordScalar("NeighborDensity", neighborDensity());
    recordScalar("NumberOfNeighbors", neighbors.size());

}

void TraCIDemo11p::onBeacon(WaveShortMessage* wsm) {

    // ONLY WORKS when sendbeacon is true in omnetpp.ini

    double distance = 0.0;
    int int_distance = 0;

    Coord wsm_pos;
    int node_address = 0;

    numBeacons++;

    wsm_pos = wsm->getSenderPos();
    distance = nodeDistance(wsm_pos);
    int_distance = (int)distance;

    if(distance <= max_r)
    {
        node_address = wsm->getSenderAddress();

        // add to my neighbors list
        ret = neighbors.insert(std::pair<int,int> (node_address, int_distance));

        if(ret.second == false)
            std::cout << "Failed to add duplicate neighbor" << endl;
    }

    std::cout << "Vehicle : " << mobility->getExternalId() << " Received BEACON from address: " << wsm->getSenderAddress() << " from distance :  " << distance << std::endl;
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
    double p_ij = 0; // weightedPersistence probability

    //std::cout << "Vehicle : " << mobility->getExternalId() << " simTime: " << simTime() << " entered with wait1_q.size() : " << wait1_q.size() << endl;

    if(wait1_q.size() > 0) // if there is anything to process in the wait1 queue
    {
        WaveShortMessage *mywsm = wait1_q.front();

        //std::cout << "Vehicle : " << mobility->getExternalId() << " simTime: " << simTime() << " processing after wait 1 timer with : " << mywsm->getWsmid() << endl;

        p_ij = minPij(mywsm);        // if wsm was received before - pick smallest pij among duplicates

        if (p_ij > p_ij_limit && discard == 0)
        {
            numWSMs++;

            Coord currentPosition = mobility->getCurrentPosition();
            mywsm->setSenderPos(currentPosition);

            sendDown(mywsm->dup());
            //std::cout << "Vehicle : " << mobility->getExternalId() << "with p = "<< p_ij << " Over "<< p_ij_limit <<" % p_ij Rebroadcasting wsmID : "<< mywsm->getWsmid() << std::endl;
        }
        else
        {
            //std::cout << "Vehicle : " << mobility->getExternalId() << "with p = "<< p_ij << " LESS "<< p_ij_limit <<" % p_ij Rebroadcasting wsmID : "<< mywsm->getWsmid() << std::endl;
            scheduleAt(simTime() + 0.003 + individualOffset, new cMessage); // lets start/schedule the waiting period # 2
            wait2_q.push(mywsm);
        }

        wait1_q.pop(); // processed this wsm - so remote it
    }

    if(wait2_q.size() > 0)
    {
        WaveShortMessage *mywsm = wait2_q.front();

        //std::cout << "Vehicle : " << mobility->getExternalId() << " Entered wait2 "<< mywsm->getWsmid() << " AND sim time is : " << simTime() << std::endl;

        // check if there any new messages with this wsmid
        if ( dupWsmCount(mywsm) > 1 ){

            //std::cout << "No force: During wait2 - there were : " << dupWsmCount(mywsm) << " messages with id " << mywsm->getWsmid() << endl;
            p_ij = 0;
        }
            else
                p_ij = 1; // force rebroadcast

        // force rebroadcast to guarantee 100% reachability
        if (p_ij == 1 && discard == 0)
        {
            numWSMs++;

            Coord currentPosition = mobility->getCurrentPosition();
            mywsm->setSenderPos(currentPosition);

            sendDown(mywsm->dup());
            numForcedMsgs++;
            //std::cout << std::endl << " Vehicle : " << mobility->getExternalId() << " Forced Rebroadcasting wsmID : " << mywsm->getWsmid() << std::endl;
        }

        wait2_q.pop();
    }
}

void TraCIDemo11p::onData(WaveShortMessage* wsm) {

    std::string iflow;

	findHost()->getDisplayString().updateWith("r=16,green");

	annotations->scheduleErase(1, annotations->drawLine(wsm->getSenderPos(), mobility->getPositionAt(simTime()), "blue"));

	if (mobility->getRoadId()[0] != ':') traciVehicle->changeRoute(wsm->getWsmData(), 9999);

    msg_log.push_back(*wsm); // add this new message to a msg log

    // flow tagging and sampling logic - we are initially tagging all incoming traffic

    // let's build the "unique" flow ID string to put into set

    iflow.append( mobility->getExternalId() );
    iflow.append(","); // making it csv
    iflow.append(std::to_string( wsm->getWsmid() ));
    iflow.append(",");
    iflow.append(std::to_string( wsm->getSenderAddress() ));

    //std::cout << "flow: " << iflow << endl << endl;

    in_flows.insert(iflow);

    //MALICIOUS NODE - rebroadcast if one of the 2 misbehaving nodes

    if(mobility->getExternalId() == "36" || mobility->getExternalId() == "62")
    {

        Coord currentPosition = mobility->getCurrentPosition();
        wsm->setSenderPos(currentPosition);
        wsm->setSenderAddress(stoi(mobility->getExternalId()));

        if(myDupWsmCount(wsm) < 5000)
        {
            //std::cout << "Vehicle : " << mobility->getExternalId() << " Misbehaving... wsmID : "<< wsm->getWsmid() << " & dupc = " << myDupWsmCount(wsm) << std::endl;
            sendDown(wsm->dup());

        }
        else
        {
            //std::cout << "Vehicle : " << mobility->getExternalId() << " NOW BEHAVING... wsmID : "<< wsm->getWsmid() << std::endl << std::endl;
        }

    }
    else
    {

    // end of mal code - proceed carefully without misbehavior accepting incoming and putting into rebroadcast queue


    if ( dupWsmCount(wsm) > 1) // discard immediately
    {
        discard = 1;
        //std::cout << "Vehicle : " << mobility->getExternalId() << " Discarding... wsmID : "<< wsm->getWsmid() << " count : " << dupWsmCount(wsm) << std::endl;
    }

    if(discard == 0)
    {
        //std::cout << "Vehicle : " << mobility->getExternalId() << " Scheduling ... wsmID : "<< wsm->getWsmid() << std::endl;

        WaveShortMessage *nwsm = new WaveShortMessage(*wsm);

        wait1_q.push(nwsm); // create FIFO scheduling - all incoming come as FIFO

        //std::cout << "Vehicle : " << mobility->getExternalId() << " indiv offset is "<< individualOffset << std::endl;
        scheduleAt(simTime() + 0.005 + individualOffset, new cMessage); // lets start/schedule the waiting period # 1 default + : 0.000000000104
    }

    }
}

void TraCIDemo11p::sendMessage(std::string blockedRoadId) {

	sentMessage = true;

	t_channel channel = dataOnSch ? type_SCH : type_CCH; // since via omnetpp.ini SCH is false - we are using CCH : channel for safety and channel mgmt

	WaveShortMessage* wsm = prepareWSM("data", dataLengthBits, channel, dataPriority, -1,2);

	wsm->setWsmData(blockedRoadId.c_str());

	std::string text = mobility->getExternalId();

	wsm->setWsmid(stoi(text)); // assign unique WSM message id

	//std::cout << "Vehicle stopped with ID :  " << text << " Sending WSM with id: "<< wsm->getWsmid() << std::endl;

	sendWSM(wsm);
}

void TraCIDemo11p::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details) {

	Enter_Method_Silent();

	if (signalID == mobilityStateChangedSignal) {
		handlePositionUpdate(obj);
	}
	else if (signalID == parkingStateChangedSignal) {
		handleParkingUpdate(obj);
	}
}

void TraCIDemo11p::handleParkingUpdate(cObject* obj) {
	isParking = mobility->getParkingState();
	if (sendWhileParking == false) {
		if (isParking == true) {
			(FindModule<BaseConnectionManager*>::findGlobalModule())->unregisterNic(this->getParentModule()->getSubmodule("nic"));
		}
		else {
			Coord pos = mobility->getCurrentPosition();
			(FindModule<BaseConnectionManager*>::findGlobalModule())->registerNic(this->getParentModule()->getSubmodule("nic"), (ChannelAccess*) this->getParentModule()->getSubmodule("nic")->getSubmodule("phy80211p"), &pos);
		}
	}
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj) {
	BaseWaveApplLayer::handlePositionUpdate(obj);

	// stopped for for at least 10s?
	if (mobility->getSpeed() < 1) {
		if (simTime() - lastDroveAt >= 10) {
			findHost()->getDisplayString().updateWith("r=16,red"); // turns RED when stopped
			if (!sentMessage) sendMessage(mobility->getRoadId());
		}
	}
	else {
		lastDroveAt = simTime();
	}
}

void TraCIDemo11p::sendWSM(WaveShortMessage* wsm) {
	if (isParking && !sendWhileParking) return;
	sendDelayedDown(wsm,individualOffset); // where individualOffset = dblrand() * maxOffset
}


/**************** CUSTOM FUNCTIONS    ***********************************************************************/

double TraCIDemo11p::neighborDensity(){
    int num_cars = 0;
    double zone_area = 0;
    double zone_density = 0;

    num_cars = neighbors.size();

    zone_area = 3.14 * (max_r * max_r);

    zone_density = (num_cars / zone_area) * 100000; // this to avoid decimals

    return zone_density;
}

void TraCIDemo11p::printWSMLog(){
    /*
	Prints the contents of the WSM log
    */

    std::cout << "Vehicle ID:  " << mobility->getExternalId() << " has the following WSMs: " << std::endl;

    for (std::vector<WaveShortMessage>::iterator it = msg_log.begin() ; it != msg_log.end(); ++it){
            std::cout << " WSM id: " << it->getWsmid() << " AND came from : " << it->getSenderAddress() << std::endl;
    }

}

int TraCIDemo11p::myDupWsmCount(WaveShortMessage* wsm){
    /*
    This function will scan the list of received wsms and return the count of wsms with this exact id
    */

    int count = 0;

    for (std::vector<WaveShortMessage>::iterator it = msg_log.begin() ; it != msg_log.end(); ++it){
        if ( it->getWsmid() == wsm->getWsmid() && wsm->getSenderAddress() == stoi(mobility->getExternalId())) {
            count++;
        }
    }

    return count;
}

int TraCIDemo11p::dupWsmCount(WaveShortMessage* wsm){
    /*
    This function will scan the list of received wsms and return the count of wsms with this exact id
    */

    int count = 0;

    for (std::vector<WaveShortMessage>::iterator it = msg_log.begin() ; it != msg_log.end(); ++it){
        if ( it->getWsmid() == wsm->getWsmid()) {
            count++;
        }
    }

    return count;
}


double TraCIDemo11p::minPij(WaveShortMessage* wsm){
    /*
    This function will scan the list of received wsms and return the one with smallest pij
    */
    double min_p_ij = 0;
    double tmp_p_ij = 0;

    Coord temp_pos;
    Coord wsm_pos;

    wsm_pos = wsm->getSenderPos();
    min_p_ij = weightedPersistence(wsm_pos); // this is the min pij for now

    for (std::vector<WaveShortMessage>::iterator it = msg_log.begin() ; it != msg_log.end(); ++it){
        if ( it->getWsmid() == wsm->getWsmid()) {
            // found duplicate wsm
            temp_pos = it->getSenderPos();
            tmp_p_ij = weightedPersistence(temp_pos);
            if (tmp_p_ij < min_p_ij)
                min_p_ij = tmp_p_ij;
        }
    }

    return min_p_ij;
}

double TraCIDemo11p::nodeDistance(const Coord& remotePosition)
{
    Coord currentPosition = mobility->getCurrentPosition();
    double d_ij = 0;

    d_ij = currentPosition.distance(remotePosition);

    return d_ij;
}

double TraCIDemo11p::weightedPersistence(const Coord& remotePosition){
    /*
    Weighted p-persistance assigns higher Pij to nodes located
    farther away from the broadcaster given the GPS information of the packet header
    The bigger the Pij - the farther are the nodes
    When node j receives a packet from node i - it will check the packet ID
    and rebroadcast with probability Pij = Dij/R, where Dij - distance
    between the nodes i and j and R is average transmission range (max_r = 350 meters)
    */

    Coord currentPosition = mobility->getCurrentPosition();

    double p_ij = 0.0;
    double d_ij = 0;
    double fp_ij = 1.0;

    d_ij = currentPosition.distance(remotePosition);

    if(d_ij <= 0)
      return p_ij;

    else if(d_ij > max_r)
    {
        numBeacons++;
        return fp_ij;
    }

    p_ij = d_ij / max_r;

    return p_ij;
}

