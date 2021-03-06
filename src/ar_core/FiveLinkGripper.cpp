//
// Created by nima on 09/08/17.
//

#include "FiveLinkGripper.h"
#include <vtkProperty.h>

FiveLinkGripper::FiveLinkGripper(
        const std::vector<std::vector<double> > link_dims_)
        : link_dims_(link_dims_),
          num_links_(5)
{


    float gripper_density = 0; // kg/m3
    float gripper_friction = 50;
    double gripper_pose[7]{0, 0, 0, 0, 0, 0, 1};

    for (int i = 0; i < 5; ++i) {

        gripper_links[i] =
                new SimObject(ObjectShape::BOX, ObjectType::KINEMATIC,
                              link_dims_[i], KDL::Frame(), gripper_density,
                              gripper_friction);
        gripper_links[i]->GetActor()->GetProperty()->SetColor(0.65f,0.7f,0.7f);
        gripper_links[i]->GetActor()->GetProperty()->SetSpecularPower(50);
        gripper_links[i]->GetActor()->GetProperty()->SetSpecular(0.8);

        gripper_links[i]->GetBody()->setContactStiffnessAndDamping
                (2000, 100);
    }
}

void FiveLinkGripper::SetPoseAndJawAngle(const KDL::Frame pose,
                                       const double grip_angle) {

    KDL::Frame grpr_links_pose[5];

    //-------------------------------- LINK 0
    grpr_links_pose[0] = pose;
    grpr_links_pose[0].p  = grpr_links_pose[0] * KDL::Vector( 0.0 , 0.0,
                                                              -link_dims_[0][2]/2);
    double x, y, z, w;
    pose.M.GetQuaternion(x,y,z,w);
    double link0_pose[7] = {grpr_links_pose[0].p.x(),
                            grpr_links_pose[0].p.y(), grpr_links_pose[0].p.z(),x,y,z,w};
    gripper_links[0]->SetKinematicPose(link0_pose);

    //-------------------------------- LINK 1
    grpr_links_pose[1] = pose;
    grpr_links_pose[1].M.DoRotX(-grip_angle);
    grpr_links_pose[1].p =  grpr_links_pose[1] *
                            KDL::Vector( 0.0, 0.0, link_dims_[1][2]/2);
    grpr_links_pose[1].M.GetQuaternion(x, y, z, w);

    double link2_pose[7] = {grpr_links_pose[1].p.x(),
                            grpr_links_pose[1].p.y(), grpr_links_pose[1].p.z(), x, y, z, w};

    gripper_links[1]->SetKinematicPose(link2_pose);

    //-------------------------------- LINK 2
    grpr_links_pose[2] = pose;
    grpr_links_pose[2].M.DoRotX(grip_angle);
    grpr_links_pose[2].p =  grpr_links_pose[2] *
                            KDL::Vector( 0.0, 0.0, link_dims_[2][2]/2);
    grpr_links_pose[2].M.GetQuaternion(x, y, z, w);

    double link3_pose[7] = {grpr_links_pose[2].p.x(),
                            grpr_links_pose[2].p.y(), grpr_links_pose[2].p.z(), x, y, z, w};

    gripper_links[2]->SetKinematicPose(link3_pose);


    //-------------------------------- LINKS 3 and 4
    for (int i = 3; i < 5; ++i) {
        // first find the end point of links 1 and 2 and then add half length
        // of links 3 and 4
        grpr_links_pose[i] = pose;
        grpr_links_pose[i].p =
                grpr_links_pose[i-2] *
                KDL::Vector(0., 0.,link_dims_[i-2][2]/2)
                + grpr_links_pose[i].M *
                  KDL::Vector(0., 0.,link_dims_[i][2]/2);

        grpr_links_pose[i].M.GetQuaternion(x,y,z,w);
        double link_pose[7] = {grpr_links_pose[i].p.x(),
                               grpr_links_pose[i].p.y(), grpr_links_pose[i].p.z(),x, y, z, w};

        gripper_links[i]->SetKinematicPose(link_pose);
    }
}

void FiveLinkGripper::AddToWorld(btDiscreteDynamicsWorld * bt_world) {

    for (int i = 0; i < num_links_; ++i) {
        bt_world->addRigidBody(gripper_links[i]->GetBody());
    }

}

void FiveLinkGripper::AddToActorsVector(
        std::vector<vtkSmartPointer<vtkProp>> &actors) {
    for (int i = 0; i < num_links_; ++i) {
        actors.push_back(gripper_links[i]->GetActor());
    }
}

bool FiveLinkGripper::IsGraspingObject(btDiscreteDynamicsWorld* bt_world,
                                     btCollisionObject *obj) {

    MyContactResultCallback result0, result1;
    bt_world->contactPairTest(gripper_links[3]->GetBody(),
                              obj, result0);
    bool jaw1 = result0.connected;

    bt_world->contactPairTest(gripper_links[4]->GetBody(),
                              obj, result1);
    bool jaw2 = result1.connected;

    return (jaw1 & jaw2);

}

