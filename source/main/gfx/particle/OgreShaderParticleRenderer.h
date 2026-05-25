#pragma once
#ifndef __OGRE_SHADER_PARTICLE_RENDERER_H__
#define __OGRE_SHADER_PARTICLE_RENDERER_H__


#include <OgreParticleSystemRenderer.h>
#include <Ogre.h>

namespace Ogre {

/// @addtogroup Gfx
/// @{

/// @addtogroup Particle
/// @{

    /** Specialisation of ParticleSystemRenderer to render particles using
        a custom shaders.
    @remarks
        This renderer has a few more options than the standard particle system,
        which will be passed to it automatically when the particle system itself
        does not understand them.
    */
    class ShaderParticleRenderer : public ParticleSystemRenderer, public Renderable
    {
    protected:
        /// rendering data
        MaterialPtr			mMaterial;				//!< material for this renderable
        VertexData*			mVertexData;
        IndexData*			mIndexData;
        uint8				mRenderQueueID;

        /// vertex format for particles
        /// * position will be always present in vertex format (don't need to specify) - float4 - w component is vertex index (0-3)
        /// * following informations will be packed into 1 texture coord:
        ///     mVertexFormatRotation + mVertexFormatRotationSpeed = float2
        ///     mVertexFormatTTL + mVertexFormatTotalTTL + mVertexFormatTimeFragment + mVertexFormatTimeFragmentInv = float4
        /// * mVertexFormatColour will be set as diffuse colour in vertex declaration
        /// * last tex coord is custom particle parameters defined by emitters (always float4)
        bool				mVertexFormatColour;			//!< particle colour (float4)
        bool				mVertexFormatTexture;			//!< true if particles use texture (float2)
        bool				mVertexFormatSize;				//!< particle size (width and height - float2)
        bool				mVertexFormatRotation;			//!< particle rotation (radians - float1)
        bool				mVertexFormatRotationSpeed;		//!< particle rotation speed (radians/s - float1)
        bool				mVertexFormatDirection;			//!< particle direction (float3)
        bool				mVertexFormatTTL;				//!< particle ttl (float1)
        bool				mVertexFormatTotalTTL;			//!< particle total ttl (float1)
        bool				mVertexFormatTimeFragment;		//!< particle time fragment (ttl / total ttl) (float1) (value 0.0f - 1.0f)
        bool				mVertexFormatTimeFragmentInv;	//!< particle inverse time fragment (1.0f - ttl / total ttl) (float1) (value 0.0f - 1.0f)
        size_t				mVertexSize;
        Vector2				mTexCoordTable[4];				//!< default texture coordinates

        /// other informations
        Vector2				mDefaultParticleSize;	//!< default particle size
        Node*				mParentNode;			//!< parent node for particle system - used for world transformation
        SortMode			mSortMode;				//!< particle sorting
        mutable LightList	mLightList;				//!< light list for renderable
        mutable ulong		mLightListUpdated;		//!< indicator if we need update light list
        Real				mRadius;				//!< maximum distance between particles and parent node
        bool				mParentIsTagPoint;		//!< true if parent node is tag point
        bool				mKeepInLocalSpace;		//!< control transformation matrix for particles

    public:
        ShaderParticleRenderer();
        virtual ~ShaderParticleRenderer();

        //////////////////////////////////////////////////////////////////////////
        /// Command objects for defining vertex format (see ParamCommand).
        /// diffuse colour
        class _OgrePrivate CmdVertexFormatColour : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// general texture coord
        class _OgrePrivate CmdVertexFormatTexture : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// particle size
        class _OgrePrivate CmdVertexFormatSize : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// rotation
        class _OgrePrivate CmdVertexFormatRotation : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// rotation speed (rad/s)
        class _OgrePrivate CmdVertexFormatRotationSpeed : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// direction
        class _OgrePrivate CmdVertexFormatDirection : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// time-to-live
        class _OgrePrivate CmdVertexFormatTTL : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// total time-to-live
        class _OgrePrivate CmdVertexFormatTotalTTL : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// TTL / total_TTL
        class _OgrePrivate CmdVertexFormatTimeFrag : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        /// 1.0f - TTL / total_TTL
        class _OgrePrivate CmdVertexFormatTimeFragInv : public ParamCommand
        {
        public:
            String doGet(const void* target) const;
            void doSet(void* target, const String& val);
        };

        //////////////////////////////////////////////////////////////////////////
        // getters / setters
        void setVertexFormatColour(bool bUse) { mVertexFormatColour = bUse; }
        bool getVertexFormatColour() const { return mVertexFormatColour; }

        void setVertexFormatTexture(bool bUse) { mVertexFormatTexture = bUse; }
        bool getVertexFormatTexture() const { return mVertexFormatTexture; }

