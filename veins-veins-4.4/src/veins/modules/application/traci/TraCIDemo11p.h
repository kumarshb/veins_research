//
// Copyright (C) 2006-2011 Christoph Sommer <christoph.sommer@uibk.ac.at>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef TraCIDemo11p_H
#define TraCIDemo11p_H

#include "veins/modules/application/ieee80211p/BaseWaveApplLayer.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"

using Veins::TraCIMobility;
using Veins::TraCICommandInterface;
using Veins::AnnotationManager;

/**
 * Small IVC Demo using 11p
 */
class TraCIDemo11p : public BaseWaveApplLayer {
	public:

		virtual void initialize(int stage);
		virtual void receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details);
		void finish();

	protected:

		TraCIMobility* mobility;
		TraCICommandInterface* traci;
		TraCICommandInterface::Vehicle* traciVehicle;
		AnnotationManager* annotations;

		simtime_t lastDroveAt;
		simtime_t wait_1_start;
		simtime_t wait_2_start;

		bool sentMessage;
		int numForcedMsgs = 0;
		int numBeacons = 0;
		int numWSMs = 0;

		int dos = 0;

		double packet_loss_ratio;

		bool isParking;
		bool sendWhileParking;
		static const simsignalwrap_t parkingStateChangedSignal;

		int wait1_done = 0;
		int wait2_done = 0;
	    int discard = 0;

		// weightedPersistence related variables
        const double max_r = 350.0;
        const double p_ij_limit = 0.5; // p_ijs higher than this get forwarded

		std::vector<WaveShortMessage> msg_log;

		std::queue<WaveShortMessage*> wait1_q;
		std::queue<WaveShortMessage*> wait2_q;

		std::pair<std::map<int,int>::iterator,bool> ret;
		std::map<int, int> neighbors; // useful to have a map of neighbors for future uses

		//entropy and flow sampling structures

		std::set<std::string> in_flows; // here string is csv of externID, senderAddr, WSM_Id
		std::set<std::string> out_flows;

	protected:
		virtual void onBeacon(WaveShortMessage* wsm);
		virtual void onData(WaveShortMessage* wsm);

		void sendMessage(std::string blockedRoadId);


		void handleSelfMsg(cMessage *msg);
		virtual void handlePositionUpdate(cObject* obj);
		virtual void handleParkingUpdate(cObject* obj);
		virtual void sendWSM(WaveShortMessage* wsm);
		virtual void printWSMLog();

		virtual double weightedPersistence(const Coord& remotePosition);
	    virtual double minPij(WaveShortMessage* wsm);
        virtual int dupWsmCount(WaveShortMessage* wsm);

        virtual int myDupWsmCount(WaveShortMessage* wsm);

        double nodeDistance(const Coord& remotePosition);
        double neighborDensity();

};

#endif
