//
// Created by nima on 15/06/17.
//

#include "SimObject.h"
#include "LoadObjGL/LoadMeshFromObj.h"
#include <kdl/frames.hpp>
// vtk headers
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>
#include <vtkTransform.h>
#include <vtkConeSource.h>
#include <vtkCylinderSource.h>
#include <vtkOBJReader.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>
//for debug message
#include "ros/ros.h"
#include <sys/stat.h>

inline bool FileExists (const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

SimObject::SimObject(const ObjectShape shape, const ObjectType o_type,
                     const std::vector<double> dimensions,
                     const KDL::Frame &pose,
                     const double density, const double friction,
                     const std::string mesh_address, const int id)
        : object_type_(o_type), id_(id)
{
    // -------------------------------------------------------------------------
    // initializations
    vtkSmartPointer<vtkPolyDataMapper> mapper =
            vtkSmartPointer<vtkPolyDataMapper>::New();
    actor_ = vtkSmartPointer<vtkActor>::New();
    double volume = 0.0;
    std::string shape_string; // for debug report

    // -------------------------------------------------------------------------
    // generate vtk actors, collision shapes and calculate volume (used to
    // find the mass for physics) according to the passed shape.
    switch (shape){

        //
        // ---------------------------------------------------------------------
        // Static Plane
        case STATICPLANE : {
            // check if we have all the dimensions
            if (dimensions.size() != 4)
                throw std::runtime_error("SimObject STATICPLANE "
                                                 "shape requires a vector of 4 "
                                                 "doubles as dimensions.");

            collision_shape_ = new btStaticPlaneShape(
                    btVector3(btScalar(B_DIM_SCALE*dimensions[0]),
                              btScalar(B_DIM_SCALE*dimensions[1]),
                              btScalar(B_DIM_SCALE*dimensions[2])),
                    btScalar(B_DIM_SCALE*dimensions[3]) );

            volume = 0.0;
            shape_string = collision_shape_->getName();

            break;
        }

        case SPHERE : {
            // -----------------------------------------------------------------
            // SPHERE
            //
            // check if we have all the dimensions
            if (dimensions.size() != 1)
                throw std::runtime_error(
                        "SimObject SPHERE shape requires a vector of 1 double "
                                "as dimensions.");
            // VTK actor_
            vtkSmartPointer<vtkSphereSource> source =
                    vtkSmartPointer<vtkSphereSource>::New();

            source->SetRadius(dimensions[0]);
            source->SetPhiResolution(30);
            source->SetThetaResolution(30);
            mapper->SetInputConnection(source->GetOutputPort());

            // Bullet Shape
            collision_shape_ = new btSphereShape(btScalar(B_DIM_SCALE*dimensions[0]));

            // calculate volume
            volume = 4/3*M_PI* pow(dimensions[0], 3);

            // set name
            shape_string = collision_shape_->getName();

            break;
        }

        case CYLINDER : {
            // -----------------------------------------------------------------
            // CYLINDER
            //
            // check if we have all the dimensions
            if (dimensions.size() != 2)
                throw std::runtime_error("SimObject CYLINDER shape requires "
                                                 "a vector of 2 doubles "
                                                 "as dimensions.");
            // VTK actor_
            vtkSmartPointer<vtkCylinderSource> source =
                    vtkSmartPointer<vtkCylinderSource>::New();

            source->SetRadius(dimensions[0]);
            source->SetHeight(dimensions[1]);
            source->SetResolution(30);
            mapper->SetInputConnection(source->GetOutputPort());

            // Bullet Shape
            collision_shape_ =
                    new btCylinderShape(
                            btVector3(btScalar(B_DIM_SCALE*dimensions[0]),
                                      btScalar(B_DIM_SCALE*dimensions[1]/2), 0.0
                            ));

            // calculate volume
            volume = M_PI * pow(dimensions[0], 2) * dimensions[1];

            // set name
            shape_string = collision_shape_->getName();

            break;
        }

        case BOX : {
            // -----------------------------------------------------------------
            // BOX
            //
            // check if we have all dimensions
            if (dimensions.size() != 3)
                throw std::runtime_error("SimObject BOX shape requires "
                                                 "a vector of three doubles "
                                                 "as dimensions.");

            // VTK actor_
            vtkSmartPointer<vtkCubeSource> board_source =
                    vtkSmartPointer<vtkCubeSource>::New();

            board_source->SetXLength(dimensions[0]);
            board_source->SetYLength(dimensions[1]);
            board_source->SetZLength(dimensions[2]);

            mapper->SetInputConnection(board_source->GetOutputPort());

            // Bullet Shape
            collision_shape_ = new btBoxShape(
                    btVector3(btScalar(B_DIM_SCALE*dimensions[0]/2),
                              btScalar(B_DIM_SCALE*dimensions[1]/2),
                              btScalar(B_DIM_SCALE*dimensions[2]/2)));
            // calculate volume
            volume = dimensions[0] *
                     dimensions[1] *
                     dimensions[2];

            // set name
            shape_string = collision_shape_->getName();;

            break;
        }
        case CONE : {
            // -----------------------------------------------------------------
            // CONE
            //
            // check if we have all dimensions
            if (dimensions.size() != 2)
                throw std::runtime_error("SimObject CONE shape requires "
                                                 "a vector of two doubles "
                                                 "as dimensions.");

            // VTK actor_
            vtkSmartPointer<vtkConeSource> source =
                    vtkSmartPointer<vtkConeSource>::New();

            source->SetRadius(dimensions[0]);
            source->SetHeight(dimensions[1]);
            source->SetResolution(30);

            mapper->SetInputConnection(source->GetOutputPort());

            // Bullet Shape
            collision_shape_ = new btConeShape(
                    btScalar(B_DIM_SCALE*dimensions[0]),
                    btScalar(B_DIM_SCALE*dimensions[1]/2));

            // calculate volume
            volume = float(M_PI* pow(dimensions[0], 2) *
                           dimensions[1]/3);

            // set name
            shape_string = collision_shape_->getName();;

            break;
        }

        case MESH : {
            // -----------------------------------------------------------------
            // MESH
            //
            if(o_type!=NOPHYSICS) {
                if (!FileExists(mesh_address.c_str())) {
                    ROS_ERROR("Can't open mesh file: %s", mesh_address.c_str());
                    throw std::runtime_error("Can't open mesh file.");
                } else
                    ROS_DEBUG("Loading mesh file from: %s", mesh_address.c_str());

                collision_shape_ =
                        LoadCompoundMeshFromObj(mesh_address, B_DIM_SCALE);
                shape_string = collision_shape_->getName();;
            } else
                collision_shape_ = NULL;
            // set name

            // -----------------------------
            // VTK TODO: We have already read the Mesh object, we should use
            // the vertices from that instead of reading it again with VTK
            // reader.
            vtkSmartPointer<vtkOBJReader> reader =
                    vtkSmartPointer<vtkOBJReader>::New();
            //if(o_type!=NOPHYSICS) {
            //
            //    //
            //    ////            // visualize the compund mesh for debug
            //    size_t last_dot_position = filepath->find_last_of(".");
            //
            //    std::string file_name_no_extension = filepath->substr(
            //        0,
            //        last_dot_position
            //    );
            //
            //    std::stringstream out_name;
            //    out_name << file_name_no_extension << "_hacd.obj";
            //
            //    reader->SetFileName(out_name.str().c_str());
            //} else
            reader->SetFileName(mesh_address.c_str());

            reader->Update();
            mapper->SetInputConnection(reader->GetOutputPort());

            // calculate the volume
            vtkSmartPointer<vtkMassProperties> Mass =
                    vtkSmartPointer<vtkMassProperties>::New();
            vtkSmartPointer<vtkTriangleFilter> tri_filt =
                    vtkSmartPointer<vtkTriangleFilter>::New();
            tri_filt->SetInputConnection(reader->GetOutputPort());
            Mass->SetInputConnection(tri_filt->GetOutputPort());
            volume = Mass->GetVolume();
            //            // transform would be necesary at this stage if
            // cellLocator is going to be used
            //            vtkSmartPointer<vtkTransform> transform =
            //                    vtkSmartPointer<vtkTransform>::New();
            //            transform->Translate(0.050, 0.060, 0.025);
            //            transform->RotateX(180);
            //            transform->RotateZ(150);
            //
            //            vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter =
            //                    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            //            transformFilter->SetInputConnection(
            //                    reader->GetOutputPort());
            //            transformFilter->SetTransform(transform);
            //            transformFilter->Update();
            //
            //            vtkSmartPointer<vtkPolyDataMapper> mapper =
            //                    vtkSmartPointer<vtkPolyDataMapper>::New();
            //            mapper->SetInputConnection(
            //                    transformFilter->GetOutputPort());

            break;
        }
    }

    actor_->SetMapper(mapper);

    //--------------------------------------------------------------------------
    // set up dynamics
    // rigid body_ is dynamic if mass is non zero, otherwise it is static.
    if(object_type_==NOPHYSICS)
        actor_->SetUserMatrix(KDLFrameToVTKMatrix(pose));
    else{

        btScalar bt_mass = float(volume * density);

        bool isStatic = (bt_mass == 0.f);
        btVector3 local_inertia(0, 0, 0);

        if (isStatic)
            actor_->SetUserMatrix(KDLFrameToVTKMatrix(pose));

        if (!isStatic && (object_type_ != ObjectType::KINEMATIC))
            // Set initial pose of graphical representation
            collision_shape_->calculateLocalInertia(bt_mass, local_inertia);

        // ensure zero mass when Kinematic
        if (object_type_ == ObjectType::KINEMATIC)
            bt_mass = 0.f;

        // construct a motion state to connect the pose of the graphical
        // representation to that of the dynamic one
        motion_state_ = new BulletVTKMotionState(pose, actor_);

        // construct body_ info
        btRigidBody::btRigidBodyConstructionInfo body_info(
                bt_mass, motion_state_, collision_shape_, local_inertia);
        //        body_info.m_restitution = (btScalar) restitution;
        //        body_info.m_friction = (btScalar) friction;

        rigid_body_ = new btRigidBody(body_info);

        // set appropriate flags if the body is kinematic
        if (object_type_ == ObjectType::KINEMATIC) {
            rigid_body_->setCollisionFlags(rigid_body_->getCollisionFlags() |
                                           btCollisionObject::CF_KINEMATIC_OBJECT);
            rigid_body_->setActivationState(DISABLE_DEACTIVATION);
        }


        // to prevent  objects from rolling forever we add a bit of rolling
        // friction
        if (shape == ObjectShape::CONE ||
            shape == ObjectShape::CYLINDER ||
            shape == ObjectShape::BOX
                ){
            rigid_body_->setRollingFriction(btScalar(0.001));
            rigid_body_->setSpinningFriction(btScalar(0.001));
            //            body_->setAnisotropicFriction
            //                    (collision_shape_->getAnisotropicRollingFrictionDirection
            //                            (),btCollisionObject::CF_ANISOTROPIC_ROLLING_FRICTION);
        }
        else if(shape == ObjectShape::SPHERE) {
            rigid_body_->setRollingFriction(btScalar(0.001));
            rigid_body_->setSpinningFriction(btScalar(0.001));
        }

        rigid_body_->setFriction(btScalar(friction));
        rigid_body_->setSpinningFriction(btScalar(0.001));

        ////set contact parameters
        //body_->setContactStiffnessAndDamping(btScalar(10000),
        //                                     btScalar(0.1));

        std::stringstream debug_msg;

        debug_msg << std::string(" shape = ") << shape_string
                  << ", mass = " << bt_mass
                  << ", volume = " << volume
                  << ", friction = " << rigid_body_->getFriction()
                  << ", Rolling Friction = " << rigid_body_->getRollingFriction()
                  << ", Spinning Friction = " << rigid_body_->getSpinningFriction();
        ROS_DEBUG("Created SimObject with properties: %s", debug_msg.str()
                .c_str());
    }
}


//------------------------------------------------------------------------------
SimObject::~SimObject() {
// deleting from outside
//    delete collision_shape_;
//    delete motion_state_;
//    delete body_;
}


//------------------------------------------------------------------------------
void SimObject::SetKinematicPose(double *pose) {

    if(object_type_==KINEMATIC){

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(float(B_DIM_SCALE*pose[0]),
                                      float(B_DIM_SCALE*pose[1]),
                                      float(B_DIM_SCALE*pose[2]) ));;
        transform.setRotation(btQuaternion((float) pose[3], (float) pose[4],
                                           (float) pose[5], (float) pose[6]));

        motion_state_->setKinematicPos(transform);
//        body_->setWorldTransform(transform);
    }
}

