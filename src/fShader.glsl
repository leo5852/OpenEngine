#version 330 core

in vec4 color;
in vec3 fragPos;    // view space
in vec3 fragNormal; // view space
out vec4  fColor;

// MAX_LIGHTS should be injected by 'light.h(C++) - Shader::addDefine'.

struct LightInfo {
  vec4 Position;  // Light position in veiw coords.
  vec3 L;         // D,S intensity
};

uniform LightInfo Lights[MAX_LIGHTS];
uniform int LightCount;          // 실제로 켜진 개수
uniform vec3 Ambient;            // 장면 전체 환경광 (광원 개수와 무관)

uniform struct MaterialInfo {
  vec3 Ks;            // Specular reflectivity
  float Shininess;    // Specular shininess factor
} Material ;

vec3 contribution(LightInfo light, vec3 pos, vec3 n, vec3 texColor) {
  vec3 s = (light.Position.w == 0.0)
         ? normalize(light.Position.xyz)            // 방향광
         : normalize(light.Position.xyz - pos);     // 점광원

  float sDotn = max(dot(s, n), 0.0);
  vec3 diffuse = texColor * sDotn;

  vec3 spec = vec3(0.0);
  if (sDotn > 0.0) {
    vec3 v = normalize(-pos);
    vec3 h = normalize(v + s);
    spec = Material.Ks * pow(max(dot(h, n), 0.0), Material.Shininess);
  }
  return light.L * (diffuse + spec);
}

void main() 
{ 
    // 법선 확인용: -1~1 범위를 0~1 색 범위로 옮김
    //fColor = vec4(normalize(fragNormal) * 0.5 + 0.5, 1.0);

    // Blinn-Phong용
    vec3 n = normalize(fragNormal);
    vec3 texColor = color.xyz;
    // 환경광은 한번만 반영해야하므로 따로 빼줌
    vec3 result = Ambient * texColor;
    // 각 light source이 미치는 영향을 계산해 합산
    for (int i = 0; i < LightCount; ++i)
      result += contribution(Lights[i], fragPos, n, texColor);

    fColor = vec4(result, 1.0);
} 

