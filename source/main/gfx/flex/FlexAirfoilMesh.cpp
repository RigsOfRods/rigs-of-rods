#include "FlexAirfoilMesh.h"

#include "Actor.h"
#include "FlexAirfoil.h"
#include "GfxActor.h"

using namespace RoR;
using namespace Ogre;

FlexAirfoilMesh::FlexAirfoilMesh(ActorPtr& actor, WingID_t wingid,
        std::string const & texband,
        Ogre::Vector2 texlf, Ogre::Vector2 texrf, Ogre::Vector2 texlb, Ogre::Vector2 texrb)
    :m_actor(actor)
    ,m_wingid(wingid)
{
    FlexAirfoil* flex_af = actor->ar_wings[wingid].fa;

    /// Create the mesh via the MeshManager
    msh = MeshManager::getSingleton().createManual(flex_af->fa_name, actor->GetGfxActor()->GetResourceGroup());

    /// Create submeshes
    subface = msh->createSubMesh();
    subband = msh->createSubMesh();
    subcup = msh->createSubMesh();
    subcdn = msh->createSubMesh();

    //materials
    subface->setMaterialName(texband);
    subband->setMaterialName(texband);
    subcup->setMaterialName(texband);
    subcdn->setMaterialName(texband);

    /// Define the vertices
    nVertices = 24*2+4+2;
    vbufCount = (2*3+2)*nVertices;
    vertices=(float*)malloc(vbufCount*sizeof(float));

    //textures coordinates
    covertices[0].texcoord=texlf;
    covertices[1].texcoord=texrf;

    covertices[2].texcoord=texlf+(texlb-texlf)*0.03;
    covertices[3].texcoord=texrf+(texrb-texrf)*0.03;
    covertices[4].texcoord=texlf+(texlb-texlf)*0.03;
    covertices[5].texcoord=texrf+(texrb-texrf)*0.03;

    covertices[6].texcoord=texlf+(texlb-texlf)*0.10;
    covertices[7].texcoord=texrf+(texrb-texrf)*0.10;
    covertices[8].texcoord=texlf+(texlb-texlf)*0.10;
    covertices[9].texcoord=texrf+(texrb-texrf)*0.10;

    covertices[10].texcoord=texlf+(texlb-texlf)*0.25;
    covertices[11].texcoord=texrf+(texrb-texrf)*0.25;
    covertices[12].texcoord=texlf+(texlb-texlf)*0.25;
    covertices[13].texcoord=texrf+(texrb-texrf)*0.25;

    covertices[14].texcoord=texlf+(texlb-texlf)*0.45;
    covertices[15].texcoord=texrf+(texrb-texrf)*0.45;
    covertices[16].texcoord=texlf+(texlb-texlf)*0.45;
    covertices[17].texcoord=texrf+(texrb-texrf)*0.45;

    covertices[18].texcoord=texlf+(texlb-texlf)*flex_af->airfoilpos[56];
    covertices[19].texcoord=texrf+(texrb-texrf)*flex_af->airfoilpos[56];
    covertices[20].texcoord=texlf+(texlb-texlf)*flex_af->airfoilpos[56];
    covertices[21].texcoord=texrf+(texrb-texrf)*flex_af->airfoilpos[56];

    covertices[22].texcoord=covertices[18].texcoord;
    covertices[23].texcoord=covertices[19].texcoord;
    covertices[24].texcoord=covertices[20].texcoord;
    covertices[25].texcoord=covertices[21].texcoord;

    covertices[26].texcoord=texlb;
    covertices[27].texcoord=texrb;
    covertices[28].texcoord=texlb;
    covertices[29].texcoord=texrb;

    for (int i=0; i<24; i++) covertices[i+30].texcoord=covertices[i].texcoord;

    /// Define triangles
    /// The values in this table refer to vertices in the above table
    bandibufCount = 3*20;
    faceibufCount = 3*20;
    cupibufCount=3*2;
    cdnibufCount=3*2;
    facefaces=(unsigned short*)malloc(faceibufCount*sizeof(unsigned short));
    bandfaces=(unsigned short*)malloc(bandibufCount*sizeof(unsigned short));
    cupfaces=(unsigned short*)malloc(cupibufCount*sizeof(unsigned short));
    cdnfaces=(unsigned short*)malloc(cdnibufCount*sizeof(unsigned short));
    
    //attack
    bandfaces[0]=0;
    bandfaces[1]=2;
    bandfaces[2]=1;

    bandfaces[3]=2;
    bandfaces[4]=3;
    bandfaces[5]=1;

    bandfaces[6]=0;
    bandfaces[7]=1;
    bandfaces[8]=4;

    bandfaces[9]=4;
    bandfaces[10]=1;
    bandfaces[11]=5;
    for (int i=0; i<5; i++)
    {
        //band
        int v=i*4+2;
        if (i!=4)
        {
            bandfaces[i*12+12]=v;
            bandfaces[i*12+13]=v+4;
            bandfaces[i*12+14]=v+1;

            bandfaces[i*12+15]=v+4;
            bandfaces[i*12+16]=v+5;
            bandfaces[i*12+17]=v+1;

            bandfaces[i*12+18]=v+2;
            bandfaces[i*12+19]=v+3;
            bandfaces[i*12+20]=v+6;

            bandfaces[i*12+21]=v+6;
            bandfaces[i*12+22]=v+3;
            bandfaces[i*12+23]=v+7;
        }

        //sides
        facefaces[i*12]=30+0;
        facefaces[i*12+1]=30+v+4;
        facefaces[i*12+2]=30+v;

        facefaces[i*12+3]=30+0;
        facefaces[i*12+4]=30+v+2;
        facefaces[i*12+5]=30+v+6;

        facefaces[i*12+6]=30+1;
        facefaces[i*12+7]=30+v+1;
        facefaces[i*12+8]=30+v+5;

        facefaces[i*12+9]=30+1;
        facefaces[i*12+10]=30+v+7;
        facefaces[i*12+11]=30+v+3;
        if (i==4)
        {
            facefaces[i*12]=30+0;
            facefaces[i*12+1]=30+v+2;
            facefaces[i*12+2]=30+v;

            facefaces[i*12+3]=30+v+4;
            facefaces[i*12+4]=30+v;
            facefaces[i*12+5]=30+v+2;

            facefaces[i*12+6]=30+1;
            facefaces[i*12+7]=30+v+1;
            facefaces[i*12+8]=30+v+3;

            facefaces[i*12+9]=30+v+5;
            facefaces[i*12+10]=30+v+3;
            facefaces[i*12+11]=30+v+1;
        }

    }
    cupfaces[0]=22;
    cupfaces[1]=26;
    cupfaces[2]=23;
    cupfaces[3]=26;
    cupfaces[4]=27;
    cupfaces[5]=23;

    cdnfaces[0]=24;
    cdnfaces[1]=25;
    cdnfaces[2]=29;
    cdnfaces[3]=24;
    cdnfaces[4]=29;
    cdnfaces[5]=28;
    

    this->updateVerticesGfx();

    /// Create vertex data structure for 8 vertices shared between submeshes
    msh->sharedVertexData = new VertexData();
    msh->sharedVertexData->vertexCount = nVertices;

    /// Create declaration (memory format) of vertex data
    decl = msh->sharedVertexData->vertexDeclaration;
    size_t offset = 0;
    decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
//        decl->addElement(0, offset, VET_FLOAT3, VES_DIFFUSE);
//        offset += VertexElement::getTypeSize(VET_FLOAT3);
    decl->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
    offset += VertexElement::getTypeSize(VET_FLOAT2);

    /// Allocate vertex buffer of the requested number of vertices (vertexCount)
    /// and bytes per vertex (offset)
    vbuf =
      HardwareBufferManager::getSingleton().createVertexBuffer(
          offset, msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

    /// Upload the vertex data to the card
    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

    /// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
    VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding;
    bind->setBinding(0, vbuf);

    //for the face
    /// Allocate index buffer of the requested number of vertices (ibufCount)
    HardwareIndexBufferSharedPtr faceibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT,
            faceibufCount,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    faceibuf->writeData(0, faceibuf->getSizeInBytes(), facefaces, true);

    /// Set parameters of the submesh
    subface->useSharedVertices = true;
    subface->indexData->indexBuffer = faceibuf;
    subface->indexData->indexCount = faceibufCount;
    subface->indexData->indexStart = 0;

    //for the band
    /// Allocate index buffer of the requested number of vertices (ibufCount)
    HardwareIndexBufferSharedPtr bandibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT,
            bandibufCount,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    bandibuf->writeData(0, bandibuf->getSizeInBytes(), bandfaces, true);

    /// Set parameters of the submesh
    subband->useSharedVertices = true;
    subband->indexData->indexBuffer = bandibuf;
    subband->indexData->indexCount = bandibufCount;
    subband->indexData->indexStart = 0;

    //for the aileron up
    /// Allocate index buffer of the requested number of vertices (ibufCount)
    HardwareIndexBufferSharedPtr cupibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT,
            cupibufCount,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    cupibuf->writeData(0, cupibuf->getSizeInBytes(), cupfaces, true);

    /// Set parameters of the submesh
    subcup->useSharedVertices = true;
    subcup->indexData->indexBuffer = cupibuf;
    subcup->indexData->indexCount = cupibufCount;
    subcup->indexData->indexStart = 0;

    //for the aileron down
    /// Allocate index buffer of the requested number of vertices (ibufCount)
    HardwareIndexBufferSharedPtr cdnibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT,
            cdnibufCount,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    cdnibuf->writeData(0, cdnibuf->getSizeInBytes(), cdnfaces, true);

    /// Set parameters of the submesh
    subcdn->useSharedVertices = true;
    subcdn->indexData->indexBuffer = cdnibuf;
    subcdn->indexData->indexCount = cdnibufCount;
    subcdn->indexData->indexStart = 0;

    /// Set bounding information (for culling)
    msh->_setBounds(AxisAlignedBox(-20,-20,-20,20,20,20), true);
    //msh->_setBoundingSphereRadius(20.0);

    /// Notify Mesh object that it has been loaded
    //MeshManager::getSingleton().setPrepareAllMeshesForShadowVolumes(false);
    //msh->prepareForShadowVolume();
    msh->load();
    //MeshManager::getSingleton().setPrepareAllMeshesForShadowVolumes()
}    


Vector3 FlexAirfoilMesh::updateVerticesGfx()
{
    RoR::GfxActor* gfx_actor = m_actor->GetGfxActor();
    WingSB& wingbuf = gfx_actor->GetSimDataBuffer().simbuf_wings[m_wingid];
    FlexAirfoil* flex_airfoil = m_actor->ar_wings[m_wingid].fa;

    auto gfx_nodes = gfx_actor->GetSimNodeBuffer();
    const NodeNum_t nfld = flex_airfoil->nfld;
    const NodeNum_t nfrd = flex_airfoil->nfrd;
    const NodeNum_t nflu = flex_airfoil->nflu;
    const NodeNum_t nfru = flex_airfoil->nfru;
    const NodeNum_t nbld = flex_airfoil->nbld;
    const NodeNum_t nbrd = flex_airfoil->nbrd;
    const NodeNum_t nblu = flex_airfoil->nblu;
    const NodeNum_t nbru = flex_airfoil->nbru;

    int i;
    Vector3 center;
    center=gfx_nodes[nfld].AbsPosition;

    Vector3 vx=gfx_nodes[nfrd].AbsPosition-gfx_nodes[nfld].AbsPosition;
    Vector3 vyl=gfx_nodes[nflu].AbsPosition-gfx_nodes[nfld].AbsPosition;
    Vector3 vzl=gfx_nodes[nbld].AbsPosition-gfx_nodes[nfld].AbsPosition;
    Vector3 vyr=gfx_nodes[nfru].AbsPosition-gfx_nodes[nfrd].AbsPosition;
    Vector3 vzr=gfx_nodes[nbrd].AbsPosition-gfx_nodes[nfrd].AbsPosition;

    Vector3 facenormal=vx;
    facenormal.normalise();

    const bool broken = wingbuf.simbuf_fa_broken;
    const float* airfoilpos = wingbuf.simbuf_fa_airfoilpos;
    const bool isstabilator = wingbuf.simbuf_fa_isstabilator;
    const bool stabilleft = wingbuf.simbuf_fa_stabilleft;
    const float deflection = wingbuf.simbuf_fa_deflection;

    if (!broken)
    {
        for (i=0; i<30; i++)
        {
            if (i%2)
                covertices[i].vertex=airfoilpos[i*3]*vx+airfoilpos[i*3+1]*vyr+airfoilpos[i*3+2]*vzr;
            else
                covertices[i].vertex=airfoilpos[i*3]*vx+airfoilpos[i*3+1]*vyl+airfoilpos[i*3+2]*vzl;
            if (i<22) covertices[i+30].vertex=covertices[i].vertex;
        }
        covertices[30+22].vertex=covertices[28].vertex;
        covertices[30+23].vertex=covertices[29].vertex;
    }
    else
    {
        for (i=0; i<30; i++)
        {
            if (i%2)
                covertices[i].vertex=airfoilpos[i*3]*Vector3(0.01,0,0)+airfoilpos[i*3+1]*Vector3(0,0.01,0)+airfoilpos[i*3+2]*Vector3(0,0,0.01);
            else
                covertices[i].vertex=airfoilpos[i*3]*Vector3(0.01,0,0)+airfoilpos[i*3+1]*Vector3(0,0.01,0)+airfoilpos[i*3+2]*Vector3(0,0,0.01);
            if (i<22) covertices[i+30].vertex=covertices[i].vertex;
        }
        covertices[30+22].vertex=covertices[28].vertex;
        covertices[30+23].vertex=covertices[29].vertex;
    }

    if (isstabilator)
    {
        //rotate stabilator
        Vector3 rcent, raxis;
        if (!stabilleft)
        {
            rcent=((gfx_nodes[nflu].AbsPosition+gfx_nodes[nbld].AbsPosition)/2.0+(gfx_nodes[nflu].AbsPosition-gfx_nodes[nblu].AbsPosition)/4.0)-center;
            raxis=(gfx_nodes[nflu].AbsPosition-gfx_nodes[nfld].AbsPosition).crossProduct(gfx_nodes[nflu].AbsPosition-gfx_nodes[nblu].AbsPosition);
        }
        else
        {
            rcent=((gfx_nodes[nfru].AbsPosition+gfx_nodes[nbrd].AbsPosition)/2.0+(gfx_nodes[nfru].AbsPosition-gfx_nodes[nbru].AbsPosition)/4.0)-center;
            raxis=(gfx_nodes[nfru].AbsPosition-gfx_nodes[nfrd].AbsPosition).crossProduct(gfx_nodes[nfru].AbsPosition-gfx_nodes[nbru].AbsPosition);
        }
        raxis.normalise();
        Quaternion rot=Quaternion(Degree(deflection), raxis);
        for (i=0; i<54; i++)
        {
            covertices[i].vertex=rcent+rot*(covertices[i].vertex-rcent);
        }
    }

    //init normals
    for (i=0; i<(int)nVertices; i++)
    {
        covertices[i].normal=Vector3::ZERO;
    }
    //normals
    //accumulate normals per triangle
    for (i=0; i<(int)bandibufCount/3; i++)
    {
        Vector3 v1, v2;
        v1=covertices[bandfaces[i*3+1]].vertex-covertices[bandfaces[i*3]].vertex;
        v2=covertices[bandfaces[i*3+2]].vertex-covertices[bandfaces[i*3]].vertex;
        v1=v1.crossProduct(v2);
        v1.normalise();
        covertices[bandfaces[i*3]].normal+=v1;
        covertices[bandfaces[i*3+1]].normal+=v1;
        covertices[bandfaces[i*3+2]].normal+=v1;
    }
    for (i=0; i<(int)cupibufCount/3; i++)
    {
        Vector3 v1, v2;
        v1=covertices[cupfaces[i*3+1]].vertex-covertices[cupfaces[i*3]].vertex;
        v2=covertices[cupfaces[i*3+2]].vertex-covertices[cupfaces[i*3]].vertex;
        v1=v1.crossProduct(v2);
        v1.normalise();
        covertices[cupfaces[i*3]].normal+=v1;
        covertices[cupfaces[i*3+1]].normal+=v1;
        covertices[cupfaces[i*3+2]].normal+=v1;
    }
    for (i=0; i<(int)cdnibufCount/3; i++)
    {
        Vector3 v1, v2;
        v1=covertices[cdnfaces[i*3+1]].vertex-covertices[cdnfaces[i*3]].vertex;
        v2=covertices[cdnfaces[i*3+2]].vertex-covertices[cdnfaces[i*3]].vertex;
        v1=v1.crossProduct(v2);
        v1.normalise();
        covertices[cdnfaces[i*3]].normal+=v1;
        covertices[cdnfaces[i*3+1]].normal+=v1;
        covertices[cdnfaces[i*3+2]].normal+=v1;
    }
    //normalize
    for (i=0; i<30; i++)
    {
        covertices[i].normal.normalise();
    }

    //for the faces
    for (i=0; i<24; i++)
        if (i%2)
            covertices[i+30].normal=facenormal;
        else
            covertices[i+30].normal=-facenormal;

    // Upload to graphics card
    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

    return center;
}

FlexAirfoilMesh::~FlexAirfoilMesh()
{
    if (msh)
    {
        msh->unload();
        Ogre::MeshManager::getSingleton().remove(msh->getHandle()); // Necessary to truly erase manually created resource.
        msh.reset(); // Important! It's a shared pointer.
    }

    if (vertices          != nullptr) { free (vertices); }
    if (facefaces         != nullptr) { free (facefaces); }
    if (bandfaces         != nullptr) { free (bandfaces); }
    if (cupfaces          != nullptr) { free (cupfaces); }
    if (cdnfaces          != nullptr) { free (cdnfaces); }
}