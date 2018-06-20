
// -------------------------------------------
// OGRE-IMGUI bindings
// See file 'README-OgreImGui.txt' for details
// -------------------------------------------

/*
    This source file is part of Rigs of Rods
    Copyright 2016-2017 Petr Ohlidal & contributors

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

#include "OgreImGui.h"

#include <imgui.h>
#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgreMeshManager.h>
#include <OgreSubMesh.h>
#include <OgreTexture.h>
#include <OgreTextureManager.h>
#include <OgreString.h>
#include <OgreStringConverter.h>
#include <OgreViewport.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreHighLevelGpuProgram.h>
#include <OgreUnifiedHighLevelGpuProgram.h>
#include <OgreRoot.h>
#include <OgreTechnique.h>
#include <OgreViewport.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTarget.h>

void OgreImGui::Init(Ogre::SceneManager* scenemgr)
{
    mSceneMgr = scenemgr;
    ImGuiIO& io = ImGui::GetIO();

    io.KeyMap[ImGuiKey_Tab] = OIS::KC_TAB;                       // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_LeftArrow] = OIS::KC_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = OIS::KC_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = OIS::KC_UP;
    io.KeyMap[ImGuiKey_DownArrow] = OIS::KC_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = OIS::KC_PGUP;
    io.KeyMap[ImGuiKey_PageDown] = OIS::KC_PGDOWN;
    io.KeyMap[ImGuiKey_Home] = OIS::KC_HOME;
    io.KeyMap[ImGuiKey_End] = OIS::KC_END;
    io.KeyMap[ImGuiKey_Delete] = OIS::KC_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = OIS::KC_BACK;
    io.KeyMap[ImGuiKey_Enter] = OIS::KC_RETURN;
    io.KeyMap[ImGuiKey_Escape] = OIS::KC_ESCAPE;
    io.KeyMap[ImGuiKey_A] = OIS::KC_A;
    io.KeyMap[ImGuiKey_C] = OIS::KC_C;
    io.KeyMap[ImGuiKey_V] = OIS::KC_V;
    io.KeyMap[ImGuiKey_X] = OIS::KC_X;
    io.KeyMap[ImGuiKey_Y] = OIS::KC_Y;
    io.KeyMap[ImGuiKey_Z] = OIS::KC_Z;

    createFontTexture();
    createMaterial();
}

//Inherhited from OIS::MouseListener
void OgreImGui::InjectMouseMoved( const OIS::MouseEvent &arg )
{

    ImGuiIO& io = ImGui::GetIO();

    io.MousePos.x = arg.state.X.abs;
    io.MousePos.y = arg.state.Y.abs;
}

void OgreImGui::InjectMousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    ImGuiIO& io = ImGui::GetIO();
    if (id<5)
    {
        io.MouseDown[id] = true;
    }
}

void OgreImGui::InjectMouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    ImGuiIO& io = ImGui::GetIO();
    if (id<5)
    {
        io.MouseDown[id] = false;
    }
}

// Inherhited from OIS::KeyListener
void OgreImGui::InjectKeyPressed( const OIS::KeyEvent &arg )
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[arg.key] = true;

    if (arg.text>0)
    {
        io.AddInputCharacter((unsigned short)arg.text);
    }
}

void OgreImGui::InjectKeyReleased( const OIS::KeyEvent &arg )
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[arg.key] = false;
}

void OgreImGui::Render()
{
    // Construct projection matrix, taking texel offset corrections in account (important for DirectX9)
    // See also:
    //     - OGRE-API specific hint: http://www.ogre3d.org/forums/viewtopic.php?f=5&p=536881#p536881
    //     - IMGUI Dx9 demo solution: https://github.com/ocornut/imgui/blob/master/examples/directx9_example/imgui_impl_dx9.cpp#L127-L138
    ImGuiIO& io = ImGui::GetIO();
    Ogre::RenderSystem* renderSys = Ogre::Root::getSingletonPtr()->getRenderSystem();
    const float texelOffsetX = renderSys->getHorizontalTexelOffset();
    const float texelOffsetY = renderSys->getVerticalTexelOffset();
    const float L = texelOffsetX;
    const float R = io.DisplaySize.x + texelOffsetX;
    const float T = texelOffsetY;
    const float B = io.DisplaySize.y + texelOffsetY;

    Ogre::Matrix4 projMatrix(       2.0f/(R-L),    0.0f,         0.0f,       (L+R)/(L-R),
                                    0.0f,         -2.0f/(B-T),   0.0f,       (T+B)/(B-T),
                                    0.0f,          0.0f,        -1.0f,       0.0f,
                                    0.0f,          0.0f,         0.0f,       1.0f);

    mPass->getVertexProgramParameters()->setNamedConstant("ProjectionMatrix", projMatrix);

    // Instruct ImGui to Render() and process the resulting CmdList-s
    /// Adopted from https://bitbucket.org/ChaosCreator/imgui-ogre2.1-binding
    /// ... Commentary on OGRE forums: http://www.ogre3d.org/forums/viewtopic.php?f=5&t=89081#p531059
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    int vpWidth = 2000; // Dummy viewport size
    int vpHeight = 2000;
    Ogre::Viewport* vp = renderSys->_getViewport();
    if (vp != nullptr) // Viewport is sometimes 'null' on Windows/Dx9 right after game startup or map load - probably related to switching Ogre::Camera-s ~ only_a_ptr, 01/2018
    {
        vpWidth  = vp->getActualWidth();
        vpHeight = vp->getActualHeight();
    }
    for (int i = 0; i < draw_data->CmdListsCount; ++i)
    {
        const ImDrawList* draw_list = draw_data->CmdLists[i];
        unsigned int startIdx = 0;

        for (int j = 0; j < draw_list->CmdBuffer.Size; ++j)
        {
            // Create a renderable and fill it's buffers
            ImGUIRenderable renderable;
            const ImDrawCmd *drawCmd = &draw_list->CmdBuffer[j];
            renderable.updateVertexData(draw_list->VtxBuffer.Data, &draw_list->IdxBuffer.Data[startIdx], draw_list->VtxBuffer.Size, drawCmd->ElemCount);

            // Set scissoring
            int scLeft   = static_cast<int>(drawCmd->ClipRect.x); // Obtain bounds
            int scTop    = static_cast<int>(drawCmd->ClipRect.y);
            int scRight  = static_cast<int>(drawCmd->ClipRect.z);
            int scBottom = static_cast<int>(drawCmd->ClipRect.w);

            scLeft   = scLeft   < 0 ? 0 : (scLeft  > vpWidth ? vpWidth : scLeft); // Clamp bounds to viewport dimensions
            scRight  = scRight  < 0 ? 0 : (scRight > vpWidth ? vpWidth : scRight);
            scTop    = scTop    < 0 ? 0 : (scTop    > vpHeight ? vpHeight : scTop);
            scBottom = scBottom < 0 ? 0 : (scBottom > vpHeight ? vpHeight : scBottom);

            renderSys->setScissorTest(true, scLeft, scTop, scRight, scBottom);

            // Render!
            mSceneMgr->_injectRenderWithPass(mPass, &renderable, false, false, nullptr);

            // Update counts
            startIdx += drawCmd->ElemCount;
        }
    }
    renderSys->setScissorTest(false);
}

void OgreImGui::createMaterial()
{
    static const char* vertexShaderSrcD3D11 =
    {
    "cbuffer vertexBuffer : register(b0) \n"
    "{\n"
    "float4x4 ProjectionMatrix; \n"
    "};\n"
    "struct VS_INPUT\n"
    "{\n"
    "float2 pos : POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "struct PS_INPUT\n"
    "{\n"
    "float4 pos : SV_POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "PS_INPUT main(VS_INPUT input)\n"
    "{\n"
    "PS_INPUT output;\n"
    "output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
    "output.col = input.col;\n"
    "output.uv  = input.uv;\n"
    "return output;\n"
    "}"
    };

    static const char* pixelShaderSrcD3D11 =
    {
    "struct PS_INPUT\n"
    "{\n"
    "float4 pos : SV_POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "sampler sampler0;\n"
    "Texture2D texture0;\n"
    "\n"
    "float4 main(PS_INPUT input) : SV_Target\n"
    "{\n"
    "float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \n"
    "return out_col; \n"
    "}"
    };

    static const char* vertexShaderSrcD3D9 =
    {
    "uniform float4x4 ProjectionMatrix; \n"
    "struct VS_INPUT\n"
    "{\n"
    "float2 pos : POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "struct PS_INPUT\n"
    "{\n"
    "float4 pos : POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "PS_INPUT main(VS_INPUT input)\n"
    "{\n"
    "PS_INPUT output;\n"
    "output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
    "output.col = input.col;\n"
    "output.uv  = input.uv;\n"
    "return output;\n"
    "}"
    };

    static const char* pixelShaderSrcSrcD3D9 =
     {
    "struct PS_INPUT\n"
    "{\n"
    "float4 pos : SV_POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "sampler2D sampler0;\n"
    "\n"
    "float4 main(PS_INPUT input) : SV_Target\n"
    "{\n"
    "float4 out_col = input.col.bgra * tex2D(sampler0, input.uv); \n"
    "return out_col; \n"
    "}"
    };

    static const char* vertexShaderSrcGLSL =
    { // See https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#OpenGL_and_GLSL_versions
    "#version 130\n"
    "uniform mat4 ProjectionMatrix; \n"
    "in vec2 vertex;\n"
    "in vec2 uv0;\n"
    "in vec4 colour;\n"
    "out vec2 Texcoord;\n"
    "out vec4 col;\n"
    "void main()\n"
    "{\n"
    "gl_Position = ProjectionMatrix* vec4(vertex.xy, 0.f, 1.f);\n"
    "Texcoord  = uv0;\n"
    "col = colour;\n"
    "}"
    };
    
    static const char* pixelShaderSrcGLSL =
    { // See https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#OpenGL_and_GLSL_versions
    "#version 130\n"
    "in vec2 Texcoord;\n"
    "in vec4 col;\n"
    "uniform sampler2D sampler0;\n"
    "out vec4 out_col;\n"
    "void main()\n"
    "{\n"
    "out_col = col * texture(sampler0, Texcoord); \n"
    "}"
    };

    //create the default shadows material
    Ogre::HighLevelGpuProgramManager& mgr = Ogre::HighLevelGpuProgramManager::getSingleton();

    Ogre::HighLevelGpuProgramPtr vertexShaderUnified = mgr.getByName("imgui/VP");
    Ogre::HighLevelGpuProgramPtr pixelShaderUnified = mgr.getByName("imgui/FP");
    
    Ogre::HighLevelGpuProgramPtr vertexShaderD3D11 = mgr.getByName("imgui/VP/D3D11");
    Ogre::HighLevelGpuProgramPtr pixelShaderD3D11 = mgr.getByName("imgui/FP/D3D11");

    Ogre::HighLevelGpuProgramPtr vertexShaderD3D9 = mgr.getByName("imgui/VP/D3D9");
    Ogre::HighLevelGpuProgramPtr pixelShaderD3D9 = mgr.getByName("imgui/FP/D3D9");

    Ogre::HighLevelGpuProgramPtr vertexShaderGL = mgr.getByName("imgui/VP/GL130");
    Ogre::HighLevelGpuProgramPtr pixelShaderGL = mgr.getByName("imgui/FP/GL130");

    if (vertexShaderUnified.isNull())
    {
        vertexShaderUnified = mgr.createProgram("imgui/VP",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,"unified",Ogre::GPT_VERTEX_PROGRAM);
    }
    if (pixelShaderUnified.isNull())
    {
        pixelShaderUnified = mgr.createProgram("imgui/FP",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,"unified",Ogre::GPT_FRAGMENT_PROGRAM);
    }

    Ogre::UnifiedHighLevelGpuProgram* vertexShaderPtr = static_cast<Ogre::UnifiedHighLevelGpuProgram*>(vertexShaderUnified.get());
    Ogre::UnifiedHighLevelGpuProgram* pixelShaderPtr = static_cast<Ogre::UnifiedHighLevelGpuProgram*>(pixelShaderUnified.get());

    if (vertexShaderD3D11.isNull())
    {
        vertexShaderD3D11 = mgr.createProgram("imgui/VP/D3D11", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                "hlsl", Ogre::GPT_VERTEX_PROGRAM);
        vertexShaderD3D11->setParameter("target", "vs_4_0");
        vertexShaderD3D11->setParameter("entry_point", "main");
        vertexShaderD3D11->setSource(vertexShaderSrcD3D11);
        vertexShaderD3D11->load();

        vertexShaderPtr->addDelegateProgram(vertexShaderD3D11->getName());
    }

    if (pixelShaderD3D11.isNull())
    {
        pixelShaderD3D11 = mgr.createProgram("imgui/FP/D3D11", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                "hlsl", Ogre::GPT_FRAGMENT_PROGRAM);
        pixelShaderD3D11->setParameter("target", "ps_4_0");
        pixelShaderD3D11->setParameter("entry_point", "main");
        pixelShaderD3D11->setSource(pixelShaderSrcD3D11);
        pixelShaderD3D11->load();

        pixelShaderPtr->addDelegateProgram(pixelShaderD3D11->getName());
    }

    if (vertexShaderD3D9.isNull())
    {
        vertexShaderD3D9 = mgr.createProgram("imgui/VP/D3D9", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "hlsl", Ogre::GPT_VERTEX_PROGRAM);
        vertexShaderD3D9->setParameter("target", "vs_2_0");
        vertexShaderD3D9->setParameter("entry_point", "main");
        vertexShaderD3D9->setSource(vertexShaderSrcD3D9);
        vertexShaderD3D9->load();

        vertexShaderPtr->addDelegateProgram(vertexShaderD3D9->getName());
    }

    if (pixelShaderD3D9.isNull())
    {
        pixelShaderD3D9 = mgr.createProgram("imgui/FP/D3D9", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "hlsl", Ogre::GPT_FRAGMENT_PROGRAM);
        pixelShaderD3D9->setParameter("target", "ps_2_0");
        pixelShaderD3D9->setParameter("entry_point", "main");
        pixelShaderD3D9->setSource(pixelShaderSrcSrcD3D9);
        pixelShaderD3D9->load();

        pixelShaderPtr->addDelegateProgram(pixelShaderD3D9->getName());
    }

    if (vertexShaderGL.isNull())
    {
        vertexShaderGL = mgr.createProgram("imgui/VP/GL130", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                "glsl", Ogre::GPT_VERTEX_PROGRAM);
        vertexShaderGL->setSource(vertexShaderSrcGLSL);
        vertexShaderGL->load();
        vertexShaderPtr->addDelegateProgram(vertexShaderGL->getName());
    }

    if (pixelShaderGL.isNull())
    {
        pixelShaderGL = mgr.createProgram("imgui/FP/GL130", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                "glsl", Ogre::GPT_FRAGMENT_PROGRAM);
        pixelShaderGL->setSource(pixelShaderSrcGLSL);
        pixelShaderGL->load();
        pixelShaderGL->setParameter("sampler0","int 0");

        pixelShaderPtr->addDelegateProgram(pixelShaderGL->getName());
    }

    Ogre::MaterialPtr imguiMaterial = Ogre::MaterialManager::getSingleton().create("imgui/material", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mPass = imguiMaterial->getTechnique(0)->getPass(0);
    mPass->setFragmentProgram("imgui/FP");
    mPass->setVertexProgram("imgui/VP");
    mPass->setCullingMode(Ogre::CULL_NONE);
    mPass->setDepthFunction(Ogre::CMPF_ALWAYS_PASS);
    mPass->setLightingEnabled(false);
    mPass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
    mPass->setSeparateSceneBlendingOperation(Ogre::SBO_ADD,Ogre::SBO_ADD);
    mPass->setSeparateSceneBlending(Ogre::SBF_SOURCE_ALPHA,Ogre::SBF_ONE_MINUS_SOURCE_ALPHA,Ogre::SBF_ONE_MINUS_SOURCE_ALPHA,Ogre::SBF_ZERO);

    mPass->createTextureUnitState()->setTextureName("ImguiFontTex");
}

void OgreImGui::createFontTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    mFontTex = Ogre::TextureManager::getSingleton().createManual("ImguiFontTex",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,Ogre::TEX_TYPE_2D,width,height,1,1,Ogre::PF_R8G8B8A8);

    // Lock texture for writing
    const Ogre::PixelBox & lockBox = mFontTex->getBuffer()->lock(Ogre::Box(0, 0, width, height), Ogre::HardwareBuffer::HBL_DISCARD);

    // Copy texture to ImGui
    size_t texDepth = Ogre::PixelUtil::getNumElemBytes(lockBox.format);
    memcpy(lockBox.data,pixels, width*height*texDepth);

    // Unlock
    mFontTex->getBuffer()->unlock();
}

void OgreImGui::NewFrame(float deltaTime, float displayWidth, float displayHeight, bool ctrl, bool alt, bool shift)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = deltaTime;

     // Read keyboard modifiers inputs
    io.KeyCtrl = ctrl;
    io.KeyShift = shift;
    io.KeyAlt = alt;
    io.KeySuper = false;

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2(displayWidth, displayHeight);

    // Start the frame
    ImGui::NewFrame();
}

// -------------------------- ImGui Renderable ------------------------------ // 

 OgreImGui::ImGUIRenderable::ImGUIRenderable():
    mVertexBufferSize(5000),
    mIndexBufferSize(10000)
{
    this->initImGUIRenderable();

    //By default we want ImGUIRenderables to still work in wireframe mode
    this->setPolygonModeOverrideable( false );
}

void OgreImGui::ImGUIRenderable::initImGUIRenderable(void)
{
    // use identity projection and view matrices
    mUseIdentityProjection  = true;
    mUseIdentityView        = true;

    mRenderOp.vertexData = OGRE_NEW Ogre::VertexData();
    mRenderOp.indexData  = OGRE_NEW Ogre::IndexData();

    mRenderOp.vertexData->vertexCount = 0;
    mRenderOp.vertexData->vertexStart = 0;

    mRenderOp.indexData->indexCount = 0;
    mRenderOp.indexData->indexStart = 0;
    mRenderOp.operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
    mRenderOp.useIndexes  = true; 
    mRenderOp.useGlobalInstancingVertexBufferIsAvailable = false;

    Ogre::VertexDeclaration* decl = mRenderOp.vertexData->vertexDeclaration;
        
    // vertex declaration
    size_t offset = 0;
    decl->addElement(0,offset,Ogre::VET_FLOAT2,Ogre::VES_POSITION);
    offset += Ogre::VertexElement::getTypeSize( Ogre::VET_FLOAT2 );
    decl->addElement(0,offset,Ogre::VET_FLOAT2,Ogre::VES_TEXTURE_COORDINATES,0);
    offset += Ogre::VertexElement::getTypeSize( Ogre::VET_FLOAT2 );
    decl->addElement(0,offset,Ogre::VET_COLOUR,Ogre::VES_DIFFUSE);

        
        // set basic white material
    this->setMaterial( "imgui/material" );
}

OgreImGui::ImGUIRenderable::~ImGUIRenderable()
{
    OGRE_DELETE mRenderOp.vertexData;
    OGRE_DELETE mRenderOp.indexData;
    mMaterial.setNull();
}

void OgreImGui::ImGUIRenderable::setMaterial( const Ogre::String& matName )
{
    mMaterial = Ogre::MaterialManager::getSingleton().getByName( matName );
    if ( !mMaterial.isNull() )
    {
        return;
    }
    
    // Won't load twice anyway
    mMaterial->load();
}

void OgreImGui::ImGUIRenderable::setMaterial(const Ogre::MaterialPtr & material)
{
    mMaterial = material;
}

const Ogre::MaterialPtr& OgreImGui::ImGUIRenderable::getMaterial(void) const
{
    return mMaterial;
}

/// @author https://bitbucket.org/ChaosCreator/imgui-ogre2.1-binding/src/8f1a01db510f543a987c3c16859d0a33400d9097/ImguiRenderable.cpp?at=master&fileviewer=file-view-default
/// Commentary on OGRE forums: http://www.ogre3d.org/forums/viewtopic.php?f=5&t=89081#p531059
void OgreImGui::ImGUIRenderable::updateVertexData(const ImDrawVert* vtxBuf, const ImDrawIdx* idxBuf, unsigned int vtxCount, unsigned int idxCount)
{
    Ogre::VertexBufferBinding* bind = mRenderOp.vertexData->vertexBufferBinding;

    if (bind->getBindings().empty() || mVertexBufferSize != vtxCount)
    {
        mVertexBufferSize = vtxCount;

        bind->setBinding(0, Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(sizeof(ImDrawVert), mVertexBufferSize, Ogre::HardwareBuffer::HBU_WRITE_ONLY));
    }
    if (mRenderOp.indexData->indexBuffer.isNull() || mIndexBufferSize != idxCount)
    {
        mIndexBufferSize = idxCount;

        mRenderOp.indexData->indexBuffer =
            Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, mIndexBufferSize, Ogre::HardwareBuffer::HBU_WRITE_ONLY);
    }

    // Copy all vertices
    ImDrawVert* vtxDst = (ImDrawVert*)(bind->getBuffer(0)->lock(Ogre::HardwareBuffer::HBL_DISCARD));
    ImDrawIdx* idxDst = (ImDrawIdx*)(mRenderOp.indexData->indexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));

    memcpy(vtxDst, vtxBuf, mVertexBufferSize * sizeof(ImDrawVert));
    memcpy(idxDst, idxBuf, mIndexBufferSize * sizeof(ImDrawIdx));

    mRenderOp.vertexData->vertexStart = 0;
    mRenderOp.vertexData->vertexCount = vtxCount;
    mRenderOp.indexData->indexStart = 0;
    mRenderOp.indexData->indexCount = idxCount;


    bind->getBuffer(0)->unlock();
    mRenderOp.indexData->indexBuffer->unlock();
}

void OgreImGui::ImGUIRenderable::getWorldTransforms( Ogre::Matrix4* xform ) const
{
    *xform = Ogre::Matrix4::IDENTITY;
}

void OgreImGui::ImGUIRenderable::getRenderOperation(Ogre::RenderOperation& op)
{
    op = mRenderOp;
}

const Ogre::LightList& OgreImGui::ImGUIRenderable::getLights(void) const
{
    static const Ogre::LightList light_list;
    return light_list;
}
