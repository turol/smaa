/**
 * Copyright (C) 2011 Jorge Jimenez (jorge@iryoku.com)
 * Copyright (C) 2011 Belen Masia (bmasia@unizar.es) 
 * Copyright (C) 2011 Jose I. Echevarria (joseignacioechevarria@gmail.com) 
 * Copyright (C) 2011 Fernando Navarro (fernandn@microsoft.com) 
 * Copyright (C) 2011 Diego Gutierrez (diegog@unizar.es)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the following disclaimer
 *       in the documentation and/or other materials provided with the 
 *       distribution:
 * 
 *      "Uses SMAA. Copyright (C) 2011 by Jorge Jimenez, Jose I. Echevarria,
 *       Belen Masia, Fernando Navarro and Diego Gutierrez."
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS 
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are 
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the copyright holders.
 */


#ifndef SMAA_H
#define SMAA_H

#include <dxgi.h>
#include <d3d10.h>
#include "RenderTarget.h"

/**
 * IMPORTANT NOTICE: please note that the documentation given in this file is
 * rather limited. For more information checkout SMAA.h in the root directory
 * of the source release (the shader header).
 */


class SMAA {
    public:
        class ExternalStorage;

        enum Preset { PRESET_LOW, PRESET_MEDIUM, PRESET_HIGH, PRESET_ULTRA, PRESET_CUSTOM };
        enum Input { INPUT_LUMA, INPUT_COLOR, INPUT_DEPTH };

        /**
         * If you have one or two spare render targets of the same size as the
         * backbuffer, you may want to pass them in the 'storage' parameter.
         * You may pass one or the two, depending on what you have available.
         *
         * A non-sRGB RG buffer (at least) is expected for storing edges.
         * A non-sRGB RGBA buffer is expected for the blending weights.
         *
         * By default, two render targets will be created for storing
         * intermediate calculations.
         */
        SMAA(ID3D10Device *device, int width, int height, Preset preset, bool predication=false,
             const ExternalStorage &storage=ExternalStorage());
        ~SMAA();

        /**
         * Mandatory input textures varies depending on 'input':
         *    INPUT_LUMA:
         *    INPUT_COLOR:
         *        go(srcGammaSRV, srcSRV, NULL,     depthSRV, dsv)
         *    INPUT_DEPTH:
         *        go(NULL,        srcSRV, depthSRV, depthSRV, dsv)
         *
         * You can safely pass everything (do not use NULLs) if you want, the
         * extra paramters will be ignored accordingly. See descriptions below.
         *
         * Input texture 'depthSRV' will be used for predication if enabled. We
         * recommend using a light accumulation buffer or object ids, if
         * available.
         *
         * IMPORTANT: The stencil component of 'dsv' is used to mask zones to
         * be processed. It is assumed to be already cleared to zero when this
         * function is called. It is not done here because it is usually
         * cleared together with the depth.
         *
         * The viewport, the binded render target and the input layout is saved
         * and restored accordingly. However, we modify but not restore the
         * depth-stencil and blend states.
         */
        void go(ID3D10ShaderResourceView *srcGammaSRV, // Non-SRGB version of the input color texture.
                ID3D10ShaderResourceView *srcSRV, // SRGB version of the input color texture.
                ID3D10ShaderResourceView *depthSRV, // Input depth texture.
                ID3D10RenderTargetView *dstRTV, // Output render target.
                ID3D10DepthStencilView *dsv, // Depth-stencil buffer.
                Input input); // Selects the input for edge detection.

        /**
         * Threshold for the edge detection. Only has effect if PRESET_CUSTOM
         * is selected.
         */
        float getThreshold() const { return threshold; }
        void setThreshold(float threshold) { this->threshold = threshold; }

        /**
         * Maximum length to search for horizontal/vertical patterns. Each step
         * is two pixels wide. Only has effect if PRESET_CUSTOM is selected.
         */
        int getMaxSearchSteps() const { return maxSearchSteps; }
        void setMaxSearchSteps(int maxSearchSteps) { this->maxSearchSteps = maxSearchSteps; }

        /**
         * Maximum length to search for diagonal patterns. Only has effect if
         * PRESET_CUSTOM is selected.
         */
        int getMaxSearchStepsDiag() const { return maxSearchStepsDiag; }
        void setMaxSearchStepsDiag(int maxSearchStepsDiag) { this->maxSearchStepsDiag = maxSearchStepsDiag; }

        /**
         * Desired corner rounding, from 0.0 (no rounding) to 100.0 (full
         * rounding). Only has effect if PRESET_CUSTOM is selected.
         */
        float getCornerRounding() const { return cornerRounding; }
        void setCornerRounding(float cornerRounding) { this->cornerRounding = cornerRounding; }

        /**
         * These two are just for debugging purposes.
         */
        RenderTarget *getEdgesRenderTarget() { return edgesRT; }
        RenderTarget *getBlendRenderTarget() { return blendRT; }

        /**
         * This class allows to pass spare storage buffers to the SMAA class.
         */
        class ExternalStorage {
            public:
                ExternalStorage(ID3D10ShaderResourceView *edgesSRV=NULL,
                                ID3D10RenderTargetView *edgesRTV=NULL,
                                ID3D10ShaderResourceView *weightsSRV=NULL,
                                ID3D10RenderTargetView *weightsRTV=NULL)
                    : edgesSRV(edgesSRV),
                      edgesRTV(edgesRTV), 
                      weightsSRV(weightsSRV),
                      weightsRTV(weightsRTV) {}

            ID3D10ShaderResourceView *edgesSRV, *weightsSRV;
            ID3D10RenderTargetView *edgesRTV, *weightsRTV;
        };

    private:
        void loadAreaTex();
        void loadSearchTex();
        void edgesDetectionPass(ID3D10DepthStencilView *dsv, Input input);
        void blendingWeightsCalculationPass(ID3D10DepthStencilView *dsv);
        void neighborhoodBlendingPass(ID3D10RenderTargetView *dstRTV, ID3D10DepthStencilView *dsv);

        ID3D10Device *device;
        Preset preset;
        ID3D10Effect *effect;
        Quad *quad;

        RenderTarget *edgesRT;
        RenderTarget *blendRT;

        ID3D10Texture2D *areaTex;
        ID3D10ShaderResourceView *areaTexSRV;
        ID3D10Texture2D *searchTex;
        ID3D10ShaderResourceView *searchTexSRV;

        ID3D10EffectScalarVariable *thresholdVariable, *cornerRoundingVariable,
                                   *maxSearchStepsVariable, *maxSearchStepsDiagVariable;
        ID3D10EffectShaderResourceVariable *areaTexVariable, *searchTexVariable,
                                           *colorTexVariable, *colorGammaTexVariable, *depthTexVariable,
                                           *edgesTexVariable, *blendTexVariable;

        ID3D10EffectTechnique *edgeDetectionTechniques[3],
                              *blendWeightCalculationTechnique,
                              *neighborhoodBlendingTechnique;

        float threshold, cornerRounding;
        int maxSearchSteps, maxSearchStepsDiag;
};

#endif