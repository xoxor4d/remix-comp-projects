#pragma once
//#include "game.hpp"

namespace mods::blackmesa::game
{
	enum view_id : __int32
	{
		VIEW_ILLEGAL = 0xFFFFFFFE,
		VIEW_NONE = 0xFFFFFFFF,
		VIEW_MAIN = 0x0,
		VIEW_3DSKY = 0x1,
		VIEW_MONITOR = 0x2,
		VIEW_REFLECTION = 0x3,
		VIEW_REFRACTION = 0x4,
		VIEW_INTRO_PLAYER = 0x5,
		VIEW_INTRO_CAMERA = 0x6,
		VIEW_SHADOW_DEPTH_TEXTURE = 0x7,
		VIEW_ID_COUNT = 0x8,
	};

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

	enum OverrideType_t : int
	{
		OVERRIDE_NORMAL = 0x0,
		OVERRIDE_BUILD_SHADOWS = 0x1,
		OVERRIDE_DEPTH_WRITE = 0x2,
	};

	enum modtype_t : std::int32_t
	{
		mod_bad = 0x0,
		mod_brush = 0x1,
		mod_sprite = 0x2,
		mod_studio = 0x3,
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

	struct VMatrix
	{
		float m[4][4];
	};

	struct QAngle
	{
		float x;
		float y;
		float z;
	};

	struct Color
	{
		unsigned __int8 _color[4];
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

	struct ITexture;
	struct IMaterial;
	struct IMaterialVar_vtbl
	{
		ITexture* (__thiscall* GetTextureValue)(IMaterialVar*);
		bool(__thiscall* IsTextureValueInternalEnvCubemap)(IMaterialVar*);
		const char* (__fastcall* GetName)(IMaterialVar*);
		unsigned __int16(__thiscall* GetNameAsSymbol)(IMaterialVar*);
		void(__thiscall* SetFloatValue)(IMaterialVar*, float);
		void(__thiscall* SetIntValue)(IMaterialVar*, int);
		void(__thiscall* SetStringValue)(IMaterialVar*, const char*);
		const char* (__thiscall* GetStringValue)(IMaterialVar*);
		void(__thiscall* SetFourCCValue)(IMaterialVar*, unsigned int, void*);
		void(__thiscall* GetFourCCValue)(IMaterialVar*, unsigned int*, void**);
		void(__thiscall* SetVecValue0)(IMaterialVar*, float, float, float, float);
		void(__thiscall* SetVecValue1)(IMaterialVar*, float, float, float);
		void(__thiscall* SetVecValue2)(IMaterialVar*, float, float);
		void(__thiscall* SetVecValue3)(IMaterialVar*, const float*, int);
		void(__thiscall* GetLinearVecValue)(IMaterialVar*, float*, int);
		void(__thiscall* SetTextureValue)(IMaterialVar*, ITexture*);
		IMaterial* (__thiscall* GetMaterialValue)(IMaterialVar*);
		void(__thiscall* SetMaterialValue)(IMaterialVar*, IMaterial*);
		bool(__thiscall* IsDefined)(IMaterialVar*);
		void(__thiscall* SetUndefined)(IMaterialVar*);
		void(__thiscall* SetMatrixValue)(IMaterialVar*, const VMatrix*);
		const VMatrix* (__thiscall* GetMatrixValue)(IMaterialVar*);
		bool(__thiscall* MatrixIsIdentity)(IMaterialVar*);
		void(__thiscall* CopyFrom)(IMaterialVar*, IMaterialVar*);
		void(__thiscall* SetValueAutodetectType)(IMaterialVar*, const char*);
		IMaterial* (__thiscall* GetOwningMaterial)(IMaterialVar*);
		void(__thiscall* SetVecComponentValue)(IMaterialVar*, float, int);
		int(__thiscall* GetIntValueInternal)(IMaterialVar*);
		float(__thiscall* GetFloatValueInternal)(IMaterialVar*);
		void(__thiscall* GetVecValueInternal0)(IMaterialVar*, float*, int);
		const float* (__thiscall* GetVecValueInternal1)(IMaterialVar*);
		int(__thiscall* VectorSizeInternal)(IMaterialVar*);
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


