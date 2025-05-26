#pragma once
//#include "game.hpp"

namespace mods::blackmesa::game
{
	enum PreviewImageRetVal_t
	{
		MATERIAL_PREVIEW_IMAGE_BAD = 0,
		MATERIAL_PREVIEW_IMAGE_OK,
		MATERIAL_NO_PREVIEW_IMAGE,
	};

	enum ImageFormat
	{
		IMAGE_FORMAT_UNKNOWN = -1,
		IMAGE_FORMAT_RGBA8888 = 0,
		IMAGE_FORMAT_ABGR8888,
		IMAGE_FORMAT_RGB888,
		IMAGE_FORMAT_BGR888,
		IMAGE_FORMAT_RGB565,
		IMAGE_FORMAT_I8,
		IMAGE_FORMAT_IA88,
		IMAGE_FORMAT_P8,
		IMAGE_FORMAT_A8,
		IMAGE_FORMAT_RGB888_BLUESCREEN,
		IMAGE_FORMAT_BGR888_BLUESCREEN,
		IMAGE_FORMAT_ARGB8888,
		IMAGE_FORMAT_BGRA8888,
		IMAGE_FORMAT_DXT1,
		IMAGE_FORMAT_DXT3,
		IMAGE_FORMAT_DXT5,
		IMAGE_FORMAT_BGRX8888,
		IMAGE_FORMAT_BGR565,
		IMAGE_FORMAT_BGRX5551,
		IMAGE_FORMAT_BGRA4444,
		IMAGE_FORMAT_DXT1_ONEBITALPHA,
		IMAGE_FORMAT_BGRA5551,
		IMAGE_FORMAT_UV88,
		IMAGE_FORMAT_UVWQ8888,
		IMAGE_FORMAT_RGBA16161616F,
		IMAGE_FORMAT_RGBA16161616,
		IMAGE_FORMAT_UVLX8888,
		IMAGE_FORMAT_R32F,  // Single-channel 32-bit floating point
		IMAGE_FORMAT_RGB323232F,
		IMAGE_FORMAT_RGBA32323232F,

		// Depth-stencil texture formats for shadow depth mapping
		IMAGE_FORMAT_NV_DST16,   //
		IMAGE_FORMAT_NV_DST24,   //
		IMAGE_FORMAT_NV_INTZ,    // Vendor-specific depth-stencil texture
		IMAGE_FORMAT_NV_RAWZ,    // formats for shadow depth mapping
		IMAGE_FORMAT_ATI_DST16,  //
		IMAGE_FORMAT_ATI_DST24,  //
		IMAGE_FORMAT_NV_NULL,    // Dummy format which takes no video memory

		// Compressed normal map formats
		IMAGE_FORMAT_ATI2N,  // One-surface ATI2N / DXN format
		IMAGE_FORMAT_ATI1N,  // Two-surface ATI1N format

		NUM_IMAGE_FORMATS
	};

	enum MaterialVarFlags_t
	{
		MATERIAL_VAR_DEBUG = (1 << 0),
		MATERIAL_VAR_NO_DEBUG_OVERRIDE = (1 << 1),
		MATERIAL_VAR_NO_DRAW = (1 << 2),
		MATERIAL_VAR_USE_IN_FILLRATE_MODE = (1 << 3),

