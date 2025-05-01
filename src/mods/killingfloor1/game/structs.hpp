#pragma once
#include "game.hpp"

namespace mods::killingfloor1::game
{
	enum EPolyFlags
	{
		// Regular in-game flags.
		PF_Invisible = 0x00000001,	// Poly is invisible.
		PF_Masked = 0x00000002,	// Poly should be drawn masked.
		PF_Translucent = 0x00000004,	// Poly is transparent.
		PF_NotSolid = 0x00000008,	// Poly is not solid, doesn't block.
		PF_Environment = 0x00000010,	// Poly should be drawn environment mapped.
		PF_Semisolid = 0x00000020,	// Poly is semi-solid = collision solid, Csg nonsolid.
		PF_Modulated = 0x00000040,	// Modulation transparency.
		PF_FakeBackdrop = 0x00000080,	// Poly looks exactly like backdrop.
		PF_TwoSided = 0x00000100,	// Poly is visible from both sides.
		PF_NoSmooth = 0x00000800,	// Don't smooth textures.
		PF_AlphaTexture = 0x00001000,	// Honor texture alpha (reuse BigWavy and SpecialPoly flags)
		PF_Flat = 0x00004000,	// Flat surface.
		PF_NoMerge = 0x00010000,	// Don't merge poly's nodes before lighting when rendering.
		PF_NoZTest = 0x00020000,	// Don't test Z buffer
		PF_Additive = 0x00040000,	// sjs - additive blending, (Aliases PF_DirtyShadows).
		PF_SpecialLit = 0x00100000,	// Only speciallit lights apply to this poly.
		PF_Wireframe = 0x00200000,	// Render as wireframe
		PF_Unlit = 0x00400000,	// Unlit.
		PF_Portal = 0x04000000,	// Portal between iZones.
		PF_AntiPortal = 0x08000000,	// Antiportal
		PF_Mirrored = 0x20000000,   // Mirrored BSP surface.

		// Editor flags.
		PF_Memorized = 0x01000000,	// Editor: Poly is remembered.
		PF_Selected = 0x02000000,	// Editor: Poly is selected.
		PF_Subtractive = 0x20000000,	// sjs - subtractive blending
		PF_FlatShaded = 0x40000000,	// FPoly has been split by SplitPolyWithPlane.   

		// Unused flags.
		PF_Unused0 = 0x00000200,
		PF_Unused1 = 0x00000400,
		PF_Unused2 = 0x00002000,
		PF_Unused3 = 0x00008000,
		PF_Unused4 = 0x00040000,
		PF_Unused5 = 0x00080000,

		// Internal.
		PF_EdProcessed = 0x40000000,	// FPoly was already processed in editorBuildFPolys.
		PF_EdCut = 0x80000000,	// FPoly has been split by SplitPolyWithPlane.  
		PF_Occlude = 0x80000000,	// Occludes even if PF_NoOcclude.

		// Combinations of flags.
		PF_NoOcclude = PF_Masked | PF_Translucent | PF_Invisible | PF_Modulated | PF_AlphaTexture,
		PF_NoEdit = PF_Memorized | PF_Selected | PF_EdProcessed | PF_NoMerge | PF_EdCut,
		PF_NoImport = PF_NoEdit | PF_NoMerge | PF_Memorized | PF_Selected | PF_EdProcessed | PF_EdCut,
		PF_AddLast = PF_Semisolid | PF_NotSolid,
		PF_NoAddToBSP = PF_EdCut | PF_EdProcessed | PF_Selected | PF_Memorized,
		PF_NoShadows = PF_Unlit | PF_Invisible | PF_Environment | PF_FakeBackdrop
	};

	struct FMatrix
	{
		float M[4][4];
	};

	struct FPlane
	{
		float xyz[3];
		float dist;
	};

	struct FConvexVolume
	{
		FPlane BoundingPlanes[32];
	}; //Size=0x0200

	struct __declspec(align(4)) FBox
	{
		Vector Min;
		Vector Max;
		std::uint8_t IsValid;
	};

	struct FArray
	{
		void* Data;
		int ArrayNum;
		int ArrayMax;
	};

	struct FName
	{
		int Index;
	};

	struct UObject;
	struct UMaterial;
	struct FBspSurf;
	struct AActor;

	struct TTransArray_FBspSurf_
	{
		FBspSurf* Data;
		int ArrayNum;
		int ArrayMax;
		UObject* Owner;
	};

