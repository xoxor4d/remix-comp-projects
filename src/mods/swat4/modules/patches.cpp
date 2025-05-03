#include "std_include.hpp"
#include "patches.hpp"

#include "imgui.hpp"
#include "shared/common/flags.hpp"
#include "shared/common/remix.hpp"
#include "shared/common/remix_api.hpp"

// extended anti culling 0x1FBD97 jmp
// ^ 0x1FBDCA jmp

// disable backface culling without issues:
// 0x1F6AD4 jmp (0xEB)



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

	void pre_vis_hk([[maybe_unused]] game::FRenderState* RenderState)
	{
#if 0
		const auto im = imgui::get();
		if (im->m_debug_stuff)
		{

			RenderState->SceneNode->InvisibleZone = -1;
			for (auto a = RenderState->ActiveZones; a && RenderState->ActiveZones->Element >= 0; a = RenderState->ActiveZones->Next)
			{
				for (auto i = 0; i < 30; i++)
				{
					RenderState->Zones[a->Element].Portals->Element->BoundingPlanes[i].dist = 10000;
					RenderState->Zones[a->Element].Portals->Element->BoundingPlanes[i].xyz[0] = 0;
					RenderState->Zones[a->Element].Portals->Element->BoundingPlanes[i].xyz[1] = 1;
					RenderState->Zones[a->Element].Portals->Element->BoundingPlanes[i].xyz[2] = 1;
				}
			}
		}
#endif
	}

	HOOK_RETN_PLACE_DEF(pre_vis_retn_addr);
	__declspec(naked) void pre_vis_stub()
	{
		__asm
		{
			pushad;
			lea     eax, [ebp - 0xA60];
			push	eax; // RenderState
			call	pre_vis_hk;
			add		esp, 4;
			popad;

			mov		[ecx], edx;
			mov     ecx, [ebp - 0x4C];
			jmp		pre_vis_retn_addr;
		}
	}

	// --------------------------

	std::vector<int> g_forcedLeaves;
	std::vector<int> g_forcedNodes;

	bool is_point_inside_box(const game::FBox& box, const Vector& pt)
	{
		return box.IsValid &&
			pt.x >= box.Min.x && pt.x <= box.Max.x &&
			pt.y >= box.Min.y && pt.y <= box.Max.y &&
			pt.z >= box.Min.z && pt.z <= box.Max.z;
	}

	Vector closest_point_on_box(const game::FBox& box, const Vector& pt)
	{
		Vector result;
		result.x = std::clamp(pt.x, box.Min.x, box.Max.x);
		result.y = std::clamp(pt.y, box.Min.y, box.Max.y);
		result.z = std::clamp(pt.z, box.Min.z, box.Max.z);
		return result;
	}

	struct FLeafAndNodeIndices
	{
		std::vector<int> Leaves;
		std::vector<int> Nodes;
	};

	FLeafAndNodeIndices collect_leaves_and_nodes_within_radius(const game::FRenderState& RenderState, const Vector& player_pos, const float radius)
	{
		FLeafAndNodeIndices result;
		const float radius_squared = radius * radius;

		// Collect leaves
		for (INT iLeaf = 0; iLeaf < RenderState.Model->Leaves.ArrayNum; iLeaf++)
		{
			game::FBox leaf_bounds;
			auto found_bounds = false;

			for (INT iNode = 0; iNode < RenderState.Model->Nodes.ArrayNum; iNode++)
			{
				auto* n = &RenderState.Model->Nodes.Data[iNode];
				if (n->iLeaf[0] == iLeaf || n->iLeaf[1] == iLeaf)
				{
					if (n->iRenderBound != -1) // INDEX_NONE
					{
						leaf_bounds = RenderState.Model->Bounds.Data[n->iRenderBound];
						found_bounds = true;
						break;
					}
				}
			}

			if (found_bounds && leaf_bounds.IsValid)
			{
				if (is_point_inside_box(leaf_bounds, player_pos)) {
					result.Leaves.push_back(iLeaf);
				}
				else
				{
					Vector closest_point = closest_point_on_box(leaf_bounds, player_pos);
					const float distance_squared = (closest_point - player_pos).LengthSqr();

					if (distance_squared <= radius_squared) {
						result.Leaves.push_back(iLeaf);
					}
				}
			}
		}

		// Collect nodes
		for (auto inode = 0; inode < RenderState.Model->Nodes.ArrayNum; inode++)
		{
			game::FBspNode& node = RenderState.Model->Nodes.Data[inode];
			if (node.iRenderBound != -1 && node.iSurf != -1 && node.iSection != -1 && node.iSection < RenderState.Model->Sections.ArrayNum) // INDEX_NONE
			{
				game::FBox node_bounds = RenderState.Model->Bounds.Data[node.iRenderBound];

				if (is_point_inside_box(node_bounds, player_pos)) {
					result.Nodes.push_back(inode);
				}
				else
				{
					Vector closest_point = closest_point_on_box(node_bounds, player_pos);
					const float distance_squared = (closest_point - player_pos).LengthSqr();

					if (distance_squared <= radius_squared) {
						result.Nodes.push_back(inode);
					}
				}
			}
		}

		return result;
	}

	void force_node_bspdrawlist(game::FRenderState* RenderState, const int node_idx)
	{
		game::FBspNode* Node = &RenderState->Model->Nodes.Data[node_idx];
		game::FBspSurf* Surf = &RenderState->Model->Surfs.Data[Node->iSurf];

		// can be used to visualize portal surfaces
		/*for (auto i = 0; i < RenderState->Model->Surfs.ArrayNum; i++)
		{
			if (RenderState->Model->Surfs.Data[i].PolyFlags & game::PF_Portal)
			{
				RenderState->Model->Surfs.Data[i].PolyFlags &= ~game::PF_Invisible;
			}
		}*/

		/* later drawn like
		 for(TList<INT>* SectionList = RenderState.SectionDrawList;SectionList;SectionList = SectionList->Next)
			RenderState.BspDrawLists[SectionList->Element]->Render(RenderState.SceneNode,RI);*/

		if (Node && Surf && Surf->Material && Surf->Material->to_vtbl && Surf->Material->to_vtbl->mat_vtbl
			&& Node->iSection != -1 && !(Surf->PolyFlags & game::PF_Invisible))
		{
			if (Surf->Material && Surf->Material->to_vtbl->mat_vtbl->RequiresSorting(Surf->Material))
			{
				// Add the node to the translucent draw list.
				// ~ renders some weird nodes - does not seem to help with anything anyway
#if 0
				game::FTranslucentDrawItem TranslucentNodeItem;
				TranslucentNodeItem.BSP = 1;
				TranslucentNodeItem.iNode = node_idx;
				TranslucentNodeItem.NumDynamicLights = 0;
				TranslucentNodeItem.NumDynamicProjectors = 0;

				const auto tlist = static_cast<game::TList<game::FTranslucentDrawItem>*>(shared::utils::hook::call<void* __fastcall(void* gscenemem, void* ecx, int, int)>(ENGINE_BASE + 0x3049B4)
					(game::GSceneMem, nullptr, 32, 8));

				if (tlist)
				{
					const auto TranslucentDrawList = RenderState->TranslucentDrawList;
					memcpy(tlist, &TranslucentNodeItem, 0x1Cu);
					tlist->Next = TranslucentDrawList;
					RenderState->TranslucentDrawList = tlist;
				}
#endif
			}
			else
			{
				// Add the node to it's section's draw list.

				// RenderState->BspDrawLists[Node.iSection] = new(game::GSceneMem) game::FBspDrawList(RenderState->Model, Node.iSection);
				// RenderState->SectionDrawList = new(game::GSceneMem) TList<INT>(Node.iSection, RenderState->SectionDrawList);

				if (!RenderState->BspDrawLists[Node->iSection])
				{
					// FMemStack::PushBytes(&GSceneMem, 28, 8); (alloc new scene memory)
					game::FBspDrawList* list_mem = static_cast<game::FBspDrawList*>(shared::utils::hook::call<void* __fastcall(void* gscenemem, void* ecx, int, int)>(ENGINE_BASE + 0x3049B4)
						(game::GSceneMem, nullptr, 28, 8));

					if (list_mem)
					{
						// FBspDrawList::FBspDrawList (init drawlist)
						game::FBspDrawList* list_out = shared::utils::hook::call<game::FBspDrawList * __fastcall(game::FBspDrawList * pthis, void* ecx, game::UModel*, int)>(ENGINE_BASE + 0x16C7)
							(list_mem, nullptr, RenderState->Model, Node->iSection);

						if (list_out)
						{
							auto BspDrawLists = RenderState->BspDrawLists;
							auto iSection = Node->iSection;
							BspDrawLists[iSection] = list_out;

							//game::TList<int>* section_list_mem = nullptr;
							// FMemStack::PushBytes(&GSceneMem, 8, 8); (alloc new scene memory)
							game::TList<int>*  section_list_mem = static_cast<game::TList<int>*>(shared::utils::hook::call<void* __fastcall(void* gscenemem, void* ecx, int, int)>(ENGINE_BASE + 0x3049B4)
								(game::GSceneMem, nullptr, 8, 8));

							if (section_list_mem)
							{
								auto SectionDrawList = RenderState->SectionDrawList;
								section_list_mem->Element = Node->iSection;
								section_list_mem->Next = SectionDrawList;
								RenderState->SectionDrawList = section_list_mem;
							}
						}
					}
				}

				if (RenderState->BspDrawLists[Node->iSection])
				{
					const auto drawlist = RenderState->BspDrawLists[Node->iSection];

					bool exits = false;
					for (auto i = 0; i < drawlist->NumNodes; i++)
					{
						if (drawlist->Nodes[i] == node_idx)
						{
							exits = true;
							break;
						}
					}

					// adding a node a second time would crash the game when it tries to render the nodes
					// prob. because the game filters out duplicated nodes but we increased the numnodes so it goes out of bounds?

					if (!exits) // AddNode logic
					{
						game::FBspNode& nn = drawlist->Model->Nodes.Data[node_idx];
						if (nn.iFirstVertex != -1)
						{
							drawlist->Nodes[drawlist->NumNodes++] = node_idx;
							drawlist->NumTriangles += nn.NumVertices - 2;
						}
					}

					// ~ does the same as the above but that is the func used by the game which also adds dynamic lights etc - we dont need that
					// ~ RenderState->BspDrawLists[Node.iSection]->AddNode(node_idx, nullptr, 0, nullptr, 0, RenderState->SceneNode);
					// shared::utils::hook::call<void* __fastcall(game::FBspDrawList* pthis, void* ecx, int NodeIndex, void** InDynamicLights, int NumDynamicLights, void** InDynamicProjectors, int NumDynamicProjectors, game::FLevelSceneNode* SceneNode)>(ENGINE_BASE + 0x5227)
					//		(RenderState->BspDrawLists[Node->iSection], nullptr, node_idx, nullptr, 0, nullptr, 0, RenderState->SceneNode);
				}
			}
		}
	}

	void post_bsp_traversal_hk(game::FRenderState* RenderState)
	{
		const auto im = imgui::get();

		g_forcedLeaves.clear();
		g_forcedNodes.clear();

		Vector player_pos = RenderState->SceneNode->ViewOrigin;
		const FLeafAndNodeIndices indices = collect_leaves_and_nodes_within_radius(*RenderState, player_pos, im->m_render_area_dist);

		g_forcedLeaves = indices.Leaves;
		g_forcedNodes = indices.Nodes;

#if 0	// does nothing
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
#endif

		// does not check if node is already added -> will crash
		//if (im->m_enable_node_forcing)
		//{
		//	for (const auto& n : g_forcedNodes)
		//	{
		//		// ProcessNode -- FDynamicLight, FProjectorRenderInfo
		//		shared::utils::hook::call<void __cdecl(game::FRenderState* RenderState, int iNode, void** a3, int NumDynamicLights, void** a5, int NumDynamicProjectors)>
		//			(ENGINE_BASE + 0x1F6A20)(RenderState, node_idx, nullptr, 0, nullptr, 0);
		//	}
		//}

		if (im->m_enable_node_forcing)
		{
			for (const auto& node_idx : g_forcedNodes) {
				force_node_bspdrawlist(RenderState, node_idx);
			}
		}

		if (im->m_manual_node_forcing)
		{
			const auto node_idx = std::clamp(im->m_manual_node_forcing_index, 0, RenderState->Model->Nodes.ArrayNum - 1);
			force_node_bspdrawlist(RenderState, node_idx);
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

	void post_get_view_frustum_hk([[maybe_unused]] game::FRenderState* RenderState, [[maybe_unused]] game::FLevelSceneNode* node, game::FConvexVolume* frustum)
	{
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

	void ProcessLeaf_wrapper([[maybe_unused]] game::FBspNode* Node, [[maybe_unused]] game::FRenderState* RenderState, [[maybe_unused]] int iLeaf)
	{
		// ProcessLeaf
		shared::utils::hook::call<void __cdecl(game::FRenderState* RenderState, int iLeaf)>(ENGINE_BASE + 0x1FAF60)(RenderState, iLeaf);

		auto backside = Node->iLeaf[0] != iLeaf ? Node->iLeaf[0] : Node->iLeaf[1];
		if (backside != -1)
		{
			shared::utils::hook::call<void __cdecl(game::FRenderState* RenderState, int iLeaf)>(ENGINE_BASE + 0x1FAF60)(RenderState, backside);
		}
	}

	HOOK_RETN_PLACE_DEF(process_leaf_retn_addr);
	__declspec(naked) void process_leaf_stub()
	{
		__asm
		{
			pushad;
			push    ecx;
			push    edx;
			push	esi; // node
			call	ProcessLeaf_wrapper;
			add 	esp, 12;
			popad;

			
			jmp		process_leaf_retn_addr;
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


		shared::utils::hook(ENGINE_BASE + 0x1FDC5D, pre_vis_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(pre_vis_retn_addr, ENGINE_BASE + 0x1FDC62);

		shared::utils::hook(ENGINE_BASE + 0x1FC3FE, post_bsp_traversal_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(post_bsp_traversal_retn_addr, ENGINE_BASE + 0x1FC403);

		// this fixes frustum culling
		shared::utils::hook(ENGINE_BASE + 0x1FDB19, post_get_view_frustum_stub, HOOK_JUMP).install()->quick();
		HOOK_RETN_PLACE(post_get_view_frustum_retn_addr, ENGINE_BASE + 0x1FDB1E);


		//shared::utils::hook::nop(ENGINE_BASE + 0x1F6B10, 6); // this for anti backface
		//shared::utils::hook::nop(ENGINE_BASE + 0x1F75D7, 6);
		//shared::utils::hook::nop(ENGINE_BASE + 0x1F75E1, 6);
		//shared::utils::hook::nop(ENGINE_BASE + 0x1F75F7, 6);
		//shared::utils::hook::nop(ENGINE_BASE + 0x1F6E46, 6);
		//shared::utils::hook::nop(ENGINE_BASE + 0x1F6E51, 6);
		//shared::utils::hook::nop(ENGINE_BASE + 0x1FB33A, 6); // this to draw more objects


		// disable skybox because a. anticull1 needs it and b. remix will leak vram with the sky enabled 
		if (shared::common::flags::has_flag("disable_sky") || shared::common::flags::has_flag("anticull1")) {
			shared::utils::hook::conditional_jump_to_jmp(ENGINE_BASE + 0x1F6BF6);
		}

		// do not disable backface culling if flag is set
		if (!shared::common::flags::has_flag("backface_culling")) 
		{
			shared::utils::hook::set<BYTE>(ENGINE_BASE + 0x1F6AD4, 0xEB); // disable backface culling
			shared::utils::hook::nop(ENGINE_BASE + 0x1F6B10, 6); // anti cull 1
		}

		// cull even less
		if (shared::common::flags::has_flag("anticull1")) 
		{
			shared::utils::hook::set<BYTE>(ENGINE_BASE + 0x1FBD97, 0xEB);
			shared::utils::hook::conditional_jump_to_jmp(ENGINE_BASE + 0x1FBDCA);
		}

		// draw more static meshes in normally culled areas
		if (shared::common::flags::has_flag("anticull2")) {
			shared::utils::hook::nop(ENGINE_BASE + 0x1FB33A, 6);
		}


		// 1F726D nop2 (might cause issues (sky flicker issue))

		//shared::utils::hook(ENGINE_BASE + 0x1FC2CE, process_leaf_stub, HOOK_JUMP).install()->quick();
		//HOOK_RETN_PLACE(process_leaf_retn_addr, ENGINE_BASE + 0x1FC2D3);

		// draw more lights?
		// 1FB5E0 jmp (E9 D3 00 00 00 90)

		// this keeps lights active longer but there is a limit so newer ones wont draw?
		// jmp (0xE9 EB 00 00 00 90) to disable these lights completely
		shared::utils::hook::nop(ENGINE_BASE + 0x1FB5B6, 6); 
		
		// disable sky
		//  1F6BF6 -> E9 C1 09 00 00 90  .... or enabled .... 0F 85 C0 09 00 00

		std::cout << "[Module] patches loaded.\n";
	}
}
