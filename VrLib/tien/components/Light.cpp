#include "Light.h"
#include "Transform.h"

#include <VrLib/gl/FBO.h>
#include <VrLib/Log.h>
#include <VrLib/json.h>
#include "../Node.h"
#include "../Scene.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using vrlib::Log;

namespace vrlib
{
	namespace tien
	{
		namespace components
		{
			struct CameraDirection
			{
				GLenum cubemapFace;
				glm::vec3 target;
				glm::vec3 up;
			};

			CameraDirection gCameraDirections[6] =
			{
				{ GL_TEXTURE_CUBE_MAP_POSITIVE_X, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
				{ GL_TEXTURE_CUBE_MAP_NEGATIVE_X, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
				{ GL_TEXTURE_CUBE_MAP_POSITIVE_Y, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
				{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
				{ GL_TEXTURE_CUBE_MAP_POSITIVE_Z, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f) },
				{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f) }
			};
			


			Light::Light(const vrlib::json::Value & json)
			{
				if (json["lighttype"] == "directional")	type = Type::directional;
				if (json["lighttype"] == "point")		type = Type::point;
				if (json["lighttype"] == "spot")		type = Type::spot;

				if (json["shadow"] == "none")			shadow = Shadow::none;
				if (json["shadow"] == "shadowmap")		shadow = Shadow::shadowmap;
				if (json["shadow"] == "shadowvolume")	shadow = Shadow::shadowvolume;

				intensity = json["intensity"];
				range = json["range"];
				for (int i = 0; i < 4; i++)
					color[i] = json["color"][i];

			}
			json::Value Light::toJson(json::Value &meshes) const
			{
				json::Value ret;
				ret["type"] = "light";

				switch (type)
				{
				case Type::directional:		ret["lighttype"] = "directional";	break;
				case Type::point:			ret["lighttype"] = "point";			break;
				case Type::spot:			ret["lighttype"] = "spot";			break;
				default:
					ret["lighttype"] = "error";
				}

				switch (shadow)
				{
				case Shadow::none:			ret["shadow"] = "none";				break;
				case Shadow::shadowmap:		ret["shadow"] = "shadowmap";		break;
				case Shadow::shadowvolume:	ret["shadow"] = "shadowvolume";		break;
				default:
					ret["shadow"] = "error";
				}

				ret["intensity"] = intensity;
				for (int i = 0; i < 4; i++)
					ret["color"].push_back(color[i]);

				ret["range"] = range;

				return ret;
			}
			Light::~Light()
			{
				if (shadowMapDirectional)
					delete shadowMapDirectional;
				shadowMapDirectional = nullptr;
			}


			void Light::generateShadowMap()
			{
				if (shadow != Shadow::shadowmap)
					return;

				if (type == Type::directional)
				{
					if (!shadowMapDirectional)
						shadowMapDirectional = new vrlib::gl::FBO(1024*4, 1024*4, true, 0, true); //shadowmap

					float size = 50.0f;

					glm::vec3 frustumCenter = node->getScene().frustum->getCenter(); //todo: cache?
					glm::vec3 eyePos = glm::vec3(glm::inverse(node->getScene().frustum->modelviewMatrix) * glm::vec4(0, 0, 0, 1));
					glm::vec3 dir = frustumCenter - eyePos;
					frustumCenter = eyePos + size * 0.5f * glm::normalize(dir);
					printf("Eyepos:\t%f\t%f\t%f\n", eyePos.x, eyePos.y, eyePos.z);
					printf("dir:   \t%f\t%f\t%f\n", dir.x, dir.y, dir.z);

					glm::vec3 lightPosition = frustumCenter + 50.0f * node->transform->position;
					projectionMatrix = glm::ortho(-size, size*2, -size, size*2, 0.0f, 250.0f); //TODO: auto generate
					//projectionMatrix = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.0f, 250.0f);
					modelViewMatrix = glm::lookAt(lightPosition, lightPosition - node->transform->position, glm::vec3(0, 1, 0));
					Scene& scene = node->getScene();

					shadowMapDirectional->bind();
					glViewport(0, 0, shadowMapDirectional->getWidth(), shadowMapDirectional->getHeight());
					glClearColor(1, 0, 0, 1);
					glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
					glDisable(GL_CULL_FACE);
					glCullFace(GL_BACK);

					for (components::Renderable::RenderContext* context : scene.renderContextsShadow)
					{
						context->frameSetup(projectionMatrix, modelViewMatrix);
						context->useCubemap(false);
					}

					for (Node* c : scene.renderables)
						c->getComponent<components::Renderable>()->drawShadowMap();

					shadowMapDirectional->unbind();
				}
				else if (type == Type::point)
				{
					if (!shadowMapDirectional)
						shadowMapDirectional = new vrlib::gl::FBO(1024*2, 1024*2, false, vrlib::gl::FBO::Type::ShadowCube); //shadowmap

					Scene& scene = node->getScene();

					glViewport(0, 0, shadowMapDirectional->getWidth(), shadowMapDirectional->getHeight());
					glEnable(GL_CULL_FACE);
					glCullFace(GL_BACK);

					for (int i = 0; i < 6; i++)
					{
						projectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, .1f, 50.0f);
						modelViewMatrix = glm::lookAt(node->transform->position, node->transform->position + gCameraDirections[i].target, gCameraDirections[i].up);
						shadowMapDirectional->bind(i);
						glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
						for (components::Renderable::RenderContext* context : scene.renderContextsShadow)
						{
							context->frameSetup(projectionMatrix, modelViewMatrix);
							context->useCubemap(true);
						}

						for (Node* c : scene.renderables)
							c->getComponent<components::Renderable>()->drawShadowMap();
					}

						shadowMapDirectional->unbind();
				}



			}

		}
	}
}