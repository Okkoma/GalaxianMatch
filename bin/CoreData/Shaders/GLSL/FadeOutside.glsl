#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;
varying vec4 vColor;

uniform vec2 cScreenSize;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    
    vTexCoord = iTexCoord;
    vColor = iColor; 
}


void PS()
{   
    const float unfadeStartPosition = 0.45;  // gauche de la zone d'effet
    const float unfadeEndPosition = 0.55;    // droite de la zone d'effet

    float normx = gl_FragCoord.x / cScreenSize.x;

    float alpha = 1.0;
       
    if (normx < unfadeStartPosition)
        alpha = smoothstep(0.0, unfadeStartPosition, normx);
    else if (normx > unfadeEndPosition)
        alpha = 1.0 - smoothstep(unfadeEndPosition, 1.0, normx);

    vec4 diffColor = cMatDiffColor * vColor;
    vec4 diffInput = texture2D(sDiffMap, vTexCoord);
    
    gl_FragColor = diffColor * diffInput;
    gl_FragColor.a *= alpha;  // Appliquer l'alpha calculé
}
