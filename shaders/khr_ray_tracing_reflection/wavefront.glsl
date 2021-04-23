struct hitPayload
{
	vec3 hitValue;
	int  depth;
	vec3 attenuation;
	int  done;
	vec3 rayOrigin;
	vec3 rayDir;
};

struct Vertex
{
	vec3 pos;
	vec3 nrm;
	vec3 color;
	vec2 texCoord;
};

struct WaveFrontMaterial
{
	vec3  ambient;
	vec3  diffuse;
	vec3  specular;
	vec3  transmittance;
	vec3  emission;
	float shininess;
	float ior;             // index of refraction
	float dissolve;        // 1 == opaque; 0 == fully transparent
	int   illum;           // illumination model (see http://www.fileformat.info/format/material/)
	int   textureId;
};

struct sceneDesc
{
	int  objId;
	int  txtOffset;
	mat4 transfo;
	mat4 transfoIT;
};

vec3 computeDiffuse(WaveFrontMaterial mat, vec3 lightDir, vec3 normal)
{
	// Lambertian
	float dotNL = max(dot(normal, lightDir), 0.0);
	vec3  c     = mat.diffuse * dotNL;
	if (mat.illum >= 1)
		c += mat.ambient;
	return c;
}

vec3 computeSpecular(WaveFrontMaterial mat, vec3 viewDir, vec3 lightDir, vec3 normal)
{
	if (mat.illum < 2)
		return vec3(0);

	// Compute specular only if not in shadow
	const float kPi        = 3.14159265;
	const float kShininess = max(mat.shininess, 4.0);

	// Specular
	const float kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);
	vec3        V                   = normalize(-viewDir);
	vec3        R                   = reflect(-lightDir, normal);
	float       specular            = kEnergyConservation * pow(max(dot(V, R), 0.0), kShininess);

	return vec3(mat.specular * specular);
}
