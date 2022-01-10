#version 400

// #define DISPLAY_LOD

struct LIGHT {
	vec4 position; // assume point or direction in EC in this example shader
	vec4 ambient_color, diffuse_color, specular_color;
	vec4 light_attenuation_factors; // compute this effect only if .w != 0.0f
	vec3 spot_direction;
	float spot_exponent;
	float spot_cutoff_angle;
	bool light_on;
};

struct MATERIAL {
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	vec4 emissive_color;
	float specular_exponent;
};

const vec4 blue = vec4(0.0f, 0.0f,1.0f, 1.0f);
const vec4 color1 = vec4(0.2f, 0.4f,0.6f, 1.0f);


uniform vec4 u_global_ambient_color;
#define NUMBER_OF_LIGHTS_SUPPORTED 7
uniform LIGHT u_light[NUMBER_OF_LIGHTS_SUPPORTED];
uniform MATERIAL u_material;
uniform MATERIAL u_block;

uniform sampler2D u_base_texture;

uniform bool block_effect = false;
uniform float block_frequency = 1.0f;
uniform float block_width = 0.125;

uniform bool u_flag_texture_mapping = true;
uniform bool u_flag_fog = false;
uniform bool u_4_effect = false;
uniform bool u_1_effect = false;
vec4 red_color;

const float zero_f = 0.0f;
const float one_f = 1.0f;

in vec3 v_position_EC;
in vec3 v_normal_EC;
in vec2 v_tex_coord;

in vec2 v_position_sc;

layout (location = 0) out vec4 final_color;

vec4 lighting_equation_textured(in vec3 P_EC, in vec3 N_EC, in vec4 base_color) {
	vec4 color_sum;
	float local_scale_factor, tmp_float, tmp_float2; 
	vec3 L_EC;

	color_sum = u_material.emissive_color + u_global_ambient_color * base_color;
 
	for (int i = 0; i < NUMBER_OF_LIGHTS_SUPPORTED; i++) {
		if (!u_light[i].light_on) continue;

		local_scale_factor = one_f;
		if (u_light[i].position.w != zero_f) { // point light source
			L_EC = u_light[i].position.xyz - P_EC.xyz;

			if (u_light[i].light_attenuation_factors.w  != zero_f) {
				vec4 tmp_vec4;

				tmp_vec4.x = one_f;
				tmp_vec4.z = dot(L_EC, L_EC);
				tmp_vec4.y = sqrt(tmp_vec4.z);
				tmp_vec4.w = zero_f;
				local_scale_factor = one_f/dot(tmp_vec4, u_light[i].light_attenuation_factors);
			}

			L_EC = normalize(L_EC);

			if (u_light[i].spot_cutoff_angle < 180.0f) { // [0.0f, 90.0f] or 180.0f
				float spot_cutoff_angle = clamp(u_light[i].spot_cutoff_angle, zero_f, 90.0f);
				vec3 spot_dir = normalize(u_light[i].spot_direction);

				tmp_float = dot(-L_EC, spot_dir);
				if (tmp_float >= cos(radians(spot_cutoff_angle))) {
					if(!u_4_effect)
						tmp_float = pow(tmp_float, u_light[i].spot_exponent);
					else{
						if(i == 4){

									if(L_EC.x > 0.05f && L_EC.z < 0.05f)
										tmp_float = zero_f;

								     if(L_EC.x < -0.05f && L_EC.z > -0.05f)
										tmp_float = zero_f;
						}
						else
							tmp_float = pow(tmp_float, u_light[i].spot_exponent);
					}

					if(!u_1_effect)
						tmp_float = pow(tmp_float, u_light[i].spot_exponent);
					else{
						if(i == 2){

									if(L_EC.x > 0.07f && L_EC.x < 0.08f)
										tmp_float = zero_f;
							

									if(L_EC.x < -0.07f && L_EC.x > -0.08f)
										tmp_float = zero_f;

									if((pow(L_EC.x, 2) + pow(L_EC.z, 2) < 0.02) && (pow(L_EC.x, 2) + pow(L_EC.z, 2) > 0.005))
										tmp_float = zero_f;

						}
						else
							tmp_float = pow(tmp_float, u_light[i].spot_exponent);
					}
					
				}
				else 
					tmp_float = zero_f;
				local_scale_factor *= tmp_float;
			}
		}
		else {  // directional light source
			L_EC = normalize(u_light[i].position.xyz);
		}	

		if (local_scale_factor > zero_f) {				
		 	vec4 local_color_sum = u_light[i].ambient_color * u_material.ambient_color;

			tmp_float = dot(N_EC, L_EC);  
			if (tmp_float > zero_f) {  
				local_color_sum += u_light[i].diffuse_color*base_color*tmp_float;
			
				vec3 H_EC = normalize(L_EC - normalize(P_EC));
				tmp_float = dot(N_EC, H_EC); 
				if (tmp_float > zero_f) {
					local_color_sum += u_light[i].specular_color
				                       *u_material.specular_color*pow(tmp_float, u_material.specular_exponent);
				}
			}

			color_sum += local_scale_factor*local_color_sum;


					if(!u_1_effect)
						tmp_float = pow(tmp_float, u_light[i].spot_exponent);
					else{
						if(i == 2){

									if(L_EC.x > 0.08f){
										color_sum = red_color; 
										//u_light[i].ambient_color[1] = 0.152f;
										//u_light[i].ambient_color[2] = 0.152f; 
										//u_light[i].ambient_color[3] = 1.0f;
									}
						}

					}

	
		}
	}
 	return color_sum;
}

