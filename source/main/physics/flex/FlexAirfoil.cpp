/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "FlexAirfoil.h"

#include "AeroEngine.h"
#include "Airfoil.h"
#include "ApproxMath.h"
#include "Actor.h"
#include "SimData.h"
#include "GfxActor.h"

using namespace RoR;

float refairfoilpos[90]={
        0.00, 0.50, 0.00,
        1.00, 0.50, 0.00,

        0.00, 0.70, 0.03,
        1.00, 0.70, 0.03,
        0.00, 0.30, 0.03,
        1.00, 0.30, 0.03,

        0.00, 0.90, 0.10,
        1.00, 0.90, 0.10,
        0.00, 0.10, 0.10,
        1.00, 0.10, 0.10,

        0.00, 1.00, 0.25,
        1.00, 1.00, 0.25,
        0.00, 0.00, 0.25,
        1.00, 0.00, 0.25,

        0.00, 1.00, 0.50,
        1.00, 1.00, 0.50,
        0.00, 0.00, 0.50,
        1.00, 0.00, 0.50,

        //updated with control surface chord ratio
        0.00, 0.75, 0.75,
        1.00, 0.75, 0.75,
        0.00, 0.25, 0.75,
        1.00, 0.25, 0.75,

        0.00, 0.75, 0.75,
        1.00, 0.75, 0.75,
        0.00, 0.25, 0.75,
        1.00, 0.25, 0.75,

        //moving with control surface
        0.00, 0.50, 1.00,
        1.00, 0.50, 1.00,

        0.00, 0.50, 1.00,
        1.00, 0.50, 1.00
    };

using namespace Ogre;

FlexAirfoil::FlexAirfoil(Ogre::String const & name, Actor* actor, int pnfld, int pnfrd, int pnflu, int pnfru, int pnbld, int pnbrd, int pnblu, int pnbru, std::string const & texband, Vector2 texlf, Vector2 texrf, Vector2 texlb, Vector2 texrb, char mtype, float controlratio, float mind, float maxd, Ogre::String const & afname, float lift_coef, bool break_able)
{
    liftcoef=lift_coef;
    breakable=break_able;
    broken=false;
    free_wash=0;
    aeroengines=actor->ar_aeroengines;
    nodes=actor->ar_nodes;
    useInducedDrag=false;
    nfld=pnfld;
    nfrd=pnfrd;
    nflu=pnflu;
    nfru=pnfru;
    nbld=pnbld;
    nbrd=pnbrd;
    nblu=pnblu;
    nbru=pnbru;
    mindef=mind;
    maxdef=maxd;
    airfoil=new Airfoil(afname);
    int i;
    for (i=0; i<90; i++) airfoilpos[i]=refairfoilpos[i];
    type=mtype;
    hascontrol=(mtype!='n' && mtype!='S'&& mtype!='T' && mtype!='U'&& mtype!='V');
    isstabilator=(mtype=='S' || mtype=='T' || mtype=='U' || mtype=='V');
    stabilleft=(mtype=='T' || mtype=='V');
    deflection=0.0;
    chordratio=controlratio;

    if (hascontrol)
    {
        //setup control surface
        airfoilpos[56]=controlratio;
        airfoilpos[56+3]=controlratio;
        airfoilpos[56+6]=controlratio;
        airfoilpos[56+9]=controlratio;

        airfoilpos[55]=-controlratio+1.5;
        airfoilpos[55+3]=-controlratio+1.5;
        airfoilpos[55+6]=controlratio-0.5;
        airfoilpos[55+9]=controlratio-0.5;
        for (i=0; i<12; i++) airfoilpos[54+12+i]=airfoilpos[54+i];
    }


   



    float tsref=2.0*(nodes[nfrd].RelPosition-nodes[nfld].RelPosition).crossProduct(nodes[nbld].RelPosition-nodes[nfld].RelPosition).length();
    sref=2.0*(nodes[nfrd].RelPosition-nodes[nfld].RelPosition).crossProduct(nodes[nbrd].RelPosition-nodes[nfrd].RelPosition).length();
    if (tsref>sref) sref=tsref;
    sref=sref*sref;

    lratio=(nodes[nfld].RelPosition-nodes[nflu].RelPosition).length()/(nodes[nfld].RelPosition-nodes[nbld].RelPosition).length();
    rratio=(nodes[nfrd].RelPosition-nodes[nfru].RelPosition).length()/(nodes[nfrd].RelPosition-nodes[nbrd].RelPosition).length();

    thickness=(nodes[nfld].RelPosition-nodes[nflu].RelPosition).length();

    //update coords
    this->updateVerticesPhysics();
    this->updateVerticesGfx(actor->GetGfxActor());

   
}

void FlexAirfoil::updateVerticesPhysics()
{
    Vector3 center;
    center=nodes[nfld].AbsPosition;

    Vector3 vx=nodes[nfrd].AbsPosition-nodes[nfld].AbsPosition;
    Vector3 vyl=nodes[nflu].AbsPosition-nodes[nfld].AbsPosition;
    Vector3 vzl=nodes[nbld].AbsPosition-nodes[nfld].AbsPosition;
    Vector3 vyr=nodes[nfru].AbsPosition-nodes[nfrd].AbsPosition;
    Vector3 vzr=nodes[nbrd].AbsPosition-nodes[nfrd].AbsPosition;

    if (breakable) {broken=broken || (vx.crossProduct(vzl).squaredLength()>sref)||(vx.crossProduct(vzr).squaredLength()>sref);}
    else {broken=(vx.crossProduct(vzl).squaredLength()>sref)||(vx.crossProduct(vzr).squaredLength()>sref);}

    //control surface
    if (hascontrol)
    {
        float radius=1.0-chordratio;
        airfoilpos[82]=0.5+radius*sin(deflection/57.0)/rratio;
        airfoilpos[79]=0.5+radius*sin(deflection/57.0)/lratio;
        airfoilpos[83]=chordratio+radius*cos(deflection/57.0);
        airfoilpos[80]=airfoilpos[83];
        airfoilpos[89]=airfoilpos[83];
        airfoilpos[88]=airfoilpos[82];
        airfoilpos[86]=airfoilpos[80];
        airfoilpos[85]=airfoilpos[79];
    }
}

