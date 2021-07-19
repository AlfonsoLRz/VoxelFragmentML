#include "stdafx.h"
#include "Octree.h"

#include "Geometry/3D/Intersections3D.h"

/// [Public methods]

Octree::Octree(const uint8_t maxLevel, const uint8_t maxTrianglesNode, Model3D* mesh, const AABB& aabb) :
	_maxLevel(maxLevel), _maxTrianglesPerNode(maxTrianglesNode), _root(nullptr)
{
	_root = new OctreeNode(0, aabb, this, nullptr);

	// Fill octree with mesh triangles
	for (Model3D::ModelComponent* modelComponent: mesh->_modelComp)
	{
		for (Model3D::FaceGPUData& face: modelComponent->_topology)
		{
			Triangle3D* triangle = new Triangle3D(modelComponent->_geometry[face._vertices.x]._position,
												  modelComponent->_geometry[face._vertices.y]._position,
												  modelComponent->_geometry[face._vertices.z]._position);

			this->push_back(triangle);
		}
	}
}

Octree::~Octree()
{
	delete _root;
}

void Octree::getAABBs(std::vector<AABB>& aabb)
{
	this->grabNodeData(_root, aabb);
}

void Octree::intersection(const Ray3D& ray, FaceListNode* triangle)
{
	unsigned char a		= 0;
	const vec3 max		= _root->_aabb.max();
	const vec3 min		= _root->_aabb.min();
	const vec3 center	= _root->_aabb.center();

	vec3 rayOrig = ray.getOrigin(), rayDir = ray.getDirection();
	for (int i = 0; i < 3; ++i)
	{
		if (BasicOperations::equal(rayDir[i], 0.0f)) rayDir[i] = FLT_EPSILON;
	}

	if (rayDir.x < 0.0f)
	{
		rayOrig[0]	= center.x - (rayOrig[0] - center.x);
		rayDir[0]	= -rayDir[0];
		a |= 4;
	}

	if (rayDir.y < 0.0f)
	{
		rayOrig[1]	= center.y - (rayOrig[1] - center.y);
		rayDir[1]	= -rayDir[1];
		a |= 2;
	}

	if (rayDir.z < 0.0f)
	{
		rayOrig[2]	= center.z - (rayOrig[2] - center.z);
		rayDir[2]	= -rayDir[2];
		a |= 1;
	}

	const vec3 div	= 1.0f / rayDir;
	const float tx0 = (min.x - rayOrig.x) * div.x;
	const float tx1 = (max.x - rayOrig.x) * div.x;
	const float ty0 = (min.y - rayOrig.y) * div.y;
	const float ty1 = (max.y - rayOrig.y) * div.y;
	const float tz0 = (min.z - rayOrig.z) * div.z;
	const float tz1 = (max.z - rayOrig.z) * div.z;

	if (std::max(tx0, std::max(ty0, tz0)) <= std::min(tx1, std::min(ty1, tz1)))
	{
		retrieveAABB(triangle, tx0, ty0, tz0, tx1, ty1, tz1, a, _root);
	}
}

void Octree::push_back(Triangle3D* triangle)
{
	std::list<OctreeNode*> nodes;
	locateNodeForInsertion(_root, nullptr, triangle, &nodes);

	for (OctreeNode* node: nodes)
	{
		insert(node, node->_parent, triangle);
	}
}

/// [Protected methods]

int Octree::getFirstNode(const float tx0, const float ty0, const float tz0, const float txm, const float tym, const float tzm)
{
	unsigned char node = 0;

	if (tx0 > ty0)
	{
		if (tx0 > tz0)				// PLANE YZ
		{
			if (tym < tx0)	node |= 2;
			if (tzm < tx0)	node |= 1;
			if (txm < ty0)	node |= 4;
			if (tzm < ty0)	node |= 1;

			return (int)node;
		}
	}
	else
	{								// PLANE XZ
		if (ty0 > tz0)
		{
			if (txm < ty0)	node |= 4;
			if (tzm < ty0)	node |= 1;

			return (int)node;
		}
	}

	// PLANE XY
	if (txm < tz0)			node |= 4;				// Position 2
	if (tym < tz0)			node |= 2;				// Position 1

	return (int)node;
}

void Octree::grabNodeData(OctreeNode* node, std::vector<AABB>& aabb)
{
	if (node->isLeaf())
	{
		if (!node->_content.empty()) aabb.push_back(node->_aabb);

		return;
	}

	for (int i = 0; i < OctreeNode::NUM_CHILDREN; ++i)
	{
		this->grabNodeData(node->_child[i], aabb);
	}
}

void Octree::insert(OctreeNode* node, OctreeNode* parent, Triangle3D* triangle)
{
	node->_content.push_back(triangle);

	if (node->_level != _maxLevel && node->_content.size() > _maxTrianglesPerNode)
	{
		node->createChildren();

		auto triangleIt = node->_content.begin();

		while (triangleIt != node->_content.end())
		{
			std::vector<uint8_t> childrenIndices = node->getChildrenIndices(*triangleIt);

			for (int child = 0; child < childrenIndices.size(); ++child)
			{
				insert(node->_child[childrenIndices[child]], node, *triangleIt);
			}

			++triangleIt;
		}

		node->_content.clear();
	}
}

void Octree::locateNodeForInsertion(OctreeNode* node, OctreeNode* parent, Triangle3D* triangle, std::list<OctreeNode*>* foundNodes)
{
	if (node->isLeaf())
	{
		foundNodes->push_back(node);
		return;
	}

	const std::vector<uint8_t> childIndices = node->getChildrenIndices(triangle);
	std::vector<OctreeNode*> nodes;

	for (int child = 0; child < childIndices.size(); ++child)
	{
		locateNodeForInsertion(node->_child[childIndices[child]], node, triangle, foundNodes);
	}
}

