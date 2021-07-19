// ------- Distance models
subroutine float distanceModel(in vec3 p, in vec3 q);
subroutine uniform distanceModel distanceUniform;

subroutine(distanceModel)
float euclideanDistance(in vec3 p, in vec3 q) 
{
	return distance(p, q);
}

subroutine(distanceModel)
float manhattanDistance(in vec3 p, in vec3 q) 
{
	return abs(p.x - q.x) + abs(p.y - q.y) + abs(p.z - q.z);
}

subroutine(distanceModel)
float chebyshevDistance(in vec3 p, in vec3 q) 
{
	return max(abs(p.x - q.x), max(abs(p.y - q.y), abs(p.z - q.z)));
}