		MATERIAL_VAR_VERTEXCOLOR = (1 << 4),
		MATERIAL_VAR_VERTEXALPHA = (1 << 5),
		MATERIAL_VAR_SELFILLUM = (1 << 6),
		MATERIAL_VAR_ADDITIVE = (1 << 7),
		MATERIAL_VAR_ALPHATEST = (1 << 8),
		MATERIAL_VAR_MULTIPASS = (1 << 9),
		MATERIAL_VAR_ZNEARER = (1 << 10),
		MATERIAL_VAR_MODEL = (1 << 11),
		MATERIAL_VAR_FLAT = (1 << 12),
		MATERIAL_VAR_NOCULL = (1 << 13),
		MATERIAL_VAR_NOFOG = (1 << 14),
		MATERIAL_VAR_IGNOREZ = (1 << 15),
		MATERIAL_VAR_DECAL = (1 << 16),
		MATERIAL_VAR_ENVMAPSPHERE = (1 << 17),
		MATERIAL_VAR_NOALPHAMOD = (1 << 18),
		MATERIAL_VAR_ENVMAPCAMERASPACE = (1 << 19),
		MATERIAL_VAR_BASEALPHAENVMAPMASK = (1 << 20),
		MATERIAL_VAR_TRANSLUCENT = (1 << 21),
		MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK = (1 << 22),
		MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING = (1 << 23),
		MATERIAL_VAR_OPAQUETEXTURE = (1 << 24),
		MATERIAL_VAR_ENVMAPMODE = (1 << 25),
		MATERIAL_VAR_SUPPRESS_DECALS = (1 << 26),
		MATERIAL_VAR_HALFLAMBERT = (1 << 27),
		MATERIAL_VAR_WIREFRAME = (1 << 28),
		MATERIAL_VAR_ALLOWALPHATOCOVERAGE = (1 << 29),
	};

	enum MaterialPropertyTypes_t
	{
		MATERIAL_PROPERTY_NEEDS_LIGHTMAP = 0,  // bool
		MATERIAL_PROPERTY_OPACITY,       // int (enum MaterialPropertyOpacityTypes_t)
		MATERIAL_PROPERTY_REFLECTIVITY,  // vec3_t
		MATERIAL_PROPERTY_NEEDS_BUMPED_LIGHTMAPS  // bool
	};

	enum VertexCompressionType_t
	{
		// This indicates an uninitialized VertexCompressionType_t value
		VERTEX_COMPRESSION_INVALID = 0xFFFFFFFF,
		VERTEX_COMPRESSION_NONE = 0,
		VERTEX_COMPRESSION_ON = 1
	};

	struct QAngle
	{
		float x;
		float y;
		float z;
	};

	struct CUtlSymbol
	{
		unsigned __int16 m_Id;
	};

	struct IMaterialVar_vtbl;
	struct IMaterialVar
	{
		IMaterialVar_vtbl* vftable;
		char* m_pStringVal;
		int m_intVal;
		Vector4D m_VecVal;
		unsigned __int8 m_Type : 4;
		unsigned __int8 m_nNumVectorComps : 3;
		unsigned __int8 m_bFakeMaterialVar : 1;
		unsigned __int8 m_nTempIndex;
		CUtlSymbol m_Name;
	};

	struct KeyValues;


	struct IMaterialInternal_vtbl;
	struct IMaterialInternal /*: IMaterial*/
	{
		IMaterialInternal_vtbl* vftable;
	};

