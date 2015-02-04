/*
 * NodeComponent.h
 *
 *  Created on: Jan 10, 2013
 *      Author: chris
 */

#ifndef NODECOMPONENT_H_
#define NODECOMPONENT_H_
#include "physics/framework/IntegrateComponents.h"
#include "datatypes/node.h"

/**
 * wrapper for a node_t so it can work with UpdateComponent
 */
class NodeComponent
: public Framework::Components::PhysicalComponent<Units::Meter, Units::MetersPerSecond, Units::Newton>
, public Framework::Components::MassiveComponent<Units::Kilogram>
{
	
public:
	NodeComponent(node_t* node) : _node(node) { } 
	virtual ~NodeComponent() { _node = NULL; } 
	
	// implement for PhysicalComponent
	virtual void applyForce(Units::Newton force) 	{ _node->Forces += force; };

	Units::Newton getForces() const { return _node->Forces; }
	Units::Meter getPosition() const {return _node->AbsPosition; }
	Units::MetersPerSecond getVelocity() const { return _node->Velocity; }
	
	// implement for MassiveComponent
	void setMass(Units::Kilogram mass) { _node->mass = mass; }
	Units::Kilogram getMass() const { return _node->mass; }

	//
	int getId() const { return _node->id; }
	node_t* getNode() const { return _node; }
	void setNode(node_t* node) { _node = node; }
private:
	node_t* _node;
};



#endif /* NODECOMPONENT_H_ */