	struct TArray_FBox_
	{
		FBox* Data;
		int ArrayNum;
		int ArrayMax;
	};

	struct TArray_int_
	{
		int* Data;
		int ArrayNum;
		int ArrayMax;
	};

	struct FLeaf;
	struct TArray_FLeaf_
	{
		FLeaf* Data;
		int ArrayNum;
		int ArrayMax;
	};

	struct TArray_AActor__
	{
		AActor* Data;
		int ArrayNum;
		int ArrayMax;
	};

	struct FBspSection
	{
		//FBspVertexStream Vertices;
		char pad[0x1C];
		int NumNodes;
		UMaterial* Material;
		unsigned int PolyFlags;
		int iLightMapTexture;
	};

	struct TArray_FBspSection_
	{
		FBspSection* Data;
		int ArrayNum;
		int ArrayMax;
	};

	struct FLightMapTexture;
	struct TArray_FLightMapTexture_
	{
		FLightMapTexture* Data;
		int ArrayNum;
		int ArrayMax;
	};

	struct FLightMap;
	struct TArray_FLightMap_
	{
		FLightMap* Data;
		int ArrayNum;
		int ArrayMax;
	};

	struct UClass;
	struct UViewport;
	struct FRenderTarget;
	struct FRenderInterface;

	struct FCameraSceneNode;
	struct FActorSceneNode;
	struct FSkySceneNode;
	struct FMirrorSceneNode;
	struct FWarpZoneSceneNode;

	struct AActor;
	struct ULevel;

	struct FLevelSceneNode;
	struct FSceneNode;
	struct FSceneNode_vtbl
	{
		void(__thiscall* destruct_FSceneNode)(FSceneNode* pthis);
		FSceneNode* (__thiscall* GetLodSceneNode)(FSceneNode* pthis);
		void(__thiscall* Render)(FSceneNode* pthis, FRenderInterface*);
		FLevelSceneNode* (__thiscall* GetLevelSceneNode)(FSceneNode* pthis);
		FCameraSceneNode* (__thiscall* GetCameraSceneNode)(FSceneNode* pthis);
		FActorSceneNode* (__thiscall* GetActorSceneNode)(FSceneNode* pthis);
		FSkySceneNode* (__thiscall* GetSkySceneNode)(FSceneNode* pthis);
		FMirrorSceneNode* (__thiscall* GetMirrorSceneNode)(FSceneNode* pthis);
		FWarpZoneSceneNode* (__thiscall* GetWarpZoneSceneNode)(FSceneNode* pthis);
	};

	struct FSceneNode
	{
		FSceneNode_vtbl* vtbl;
		UViewport* Viewport;
		FRenderTarget* RenderTarget;
		FSceneNode* Parent;
		int Recursion;
		FMatrix WorldToCamera;
		FMatrix CameraToWorld;
		FMatrix CameraToScreen;
		FMatrix ScreenToCamera;
		FMatrix WorldToScreen;
		FMatrix ScreenToWorld;
		Vector ViewOrigin;
		Vector CameraX;
		Vector CameraY;
		float Determinant;
	}; STATIC_ASSERT_OFFSET(FSceneNode, ViewOrigin, 0x194);

	struct FSphere : FPlane
	{ };

#pragma pack(push, 2)
	struct FBspNode
	{
		FPlane Plane;
		std::uint64_t ZoneMask;
		int iVertPool;
		int iSurf;

		union
		{
			int iBack;
			int iChild[1];
		};

		int iFront; //0x0024 
		int iPlane; //0x0028 
		FSphere ExclusiveSphereBound;
		char pad_0x003C[0x10]; //0x003C
		int iCollisionBound;
		int iRenderBound;
		std::uint8_t iZone[2];
		std::uint8_t NumVertices;
		std::uint8_t NodeFlags;
		int iLeaf[2];
		int iSection;
		int iFirstVertex;
		int iLightMap;
		int pad_out[6];
		//FArray Projectors; // FStaticProjectorInfo*
	};
#pragma pack(pop)
	STATIC_ASSERT_OFFSET(FBspNode, ZoneMask, 0x10);
	STATIC_ASSERT_OFFSET(FBspNode, iFront, 0x24);
	STATIC_ASSERT_OFFSET(FBspNode, iRenderBound, 0x50);
	STATIC_ASSERT_OFFSET(FBspNode, NodeFlags, 0x57);
	STATIC_ASSERT_OFFSET(FBspNode, iLightMap, 0x68);
	STATIC_ASSERT_SIZE(FBspNode, 0x84);

