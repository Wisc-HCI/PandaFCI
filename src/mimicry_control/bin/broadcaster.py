__author__ = 'drakita'
import numpy as N
import math
from robotiq_85_msgs.msg import GripperCmd, GripperStat
import rospy
from trajectory_msgs.msg import JointTrajectory, JointTrajectoryPoint
from sensor_msgs.msg import JointState
# from robotiq_85_msgs.msg import GripperCmd, GripperStat
from copy import deepcopy
import numpy as np

def pubVREP(vrepFile, xopt):
    # solution file
    xoptS = ",".join(str(e) for e in xopt)
    f = open(vrepFile, 'w')
    f.write(xoptS)
    f.close()

def pubROS(vars):
    # TODO: complete this
    rotDisp = N.array(vars.rosDisp)
    a = []

def move_ur5(ur_sock, xopt, disp, time=0.08):
    '''
    author: drakita
    moves the physical ur5 robot using urscript
    this uses the command servoj to be used for real-time streaming commands, no interpolation is used
    :param q: configuration of robot in radians [q1, q2, q3 ,..., q6]
    :param sock: socket that communicates with robot via urscript (port number should be 30002)
    :return:
    '''

    xopt = N.array(xopt)
    disp = N.array(disp)
    if not len(xopt) == 6:
        return
    xopt = (xopt + disp).tolist()
    q = xopt
    gain = '300'
    # switched t=.08 to t=.1
    command = "servoj([{0},{1},{2},{3},{4},{5}],t={7},gain={6})".format(str(q[0]),str(q[1]),str(q[2]),str(q[3]),str(q[4]),str(q[5]),gain,time) + "\n"

    ur_sock.send(command)

def move_ur5_vel(ur_sock, vel, time=0.008):
    '''
    author: drakita
    moves the physical ur5 robot using urscript
    this uses the command speedj to be used for real-time streaming commands, no interpolation is used
    :param q: configuration of robot in radians [q1, q2, q3 ,..., q6]
    :param sock: socket that communicates with robot via urscript (port number should be 30002)
    :return:
    '''
    command = "speedj([{0},{1},{2},{3},{4},{5}],6.0,{6})".format(str(vel[0]),str(vel[1]),str(vel[2]),str(vel[3]),str(vel[4]),str(vel[5]),time) + "\n"
    ur_sock.send(command)

'''
import actionlib
import math
import kinova_msgs.msg
prefix = 'm1n6s200_'
action_address = '/' + prefix + 'driver/joints_action/joint_angles'
client = actionlib.SimpleActionClient(action_address,
                                          kinova_msgs.msg.ArmJointAnglesAction)
def move_mico(xopt, duration=0.15):
    client.wait_for_server()

    goal = kinova_msgs.msg.ArmJointAnglesGoal()

    goal.angles.joint1 = (xopt[0]/math.pi)*180.0
    goal.angles.joint2 = (xopt[1]/math.pi)*180.0
    goal.angles.joint3 = (xopt[2]/math.pi)*180.0
    goal.angles.joint4 = (xopt[3]/math.pi)*180.0
    goal.angles.joint5 = (xopt[4]/math.pi)*180.0
    goal.angles.joint6 = (xopt[5]/math.pi)*180.0
    goal.angles.joint7 = 0.0

    client.send_goal(goal)
    if client.wait_for_result(rospy.Duration(duration)):
        return client.get_result()
    else:
        print('        the joint angle action timed-out')
        client.cancel_all_goals()
        return None
'''

# rostopic pub -r 100 /m1n4s200_driver/in/joint_velocity kinova_msgs/JointVelocity "{joint1: 0.0, joint2: 0.0, joint3: 0.0, joint4: 10.0}
'''
from kinova_msgs.msg import JointVelocity
mico_vel_pub = rospy.Publisher('/m1n6s200_driver/in/joint_velocity',JointVelocity, queue_size=5)
def move_mico_velocity(xopt, prev_xopt):
    updates_per_second = 60.0
    # xopt = (np.array(xopt) + np.array(disp)).tolist()
    s = 1.0
    jv = JointVelocity()
    vel = np.array(xopt) - np.array(prev_xopt)
    jv.joint1 = s*((vel[0]/math.pi)*180.0)
    jv.joint2 = s*((vel[1]/math.pi)*180.0)
    jv.joint3 = s*((vel[2]/math.pi)*180.0)
    jv.joint4 = s*((vel[3]/math.pi)*180.0)
    jv.joint5 = s*((vel[4]/math.pi)*180.0)
    jv.joint6 = s*((vel[5]/math.pi)*180.0)
    jv.joint7 = 0.0

    # print (s*vel).tolist()
    # print jv
    mico_vel_pub.publish(jv)
'''

