#version 120

attribute vec4 aPos;
attribute vec3 aNor;
attribute vec2 aTex;
// attribute vec4 u;
// attribute vec4  bw;
// attribute ivec4 bi;

attribute float u;

uniform mat4 P;
uniform mat4 MV;
// uniform mat4 bones[50];  // bone transformations
// uniform mat3x4 Gs[50];   // max 50 bones
uniform mat4x3 G;
uniform mat4 B;          // spline basis
uniform mat2x4 bone;

varying vec3 vPos;
varying vec3 vNor;
varying vec2 vTex;

void main() {
    // vec4 posCam = MV * aPos;
    vec4 uVec   = vec4(1, u, u*u, u*u*u);
    vec4 uVec_  = vec4(0, 1, 2*u, 3*u*u);
    vec4 uVec_2 = vec4(0, 0,   2,   6*u);

    // vec4 sum = 0;
    // for (int i = 0; i < 4; i++) {
    //     vec4 sPos = Gs[i] * B * uVec;
    //     sum += bw[i] * sPos;
    // }

    // compute spline basis
    mat4 GB = mat4(G * B);
    vec4 origin = GB * uVec;
    vec3 p_     = vec3(GB * uVec_);
    vec3 p_2    = vec3(GB * uVec_2);

    vec3 tan   = normalize(p_);
    vec3 bnorm = normalize(cross(p_, p_2));
    vec3 norm  = normalize(cross(bnorm, tan));

    mat4 basis = mat4(vec4(norm, 0), vec4(bnorm, 0), vec4(tan, 0), origin);

    // need original basis
    // can precalculate at bone base and translate by u
    // vec4 pos_og = (1 - u) * bone[0] + u * bone[1];
    vec4 pos_og   = bone * vec2(1 - u, u);
    // vec4 tan_og   = normalize(bone[1] - bone[0]);
    // vec4 norm_og  = normalize(cross(tan_og, vec4(0, 1, 0, 0)));
    // vec4 bnorm_og = normalize(cross(tan_og, norm_og));
    // mat4 basis_og = mat4(norm_og, bnorm_og, tan_og, pos_og);

    // TODO: may need to change
    mat4 basis_og_inv = mat4(1);
    basis_og_inv[3].xyz = -pos_og.xyz;

    vec4 pos = basis * basis_og_inv * aPos;
    vec4 posCam = MV * pos;

    gl_Position = P * posCam;
    vPos = posCam.xyz;
    vNor = vec3(MV * vec4(aNor, 0.0)).xyz;
    vTex = aTex;
}
