// Phong fragment shader phong-tex.frag matched with phong-tex.vert
#version 330

// Some drivers require the following
precision highp float;

struct lightStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

struct materialStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform lightStruct light;
uniform materialStruct material;
uniform sampler2D textureUnit0;

in float ex_attenuation;

in vec3 ex_N;
in vec3 ex_V;
in vec3 ex_L;
in vec2 ex_TexCoord;
layout(location = 0) out vec4 out_Color;
 
void main(void) {
    
	
	vec4 ambientI = light.ambient * material.ambient;											// Ambient intensity

	
	vec4 diffuseI = light.diffuse * material.diffuse;											// Diffuse intensity
	diffuseI = diffuseI * max(dot(normalize(ex_N),normalize(ex_L)),0);
	
	vec3 R = normalize(reflect(normalize(-ex_L),normalize(ex_N)));								// Specular intensity. Calculate R - reflection of light

	vec4 specularI = light.specular * material.specular;
	specularI = specularI * pow(max(dot(R,ex_V),0), material.shininess);

																								// Fragment colour

	//vec4 tmp_Color = (diffuseI + specularI);//attenuation does not affect ambient light
	//tmp_Color = vec4(vec3(tmp_Color.rgb)*ex_attenuation,1.0);
	//vec4 litColour = ambientI + tmp_Color;
	//vec4 litColour = vec4(ex_attenuation,ex_attenuation,ex_attenuation,1.0);
	//litColour = litColour*texture2D(textureUnit0, ex_TexCoord);

	//out_Color = litColour;

	//out_Color = (ambientI + diffuseI + specularI) * texture(textureUnit0, ex_TexCoord);

	vec4 tmp_Color = (diffuseI + specularI); //attenuation does not affect ambient light

	
	vec4 litColor = ambientI + vec4(tmp_Color.rgb * ex_attenuation, 1.0);

	out_Color = litColor * texture2D(textureUnit0, ex_TexCoord);
}