def move_sawyer(xopt, limb, timeout=1.0, threshold=0.07):
    angles = limb.joint_angles()
    angles['right_j0'] = xopt[0]
    angles['right_j1'] = xopt[1]
    angles['right_j2'] = xopt[2]
    angles['right_j3'] = xopt[3]
    angles['right_j4'] = xopt[4]
    angles['right_j5'] = xopt[5]
    angles['right_j6'] = xopt[6]

    limb.move_to_joint_positions(angles, timeout=timeout, threshold=threshold)

def move_sawyer_set_joints(xopt, limb):
    angles = limb.joint_angles()
    angles['right_j0'] = xopt[0]
    angles['right_j1'] = xopt[1]
    angles['right_j2'] = xopt[2]
    angles['right_j3'] = xopt[3]
    angles['right_j4'] = xopt[4]
    angles['right_j5'] = xopt[5]
    angles['right_j6'] = xopt[6]

    limb.set_joint_positions(angles)

def move_sawyer_set_velocities(xopt, limb, velocity_scalar=1.0):
    joint_names = limb.joint_names()
    curr_angles = limb.joint_angles()
    curr_joints = []
    for name in joint_names:
        curr_joints.append(curr_angles[name])

    vels = []
    for i,x in enumerate(xopt):
        vels.append(velocity_scalar * (x - curr_joints[i]))

    for i,name in enumerate(joint_names):
        curr_angles[name] = vels[i]

    limb.set_joint_velocities(curr_angles)

def move_sawyer_raw_velocities(vels, limb):
    joint_names = limb.joint_names()
    curr_angles = limb.joint_angles()

    for i,name in enumerate(joint_names):
       curr_angles[name] = vels[i]

    limb.set_joint_velocities(curr_angles)

def pubGripper_vrep(gripperFile, val):
    f = open(gripperFile, 'w')
    f.write(str(val))
    f.close()

def pubGripper_robotiq(val, gripper_pub):
    # gripper_pub = rospy.Publisher('/gripper/cmd', GripperCmd, queue_size=10)
    gripper_cmd = GripperCmd()
    posValue = 0.085
    if val == 1:
        posValue = 0.0

    gripper_cmd.position = posValue
    gripper_cmd.speed = 0.5
    gripper_cmd.force = 100.0
    gripper_pub.publish(gripper_cmd)

'''
def pubGripper_ur5(val):
    gripper_pub = rospy.Publisher('/gripper/cmd', GripperCmd, queue_size=10)
    gripper_cmd = GripperCmd()
    posValue = 0.085
    if val == 1:
        posValue = 0.0

    gripper_cmd.position = posValue
    gripper_cmd.speed = 0.5
    gripper_cmd.force = 100.0
    gripper_pub.publish(gripper_cmd)
'''

def pubGripper_sawyer(val, gripper):
    if val == 1:
        gripper.close()
    else:
        gripper.open()


def move_ur5_joint_trajectory(xopt, jt_pub_ur5, duration=0.1):
    '''
    jt_pub_ur5 =  = rospy.Publisher('/ur5/arm_controller/command', JointTrajectory)
    generally for use with gazebo
    :param xopt:
    :return:
    '''
    jt_ur5 = JointTrajectory()
    jt_ur5.joint_names = ['shoulder_pan_joint','shoulder_lift_joint','elbow_joint',
                      'wrist_1_joint','wrist_2_joint','wrist_3_joint']

    jtpt = JointTrajectoryPoint()
    jtpt.positions = [xopt[0], xopt[1], xopt[2], xopt[3], xopt[4], xopt[5]]
    jtpt.time_from_start = rospy.Duration.from_sec(duration)

    jt_ur5.points.append(jtpt)
    jt_pub_ur5.publish(jt_ur5)


