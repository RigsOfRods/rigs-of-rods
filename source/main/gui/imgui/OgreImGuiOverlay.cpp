// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.


#include "Application.h"
#include "GfxScene.h"
#include "OgreImGuiOverlay.h"


#include <Ogre.h>
#include <OgreFontManager.h>
#include <OgreOverlayManager.h>
#include <OgrePixelFormatGpuUtils.h>
#include <OgreStagingTexture.h>
#include <OgreTextureGpuManager.h>
#include <OgreUnifiedHighLevelGpuProgram.h>

#include <imgui.h>

namespace Ogre
{

ImGuiOverlay::ImGuiOverlay(Ogre::ObjectMemoryManager* memory_manager)
    : Overlay( "ImGuiOverlay", Id::generateNewId<Overlay>(),
               memory_manager, /*Ogre::RENDER_QUEUE_OVERLAY (100) from OGRE1x*/100 )
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.BackendPlatformName = "OGRE";
}
ImGuiOverlay::~ImGuiOverlay()
{
    ImGui::DestroyContext();
}

void ImGuiOverlay::initialise()
{
    if (!mInitialised)
    {
        mRenderable.initialise();
        mCodePointRanges.clear();
    }
    mInitialised = true;
}


//-----------------------------------------------------------------------------------
// From: https://gitlab.com/edherbert/southsea/-/blob/master/src/Gui/ImguiOgre/ImguiManager.cpp#L278
void ImGuiOverlay::ImGUIRenderable::createMaterial()
{
	static const char* vertexShaderSrcD3D11 =
	{
		"uniform float4x4 ProjectionMatrix;\n"
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
		"output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
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
		"sampler sampler0: register(s0);\n"
		"Texture2D texture0: register(t0);\n"
		"\n"
		"float4 main(PS_INPUT input) : SV_Target\n"
		"{\n"
		"float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \n"
		"return out_col; \n"
		"}"
	};


	static const char* vertexShaderSrcGLSL =
	{
		"#version 150\n"
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
	{
		"#version 150\n"
		"in vec2 Texcoord;\n"
		"in vec4 col;\n"
		"uniform sampler2D sampler0;\n"
		"out vec4 out_col;\n"
		"void main()\n"
		"{\n"
		"out_col = col * texture(sampler0, Texcoord);\n"
		"}"
	};


    static const char* fragmentShaderSrcMetal =
    {
        "#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "\n"
        "struct VertexOut {\n"
        "    float4 position [[position]];\n"
        "    float2 texCoords;\n"
        "    float4 colour;\n"
        "};\n"
        "\n"
        "fragment float4 main_metal(VertexOut in [[stage_in]],\n"
        "                             texture2d<float> texture [[texture(0)]]) {\n"
        "    constexpr sampler linearSampler(coord::normalized, min_filter::linear, mag_filter::linear, mip_filter::linear);\n"
        "    float4 texColour = texture.sample(linearSampler, in.texCoords);\n"
        "    return in.colour * texColour;\n"
        "}\n"
    };

    static const char* vertexShaderSrcMetal =
    {
        "#include <metal_stdlib>\n"
        "using namespace metal;\n"
        "\n"
        "struct Constant {\n"
        "    float4x4 ProjectionMatrix;\n"
        "};\n"
        "\n"
        "struct VertexIn {\n"
        "    float2 position  [[attribute(VES_POSITION)]];\n"
        "    float2 texCoords [[attribute(VES_TEXTURE_COORDINATES0)]];\n"
        "    float4 colour     [[attribute(VES_DIFFUSE)]];\n"
        "};\n"
        "\n"
        "struct VertexOut {\n"
        "    float4 position [[position]];\n"
        "    float2 texCoords;\n"
        "    float4 colour;\n"
        "};\n"
        "\n"
        "vertex VertexOut vertex_main(VertexIn in                 [[stage_in]],\n"
        "                             constant Constant &uniforms [[buffer(PARAMETER_SLOT)]]) {\n"
        "    VertexOut out;\n"
        "    out.position = uniforms.ProjectionMatrix * float4(in.position, 0, 1);\n"

        "    out.texCoords = in.texCoords;\n"
        "    out.colour = in.colour;\n"

        "    return out;\n"
        "}\n"
    };


	//create the default shadows material
	Ogre::HighLevelGpuProgramManager& mgr = Ogre::HighLevelGpuProgramManager::getSingleton();

	Ogre::HighLevelGpuProgramPtr vertexShaderUnified = mgr.getByName("imgui/VP");
	Ogre::HighLevelGpuProgramPtr pixelShaderUnified = mgr.getByName("imgui/FP");

	Ogre::HighLevelGpuProgramPtr vertexShaderD3D11 = mgr.getByName("imgui/VP/D3D11");
	Ogre::HighLevelGpuProgramPtr pixelShaderD3D11 = mgr.getByName("imgui/FP/D3D11");

	Ogre::HighLevelGpuProgramPtr vertexShaderGL = mgr.getByName("imgui/VP/GL150");
	Ogre::HighLevelGpuProgramPtr pixelShaderGL = mgr.getByName("imgui/FP/GL150");

    Ogre::HighLevelGpuProgramPtr vertexShaderMetal = mgr.getByName("imgui/VP/Metal");
    Ogre::HighLevelGpuProgramPtr pixelShaderMetal = mgr.getByName("imgui/FP/Metal");

	if (vertexShaderUnified.isNull())
	{
		vertexShaderUnified = mgr.createProgram("imgui/VP", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "unified", Ogre::GPT_VERTEX_PROGRAM);
	}
	if (pixelShaderUnified.isNull())
	{
		pixelShaderUnified = mgr.createProgram("imgui/FP", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "unified", Ogre::GPT_FRAGMENT_PROGRAM);
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

    if (vertexShaderMetal.isNull()){
        vertexShaderMetal = mgr.createProgram("imgui/VP/Metal", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                           "metal", Ogre::GPT_VERTEX_PROGRAM);
        vertexShaderMetal->setParameter("entry_point", "vertex_main");
        vertexShaderMetal->setSource(vertexShaderSrcMetal);
        vertexShaderMetal->load();
        vertexShaderPtr->addDelegateProgram(vertexShaderMetal->getName());

    }
    if (pixelShaderMetal.isNull()){
        pixelShaderMetal = mgr.createProgram("imgui/FP/Metal", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                              "metal", Ogre::GPT_FRAGMENT_PROGRAM);
        vertexShaderMetal->setParameter("entry_point", "fragment_main");
        pixelShaderMetal->setSource(fragmentShaderSrcMetal);
        pixelShaderMetal->load();
        pixelShaderPtr->addDelegateProgram(pixelShaderMetal->getName());
    }


	if (vertexShaderGL.isNull())
	{
		vertexShaderGL = mgr.createProgram("imgui/VP/GL150", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			"glsl", Ogre::GPT_VERTEX_PROGRAM);
		vertexShaderGL->setSource(vertexShaderSrcGLSL);
		vertexShaderGL->load();
		vertexShaderPtr->addDelegateProgram(vertexShaderGL->getName());
	}
	if (pixelShaderGL.isNull())
	{
		pixelShaderGL = mgr.createProgram("imgui/FP/GL150", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			"glsl", Ogre::GPT_FRAGMENT_PROGRAM);
		pixelShaderGL->setSource(pixelShaderSrcGLSL);
		pixelShaderGL->load();
		pixelShaderGL->setParameter("sampler0", "int 0");

		pixelShaderPtr->addDelegateProgram(pixelShaderGL->getName());
	}


	Ogre::MaterialPtr imguiMaterial = Ogre::MaterialManager::getSingleton().create("imgui/material", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::Pass* mPass = imguiMaterial->getTechnique(0)->getPass(0);
	mPass->setFragmentProgram("imgui/FP");
	mPass->setVertexProgram("imgui/VP");


	Ogre::HlmsBlendblock blendblock(*mPass->getBlendblock());
	blendblock.mSourceBlendFactor = Ogre::SBF_SOURCE_ALPHA;
	blendblock.mDestBlendFactor = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;
	blendblock.mSourceBlendFactorAlpha = Ogre::SBF_ONE_MINUS_SOURCE_ALPHA;
	blendblock.mDestBlendFactorAlpha = Ogre::SBF_ZERO;
	blendblock.mBlendOperation = Ogre::SBO_ADD;
	blendblock.mBlendOperationAlpha = Ogre::SBO_ADD;
	blendblock.mSeparateBlend = true;
	blendblock.mIsTransparent = true;

	Ogre::HlmsMacroblock macroblock(*mPass->getMacroblock());
	macroblock.mCullMode = Ogre::CULL_NONE;
	macroblock.mDepthFunc = Ogre::CMPF_ALWAYS_PASS;
	macroblock.mDepthCheck = false;
	macroblock.mDepthWrite = false;
	macroblock.mScissorTestEnabled = true;

	mPass->setBlendblock(blendblock);
	mPass->setMacroblock(macroblock);

    Ogre::String renderSystemName = RoR::App::GetGfxScene()->GetSceneManager()->getDestinationRenderSystem()->getName();
    //Apparently opengl was the only one that needed this.
    if(renderSystemName == "OpenGL 3+ Rendering Subsystem"){
        mPass->getFragmentProgramParameters()->setNamedConstant("sampler0", 0);
    }
	mPass->createTextureUnitState()->setTextureName("ImGui/FontTex");
}


ImFont* ImGuiOverlay::addFont(const String& name, const String& group)
{
    FontPtr font = FontManager::getSingleton().getByName(name, group);
    OgreAssert(font, "font does not exist");
    OgreAssert(font->getType() == FT_TRUETYPE, "font must be of FT_TRUETYPE");
    DataStreamPtr dataStreamPtr =
        ResourceGroupManager::getSingleton().openResource(font->getSource(), font->getGroup());
    MemoryDataStream ttfchunk(dataStreamPtr, false); // transfer ownership to imgui

    // convert codepoint ranges for imgui
    CodePointRange cprange;
    for (const auto& r : font->getCodePointRangeList())
    {
        cprange.push_back(r.first);
        cprange.push_back(r.second);
    }

    ImGuiIO& io = ImGui::GetIO();
    const ImWchar* cprangePtr = io.Fonts->GetGlyphRangesAll();
    if (!cprange.empty())
    {
        cprange.push_back(0); // terminate
        mCodePointRanges.push_back(cprange);
        // ptr must persist until createFontTexture
        cprangePtr = mCodePointRanges.back().data();
    }

    ImFontConfig cfg;
    strncpy(cfg.Name, name.c_str(), 40);
    return io.Fonts->AddFontFromMemoryTTF(ttfchunk.getPtr(), (int)ttfchunk.size(), font->getTrueTypeSize(), &cfg,
                                          cprangePtr);
}

void ImGuiOverlay::ImGUIRenderable::createFontTexture()
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts->Fonts.empty())
        io.Fonts->AddFontDefault();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingleton().getRenderSystem()->getTextureGpuManager();
    Ogre::TextureGpu* texture = textureManager->createTexture(
        "ImGui/FontTex",
        Ogre::GpuPageOutStrategy::Discard,
        Ogre::TextureFlags::ManualTexture,
        Ogre::TextureTypes::Type2D,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    texture->setResolution(width, height);
    texture->setPixelFormat( Ogre::PFG_RGBA8_UNORM_SRGB );
    texture->setNumMipmaps(1u);

    // ## Following is a copypaste from OGRE manual: https://ogrecave.github.io/ogre-next/api/2.2/_ogre22_changes.html

    const uint32 rowAlignment = 4u;
    const size_t dataSize = Ogre::PixelFormatGpuUtils::getSizeBytes( texture->getWidth(),
                                                                     texture->getHeight(),
                                                                     texture->getDepth(),
                                                                     texture->getNumSlices(),
                                                                     texture->getPixelFormat(),
                                                                     rowAlignment );
    const size_t bytesPerRow = texture->_getSysRamCopyBytesPerRow( 0 );
    uint8 *imageData = reinterpret_cast<uint8*>( OGRE_MALLOC_SIMD( dataSize,
                                                                   MEMCATEGORY_RESOURCE ) );
    // ... fill imageData ...
    std::memcpy(imageData, pixels, dataSize);

    //Tell the texture we're going resident. The imageData pointer is only needed
    //if the texture pageout strategy is GpuPageOutStrategy::AlwaysKeepSystemRamCopy
    //which is in this example is not, so a nullptr would also work just fine.
    texture->_transitionTo( GpuResidency::Resident, imageData );
    texture->_setNextResidencyStatus( GpuResidency::Resident );
    //We have to upload the data via a StagingTexture, which acts as an intermediate stash
    //memory that is both visible to CPU and GPU.
    StagingTexture *stagingTexture = textureManager->getStagingTexture( texture->getWidth(),
                                                                        texture->getHeight(),
                                                                        texture->getDepth(),
                                                                        texture->getNumSlices(),
                                                                        texture->getPixelFormat() );
    //Call this function to indicate you're going to start calling mapRegion. startMapRegion
    //must be called from main thread.
    stagingTexture->startMapRegion();
    //Map region of the staging texture. This function can be called from any thread after
    //startMapRegion has already been called.
    TextureBox texBox = stagingTexture->mapRegion( texture->getWidth(), texture->getHeight(),
                                                   texture->getDepth(), texture->getNumSlices(),
                                                   texture->getPixelFormat() );
    texBox.copyFrom( imageData, texture->getWidth(), texture->getHeight(), bytesPerRow );
    //stopMapRegion indicates you're done calling mapRegion. Call this from the main thread.
    //It is your responsability to ensure you're done using all pointers returned from
    //previous mapRegion calls, and that you won't call it again.
    //You cannot upload until you've called this function.
    //Do NOT call startMapRegion again until you're done with upload() calls.
    stagingTexture->stopMapRegion();
    //Upload an area of the staging texture into the texture. Must be done from main thread.
    stagingTexture->upload( texBox, texture, 0, 0);
    //Tell the TextureGpuManager we're done with this StagingTexture. Otherwise it will leak.
    textureManager->removeStagingTexture( stagingTexture );
    stagingTexture = 0;
    //Do not free the pointer if texture's paging strategy is GpuPageOutStrategy::AlwaysKeepSystemRamCopy
    OGRE_FREE_SIMD( imageData, MEMCATEGORY_RESOURCE );
    imageData = 0;
    //This call is very important. It notifies the texture is fully ready for being displayed.
    //Since we've scheduled the texture to become resident and pp until now, the texture knew
    //it was being loaded and that only the metadata was certain. This call here signifies
    //loading is done; and any registered listeners will be notified.
    texture->notifyDataIsReady();

    mFontTex = texture;
}
void ImGuiOverlay::NewFrame(const FrameEvent& evt)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = std::max<float>(
        evt.timeSinceLastFrame,
        1e-4f); // see https://github.com/ocornut/imgui/commit/3c07ec6a6126fb6b98523a9685d1f0f78ca3c40c

    // Read keyboard modifiers inputs
    io.KeyAlt = false;
    io.KeySuper = false;

    v1::OverlayManager& oMgr = v1::OverlayManager::getSingleton();

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2(oMgr.getViewportWidth(), oMgr.getViewportHeight());

    // Start the frame
    ImGui::NewFrame();
}

void ImGuiOverlay::ImGUIRenderable::_update()
{
    if (!mMaterial->getNumSupportedTechniques())
    {
        mMaterial->load(); // Support for adding lights run time
    }

    RenderSystem* rSys = Root::getSingleton().getRenderSystem();
    v1::OverlayManager& oMgr = v1::OverlayManager::getSingleton();

    // Construct projection matrix, taking texel offset corrections in account (important for DirectX9)
    // See also:
    //     - OGRE-API specific hint: http://www.ogre3d.org/forums/viewtopic.php?f=5&p=536881#p536881
    //     - IMGUI Dx9 demo solution:
    //     https://github.com/ocornut/imgui/blob/master/examples/directx9_example/imgui_impl_dx9.cpp#L127-L138
    float texelOffsetX = rSys->getHorizontalTexelOffset();
    float texelOffsetY = rSys->getVerticalTexelOffset();
    float L = texelOffsetX;
    float R = oMgr.getViewportWidth() + texelOffsetX;
    float T = texelOffsetY;
    float B = oMgr.getViewportHeight() + texelOffsetY;

    mXform = Matrix4(2.0f / (R - L), 0.0f, 0.0f, (L + R) / (L - R), 0.0f, -2.0f / (B - T), 0.0f,
                     (T + B) / (B - T), 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

bool ImGuiOverlay::ImGUIRenderable::preRender(SceneManager* sm, RenderSystem* rsys)
{
    Viewport* vp = rsys->getCurrentRenderViewports();

    // Instruct ImGui to Render() and process the resulting CmdList-s
    // Adopted from https://bitbucket.org/ChaosCreator/imgui-ogre2.1-binding
    // ... Commentary on OGRE forums: http://www.ogre3d.org/forums/viewtopic.php?f=5&t=89081#p531059
    // OGRE 2.2+ version: https://gitlab.com/edherbert/southsea/-/tree/master/src/Gui/ImguiOgre
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    int vpWidth = vp->getActualWidth();
    int vpHeight = vp->getActualHeight();

    TextureUnitState* tu = mMaterial->getBestTechnique()->getPass(0)->getTextureUnitState(0);

    for (int i = 0; i < draw_data->CmdListsCount; ++i)
    {
        const ImDrawList* draw_list = draw_data->CmdLists[i];
        updateVertexData(draw_list->VtxBuffer, draw_list->IdxBuffer);

        unsigned int startIdx = 0;

        for (int j = 0; j < draw_list->CmdBuffer.Size; ++j)
        {
            // Create a renderable and fill it's buffers
            const ImDrawCmd* drawCmd = &draw_list->CmdBuffer[j];

            // Set scissoring
            Rect scissor(drawCmd->ClipRect.x, drawCmd->ClipRect.y, drawCmd->ClipRect.z,
                          drawCmd->ClipRect.w);

            // Clamp bounds to viewport dimensions
            scissor = scissor.intersect(Rect(0, 0, vpWidth, vpHeight));

            if (drawCmd->TextureId)
            {
                TextureGpu* tex = static_cast<TextureGpu*>(drawCmd->TextureId);
                if (tex)
                {
                    rsys->_setTexture(0, tex, /*bDepthReadOnly=*/false);
                    rsys->_setCurrentDeviceFromTexture(tex); // Needed?
                }
            }

            // see https://gitlab.com/edherbert/southsea/-/blob/master/src/Gui/ImguiOgre/ImguiManager.cpp#L226
            vp->setScissors(scissor.left, scissor.top, scissor.right-scissor.left, scissor.bottom-scissor.top);

            // Render!
            mRenderOp.indexData->indexStart = startIdx;
            mRenderOp.indexData->indexCount = drawCmd->ElemCount;

            rsys->_render(mRenderOp);

            if (drawCmd->TextureId)
            {
                // reset to pass state
                rsys->_setTexture(0, mFontTex, /*bDepthReadOnly=*/false);
                rsys->_setCurrentDeviceFromTexture(mFontTex); // Needed?
            }

            // Update counts
            startIdx += drawCmd->ElemCount;
        }
    }
    vp->setScissors(0, 0, 1, 1); // reset
    return false;
}

const LightList& ImGuiOverlay::ImGUIRenderable::getLights() const
{
    // Overlayelements should not be lit by the scene, this will not get called
    static LightList ll;
    return ll;
}

ImGuiOverlay::ImGUIRenderable::ImGUIRenderable()
{
    // default overlays to preserve their own detail level
    mPolygonModeOverrideable = false;

    // use identity projection and view matrices
    mUseIdentityProjection = true;
    mUseIdentityView = true;
}
//-----------------------------------------------------------------------------------
void ImGuiOverlay::ImGUIRenderable::initialise(void)
{
    createFontTexture();
    createMaterial();

    mRenderOp.vertexData = OGRE_NEW v1::VertexData();
    mRenderOp.indexData = OGRE_NEW v1::IndexData();

    mRenderOp.vertexData->vertexCount = 0;
    mRenderOp.vertexData->vertexStart = 0;

    mRenderOp.indexData->indexCount = 0;
    mRenderOp.indexData->indexStart = 0;
    mRenderOp.operationType = OperationType::OT_TRIANGLE_LIST;
    mRenderOp.useIndexes = true;
    mRenderOp.useGlobalInstancingVertexBufferIsAvailable = false;

    v1::VertexDeclaration* decl = mRenderOp.vertexData->vertexDeclaration;

    // vertex declaration
    size_t offset = 0;
    decl->addElement(0, offset, VET_FLOAT2, VES_POSITION);
    offset += v1::VertexElement::getTypeSize(VET_FLOAT2);
    decl->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
    offset += v1::VertexElement::getTypeSize(VET_FLOAT2);
    decl->addElement(0, offset, VET_COLOUR, VES_DIFFUSE);
}
//-----------------------------------------------------------------------------------
ImGuiOverlay::ImGUIRenderable::~ImGUIRenderable()
{
    OGRE_DELETE mRenderOp.vertexData;
    OGRE_DELETE mRenderOp.indexData;
}
//-----------------------------------------------------------------------------------
void ImGuiOverlay::ImGUIRenderable::updateVertexData(const ImVector<ImDrawVert>& vtxBuf,
                                                     const ImVector<ImDrawIdx>& idxBuf)
{
    v1::VertexBufferBinding* bind = mRenderOp.vertexData->vertexBufferBinding;

    if (bind->getBindings().empty() || bind->getBuffer(0)->getNumVertices() != size_t(vtxBuf.size()))
    {
        bind->setBinding(0, v1::HardwareBufferManager::getSingleton().createVertexBuffer(
                                sizeof(ImDrawVert), vtxBuf.size(), v1::HardwareBuffer::HBU_WRITE_ONLY));
    }
    if (!mRenderOp.indexData->indexBuffer ||
        mRenderOp.indexData->indexBuffer->getNumIndexes() != size_t(idxBuf.size()))
    {
        mRenderOp.indexData->indexBuffer = v1::HardwareBufferManager::getSingleton().createIndexBuffer(
            v1::HardwareIndexBuffer::IT_16BIT, idxBuf.size(), v1::HardwareBuffer::HBU_WRITE_ONLY);
    }

    // Copy all vertices
    bind->getBuffer(0)->writeData(0, vtxBuf.size_in_bytes(), vtxBuf.Data, true);
    mRenderOp.indexData->indexBuffer->writeData(0, idxBuf.size_in_bytes(), idxBuf.Data, true);

    mRenderOp.vertexData->vertexStart = 0;
    mRenderOp.vertexData->vertexCount = vtxBuf.size();
}
} // namespace Ogre
