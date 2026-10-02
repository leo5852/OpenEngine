#version 330 core

in vec4 color;
in vec3 fragPos;    // view space
in vec3 fragNormal; // world space
out vec4  fColor;

uniform struct LightInfo {
  vec4 Position;  // Light position in cam. coords.
  vec3 L;         // D,S intensity
  vec3 La;        // Amb intensity
} Light ;

uniform struct MaterialInfo {
  vec3 Ks;            // Specular reflectivity
  float Shininess;    // Specular shininess factor
} Material ;

vec3 blinnPhong( vec3 fragPos, vec3 n) {
  //vec3 texColor = texture(Tex1, TexCoord).rgb;
  vec3 texColor = color.xyz;

  vec3 ambient = Light.La * texColor;
  vec3 s = normalize( Light.Position.xyz - fragPos );
  float sDotn = max( dot(s,n), 0.0 );
  vec3 diffuse = texColor * sDotn;
  
  vec3 spec = vec3(0.0);
  if( sDotn > 0.0 ) {
    vec3 v = normalize(-fragPos.xyz);
    vec3 h = normalize( v + s );
    //spec = Material.Ks * pow( max( dot(h,n), 0.0 ), Material.Shininess );
    spec = vec3(1.0) * pow( max( dot(h,n), 0.0 ), 128 );
  }
  return ambient + Light.L * (diffuse + spec);
}

void main() 
{ 
    // 법선 확인용: -1~1 범위를 0~1 색 범위로 옮김
    //fColor = vec4(normalize(fragNormal) * 0.5 + 0.5, 1.0);

    // Blinn-Phong용
    fColor = vec4( blinnPhong( fragPos, normalize(fragNormal) ), 1.0 );
} 

