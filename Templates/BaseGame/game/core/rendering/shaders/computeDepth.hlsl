float2 ComputeMoments(float Depth) 
{   
	float2 Moments;   
	// First moment is the depth itself.   
	Moments.x = Depth;   
	// Compute partial derivatives of depth.    
	float dx = ddx(Depth);   
	float dy = ddy(Depth);   
	// Compute second moment over the pixel extents.   
	Moments.y = Depth*Depth + 0.25*(dx*dx + dy*dy);   
	return Moments; 
} 

float2 GetEVSMExponent(float pos, float neg)
{
	float max = 42.0f;
	
	float2 exponents = float2(pos, neg);
	
	return min(exponents, max);
}

float2 WarpDepth(float depth, float2 exponents)
{
	depth = 2.0f * depth - 1.0f;
	float pos = exp(exponents.x * depth);
	float neg = -exp(-exponents.y * depth);
	
	return float2(pos, neg);
}