def move_sawyer_joint_trajectory(xopt, jt_pub_sawyer, duration=0.1):
    '''
    jt_pub_sawyer = rospy.Publisher('/sawyer/arm_controller/command',JointTrajectory)
    generally for use with gazebo
    :param xopt:
    :return:
    '''
    jt_sawyer = JointTrajectory()
    jt_sawyer.joint_names = ['right_j0','right_j1','right_j2',
                      'right_j3','right_j4','right_j5', 'right_j6']

    jtpt = JointTrajectoryPoint()
    jtpt.positions = [xopt[0], xopt[1], xopt[2], xopt[3], xopt[4], xopt[5], xopt[6]]
    jtpt.time_from_start = rospy.Duration.from_sec(duration)

    jt_sawyer.points.append(jtpt)
    jt_pub_sawyer.publish(jt_sawyer)


def joint_state_publish(xopt, robot, pub, solver='relaxed_ik'):
    '''
    pub = rospy.Publisher('joint_states', JointState, queue_size=5)
    :param xopt:
    :param robot:
    :param solver:
    :return:
    '''

    js = JointState()
    if robot == 'ur5':
        js.name = ['shoulder_pan_joint', 'shoulder_lift_joint', 'elbow_joint', 'wrist_1_joint', 'wrist_2_joint', 'wrist_3_joint']
        js.position = tuple(xopt)
    elif robot == 'sawyer':
        js.name = ['right_j0', 'right_j1', 'right_j2', 'right_j3', 'right_j4', 'right_j5', 'right_j6']
        js.position = tuple(xopt)
    elif robot == 'mico':
        js.name = ['m1n6s200_joint_1', 'm1n6s200_joint_2', 'm1n6s200_joint_3', 'm1n6s200_joint_4', 'm1n6s200_joint_5', 'm1n6s200_joint_6', 'm1n6s200_joint_finger_1','m1n6s200_joint_finger_2']
        js.position = tuple(xopt) + (0,0)
    elif robot == 'jaco':
        js.name = ['jaco_joint_1', 'jaco_joint_2', 'jaco_joint_3', 'jaco_joint_4', 'jaco_joint_5', 'jaco_joint_6', 'jaco_joint_finger_1', 'jaco_joint_finger_2', 'jaco_joint_finger_3', 'jaco_joint_finger_tip_1', 'jaco_joint_finger_tip_2', 'jaco_joint_finger_tip_3']
        js.position = tuple(xopt) + (0,0,0,0,0,0)
    elif robot == 'reactor':
        js.name = ['shoulder_yaw_joint', 'shoulder_pitch_joint', 'elbow_pitch_joint', 'wrist_pitch_joint', 'wrist_roll_joint', 'gripper_revolute_joint', 'gripper_right_joint', 'gripper_left_joint']
        x = deepcopy(xopt)
        js.position = (x[0],x[1],-x[2],-x[3],x[4]) + (0,0,0)
    elif robot == 'iiwa':
        js.name = ['iiwa_joint_1', 'iiwa_joint_2', 'iiwa_joint_3', 'iiwa_joint_4', 'iiwa_joint_5', 'iiwa_joint_6', 'iiwa_joint_7']
        js.position = tuple(xopt)
    elif robot == 'fanuc':
        js.name = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'joint_5']
        x = deepcopy(xopt)
        js.position = (x[0],x[1],-x[2],-x[3],-x[4])
    elif robot == 'hubo_right_arm':
        js.name = ['LEFT_SHOULDER_PITCH', 'LEFT_SHOULDER_ROLL', 'LEFT_SHOULDER_YAW', 'LEFT_ELBOW', 'LEFT_WRIST_YAW', # five per row
                   'LEFT_WRIST_PITCH', 'LWFT', 'LEFT_WRIST_YAW_2', 'LHAND_a1', 'LHAND_a2',
                   'LHAND_a3', 'LHAND_b1', 'LHAND_b2', 'LHAND_b3', 'LHAND_c1',
                   'LHAND_c2', 'LHAND_c3', 'RIGHT_SHOULDER_PITCH', 'RIGHT_SHOULDER_ROLL', 'RIGHT_SHOULDER_YAW',
                   'RIGHT_ELBOW', 'RIGHT_WRIST_YAW', 'RIGHT_WRIST_PITCH', 'RWFT', 'RIGHT_WRIST_YAW_2',
                   'RHAND_a1', 'RHAND_a2', 'RHAND_a3', 'RHAND_b1', 'RHAND_b2',
                   'RHAND_b3', 'RHAND_c1', 'RHAND_c2', 'RHAND_c3', 'WAIST',
                   'LEFT_HIP_YAW', 'LEFT_HIP_ROLL', 'LEFT_HIP_PITCH', 'LEFT_KNEE', 'LEFT_ANKLE_PITCH',
                   'LEFT_ANKLE_ROLL', 'LAFT', 'LFUNI', 'LFWH', 'LEFT_WHEEL',
                   'RIGHT_HIP_YAW', 'RIGHT_HIP_ROLL', 'RIGHT_HIP_PITCH', 'RIGHT_KNEE', 'RIGHT_ANKLE_PITCH',
                   'RIGHT_ANKLE_ROLL', 'RAFT', 'RFUNI','RFWH', 'RIGHT_WHEEL',
                   'NECK_PITCH_1', 'NECK_YAW', 'NECK_PITCH_2', 'NECK_ROLL']

        x = deepcopy(xopt)
        js.position = 59*[0]
        js.position[17] = x[0]
        js.position[18] = x[1]
        js.position[19] = x[2]
        js.position[20] = x[3]
        js.position[21] = x[4]
        js.position[22] = x[5]
        js.position[24] = x[6]
        js.position = tuple(js.position)
        # print js.position
    elif robot == 'hubo_upper_body':
        js.name = ['LEFT_SHOULDER_PITCH', 'LEFT_SHOULDER_ROLL', 'LEFT_SHOULDER_YAW', 'LEFT_ELBOW', 'LEFT_WRIST_YAW', # five per row
                   'LEFT_WRIST_PITCH', 'LWFT', 'LEFT_WRIST_YAW_2', 'LHAND_a1', 'LHAND_a2',
                   'LHAND_a3', 'LHAND_b1', 'LHAND_b2', 'LHAND_b3', 'LHAND_c1',
                   'LHAND_c2', 'LHAND_c3', 'RIGHT_SHOULDER_PITCH', 'RIGHT_SHOULDER_ROLL', 'RIGHT_SHOULDER_YAW',
                   'RIGHT_ELBOW', 'RIGHT_WRIST_YAW', 'RIGHT_WRIST_PITCH', 'RWFT', 'RIGHT_WRIST_YAW_2',
                   'RHAND_a1', 'RHAND_a2', 'RHAND_a3', 'RHAND_b1', 'RHAND_b2',
                   'RHAND_b3', 'RHAND_c1', 'RHAND_c2', 'RHAND_c3', 'WAIST',
                   'LEFT_HIP_YAW', 'LEFT_HIP_ROLL', 'LEFT_HIP_PITCH', 'LEFT_KNEE', 'LEFT_ANKLE_PITCH',
                   'LEFT_ANKLE_ROLL', 'LAFT', 'LFUNI', 'LFWH', 'LEFT_WHEEL',
                   'RIGHT_HIP_YAW', 'RIGHT_HIP_ROLL', 'RIGHT_HIP_PITCH', 'RIGHT_KNEE', 'RIGHT_ANKLE_PITCH',
                   'RIGHT_ANKLE_ROLL', 'RAFT', 'RFUNI','RFWH', 'RIGHT_WHEEL',
                   'NECK_PITCH_1', 'NECK_YAW', 'NECK_PITCH_2', 'NECK_ROLL']

        x = deepcopy(xopt)
        js.position = 59*[0]
        if solver == 'relaxed_ik':
            js.position[34] = -x[0]
        elif solver == 'trac_ik':
            js.position[34] = x[0]
        js.position[17] = x[1]
        js.position[18] = x[2]
        js.position[19] = x[3]
        js.position[20] = x[4]
        js.position[21] = x[5]
        js.position[22] = x[6]
        js.position[24] = x[7]
        js.position = tuple(js.position)

    js.velocity = ()
    now = rospy.Time.now()
    js.header.stamp.secs = now.secs
    js.header.stamp.nsecs = now.nsecs

    pub.publish(js)