	struct FUnknown;
	struct FUnknown_vtbl
	{
		unsigned int(__stdcall* QueryInterface)(FUnknown* pthis, const void*, void**); // FGuid, void**
		unsigned int(__stdcall* AddRef)(FUnknown* pthis);
		unsigned int(__stdcall* Release)(FUnknown* pthis);
	};

	struct FUnknown
	{
		FUnknown_vtbl* vtbl;
	};

	struct UObject : FUnknown
	{
		unsigned int Index;
		UObject* HashNext;
		void* StateFrame; // FStateFrame
		void* Linker; // ULinkerLoad
		unsigned int LinkerIndex;
		UObject* Outer;
		unsigned int ObjectFlags;
		FName Name;
		UClass* Class;
	};

	struct UPrimitive : UObject
	{
	  FBox BoundingBox;
	  FSphere BoundingSphere;
	};

	struct UPolys;

	struct TTransArray_FBspNode_ //: FArray
	{
		FBspNode* Data;
		int ArrayNum;
		int ArrayMax;
		UObject* Owner;
	}; STATIC_ASSERT_SIZE(TTransArray_FBspNode_, 0x10);

	struct FVert;
	struct TTransArray_FVert_
	{
		FVert* Data;
		int ArrayNum;
		int ArrayMax;
		UObject* Owner;
	};

	struct TTransArray_FVector_
	{
		Vector* Data;
		int ArrayNum;
		int ArrayMax;
		UObject* Owner;
	};

	struct UMaterial_vtbl
	{
		unsigned int(__stdcall* QueryInterface)(FUnknown* pthis, const void*, void**);
		unsigned int(__stdcall* AddRef)(FUnknown* pthis);
		unsigned int(__stdcall* Release)(FUnknown* pthis);
		void(__thiscall * UObject_Desctructor)(UObject* pthis);
		void(__thiscall* ProcessEvent)(UObject* pthis, void*, void*, void*);
		void(__thiscall* ProcessDelegate)(UObject* pthis, FName, void*, void*, void*);
		void(__thiscall* ProcessState)(UObject* pthis, float);
		int(__thiscall* ProcessRemoteFunction)(UObject* pthis, void*, void*, void*);
		void(__thiscall* Modify)(UObject* pthis);
		void(__thiscall* PostLoad)(UObject* pthis);
		void(__thiscall* Destroy)(UObject* pthis);
		void(__thiscall* Serialize)(UObject* pthis, void*);
		int(__thiscall* IsPendingKill)(UObject* pthis);
		int(__thiscall* GotoState)(UObject* pthis, FName);
		int(__thiscall* GotoLabel)(UObject* pthis, FName);
		void(__thiscall* ScriptInit)(UObject* pthis, void*);
		void(__thiscall* InitExecution)(UObject* pthis);
		void(__thiscall* ShutdownAfterError)(UObject* pthis);
		void(__thiscall* PostEditChange)(UObject* pthis);
		void(__thiscall* PreEditUndo)(UObject* pthis);
		void(__thiscall* PostEditUndo)(UObject* pthis);
		void(__thiscall* CallFunction)(UObject* pthis, void*, void* const, void*);
		int(__thiscall* ScriptConsoleExec)(UObject* pthis, const unsigned __int16*, void*, UObject*);
		void(__thiscall* Register)(UObject* pthis);
		void(__thiscall* LanguageChange)(UObject* pthis);
		void(__thiscall* Rename)(UObject* pthis, const unsigned __int16*, UObject*);
		void(__thiscall* NetDirty)(UObject* pthis, void*);
		void(__thiscall* ExecutingBadStateCode)(UObject* pthis, void*);
		int(__thiscall* CheckCircularReferences)(UMaterial* pthis, void*);
		int(__thiscall* GetValidated)(UMaterial* pthis);
		void(__thiscall* SetValidated)(UMaterial* pthis, int);
		void(__thiscall* PreSetMaterial)(UMaterial* pthis, float);
		int(__thiscall* MaterialUSize)(UMaterial* pthis);
		int(__thiscall* MaterialVSize)(UMaterial* pthis);
		int(__thiscall* RequiresSorting)(UMaterial* pthis);
		int(__thiscall* IsTransparent)(UMaterial* pthis);
		unsigned __int8(__thiscall* RequiredUVStreams)(UMaterial* pthis);
		int(__thiscall* RequiresNormal)(UMaterial* pthis);
		UMaterial* (__thiscall* CheckFallback)(UMaterial* pthis);
		int(__thiscall* HasFallback)(UMaterial* pthis);
	};