	struct ConCommandBase_vtbl;
	struct ConCommandBase
	{
		ConCommandBase_vtbl* vftable;
		ConCommandBase* m_pNext;
		bool m_bRegistered;
		const char* m_pszName;
		const char* m_pszHelpString;
		int m_nFlags;
	};

	struct IConVar_vtbl;
	struct IConVar
	{
		IConVar_vtbl* vtbl;
	};

	struct IConVar_vtbl
	{
		void(__thiscall* SetValue_Color)(IConVar*, Color);
		void(__thiscall* SetValue_Int)(IConVar*, int);
		void(__thiscall* SetValue_Float)(IConVar*, float);
		void(__thiscall* SetValue_String)(IConVar*, const char*);
		const char* (__thiscall* GetName)(IConVar*);
		const char* (__thiscall* GetBaseName)(IConVar*);
		bool(__thiscall* IsFlagSet)(IConVar*, int);
		int(__thiscall* GetSplitScreenPlayerSlot)(IConVar*);
	};

	struct ConVar_CVValue_t
	{
		char* m_pszString;
		int m_StringLength;
		float m_fValue;
		int m_nValue;
	};

	const struct ConVar : ConCommandBase, IConVar
	{
		ConVar* m_pParent;
		const char* m_pszDefaultValue;
		ConVar_CVValue_t m_Value;
		bool m_bHasMin;
		float m_fMinVal;
		bool m_bHasMax;
		float m_fMaxVal;
		//m_fnChangeCallbacks;
	};

	const struct CCommand
	{
		int m_nArgc;
		int m_nArgv0Size;
		char m_pArgSBuffer[512];
		char m_pArgvBuffer[512];
		const char* m_ppArgv[64];
	};

	struct ICommandCallback_vtbl;
	struct ICommandCallback
	{
		ICommandCallback_vtbl* vftable;
	};

	struct ICommandCallback_vtbl
	{
		void(__thiscall* CommandCallback)(ICommandCallback*, const CCommand*);
	};

	union $DBE510182F331AFA63E289BCE7D04441
	{
		void(__cdecl* m_fnCommandCallbackV1)();
		void(__cdecl* m_fnCommandCallback)(const CCommand*);
		ICommandCallback* m_pCommandCallback;
	};

	struct ICommandCompletionCallback_vtbl;
	struct ICommandCompletionCallback
	{
		ICommandCompletionCallback_vtbl* vftable;
	};

	struct ICommandCompletionCallback_vtbl
	{
		int(__thiscall* CommandCompletionCallback)(ICommandCompletionCallback*, const char*, void* CUtlString);
	};

	union $E9983340D3446DFCFEC741F54422A481
	{
		int(__cdecl* m_fnCompletionCallback)(const char*, char(*)[64]);
		ICommandCompletionCallback* m_pCommandCompletionCallback;
	};

	struct __declspec(align(4)) ConCommand : ConCommandBase
	{
		$DBE510182F331AFA63E289BCE7D04441 u1;
		$E9983340D3446DFCFEC741F54422A481 u2;
		__int8 m_bHasCompletionCallback : 1;
		__int8 m_bUsingNewCommandCallback : 1;
		__int8 m_bUsingCommandCallbackInterface : 1;
	};

	struct CCvar_vtbl;
	struct CCvar
	{
		CCvar_vtbl* vftable;
	};

