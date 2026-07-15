#ifndef __H02TRACKINGACTION_H__
#define __H02TRACKINGACTION_H__

//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file runAndEvent/H02/src/TrackingAction.cc
/// \brief Implementation of the TrackingAction class
//
//
#include "OutputRunAction.hh"
// #include "TrackInformation.hh"
#include "TrackingAction.hh"

#include "G4TrackingManager.hh"
#include "G4Track.hh"
#include "G4Trajectory.hh"
#include "G4RunManager.hh"
// #include "DataStorage.hh"

#include "SteppingAction.hh"
//#include "FullTrajectoryInfo.hh"
#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
using namespace std;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
TrackingAction::TrackingAction()
	: G4UserTrackingAction()
{
	;
}

void TrackingAction::PreUserTrackingAction(const G4Track*aTrack) 
{
	auto runGeneratorAction = static_cast<const PrimaryGeneratorAction *>(G4RunManager::GetRunManager()->GetUserPrimaryGeneratorAction());
	G4String GeneratorName = runGeneratorAction->GetGeneratorName();
	Full_trajectory_info_data &trajectories = Full_trajectory_info_data::GetInstance();
	G4int ParentID = aTrack->GetParentID();
	if (aTrack->GetParentID() == 0)
	{
	        FullTrajectoryInfo trjInfo;
		trjInfo.is_conversion_track = false;
		trjInfo.fParentID = aTrack->GetDynamicParticle()->GetPrimaryParticle()->GetTrackID();
		trjInfo.fTrackID = aTrack->GetTrackID();
		trjInfo.fPDGCharge = aTrack->GetDynamicParticle()->GetPrimaryParticle()->GetCharge();
		trjInfo.fPDGCode = aTrack->GetDefinition()->GetPDGEncoding();
		trjInfo.fMomentum = aTrack->GetDynamicParticle()->GetPrimaryParticle()->GetMomentum();
		trjInfo.fMomentumDir = aTrack->GetDynamicParticle()->GetPrimaryParticle()->GetMomentumDirection();
		trjInfo.fEnergy = aTrack->GetDynamicParticle()->GetPrimaryParticle()->GetTotalEnergy();
		trjInfo.fMass = aTrack->GetDynamicParticle()->GetPrimaryParticle()->GetMass();

		trjInfo.fVertexPosition = aTrack->GetVertexPosition();
		trjInfo.fGlobalTime = aTrack->GetGlobalTime();
		
		trjInfo.caloExtrapolMaxEkin = 0.0;
		trjInfo.caloExtrapolEta     = trjInfo.fMomentum.getEta();
		trjInfo.caloExtrapolPhi     = GetPhi( trjInfo.fMomentum.x(),
						      trjInfo.fMomentum.y() );
		
		trjInfo.idExtrapolMaxEkin = trjInfo.caloExtrapolMaxEkin;
		trjInfo.idExtrapolEta     = trjInfo.caloExtrapolEta;
		trjInfo.idExtrapolPhi     = trjInfo.caloExtrapolPhi;
		
		trjInfo.vTrackMomentumDir.push_back(aTrack->GetMomentum());
		trjInfo.vTrackID.push_back(aTrack->GetTrackID());
		trjInfo.vParentID.push_back(aTrack->GetParentID());
		trjInfo.vTrackPos.push_back(aTrack->GetPosition());
		trjInfo.vTrackTime.push_back(aTrack->GetGlobalTime());
		trjInfo.vTrackPDGID.push_back(aTrack->GetDefinition()->GetPDGEncoding());

		trajectories.m_trackid_to_traj[trjInfo.fTrackID] = (int)trajectories.fAllTrajectoryInfo.size();
		if (trajectories.fAllTrajectoryInfo.size() < 5) {
			G4double px = trjInfo.fMomentum.x(), py = trjInfo.fMomentum.y();
			std::cout << "[TrackingAction] primary #" << trajectories.fAllTrajectoryInfo.size()
			          << " pdg=" << trjInfo.fPDGCode
			          << " pT(Geant4units)=" << std::sqrt(px*px+py*py)
			          << " pT(GeV)=" << std::sqrt(px*px+py*py)/1000.0 << std::endl;
		}
		trajectories.fAllTrajectoryInfo.push_back(trjInfo);
		//!Pythia8
		if (GeneratorName == "pythia8")
		{
			if (aTrack->GetDynamicParticle()->GetPrimaryParticle()->GetCharge() == 0)
			{
				float Enu = aTrack->GetDynamicParticle()->GetPrimaryParticle()->GetTotalEnergy();
				(void) Enu;
			}
			else
			{
				float Ech = aTrack->GetDynamicParticle()->GetPrimaryParticle()->GetTotalEnergy();
				(void) Ech;
			}
			// trajectories.AddTrueEnergy(Ech, Enu);
		}
		//!Pythia8 END
		//break;
	} // if(trj->GetParentID() == 0)
	else
	{
		// fAllTrajectoryInfo: O(1) lookup via hash map instead of O(n*m) nested scan
		{
			auto it = trajectories.m_trackid_to_traj.find(ParentID);
			if (it != trajectories.m_trackid_to_traj.end())
			{
				int mTraj = it->second;
				trajectories.fAllTrajectoryInfo.at(mTraj).vTrackMomentumDir.push_back(aTrack->GetMomentum());
				trajectories.fAllTrajectoryInfo.at(mTraj).vParentID.push_back(aTrack->GetParentID());
				trajectories.fAllTrajectoryInfo.at(mTraj).vTrackID.push_back(aTrack->GetTrackID());
				trajectories.fAllTrajectoryInfo.at(mTraj).vTrackPos.push_back(aTrack->GetPosition());
				trajectories.fAllTrajectoryInfo.at(mTraj).vTrackTime.push_back(aTrack->GetGlobalTime());
				trajectories.fAllTrajectoryInfo.at(mTraj).vTrackPDGID.push_back(aTrack->GetDefinition()->GetPDGEncoding());
				trajectories.m_trackid_to_traj[aTrack->GetTrackID()] = mTraj;
			}
		}
		// fAllConvElectrons: linear search (small container, rare case)
		{
			bool foundTraj(false);
			int mTraj(-1);
			for (int iTraj = (int)trajectories.fAllConvElectrons.size() - 1; iTraj >= 0; iTraj--)
			{
				for (int iParent = (int)trajectories.fAllConvElectrons.at(iTraj).vTrackID.size() - 1; iParent >= 0; iParent--)
				{
					if (ParentID == trajectories.fAllConvElectrons.at(iTraj).vTrackID.at(iParent))
					{
						foundTraj = true;
						mTraj = iTraj;
						break;
					}
				}
				if (foundTraj) break;
			}
			if (foundTraj)
			{
				trajectories.fAllConvElectrons.at(mTraj).vTrackMomentumDir.push_back(aTrack->GetMomentum());
				trajectories.fAllConvElectrons.at(mTraj).vParentID.push_back(aTrack->GetParentID());
				trajectories.fAllConvElectrons.at(mTraj).vTrackID.push_back(aTrack->GetTrackID());
				trajectories.fAllConvElectrons.at(mTraj).vTrackPos.push_back(aTrack->GetPosition());
				trajectories.fAllConvElectrons.at(mTraj).vTrackTime.push_back(aTrack->GetGlobalTime());
				trajectories.fAllConvElectrons.at(mTraj).vTrackPDGID.push_back(aTrack->GetDefinition()->GetPDGEncoding());
			}
		}
	}	  //if(aTrack->GetParentID() != 0)
	if ( IsPrimaryPhotonDaughter( aTrack ) &&
	     IsInnerDetectorTrack( aTrack ) ) {

	    FullTrajectoryInfo conv_el_tr;
		conv_el_tr.is_conversion_track = true;
		conv_el_tr.fPDGCharge = aTrack->GetDynamicParticle()->GetCharge();
		conv_el_tr.fMomentumDir = aTrack->GetDynamicParticle()->GetMomentumDirection();
		conv_el_tr.fEnergy = aTrack->GetDynamicParticle()->GetTotalEnergy();
		conv_el_tr.fMass = aTrack->GetDynamicParticle()->GetMass();
		conv_el_tr.fTrackID        = aTrack->GetTrackID();
		conv_el_tr.fPDGCode        = 22; //Choose to label conv. electron track as a photon //aTrack->GetDefinition()->GetPDGEncoding();
		conv_el_tr.fMomentum       = aTrack->GetMomentum();
		conv_el_tr.caloExtrapolMaxEkin = 0.0;
		conv_el_tr.caloExtrapolEta     = conv_el_tr.fMomentum.getEta();
		conv_el_tr.caloExtrapolPhi     = GetPhi( conv_el_tr.fMomentum.x(),conv_el_tr.fMomentum.y() );
		conv_el_tr.idExtrapolMaxEkin = conv_el_tr.caloExtrapolMaxEkin;
		conv_el_tr.idExtrapolEta     = conv_el_tr.caloExtrapolEta;
		conv_el_tr.idExtrapolPhi     = conv_el_tr.caloExtrapolPhi;
		conv_el_tr.fVertexPosition = aTrack->GetVertexPosition();
		conv_el_tr.fGlobalTime     = aTrack->GetGlobalTime();
		conv_el_tr.vTrackMomentumDir.push_back(aTrack->GetMomentum());
		conv_el_tr.vParentID.push_back(aTrack->GetParentID());
		conv_el_tr.vTrackID.push_back(aTrack->GetTrackID());
		conv_el_tr.vTrackPos.push_back(aTrack->GetPosition());
		conv_el_tr.vTrackTime.push_back(aTrack->GetGlobalTime());
		conv_el_tr.vTrackPDGID.push_back(aTrack->GetDefinition()->GetPDGEncoding());

	    for( size_t i_primary_tr = 0; i_primary_tr < trajectories.fAllTrajectoryInfo.size(); ++i_primary_tr ) {
		if ( trajectories.fAllTrajectoryInfo[i_primary_tr].fTrackID == aTrack->GetParentID() ) {
		    conv_el_tr.fParentID = i_primary_tr;
		    break;
		}		    
	    }

	    trajectories.fAllConvElectrons.push_back( conv_el_tr );
	    
	}
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void TrackingAction::PostUserTrackingAction(const G4Track*)
{}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
bool TrackingAction::IsPrimaryPhotonDaughter(const G4Track* aTrack) const {

    if ( fabs( aTrack->GetDefinition()->GetPDGEncoding() ) != 11 )
	return false;

    G4int parentID                = aTrack->GetParentID();
    for ( const FullTrajectoryInfo& primaryParticle : Full_trajectory_info_data::GetInstance().fAllTrajectoryInfo ) {
	if ( primaryParticle.fTrackID == parentID &&
	     primaryParticle.fPDGCode == 22 )
	    return true;
    }

    return false;
    
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
bool TrackingAction::IsInnerDetectorTrack(const G4Track* aTrack) const {

    G4String logicalVolumeName = aTrack->GetVolume()->GetName();
    logicalVolumeName.toLower();
    
    return logicalVolumeName( 0, 5 ) == "inner";
    
}

#endif // __H02TRACKINGACTION_H__
