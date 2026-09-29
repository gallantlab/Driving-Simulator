// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "GraphTypes.h"
#include "Position.h"
#include "Util/ListView.h"

#include <array>
#include <list>
#include <cmath>

#define BETWEEN(p, a, b) (p >= a && b >= p) || (p >= b && a >= p)

using namespace std;

namespace MapGen
{

	/// Simple doubly-connected edge list structure. It only allows adding
	/// elements, not removing them.
	class CARLA_API DoublyConnectedEdgeList

	: private NonCopyable
{
	// =========================================================================
	// -- DCEL types -----------------------------------------------------------
	// =========================================================================

	public:

	using Position = MapGen::Position<int32>;

	struct Node;
	struct HalfEdge;
	struct Face;

	struct Node : public GraphNode
	{
		friend DoublyConnectedEdgeList;

		Node(const Position& Pos) : Position(Pos)
		{}

		Node& operator=(const Node&) = delete;

		const DoublyConnectedEdgeList::Position& GetPosition() const
		{
			return Position;
		}

	private:
		DoublyConnectedEdgeList::Position Position;
		HalfEdge* LeavingHalfEdge = nullptr;
	};

	struct HalfEdge : public GraphHalfEdge
	{
		friend DoublyConnectedEdgeList;

		HalfEdge& operator=(const HalfEdge&) = delete;

		double Length() const
		{
			using namespace std;
			Position diff = Target->GetPosition() - Source->GetPosition();
			return sqrt(diff.x * diff.x + diff.y * diff.y);
		}

		HalfEdge* NextEdge() const
		{
			return Next;
		}

		HalfEdge* PreviousEdge() const
		{
			HalfEdge *edge = Next->NextEdge();
			while (edge->NextEdge() != this)
				edge = edge->NextEdge();
			return edge;
		}

		HalfEdge* OrthogonalEdge() const
		{
			HalfEdge *edge = Next;
			while (edge->Parallel(*this))
				edge = edge->NextEdge();
			return edge;
		}

		FVector2D Orientation() const
		{
			Position startPos = Source->GetPosition();
			Position endPos = Target->GetPosition();
			FVector2D out = FVector2D(endPos.x - startPos.x, endPos.y - startPos.y);
			out.Normalize(0.001);
			return out;
		}

		// is pos approximately on this edge?
		// rough estimate for whether this point is on this edge using the dot product of segments
		bool PointIsBetween(Position& pos, double margin = 0) const
		{
			Position startPos = Source->GetPosition();
			Position endPos = Target->GetPosition();
			FVector2D first = FVector2D(pos.x - startPos.x, pos.y - startPos.y);
			FVector2D second = FVector2D(endPos.x - pos.x, endPos.y - pos.y);
			if (first.Size() < margin || second.Size() < margin)	// exclusion zone around the endpoints
				return false;
			return abs((first.Size() + second.Size()) - Length()) < 0.001;
		}

		bool AntiParallel(const HalfEdge& other) const
		{
			return abs(FVector2D::DotProduct(this->Orientation(), other.Orientation()) + 1) < 0.001;
		}

		bool AntiParallel(HalfEdge *other) const
		{
			return this->AntiParallel(*other);
		}

		bool Parallel(const HalfEdge& other) const
		{
			return abs(FVector2D::DotProduct(this->Orientation(), other.Orientation()) - 1) < 0.001;
		}

		bool Parallel(HalfEdge *other) const
		{
			return this->Parallel(*other);
		}


	private:
		Node* Source = nullptr;
		Node* Target = nullptr;
		HalfEdge* Next = nullptr;
		HalfEdge* Pair = nullptr;
		Face* Face = nullptr;
	};

	struct Face : public GraphFace
	{
		friend DoublyConnectedEdgeList;

		Face& operator=(const Face&) = delete;

		// assumes rectangular regions, which as of Carla 0.8.4 is reasonable
		double Area() const
		{
			HalfEdge* edge = halfEdge;
			// iterator to an edge directed away from a corner
			while (edge->Parallel(edge->NextEdge()))
				edge = edge->NextEdge();
			edge = edge->NextEdge();
			double width = edge->Length();
			// tally up length
			while (edge->Parallel(edge->NextEdge()))
			{
				edge = edge->NextEdge();
				width += edge->Length();
			}
			edge = edge->NextEdge();
			double height = edge->Length();
			while (edge->Parallel(edge->NextEdge()))
			{
				edge = edge->NextEdge();
				height += edge->Length();
			}
			return width * height;

//			return HalfEdge->Length() * HalfEdge->NextEdge()->Length();
		}

	private:
		HalfEdge* halfEdge = nullptr;
	};

	using NodeContainer = std::list<Node>;
	using NodeIterator = typename NodeContainer::iterator;
	using ConstNodeIterator = typename NodeContainer::const_iterator;

	using HalfEdgeContainer = std::list<HalfEdge>;
	using HalfEdgeIterator = typename HalfEdgeContainer::iterator;
	using ConstHalfEdgeIterator = typename HalfEdgeContainer::const_iterator;

	using FaceContainer = std::list<Face>;
	using FaceIterator = typename FaceContainer::iterator;
	using ConstFaceIterator = typename FaceContainer::const_iterator;

	// =========================================================================
	// -- Constructors and destructor ------------------------------------------
	// =========================================================================

	public:

	/// Create a DoublyConnectedEdgeList with two nodes, two edges and one face.
	explicit DoublyConnectedEdgeList(const Position& Position0, const Position& Position1);

	/// Create a DoublyConnectedEdgeList consisting of a cycle of N nodes.
	template<size_t N>
	explicit DoublyConnectedEdgeList(const std::array <Position, N>& Cycle)
			: DoublyConnectedEdgeList(Cycle[0u], Cycle[1u])
	{
		static_assert(N > 2u, "Not enough nodes to make a cycle!");
		for (auto i = 2u; i < Cycle.size(); ++i)
		{
			AddNode(Cycle[i], Nodes.back());
		}
		ConnectNodes(Nodes.front(), Nodes.back());
	}

	~DoublyConnectedEdgeList();

	// =========================================================================
	/// @name Adding elements to the graph -------------------------------------
	// =========================================================================
	/// {
	public:

	/// Add a node at @a NodePosition and attach it to @a OtherNode.
	///
	/// The time complexity is O(n*log(n)) where n is the number of edges
	/// leaving @a OtherNode.
	///
	/// @return The newly generated node.
	Node& AddNode(const Position& NodePosition, Node& OtherNode);

	/// Split @a HalfEdge (and its pair) at @a Position.
	///
	/// The time complexity is O(n*log(n)) where n is the number of edges
	/// leaving @a HalfEdge's source.
	///
	/// @return The newly generated node.
	Node& SplitEdge(const Position& Position, HalfEdge& HalfEdge);

	/// Connect two nodes by a pair of edges.
	///
	/// It is assumed that both nodes are connected by the same face.
	///
	/// The time complexity is O(n0*log(n0) + n1*log(n1) + nf) where n0 and n1
	/// are the number of edges leaving @a Node0 and @a Node1 respectively, and
	/// nf is the number of edges in the face containing both nodes.
	///
	/// @return The newly generated face.
	Face& ConnectNodes(Node& Node0, Node& Node1);

	/// @}
	// =========================================================================
	/// @name Counting graph elements ------------------------------------------
	// =========================================================================
	/// @{
	public:

	size_t CountNodes() const
	{
		return Nodes.size();
	}

	size_t CountHalfEdges() const
	{
		return HalfEdges.size();
	}

	size_t CountFaces() const
	{
		return Faces.size();
	}

	/// @}
	// =========================================================================
	/// @name Accessing graph elements -----------------------------------------
	// =========================================================================
	/// @{
	public:

	ListView <NodeIterator> GetNodes()
	{
		return ListView<NodeIterator>(Nodes);
	}

	ListView <ConstNodeIterator> GetNodes() const
	{
		return ListView<ConstNodeIterator>(Nodes);
	}

	ListView <HalfEdgeIterator> GetHalfEdges()
	{
		return ListView<HalfEdgeIterator>(HalfEdges);
	}

	ListView <ConstHalfEdgeIterator> GetHalfEdges() const
	{
		return ListView<ConstHalfEdgeIterator>(HalfEdges);
	}

	ListView <FaceIterator> GetFaces()
	{
		return ListView<FaceIterator>(Faces);
	}

	ListView <ConstFaceIterator> GetFaces() const
	{
		return ListView<ConstFaceIterator>(Faces);
	}

	/// @}
	// =========================================================================
	/// @name Accessing graph pointers -----------------------------------------
	// =========================================================================
	/// @{
	public:

	// -- Primary pointers -----------------------------------------------------

	static Node& GetSource(HalfEdge& halfEdge)
	{
		check(halfEdge.Source != nullptr);
		return *halfEdge.Source;
	}

	static const Node& GetSource(const HalfEdge& halfEdge)
	{
		check(halfEdge.Source != nullptr);
		return *halfEdge.Source;
	}

	static Node& GetTarget(HalfEdge& halfEdge)
	{
		check(halfEdge.Target != nullptr);
		return *halfEdge.Target;
	}

	static const Node& GetTarget(const HalfEdge& halfEdge)
	{
		check(halfEdge.Target != nullptr);
		return *halfEdge.Target;
	}

	static HalfEdge& GetPair(HalfEdge& halfEdge)
	{
		check(halfEdge.Pair != nullptr);
		return *halfEdge.Pair;
	}

	static const HalfEdge& GetPair(const HalfEdge& halfEdge)
	{
		check(halfEdge.Pair != nullptr);
		return *halfEdge.Pair;
	}

	static Face& GetFace(HalfEdge& halfEdge)
	{
		check(halfEdge.Face != nullptr);
		return *halfEdge.Face;
	}

	static const Face& GetFace(const HalfEdge& halfEdge)
	{
		check(halfEdge.Face != nullptr);
		return *halfEdge.Face;
	}

	static HalfEdge& GetLeavingHalfEdge(Node& node)
	{
		check(node.LeavingHalfEdge != nullptr);
		return *node.LeavingHalfEdge;
	}

	static const HalfEdge& GetLeavingHalfEdge(const Node& node)
	{
		check(node.LeavingHalfEdge != nullptr);
		return *node.LeavingHalfEdge;
	}

	static HalfEdge& GetHalfEdge(Face& face)
	{
		check(face.halfEdge != nullptr);
		return *face.halfEdge;
	}

	static const HalfEdge& GetHalfEdge(const Face& face)
	{
		check(face.halfEdge != nullptr);
		return *face.halfEdge;
	}

	// -- Secondary pointers ---------------------------------------------------

	static HalfEdge& GetNextInFace(HalfEdge& halfEdge)
	{
		check(halfEdge.Next != nullptr);
		return *halfEdge.Next;
	}

	static const HalfEdge& GetNextInFace(const HalfEdge& halfEdge)
	{
		check(halfEdge.Next != nullptr);
		return *halfEdge.Next;
	}

	static HalfEdge& GetNextInNode(HalfEdge& halfEdge)
	{
		return GetNextInFace(GetPair(halfEdge));
	}

	static const HalfEdge& GetNextInNode(const HalfEdge& halfEdge)
	{
		return GetNextInFace(GetPair(halfEdge));
	}

	/// @}
	// =========================================================================
	/// @name Other member functions -------------------------------------------
	// =========================================================================
	/// @{
	public:

	/// Return the angle [-pi, pi] of the half-edge.
	static float GetAngle(const HalfEdge& halfEdge);

#ifdef CARLA_ROAD_GENERATOR_EXTRA_LOG
	void PrintToLog() const;
#endif // CARLA_ROAD_GENERATOR_EXTRA_LOG

	/// @}
	// =========================================================================
	// -- Private members ------------------------------------------------------
	// =========================================================================

	private:

	NodeContainer Nodes;

	HalfEdgeContainer HalfEdges;

	FaceContainer Faces;
};

} // namespace MapGen
