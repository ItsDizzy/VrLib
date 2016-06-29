#pragma once

#include "../Component.h"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace vrlib
{
	namespace tien
	{
		class Scene;

		namespace components
		{
			class RigidBody : public Component, btMotionState
			{
				static btCollisionShape* emptyShape;
				float mass;
				void init(btDynamicsWorld* world);
				void updateCollider(btDynamicsWorld* world);

				friend class Scene;
				friend class Node;

				enum class Type
				{
					Static,
					Kinematic,
					Dynamic,
					Auto
				} type;
			public:
				virtual void getWorldTransform(btTransform & worldTrans) const override;
				virtual void setWorldTransform(const btTransform & worldTrans) override;

				btDynamicsWorld* world;

				RigidBody(float mass, Type type = Type::Auto);
				~RigidBody();
				btRigidBody* body;

				inline float getMass() const {return mass; };
				void setMass(float newMass);

				inline Type getType() const { return type; };
				void setType(Type newType);


				void resetMotion();
				void setLinearFactor(const glm::vec3 &axis) { body->setLinearFactor(btVector3(axis.x, axis.y, axis.z)); }
				void setAngularFactor(const glm::vec3 axis) { body->setAngularFactor(btVector3(axis.x, axis.y, axis.z)); };



			};
		}
	}
}