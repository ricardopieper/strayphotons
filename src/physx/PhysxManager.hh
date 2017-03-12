#ifndef SP_PHYSXMANAGER_H
#define SP_PHYSXMANAGER_H

#include "Common.hh"
#include "ConvexHull.hh"
#include "core/CFunc.hh"
#include "threading/MutexedVector.hh"

#include <Ecs.hh>

#include <PxPhysicsAPI.h>
#include <extensions/PxDefaultErrorCallback.h>
#include <extensions/PxDefaultAllocator.h>
#include <unordered_map>
#include <thread>
#include <functional>

namespace sp
{
	class Model;
	class PhysxManager;

	struct PhysxConstraint
	{
		ecs::Entity parent;
		physx::PxRigidDynamic *child;
		physx::PxVec3 offset;
	};

	class ControllerHitReport : public physx::PxUserControllerHitReport
	{
	public:
		ControllerHitReport(PhysxManager *manager) : manager(manager) {}
		void onShapeHit(const physx::PxControllerShapeHit &hit);
		void onControllerHit(const physx::PxControllersHit &hit) {}
		void onObstacleHit(const physx::PxControllerObstacleHit &hit) {}
	private:
		PhysxManager *manager;
	};

	class PhysxManager
	{
		typedef std::list<PhysxConstraint> ConstraintList;

	public:
		PhysxManager();
		~PhysxManager();

		void Frame(double timeStep);
		bool LogicFrame(ecs::EntityManager &manager);
		void StartThread();
		void StartSimulation();
		void StopSimulation();
		void Lock();
		void Unlock();
		void ReadLock();
		void ReadUnlock();

		void CreateConstraint(ecs::Entity parent, physx::PxRigidDynamic *child, physx::PxVec3 offset);
		void RemoveConstraints(ecs::Entity parent, physx::PxRigidDynamic *child);

		ConvexHullSet *GetCachedConvexHulls(Model *model);

		struct ActorDesc
		{
			physx::PxTransform transform = physx::PxTransform(physx::PxVec3(0));
			physx::PxMeshScale scale = physx::PxMeshScale();
			bool dynamic = true;

			// only dynamic actors can be kinematic
			bool kinematic = false;
			//bool mergePrimitives = true;
		};

		physx::PxRigidActor *CreateActor(shared_ptr<Model> model, ActorDesc desc);
		void RemoveActor(physx::PxRigidActor *actor);
		physx::PxController *CreateController(physx::PxVec3 pos, float radius, float height, float density);
		void MoveController(physx::PxController *controller, double dt, physx::PxVec3 displacement);
		void TeleportController(physx::PxController *controller, physx::PxExtendedVec3 position);
		void ResizeController(physx::PxController *controller, const float height);
		void RemoveController(physx::PxController *controller);

		bool RaycastQuery(
			ecs::Entity &entity,
			const physx::PxVec3 origin,
			const physx::PxVec3 dir,
			const float distance,
			physx::PxRaycastBuffer &hit);

		bool SweepQuery(
			physx::PxRigidDynamic *actor,
			const physx::PxVec3 dir,
			const float distance);

		bool OverlapQuery(physx::PxRigidDynamic *actor);

		/**
		 * Translates a kinematic @actor by @transform.
		 * Throws a runtime_error if @actor is not kinematic
		 */
		void Translate(
			physx::PxRigidDynamic *actor,
			const physx::PxVec3 &transform);

		/**
		 * Collisions between this actor's shapes and other physx objects
		 * will be enabled (default).
		 */
		void EnableCollisions(physx::PxRigidActor *actor);

		/**
		 * Collisions between this actor's shapes and other physx objects
		 * will be disabled.
		 */
		void DisableCollisions(physx::PxRigidActor *actor);

		/**
		 * Enable or disable collisions for an actor.
		 */
		void ToggleCollisions(physx::PxRigidActor *actor, bool enabled);

		void ToggleDebug(bool enabled);
		bool IsDebugEnabled() const;

		/**
		 * Get the lines for the bounds of all physx objects
		 */
		MutexedVector<physx::PxDebugLine> GetDebugLines();

	private:
		void CreatePhysxScene();
		void DestroyPhysxScene();
		void CacheDebugLines();

		ConvexHullSet *BuildConvexHulls(Model *model);
		ConvexHullSet *LoadCollisionCache(Model *model);
		void SaveCollisionCache(Model *model, ConvexHullSet *set);

		physx::PxFoundation *pxFoundation = nullptr;
		physx::PxPhysics *physics = nullptr;
		physx::PxDefaultCpuDispatcher *dispatcher = nullptr;
		physx::PxDefaultErrorCallback defaultErrorCallback;
		physx::PxDefaultAllocator defaultAllocatorCallback;
		physx::PxCooking *pxCooking = nullptr;
		physx::PxControllerManager *manager = nullptr;

		physx::PxScene *scene = nullptr;
		bool simulate = false, exiting = false, resultsPending = false;
		bool debug = false;

		std::thread thread;

		ConstraintList constraints;

		std::unordered_map<string, ConvexHullSet *> cache;

		vector<physx::PxDebugLine> debugLines;
		std::mutex debugLinesMutex;
	};
}

#endif
