// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "Carla.h"
#include "GraphGenerator.h"

#include <vector>
#include <cmath>

using namespace std;

#define FAST_LOG(a) UE_LOG(LogCarla, Log, TEXT(a));

namespace MapGen
{

	using Graph = DoublyConnectedEdgeList;

	constexpr static int32 MARGIN = 6;

	// ===========================================================================
	// -- Static local methods ---------------------------------------------------
	// ===========================================================================

	static int32 signOf(int32 val)
	{
		return (0 < val) - (val < 0);
	}

	static const Graph::Position& GetSourceCorner(const Graph::HalfEdge& edge)
	{
		const Graph::HalfEdge *startEdge = &edge;
		while (startEdge->PreviousEdge()->Parallel(*startEdge))
			startEdge = startEdge->PreviousEdge();
		return Graph::GetSource(edge).GetPosition();
	}

	static Graph::HalfEdge* GetSourceEdge(const Graph::HalfEdge& edge)
	{
		Graph::HalfEdge *startEdge = edge.NextEdge()->PreviousEdge();	// lol get non-const pointer to const object
		while (startEdge->PreviousEdge()->Parallel(*startEdge))
		{
			FAST_LOG("Source edge step");
			startEdge = startEdge->PreviousEdge();
		}
		return startEdge;
	}

	// randomly choose a point on [0, max] normally distributed around the center with a stdev of max / 8
	// and at least MARGIN away from the endpoints
	static double RandomNormalSplit(double max, FRandomStream &randomStream)
	{
		double rand1 = randomStream.FRandRange(0, 1);
		double rand2 = randomStream.FRandRange(0, 1);
		double randN = sqrt(-2 * log(rand1)) * cos(2 * rand2 * 3.14159265358797323);
		double out = randN * (max / 8.0) + (max / 2.0);
		if (out < MARGIN)
			out = MARGIN;
		if ((max - out) < MARGIN)
			out = max - out;
		return out;
	}

	static Graph::HalfEdge* GetAntiParallelSourceEdge(const Graph::HalfEdge& edge)
	{
		Graph::HalfEdge *startEdge = edge.NextEdge()->NextEdge();		// is at least 2 away from the current edge
		while (!startEdge->AntiParallel(edge))
		{
			FAST_LOG("Antiparallel step");
			startEdge = startEdge->NextEdge();
		}
		return startEdge;
	}

	static const Graph::Position& GetTargetCorner(const Graph::HalfEdge& edge)
	{
		// modified such that the target position is at the end of the mega-edge
		// in cases when one side of the rectangle is composed of multiple half-edges
		// we want the corner of the rectangle, which is not necessarily the end of
		// one particular edge
		const Graph::HalfEdge *endEdge = &edge;
		while (endEdge->NextEdge()->Parallel(*endEdge))
			endEdge = endEdge->NextEdge();
		return Graph::GetTarget(*endEdge).GetPosition();
	}

	static Graph::Position getDirection(const Graph::HalfEdge& edge)
	{
		return GetTargetCorner(edge) - GetSourceCorner(edge);
	}

	static const double GetSideLength(const Graph::HalfEdge *edge)
	{
		edge = GetSourceEdge(*edge);
		double length = edge->Length();
		while (edge->NextEdge()->Parallel(*edge))
		{
			edge = edge->NextEdge();
			length += edge->Length();
			FAST_LOG("Side sum");
		}
		return length;
	}

	static std::pair<Graph::HalfEdge*, Graph::HalfEdge*> getRandomOpposingEdges(Graph::Face& face, FRandomStream& random)
	{
		// Get all the edges in the face.
		std::vector < Graph::HalfEdge * > edges;
		edges.reserve(4u);
		auto& firstEdge = Graph::GetHalfEdge(face);
		auto* edge = &firstEdge;
		do
		{
			UE_LOG(LogCarla, Log, TEXT("Edge orientation %f %f"), edge->Orientation().X, edge->Orientation().Y);
			edges.emplace_back(edge);
			edge = &Graph::GetNextInFace(*edge);
		} while (edge != &firstEdge);
		UE_LOG(LogCarla, Log, TEXT("Edge size %d"), edges.size());
		// this assumption of only 4 edges per face is no longer valid under my super-duper splitting scheme
//		check(edges.size() == 4u);
		auto randomIndex = random.RandRange(0, edges.size() - 1);
		// get two opposing edges that are originate from a corner
		Graph::HalfEdge *first, *second;
		first = GetSourceEdge(*edges[randomIndex]);
		FAST_LOG("Source got");
		// split the longer side.
		double thisLength = GetSideLength(first);
		double otherLength = GetSideLength(first->OrthogonalEdge());
		first = (thisLength >= otherLength) ? first : first->OrthogonalEdge();
		FAST_LOG("Side decided");
		second = GetAntiParallelSourceEdge(*first);
		FAST_LOG("antiparallel got");
		return {first, second};
	}

