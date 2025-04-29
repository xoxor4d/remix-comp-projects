#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/remix.hpp"
#include "shared/common/remix_api.hpp"

namespace mods::swat4
{
	namespace tex_addons
	{
		LPDIRECT3DTEXTURE9 sky_gray_up;
	}

	void patches::init_texture_addons(bool release)
	{
		if (release)
		{
			if (tex_addons::sky_gray_up) tex_addons::sky_gray_up->Release();
			return;
		}

		if (!m_textures_initialized)
		{
			const auto dev = shared::globals::d3d_device;
			D3DXCreateTextureFromFileA(dev, "swat4-rtx\\textures\\graycloud_up.jpg", &tex_addons::sky_gray_up);

			m_textures_initialized = true;
		}
	}

	// -----

	void begin_scene_cb()
	{
	}

	void end_scene_cb()
	{
	}

	void present_scene_cb()
	{
	}

	// --------------------------

	std::vector<int> g_forcedLeaves;
	std::vector<int> g_forcedNodes;

	bool IsPointInsideBox(const game::FBox& Box, const Vector& Point)
	{
		return Box.IsValid &&
			Point.x >= Box.Min.x && Point.x <= Box.Max.x &&
			Point.y >= Box.Min.y && Point.y <= Box.Max.y &&
			Point.z >= Box.Min.z && Point.z <= Box.Max.z;
	}

	Vector ClosestPointOnBox(const game::FBox& Box, const Vector& Point)
	{
		Vector result;
		result.x = std::clamp(Point.x, Box.Min.x, Box.Max.x);
		result.y = std::clamp(Point.y, Box.Min.y, Box.Max.y);
		result.z = std::clamp(Point.z, Box.Min.z, Box.Max.z);
		return result;
	}

	struct FLeafAndNodeIndices
	{
		std::vector<int> Leaves;
		std::vector<int> Nodes;
	};

	FLeafAndNodeIndices CollectLeavesAndNodesInRadius(game::FRenderState& RenderState, Vector PlayerPos, float Radius)
	{
		FLeafAndNodeIndices Result;
		FLOAT RadiusSquared = Radius * Radius;

		// Collect leaves
		for (INT iLeaf = 0; iLeaf < RenderState.Model->Leaves.ArrayNum; iLeaf++)
		{
			game::FBox LeafBounds;
			auto FoundBounds = false;
			for (INT iNode = 0; iNode < RenderState.Model->Nodes.ArrayNum; iNode++)
			{
				auto* n = &RenderState.Model->Nodes.Data[iNode];
				if (RenderState.Model->Nodes.Data[iNode].iLeaf[0] == iLeaf || RenderState.Model->Nodes.Data[iNode].iLeaf[1] == iLeaf)
				{
					if (RenderState.Model->Nodes.Data[iNode].iRenderBound != -1) // INDEX_NONE
					{
						LeafBounds = RenderState.Model->Bounds.Data[RenderState.Model->Nodes.Data[iNode].iRenderBound];
						FoundBounds = true;
						break;
					}
				}
			}

			if (FoundBounds && LeafBounds.IsValid)
			{
				if (IsPointInsideBox(LeafBounds, PlayerPos)) {
					Result.Leaves.push_back(iLeaf); // do not add
				}
				else
				{
					Vector ClosestPoint = ClosestPointOnBox(LeafBounds, PlayerPos);
					float DistanceSquared = (ClosestPoint - PlayerPos).LengthSqr();
					if (DistanceSquared <= RadiusSquared)
					{
						Result.Leaves.push_back(iLeaf);
					}
				}
			}
		}

		// Collect nodes
		for (INT iNode = 0; iNode < RenderState.Model->Nodes.ArrayNum; iNode++)
		{
			game::FBspNode& Node = RenderState.Model->Nodes.Data[iNode];
			if (Node.iRenderBound != -1 && Node.iSurf != -1 && Node.iSection != -1 && Node.iSection < RenderState.Model->Sections.ArrayNum) // INDEX_NONE
			{
				game::FBox NodeBounds = RenderState.Model->Bounds.Data[Node.iRenderBound];

				if (IsPointInsideBox(NodeBounds, PlayerPos)) {
					//Result.Nodes.push_back(iNode); // do not add 
				}
				else
				{
					Vector ClosestPoint = ClosestPointOnBox(NodeBounds, PlayerPos);
					float DistanceSquared = (ClosestPoint - PlayerPos).LengthSqr();
					if (DistanceSquared <= RadiusSquared)
					{
						Result.Nodes.push_back(iNode);
					}
				}

			}
		}

		return Result;
	}