KDL::Frame SimObject::GetPose() {
    KDL::Frame out = motion_state_->getKDLFrame();
    return out;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkMatrix4x4> PoseArrayToVTKMatrix(double *pose) {

    vtkSmartPointer<vtkMatrix4x4> out =
            vtkSmartPointer<vtkMatrix4x4>::New();

    KDL::Frame k(KDL::Rotation::Quaternion( pose[3],  pose[4],
                                            pose[5],  pose[6])
            , KDL::Vector(pose[0], pose[1], pose[2]) );

    // Convert to VTK matrix.
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            out->SetElement(i, j, k.M(i,j));
        }
        out->SetElement(i, 3, k.p[i]);
    }

    return out;
}
//------------------------------------------------------------------------------
vtkSmartPointer<vtkMatrix4x4> KDLFrameToVTKMatrix(const KDL::Frame &pose) {

    vtkSmartPointer<vtkMatrix4x4> out =
            vtkSmartPointer<vtkMatrix4x4>::New();

    // Convert to VTK matrix.
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            out->SetElement(i, j, pose.M(i,j));
        }
        out->SetElement(i, 3, pose.p[i]);
    }

    return out;
}

//KDL::Frame VTKMatrixToKDLFrame(const vtkSmartPointer<vtkMatrix4x4>vtk_mat_in) {
//    KDL::Frame out;
//
//    // Convert to VTK matrix.
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 3; j++) {
//            out.M(i,j) = vtk_mat_in->GetElement(i, j);
//        }
//        out.p[i] = vtk_mat_in->GetElement(i, 3);
//    }
//    return out;
//}