	struct UMaterialVtbl
	{
		UMaterial_vtbl* mat_vtbl;
	};

	struct UMaterial : UObject
	{
		int pad;
		UMaterialVtbl* to_vtbl;
	};

	struct ABrush;

	struct FBspSurf
	{
		UMaterial* Material;
		DWORD		PolyFlags;
		INT			pBase;	
		INT			vNormal;	
		INT			vTextureU;
		INT			vTextureV;
		INT			iBrushPoly;
		ABrush*		Actor;
		TArray_int_	Nodes;
		FPlane		Plane;
		FLOAT		LightMapScale;
		char pad_0x0040[0x18]; //0x0040
	};

	STATIC_ASSERT_SIZE(FBspSurf, 0x58);

	

	struct AZoneInfo //: AInfo
	{
		//ASkyZoneInfo* SkyZone;
		//FName ZoneTag;
		//FStringNoInit LocationName;
		//float KillZ;
		//unsigned __int8 KillZType;
		//unsigned __int32 bSoftKillZ : 1;
		//unsigned __int32 bTerrainZone : 1;
		//unsigned __int32 bDistanceFog : 1;
		//unsigned __int32 bClearToFogColor : 1;
		//TArrayNoInit_ATerrainInfo__ Terrains;
		//FVector AmbientVector;
		//unsigned __int8 AmbientBrightness;
		//unsigned __int8 AmbientHue;
		//unsigned __int8 AmbientSaturation;
		//FColor DistanceFogColor;
		//float DistanceFogStart;
		//float DistanceFogEnd;
		//float RealDistanceFogEnd;
		//float DistanceFogEndMin;
		//float DistanceFogBlendTime;
		//UTexture* EnvironmentMap;
		//float TexUPanSpeed;
		//float TexVPanSpeed;
		//float DramaticLightingScale;
		//UI3DL2Listener* ZoneEffect;
		//unsigned __int32 bLonelyZone : 1;
		//TArrayNoInit_AZoneInfo__ ManualExcludes;
	};

	struct FZoneProperties
	{
		AZoneInfo* ZoneActor;
		float LastRenderTime;
		std::uint64_t Connectivity;
		std::uint64_t Visibility;
	};

	struct UModel : UPrimitive
	{
	  UPolys* Polys;
	  TTransArray_FBspNode_ Nodes;
	  TTransArray_FVert_ Verts;
	  TTransArray_FVector_ Vectors;
	  TTransArray_FVector_ Points;
	  TTransArray_FBspSurf_ Surfs;
	  TArray_FBox_ Bounds;
	  TArray_int_ LeafHulls;
	  TArray_FLeaf_ Leaves;
	  TArray_AActor__ Lights;
	  TArray_FBspSection_ Sections;
	  //TArray<FBspSection>			Sections;
	  TArray_FLightMapTexture_ LightMapTextures;
	  TArray_FLightMap_ LightMaps;
	  TArray_int_ DynamicLightMaps;
	  int pad[10];
	  int RootOutside__0x130;
	  int Linked;
	  int MoverLink;
	  int NumSharedSides;
	  int NumZones;
	  FZoneProperties Zones[64];
	};
	STATIC_ASSERT_OFFSET(UModel, Polys, 0x54);
	STATIC_ASSERT_OFFSET(UModel, Bounds, 0xA8);
	STATIC_ASSERT_OFFSET(UModel, DynamicLightMaps, 0xFC);
	STATIC_ASSERT_OFFSET(UModel, RootOutside__0x130, 0x130);


	struct FLevelSceneNode : FSceneNode
	{
	  ULevel* Level;
	  UModel* Model;
	  AActor* ViewActor;
	  int ViewZone;
	  int InvisibleZone;
	  unsigned int StencilMask;
	};

	//struct FBspDrawList;

	/*struct TList_int_
	{
	    int Element;
	    TList_int_* Next;
	};*/

