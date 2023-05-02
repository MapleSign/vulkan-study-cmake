#include "host_device.h"

vec3 computeDiffuse(vec3 diffuse, vec3 ambient, vec3 lightDir, vec3 normal)
{
    // Lambertian
    float dotNL = max(dot(normal, lightDir), 0.0);
    vec3  c     = diffuse * dotNL;
    c += ambient;
    return c;
}

vec3 computeSpecular(vec3 objSpecular, float shininess, vec3 viewDir, vec3 lightDir, vec3 normal)
{
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    return vec3(objSpecular * spec);
}