	struct IMaterialInternal_vtbl
	{
		const char* (__fastcall* GetName)(IMaterialInternal*);
		const char* (__thiscall* GetTextureGroupName)(IMaterialInternal*);
		PreviewImageRetVal_t(__thiscall* GetPreviewImageProperties)(IMaterialInternal*, int*, int*, ImageFormat*, bool*);
		PreviewImageRetVal_t(__thiscall* GetPreviewImage)(IMaterialInternal*, unsigned __int8*, int, int, ImageFormat);
		int(__thiscall* GetMappingWidth)(IMaterialInternal*);
		int(__thiscall* GetMappingHeight)(IMaterialInternal*);
		int(__thiscall* GetNumAnimationFrames)(IMaterialInternal*);
		bool(__thiscall* InMaterialPage)(IMaterialInternal*);
		void(__thiscall* GetMaterialOffset)(IMaterialInternal*, float*);
		void(__thiscall* GetMaterialScale)(IMaterialInternal*, float*);
		IMaterialInternal* (__thiscall* GetMaterialPage)(IMaterialInternal*);
		IMaterialVar* (__fastcall* FindVar)(IMaterialInternal*, void* null, const char*, bool*, bool);
		void(__thiscall* IncrementReferenceCount)(IMaterialInternal*);
		void(__thiscall* DecrementReferenceCount)(IMaterialInternal*);
		int(__thiscall* GetEnumerationID)(IMaterialInternal*);
		void(__thiscall* GetLowResColorSample)(IMaterialInternal*, float, float, float*);
		void(__thiscall* RecomputeStateSnapshots)(IMaterialInternal*);
		bool(__thiscall* IsTranslucent)(IMaterialInternal*);
		bool(__thiscall* IsAlphaTested)(IMaterialInternal*);
		bool(__thiscall* IsVertexLit)(IMaterialInternal*);
		unsigned __int64(__fastcall* GetVertexFormat)(IMaterialInternal*, void* null);
		bool(__thiscall* HasProxy)(IMaterialInternal*);
		bool(__thiscall* UsesEnvCubemap)(IMaterialInternal*);
		bool(__thiscall* NeedsTangentSpace)(IMaterialInternal*);
		bool(__thiscall* NeedsPowerOfTwoFrameBufferTexture)(IMaterialInternal*, bool);
		bool(__thiscall* NeedsFullFrameBufferTexture)(IMaterialInternal*, bool);
		bool(__thiscall* NeedsSoftwareSkinning)(IMaterialInternal*);
		void(__thiscall* AlphaModulate)(IMaterialInternal*, float);
		void(__thiscall* ColorModulate)(IMaterialInternal*, float, float, float);
		void(__thiscall* SetMaterialVarFlag)(IMaterialInternal*, MaterialVarFlags_t, bool);
		bool(__fastcall* GetMaterialVarFlag)(IMaterialInternal*, void* null, MaterialVarFlags_t);
		void(__thiscall* GetReflectivity)(IMaterialInternal*, Vector*);
		bool(__thiscall* GetPropertyFlag)(IMaterialInternal*, MaterialPropertyTypes_t);
		bool(__thiscall* IsTwoSided)(IMaterialInternal*);
		void(__thiscall* SetShader)(IMaterialInternal*, const char*);
		int(__thiscall* GetNumPasses)(IMaterialInternal*);
		int(__thiscall* GetTextureMemoryBytes)(IMaterialInternal*);
		void(__thiscall* Refresh)(IMaterialInternal*);
		bool(__thiscall* NeedsLightmapBlendAlpha)(IMaterialInternal*);
		bool(__thiscall* NeedsSoftwareLighting)(IMaterialInternal*);
		int(__thiscall* ShaderParamCount)(IMaterialInternal*);
		IMaterialVar** (__thiscall* GetShaderParams)(IMaterialInternal*);
		bool(__thiscall* IsErrorMaterial)(IMaterialInternal*);
		void(__thiscall* Unused)(IMaterialInternal*);
		float(__thiscall* GetAlphaModulation)(IMaterialInternal*);
		void(__thiscall* GetColorModulation)(IMaterialInternal*, float*, float*, float*);
		bool(__thiscall* IsTranslucentUnderModulation)(IMaterialInternal*, float);

		int pad[2];

