//
// Created by nima on 14/06/17.
//

#ifndef ATAR_BULLETVTKMOTIONSTATE_H
#define ATAR_BULLETVTKMOTIONSTATE_H


#include <LinearMath/btMotionState.h>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkMatrix4x4.h>
#include <kdl/frames.hpp>
#include <mutex>

#define B_DIM_SCALE 100.0f
//==============================================================================
/*!
    \class      BulletVTKMotionState
    \brief
    This class simplifies the graphics and dynamic objects synchronization.
    \details
	If the dynamic object moves the setWorldTransform will be called by
    bullet and we set the pose of the vtk actor.
    If the object is kinematic (e.g. a tool)the getWorldTransform method
    is called at every loop, and the pose of the object must be set
    externally using the setKinematicPos method.

    IMPORTANT NOTE: We were interested in objects with dimensions in the
    order of a few mm. It turned out that the bullet simulation
    becomes unstable for such small dimensions. To get around this, the
    dimensions of all the bullet related things are multiplied by B_DIM_SCALE.

    \author    Nima Enayati

*/

class BulletVTKMotionState : public btMotionState{

protected:
    vtkSmartPointer<vtkActor>   actor_;
    btTransform                 bt_pose_;
    KDL::Frame                  frame;

public:
    BulletVTKMotionState(const KDL::Frame &pose,
                         vtkSmartPointer<vtkActor> actor)
        : actor_(actor){

        btTransform init_transform;
        init_transform.setIdentity();
        init_transform.setOrigin(btVector3(float(B_DIM_SCALE*pose.p[0]),
                                           float(B_DIM_SCALE*pose.p[1]),
                                           float(B_DIM_SCALE*pose.p[2]) ));
        double qx, qy,qz, qw;
        pose.M.GetQuaternion(qx, qy,qz, qw);
        init_transform.setRotation(btQuaternion((float)qx, (float)qy,
                                                (float)qz, (float)qw ));
        bt_pose_ = init_transform;
    }


    // -------------------------------------------------------------------------
    virtual ~BulletVTKMotionState(){}


    // -------------------------------------------------------------------------
    //! This shouold not really be needed!
//    void setActor(vtkSmartPointer<vtkActor> actor){ actor_ = actor; }


    // -------------------------------------------------------------------------
    //! called by bullet at initialization for all objects and if kinematic
    //! object it is called at each loop
    virtual void getWorldTransform(btTransform &worldTrans) const {
        worldTrans = bt_pose_;
    }


    // -------------------------------------------------------------------------
    //! called by user to get the pose of the object
    KDL::Frame getKDLFrame() {
        KDL::Frame out = frame;
        return out;
    }


    // -------------------------------------------------------------------------
    //! Called by bullet to set the pose of dynamic objects
    virtual void setWorldTransform(const btTransform &worldTrans) {

        bt_pose_ = worldTrans;

        // Set the orientation
        btQuaternion rot = worldTrans.getRotation();
        btVector3 pos = worldTrans.getOrigin();
        KDL::Frame pose_kdl(
            KDL::Rotation::Quaternion( rot.x(),  rot.y(), rot.z(), rot.w()),
            KDL::Vector(pos.x(), pos.y(), pos.z())/B_DIM_SCALE);

        frame = pose_kdl;

        // Convert to VTK matrix.
        vtkSmartPointer<vtkMatrix4x4> vtk_mat =
            vtkSmartPointer<vtkMatrix4x4>::New();

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                vtk_mat->SetElement(i, j, pose_kdl.M(i,j));
            }
            vtk_mat->SetElement(i, 3, pose_kdl.p[i]);
        }

        actor_->SetUserMatrix(vtk_mat);
        // set the position
        //actor_->SetPosition(pos.x(), pos.y(), pos.z());

    }


    // -------------------------------------------------------------------------
    //! Used by the user to set the pose of kinematic object
    void setKinematicPos(btTransform &currentPos) {

        bt_pose_ = currentPos;
        btQuaternion rot = bt_pose_.getRotation();
        btVector3 pos = bt_pose_.getOrigin();

        KDL::Frame pose_kdl(
            KDL::Rotation::Quaternion( rot.x(),  rot.y(), rot.z(), rot.w()),
            KDL::Vector(pos.x(), pos.y(), pos.z())/B_DIM_SCALE);

        frame = pose_kdl;

        // Convert to VTK matrix.
        vtkSmartPointer<vtkMatrix4x4> vtk_mat =
            vtkSmartPointer<vtkMatrix4x4>::New();

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                vtk_mat->SetElement(i, j, pose_kdl.M(i,j));
            }
            vtk_mat->SetElement(i, 3, pose_kdl.p[i]);
        }

        actor_->SetUserMatrix(vtk_mat);

    }

};


#endif //ATAR_BULLETVTKMOTIONSTATE_H