	struct CCvar_vtbl
	{
		bool(__thiscall* Connect)(void*, void* (__cdecl*)(const char*, int*)); // IAppSystem* this
		void(__thiscall* Disconnect)(void*);
		void* (__thiscall* QueryInterface)(void*, const char*);
		int(__thiscall* Init)(void*); // InitReturnVal_t
		void(__thiscall* Shutdown)(void*);
		const void* (__thiscall* GetDependencies)(void*); // AppSystemInfo_t
		int(__thiscall* GetTier)(void*); // AppSystemTier_t
		void(__thiscall* Reconnect)(void*, void* (__cdecl*)(const char*, int*), const char*); // ^
		int(__thiscall* AllocateDLLIdentifier)(CCvar*);
		void(__thiscall* RegisterConCommand)(CCvar*, ConCommandBase*);
		void(__thiscall* UnregisterConCommand)(CCvar*, ConCommandBase*);
		void(__thiscall* UnregisterConCommands)(CCvar*, int);
		const char* (__thiscall* GetCommandLineValue)(CCvar*, const char*);
		const ConCommandBase* (__thiscall* FindCommandBase_const)(CCvar*, const char*);
		ConCommandBase* (__thiscall* FindCommandBase)(CCvar*, const char*);
		const ConVar* (__thiscall* FindVar_const)(CCvar*, const char*);
		ConVar* (__thiscall* FindVar)(CCvar*, const char*);
		const ConCommand* (__thiscall* FindCommand_const)(CCvar*, const char*);
		ConCommand* (__thiscall* FindCommand)(CCvar*, const char*);
	};

	struct cplane_t
	{
		Vector normal;
		float dist;
		unsigned __int8 type;
		unsigned __int8 signbits;
		unsigned __int8 pad[2];
	};

	struct mnode_t
	{
		int contents;
		int visframe;
		mnode_t* parent;
		__int16 area;
		__int16 flags;
		VectorAligned m_vecCenter;
		VectorAligned m_vecHalfDiagonal;
		cplane_t* plane;
		mnode_t* children[2];
		unsigned __int16 firstsurface;
		unsigned __int16 numsurfaces;
	};

	struct fourplanes_t
	{
		__m128 nX;
		__m128 nY;
		__m128 nZ;
		__m128 dist;
		__m128 xSign;
		__m128 ySign;
		__m128 zSign;
		__m128 nXAbs;
		__m128 nYAbs;
		__m128 nZAbs;
	};

	struct Frustum_t
	{
		fourplanes_t planes[2];
	};

	struct mleaf_t
	{
		int contents;
		int visframe;
		mnode_t* parent; // mnode_t
		__int16 area;
		__int16 flags;
		VectorAligned m_vecCenter;
		VectorAligned m_vecHalfDiagonal;
		__int16 cluster;
		__int16 leafWaterDataID;
		unsigned __int16 firstmarksurface;
		unsigned __int16 nummarksurfaces;
		__int16 nummarknodesurfaces;
		__int16 unused;
		unsigned __int16 dispListStart;
		unsigned __int16 dispCount;
	};

	struct mleafwaterdata_t;
	struct mvertex_t;
	struct doccluderdata_t;
	struct doccluderpolydata_t;
	struct mtexinfo_t;
	struct csurface_t;

	struct worldbrushdata_t
	{
		int numsubmodels;

		int numplanes;
		cplane_t* planes;

		int numleafs;  // number of visible leafs, not counting 0
		mleaf_t* leafs;

		int numleafwaterdata;
		mleafwaterdata_t* leafwaterdata;

		int numvertexes;
		mvertex_t* vertexes;

		int numoccluders;
		doccluderdata_t* occluders;

		int numoccluderpolys;
		doccluderpolydata_t* occluderpolys;

		int numoccludervertindices;
		int* occludervertindices;

		int numvertnormalindices;  // These index vertnormals.
		std::uint16_t* vertnormalindices;

		int numvertnormals;
		Vector* vertnormals;

		int numnodes;
		mnode_t* nodes;
		std::uint16_t* m_LeafMinDistToWater;

		int numtexinfo;
		mtexinfo_t* texinfo;

		int numtexdata;
		csurface_t* texdata;

