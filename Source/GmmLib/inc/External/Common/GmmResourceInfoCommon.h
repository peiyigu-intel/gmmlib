/*==============================================================================
Copyright(c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files(the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and / or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
============================================================================*/
#pragma once

#ifdef __cplusplus
#include "GmmMemAllocator.hpp"
#include "GmmCachePolicy.h"
#include "GmmUtil.h"
#include "GmmInfoExt.h"
#include "GmmInfo.h"
#include "../../../Platform/GmmPlatforms.h"
#include "../../../../inc/common/gfxmacro.h"

// Macro definitions
#ifndef __GMM_ASSERT
    // Needs to be defined before including this file. If not defined, then
    // we'll nop these macros.
    #define __GMM_ASSERT(expr) 
    #define GMM_ASSERTDPF(expr, ret) 
    #define __GMM_ASSERTPTR(expr, ret)
#endif

//Forward declaration
GMM_GFX_SIZE_T  __GmmTexGetMipWidth(GMM_TEXTURE_INFO *pTexInfo, uint32_t MipLevel);
uint32_t           __GmmTexGetMipHeight(GMM_TEXTURE_INFO *pTexInfo, uint32_t MipLevel);
uint32_t           __GmmTexGetMipDepth(GMM_TEXTURE_INFO *pTexInfo, uint32_t MipLevel);

