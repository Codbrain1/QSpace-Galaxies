#version 330 core
in vec2  vUV;
out vec4 FragColor;

uniform sampler2D uAccumTex;
uniform int       uNormalizationMode;

void main() {
    vec4  data         = texture(uAccumTex, vUV);
    float fieldValue   = data.r;
    float Wsum         = data.g;
    int   numParticles = int(data.b);

    if (numParticles == 0 || Wsum < 1e-12)
        discard;

    // Unknown fields use the accumulated scalar value instead of leaving the
    // output undefined. Undefined values break auto-range readback.
    float value = fieldValue;
    if (uNormalizationMode == 1) {
        value = fieldValue / Wsum;
    } else if (uNormalizationMode == 2) {
        value = fieldValue;
    }

    FragColor = vec4(value, Wsum, 0, 1.0);
}