		int numDispInfos;
		void* hDispInfos;
		//void* surfaces1; // msurface1_t
		//msurface2_t* surfaces2;
		//void* surfacelighting; // msurfacelighting_t
		//msurfacenormal_t* surfacenormals;
		//unsigned __int16* m_pSurfaceBrushes;
		//dfacebrushlist_t* m_pSurfaceBrushList;
		//int numvertindices;
		//unsigned __int16* vertindices;
		//int nummarksurfaces;
		//msurface2_t** marksurfaces;
		//void* lightdata; // ColorRGBExp32
		//int m_nLightingDataSize;
		//int numworldlights;
		//void* worldlights; // dworldlight_t
		//void* shadowzbuffers;
		//int numprimitives;
		//mprimitive_t* primitives;
		//int numprimverts;
		//mprimvert_t* primverts;
		//int numprimindices;
		//unsigned __int16* primindices;
		//int m_nAreas;
		//void* m_pAreas; // darea_t
		//int m_nAreaPortals;
		//void* m_pAreaPortals; // dareaportal_t
		//int m_nClipPortalVerts;
		//Vector* m_pClipPortalVerts;
		//void* m_pCubemapSamples; // mcubemapsample_t
		//int m_nCubemapSamples;
		//int m_nDispInfoReferences;
		//unsigned __int16* m_pDispInfoReferences;
		//void* m_pLeafAmbient; // dleafambientindex_t
		//void* m_pAmbientSamples; // dleafambientlighting_t
		//bool m_bUnloadedAllLightmaps;
		//void* m_pLightingDataStack; // CMemoryStack
		//int m_nBSPFileSize;
	};


	struct model_t
	{
		void* fnHandle;
		const char* szPathName;
		int nLoadFlags;
		int nServerCount;
		int unk;
		modtype_t type;
		int flags;
		Vector mins;
		Vector maxs;
		float radius;
		KeyValues* m_pKeyValues;
		//$827FC955A655E715E2ACE31D316F483B ___u10;
	}; STATIC_ASSERT_OFFSET(model_t, radius, 0x34);

	struct CCommonHostState
	{
		model_t* worldmodel;
		worldbrushdata_t* worldbrush;
		float interval_per_tick;
		int max_splitscreen_players;
		int max_splitscreen_players_clientdll;
	};

	struct CGlobalVarsBase
	{
		float realtime;
		int framecount;
		float absoluteframetime;
		float curtime;
		float frametime;
		int maxClients;
		int tickcount;
		// ....
	};

	struct studiohdr_t;
	struct studiohwdata_t;
	struct IClientRenderable;
	struct StudioDecalHandle_t__;
	struct DrawModelInfo_t;
	struct CStudioHdr;
	struct Ray_t;

	struct DrawModelState_t
	{
		studiohdr_t* m_pStudioHdr;
		studiohwdata_t* m_pStudioHWData;
		IClientRenderable* m_pRenderable;
		const shared::matrix3x4_t* m_pModelToWorld;
		StudioDecalHandle_t__* m_decals;
		int m_drawFlags;
		int m_lod;
	};

	struct __declspec(align(4)) ModelRenderInfo_t
	{
		Vector origin;
		QAngle angles;
		IClientRenderable* pRenderable;
		const model_t* pModel;
		const shared::matrix3x4_t* pModelToWorld;
		const shared::matrix3x4_t* pLightingOffset;
		const Vector* pLightingOrigin;
		int flags;
		int entity_index;
		int skin;
		int body;
		int hitboxset;
		unsigned __int16 instance;
	};

	struct mstudio_modelvertexdata_t
	{
		const void* pVertexData;
		const void* pTangentData;
	};

	struct mstudiomodel_t
	{
		char name[64];
		int type;
		float boundingradius;
		int nummeshes;
		int meshindex;
		int numvertices;
		int vertexindex;
		int tangentsindex;
		int numattachments;
		int attachmentindex;
		int numeyeballs;
		int eyeballindex;
		mstudio_modelvertexdata_t vertexdata;
		int unused[8];
	};
}
