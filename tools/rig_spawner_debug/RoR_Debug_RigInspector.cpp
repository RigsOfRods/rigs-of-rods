/*

RIG INSPECTOR

Written 10/2014 by Petr "only_a_ptr" Ohlidal

This code logs all data contained within class Beam and associated data structs.

The purpose is to debug class RigSpawner, introduced in TruckParser2013 project 
	and replacing class SerializedRig.

*/

#include "RoR_Debug_RigInspector.h"

#include "FlexAirfoil.h"

void RigInspector::InspectWings(std::ofstream & f, Beam* rig)
{

	//wing_t wings[MAX_WINGS];
	//int free_wing;

	f << "\n\n***********************\n wings double check (free_wing:" << rig->free_wing<<")";
	for (int i = 0; i < rig->free_wing; i++)
	{
		wing_t & data = rig->wings[i];
		f<<"\n wing "<<i<<":"
			<<"(FlexAirfoil*) fa="<<ECHO_PTR(data.fa)
			<<"(Ogre::SceneNode*) cnode="<<ECHO_PTR(data.cnode);
			// <<" FOOOOOOO="<<data.BARRRRRRR
			// <<" FOOOOOOO="<<ECHO_PTR(data.PTRVARNAME)
			// <<" FOOOOOOO="<<ECHO_NODE(data.NODEPTR)
			// <<" FOOOOOOO="<<ECHO_BEAM(data.BEAMPTR)
			// <<" FOOOOOOO="<<ECHO_V3(data.OGREVECTOR3)
			// <<" FOOOOOOO="<<ECHO_QUAT(data.OGREQUATERNION)

		FlexAirfoil & fa = *data.fa;
		RigInspector::InspectFlexAirfoil(f, fa);
	}
}

void RigInspector::InspectFlexAirfoil(std::ofstream & f, FlexAirfoil & fa)
{
	f<<"\n FlexAirfoil:\n"
		<<" aoa="<<fa.aoa
		<<" type="<<fa.type
		<<" nfld="<<fa.nfld
		<<" nfrd="<<fa.nfrd
		<<" nflu="<<fa.nflu
		<<" nfru="<<fa.nfru
		<<" nbld="<<fa.nbld
		<<" nbrd="<<fa.nbrd
		<<" nblu="<<fa.nblu
		<<" nbru="<<fa.nbru
		<<" broken="<<fa.broken
		<<" breakable="<<fa.breakable
		<<" liftcoef="<<fa.liftcoef

		<<" (Ogre::MeshPtr)msh="<<!fa.msh.isNull()
		<<" subface="<<ECHO_PTR(fa.subface)
		<<" subband="<<ECHO_PTR(fa.subband)
		<<" subcup="<<ECHO_PTR(fa.subcup)
		<<" subcdn="<<ECHO_PTR(fa.subcdn)
		<<" decl="<<ECHO_PTR(fa.decl)
		<<" vbuf="<<fa.vbuf.isNull()
		<<" nVertices="<<fa.nVertices
		<<" vbufCount="<<fa.vbufCount

		/* SHADOW STUFF
			union
			{
				float *shadowposvertices;
				posVertice_t *coshadowposvertices;
			};
			union
			{
				float *shadownorvertices;
				norVertice_t *coshadownorvertices;
			};
			union
			{
				float *vertices;
				CoVertice_t *covertices;
			};
		*/

		<<" faceibufCount="<<fa.faceibufCount
		<<" bandibufCount="<<fa.bandibufCount
		<<" cupibufCount="<<fa.cupibufCount
		<<" cdnibufCount="<<fa.cdnibufCount

		<<" facefaces="<<ECHO_PTR(fa.facefaces)
		<<" bandfaces="<<ECHO_PTR(fa.bandfaces)
		<<" cupfaces="<<ECHO_PTR(fa.cupfaces)
		<<" cdnfaces="<<ECHO_PTR(fa.cdnfaces)

		<<" nodes(TODO:C_ARRAY)="<<ECHO_NODE(fa.nodes)

		<<" sref="<<fa.sref
		<<" deflection="<<fa.deflection
		<<" chordratio="<<fa.chordratio
		<<" hascontrol="<<fa.hascontrol
		<<" isstabilator="<<fa.isstabilator
		<<" stabilleft="<<fa.stabilleft
		<<" lratio="<<fa.lratio
		<<" rratio="<<fa.rratio
		<<" mindef="<<fa.mindef
		<<" maxdef="<<fa.maxdef
		<<" thickness="<<fa.thickness
		<<" useInducedDrag="<<fa.useInducedDrag
		<<" idSpan="<<fa.idSpan
		<<" idArea="<<fa.idArea
		<<" idLeft="<<fa.idLeft
		<<" airfoil="<<ECHO_PTR(fa.airfoil)
		<<" aeroengines="<<ECHO_PTR(fa.aeroengines)

		<<" free_wash="<<fa.free_wash;
		
		LOG_ARRAY(" waspropnum", fa.washpropnum, MAX_AEROENGINES);
		LOG_ARRAY(" washpropratio", fa.washpropratio, MAX_AEROENGINES);
		
		// <<" FOOOOOOO="<<fa.BARRRRRRR
		// <<" FOOOOOOO="<<ECHO_PTR(fa.PTRVARNAME)
		// <<" FOOOOOOO="<<ECHO_NODE(fa.NODEPTR)
		// <<" FOOOOOOO="<<ECHO_BEAM(fa.BEAMPTR)
		// <<" FOOOOOOO="<<ECHO_V3(fa.OGREVECTOR3)
		// <<" FOOOOOOO="<<ECHO_QUAT(fa.OGREQUATERNION)
		//
		// LOG_ARRAY("", , 10);
		
}