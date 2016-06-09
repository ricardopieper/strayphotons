#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ecs/Ecs.hh"
#include "ecs/components/Placement.hh"

namespace ECS
{
	glm::mat4 Placement::GetModelTransform(sp::EntityManager& manager)
	{
		glm::mat4 model;

		if (this->relativeTo != sp::Entity::Id())
		{
			sp::Assert(
				manager.Has<Placement>(this->relativeTo),
				"cannot be relative to something that does not have a Transform"
			);

			model = manager.Get<Placement>(this->relativeTo)->GetModelTransform(manager);
		}

		return model * this->position * this->rotate * this->scale;
	}

	void Placement::SetRelativeTo(sp::Entity ent)
	{
		if (!ent.Has<Placement>())
		{
			std::stringstream ss;
			ss << "Cannot set placement relative to " << ent
			   << " because it does not have a placement.";
			throw std::runtime_error(ss.str());
		}

		this->relativeTo = ent.GetId();
	}

	void Placement::Rotate(float radians, glm::vec3 axis)
	{
		this->rotate = glm::rotate(this->rotate, radians, axis);
	}

	void Placement::Translate(glm::vec3 xyz)
	{
		this->position = glm::translate(this->position, xyz);
	}

	void Placement::Scale(glm::vec3 xyz)
	{
		this->scale = glm::scale(this->scale, xyz);
	}
}