	static Graph::Face* splitFace(Graph& graph, Graph::Face& face, FRandomStream& random)
	{
		std::pair<Graph::HalfEdge*, Graph::HalfEdge*> edgePair = getRandomOpposingEdges(face, random);
		if (!edgePair.first)
			return nullptr;
		FAST_LOG("Edge pair got");
		Graph::Position dir = getDirection(*edgePair.first);
		FAST_LOG("direction got");
		// account for several edges along one side of the rectangle
		Graph::HalfEdge* next = edgePair.first;
		while (next->Parallel(next->NextEdge()))
		{
			FAST_LOG("add length");
			next = next->NextEdge();
			dir += getDirection(*next);
		}
//		// Assumes both edges are opposing faces on a rectangle.
//		auto otherDir = getDirection(*edgePair.second);
//		check((dir.x == -1 * otherDir.x) && (dir.y == -1 * otherDir.y));
		check(edgePair.first->AntiParallel(edgePair.second));
		// If the rectangle is not big enough do not split it.
		if ((std::abs(dir.x) < 2 * MARGIN + 1) && (std::abs(dir.y) < 2 * MARGIN + 1))
			return nullptr;
		// Get a random point along the edges.
		FAST_LOG("Generate rand nums")
		auto randX = (dir.x != 0 ? signOf(dir.x) * (int)RandomNormalSplit(abs(dir.x), random) : 0);
		auto randY = (dir.y != 0 ? signOf(dir.y) * (int)RandomNormalSplit(abs(dir.y), random) : 0);
		Graph::Position position0 = GetSourceCorner(*edgePair.first) + Graph::Position{randX, randY};
		Graph::Position position1 = GetTargetCorner(*edgePair.second) + Graph::Position{randX, randY};
		// figure out which edges to split
		Graph::HalfEdge *firstToSplit = edgePair.first;
		Graph::HalfEdge *reference= edgePair.first;
		FAST_LOG("Pick first split")
		while (!firstToSplit->PointIsBetween(position0, MARGIN))
		{
			FAST_LOG("first split step");
			firstToSplit = firstToSplit->NextEdge();
			if (firstToSplit == reference)
				return nullptr;
		}
		FAST_LOG("Pick second step")
		Graph::HalfEdge *secondToSplit = edgePair.second;
		reference = secondToSplit;
		while (!secondToSplit->PointIsBetween(position1, MARGIN))
		{
			FAST_LOG("second split step");
			secondToSplit = secondToSplit->NextEdge();
			if (secondToSplit == reference)
				return nullptr;
		}
		// Split the edges and connect.
		FAST_LOG("Splitting edges")
		Graph::Node& node0 = graph.SplitEdge(position0, *firstToSplit);
		Graph::Node& node1 = graph.SplitEdge(position1, *secondToSplit);
		FAST_LOG("reconnecting nodes")
		return &graph.ConnectNodes(node0, node1);
	}

	static void randomize(Graph& graph, const int32 seed, const int32 targetAverageSize)
	{
		check(graph.CountNodes() == 4u);
		check(graph.CountHalfEdges() == 8u);
		check(graph.CountFaces() == 2u);
		FRandomStream random(seed);
		/// @todo We skip first face because is the surrounding face. But this won't
		/// be always the case, if the graph is generated differently it might be a
		/// different one.
		if (targetAverageSize == 0)	// no average size specified, do what original map generation does
		{
			Graph::Face* face = &*(++graph.GetFaces().begin()); // addr of deref b/c it's an iterator being derefed
			do
			{
				face = splitFace(graph, *face, random);
#ifdef CARLA_ROAD_GENERATOR_EXTRA_LOG
				graph.PrintToLog();
#endif // CARLA_ROAD_GENERATOR_EXTRA_LOG
			} while (face != nullptr);
		}
		else
		{
			Graph::Face* outsideFace = &*graph.GetFaces().begin();	// the outside face that we should never split
			const double overallSize = outsideFace->Area();
			int nFaces = 1;
			double averageSize = overallSize / nFaces;
			int faceToSplit = 1;
//			Graph::Face * faceToSplit = &*(++graph.GetFaces().begin());
			while (averageSize > targetAverageSize)
			{
				UE_LOG(LogCarla, Log, TEXT("%d faces, average size %f"), nFaces, averageSize);
				DoublyConnectedEdgeList::FaceIterator iterator = graph.GetFaces().begin();
				faceToSplit = random.RandRange(1, nFaces);
//				double largestSize = faceToSplit->Area();
				for (int i = 0; i < faceToSplit; i++)
					iterator++;
				if (splitFace(graph, *iterator, random))
					averageSize = overallSize / ++nFaces;
			}
		}
	}

	// =============================================================================
	// -- GraphGenerator -----------------------------------------------------------
	// =============================================================================

	TUniquePtr <DoublyConnectedEdgeList> GraphGenerator::Generate(const uint32 SizeX, const uint32 SizeY, const int32 Seed, const int32 averageSize)
	{
		using Position = typename DoublyConnectedEdgeList::Position;
		std::array<Position, 4u> box;
		box[0u] = Position(0, 0);
		box[1u] = Position(0, SizeY);
		box[2u] = Position(SizeX, SizeY);
		box[3u] = Position(SizeX, 0);
		auto Dcel = MakeUnique<DoublyConnectedEdgeList>(box);
		randomize(*Dcel, Seed, averageSize);
		return Dcel;
	}

} // namespace MapGen
