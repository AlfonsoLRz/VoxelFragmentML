#pragma once

#include "Graphics/Core/Model3D.h"
#include "Geometry/3D/Ray3D.h"
#include "Geometry/3D/Triangle3D.h"
#include "Geometry/3D/TriangleMesh.h"

typedef std::list<std::list<Triangle3D*>> FaceListNode;

/**
*	@file OctreeTriangle.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 08/06/2019
*/

/**
*	@brief Octree of a triangle mesh.
*/
class Octree
{
protected:
	class OctreeNode;																//!< Anidated class to be defined below

	// [Nodes references]
	OctreeNode*								_root;									//!< Node where the traversal starts

	// [Tree data]
	uint8_t									_maxLevel;								//!< Higher priority than max triangles per node
	uint8_t									_maxTrianglesPerNode;					//!< Maximum capacity of a node
	std::vector<Triangle3D*>				_triangles;								//!< List of mesh triangles

protected:
	/**
	*	@brief Computes the child index where the search should start from the current node. Octree traversal.
	*/
	int getFirstNode(const float tx0, const float ty0, const float tz0, const float txm, const float tym, const float tzm);

	/**
	*	@brief Obtains the metadata of each node in a recursive way (bounding box of each node).
	*	@param node Node where we're searching.
	*	@param aabb Retrieved aligned axis bounding boxes.
	*/
	void grabNodeData(OctreeNode* node, std::vector<AABB>& aabb);

	/**
	*	@brief Inserts a triangle at a certain node.
	*	@param node Node where we're currently working.
	*	@param parent Previous node.
	*	@param triangle Triangle to be inserted.
	*/
	void insert(OctreeNode* node, OctreeNode* parent, Triangle3D* triangle);

	/**
	*	@brief Finds the node where the triangle should be located. Warning: if the located node is null then the parent creates its children.
	*	@param node Node where we're currently working.
	*	@param parent Previous node.
	*	@param triangle Triangle to be located.
	*	@param foundNodes Nodes which may contain the triangle.
	*/
	void locateNodeForInsertion(OctreeNode* node, OctreeNode* parent, Triangle3D* triangle, std::list<OctreeNode*>* foundNodes);

	/**
	*	@brief Finds the nodes where the triangle should be. In case it's null we simply return it.
	*	@param node Node where we're currently searching.
	*	@param triangle Triangle to be searched.
	*	@param foundNodes Nodes which may contain the triangle.
	*/
	void locateNode(OctreeNode* node, Triangle3D* triangle, std::list<OctreeNode*>* foundNodes);

	/**
	*	@brief Locates the node where the point should be. In case it's null we simply return it.
	*/
	OctreeNode* locateNode(OctreeNode* node, vec3& point);

	/**
	*	@brief Locates the node which comes after the current one. Octree traversal.
	*/
	int newNode(const float txm, const int x, const float tym, const int y, const float tzm, const int z);

	/**
	*	@brief Octree exploration through a ray.
	*	@param face Faces which the ray intersects.
	*	@param node Current node.
	*/
	void retrieveAABB(FaceListNode* face, const float tx0, const float ty0, const float tz0, const float tx1, const float ty1, const float tz1, unsigned char a, OctreeNode* node);

public:
	/**
	*	@brief Constructor with no triangles by default.
	*	@param maxLevel Maximum depth.
	*	@param maxTrianglesNode Maximum number of triangles per node even though maxLevel has more priority.
	*	@param mesh Faces which must be saved in the octree.
	*/
	Octree(const uint8_t maxLevel, const uint8_t maxTrianglesNode, Model3D* mesh, const AABB& aabb);

	/**
	*	@brief Unsupported copy constructor.
	*/
	Octree(const Octree& orig) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~Octree();

	/**
	*	@return AABB which marks the octree boundaries.
	*/
	virtual AABB getAABB() const { return _root->_aabb; }

	/**
	*	@brief Returns the bounding boxes for each node of the octree (rendering purposes).
	*	@param aabb Bounding boxes which represents the octree.
	*/
	virtual void getAABBs(std::vector<AABB>& aabb);

	/**
	*	@brief Computes the intersection with a ray.
	*	@param result Triangles which can be intersected by the ray.
	*/
	void intersection(const Ray3D& ray, FaceListNode* face);

	/**
	*	@brief Unsupported assignment overriding.
	*/
	Octree& operator=(const Octree& orig) = delete;

	/**
	*	@brief Includes new data on the tree.
	*	@param modelComp Model component where that face is defined.
	*/
	void push_back(Triangle3D* triangle);

protected:
	class OctreeNode
	{
	public:
		const static int					NUM_CHILDREN = 8;				//!< Octree ==> 8

	public:
		// [Node data]
		AABB _aabb;
		std::vector<AABB>					_childAABB;						//!< Bounding boxes of our childs, which are contained in our _aabb
		std::list<Triangle3D*>				_content;						//!< Triangles

		// [Tree data]
		Octree*								_root;							//!< Reference to global class
		OctreeNode*							_parent;						//!< Previous node

		// [Child data]
		std::vector<OctreeNode*>			_child;							//!< Nodes which are behind this one
		bool								_createdChildren;				//!< Child array is initialized from the start but node pointers are null
		uint8_t								_level;							//!< Tree depth

	public:
		/**
		*	@brief Constructor.
		*	@param level Level where this node is located at the tree.
		*	@param aabb Bounding box that represents the boundary of this node.
		*	@param root Octree root.
		*	@param parent Parent node. Null if this node is root.
		*/
		OctreeNode(uint8_t level, const AABB& aabb, Octree* root, OctreeNode* parent);

		/**
		*	@brief Destructor.
		*/
		~OctreeNode();

		/**
		*	@brief Initializes all the children pointers.
		*/
		void createChildren();

		/**
		*	@brief Searchs the octree node where the point should be located.
		*/
		int getChildrenIndex(vec3& point);

		/**
		*	@brief Searchs the octree nodes where the triangle should be linked.
		*/
		std::vector<uint8_t> getChildrenIndices(Triangle3D* triangle);

		/**
		*	@return Amount of points the childs contains.
		*/
		size_t getChildrenSize();

		/**
		*	@return True if the node is a leaf. 
		*/
		bool isLeaf() { return !_createdChildren; }
	};
};

