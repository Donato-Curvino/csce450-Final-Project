#version 120

attribute vec3 aPos;
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
    // vec4 uVec_2 = vec4(0, 0,   2,   6*u);

    // compute spline basis
    mat4 GB = mat4(G * B);
    vec4 origin = GB * uVec;
    origin.w = 1;
    vec3 p_     = vec4(GB * uVec_).xyz;
    // if (!(p_.x == 0 || p_.z == 0))
    //     p_ += vec3(GB * (1e-15 * uVec_));
    // vec3 p_2    = vec3(GB * uVec_2);

    vec3 tan = (p_ == vec3(0)) ? vec3(1, 0, 0) : normalize(p_);
    vec3 bnorm = (tan.x == 0 && tan.z == 0)
        ? (u <= 0.5)
            ? normalize(cross(G[2] - G[1], G[1] - G[0]))
            : normalize(cross(G[3] - G[2], G[2] - G[1]))
        : normalize(vec3(-tan.z, 0, tan.x));
    vec3 norm  = normalize(cross(bnorm, tan));

    mat4 basis = mat4(vec4(norm, 0), vec4(bnorm, 0), vec4(tan, 0), origin);

    // need original basis
    // can precalculate at bone base and translate by u
    // vec4 pos_og = (1 - u) * bone[0] + u * bone[1];
    // vec4 pos_og   = bone * vec2(1 - u, u);
    // vec3 tan_og   = normalize(vec3(bone[1]) - vec3(bone[0]));
    // vec3 bnorm_og = (!(tan_og.x != 0 || tan_og.z != 0))
    //     ? normalize(vec3(-tan_og.y, tan_og.x, 0))
    //     : normalize(vec3(-tan_og.z, 0, tan_og.x));
    // vec3 norm_og = normalize(cross(bnorm_og, tan_og));
    // mat3 basis_og_T = transpose(mat3(norm_og, bnorm_og, tan_og));

    // // mat4 basis_og = mat4(vec4(norm_og, 0), vec4(bnorm_og, 0), vec4(tan_og, 0), vec4(pos_og, 1));
    // mat4 basis_og_inv = mat4(
    //     vec4(basis_og_T[0], 0),
    //     vec4(basis_og_T[1], 0),
    //     vec4(basis_og_T[2], 0),
    //     pos_og
    // );
    vec4 pos = basis * vec4(aPos, 1);
    // vec4 pos = basis * basis_og_inv * vec4(aPos, 1);
    vec4 posCam = MV * pos;

    gl_Position = P * posCam;
    // gl_Position = pos;
    // gl_Position = P * MV * aPos;
    vPos = posCam.xyz;
    vNor = vec3(basis * vec4(aNor, 0.0)).xyz;
    vTex = aTex;
}
