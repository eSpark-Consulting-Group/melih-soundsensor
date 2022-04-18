#pragma once

#include <list>
#include <algorithm>

namespace common {

template <typename T>
struct Vertex;

template <typename T>
struct Edge {
    Vertex<T>* vertex;
    bool visited;

    Edge() = delete;
    Edge( Vertex<T>* vertex )
    : vertex{ vertex } {}

    Edge( const Edge& ) = delete;
    Edge operator=( const Edge& ) = delete;
    Edge( const Edge&& ) = delete;
    Edge operator=( const Edge&& ) = delete;

    bool operator==( const Vertex<T>* other ) const {
        return vertex && other ? vertex == other : false;
    }

    bool operator!=( const Vertex<T>* other ) const {
        return !( *this == other );
    }
};

template <typename T>
struct Vertex {
    const T item;
    bool visited{ false };

    Vertex() = delete;
    Vertex( const T item )
    : item{ item } {}
    Vertex( const Vertex& ) = delete;
    Vertex operator=( const Vertex& ) = delete;
    Vertex( const Vertex&& ) = delete;
    Vertex operator=( const Vertex&& ) = delete;

    bool operator==( const T& other_item ) const {
        return item == other_item;
    }

    bool operator!=( const T& other_item ) const {
        return !( *this == other_item );
    }

    void addEdge( Vertex<T>* vertex ) {
        auto edge_it = std::find( m_edges.begin(), m_edges.end(), vertex );
        if( edge_it == m_edges.end() ) {
            m_edges.emplace_back( vertex );
        }
    }

    void removeEdge( const Vertex<T>* vertex ) {
        m_edges.remove_if( [&vertex]( const auto& edge ) { return edge == vertex; } );
    }

    void visitEdge( const Vertex<T>* vertex, bool visited = true ) {
        auto edge_it = std::find( m_edges.begin(), m_edges.end(), vertex );
        if( edge_it != m_edges.end() ) {
            edge_it->visited = visited;
        }
    }

    const std::list<Edge<T>>& edges() const {
        return m_edges;
    }

    bool hasEdge( const Vertex<T>* vertex ) const {
        return std::any_of( m_edges.begin(), m_edges.end(), [&vertex]( const Edge<T>& next_edge ) {
            return vertex && next_edge.vertex ? vertex == next_edge.vertex : false;
        } );
    }

private:
    std::list<Edge<T>> m_edges;
};

template <typename T>
class AdjacencyList {
public:
    std::list<T> vertices() const {
        std::list<T> vertices;
        for( const auto& vertex : m_vertices ) {
            vertices.push_back( vertex.item );
        }
        return vertices;
    }

    void clear() {
        m_vertices.clear();
    }

    void addVertex( const T& item ) {
        if( !findVertex( item ) ) {
            m_vertices.emplace_back( item );
        }
    }

    void removeVertex( const T& item ) {
        Vertex<T>* vertex = findVertex( item );
        if( vertex ) {
            for( const auto& next_vertex : m_vertices ) {
                next_vertex.removeEdge( vertex );
            }
            m_vertices.remove( vertex );
        }
    }

    bool addEdge( const T& from_item, const T& to_item ) {
        bool result{ true };
        Vertex<T>* from_vertex = findVertex( from_item );
        Vertex<T>* to_vertex = findVertex( to_item );
        if( from_vertex && to_vertex ) {
            from_vertex->addEdge( to_vertex );
        }
        else {
            result = false;
        }
        return result;
    }

    bool removeEdge( const T& from_item, const T& to_item ) {
        bool result{ false };
        Vertex<T>* from_vertex = findVertex( from_item );
        Vertex<T>* to_vertex = findVertex( to_item );
        if( from_vertex && to_vertex ) {
            from_vertex->removeEdge( to_vertex );
        }
        else {
            result = false;
        }
        return result;
    }

    std::list<T> getEdgesFrom( const T& item ) const {
        std::list<T> edges;
        const Vertex<T>* vertex = findVertex( item );
        if( vertex ) {
            for( auto& next_edge : vertex->edges() ) {
                edges.push_back( next_edge.vertex->item );
            }
        }
        return edges;
    }

    std::list<T> getEdgesTo( const T& item ) const {
        std::list<T> edges;
        const Vertex<T>* to_vertex = findVertex( item );
        if( to_vertex ) {
            for( const auto& next_vertex : m_vertices ) {
                if( next_vertex.hasEdge( to_vertex ) ) {
                    edges.push_back( next_vertex.item );
                }
            }
        }
        return edges;
    }

    std::list<T> topologicalSort( const T& root_item ) {
        Vertex<T>* root_vertex = findVertex( root_item );
        if( !root_vertex ) {
            return {};
        }
        std::list<T> sorted_vertices;
        resetVertices();
        std::list<Vertex<T>*> stack;
        stack.push_back( root_vertex );
        bool cycle_detected{ false };
        while( !stack.empty() && !cycle_detected ) {
            bool vertex_added{ false };
            for( auto& next_edge : stack.back()->edges() ) {
                if( !( next_edge.vertex->visited ) ) {
                    if( std::any_of( stack.begin(), stack.end(), [&next_edge]( const auto& next_vertex ) {
                            return next_edge.vertex == next_vertex;
                        } ) ) {
                        cycle_detected = true;
                        break;
                    }
                    stack.push_back( next_edge.vertex );
                    vertex_added = true;
                    break;
                }
            }
            if( !vertex_added && !cycle_detected ) {
                stack.back()->visited = true;
                sorted_vertices.push_back( stack.back()->item );
                stack.pop_back();
            }
        }
        if( cycle_detected ) {
            sorted_vertices.clear();
        }
        return sorted_vertices;
    }

    void reverseEdges() {
        resetEdges();
        for( auto& next_vertex : m_vertices ) {
            while( !next_vertex.edges().empty() && !next_vertex.edges().front().visited ) {
                Vertex<T>* from_vertex = next_vertex.edges().front().vertex;
                next_vertex.removeEdge( from_vertex );
                from_vertex->addEdge( &next_vertex );
                from_vertex->visitEdge( &next_vertex );
            }
        }
    }

private:
    Vertex<T>* findVertex( const T& item ) {
        auto vertex = std::find( m_vertices.begin(), m_vertices.end(), item );
        if( vertex != m_vertices.end() ) {
            return &*vertex;
        }
        return nullptr;
    }

    const Vertex<T>* findVertex( const T& item ) const {
        return const_cast<AdjacencyList*>( this )->findVertex( item );
    }

    void resetVertices() {
        for( auto& vertex : m_vertices ) {
            vertex.visited = false;
        }
    }

    void resetEdges() {
        for( auto& vertex : m_vertices ) {
            for( auto& edge : vertex.edges() ) {
                vertex.visitEdge( edge.vertex, false );
            }
        }
    }

private:
    std::list<Vertex<T>> m_vertices;
};

}  // namespace common
