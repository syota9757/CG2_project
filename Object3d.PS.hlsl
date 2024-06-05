struct PixelShaderOutput
{
    float4 color : SV_Target0;
};


PixelShaderOutput main()
{
    PixelShaderOutput output;
    output.color = float4(1.0, 1.0, 1.0, 1.0);
    return output;
    
}