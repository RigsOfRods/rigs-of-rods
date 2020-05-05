// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.

#pragma once

#include <OgreOverlay.h>

#include <imgui.h>

namespace Ogre
{
class ImGuiOverlay : public Overlay
{
public:
    ImGuiOverlay();
    ~ImGuiOverlay();

    /// add font from ogre .fontdef file
    /// must be called before first show()
    ImFont* addFont(const String& name, const String& group OGRE_RESOURCE_GROUP_INIT);

    static void NewFrame(const FrameEvent& evt);

    void _findVisibleObjects(Camera* cam, RenderQueue* queue, Viewport* vp);

    void initialise(); // RIGSOFRODS: Private and non-virtual in OGRE... how does it even work?

private:

    typedef std::vector<ImWchar> CodePointRange;
    std::vector<CodePointRange> mCodePointRanges;

    class ImGUIRenderable : public Ogre::Renderable
    {
    public:
        ImGUIRenderable();
        ~ImGUIRenderable();

        void initialise();

        void updateVertexData(const ImVector<ImDrawVert>& vtxBuf, const ImVector<ImDrawIdx>& idxBuf);

        bool preRender(SceneManager* sm, RenderSystem* rsys);

        virtual void getWorldTransforms(Matrix4* xform) const { *xform = mXform; }
        virtual void getRenderOperation(RenderOperation& op) { op = mRenderOp; }

        const LightList& getLights(void) const;

        void createMaterial();
        void createFontTexture();

        const MaterialPtr& getMaterial() const { return mMaterial; }

        Real getSquaredViewDepth(const Camera*) const { return 0; }

        void _update();

        bool mConvertToBGR;

        Matrix4 mXform;
        RenderOperation mRenderOp;
        TexturePtr mFontTex;
        MaterialPtr mMaterial;
    };

    ImGUIRenderable mRenderable;
};
} // namespace Ogre