	void post_bsp_traversal_hk(game::FRenderState* RenderState)
	{
		const auto im = imgui::get();

		g_forcedLeaves.clear();
		g_forcedNodes.clear();

		Vector PlayerPos = RenderState->SceneNode->ViewOrigin;
		FLOAT Radius = im->m_render_area_dist;
		FLeafAndNodeIndices Indices = CollectLeavesAndNodesInRadius(*RenderState, PlayerPos, Radius);

		g_forcedLeaves = Indices.Leaves;
		g_forcedNodes = Indices.Nodes;

		if (im->m_enable_leaf_forcing)
		{
			for (const auto& l : g_forcedLeaves)
			{
				const auto OriginalInvisibleZone = RenderState->SceneNode->InvisibleZone;
				RenderState->SceneNode->InvisibleZone = -1;

				// ProcessLeaf
				shared::utils::hook::call<void __cdecl(game::FRenderState* RenderState, int iLeaf)>(ENGINE_BASE + 0x1FAF60)(RenderState, l);

				RenderState->SceneNode->InvisibleZone = OriginalInvisibleZone;
			}
		}

		//if (im->m_enable_node_forcing)
		//{
		//	for (const auto& n : g_forcedNodes)
		//	{
		//		// ProcessNode -- FDynamicLight, FProjectorRenderInfo
		//		shared::utils::hook::call<void __cdecl(game::FRenderState* RenderState, int iNode, void** a3, int NumDynamicLights, void** a5, int NumDynamicProjectors)>(ENGINE_BASE + 0x1F6A20)(RenderState, n, nullptr, 0, nullptr, 0);
		//	}
		//}

		if (im->m_enable_node_forcing)
		{
			for (const auto& node_idx : g_forcedNodes)
			{
				{
					game::FBspNode* Node = &RenderState->Model->Nodes.Data[node_idx];
					game::FBspSurf* Surf = &RenderState->Model->Surfs.Data[Node->iSurf];

					//for (auto i = 0; i < RenderState->Model->Nodes.ArrayNum; i++) 
					//{
					//	RenderState->Model->Nodes.Data[i].ZoneMask = -1;
					//	RenderState->Model->Nodes.Data[i].ExclusiveSphereBound.dist += 10000.0f;
					//	RenderState->Model->Nodes.Data[i].iRenderBound = -1;

					//	// setting izone for all to 0 makes everything visible but also kills me lmao
					//	//RenderState->Model->Nodes.Data[i].iZone[0] = 0;
					//	//RenderState->Model->Nodes.Data[i].iZone[1] = 0;
					//}

					// can be used to visualize portal surfaces
					/*for (auto i = 0; i < RenderState->Model->Surfs.ArrayNum; i++)
					{
						if (RenderState->Model->Surfs.Data[i].PolyFlags & game::PF_Portal)
						{
							RenderState->Model->Surfs.Data[i].PolyFlags &= ~game::PF_Invisible;
						}
					}*/



					/*for (auto i = 0; i < 64 ; i++)
					{
						RenderState->Zones[i].Portals = nullptr;
					}
					RenderState->ActiveZoneMask = -1;*/





					/* later drawn like
					 
					 for(TList<INT>* SectionList = RenderState.SectionDrawList;SectionList;SectionList = SectionList->Next)
						RenderState.BspDrawLists[SectionList->Element]->Render(RenderState.SceneNode,RI);
					 
					if (NumTriangles > 0)
					{
						FBspSection& Section = Model->Sections(SectionIndex);*/





					if (Node && Surf && Surf->Material && Surf->Material->to_vtbl && Surf->Material->to_vtbl->mat_vtbl
						&& Node->iSection != -1 && !(Surf->PolyFlags & game::PF_Invisible))
					{
						if (Surf->Material && Surf->Material->to_vtbl->mat_vtbl->RequiresSorting(Surf->Material))
						{
							int x = 0;
							// Add the node to the translucent draw list.

							/*FTranslucentDrawItem	TranslucentNodeItem;
							TranslucentNodeItem.BSP = 1;
							TranslucentNodeItem.iNode = iNode;
							TranslucentNodeItem.NumDynamicLights = NumDynamicLights;
							TranslucentNodeItem.NumDynamicProjectors = NumDynamicProjectors;*/
						}
						else
						{
							// Add the node to it's section's draw list.

							if (!RenderState->BspDrawLists[Node->iSection])
							{
								game::FBspDrawList* list_mem = nullptr;
								game::FBspDrawList* list_out = nullptr;

								// FMemStack::PushBytes(&GSceneMem, 28, 8);
								list_mem = static_cast<game::FBspDrawList*>(shared::utils::hook::call<void* __fastcall(void* gscenemem, void* ecx, int, int)>(ENGINE_BASE + 0x3049B4)
									(game::GSceneMem, nullptr, 28, 8));

								if (list_mem)
								{
									// FBspDrawList::FBspDrawList
									list_out = shared::utils::hook::call<game::FBspDrawList * __fastcall(game::FBspDrawList * pthis, void* ecx, game::UModel*, int)>(ENGINE_BASE + 0x16C7)
										(list_mem, nullptr, RenderState->Model, Node->iSection);
								}

								auto BspDrawLists = RenderState->BspDrawLists;
								auto iSection = Node->iSection;
								BspDrawLists[iSection] = list_out;

								game::TList<int>* section_list_mem = nullptr;
								// FMemStack::PushBytes(&GSceneMem, 8, 8);
								section_list_mem = static_cast<game::TList<int>*>(shared::utils::hook::call<void* __fastcall(void* gscenemem, void* ecx, int, int)>(ENGINE_BASE + 0x3049B4)
									(game::GSceneMem, nullptr, 8, 8));

								if (section_list_mem)
								{
									auto SectionDrawList = RenderState->SectionDrawList;
									section_list_mem->Element = Node->iSection;
									section_list_mem->Next = SectionDrawList;
								}

								RenderState->SectionDrawList = section_list_mem;


								//RenderState->BspDrawLists[Node.iSection] = new(game::GSceneMem) game::FBspDrawList(RenderState->Model, Node.iSection);
								//RenderState->SectionDrawList = new(game::GSceneMem) TList<INT>(Node.iSection, RenderState->SectionDrawList);
							}

							//RenderState->BspDrawLists[Node.iSection]->AddNode(node_idx, nullptr, 0, nullptr, 0, RenderState->SceneNode);
							// void __thiscall FBspDrawList::AddNode(FBspDrawList *this, int NodeIndex, FDynamicLight **InDynamicLights, FProjectorRenderInfo *NumDynamicLights, FProjectorRenderInfo **InDynamicProjectors, int NumDynamicProjectors, FLevelSceneNode *SceneNode)
							// 5227



							//shared::utils::hook::call<void* __fastcall(game::FBspDrawList* pthis, void* ecx, int NodeIndex, void** InDynamicLights, int NumDynamicLights, void** InDynamicProjectors, int NumDynamicProjectors, game::FLevelSceneNode* SceneNode)>(ENGINE_BASE + 0x5227)
							//	(RenderState->BspDrawLists[Node->iSection], nullptr, node_idx, nullptr, 0, nullptr, 0, RenderState->SceneNode);

							// this is what the func above does minus the actor crap
							const auto pthis = RenderState->BspDrawLists[Node->iSection];
							game::FBspNode& nn = pthis->Model->Nodes.Data[node_idx];

							if (nn.iFirstVertex != -1)
							{
								pthis->Nodes[pthis->NumNodes++] = node_idx;
								pthis->NumTriangles += nn.NumVertices - 2;
							}

							int xx = 1;
						}
					}
				}
			}
		}

		if (im->m_manual_node_forcing)
		{
			auto node_idx = std::clamp(im->m_manual_node_forcing_index, 0, RenderState->Model->Nodes.ArrayNum - 1);

			// ProcessNode -- FDynamicLight, FProjectorRenderInfo
			/*shared::utils::hook::call<void __cdecl(game::FRenderState* RenderState, int iNode, void** a3, int NumDynamicLights, void** a5, int NumDynamicProjectors)>
				(ENGINE_BASE + 0x1F6A20)(RenderState, node_idx, nullptr, 0, nullptr, 0);*/

			// noclipping into a wall fills outside actors 

			{
				RenderState->ActiveZoneMask = 1;
				RenderState->ActiveZones->Element = 0;
				RenderState->ActiveZones->Next = nullptr;
			}

			{
				game::FBspNode* Node = &RenderState->Model->Nodes.Data[node_idx];
				game::FBspSurf* Surf = &RenderState->Model->Surfs.Data[Node->iSurf];

				//for (auto i = 0; i < RenderState->Model->Nodes.ArrayNum; i++) 
				//{
				//	RenderState->Model->Nodes.Data[i].ZoneMask = -1;
				//	RenderState->Model->Nodes.Data[i].ExclusiveSphereBound.dist += 10000.0f;
				//	RenderState->Model->Nodes.Data[i].iRenderBound = -1;

				//	// setting izone for all to 0 makes everything visible but also kills me lmao
				//	//RenderState->Model->Nodes.Data[i].iZone[0] = 0;
				//	//RenderState->Model->Nodes.Data[i].iZone[1] = 0;
				//}

				// can be used to visualize portal surfaces
				/*for (auto i = 0; i < RenderState->Model->Surfs.ArrayNum; i++)
				{
					if (RenderState->Model->Surfs.Data[i].PolyFlags & game::PF_Portal)
					{
						RenderState->Model->Surfs.Data[i].PolyFlags &= ~game::PF_Invisible;
					}
				}*/



				/*for (auto i = 0; i < 64 ; i++)
				{
					RenderState->Zones[i].Portals = nullptr;
				}
				RenderState->ActiveZoneMask = -1;*/



				if (Node && Surf && Surf->Material && Surf->Material->to_vtbl && Surf->Material->to_vtbl->mat_vtbl)
				{
					if (Surf->Material && Surf->Material->to_vtbl->mat_vtbl->RequiresSorting(Surf->Material))
					{
						int x = 0;
						// Add the node to the translucent draw list.

						/*FTranslucentDrawItem	TranslucentNodeItem;
						TranslucentNodeItem.BSP = 1;
						TranslucentNodeItem.iNode = iNode;
						TranslucentNodeItem.NumDynamicLights = NumDynamicLights;
						TranslucentNodeItem.NumDynamicProjectors = NumDynamicProjectors;*/
					}
					else
					{
						// Add the node to it's section's draw list.

						if (!RenderState->BspDrawLists[Node->iSection])
						{
							game::FBspDrawList* list_mem = nullptr;
							game::FBspDrawList* list_out = nullptr;

							// FMemStack::PushBytes(&GSceneMem, 28, 8);
							list_mem = static_cast<game::FBspDrawList*>(shared::utils::hook::call<void* __fastcall(void* gscenemem, void* ecx, int, int)>(ENGINE_BASE + 0x3049B4)
								(game::GSceneMem, nullptr, 28, 8));

							if (list_mem)
							{
								// FBspDrawList::FBspDrawList
								list_out = shared::utils::hook::call<game::FBspDrawList * __fastcall(game::FBspDrawList * pthis, void* ecx, game::UModel*, int)>(ENGINE_BASE + 0x16C7)
									(list_mem, nullptr, RenderState->Model, Node->iSection);
							}

							auto BspDrawLists = RenderState->BspDrawLists;
							auto iSection = Node->iSection;
							BspDrawLists[iSection] = list_out;

							game::TList<int>* section_list_mem = nullptr;
							// FMemStack::PushBytes(&GSceneMem, 8, 8);
							section_list_mem = static_cast<game::TList<int>*>(shared::utils::hook::call<void* __fastcall(void* gscenemem, void* ecx, int, int)>(ENGINE_BASE + 0x3049B4)
								(game::GSceneMem, nullptr, 8, 8));

							if (section_list_mem)
							{
								auto SectionDrawList = RenderState->SectionDrawList;
								section_list_mem->Element = Node->iSection;
								section_list_mem->Next = SectionDrawList;
							}

							RenderState->SectionDrawList = section_list_mem;


							//RenderState->BspDrawLists[Node.iSection] = new(game::GSceneMem) game::FBspDrawList(RenderState->Model, Node.iSection);
							//RenderState->SectionDrawList = new(game::GSceneMem) TList<INT>(Node.iSection, RenderState->SectionDrawList);
						}

						//RenderState->BspDrawLists[Node.iSection]->AddNode(node_idx, nullptr, 0, nullptr, 0, RenderState->SceneNode);
						// void __thiscall FBspDrawList::AddNode(FBspDrawList *this, int NodeIndex, FDynamicLight **InDynamicLights, FProjectorRenderInfo *NumDynamicLights, FProjectorRenderInfo **InDynamicProjectors, int NumDynamicProjectors, FLevelSceneNode *SceneNode)
						// 5227

						shared::utils::hook::call<void* __fastcall(game::FBspDrawList* pthis, void* ecx, int NodeIndex, void** InDynamicLights, int NumDynamicLights, void** InDynamicProjectors, int NumDynamicProjectors, game::FLevelSceneNode* SceneNode)>(ENGINE_BASE + 0x5227)
							(RenderState->BspDrawLists[Node->iSection], nullptr, node_idx, nullptr, 0, nullptr, 0, RenderState->SceneNode);

						int xx = 1;
					}
				}
			}
		}
	}

