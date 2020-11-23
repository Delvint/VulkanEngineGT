#ifndef CONTACT_H
#define CONTACT_H

#include <iostream>

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

#include "hash.h"
#include <random>
#include <set>

#include "collider.h"
#include "sat.h"

namespace gjk {

	using vec3pair = std::pair<vec3, vec3>;


	struct contact {
		Polytope* obj1;
		Polytope* obj2;
		vec3 pos;
		vec3 normal;

		bool operator<(const contact& rhs) const noexcept {
			return glm::length(this->pos) < glm::length(rhs.pos);
		}
		bool operator==(const contact& rhs)const noexcept {
			return this->pos == rhs.pos;
		}
	};


	/*template<>
	struct std::hash<face_contact> {
	   size_t operator()(const face_contact& c) {
		   return std::hash<std::pair<int,int>>()( std::make_pair(c.f1, c.f2));
	   }
	};*/

	void create_vertex_face_contact(Polytope& obj1, Polytope& obj2, glm::vec3 pos, int f2, std::set<contact>& contacts) {
		contact contact;
		contact.obj1 = &obj1;
		contact.obj2 = &obj2;
		contact.pos = pos;
		contact.normal = obj1.get_face_normal(f2);
		contacts.insert(contact);
	}


	void process_vertex_face_contact(Polytope& obj1, Polytope& obj2, int v1, int f2, std::set<contact>& contacts) {
		create_vertex_face_contact(obj1, obj2, obj1.m_points[v1], f2, contacts);
		std::vector<Line> obj1Edges;
		obj1.get_edges(obj1Edges);
		//Go through each point on the edge and add it to contacts
		std::for_each(std::begin(obj1Edges), std::end(obj1Edges),
			[&](Line l) {
			//if Start or endpoint are equal to initial contact point
			//go by steps of 0.5. Determines the precision.
			float stepsize = .5f;
			glm::vec3 line_end = l.m_pos + l.m_dir;
			glm::vec3 n_dir = glm::normalize(l.m_dir);
			if (l.m_pos == obj1.m_points[v1]) {
				// for(glm::vec3 p = l.m_pos; glm::distance(p,line_end) > stepsize; p+=n_dir*stepsize){
				create_vertex_face_contact(obj1, obj2, l.m_pos, f2, contacts);
				// }
			}
			if (line_end == obj1.m_points[v1]) {
				// for(glm::vec3 p = l.m_pos; glm::distance(p,line_end) > stepsize; p+=n_dir*stepsize){
				create_vertex_face_contact(obj1, obj2, line_end
					, f2, contacts);
				// }
			}
		});
	}

	void process_edge_edge_contact(Line& edge1, Line& edge2, std::set<contact>& contacts) {
		contact contact;
		contact.normal = glm::cross(edge1.m_dir, edge2.m_dir);
		contact.pos = edge1.intersect(edge2);
		// if (!isnan(contact.pos.x) &&
		//     !isnan(contact.pos.y) &&
		//     !isnan(contact.pos.z))
		contacts.insert(contact);
	}

	//test a pair of faces against each other.
	//collide each vertex from one face with the other face
	//collide each edge from one face with all edges from the other face
	void process_face_face_contact(Polytope& obj1, Polytope& obj2, vec3& dir
		, int f1, int f2, std::set<contact>& contacts) {

		Face& face1 = obj1.m_faces[f1];
		Face& face2 = obj1.m_faces[f2];

		for (int v1 : face1.m_data->m_vertices) {      //go through all vertices of face 1
			if (sat(obj1.m_vertices[v1], face2, dir)) {
				process_vertex_face_contact(obj1, obj2, v1, f2, contacts);
			}
		}

		for (int v2 : face2.m_data->m_vertices) {      //go through all vertices of face 2
			if (sat(obj2.m_vertices[v2], face1, dir)) {
				process_vertex_face_contact(obj2, obj1, v2, f1, contacts);
			}
		}

		std::vector<Line> edges1;
		std::vector<Line> edges2;
		face1.get_edges(edges1);
		face2.get_edges(edges2);

		for (auto& edge1 : edges1) {      //go through all edge pairs
			for (auto& edge2 : edges2) {
				if (sat(edge1, edge2, dir)) {
					process_edge_edge_contact(edge1, edge2, contacts);
				}
			}
		}
	}


	//find a list of face-pairs that touch each other
	//process these pairs by colliding a face agains vertices and edges of the other face
	void process_face_obj_contacts(Polytope& obj1, Polytope& obj2, vec3& dir
		, std::vector<int>& obj1_faces, std::vector<int>& obj2_faces
		, std::set<contact>& contacts) {

		for (int f1 : obj1_faces) {             // go through all face-face pairs
			for (int f2 : obj2_faces) {
				if (sat(obj1.m_faces[f1], obj2.m_faces[f2], dir)) {           //only if the faces actually touch - can also drop this if statement
					process_face_face_contact(obj1, obj2, dir, f1, f2, contacts); //compare all vertices and edges in the faces
				}
			}
		}
	}


	//find a face of obj1 that touches obj2
	//return it and its neighbors by adding their indices to a list
	void get_face_obj_contacts(Polytope& obj1, Polytope& obj2, vec3& dir, std::vector<int>& obj_faces) {
		for (int f = 0; f < obj1.m_faces.size(); ++f) {
			Face& face = obj1.m_faces[f];
			if (sat(face, obj2, dir)) {              //find the first face from obj1 that touches obj2
				obj_faces.push_back(f);                    //insert into result list
				auto& neighbors = obj1.get_face_neighbors(f);
				std::copy(std::begin(neighbors), std::end(neighbors), std::back_inserter(obj_faces));   //also insert its neighbors
				return;
			}
		}
	}


	//neighboring faces algorithm
	void neighboring_faces(Polytope& obj1, Polytope& obj2, vec3& dir, std::set<contact>& contacts) {
		std::vector<int> obj1_faces;
		std::vector<int> obj2_faces;

		get_face_obj_contacts(obj1, obj2, dir, obj1_faces);    //get list of faces from obj1 that touch obj2
		get_face_obj_contacts(obj2, obj1, dir, obj2_faces);    //get list of faces from obj2 that touch obj1
		process_face_obj_contacts(obj1, obj2, dir, obj1_faces, obj2_faces, contacts); //collide them pairwise
	}


	//compute a list of contact points between two objects
	void  contacts(Polytope& obj1, Polytope& obj2, vec3& dir, std::set<contact>& contacts) {
		if (dot(dir, dir) < 1.0e-6) dir = vec3(0.0f, 1.0f, 0.0f);
		neighboring_faces(obj1, obj2, dir, contacts);
	}


}
#endif