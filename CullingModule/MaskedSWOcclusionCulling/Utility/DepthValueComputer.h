#pragma once

#include "../../../EveryCullingCore.h"

#include "depthUtility.h"

namespace culling
{
	namespace DepthValueComputer
	{

		/*
		enum class eDepthType
		{
			MinDepth,
			MaxDepth
		};
		*/
		

		FORCE_INLINE extern void ComputeFlatTriangleMaxDepthValue
		(
			const size_t triangleCount,
			culling::M256F* const subTileMaxValues,
			const std::uint32_t tileOriginX, // 32x8 tile
			const std::uint32_t tileOriginY, // 32x8 tile

			const culling::M256F& vertexPoint1X, 
			const culling::M256F& vertexPoint1Y,
			const culling::M256F& vertexPoint1Z,

			const culling::M256F& vertexPoint2X,
			const culling::M256F& vertexPoint2Y,
			const culling::M256F& vertexPoint2Z,

			const culling::M256F& vertexPoint3X,
			const culling::M256F& vertexPoint3Y,
			const culling::M256F& vertexPoint3Z,

			const culling::M256I* const leftFaceEventOfTriangles, // eight _mm256i
			const culling::M256I* const rightFaceEventOfTriangles, // eight _mm256i

			const culling::M256F& minYOfTriangle,
			const culling::M256F& maxYOfTriangle,

			const std::uint32_t triangleMask
		)
		{
			const culling::M256I minYIntOfTriangles = _mm256_cvtps_epi32(_mm256_floor_ps(_mm256_add_ps(minYOfTriangle, _mm256_set1_ps(0.5f))));
			const culling::M256I maxYIntOfTriangles = _mm256_cvtps_epi32(_mm256_floor_ps(_mm256_add_ps(maxYOfTriangle, _mm256_set1_ps(0.5f))));

			const culling::M256I tileStartRowIndex = _mm256_max_epi32(_mm256_sub_epi32(minYIntOfTriangles, _mm256_set1_epi32(tileOriginY)), _mm256_set1_epi32(0));
			const culling::M256I tileEndRowIndex = _mm256_sub_epi32(_mm256_set1_epi32(TILE_HEIGHT), _mm256_max_epi32(_mm256_sub_epi32(_mm256_set1_epi32(tileOriginY + TILE_HEIGHT), maxYIntOfTriangles), _mm256_set1_epi32(0)));

			culling::M256F zPixelDxOfTriangles, zPixelDyOfTriangles;
			culling::depthUtility::ComputeDepthPlane
			(
				vertexPoint3X,
				vertexPoint3Y,
				vertexPoint3Z,

				vertexPoint1X,
				vertexPoint1Y,
				vertexPoint1Z,

				vertexPoint2X,
				vertexPoint2Y,
				vertexPoint2Z,

				zPixelDxOfTriangles,
				zPixelDyOfTriangles
			);

			const culling::M256F bbMinXV0 = _mm256_sub_ps(_mm256_cvtepi32_ps(_mm256_set1_epi32(tileOriginX)), vertexPoint3X);
			const culling::M256F bbMinYV0 = _mm256_sub_ps(_mm256_cvtepi32_ps(_mm256_set1_epi32(tileOriginY)), vertexPoint3Y);

			// depth value at tile origin ( 0, 0 )
			culling::M256F depthValueAtTileOriginPoint = _mm256_fmadd_ps(zPixelDxOfTriangles, bbMinXV0, _mm256_fmadd_ps(zPixelDyOfTriangles, bbMinYV0, vertexPoint3Z)); 

			//const culling::M256F zTileDx = _mm256_mul_ps(zPixelDx, _mm256_set1_ps((float)TILE_WIDTH));
			//const culling::M256F zTileDy = _mm256_mul_ps(zPixelDy, _mm256_set1_ps((float)TILE_HEIGHT));

			//zPlaneOffset = _mm256_add_ps(zPlaneOffset, _mm256_max_ps(_mm256_setzero_ps(), _mm256_mul_ps(zPixelDx, _mm256_set1_ps(SUB_TILE_WIDTH))));
			//zPlaneOffset = _mm256_add_ps(zPlaneOffset, _mm256_max_ps(_mm256_setzero_ps(), _mm256_mul_ps(zPixelDy, _mm256_set1_ps(SUB_TILE_HEIGHT))));
			

			// Compute Zmin and Zmax for the triangle (used to narrow the range for difficult tiles)
			const culling::M256F zMinOfTriangle = _mm256_min_ps(vertexPoint1Z, _mm256_min_ps(vertexPoint2Z, vertexPoint3Z));
			const culling::M256F zMaxOfTriangle = _mm256_max_ps(vertexPoint1Z, _mm256_max_ps(vertexPoint2Z, vertexPoint3Z));


			for (size_t triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
			{
				if ((triangleMask & (1 << triangleIndex)) != 0x00000000)
				{
					const culling::M256I& leftFaceEvent = leftFaceEventOfTriangles[triangleIndex];
					const culling::M256I& rightFaceEvent = rightFaceEventOfTriangles[triangleIndex];

					const float zPixelDx = reinterpret_cast<const float*>(&zPixelDxOfTriangles)[triangleIndex];
					const float zPixelDy = reinterpret_cast<const float*>(&zPixelDyOfTriangles)[triangleIndex];

					const culling::M256F zTriMax = _mm256_set1_ps((reinterpret_cast<const float*>(&zMaxOfTriangle))[triangleIndex]);
					const culling::M256F zTriMin = _mm256_set1_ps((reinterpret_cast<const float*>(&zMinOfTriangle))[triangleIndex]);

					// depth value at (0, 0) of subtiles
					culling::M256F zValueAtOriginPointOfSubTiles = _mm256_fmadd_ps(_mm256_set1_ps((reinterpret_cast<const float*>(&zPixelDxOfTriangles))[triangleIndex]), _mm256_setr_ps(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, 0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3),
						_mm256_fmadd_ps(_mm256_set1_ps((reinterpret_cast<const float*>(&zPixelDyOfTriangles))[triangleIndex]), _mm256_setr_ps(0, 0, 0, 0, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT, SUB_TILE_HEIGHT), _mm256_set1_ps((reinterpret_cast<const float*>(&depthValueAtTileOriginPoint))[triangleIndex])));

					//
					// 4 5 6 7   <-- _mm256i
					// 0 1 2 3
					// FaceEventOfSubTiles
					//
					// face event in each subtile
					culling::M256I leftFaceEventInSubTiles[SUB_TILE_HEIGHT]; // array 4 -> row index in subtile
					culling::M256I rightFaceEventInSubTiles[SUB_TILE_HEIGHT]; // array 4 -> row index in subtile

					for(int rowIndexInSubtiles = 0 ; rowIndexInSubtiles < SUB_TILE_HEIGHT ; rowIndexInSubtiles++)
					{
						const culling::M256I leftFaceEventOfRowIndexInSubTiles = _mm256_setr_epi32(reinterpret_cast<const int*>(&leftFaceEvent)[rowIndexInSubtiles], reinterpret_cast<const int*>(&leftFaceEvent)[rowIndexInSubtiles], reinterpret_cast<const int*>(&leftFaceEvent)[rowIndexInSubtiles], reinterpret_cast<const int*>(&leftFaceEvent)[rowIndexInSubtiles], reinterpret_cast<const int*>(&leftFaceEvent)[SUB_TILE_HEIGHT + rowIndexInSubtiles], reinterpret_cast<const int*>(&leftFaceEvent)[SUB_TILE_HEIGHT + rowIndexInSubtiles], reinterpret_cast<const int*>(&leftFaceEvent)[SUB_TILE_HEIGHT + rowIndexInSubtiles], reinterpret_cast<const int*>(&leftFaceEvent)[SUB_TILE_HEIGHT + rowIndexInSubtiles]);
						leftFaceEventInSubTiles[rowIndexInSubtiles] = _mm256_sub_epi32(leftFaceEventOfRowIndexInSubTiles, _mm256_setr_epi32(SUB_TILE_WIDTH * 0, SUB_TILE_WIDTH * 1, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, SUB_TILE_WIDTH * 0, SUB_TILE_WIDTH * 1, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3));

						const culling::M256I rightFaceEventOfRowIndexInSubTiles = _mm256_setr_epi32(reinterpret_cast<const int*>(&rightFaceEvent)[rowIndexInSubtiles], reinterpret_cast<const int*>(&rightFaceEvent)[rowIndexInSubtiles], reinterpret_cast<const int*>(&rightFaceEvent)[rowIndexInSubtiles], reinterpret_cast<const int*>(&rightFaceEvent)[rowIndexInSubtiles], reinterpret_cast<const int*>(&rightFaceEvent)[SUB_TILE_HEIGHT + rowIndexInSubtiles], reinterpret_cast<const int*>(&rightFaceEvent)[SUB_TILE_HEIGHT + rowIndexInSubtiles], reinterpret_cast<const int*>(&rightFaceEvent)[SUB_TILE_HEIGHT + rowIndexInSubtiles], reinterpret_cast<const int*>(&rightFaceEvent)[SUB_TILE_HEIGHT + rowIndexInSubtiles]);
						rightFaceEventInSubTiles[rowIndexInSubtiles] = _mm256_sub_epi32(rightFaceEventOfRowIndexInSubTiles, _mm256_setr_epi32(SUB_TILE_WIDTH * 0, SUB_TILE_WIDTH * 1, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3, SUB_TILE_WIDTH * 0, SUB_TILE_WIDTH * 1, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3));
					}

					for (int rowIndexInSubtiles = 0; rowIndexInSubtiles < SUB_TILE_HEIGHT; rowIndexInSubtiles++)
					{
						const culling::M256F leftFaceDepthValueOfRowIndexInSubTiles =
							_mm256_add_ps(_mm256_add_ps(_mm256_set1_ps(zPixelDy * rowIndexInSubtiles), zValueAtOriginPointOfSubTiles), _mm256_mul_ps(_mm256_cvtepi32_ps(leftFaceEventInSubTiles[rowIndexInSubtiles]), _mm256_set1_ps(zPixelDx)));

						const culling::M256F rightFaceDepthValueOfRowIndexInSubTiles =
							_mm256_add_ps(_mm256_add_ps(_mm256_set1_ps(zPixelDy * rowIndexInSubtiles), zValueAtOriginPointOfSubTiles), _mm256_mul_ps(_mm256_cvtepi32_ps(rightFaceEventInSubTiles[rowIndexInSubtiles]), _mm256_set1_ps(zPixelDx)));

						const culling::M256F maxZValueAtRowOfSubTiles = _mm256_max_ps(leftFaceDepthValueOfRowIndexInSubTiles, rightFaceDepthValueOfRowIndexInSubTiles);

						// TODO : consider zMin, zMax

						subTileMaxValues[triangleIndex] = _mm256_max_ps(subTileMaxValues[triangleIndex], maxZValueAtRowOfSubTiles);
					}
					
					/*
					/*
					z0 = _mm256_max_ps(_mm256_min_ps(z0, zTriMax), zTriMin);
					subTileMaxValues[triangleIndex] = z0;
					#1#


					// TODO : ScanLine DepthValue based on left, right event.
					//leftFaceEvent[triangleIndex]
					//rightFaceEvent[triangleIndex]

					const culling::M256F leftFaceZValueOfSubTiles = _mm256_mul_ps(zPixelDxOfTriangles, _mm256_cvtepi32_ps(_mm256_max_epi32(leftFaceEvent[triangleIndex], _mm256_set1_epi32(0))));
					const culling::M256F rightFaceZValueOfSubTiles = _mm256_mul_ps(zPixelDxOfTriangles, _mm256_cvtepi32_ps(_mm256_min_epi32(rightFaceEvent[triangleIndex], _mm256_set1_epi32(TILE_WIDTH))));

					const int startRowIndexOfTriangle = reinterpret_cast<const int*>(&tileStartRowIndex)[triangleIndex];
					const int endRowIndexOfTriangle = reinterpret_cast<const int*>(&tileEndRowIndex)[triangleIndex];

					for (int scanLineRowIndex = startRowIndexOfTriangle; scanLineRowIndex < endRowIndexOfTriangle; scanLineRowIndex++)
					{
						const float leftFaceZValue = reinterpret_cast<const float*>(&leftFaceZValueOfRows)[scanLineRowIndex];
						const float rightFaceZValue = reinterpret_cast<const float*>(&rightFaceZValueOfRows)[scanLineRowIndex];
					}
					*/
					

					
				}
				

			}

		}
		

	};
}