        void setVertexFormatSize(bool bUse) { mVertexFormatSize = bUse; }
        bool getVertexFormatSize() const { return mVertexFormatSize; }

        void setVertexFormatRotation(bool bUse) { mVertexFormatRotation = bUse; }
        bool getVertexFormatRotation() const { return mVertexFormatRotation; }

        void setVertexFormatRotationSpeed(bool bUse) { mVertexFormatRotationSpeed = bUse; }
        bool getVertexFormatRotationSpeed() const { return mVertexFormatRotationSpeed; }

        void setVertexFormatDirection(bool bUse) { mVertexFormatDirection = bUse; }
        bool getVertexFormatDirection() const { return mVertexFormatDirection; }

        void setVertexFormatTTL(bool bUse) { mVertexFormatTTL = bUse; }
        bool getVertexFormatTTL() const { return mVertexFormatTTL; }

        void setVertexFormatTotalTTL(bool bUse) { mVertexFormatTotalTTL = bUse; }
        bool getVertexFormatTotalTTL() const { return mVertexFormatTotalTTL; }

        void setVertexFormatTimeFragment(bool bUse) { mVertexFormatTimeFragment = bUse; }
        bool getVertexFormatTimeFragment() const { return mVertexFormatTimeFragment; }

        void setVertexFormatTimeFragmentInv(bool bUse) { mVertexFormatTimeFragmentInv = bUse; }
        bool getVertexFormatTimeFragmentInv() const { return mVertexFormatTimeFragmentInv; }

        //////////////////////////////////////////////////////////////////////////
        /// @name Ogre::ParticleSystemRenderer interface
        /// @{
        virtual const String& getType(void) const override;
        virtual void _updateRenderQueue(RenderQueue* queue, std::vector<Particle*>& currentParticles, bool cullIndividually) override;
        virtual void visitRenderables(Renderable::Visitor* visitor, bool debugRenderables = false) override;
        virtual void _setMaterial(MaterialPtr& mat) override;
        virtual void _notifyCurrentCamera(Camera* cam) override;
        virtual void _notifyParticleRotated(void); // GCC says it does not override
        virtual void _notifyParticleResized(void); // GCC says it does not override
        virtual void _notifyParticleQuota(size_t quota) override;
        virtual void _notifyAttached(Node* parent, bool isTagPoint = false) override;
        virtual void _notifyDefaultDimensions(Real width, Real height) override;
        virtual void setRenderQueueGroup(uint8 queueID) override;
        virtual void setKeepParticlesInLocalSpace(bool keepLocal) override;
        virtual SortMode _getSortMode(void) const override;
        /// @}

        //////////////////////////////////////////////////////////////////////////
        /// @name Ogre::Renderable interface
        /// @{
        virtual const MaterialPtr& getMaterial(void) const override;
        virtual void getRenderOperation(RenderOperation& op) override;
        virtual void getWorldTransforms(Matrix4* xform) const override;
        virtual Real getSquaredViewDepth(const Camera* cam) const override;
        virtual const LightList& getLights(void) const override;
        /// @}

    private:
        const String rendererTypeName = "shader";

        /// allocate hardware buffers and prepare them for filling particles
        bool allocateBuffers(size_t iNumParticles);

        /// add particle to vertex buffer
        void addParticle(uint8* pDataVB, const Particle& particle) const;
        void setRenderQueueGroupAndPriority(Ogre::uint8,Ogre::ushort);
    protected:
        static CmdVertexFormatColour		msVertexFmtColour;
        static CmdVertexFormatTexture		msVertexFmtTexture;
        static CmdVertexFormatSize			msVertexFmtSize;
        static CmdVertexFormatRotation		msVertexFmtRotation;
        static CmdVertexFormatRotationSpeed	msVertexFmtRotationSpeed;
        static CmdVertexFormatDirection		msVertexFmtDirection;
        static CmdVertexFormatTTL			msVertexFmtTTL;
        static CmdVertexFormatTotalTTL		msVertexFmtTotalTTL;
        static CmdVertexFormatTimeFrag		msVertexFmtTimeFrag;
        static CmdVertexFormatTimeFragInv	msVertexFmtTimeFragInv;
    };

    /// Factory class for ShaderParticleRenderer
    class ShaderParticleRendererFactory : public ParticleSystemRendererFactory
    {
    public:
        const String& getType() const override;
        ParticleSystemRenderer* createInstance( const String& name ) override;
        void destroyInstance( ParticleSystemRenderer* inst) override;
    private:
        const String rendererTypeName = "shader";
    };

/// @} // addtogroup Particle
/// @} // addtogroup Gfx

} // namespace Ogre

#endif // __OGRE_SHADER_PARTICLE_RENDERER_H__
