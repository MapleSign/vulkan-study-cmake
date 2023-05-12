#include "GLTFHelper.h"

void createNormals(const std::vector<uint32_t>& indices, const std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals) {
	// Need to compute the normals
	std::vector<glm::vec3> geonormal(positions.size());
	for (size_t i = 0; i < indices.size(); i += 3)
	{
		uint32_t ind0 = indices[i + 0];
		uint32_t ind1 = indices[i + 1];
		uint32_t ind2 = indices[i + 2];
		const auto& pos0 = positions[ind0];
		const auto& pos1 = positions[ind1];
		const auto& pos2 = positions[ind2];
		const auto  v1 = glm::normalize(pos1 - pos0);  // Many normalize, but when objects are really small the
		const auto  v2 = glm::normalize(pos2 - pos0);  // cross will go below nv_eps and the normal will be (0,0,0)
		const auto  n = glm::cross(v1, v2);
		geonormal[ind0] += n;
		geonormal[ind1] += n;
		geonormal[ind2] += n;
	}
	for (auto& n : geonormal)
		n = glm::normalize(n);
	normals.insert(normals.end(), geonormal.begin(), geonormal.end());
}

void createTangents(
	const std::vector<uint32_t>& indices, const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals, 
	const std::vector<glm::vec2>& texCoords, std::vector<glm::vec3>& tangents, std::vector<glm::vec3>& bitangents)
{
	for (size_t i = 0; i < indices.size(); i += 3)
	{
		// local index
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		const auto& p0 = positions[i0];
		const auto& p1 = positions[i1];
		const auto& p2 = positions[i2];

		const auto& uv0 = texCoords[i0];
		const auto& uv1 = texCoords[i1];
		const auto& uv2 = texCoords[i2];

		glm::vec3 e1 = p1 - p0;
		glm::vec3 e2 = p2 - p0;

		glm::vec2 duvE1 = uv1 - uv0;
		glm::vec2 duvE2 = uv2 - uv0;

		float r = 1.0f;
		float a = duvE1.x * duvE2.y - duvE2.x * duvE1.y;
		if (fabs(a) > 0)  // Catch degenerated UV
		{
			r = 1.0f / a;
		}

		glm::vec3 t = (e1 * duvE2.y - e2 * duvE1.y) * r;
		glm::vec3 b = (e2 * duvE1.x - e1 * duvE2.x) * r;


		tangents[i0] += t;
		tangents[i1] += t;
		tangents[i2] += t;

		bitangents[i0] += b;
		bitangents[i1] += b;
		bitangents[i2] += b;
	}

	for (size_t i = 0; i < normals.size(); ++i) {
		auto& n = normals[i];
		auto& t = tangents[i];
		auto& b = bitangents[i];

		// Gram-Schmidt orthogonalize
		glm::vec3 otangent = glm::normalize(t - (glm::dot(n, t) * n));

		// In case the tangent is invalid
		if (otangent == glm::vec3(0, 0, 0))
		{
			if (abs(n.x) > abs(n.y))
				otangent = glm::vec3(n.z, 0, -n.x) / sqrt(n.x * n.x + n.z * n.z);
			else
				otangent = glm::vec3(0, -n.z, n.y) / sqrt(n.y * n.y + n.z * n.z);
		}

		// Calculate handedness
		float handedness = (glm::dot(glm::cross(n, t), b) < 0.f) ? 1.f : -1.f;
		t = otangent;
		b = glm::vec3(handedness);
	}
}

void importGLTFMaterial(GltfMaterial& mat, const tinygltf::Material& tMat) {
	mat.alphaCutoff = static_cast<float>(tMat.alphaCutoff);
	mat.alphaMode = tMat.alphaMode == "MASK" ? 1 : (tMat.alphaMode == "BLEND" ? 2 : 0);
	mat.doubleSided = tMat.doubleSided ? 1 : 0;
	mat.emissiveFactor = tMat.emissiveFactor.size() == 3 ?
		glm::vec3(tMat.emissiveFactor[0], tMat.emissiveFactor[1], tMat.emissiveFactor[2]) :
		glm::vec3(0.f);
	mat.emissiveTexture = tMat.emissiveTexture.index;
	mat.normalTexture = tMat.normalTexture.index;
	mat.normalTextureScale = static_cast<float>(tMat.normalTexture.scale);
	mat.occlusionTexture = tMat.occlusionTexture.index;
	mat.occlusionTextureStrength = static_cast<float>(tMat.occlusionTexture.strength);

	// PbrMetallicRoughness
	auto& tPbr = tMat.pbrMetallicRoughness;
	mat.pbrBaseColorFactor =
		glm::vec4(tPbr.baseColorFactor[0], tPbr.baseColorFactor[1], tPbr.baseColorFactor[2], tPbr.baseColorFactor[3]);
	mat.pbrBaseColorTexture = tPbr.baseColorTexture.index;
	mat.pbrMetallicFactor = static_cast<float>(tPbr.metallicFactor);
	mat.pbrMetallicRoughnessTexture = tPbr.metallicRoughnessTexture.index;
	mat.pbrRoughnessFactor = static_cast<float>(tPbr.roughnessFactor);

	// KHR_materials_pbrSpecularGlossiness
	if (tMat.extensions.find(KHR_MATERIALS_PBRSPECULARGLOSSINESS_EXTENSION_NAME) != tMat.extensions.end())
	{
		mat.shadingModel = 1;

		const auto& ext = tMat.extensions.find(KHR_MATERIALS_PBRSPECULARGLOSSINESS_EXTENSION_NAME)->second;
		getVec4(ext, "diffuseFactor", mat.khrDiffuseFactor);
		getFloat(ext, "glossinessFactor", mat.khrGlossinessFactor);
		getVec3(ext, "specularFactor", mat.khrSpecularFactor);
		getTexId(ext, "diffuseTexture", mat.khrDiffuseTexture);
		getTexId(ext, "specularGlossinessTexture", mat.khrSpecularGlossinessTexture);
	}

	// KHR_materials_clearcoat
	if (tMat.extensions.find(KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME) != tMat.extensions.end())
	{
		const auto& ext = tMat.extensions.find(KHR_MATERIALS_CLEARCOAT_EXTENSION_NAME)->second;
		getFloat(ext, "clearcoatFactor", mat.clearcoatFactor);
		getTexId(ext, "clearcoatTexture", mat.clearcoatTexture);
		getFloat(ext, "clearcoatRoughnessFactor", mat.clearcoatRoughness);
		getTexId(ext, "clearcoatRoughnessTexture", mat.clearcoatRoughnessTexture);
	}

	// KHR_materials_transmission
	if (tMat.extensions.find(KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME) != tMat.extensions.end())
	{
		const auto& ext = tMat.extensions.find(KHR_MATERIALS_TRANSMISSION_EXTENSION_NAME)->second;
		getFloat(ext, "transmissionFactor", mat.transmissionFactor);
		getTexId(ext, "transmissionTexture", mat.transmissionTexture);
	}

	// KHR_materials_ior
	if (tMat.extensions.find(KHR_MATERIALS_IOR_EXTENSION_NAME) != tMat.extensions.end())
	{
		const auto& ext = tMat.extensions.find(KHR_MATERIALS_IOR_EXTENSION_NAME)->second;
		getFloat(ext, "ior", mat.ior);
	}
}
