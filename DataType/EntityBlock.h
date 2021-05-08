#pragma once

#include "../FrotbiteCullingSystemCore.h"

#include "Math/AABB.h"
#include "Math/Vector.h"
#include "TransformData.h"



namespace culling
{

	//This code doesn't consider Memory alignment optimzation.
#ifdef ENABLE_SCREEN_SAPCE_AABB_CULLING
	inline static constexpr size_t ENTITY_COUNT_IN_ENTITY_BLOCK = (4096 - sizeof(unsigned int)) /
		(
			sizeof(Vector4) +
			sizeof(char) +
			sizeof(void*) +
			sizeof(AABB)
			) - 1;
#else
	inline static constexpr size_t ENTITY_COUNT_IN_ENTITY_BLOCK = (4096 - sizeof(unsigned int)) /
		(
			sizeof(Vector4) +
			sizeof(char) +
			sizeof(void*) 
			) - 3; //offset
#endif
		/// <summary>
		/// EntityBlock size should be less 4KB(Page size) for Block data being allocated in a page
		/// </summary>
	struct EntityBlock
	{
		/// <summary>
		/// Why align to 32byte?
		/// To set mIsVisibleBitflag, We use _m256
		/// </summary>
		alignas(32) char mIsVisibleBitflag[ENTITY_COUNT_IN_ENTITY_BLOCK];

		//SoA (Structure of Array) !!!!!! for performance 

		/// <summary>
		/// x, y, z : components is position of entity
		/// w : component is radius of entity's sphere bound, should be negative!!!!!!!!!!!!!!
		/// 
		/// This will be used for linearlly Frustum intersection check
		/// 
		/// IMPORTANT : you should pass nagative value of radius!!!!!!!!!!!!!!!!!!!!!!!!!!!
		/// 
		/// Why align to 32byte?
		/// To set mIsVisibleBitflag, We use _m256
		/// 
		/// If Size of mIsVisibleBitflag isn't multiples of 256bit,
		/// Setting mIsVisibleBitflag will make mPositions value dirty
		/// </summary>
		alignas(32) Vector4 mPositions[ENTITY_COUNT_IN_ENTITY_BLOCK];

#ifdef ENABLE_SCREEN_SAPCE_AABB_CULLING
		/// <summary>
		/// Size of AABB is 32 byte.
		/// 2 AABB is 64 byte.
		/// 
		/// i recommend AABB's point type is Vector4
		/// because when multiply Matrix4x4 with point for calculating ScreenSpace AABB, we can use SIMD if point is vector
		/// 
		/// It's cache friendlly!!
		/// </summary>
		AABB mWorldAABB[ENTITY_COUNT_IN_ENTITY_BLOCK];
#endif

		
		
		/// <summary>
		/// mIsVisibleBitflag is stored through two __m128
		/// 
		/// So if this variable isn't aligned to 256bit,
		/// To prevent that sttoring mIsVisibleBitflag code set value to this variable
		/// 
		/// </summary>
		void* mRenderer[ENTITY_COUNT_IN_ENTITY_BLOCK];
		//EntityHandle mHandles[ENTITY_COUNT_IN_ENTITY_BLOCK];

		/// <summary>
		/// this variable is only used to decide whether to free this EntityBlock
		/// </summary>
		unsigned int mCurrentEntityCount;
	};

	/// <summary>
	/// Size of Entity block should be less than 4kb(page size)
	/// </summary>
	static_assert(sizeof(EntityBlock) < 4096);

	/// <summary>
	/// ENTITY_COUNT_IN_ENTITY_BLOCK should be even number
	/// </summary>
	static_assert(ENTITY_COUNT_IN_ENTITY_BLOCK > 0 && ENTITY_COUNT_IN_ENTITY_BLOCK % 2 == 0);
}