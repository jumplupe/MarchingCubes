#version 330 core

layout (location = 0) in vec3 vPosition;
layout (location=1) in vec3 vNormal;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

void main(){
	vec4 v = vec4(vPosition,1);
	gl_Position = projection_matrix*view_matrix*model_matrix*v;
}
