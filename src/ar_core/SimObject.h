//
// Created by nima on 15/06/17.
//

#ifndef ATAR_SIMOBJECT_H
#define ATAR_SIMOBJECT_H

#include "BulletVTKMotionState.h"
#include <btBulletDynamicsCommon.h>
#include <vector>

/**
 * \class SimObject
 * \brief This class represents a simulated object with graphics and physics.
 *
 * When a SimObject is generated its graphic actor (actor_) and physics body
 * (rigid_body_) has to be added to the renderer and physics world.
 * After which, the pose of the graphical representation will be
 * automatically updated by the physics simulation, thanks the
 * toBulletVTKMotionState member. To learn more about how to add simObjects
 * to the simulation world refer to VTKTask.h
 *
 * \arg To construct a SimObject at least 3 arguments are needed:
 *
 *      ObjectShape: Can be predefined shapes (ex: ObjectType::SPHERE) or mesh
 *      (ObjectType::MESH). In the Mesh case the address of the .obj file
 *      must be provided with the mesh_address argument.
 *
 *      dimensions: If ObjectShape is one of the predefined shapes the
 *      dimensions must be specified as an array described in ObjectShape
 *      enum definition.
 *
 *      ObjectType: Four cases
 *          1. NOPHYSICS: The physics will not be initialized.
 *          2. KINEMATIC: The pose of the object will be set at each
 *          iteration using the SetKinematicPose. This is useful for things
 *          like tools that their pose is read from a master device.
 *          3. DYNAMIC and density!=0.0 The pose of the object will be
 *          automatically update from the physics simulation.
 *          4. DYNAMIC and no density provided (density defults to 0.0) in
 *          this case the object will be Static, meaning its pose will be
 *          constant but dynamic objects can collide with the object
 * Other params:
 *      Density: Is needed for dynamic objects to calculate their mass.
 *      pose: The Initial pose of the dynamics objects.
 *      friction: friction!
 *      id: can be used in collision detection
 *
 * \attention MESH:  Meshes are decomposed into approximated compound meshes
 * using the VHACD method. Generating the approximated compound mesh can take
 * up to a few minutes. So we do this only once and save the compound mesh in
 * a separate file that has the same name of the original mesh file with an
 * added _hacd. Next time the application is executed we search for the file
 * with _hacd and if found, it is used and compound mesh generation is not
 * repeated.
 *
 * \attention Dimension scaling: We were interested in objects with
 * dimensions in the order of a few millimiters. It turned out that the bullet
 * simulation becomes unstable for such small dimensions. To get around this,
 * the dimensions of all the bullet related things are multiplied by
 * B_DIM_SCALE.
 *
 * \author Nima Enayati
 * \date June 2017
 */




// -----------------------------------------------------------------------------
enum ObjectType {
    NOPHYSICS,  // Just graphics
    DYNAMIC,    // If density==0.0 object is STATIC
    KINEMATIC,   // Set the pose of the object externally
};

// -----------------------------------------------------------------------------
enum ObjectShape {
    STATICPLANE,    //dims = [normal_x, normal_y, normal_z]
    SPHERE,         //dims = [radius]
    CYLINDER,       //dims = [radius, height]
    BOX,            //dims = [width, length, height]
    CONE,           //dims = [radius, height]
    MESH
};

// -----------------------------------------------------------------------------
class SimObject {

public:
    /**
    * SimObject constructor.
    * Needs at least 3 arguments.
    */
    SimObject(const ObjectShape shape, const ObjectType type,
              const std::vector<double> dimensions,
              const KDL::Frame &pose=KDL::Frame(),
              const double density=0.0, const double friction = 0.1,
              const std::string mesh_address = {}, const int id = 0);

    ~SimObject();

    /**
    * Returns the rigid body member of the object.
    */
    btRigidBody* GetBody() { return rigid_body_; }

    /**
    * Returns the VTK actor member of the object.
    */
    vtkSmartPointer<vtkActor> GetActor() { return actor_; };

    /**
    * Set the pose if the object is kinematic.
    */
    void SetKinematicPose(double pose[]);

    /**
    * Can be used in physics collision handling
    */
    int GetId() {return id_; };

    /**
    * Get the pose of the object.
    */
    KDL::Frame GetPose();

private:

    int id_;
    ObjectType object_type_;
    btRigidBody* rigid_body_;
    vtkSmartPointer<vtkActor> actor_;
    BulletVTKMotionState* motion_state_;
    btCollisionShape* collision_shape_;

};

// -----------------------------------------------------------------------------
// helper functions
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
vtkSmartPointer<vtkMatrix4x4> PoseArrayToVTKMatrix(double *pose);

vtkSmartPointer<vtkMatrix4x4> KDLFrameToVTKMatrix(const KDL::Frame &pose);

//KDL::Frame VTKMatrixToKDLFrame(const vtkSmartPointer<vtkMatrix4x4>);

// -----------------------------------------------------------------------------
/**
* callback for pairwise collision test.
*/
struct MyContactResultCallback : public btCollisionWorld::ContactResultCallback
{
    bool connected;
    btScalar margin;
    MyContactResultCallback() :connected(false),
                               margin(0.001f*B_DIM_SCALE) {}

    virtual btScalar addSingleResult(btManifoldPoint& cp,
                                     const btCollisionObjectWrapper* colObj0Wrap,
                                     int partId0,int index0,
                                     const btCollisionObjectWrapper* colObj1Wrap,
                                     int partId1,int index1){
        if (cp.getDistance()<=margin)
            connected = true;
        return 1.f;
    }
};


// ----------------------------------------------------------------------------
#endif //ATAR_SIMOBJECT_H