void Octree::locateNode(OctreeNode * node, Triangle3D* triangle, std::list<OctreeNode*>* foundNodes)
{
	if (!node || node->isLeaf())
	{
		foundNodes->push_back(node);
		return;
	}

	const std::vector<uint8_t> childrenIndices = node->getChildrenIndices(triangle);
	std::vector<OctreeNode*> nodes;

	for (int child = 0; child < childrenIndices.size(); ++child)
	{
		locateNode(node->_child[childrenIndices[child]], triangle, foundNodes);
	}
}

Octree::OctreeNode* Octree::locateNode(OctreeNode* node, vec3& point)
{
	if (node->isLeaf())
	{
		return node;
	}

	int childrenIndex = node->getChildrenIndex(point);
	return locateNode(node->_child[childrenIndex], point);
}

int Octree::newNode(const float txm, const int x, const float tym, const int y, const float tzm, const int z)
{
	if (txm < tym)
	{
		if (txm < tzm) return x;				// YZ plane
	}
	else
	{
		if (tym < tzm) return y;				// XZ plane
	}

	return z;
}

void Octree::retrieveAABB(FaceListNode* face, const float tx0, const float ty0, const float tz0, const float tx1, const float ty1, const float tz1, unsigned char a, OctreeNode* node)
{
	if (tx1 < 0.0f || ty1 < 0.0f || tz1 < 0.0f)
	{
		return;
	}

	if (node->isLeaf())
	{
		if (node->_content.size() > 0)
		{
			face->push_back(node->_content);
		}

		return;
	}

	const float txm = (tx0 + tx1) / 2.0f, tym = (ty0 + ty1) / 2.0f, tzm = (tz0 + tz1) / 2.0f;
	int currNode	= getFirstNode(tx0, ty0, tz0, txm, tym, tzm);

	do {
		switch (currNode) {
		case 0:
		{
			retrieveAABB(face, tx0, ty0, tz0, txm, tym, tzm, a, node->_child[a]);
			currNode = newNode(txm, 4, tym, 2, tzm, 1);
			break;
		}
		case 1:
		{
			retrieveAABB(face, tx0, ty0, tzm, txm, tym, tz1, a, node->_child[1 ^ a]);
			currNode = newNode(txm, 5, tym, 3, tz1, 8);
			break;
		}
		case 2:
		{
			retrieveAABB(face, tx0, tym, tz0, txm, ty1, tzm, a, node->_child[2 ^ a]);
			currNode = newNode(txm, 6, ty1, 8, tzm, 3);
			break;
		}
		case 3:
		{
			retrieveAABB(face, tx0, tym, tzm, txm, ty1, tz1, a, node->_child[3 ^ a]);
			currNode = newNode(txm, 7, ty1, 8, tz1, 8);
			break;
		}
		case 4:
		{
			retrieveAABB(face, txm, ty0, tz0, tx1, tym, tzm, a, node->_child[4 ^ a]);
			currNode = newNode(tx1, 8, tym, 6, tzm, 5);
			break;
		}
		case 5:
		{
			retrieveAABB(face, txm, ty0, tzm, tx1, tym, tz1, a, node->_child[5 ^ a]);
			currNode = newNode(tx1, 8, tym, 7, tz1, 8);
			break;
		}
		case 6:
		{
			retrieveAABB(face, txm, tym, tz0, tx1, ty1, tzm, a, node->_child[6 ^ a]);
			currNode = newNode(tx1, 8, ty1, 8, tzm, 7);
			break;
		}
		case 7:
		{
			retrieveAABB(face, txm, tym, tzm, tx1, ty1, tz1, a, node->_child[7 ^ a]);
			currNode = 8;
			break;
		}
		}
	} while (currNode < 8);
}


/// [Node subclass]

Octree::OctreeNode::OctreeNode(uint8_t level, const AABB& aabb, Octree* root, Octree::OctreeNode* parent) :
	_level(level), _aabb(aabb), _root(root), _parent(parent), _createdChildren(false)
{
	_child.resize(NUM_CHILDREN);
	for (int i = 0; i < NUM_CHILDREN; ++i)
	{
		_child[i] = nullptr;
	}

	_childAABB = _aabb.split(2);
}

Octree::OctreeNode::~OctreeNode()
{
	for (int i = 0; i < NUM_CHILDREN; ++i)
	{
		delete _child[i];
	}
}

void Octree::OctreeNode::createChildren()
{
	if (!_createdChildren)
	{
		for (int i = 0; i < _childAABB.size(); ++i)
		{
			_child[i] = new OctreeNode(_level + 1, _childAABB[i], _root, this);
		}

		_createdChildren = true;
	}
}

int Octree::OctreeNode::getChildrenIndex(vec3& point)
{
	vec3 center = _aabb.center();
	int childrenIndex = 0;

	if (point.y > center.y) childrenIndex += 4;				// X and Z are first explored
	if (point.z > center.z) childrenIndex += 2;				// Z is explored in second place
	if (point.x > center.x) childrenIndex += 1;

	return childrenIndex;
}

std::vector<uint8_t> Octree::OctreeNode::getChildrenIndices(Triangle3D* triangle)
{
	std::vector<uint8_t> childrenIndices;

	for (int i = 0; i < _childAABB.size(); ++i)
	{
		if (Intersections3D::intersect(*triangle, _childAABB[i]))
		{
			childrenIndices.push_back(i);
		}
	}

	return childrenIndices;
}

size_t Octree::OctreeNode::getChildrenSize()
{
	size_t size = 0;

	for (int i = 0; i < _child.size(); ++i)
	{
		if (_child[i])
		{
			size += _child[i]->_content.size();
		}
	}

	return size;
}