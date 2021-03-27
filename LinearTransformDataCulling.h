#pragma once

#include "LinearTransformDataCulling_Core.h"
#include "DataStructure/EntityGridCell.h"
#include "DataStructure/EntityBlockViewer.h"

#include <memory>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>

#include <Vector4.h>
#include <Matrix4x4.h>

#include <Simple_SingleTon/Singleton.h>

namespace doom
{
	namespace graphics
	{
		struct SIMDFrustumPlanes
		{
			math::Vector4 mFrustumPlanes[8];
		};

		/// <summary>
		/// This is implementation of Data Oriented ViewFrustumCulling of Frostbite in 2011
		/// 
		/// This culling use SIMD DotProduct, So Check LightMath_Cpp/Matrix4x4Float_Aligned.inl
		///
		/// Feature :
		/// 1. Linear arrays scale great. All entity's position data is stored linearlly.
		/// 2. Tree based sturucture or other Acceleration strucutures isn;t required 
		/// 3. Frustum vs Sphere intersections check per loop
		/// 3. Threads solve intersections(blocks) parrallily
		/// 
		/// references : 
		/// https://www.gdcvault.com/play/1014491/Culling-the-Battlefield-Data-Oriented
		/// https://www.slideshare.net/DICEStudio/culling-the-battlefield-data-oriented-design-in-practice
		/// 
		/// 
		/// https://macton.smugmug.com/Other/2008-07-15-by-Eye-Fi/n-xmKDH/
		/// 
		/// </summary>
		class LinearTransformDataCulling
		{
		private:

			unsigned int mCameraCount;
			SIMDFrustumPlanes mSIMDFrustumPlanes[MAX_CAMERA_COUNT];

			bool bmIsInitializedEntityBlockPool{ false };
			std::unique_ptr<EntityBlock[]> mEntityBlockPool;
			std::vector<EntityBlock*> mFreeEntityBlockList{};
			EntityGridCell mEntityGridCell{};

			//static inline constexpr unsigned int M256_COUNT_OF_VISIBLE_ARRAY = 1 + ( (ENTITY_COUNT_IN_ENTITY_BLOCK * sizeof(decltype(*EntityBlock::mIsVisibleBitflag)) - 1) / 32 );
			/// <summary>
			/// will be used at CullBlockEntityJob
			/// </summary>
			//std::atomic<unsigned int> mAtomicCurrentBlockIndex;
			std::atomic<unsigned int> mFinishedCullJobBlockCount;
			/// <summary>
			/// 
			/// </summary>
			std::condition_variable mCullJobConditionVaraible;
			std::mutex mCullJobMutex;

			void ClearIsVisibleFlag();

			void RemoveEntityFromBlock(EntityBlock* ownerEntityBlock, unsigned int entityIndexInBlock);

			/// <summary>
			/// Block Swap removedblock with last block, and return swapped lastblock to pool
			/// </summary>
			void FreeEntityBlock(EntityBlock* freedEntityBlock);
			EntityBlock* GetNewEntityBlockFromPool();
		public:

			LinearTransformDataCulling();

			/// <summary>
			/// Remove Entity is nothing, Just decrement AllocatedEntityCountInBlocks
			/// And if AllocatedEntityCountInBlocks Of Block become zero, Remove the block using RemoveBlock function
			/// 
			/// Removing Entity isn't thread safe
			/// </summary>
			void RemoveEntityFromBlock(EntityBlockViewer* entityBlockViewer);
			
			/// <summary>
			/// increment EntityCountInBlock of TargetBlock.
			/// If All blocks is full, Get new block from Block Pool
			/// 
			/// Allocating New Entity isn't thread safe
			/// </summary>
			EntityBlockViewer AllocateNewEntity(const math::Vector3& position, float radius);


			// ////////////////////////////////////////////
			

			
			/// <summary>
			/// Solve View Frustum Culling from multiple threads
			/// 
			/// for pushing to job pool , this function is declared with static
			/// 
			/// CullBlockEntityJob never access to shared variable.
			/// So CullBlockEntityJob is thread safe.
			/// </summary>
			void CullBlockEntityJob(unsigned int blockIndex);
			/// <summary>
			/// Get Culling BlockEntity Jobs
			/// return type is std::vector<std::function<void()>>
			/// 
			/// So push returned all std::function to JobPool
			/// </summary>
			/// <returns></returns>
			std::vector<std::function<void()>> GetCullBlockEnityJobs();

			bool GetIsCullJobFinished();
			/// <summary>
			/// Call thread will stall until cull job of all entity block is finished
			/// </summary>
			bool WaitToFinishCullJobs();

		
			/// <summary>
			/// Reset cull job state before pushing cull job to job pool
			/// </summary>
			void ResetCullJobState();
			/// <summary>
			/// before Start solving culling, Update Every Camera's frustum plane
			/// </summary>
			/// <param name="frustumPlaneIndex"></param>
			/// <param name="mvpMatrix"></param>
			void UpdateFrustumPlane(unsigned int frustumPlaneIndex, const math::Matrix4x4& mvpMatrix);
		};
	}
}