	template <class ElementType> class TList
	{
	public:

		ElementType			Element;
		TList<ElementType>* Next;

		// Constructor.

		TList(ElementType InElement, TList<ElementType>* InNext = NULL)
		{
			Element = InElement;
			Next = InNext;
		}
	};

	struct FTranslucentDrawItem
	{
		int	BSP;
		int	iNode;
		void** DynamicLights; // FDynamicLight
		int	NumDynamicLights;
		void** DynamicProjectors; // FProjectorRenderInfo
		int NumDynamicProjectors;
		void* DynamicActor; // FDynamicActor
	};

	struct FBspNodeList
	{
		UModel* Model;
		INT		SectionIndex,
				NumNodes,
				NumTriangles;
		INT*	Nodes;
	};

	class FBspDrawList : public FBspNodeList
	{
	public:

		TList<class FBspLightDrawList*>* DynamicLights;
		TList<class FBspProjectorDrawList*>* Projectors;

		// Constructors.

		FBspDrawList(UModel* InModel, INT InSectionIndex) : FBspNodeList(InModel, InSectionIndex)
		{
			DynamicLights = NULL;
			Projectors = NULL;
		}

		FBspDrawList(UModel* InModel) :
			FBspNodeList(InModel)
		{
			DynamicLights = NULL;
			Projectors = NULL;
		}

		// AddNode

		void AddNode(INT NodeIndex, void** InDynamicLights, INT NumDynamicLights, void** InDynamicProjectors, INT NumDynamicProjectors, FLevelSceneNode* SceneNode);
	};

	struct UClient;

	struct FColor
	{
		unsigned __int8 B;
		unsigned __int8 G;
		unsigned __int8 R;
		unsigned __int8 A;
	};

	struct FDynamicActor
	{
		AActor* Actor;
		int Revision;
		FMatrix LocalToWorld;
		FMatrix WorldToLocal;
		float Determinant;
		FBox BoundingBox;
		FSphere BoundingSphere;
		FColor AmbientColor;
		int Translucent;
		unsigned int VisibilityTag;
		FBox PredictedBox;
		Vector AmbientVector;
	};


	struct TList_FDynamicActor__
	{
		FDynamicActor* Element;
		TList_FDynamicActor__* Next;
	};

	
	struct FVisibilityInterface;
	struct FVisibilityInterface_vtbl
	{
		void(__thiscall* destruct_FVisibilityInterface)(FVisibilityInterface* pthis);
		BYTE gap4[4];
		int(__thiscall* Visible)(FVisibilityInterface* pthis, const FBox*);
	};

	struct FVisibilityInterface
	{
		FVisibilityInterface_vtbl* vtbl;
	};

	struct TList_FConvexVolume__
	{
		FConvexVolume* Element;
		TList_FConvexVolume__* Next;
	};

	struct FZoneInfo : FVisibilityInterface
	{
		TList_FConvexVolume__* Portals;
		TList_FConvexVolume__* AntiPortals;
	};

	struct /*__declspec(align(4))*/ FRenderState
	{
		FLevelSceneNode* SceneNode;
		FRenderInterface* RI;
		ULevel* Level;
		UModel* Model;
		UClient* Client;
		TList_FDynamicActor__** LeafActors;
		TList_FDynamicActor__* OutsideActors;
		FZoneInfo Zones[64];
		TList<int>* ActiveZones;
		unsigned __int64 ActiveZoneMask;
		unsigned __int8* RenderedPortals;
		TList<FLevelSceneNode*>* ChildSceneNodes;
		//FDynamicLight*** LeafLights;
		//FProjectorRenderInfo*** LeafProjectors;
		//TList_FDynamicActor__* ActorDrawList;
		int pad1[3];
		TList<FTranslucentDrawItem>* TranslucentDrawList;
		//FStaticMeshBatchList StaticMeshBatchList;
		int pad[5];
		FBspDrawList** BspDrawLists;
		TList<INT>* SectionDrawList;
		//TList_FBspStencilDrawList__* StencilDrawLists;
		//unsigned int SkyStencilMask;
	};

	STATIC_ASSERT_OFFSET(FRenderState, Zones, 0x1C);
	STATIC_ASSERT_OFFSET(FRenderState, TranslucentDrawList, 0x33C);
	STATIC_ASSERT_OFFSET(FRenderState, BspDrawLists, 0x354);

}
