#pragma once

#include <tiny_gltf.h>

#include "Rendering/VulkanResource.h"

#define KHR_MATERIALS_PBRSPECULARGLOSSINESS_EXTENSION_NAME "KHR_materials_pbrSpecularGlossiness"
#define KHR_MATERIALS_SPECULAR_EXTENSION_NAME "KHR_materials_specular"
#define KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME "KHR_materials_clearcoat"
#define KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME "KHR_materials_transmission"
#define KHR_MATERIALS_IOR_EXTENSION_NAME "KHR_materials_ior"

// Return a vector of data for a tinygltf::Value
template <typename T>
inline std::vector<T> getVector(const tinygltf::Value& value)
{
    std::vector<T> result{ 0 };
    if (!value.IsArray())
        return result;
    result.resize(value.ArrayLen());
    for (int i = 0; i < value.ArrayLen(); i++)
    {
        result[i] = static_cast<T>(value.Get(i).IsNumber() ? value.Get(i).Get<double>() : value.Get(i).Get<int>());
    }
    return result;
}

inline void getFloat(const tinygltf::Value& value, const std::string& name, float& val)
{
    if (value.Has(name))
    {
        val = static_cast<float>(value.Get(name).Get<double>());
    }
}

inline void getInt(const tinygltf::Value& value, const std::string& name, int& val)
{
    if (value.Has(name))
    {
        val = value.Get(name).Get<int>();
    }
}

inline void getVec2(const tinygltf::Value& value, const std::string& name, glm::vec2& val)
{
    if (value.Has(name))
    {
        auto s = getVector<float>(value.Get(name));
        val = glm::vec2{ s[0], s[1] };
    }
}

inline void getVec3(const tinygltf::Value& value, const std::string& name, glm::vec3& val)
{
    if (value.Has(name))
    {
        auto s = getVector<float>(value.Get(name));
        val = glm::vec3{ s[0], s[1], s[2] };
    }
}

inline void getVec4(const tinygltf::Value& value, const std::string& name, glm::vec4& val)
{
    if (value.Has(name))
    {
        auto s = getVector<float>(value.Get(name));
        val = glm::vec4{ s[0], s[1], s[2], s[3] };
    }
}

inline void getTexId(const tinygltf::Value& value, const std::string& name, int& val)
{
    if (value.Has(name))
    {
        val = value.Get(name).Get("index").Get<int>();
    }
}

template<class T, class Component = float>
bool getAccessorData(const tinygltf::Model& tModel, const tinygltf::Accessor& tAcces, std::vector<T>& attribVec)
{
	size_t componentType = tAcces.componentType;
	int componentNum = tinygltf::GetNumComponentsInType(tAcces.type);

	size_t oldAttribVecSize = attribVec.size();
	attribVec.resize(oldAttribVecSize + tAcces.count);

	auto& tBufferView = tModel.bufferViews[tAcces.bufferView];
	auto& tBuffer = tModel.buffers[tBufferView.buffer];
	const uint8_t* data = &tBuffer.data[tBufferView.byteOffset + tAcces.byteOffset];
	const size_t dataStride = tAcces.ByteStride(tBufferView);

	for (size_t attribIndex = oldAttribVecSize; attribIndex < attribVec.size(); ++attribIndex) {
		T attrib{};
		switch (componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			for (int i = 0; i < componentNum; ++i)
				attrib[i] = static_cast<Component>(*(reinterpret_cast<const float*>(data) + i));
			break;
		case TINYGLTF_COMPONENT_TYPE_BYTE:
			for (int i = 0; i < componentNum; ++i)
				attrib[i] = static_cast<Component>(*(reinterpret_cast<const char*>(data) + i));
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			for (int i = 0; i < componentNum; ++i)
				attrib[i] = static_cast<Component>(*(reinterpret_cast<const unsigned char*>(data) + i));
			break;
		case TINYGLTF_COMPONENT_TYPE_SHORT:
			for (int i = 0; i < componentNum; ++i)
				attrib[i] = static_cast<Component>(*(reinterpret_cast<const short*>(data) + i));
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			for (int i = 0; i < componentNum; ++i)
				attrib[i] = static_cast<Component>(*(reinterpret_cast<const unsigned short*>(data) + i));
			break;
		default:
			throw std::runtime_error("Unhandled tinygltf component type!");
			break;
		}

		attribVec[attribIndex] = attrib;
		data += dataStride;
	}

	return true;
}

template<class T>
bool getAccessorDataScalar(const tinygltf::Model& tModel, const tinygltf::Accessor& tAcces, std::vector<T>& attribVec)
{
	size_t componentType = tAcces.componentType;
	int componentNum = tinygltf::GetNumComponentsInType(tAcces.type);

	size_t oldAttribVecSize = attribVec.size();
	attribVec.resize(oldAttribVecSize + tAcces.count);

	auto& tBufferView = tModel.bufferViews[tAcces.bufferView];
	auto& tBuffer = tModel.buffers[tBufferView.buffer];
	const uint8_t* data = &tBuffer.data[tBufferView.byteOffset + tAcces.byteOffset];
	const size_t dataStride = tAcces.ByteStride(tBufferView);

	for (size_t attribIndex = oldAttribVecSize; attribIndex < attribVec.size(); ++attribIndex) {
		T attrib{};
		switch (componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			attrib = static_cast<T>(*reinterpret_cast<const float*>(data));
			break;
		case TINYGLTF_COMPONENT_TYPE_INT:
			attrib = static_cast<T>(*reinterpret_cast<const int*>(data));
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			attrib = static_cast<T>(*reinterpret_cast<const unsigned int*>(data));
			break;
		case TINYGLTF_COMPONENT_TYPE_BYTE:
			attrib = static_cast<T>(*reinterpret_cast<const char*>(data));
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			attrib = static_cast<T>(*reinterpret_cast<const unsigned char*>(data));
			break;
		case TINYGLTF_COMPONENT_TYPE_SHORT:
			attrib = static_cast<T>(*reinterpret_cast<const short*>(data));
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			attrib = static_cast<T>(*reinterpret_cast<const unsigned short*>(data));
			break;
		default:
			throw std::runtime_error("Unhandled tinygltf component type!");
			break;
		}

		attribVec[attribIndex] = attrib;
		data += dataStride;
	}

	return true;
}

template<class T, class Component = float>
bool getAttribute(const tinygltf::Model& tModel, const tinygltf::Primitive& tPrimitive,
	std::vector<T>& attribVec, const std::string& attribName)
{
	auto& tAttr = tPrimitive.attributes;
	auto it = tAttr.find(attribName);
	if (it != tAttr.end()) {
		auto& tAcces = tModel.accessors[it->second];
		return getAccessorData<T, Component>(tModel, tAcces, attribVec);
	}
	else {
		return false;
	}
}

void createNormals(const std::vector<uint32_t>& indices, const std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals);

void createTangents(const std::vector<uint32_t>& indices, const std::vector<glm::vec3>& positions,
	const std::vector<glm::vec2> texCoords, std::vector<glm::vec3>& tangents, std::vector<glm::vec3>& bitangents);

void importGLTFMaterial(GltfMaterial& mat, const tinygltf::Material& tMat);