	HOOK_RETN_PLACE_DEF(post_bsp_traversal_retn_addr);
	__declspec(naked) void post_bsp_traversal_stub()
	{
		__asm
		{
			pushad;
			push	eax;
			call	post_bsp_traversal_hk;
			add		esp, 4;
			popad;

			mov     ecx, [eax];
			mov     edx, [ecx + 4];
			jmp		post_bsp_traversal_retn_addr;
		}
	}

	// --------------------------

	void post_get_view_frustum_hk(game::FRenderState* RenderState, game::FLevelSceneNode* node, game::FConvexVolume* frustum)
	{
		/*if (node->ViewZone > 15)
		{
			node->ViewZone = 0;
		}*/

		const auto& im = imgui::get();

		if (im->m_cull_disable_frustum)
		{
			for (auto i = 0u; i < 5; i++) {
				frustum->BoundingPlanes[i].dist += im->m_cull_frustum_tweak_distance_offset; //10000.0f;
			}
		}
		
	}

	HOOK_RETN_PLACE_DEF(post_get_view_frustum_retn_addr);
	__declspec(naked) void post_get_view_frustum_stub()
	{
		__asm
		{
			pushad;
			push	eax;
			mov     edx, [ebp + 8];
			push	edx;
			lea     edx, [ebp - 0xA60];
			push    edx;
			call	post_get_view_frustum_hk;
			add		esp, 12;
			popad;

			mov		[edi], eax;
			mov		[edi + 4], esi;
			jmp		post_get_view_frustum_retn_addr;
		}
	}

	// ---

	patches::patches()
	{
		p_this = this;

		// init addon textures
		//init_texture_addons();

		// uhm .. this crashes the game when pressing num7 or 8 on the numpad lmao (just 
		shared::common::remix_api::initialize(begin_scene_cb, end_scene_cb, present_scene_cb, true);



		shared::utils::hook(ENGINE_BASE + 0x1FC3FE, post_bsp_traversal_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(post_bsp_traversal_retn_addr, ENGINE_BASE + 0x1FC403);


		// this fixes frustum culling
		shared::utils::hook(ENGINE_BASE + 0x1FDB19, post_get_view_frustum_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(post_get_view_frustum_retn_addr, ENGINE_BASE + 0x1FDB1E);

		// ENGINE + 0x1F6ABB (nop2)
		// ^ + 0x1F6ACD+3 to byte 01 (to mov [ebp-20],00000001)

		// ^ + 0x1F6AEC (nop5)
		// ^ + 0x1F6AF1 set bytes to B8 01 00 00 00 (mov eax,00000001)


		printf("[Module] patches loaded.\n");
	}
}
