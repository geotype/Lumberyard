/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef __RENDERGL_GBUFFER_H
#define __RENDERGL_GBUFFER_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Color.h>
#include "RenderGLConfig.h"


namespace RenderGL
{
    // forward declarations
    class GLSLShader;
    class RenderTexture;

    /**
     *
     *
     */
    class RENDERGL_API GBuffer
    {
    public:
        enum EComponent
        {
            COMPONENT_SHADED    = 0,
            COMPONENT_GLOW      = 1,
            NUM_COMPONENTS
        };

        GBuffer();
        ~GBuffer();

        bool Init(uint32 width, uint32 height);
        void Release();

        void Activate();
        void Deactivate();
        void Clear(const MCore::RGBAColor& color);

        void Render();

        MCORE_INLINE uint32 GetTextureID(EComponent component) const    { return mComponents[component]; }

        bool Resize(uint32 width, uint32 height);
        bool ResizeTextures(uint32 screenWidth, uint32 screenHeight);

        MCORE_INLINE RenderTexture* GetRenderTargetA()                  { return mRenderTargetA; }
        MCORE_INLINE RenderTexture* GetRenderTargetB()                  { return mRenderTargetB; }
        MCORE_INLINE RenderTexture* GetRenderTargetC()                  { return mRenderTargetC; }
        MCORE_INLINE RenderTexture* GetRenderTargetD()                  { return mRenderTargetD; }
        MCORE_INLINE RenderTexture* GetRenderTargetE()                  { return mRenderTargetE; }


    private:
        uint32  mFBO;
        uint32  mComponents[NUM_COMPONENTS];
        uint32  mDepthBufferID;
        //uint32    mPrevWidth;
        //uint32    mPrevHeight;
        uint32  mWidth;
        uint32  mHeight;

        RenderTexture*      mRenderTargetA; /**< A temp render target. */
        RenderTexture*      mRenderTargetB; /**< A temp render target. */
        RenderTexture*      mRenderTargetC; /**< A temp render target. */
        RenderTexture*      mRenderTargetD; /**< Render target with width and height divided by four. */
        RenderTexture*      mRenderTargetE; /**< Render target with width and height divided by four. */
    };
}   // namespace RenderGL


#endif