		IMaterialVar* (__fastcall* FindVarFast)(IMaterialInternal*, void* null, const char*, unsigned int*);
		void(__thiscall* SetShaderAndParams)(IMaterialInternal*, KeyValues*);
		const char* (__fastcall* GetShaderName)(IMaterialInternal*); // 0xCC
		void(__thiscall* DeleteIfUnreferenced)(IMaterialInternal*);
		bool(__thiscall* IsSpriteCard)(IMaterialInternal*);
		void(__thiscall* CallBindProxy)(IMaterialInternal*, void*, void*); // ICallQueue
		void(__thiscall* RefreshPreservingMaterialVars)(IMaterialInternal*);
		bool(__thiscall* WasReloadedFromWhitelist)(IMaterialInternal*);
		int(__thiscall* GetReferenceCount)(IMaterialInternal*);
		void(__thiscall* SetEnumerationID)(IMaterialInternal*, int);
		void(__thiscall* SetNeedsWhiteLightmap)(IMaterialInternal*, bool);
		bool(__thiscall* GetNeedsWhiteLightmap)(IMaterialInternal*);
		void(__thiscall* Uncache)(IMaterialInternal*, bool);
		void(__thiscall* Precache)(IMaterialInternal*);
		bool(__thiscall* PrecacheVars)(IMaterialInternal*, KeyValues*, KeyValues*, void*, void*, int); // CUtlVector CUtlMemory
		void(__thiscall* ReloadTextures)(IMaterialInternal*);
		void(__thiscall* SetMinLightmapPageID)(IMaterialInternal*, int);
		void(__thiscall* SetMaxLightmapPageID)(IMaterialInternal*, int);
		int(__thiscall* GetMinLightmapPageID)(IMaterialInternal*);
		int(__thiscall* GetMaxLightmapPageID)(IMaterialInternal*);
		void* (__thiscall* GetShader)(IMaterialInternal*); // IShader
		bool(__thiscall* IsPrecached)(IMaterialInternal*);
		bool(__thiscall* IsPrecachedVars)(IMaterialInternal*);
		void(__thiscall* DrawMesh)(IMaterialInternal*, VertexCompressionType_t, bool, bool);
		unsigned __int64(__thiscall* GetVertexUsage)(IMaterialInternal*);
		bool(__thiscall* PerformDebugTrace)(IMaterialInternal*);
		bool(__thiscall* NoDebugOverride)(IMaterialInternal*);
		void(__thiscall* ToggleSuppression)(IMaterialInternal*);
		bool(__thiscall* IsSuppressed)(IMaterialInternal*);
		void(__thiscall* ToggleDebugTrace)(IMaterialInternal*);
		bool(__thiscall* UseFog)(IMaterialInternal*);
		void(__fastcall* AddMaterialVar)(IMaterialInternal*, void* null, IMaterialVar*);
		struct ShaderRenderState_t* (__thiscall* GetRenderState)(IMaterialInternal*);
		bool(__thiscall* IsManuallyCreated)(IMaterialInternal*);
		bool(__thiscall* NeedsFixedFunctionFlashlight)(IMaterialInternal*);
		bool(__thiscall* IsUsingVertexID)(IMaterialInternal*);
		void(__thiscall* MarkAsPreloaded)(IMaterialInternal*, bool);
		bool(__thiscall* IsPreloaded)(IMaterialInternal*);
		void(__thiscall* ArtificialAddRef)(IMaterialInternal*);
		void(__thiscall* ArtificialRelease)(IMaterialInternal*);
		void(__thiscall* ReportVarChanged)(IMaterialInternal*, struct IMaterialVar*);
		unsigned int(__thiscall* GetChangeID)(IMaterialInternal*);
		bool(__thiscall* IsTranslucentInternal)(IMaterialInternal*, float);
		bool(__thiscall* IsRealTimeVersion)(IMaterialInternal*);
		void(__thiscall* ClearContextData)(IMaterialInternal*);
		IMaterialInternal* (__thiscall* GetRealTimeVersion)(IMaterialInternal*);
		IMaterialInternal* (__thiscall* GetQueueFriendlyVersion)(IMaterialInternal*);
		void(__thiscall* PrecacheMappingDimensions)(IMaterialInternal*);
		void(__thiscall* FindRepresentativeTexture)(IMaterialInternal*);
		void(__thiscall* DecideShouldReloadFromWhitelist)(IMaterialInternal*, struct IFileList*);
		void(__thiscall* ReloadFromWhitelistIfMarked)(IMaterialInternal*);
		void(__thiscall* CompactMaterialVars)(IMaterialInternal*);
	};

	STATIC_ASSERT_OFFSET(IMaterialInternal_vtbl, GetShaderName, 0xCC);

	struct IIndexBuffer_vtbl;
	struct IIndexBuffer
	{
		IIndexBuffer_vtbl* vftable;
	};

	struct IVertexBuffer_vtbl;
	struct IVertexBuffer
	{
		IVertexBuffer_vtbl* vftable;
	};

	struct IMesh : IVertexBuffer, IIndexBuffer
	{
	};

	struct CMeshBase : IMesh
	{
	};

	struct CBaseMeshDX8 : CMeshBase
	{
		bool m_bMeshLocked;
		std::uint64_t m_VertexFormat;
	};

	struct __declspec(align(8)) CMeshDX8 : CBaseMeshDX8
	{
	};

	struct BufferedState_t
	{
		D3DXMATRIX m_Transform[3];
		_D3DVIEWPORT9 m_Viewport;
		int m_BoundTexture[16];
		void* m_VertexShader;
		void* m_PixelShader;
	};