/////////////////////////////////////////////////////////////////////////////////////
/// @file GmmResourceInfoCommon.h
/// @brief This file contains the functions and members of GmmResourceInfo that is 
///       common for both Linux and Windows.
///
/////////////////////////////////////////////////////////////////////////////////////
namespace GmmLib
{
    /////////////////////////////////////////////////////////////////////////
    /// Contains functions and members that are common between Linux and
    /// Windows implementation.  This class is inherited by the Linux and
    /// Windows specific class, so clients shouldn't have to ever interact
    /// with this class directly.
    /////////////////////////////////////////////////////////////////////////
    class NON_PAGED_SECTION GmmResourceInfoCommon: 
                            public GmmMemAllocator
    {
        protected:  
            /// Type of Client type using the library. Can be used by GmmLib to 
            /// implement client specific functionality.
            GMM_CLIENT                          ClientType; 
            GMM_TEXTURE_INFO                    Surf;                       ///< Contains info about the surface being created
            GMM_TEXTURE_INFO                    AuxSurf;                    ///< Contains info about the auxiliary surface if using Unified Auxiliary surfaces.
            GMM_TEXTURE_INFO                    AuxSecSurf;                 ///< For multi-Aux surfaces (eg: unified lossless MSAA compression, Z compression), contains info about the secondary auxiliary surface
            GMM_TEXTURE_INFO                    PlaneSurf[GMM_MAX_PLANE];   ///< Contains info for each plane for tiled Ys/Yf planar resources
            GMM_TEXTURE_INFO                    PlaneAuxSurf[GMM_MAX_PLANE];   ///< Contains auxiliary surface info for each plane for tiled Ys/Yf planar resources

            uint32_t                               RotateInfo;     
            GMM_EXISTING_SYS_MEM                ExistingSysMem;     ///< Info about resources initialized with existing system memory
            GMM_GFX_ADDRESS                     IsolatedGfxAddress; ///< PIGMS address (WDDM1.x only)
            GMM_GFX_ADDRESS                     SvmAddress;         ///< Driver managed SVM address

            GMM_VOIDPTR64                       pGmmLibContext;     ///< Pointer to GmmLib context passed in during Create()
            GMM_VOIDPTR64                       pPrivateData;       ///< Allows clients to attach any private data to GmmResourceInfo

        private:
            GMM_STATUS          ApplyExistingSysMemRestrictions();

        protected:
            /* Function prototypes */
            BOOLEAN             IsPresentableformat();
            // Move GMM Restrictions to it's own class?
            void                GetGenericRestrictions(__GMM_BUFFER_TYPE *pBuff);
            __GMM_BUFFER_TYPE*  GetBestRestrictions(__GMM_BUFFER_TYPE *pFirstBuffer, const __GMM_BUFFER_TYPE *pSecondBuffer);
            virtual BOOLEAN     CopyClientParams(GMM_RESCREATE_PARAMS &CreateParams);
            BOOLEAN             RedescribePlanes();
            BOOLEAN             ReAdjustPlaneProperties(BOOLEAN IsAuxSurf);

            /* Inline functions */

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the Platform info.  If Platform has been overriden by the clients, then
            /// it returns the overriden Platform Info struct.
            /// @return     Reference to the relevent ::GMM_PLATFORM_INFO
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE const GMM_PLATFORM_INFO& GetPlatformInfo()
            {
                #if(defined(__GMM_KMD__) && (_DEBUG || _RELEASE_INTERNAL))
                    if(GFX_GET_CURRENT_RENDERCORE(Surf.Platform) !=  GFX_GET_CURRENT_RENDERCORE(((Context*)pGmmLibContext)->GetPlatformInfo().Platform))
                    {
                        return ((Context*)pGmmLibContext)->GetOverridePlatformInfo();
                    }
                    else
                    {
                        return ((Context*)pGmmLibContext)->GetPlatformInfo();
                    }
                #else
                    return ((Context*)pGmmLibContext)->GetPlatformInfo();
                #endif
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns GmmLib Context associated with this resource
            /// @return ::GmmLib Context
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE Context* GetGmmLibContext()
            {
                return reinterpret_cast<Context*>(pGmmLibContext);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Checks where the restrictions are invalid or not
            /// @param[in]  pRestriction Restrictions to check
            /// @return     TRUE if restriction is invalid. FALSE otherwise.
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE BOOLEAN IsRestrictionInvalid(__GMM_BUFFER_TYPE *pRestriction)
            {
                return ((pRestriction->MinDepth == 0xffffffff) ? TRUE : FALSE);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns restrictions for a linear buffer.
            /// @param[out]     pBuff Restrictions are returned in this buffer
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GetLinearRestrictions(__GMM_BUFFER_TYPE* pBuff)
            {
                *pBuff = GMM_OVERRIDE_PLATFORM_INFO(&Surf)->Linear;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns restrictions for the primary buffer.
            /// @param[out]     pBuff Restrictions are returned in this buffer
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GetPrimaryRestrictions(__GMM_BUFFER_TYPE* pBuff)
            {
                *pBuff = GMM_OVERRIDE_PLATFORM_INFO(&Surf)->ASyncFlipSurface;
            }

        public:
            /* Constructors */
            GmmResourceInfoCommon():
                ClientType(),
                Surf(),
                AuxSurf(),
                AuxSecSurf(),
                PlaneSurf{},
                PlaneAuxSurf{},
                RotateInfo(),
                ExistingSysMem(),
                IsolatedGfxAddress(),
                SvmAddress(),
                pGmmLibContext(),
                pPrivateData()
            {
                
            }

            GmmResourceInfoCommon& operator=(const GmmResourceInfoCommon& rhs)
            {
                ClientType          = rhs.ClientType;
                Surf                = rhs.Surf;
                AuxSurf             = rhs.AuxSurf;
                RotateInfo          = rhs.RotateInfo;
                ExistingSysMem      = rhs.ExistingSysMem;
                IsolatedGfxAddress  = rhs.IsolatedGfxAddress;
                SvmAddress          = rhs.SvmAddress;
                pPrivateData        = rhs.pPrivateData;
                pGmmLibContext      = rhs.pGmmLibContext;

                return *this;
            }

            virtual ~GmmResourceInfoCommon()
            {
                if (ExistingSysMem.pVirtAddress && ExistingSysMem.IsGmmAllocated)
                {
                    GMM_FREE((void *)ExistingSysMem.pVirtAddress);
                }
            }

            /* Function prototypes */
            GMM_STATUS              GMM_STDCALL Create(Context &GmmLibContext, GMM_RESCREATE_PARAMS &CreateParams);
            BOOLEAN                 GMM_STDCALL ValidateParams();
            void                    GMM_STDCALL GetRestrictions(__GMM_BUFFER_TYPE& Restrictions);
            uint32_t                   GMM_STDCALL GetPaddedWidth(uint32_t MipLevel);
            uint32_t                   GMM_STDCALL GetPaddedHeight(uint32_t MipLevel);
            uint32_t                   GMM_STDCALL GetPaddedPitch(uint32_t MipLevel);
            uint32_t                   GMM_STDCALL GetQPitch();
            GMM_STATUS              GMM_STDCALL GetOffset(GMM_REQ_OFFSET_INFO &ReqInfo);
            BOOLEAN                 GMM_STDCALL CpuBlt(GMM_RES_COPY_BLT *pBlt);
            BOOLEAN                 GMM_STDCALL GetMappingSpanDesc(GMM_GET_MAPPING *pMapping);
            BOOLEAN                 GMM_STDCALL Is64KBPageSuitable();
            void                    GMM_STDCALL GetTiledResourceMipPacking(UINT *pNumPackedMips,
                                                                           UINT *pNumTilesForPackedMips);
            uint32_t                   GMM_STDCALL GetPackedMipTailStartLod();
            BOOLEAN                 GMM_STDCALL GetDisplayFastClearSupport();
            BOOLEAN                 GMM_STDCALL GetDisplayCompressionSupport();


            /* inline functions */

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the system memory pointer. It selectively returns either the natural 
            /// pointer or a value appriopriately page aligned for D3DDI_ALLOCATIONINFO, 
            /// depending on what the caller request.
            /// @param[in]      IsD3DDdiAllocation: Specifies where allocation was made by a D3D client
            /// @return         Pointer to system memory. NULL if not available.
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void* GMM_STDCALL GetSystemMemPointer(BOOLEAN IsD3DDdiAllocation)
            {
                if (IsD3DDdiAllocation)
                {
                    return (void *)GMM_GFX_ADDRESS_CANONIZE(ExistingSysMem.pGfxAlignedVirtAddress);
                }
                else
                {
                    return (void *)GMM_GFX_ADDRESS_CANONIZE(ExistingSysMem.pVirtAddress);
                }
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the system memory size.
            /// @return     Size of memory.
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetSystemMemSize()
            {
                return ExistingSysMem.Size;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns a reference to the surface flags.
            /// @return     Reference to ::GMM_RESOURCE_FLAGS
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_RESOURCE_FLAG& GMM_STDCALL GetResFlags()
            {
                return Surf.Flags;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource type
            /// @return     ::GMM_RESOURCE_TYPE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_RESOURCE_TYPE GMM_STDCALL GetResourceType()
            {
                return Surf.Type;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource format
            /// @return     ::GMM_RESOURCE_FORMAT
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_RESOURCE_FORMAT GMM_STDCALL GetResourceFormat()
            {
                return Surf.Format;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource width
            /// @return     width
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetBaseWidth()
            {
                return Surf.BaseWidth;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource height
            /// @return     height
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetBaseHeight()
            {
                return Surf.BaseHeight;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource depth
            /// @return     depth
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetBaseDepth()
            {
                return Surf.Depth;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource's base alignment
            /// @return     Base Alignment
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetBaseAlignment()
            {
                return Surf.Alignment.BaseAlignment;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource's max lod
            /// @return     Max Lod
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetMaxLod()
            {
                return Surf.MaxLod;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource's max array size
            /// @return     Max Array Size
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetArraySize()
            {
                return Surf.ArraySize;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource's rotation info
            /// @return    rotation info
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetRotateInfo()
            {
                return RotateInfo;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource's maximum remaining list length
            /// @return    maximum remaining list length
            /////////////////////////////////////////////////////////////////////////////////////
            uint32_t GMM_STDCALL GetMaximumRenamingListLength()
            {
                return Surf.MaximumRenamingListLength;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource's compressions block width
            /// @return    Compression block width
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetCompressionBlockWidth()
            {
                GMM_RESOURCE_FORMAT Format;
                Format = Surf.Format;

                __GMM_ASSERT((Format > GMM_FORMAT_INVALID) &&
                              (Format < GMM_RESOURCE_FORMATS));

                return pGmmGlobalContext->GetPlatformInfo().FormatTable[Format].Element.Width;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource's compressions block height
            /// @return    Compression block width
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetCompressionBlockHeight()
            {
                GMM_RESOURCE_FORMAT Format;
                Format = Surf.Format;

                __GMM_ASSERT((Format > GMM_FORMAT_INVALID) &&
                              (Format < GMM_RESOURCE_FORMATS));

                return pGmmGlobalContext->GetPlatformInfo().FormatTable[Format].Element.Height;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource's compressions block depth
            /// @return    Compression block width
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetCompressionBlockDepth()
            {
                GMM_RESOURCE_FORMAT Format;
                Format = Surf.Format;

                __GMM_ASSERT((Format > GMM_FORMAT_INVALID) &&
                              (Format < GMM_RESOURCE_FORMATS));

                return pGmmGlobalContext->GetPlatformInfo().FormatTable[Format].Element.Depth;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the auxiliary resource's QPitch
            /// @return    Aux QPitch
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetAuxQPitch()
            {
                if (Surf.Flags.Gpu.UnifiedAuxSurface)
                {
                    if (GmmIsPlanar(Surf.Format))
                    {
                        return static_cast<uint32_t>(AuxSurf.OffsetInfo.Plane.ArrayQPitch);
                    }
                    else
                    {
                        return AuxSurf.Alignment.QPitch;
                    }
                }
                else
                {
                    return GetQPitch();
                }
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the planar resource's QPitch
            /// @return    planar QPitch
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetQPitchPlanar(GMM_YUV_PLANE Plane)
            {
                uint32_t               QPitch;
                const GMM_PLATFORM_INFO   *pPlatform;

                __GMM_ASSERT(GmmIsPlanar(Surf.Format));

                pPlatform = GMM_OVERRIDE_PLATFORM_INFO(&Surf);
                __GMM_ASSERT(GFX_GET_CURRENT_RENDERCORE(pPlatform->Platform) >= IGFX_GEN8_CORE);

                if (Surf.Flags.Info.YUVShaderFriendlyLayout)
                {
                    switch (Plane)
                    {
                        case GMM_PLANE_Y:
                            QPitch =
                                static_cast<uint32_t>(Surf.OffsetInfo.Plane.Y[GMM_PLANE_3D_Y1] - Surf.OffsetInfo.Plane.Y[GMM_PLANE_3D_Y0]);
                            break;

                        case GMM_PLANE_U:
                        case GMM_PLANE_V:
                            QPitch =
                                static_cast<uint32_t>(Surf.OffsetInfo.Plane.Y[GMM_PLANE_3D_UV1] - Surf.OffsetInfo.Plane.Y[GMM_PLANE_3D_UV0]);
                            break;

                        default:
                            __GMM_ASSERT(0);
                            QPitch = 0;
                    }
                }
                else
                {
                    QPitch = static_cast<uint32_t>(Surf.OffsetInfo.Plane.ArrayQPitch);
                }

                return QPitch;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns distance in bytes between array elements (or pseudo-array-elements--e.g. 
            /// cube faces, MSFMT_MSS sample planes).
            /// @return    QPitch
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetQPitchInBytes()
            {
                return Surf.OffsetInfo.Texture2DOffsetInfo.ArrayQPitchRender;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns resource's pitch
            /// @return    Pitch
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetRenderPitch()
            {
                return Surf.Pitch;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns resource's pitch in tiles
            /// @return    Pitch in tiles
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetRenderPitchTiles()
            {
                uint32_t               PitchInTiles;
                const GMM_PLATFORM_INFO   *pPlatform;
                GMM_TILE_MODE       TileMode;

                __GMM_ASSERT(!Surf.Flags.Info.Linear);

                TileMode = Surf.TileMode;
                __GMM_ASSERT(TileMode < GMM_TILE_MODES);

                pPlatform = GMM_OVERRIDE_PLATFORM_INFO(&Surf);
                if (pPlatform->TileInfo[TileMode].LogicalTileWidth != 0)
                {
                    // In case of Depth/Stencil buffer MSAA TileYs surface, the LogicalTileWidth/Height is smaller than non-MSAA ones
                    // Thus introducting the below variable to get the right PitchInTiles
                    uint32_t MSAASpecialFactorForDepthAndStencil = 1;

                    if ((Surf.Flags.Gpu.Depth || Surf.Flags.Gpu.SeparateStencil) &&
                         (Surf.MSAA.NumSamples > 1 && (Surf.Flags.Info.TiledYs || Surf.Flags.Info.TiledYf)))
                    {
                        switch (Surf.MSAA.NumSamples)
                        {
                            case 2:
                            case 4:
                                MSAASpecialFactorForDepthAndStencil = 2;
                                break;
                            case 8:
                            case 16:
                                MSAASpecialFactorForDepthAndStencil = 4;
                                break;
                            default:
                                break;
                        }
                    }

                    PitchInTiles = static_cast<uint32_t>(Surf.Pitch / pPlatform->TileInfo[TileMode].LogicalTileWidth);
                    PitchInTiles /= MSAASpecialFactorForDepthAndStencil;
                }
                else
                {
                    // Surf.TileMode not set correctly
                    __GMM_ASSERT(FALSE);
                    PitchInTiles = 0;
                }

                return PitchInTiles;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns unified auxiliary resource's pitch in tiles
            /// @return    Aux Pitch in bytes
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetUnifiedAuxPitch()
            {
                return AuxSurf.Pitch;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns auxiliary resource's pitch in tiles
            /// @return    Aux Pitch in tiles
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetRenderAuxPitchTiles()
            {
                uint32_t               PitchInTiles = 0;
                const GMM_PLATFORM_INFO   *pPlatform;

                __GMM_ASSERT(!AuxSurf.Flags.Info.Linear);

                pPlatform = GMM_OVERRIDE_PLATFORM_INFO(&AuxSurf);

                if (Surf.Flags.Gpu.UnifiedAuxSurface)
                {
                    const GMM_TILE_MODE TileMode = AuxSurf.TileMode;
                    __GMM_ASSERT(TileMode < GMM_TILE_MODES);

                    if (pPlatform->TileInfo[TileMode].LogicalTileWidth)
                    {
                        PitchInTiles = static_cast<uint32_t>(AuxSurf.Pitch / pPlatform->TileInfo[TileMode].LogicalTileWidth);
                    }
                }
                else
                {
                    PitchInTiles = GetRenderPitchTiles();
                }

                return PitchInTiles;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns resource's bits per pixel
            /// @return    bpp
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetBitsPerPixel()
            {
                return Surf.BitsPerPixel;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns unified aux resource's bits per pixel
            /// @return    aux bpp
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetUnifiedAuxBitsPerPixel()
            {
                __GMM_ASSERT(Surf.Flags.Gpu.UnifiedAuxSurface);
                return AuxSurf.BitsPerPixel;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns layout of the mips: right or below.
            /// @return    ::GMM_TEXTURE_LAYOUT
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_TEXTURE_LAYOUT GMM_STDCALL GetTextureLayout()
            {
                return Surf.Flags.Info.LayoutRight? GMM_2D_LAYOUT_RIGHT : GMM_2D_LAYOUT_BELOW;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns resource's tile type
            /// @return    ::GMM_TILE_TYPE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_TILE_TYPE GMM_STDCALL GetTileType()
            {
                if (Surf.Flags.Info.TiledW) 
                {
                    return GMM_TILED_W;
                }
                else if (Surf.Flags.Info.TiledX)
                {
                    return GMM_TILED_X;
                }
                // Surf.Flags.Info.TiledYs/Yf tiling are only in
                // conjunction with Surf.Flags.Info.TiledY/Linear depending on resource type (1D).
                else if (Surf.Flags.Info.TiledY)
                {
                    return GMM_TILED_Y;
                }
                
                return GMM_NOT_TILED;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns resource's tile mode
            /// @return    ::GMM_TILE_MODE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_TILE_MODE GMM_STDCALL GmmGetTileMode()
            {
                return Surf.TileMode;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Return the logical width of mip level
            /// @param[in] MipLevel: Mip level for which the info is needed
            /// @return    Mip width
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetMipWidth(uint32_t MipLevel)
            {
                return __GmmTexGetMipWidth(&Surf, MipLevel);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Return the logical height of mip level
            /// @param[in] MipLevel: Mip level for which the info is needed
            /// @return    Mip width
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetMipHeight(uint32_t MipLevel)
            {
                return __GmmTexGetMipHeight(&Surf, MipLevel);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Return the logical depth of mip level
            /// @param[in] MipLevel Mip level for which the info is needed
            /// @return    Mip width
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetMipDepth(uint32_t MipLevel)
            {
                return __GmmTexGetMipDepth(&Surf, MipLevel);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns CPU cacheability information
            /// @return    ::GMM_CPU_CACHE_TYPE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_CPU_CACHE_TYPE GMM_STDCALL GetCpuCacheType()
            {
                if (Surf.Flags.Info.Cacheable)
                {
                    return GMM_CACHEABLE;
                }

                return GMM_NOTCACHEABLE;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns Media Memory Compression mode.
            /// @param[in] ArrayIndex ArrayIndex for which this info is needed
            /// @return    Media Memory Compression Mode (Disabled, Horizontal, Vertical)
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_RESOURCE_MMC_INFO GMM_STDCALL GetMmcMode(uint32_t ArrayIndex)
            {
                __GMM_ASSERT(ArrayIndex < GMM_MAX_MMC_INDEX);

                return
                    (ArrayIndex < GMM_MAX_MMC_INDEX) ?
                        (GMM_RESOURCE_MMC_INFO)Surf.MmcMode[ArrayIndex] : 
                            GMM_MMC_DISABLED;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Sets Media Memory Compression mode.
            /// @param[in] Mode Media Memory Compression Mode (Disabled, Horizontal, Vertical)
            /// @param[in] ArrayIndex ArrayIndex for which this info needs to be set
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL SetMmcMode(GMM_RESOURCE_MMC_INFO Mode, uint32_t ArrayIndex)
            {
                __GMM_ASSERT((Mode == GMM_MMC_DISABLED) || (Mode == GMM_MMC_HORIZONTAL) || (Mode == GMM_MMC_VERTICAL));
                __GMM_ASSERT(ArrayIndex < GMM_MAX_MMC_INDEX);

                if (ArrayIndex < GMM_MAX_MMC_INDEX)
                {
                    Surf.MmcMode[ArrayIndex] = static_cast<uint8_t>(Mode);
                }
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns whether Media Memory Compression enabled or not.
            /// @param[in]  ArrayIndex ArrayIndex for which this info is needed
            /// @return     TRUE (enabled), FALSE (disabled) 
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE BOOLEAN GMM_STDCALL IsMediaMemoryCompressed(uint32_t ArrayIndex)
            {
                __GMM_ASSERT(ArrayIndex < GMM_MAX_MMC_INDEX);

                return
                    (ArrayIndex < GMM_MAX_MMC_INDEX) ?
                        Surf.MmcMode[ArrayIndex] != GMM_MMC_DISABLED : 
                            FALSE;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns mmc hints.
            /// @param[in]  ArrayIndex ArrayIndex for which this info is needed
            /// @return     TRUE/FALSE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_RESOURCE_MMC_HINT GMM_STDCALL GetMmcHint(uint32_t ArrayIndex)
            {
                __GMM_ASSERT(ArrayIndex < GMM_MAX_MMC_INDEX);
                return Surf.MmcHint[ArrayIndex] ? GMM_MMC_HINT_OFF : GMM_MMC_HINT_ON;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Sets mmc hints.
            /// @param[in]  Hint Mmc hint to store
            /// @param[in]  ArrayIndex ArrayIndex for which this info is needed
            /// @return     TRUE/FALSE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL SetMmcHint(GMM_RESOURCE_MMC_HINT Hint, uint32_t ArrayIndex)
            {
                __GMM_ASSERT(ArrayIndex < GMM_MAX_MMC_INDEX);
                __GMM_ASSERT(GMM_MMC_HINT_ON == 0);
                __GMM_ASSERT(GMM_MMC_HINT_OFF == 1);

                Surf.MmcHint[ArrayIndex] = static_cast<UCHAR>(Hint);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the MSAA Sample Counter
            /// @return     Sample count
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetNumSamples()
            {
                return Surf.MSAA.NumSamples;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the MSAA Sample Pattern
            /// @return     Sample pattern
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_MSAA_SAMPLE_PATTERN GMM_STDCALL GetSamplePattern()
            {
                return Surf.MSAA.SamplePattern;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the X offset of planar surface
            /// @param[in]  Plane: Plane for which the offset is needed
            /// @return     X offset
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetPlanarXOffset(GMM_YUV_PLANE Plane)
            {
                __GMM_ASSERT(Plane < GMM_MAX_PLANE);
                return Surf.OffsetInfo.Plane.X[Plane];
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the Y offset of planar surface
            /// @param[in]  Plane: Plane for which the offset is needed
            /// @return     Y offset
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetPlanarYOffset(GMM_YUV_PLANE Plane)
            {
                __GMM_ASSERT(Plane < GMM_MAX_PLANE);
                return Surf.OffsetInfo.Plane.Y[Plane];
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the Aux offset of planar surface
            /// @param[in]  ArrayIndex: Surf index for which aux offset is required
            /// @param[in]  GmmAuxType: Aux Plane for which the offset is needed
            /// @return     Y_CCS offset/ UV_CCS offset/ Media compression state
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetPlanarAuxOffset(uint32_t ArrayIndex, GMM_UNIFIED_AUX_TYPE GmmAuxType)
            {
                GMM_GFX_SIZE_T Offset = 0;
                
                __GMM_ASSERT(ArrayIndex < Surf.ArraySize);
                __GMM_ASSERT(GmmIsPlanar(Surf.Format));

                if (Surf.Flags.Gpu.UnifiedAuxSurface)
                {
                    if (GmmAuxType == GMM_AUX_Y_CCS)
                    {
                        Offset = Surf.Size;
                    }
                    else if (GmmAuxType == GMM_AUX_UV_CCS)
                    {
                        Offset = Surf.Size + (AuxSurf.Pitch * AuxSurf.OffsetInfo.Plane.Y[GMM_PLANE_U]); //Aux Offset in HwLayout

                        if (Surf.Flags.Gpu.CCS && AuxSurf.Flags.Gpu.__NonMsaaLinearCCS) //ie Linear CCS
                        {
                            Offset = Surf.Size + AuxSurf.OffsetInfo.Plane.X[GMM_PLANE_U];
                        }
                        else if (Surf.Flags.Gpu.MMC && AuxSurf.Flags.Gpu.__NonMsaaLinearCCS ) //ie Linear CCS
                        {
                            Offset = Surf.Size + AuxSurf.OffsetInfo.Plane.X[GMM_PLANE_Y];
                        }
                    }
                    else if (GmmAuxType == GMM_AUX_COMP_STATE)
                    {
                        Offset = Surf.Size + AuxSurf.OffsetInfo.Plane.X[GMM_PLANE_Y] + AuxSurf.OffsetInfo.Plane.X[GMM_PLANE_U];
                    }

                    Offset += AuxSurf.OffsetInfo.Plane.ArrayQPitch * ArrayIndex;
                }
                else
                {
                    Offset = 0;
                }

                return Offset;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource Horizontal alignment
            /// @return     HAlign
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetHAlign()
            {
                const __GMM_PLATFORM_RESOURCE   *pPlatformResource;
                uint32_t HAlign;
                pPlatformResource = GMM_OVERRIDE_PLATFORM_INFO(&Surf);

                if ((GFX_GET_CURRENT_RENDERCORE(pPlatformResource->Platform) >= IGFX_GEN9_CORE) &&
                    !(Surf.Flags.Info.TiledYf || Surf.Flags.Info.TiledYs))
                {
                    HAlign = Surf.Alignment.HAlign / GetCompressionBlockWidth();
                }
                else
                {
                    HAlign = Surf.Alignment.HAlign;
                }

                return HAlign;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource Vertical alignment
            /// @return     VAlign
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetVAlign()
            {
                const __GMM_PLATFORM_RESOURCE   *pPlatformResource;
                uint32_t VAlign;
                pPlatformResource = GMM_OVERRIDE_PLATFORM_INFO(&Surf);

                if ((GFX_GET_CURRENT_RENDERCORE(pPlatformResource->Platform) >= IGFX_GEN9_CORE) &&
                    !(GetResFlags().Info.TiledYf || GetResFlags().Info.TiledYs))
                {
                    VAlign = Surf.Alignment.VAlign / GetCompressionBlockHeight();
                }
                else
                {
                    VAlign = Surf.Alignment.VAlign;
                }

                return VAlign;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the auxiliary resource Horizontal alignment
            /// @return     HAlign
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetAuxHAlign()
            {
                if (Surf.Flags.Gpu.UnifiedAuxSurface)
                {
                    return AuxSurf.Alignment.HAlign;
                }
                else
                {
                    return GetHAlign();
                }
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the auxiliary resource Vertical alignment
            /// @return     HAlign
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetAuxVAlign()
            {
                if (Surf.Flags.Gpu.UnifiedAuxSurface)
                {
                    return AuxSurf.Alignment.VAlign;
                }
                else
                {
                    return GetVAlign();
                }
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns whether resource uses LOD0-only or Full array spacing
            /// @return     TRUE/FALSE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE BOOLEAN GMM_STDCALL IsArraySpacingSingleLod()
            {
                __GMM_ASSERT(GFX_GET_CURRENT_RENDERCORE(pGmmGlobalContext->GetPlatformInfo().Platform) < IGFX_GEN8_CORE);
                return Surf.Alignment.ArraySpacingSingleLod;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns whether resource is ASTC
            /// @return     TRUE/FALSE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE BOOLEAN GMM_STDCALL IsASTC()
            {
                GMM_RESOURCE_FORMAT Format;
                Format = Surf.Format;

                return
                    (Format > GMM_FORMAT_INVALID) &&
                    (Format < GMM_RESOURCE_FORMATS) &&
                    pGmmGlobalContext->GetPlatformInfo().FormatTable[Format].ASTC;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns indication of whether resource uses the MSFMT_DEPTH_STENCIL Multisampled 
            /// Surface Storage Format.
            /// @return     TRUE/FALSE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE BOOLEAN GMM_STDCALL IsMsaaFormatDepthStencil()
            {
                // Gen7 MSAA (non-Depth/Stencil) render targets use (MSFMT_DEPTH_MSS) array
                // expansion instead of (MSFMT_DEPTH_STENCIL) Width/Height expansion.
                return (Surf.MSAA.NumSamples > 1) &&
                        (Surf.Flags.Gpu.Depth ||
                        Surf.Flags.Gpu.SeparateStencil);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns indication of whether resource is SVM or not
            /// @return     TRUE/FALSE
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE BOOLEAN GMM_STDCALL IsSvm()
            {
                return static_cast<BOOLEAN>(Surf.Flags.Info.SVM);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Allows clients to attach a private data to the resource
            /// @param[in]  pNewPrivateData: pointer to opaque private data from clients
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL SetPrivateData(void *pNewPrivateData)
            {
                this->pPrivateData = reinterpret_cast<GMM_VOIDPTR64>(pNewPrivateData);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns private data attached to the resource
            /// @return     Pointer to opaque private data
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void* GMM_STDCALL GetPrivateData()
            {
                return reinterpret_cast<void*>(pPrivateData);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the resource GFX address
            /// @return     Gfx Address
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_ADDRESS GMM_STDCALL GetGfxAddress()
            {
                // Currently only supports Isolated GFX Space cases.
                // Support for Sparse/Tiled resources will be unified in later
                if (SvmAddress)
                {
                    return GMM_GFX_ADDRESS_CANONIZE(SvmAddress);
                }
                else
                {
                    return GMM_GFX_ADDRESS_CANONIZE(IsolatedGfxAddress);
                }
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// This function returns the total height of an S3D tall buffer. For non-S3D
            /// resources, it returns base height.
            /// @return     Surface height
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetTallBufferHeight()
            {
                if (Surf.Flags.Gpu.S3d)
                {
                    return Surf.S3d.TallBufferHeight;
                }
                else
                {
                    GMM_ASSERTDPF(0, "Unsupported S3D Resource Type!");
                    return Surf.BaseHeight;
                }
            };

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns size of the main surface only. Aux surface size not included.
            /// @return     Size of main surface
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T  GMM_STDCALL GetSizeMainSurface() const
            {
                return Surf.Size;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the number of bytes that are required to back this padded and aligned 
            /// resource. The calculation takes into consideration more than simply width
            /// height and bits per pixel. Width padding (stride), pixel formats, inter-plane
            /// padding depts/array-size and so on also for part of the list of factors.
            /// @return     Surface Size
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T  GMM_STDCALL GetSizeSurface()
            {
                    return (Surf.Size + AuxSurf.Size + AuxSecSurf.Size);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns surface size(GetSizeSurface) plus additional padding due to 64kb pages
            /// @return     Allocation Size
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T  GMM_STDCALL GetSizeAllocation()
            {
                #define ALIGN_SIZE(x, a)  (((x) + ((a) - 1)) - (((x) + ((a) - 1)) & ((a) - 1)))
                if (Is64KBPageSuitable())
                {
                    return(ALIGN_SIZE(Surf.Size + AuxSurf.Size + AuxSecSurf.Size, GMM_KBYTE(64)));
                }
                else
                {
                    return (Surf.Size + AuxSurf.Size + AuxSecSurf.Size);
                }
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns max no of GpuVa bits supported per resource on a given platform
            /// @return     Max # of GpuVA bits per resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t  GMM_STDCALL GetMaxGpuVirtualAddressBits()
            {
                const GMM_PLATFORM_INFO *pPlatform = GMM_OVERRIDE_PLATFORM_INFO(&Surf);
                __GMM_ASSERTPTR(pPlatform, 0);

                return pPlatform->MaxGpuVirtualAddressBitsPerResource;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the surface offset for unified allocations
            /// @param[in]  GmmAuxType: the type of aux the offset is needed for
            /// @return     Surface Offset in bytes
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetUnifiedAuxSurfaceOffset(GMM_UNIFIED_AUX_TYPE GmmAuxType)
            {
                GMM_GFX_SIZE_T Offset = 0;
                const GMM_PLATFORM_INFO *pPlatform;
                pPlatform = GMM_OVERRIDE_PLATFORM_INFO(&Surf);
                if (Surf.Flags.Gpu.UnifiedAuxSurface)
                {
                    if ((GmmAuxType == GMM_AUX_CCS) || (GmmAuxType == GMM_AUX_SURF) || (GmmAuxType == GMM_AUX_Y_CCS)
                        || (GmmAuxType == GMM_AUX_HIZ) || (GmmAuxType == GMM_AUX_MCS))
                    {
                        Offset = Surf.Size;
                        if (GmmAuxType == GMM_AUX_CCS && AuxSecSurf.Type != RESOURCE_INVALID
                            && (Surf.Flags.Gpu.CCS && (Surf.MSAA.NumSamples > 1 ||
                                Surf.Flags.Gpu.Depth)))
                        {
                            Offset += AuxSurf.Size;
                        }
                    }
                    else if (GmmAuxType == GMM_AUX_UV_CCS)
                    {
                        Offset = Surf.Size + (AuxSurf.Pitch * AuxSurf.OffsetInfo.Plane.Y[GMM_PLANE_U]); //Aux Offset in HwLayout

                        if (Surf.Flags.Gpu.CCS && AuxSurf.Flags.Gpu.__NonMsaaLinearCCS) //ie Linear CCS
                        {
                            Offset = Surf.Size + AuxSurf.OffsetInfo.Plane.X[GMM_PLANE_U];
                        }
                        else if (Surf.Flags.Gpu.MMC && AuxSurf.Flags.Gpu.__NonMsaaLinearCCS ) //ie Linear CCS
                        {
                            Offset = Surf.Size + AuxSurf.OffsetInfo.Plane.X[GMM_PLANE_Y];
                        }
                    }
                    else if ((GmmAuxType == GMM_AUX_CC) && Surf.Flags.Gpu.IndirectClearColor)
                    {
                        Offset = Surf.Size + AuxSurf.UnpaddedSize;
                    }
                    else if (GmmAuxType == GMM_AUX_COMP_STATE)
                    {
                        Offset = Surf.Size + AuxSurf.OffsetInfo.Plane.X[GMM_PLANE_Y] + AuxSurf.OffsetInfo.Plane.X[GMM_PLANE_U];
                    }
                    else if ((GmmAuxType == GMM_AUX_MCS_LCE && (Surf.MSAA.NumSamples > 1 && Surf.Flags.Gpu.CCS)) ||
                        (GmmAuxType == GMM_AUX_ZCS && (Surf.Flags.Gpu.Depth && Surf.Flags.Gpu.CCS)))
                    {
                        if (AuxSecSurf.Type != RESOURCE_INVALID)
                        {
                            Offset = Surf.Size + AuxSurf.Size;
                        }
                    }
                }
                else if(GmmAuxType == GMM_AUX_CC && 
                        Surf.Flags.Gpu.IndirectClearColor &&
                        Surf.Flags.Gpu.HiZ)
                {
                    Offset = Surf.Size - GMM_HIZ_CLEAR_COLOR_SIZE;
                }
                else
                {
                    Offset = 0;
                }

                return Offset;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the surface size for unified allocations
            /// @param[in]  GmmAuxType: the type of aux the size is needed for
            /// @return     Surface Size in bytes
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetSizeAuxSurface(GMM_UNIFIED_AUX_TYPE GmmAuxType)
            {
                if (GmmAuxType == GMM_AUX_SURF)
                {
                    return (AuxSurf.Size + AuxSecSurf.Size);
                }
                else if (GmmAuxType == GMM_AUX_CCS || GmmAuxType == GMM_AUX_HIZ || GmmAuxType == GMM_AUX_MCS)
                {
                    if (GmmAuxType == GMM_AUX_CCS && AuxSecSurf.Type != RESOURCE_INVALID &&
                        (Surf.Flags.Gpu.CCS && (Surf.MSAA.NumSamples > 1 ||
                            Surf.Flags.Gpu.Depth)))
                    {
                        return AuxSecSurf.Size;
                    }
                    else
                    {
                        return (AuxSurf.UnpaddedSize);
                    }
                }
                else if (GmmAuxType == GMM_AUX_COMP_STATE)
                {
                    return GMM_MEDIA_COMPRESSION_STATE_SIZE;
                }
                else if (GmmAuxType == GMM_AUX_CC)
                {
                    if (!Surf.Flags.Gpu.UnifiedAuxSurface && Surf.Flags.Gpu.HiZ)
                    {
                        return GMM_HIZ_CLEAR_COLOR_SIZE;
                    }
                    else
                    {
                        return (AuxSurf.CCSize);
                    }
                }
                else if (GmmAuxType == GMM_AUX_MCS_LCE || GmmAuxType == GMM_AUX_ZCS)
                {
                    if (Surf.Flags.Gpu.UnifiedAuxSurface && AuxSecSurf.Type != RESOURCE_INVALID)
                    {
                        return AuxSecSurf.Size;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return 0;
                }
            }

            /////////////////////////////////////////////////////////////////////////
            /// This function returns or sets the value of the hardware protected flag 
            /// associated with the given GMM resource within same process.
            /// @param[in]  GetIsEncrypted: Read encryption status
            /// @param[in]  SetIsEncrypted: Write encryption status
            /// @return     Whether surface is encrypted or not
            /////////////////////////////////////////////////////////////////////////
            virtual GMM_INLINE BOOLEAN GMM_STDCALL GetSetHardwareProtection(BOOLEAN GetIsEncrypted, BOOLEAN SetIsEncrypted)
            {
                BOOLEAN IsEncrypted = FALSE;

                if (GetIsEncrypted)
                {
                    IsEncrypted = Surf.Flags.Info.HardwareProtected;
                }
                else
                {
                    Surf.Flags.Info.HardwareProtected = IsEncrypted = SetIsEncrypted;
                }

                return IsEncrypted;
            }

            /////////////////////////////////////////////////////////////////////////
            /// Returns the size of the surface in StdLayout format
            /// @return  Size in bytes of Standard Layout version of surface.   
            /////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_GFX_SIZE_T GMM_STDCALL GetStdLayoutSize() 
            {
                GMM_REQ_OFFSET_INFO GetOffset = {};

                GetOffset.ReqStdLayout = TRUE;
                GetOffset.StdLayout.Offset = static_cast<GMM_GFX_SIZE_T>(-1); // Special Req for StdLayout Size
                this->GetOffset(GetOffset);

                return GetOffset.StdLayout.Offset;
            }

            /////////////////////////////////////////////////////////////////////////
            /// Returns whether resource is color separated target
            /// @return  TRUE if the resource is color separated target, FALSE otherwise   
            /////////////////////////////////////////////////////////////////////////
            GMM_INLINE BOOLEAN GMM_STDCALL IsColorSeparation()
            {
                return Surf.Flags.Gpu.ColorSeparation || Surf.Flags.Gpu.ColorSeparationRGBX;
            }

            /////////////////////////////////////////////////////////////////////////
            /// Translate packed source x coordinate to color separation target x coordinate
            /// @param[in]  x: X coordinate
            /// @return   Translated color separation target x coordinate  
            /////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL TranslateColorSeparationX(uint32_t x)
            {
                uint32_t ret = x;

                if (Surf.Flags.Gpu.ColorSeparation)
                {
                    ret /= GMM_COLOR_SEPARATION_WIDTH_DIVISION;
                }
                else if (Surf.Flags.Gpu.ColorSeparationRGBX)
                {
                    ret /= GMM_COLOR_SEPARATION_RGBX_WIDTH_DIVISION;
                }

                return ret;
            }

            /////////////////////////////////////////////////////////////////////////
            /// Returns the array size of a color separated target resource.
            /// @return   Array size of a color separated target resource
            /////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetColorSeparationArraySize()
            {
                if (Surf.Flags.Gpu.ColorSeparation ||
                    Surf.Flags.Gpu.ColorSeparationRGBX)
                {
                    return GMM_COLOR_SEPARATION_ARRAY_SIZE;
                }
                else
                {
                    return Surf.ArraySize;
                }
            }

            /////////////////////////////////////////////////////////////////////////
            /// Returns the physical width of a color separated target resource
            /// @return   physical width of a color separated target resource
            /////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetColorSeparationPhysicalWidth()
            {
                if (Surf.Flags.Gpu.ColorSeparation)
                {
                    return ((uint32_t)Surf.BaseWidth * Surf.ArraySize) / GMM_COLOR_SEPARATION_WIDTH_DIVISION;
                }
                else if (Surf.Flags.Gpu.ColorSeparationRGBX)
                {
                    return ((uint32_t)Surf.BaseWidth * Surf.ArraySize) / GMM_COLOR_SEPARATION_RGBX_WIDTH_DIVISION;
                }
                else
                {
                    return (uint32_t)Surf.BaseWidth;
                }
            }

            /////////////////////////////////////////////////////////////////////////
            /// Returns whether surface can be faulted on
            /// @return   TRUE is surface can be faulted on
            /////////////////////////////////////////////////////////////////////////
            virtual GMM_INLINE BOOLEAN GMM_STDCALL IsSurfaceFaultable()
            {
                return FALSE;
            }
     
            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the cache policy usage associated with this surface.
            /// @return     Cache Policy Usage
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_RESOURCE_USAGE_TYPE GMM_STDCALL GetCachePolicyUsage()
            {
                return Surf.CachePolicy.Usage;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns MOCS associated with the resource
            /// @param[in]     MOCS
            /////////////////////////////////////////////////////////////////////////////////////
            MEMORY_OBJECT_CONTROL_STATE GMM_STDCALL GetMOCS()
            {
                const GMM_CACHE_POLICY_ELEMENT *CachePolicy = pGmmGlobalContext->GetCachePolicyUsage();

                __GMM_ASSERT(CachePolicy[GetCachePolicyUsage()].Initialized);

                // Prevent wrong Usage for XAdapter resources. UMD does not call GetMemoryObject on shader resources but,
                // when they add it someone could call it without knowing the restriction.
                if (Surf.Flags.Info.XAdapter &&
                    GetCachePolicyUsage() != GMM_RESOURCE_USAGE_XADAPTER_SHARED_RESOURCE)
                {
                    __GMM_ASSERT(FALSE);
                }

                if ((CachePolicy[GetCachePolicyUsage()].Override & CachePolicy[GetCachePolicyUsage()].IDCode) ||
                    (CachePolicy[GetCachePolicyUsage()].Override == ALWAYS_OVERRIDE))
                {
                    return CachePolicy[GetCachePolicyUsage()].MemoryObjectOverride;
                }

                return CachePolicy[GetCachePolicyUsage()].MemoryObjectNoOverride;
            }


            //##################################################################################
            // Functions that can help clients program the SURFACE_STATE with appropriate values.
            //##################################################################################
            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the surface state value for Mip Tail Start LOD
            /// @return     Mip Tail Start
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetMipTailStartLodSurfaceState()
            {
                return Surf.Alignment.MipTailStartLod;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the Tile Address Mapping Mode, for SURFACE_STATE programming and is 
            /// applicable only for 3D surface
            /// @return     Tile Address Mapping Mode
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetTileAddressMappingModeSurfaceState()
            {
                return 0;
            }


            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the surface state value for Standard Tiling Mode Extension 
            /// @return     Standard Tiling Mode Extension
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetStdTilingModeExtSurfaceState()
            {
                const GMM_PLATFORM_INFO *pPlatform = GMM_OVERRIDE_PLATFORM_INFO(&Surf);
                GMM_UNREFERENCED_LOCAL_VARIABLE(pPlatform); // Only used for debug
                __GMM_ASSERT(GFX_GET_CURRENT_RENDERCORE(pPlatform->Platform) > IGFX_GEN10_CORE);

                if (pGmmGlobalContext->GetSkuTable().FtrStandardMipTailFormat)
                {
                    return 1;
                }

                return 0;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the surface state value for Resource Format
            /// @return     Resource Format
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE GMM_SURFACESTATE_FORMAT GMM_STDCALL GetResourceFormatSurfaceState() 
            {
                GMM_RESOURCE_FORMAT Format;

                Format = Surf.Format;
                __GMM_ASSERT((Format > GMM_FORMAT_INVALID) && (Format < GMM_RESOURCE_FORMATS));

                return pGmmGlobalContext->GetPlatformInfo().FormatTable[Format].SurfaceStateFormat;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the horizontal alignment for SURFACE_STATE programming.
            /// @return     HAlign
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetHAlignSurfaceState()
            {
                uint32_t               HAlign;
                const GMM_PLATFORM_INFO   *pPlatform;

                pPlatform = GMM_OVERRIDE_PLATFORM_INFO(&Surf);

                if (GFX_GET_CURRENT_RENDERCORE(pPlatform->Platform) >= IGFX_GEN8_CORE)
                {
                    if (GetResFlags().Info.TiledYf || GetResFlags().Info.TiledYs)
                    {
                        HAlign = 0;
                    }
                    else
                    {
                        switch (GetHAlign())
                        {
                            case 4:  HAlign = 1; break;
                            case 8:  HAlign = 2; break;
                            case 16: HAlign = 3; break;
                            default: HAlign = 0; __GMM_ASSERT(0);
                        }
                    }
                }
                else
                {
                    switch (Surf.Alignment.HAlign)
                    {
                        case 4:  HAlign = 0; break;
                        case 8:  HAlign = 1; break;
                        default: HAlign = 0; __GMM_ASSERT(0);
                    }
                }

                return HAlign;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns the vertical alignment for SURFACE_STATE programming.
            /// @return     HAlign
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetVAlignSurfaceState()
            {
                uint32_t               VAlign;
                const GMM_PLATFORM_INFO   *pPlatform;

                pPlatform = GMM_OVERRIDE_PLATFORM_INFO(&Surf);

                if (GFX_GET_CURRENT_RENDERCORE(pPlatform->Platform) >= IGFX_GEN8_CORE)
                {
                    if (GetResFlags().Info.TiledYf || GetResFlags().Info.TiledYs)
                    {
                        VAlign = 0;
                    }
                    else
                    {
                        switch (GetVAlign())
                        {
                            case 4:  VAlign = 1; break;
                            case 8:  VAlign = 2; break;
                            case 16: VAlign = 3; break;
                            default: VAlign = 0; __GMM_ASSERT(0);
                        }
                    }
                }
                else
                {
                    switch (Surf.Alignment.VAlign)
                    {
                        case 2:  VAlign = 0; break;
                        case 4:  VAlign = 1; break;
                        default: VAlign = 0; __GMM_ASSERT(0);
                    }
                }

                return VAlign;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Returns tiled resource mode for SURFACE_STATE programming.
            /// @return     Tiled Resource Mode
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE uint32_t GMM_STDCALL GetTiledResourceModeSurfaceState()
            {
                uint32_t   TiledResourceMode;

                if (Surf.Flags.Info.TiledYf)
                {
                    TiledResourceMode = 1;
                }
                else if (Surf.Flags.Info.TiledYs)
                {
                    TiledResourceMode = 2;
                }
                else
                {
                    TiledResourceMode = 0;
                }

                return TiledResourceMode;
            }


            //###################################################################################
            // Functions that allows clients to override certain members
            // of ResourceInfo. Client assumes the risk of using these functions.
            // May cause unintended side-affects.
            //##################################################################################
            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the main surface size
            /// @param[in]  Size: new size of the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideSize(GMM_GFX_SIZE_T Size)
            {
                Surf.Size = Size;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the surface pitch
            /// @param[in]  Pitch: new pitch of the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverridePitch(GMM_GFX_SIZE_T Pitch)
            {
                Surf.Pitch = Pitch;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the aux surface pitch
            /// @param[in]  Pitch: new pitch of the aux surface
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideUnifiedAuxPitch(GMM_GFX_SIZE_T Pitch)
            {
                __GMM_ASSERT(Surf.Flags.Gpu.UnifiedAuxSurface);
                AuxSurf.Pitch = Pitch;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the allocation flags
            /// @param[in]  Flags: new set of flags for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideAllocationFlags(GMM_RESOURCE_FLAG& Flags)
            {
                Surf.Flags = Flags;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource HAlign
            /// @param[in]  HAlign: new HAlign for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideHAlign(uint32_t HAlign)
            {
                Surf.Alignment.HAlign = HAlign;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource BaseAlignment
            /// @param[in]  Alignment: new BaseAlignment for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideBaseAlignment(uint32_t Alignment)
            {
                Surf.Alignment.BaseAlignment = Alignment;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource BaseWidth
            /// @param[in]  BaseWidth: new BaseWidth for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideBaseWidth(GMM_GFX_SIZE_T BaseWidth)
            {
                Surf.BaseWidth = BaseWidth;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource BaseHeight
            /// @param[in]  BaseHeight: new BaseHeight for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideBaseHeight(uint32_t BaseHeight)
            {
                Surf.BaseHeight = BaseHeight;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource Depth
            /// @param[in]  Depth: new Depth for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideDepth(uint32_t Depth)
            {
                Surf.Depth = Depth;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource tile mode
            /// @param[in]  TileMode: new tile mode for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideTileMode(GMM_TILE_MODE TileMode)
            {
                Surf.TileMode = TileMode;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource tile mode
            /// @param[in]  TileMode: new tile mode for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideUnifiedAuxTileMode(GMM_TILE_MODE TileMode)
            {
                __GMM_ASSERT(Surf.Flags.Gpu.UnifiedAuxSurface);
                AuxSurf.TileMode = TileMode;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the surface format
            /// @param[in]  Format: new format for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideSurfaceFormat(GMM_RESOURCE_FORMAT Format)
            {
                Surf.Format = Format;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the surface type
            /// @param[in]  Type: new surface type for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideSurfaceType(GMM_RESOURCE_TYPE Type)
            {
                Surf.Type = Type;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the isolated gfx address
            /// @param[in]  NewIsolatedGfxAddress: new isolated gfx address for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideIsolatedGfxAddress(GMM_GFX_ADDRESS NewIsolatedGfxAddress)
            {
                this->IsolatedGfxAddress = NewIsolatedGfxAddress;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the svm gfx address
            /// @param[in]  SvmGfxAddress: new svm gfx address for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideSvmGfxAddress(GMM_GFX_ADDRESS SvmGfxAddress)
            {
                this->SvmAddress = SvmGfxAddress;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource array size
            /// @param[in]  ArraySize: new array size for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideArraySize(uint32_t ArraySize)
            {
                Surf.ArraySize = ArraySize;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource max LOD
            /// @param[in]  MaxLod: new max LOD for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideMaxLod(uint32_t MaxLod)
            {
                Surf.MaxLod = MaxLod;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the resource cache policy usage
            /// @param[in]  Usage: new usage for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideCachePolicyUsage(GMM_RESOURCE_USAGE_TYPE Usage)
            {
                Surf.CachePolicy.Usage = Usage;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the platform associated with this resource
            /// @param[in]  Platform: new platform for the resource
            /// @note Function only available for Debug/Release-Internal builds.
            /////////////////////////////////////////////////////////////////////////////////////
            #if(_DEBUG || _RELEASE_INTERNAL)
                GMM_INLINE void GMM_STDCALL OverridePlatform(PLATFORM Platform)
                {
                    Surf.Platform = Platform;
                }
            #endif

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the GmmLibContext associated with this resource
            /// @param[in]  pNewGmmLibContext: new GmmLibContext for the resource
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverrideGmmLibContext(Context *pNewGmmLibContext)
            {
                this->pGmmLibContext = reinterpret_cast<GMM_VOIDPTR64>(pNewGmmLibContext);
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the X offset of planar surface
            /// @param[in]  Plane: Plane for which the offset needs to be overriden
            /// @param[in]  XOffset: X offset
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverridePlanarXOffset(GMM_YUV_PLANE Plane, GMM_GFX_SIZE_T XOffset)
            {
                __GMM_ASSERT(Plane < GMM_MAX_PLANE);
                Surf.OffsetInfo.Plane.X[Plane] = XOffset;
            }

            /////////////////////////////////////////////////////////////////////////////////////
            /// Overrides the Y offset of planar surface
            /// @param[in]  Plane: Plane for which the offset needs to be overriden
            /// @param[in]  YOffset: Y offset
            /////////////////////////////////////////////////////////////////////////////////////
            GMM_INLINE void GMM_STDCALL OverridePlanarYOffset(GMM_YUV_PLANE Plane, GMM_GFX_SIZE_T YOffset)
            {
                __GMM_ASSERT(Plane < GMM_MAX_PLANE);
                Surf.OffsetInfo.Plane.Y[Plane] = YOffset;
            }

    };

} // namespace GmmLib  
#endif // #ifdef __cplusplus