// May contol these fog parameters through uniform variables
#define FOG_COLOR vec4(0.7f, 0.7f, 0.7f, 1.0f)
#define FOG_NEAR_DISTANCE 350.0f
#define FOG_FAR_DISTANCE 700.0f

void main(void) {
	vec4 base_color, shaded_color;
	float fog_factor;

	red_color[0] = 0.8f;
	red_color[1] = 0.2f;
	red_color[2] = 0.2f;
	red_color[0] = 1.0f;


	if(block_effect){
		float x_mod, y_mod;
		x_mod = mod(v_position_sc.x*block_frequency, 1.0f);
		y_mod = mod(v_position_sc.y*block_frequency, 1.0f);

		if((x_mod > block_width ) && (x_mod < block_width * 2))
			final_color = color1;
		if((x_mod > block_width * 3) && (x_mod < block_width * 4))
			discard;
		if((x_mod > block_width * 5) && (x_mod < block_width * 6))
			discard;
		if((x_mod > block_width * 7) && (x_mod < block_width * 8))
			discard;
		if((x_mod > block_width) && (x_mod < 1.0f - block_width) && (y_mod > block_width) && (y_mod < 1.0f - block_width))
			discard;

		
		return;
	
	}

	//final_color = lighting_equation(v_position_EC, normalize(v_normal_EC));

#if (__VERSION__ >= 400) && defined(DISPLAY_LOD)
// Just to see the computed mipmap level for debugging purposes. Remove this part for a regular rendering.
	float mipmap_level;

 	mipmap_level = textureQueryLod(u_base_texture, v_tex_coord).x;
 	// base_color = textureLod(u_base_texture, v_tex_coord, mipmap_level); 

	if (mipmap_level < 0.5f) 
		final_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);  // White
	else if (mipmap_level < 1.5f) 
		final_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red
	else if (mipmap_level < 2.5f)
		final_color = vec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green
	else if (mipmap_level < 3.5f)
		final_color = vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
	else if (mipmap_level < 4.5f)
		final_color = vec4(0.0f, 1.0f, 1.0f, 1.0f);  // Cyan
	else if (mipmap_level < 5.5f)
		final_color = vec4(1.0f, 0.0f, 1.0f, 1.0f);  // Magenta
	else if (mipmap_level < 6.5f)
		final_color = vec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
	else 
		final_color = vec4(0.5f, 0.5f, 0.5f, 1.0f);  // Grey
#else
// For a normal rendering ...
	if (u_flag_texture_mapping) 
		base_color = texture(u_base_texture, v_tex_coord);
	else 
		base_color = u_material.diffuse_color;

	shaded_color = lighting_equation_textured(v_position_EC, normalize(v_normal_EC), base_color);

	if (u_flag_fog) {
 	  	fog_factor = (FOG_FAR_DISTANCE - length(v_position_EC.xyz))/(FOG_FAR_DISTANCE - FOG_NEAR_DISTANCE);  		
//      fog_factor = (FOG_FAR_DISTANCE + v_position_EC.z)/(FOG_FAR_DISTANCE - FOG_NEAR_DISTANCE);
		fog_factor = clamp(fog_factor, 0.0f, 1.0f);
		final_color = mix(FOG_COLOR, shaded_color, fog_factor);
	}
	else 
		final_color = shaded_color;

	if(block_effect){
		float x_mod, y_mod;
		x_mod = mod(v_position_sc.x*block_frequency, 1.0f);
		y_mod = mod(v_position_sc.y*block_frequency, 1.0f);

		if((x_mod > block_width ) && (x_mod < block_width * 2))
			discard;
		if((x_mod > block_width * 3) && (x_mod < block_width * 4))
			discard;
		if((x_mod > block_width * 5) && (x_mod < block_width * 6))
			discard;
		if((x_mod > block_width * 7) && (x_mod < block_width * 8))
			discard;
		if((x_mod > block_width) && (x_mod < 1.0f - block_width) && (y_mod > block_width) && (y_mod < 1.0f - block_width))
			discard;
	
	}
#endif
}
