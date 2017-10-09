__kernel void VectorAdd(float alpha, __global float* a,__global float* b,__global float* c)
{
	//get index into global data array
	int index=get_global_id(0); //Returns the unique global work-item ID value for dimension;
	//add the vector elements
	c[index]=alpha*a[index]+b[index];
}