Vector3 FlexAirfoil::updateVerticesGfx(RoR::GfxActor* gfx_actor)
{
    auto gfx_nodes = gfx_actor->GetSimNodeBuffer();
    int i;
    Vector3 center;
    center=gfx_nodes[nfld].AbsPosition;


    return center;
}

void FlexAirfoil::uploadVertices()
{

}


void FlexAirfoil::setControlDeflection(float val)
{
    if (val<0) deflection=-val*mindef;
    else deflection=val*maxdef;
}

void FlexAirfoil::enableInducedDrag(float span, float area, bool l)
{
    idSpan=span;
    idArea=area;
    useInducedDrag=true;
    idLeft=l;
}

void FlexAirfoil::addwash(int propid, float ratio)
{
    washpropnum[free_wash]=propid;
    washpropratio[free_wash]=ratio;
    free_wash++;
}

void FlexAirfoil::updateForces()
{
    if (!airfoil) return;
    if (broken) return;

    //evaluate wind direction
    Vector3 wind=-(nodes[nfld].Velocity+nodes[nfrd].Velocity)/2.0;
    //add wash
    int i;
    for (i=0; i<free_wash; i++)
        wind-=(0.5*washpropratio[i]*aeroengines[washpropnum[i]]->getpropwash())*aeroengines[washpropnum[i]]->getAxis();
    float wspeed=wind.length();
    //chord vector, front to back
    Vector3 chordv=((nodes[nbld].RelPosition-nodes[nfld].RelPosition)+(nodes[nbrd].RelPosition-nodes[nfrd].RelPosition))/2.0;
    float chord=chordv.length();
    //span vector, left to right
    Vector3 spanv=((nodes[nfrd].RelPosition-nodes[nfld].RelPosition)+(nodes[nbrd].RelPosition-nodes[nbld].RelPosition))/2.0;
    float span=spanv.length();
    //lift vector
    Vector3 liftv=spanv.crossProduct(-wind);

    //wing normal
    float s=span*chord;
    Vector3 normv=chordv.crossProduct(spanv);
    normv.normalise();
    //calculate angle of attack
    Vector3 pwind;
    pwind=Plane(Vector3::ZERO, normv, chordv).projectVector(-wind);
    Vector3 dumb;
    Degree daoa;
    chordv.getRotationTo(-pwind).ToAngleAxis(daoa, dumb);
    aoa=daoa.valueDegrees();
    float raoa=daoa.valueRadians();
    if (dumb.dotProduct(spanv)>0) {aoa=-aoa; raoa=-raoa;};

    //get airfoil data
    float cz, cx, cm;
    if (isstabilator)
        airfoil->getparams(aoa-deflection, chordratio, 0, &cz, &cx, &cm);
    else
        airfoil->getparams(aoa, chordratio, deflection, &cz, &cx, &cm);


    //tropospheric model valid up to 11.000m (33.000ft)
    float altitude=nodes[nfld].AbsPosition.y;
    float sea_level_pressure=101325; //in Pa
    float airpressure=sea_level_pressure*approx_pow(1.0-0.0065*altitude/288.15, 5.24947); //in Pa
    float airdensity=airpressure*0.0000120896;//1.225 at sea level

    Vector3 wforce=Vector3::ZERO;
    //drag
    wforce=(cx*0.5*airdensity*wspeed*s)*wind;

    //induced drag
    if (useInducedDrag)
    {
        Vector3 idf=(cx*cx*0.25*airdensity*wspeed*idArea*idArea/(3.14159*idSpan*idSpan))*wind;

        if (idLeft)
        {
            nodes[nblu].Forces+=idf;
            nodes[nbld].Forces+=idf;
        }
        else
        {
            nodes[nbru].Forces+=idf;
            nodes[nbrd].Forces+=idf;
        }
    }

    //lift
    wforce+=(cz*0.5*airdensity*wspeed*chord)*liftv;

    //moment
    float moment=-cm*0.5*airdensity*wspeed*wspeed*s;//*chord;
    //apply forces

    Vector3 f1=wforce*(liftcoef * 0.75/4.0f)+normv*(liftcoef *moment/(4.0f*0.25f));
    Vector3 f2=wforce*(liftcoef *0.25/4.0f)-normv*(liftcoef *moment/(4.0f*0.75f));

    //focal at 0.25 chord
    nodes[nfld].Forces+=f1;
    nodes[nflu].Forces+=f1;
    nodes[nfrd].Forces+=f1;
    nodes[nfru].Forces+=f1;
    nodes[nbld].Forces+=f2;
    nodes[nblu].Forces+=f2;
    nodes[nbrd].Forces+=f2;
    nodes[nbru].Forces+=f2;

}

FlexAirfoil::~FlexAirfoil()
{
    if (airfoil) delete airfoil;

}