	struct IShaderAPIDX8_vtbl
	{
		char pad[0x560];
		IDirect3DBaseTexture9* (__fastcall* GetD3DTexture)(void* shaderapi_ptr, void* ecx, int handle);
		void* pad96;
		void* pad97;
		void* pad98;
		void* pad99;
		void(__fastcall* GetBufferedState)(void* shaderapi_ptr, void* ecx, BufferedState_t*);
		_D3DCULL(__fastcall* GetCullMode)(void* shaderapi_ptr, void* ecx);
		void* ComputeFillRate;
		void* IsInSelectionMode;
		void* RegisterSelectionHit;
		IMaterialInternal* (__fastcall* GetBoundMaterial)(void* shaderapi_ptr, void* ecx);
	};

	STATIC_ASSERT_OFFSET(IShaderAPIDX8_vtbl, GetD3DTexture, 0x560);
	STATIC_ASSERT_OFFSET(IShaderAPIDX8_vtbl, GetBufferedState, 0x574);

	struct IShaderAPIDX8
	{
		IShaderAPIDX8_vtbl* vtbl;
	};

	struct CViewSetup
	{
		int x;
		// top side of view window
		int y;
		// width of view window
		int width;
		// height of view window
		int height;

		// the rest are only used by 3D views

		// Orthographic projection?
		bool m_bOrtho;
		// View-space rectangle for ortho projection.
		float m_OrthoLeft;
		float m_OrthoTop;
		float m_OrthoRight;
		float m_OrthoBottom;

		// horizontal FOV in degrees
		float fov;
		// horizontal FOV in degrees for in-view model
		float fovViewmodel;

		// 3D origin of camera
		Vector origin;

		// heading of camera (pitch, yaw, roll)
		QAngle angles;
		// local Z coordinate of near plane of camera
		float zNear;
		// local Z coordinate of far plane of camera
		float zFar;

		// local Z coordinate of near plane of camera ( when rendering view model )
		float zNearViewmodel;
		// local Z coordinate of far plane of camera ( when rendering view model )
		float zFarViewmodel;

		// set to true if this is to draw into a subrect of the larger screen
		// this really is a hack, but no more than the rest of the way this class is
		// used
		bool m_bRenderToSubrectOfLargerScreen;

		// The aspect ratio to use for computing the perspective projection matrix
		// (0.0f means use the viewport)
		float m_flAspectRatio;

		// Controls for off-center projection (needed for poster rendering)
		bool m_bOffCenter;
		float m_flOffCenterTop;
		float m_flOffCenterBottom;
		float m_flOffCenterLeft;
		float m_flOffCenterRight;

		// Control that the SFM needs to tell the engine not to do certain
		// post-processing steps
		bool m_bDoBloomAndToneMapping;

		// Cached mode for certain full-scene per-frame varying state such as sun
		// entity coverage
		bool m_bCacheFullSceneState;
	};

	struct __declspec(align(4)) ViewStack_t
	{
		CViewSetup m_View;
		D3DXMATRIX m_matrixView;
		D3DXMATRIX m_matrixProjection;
		D3DXMATRIX m_matrixWorldToScreen;
		bool m_bIs2DView;
		bool m_bNoDraw;
	};

	struct IRender_vtbl;
	struct IRender
	{
		IRender_vtbl* vftable;
	};

	struct CRender_vtbl;
	struct __declspec(align(8)) CRender : IRender
	{
		//CRender_vtbl* vftable;
		float m_yFOV;
		long double m_frameStartTime;
		float m_framerate;
		float m_zNear;
		float m_zFar;
		D3DXMATRIX m_matrixView;
		D3DXMATRIX m_matrixProjection;
		D3DXMATRIX m_matrixWorldToScreen;
		//CUtlStack<CRender::ViewStack_t, CUtlMemory<CRender::ViewStack_t, int> > m_ViewStack;
		char pad_m_ViewStack_memory[0xC];
		int m_ViewStack_size;
		ViewStack_t* m_ViewStack_m_pElements;
		//int m_iLightmapUpdateDepth;
	}; //STATIC_ASSERT_OFFSET(CRender, m_ViewStack_size, 0xDC + 0xC);
}
