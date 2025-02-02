import pybullet
import math
import time
import abc
import numpy as np
import os


class LeggedSimulator(metaclass=abc.ABCMeta):
    def __init__(self, path_to_urdf, time_step, start_time, end_time):
        self.path_to_urdf = path_to_urdf
        self.time_step = time_step
        self.start_time = start_time
        self.end_time = end_time
        self.calib_camera = False
        self.camera_distance = 0.0
        self.camera_yaw = 0.0
        self.camera_pitch = 0.0
        self.camera_target_pos = [0., 0., 0.]
        self.print_items = []
        self.box_pos = []

    def set_sim_settings(self, time_step, start_time, end_time):
        self.time_step  = time_step
        self.start_time = start_time
        self.end_time   = end_time

    def set_urdf(self, path_to_urdf):
        self.path_to_urdf = path_to_urdf

    def set_camera(self, camera_distance, camera_yaw, camera_pitch, camera_target_pos):
        self.calib_camera = True
        self.camera_distance = camera_distance
        self.camera_yaw = camera_yaw
        self.camera_pitch = camera_pitch
        self.camera_target_pos = camera_target_pos

    def get_body_local_velocity(pybullet_robot):
        basePos, baseOrn = pybullet.getBasePositionAndOrientation(pybullet_robot)
        R = np.reshape(pybullet.getMatrixFromQuaternion(baseOrn), [3, 3]) 
        baseVel, baseAngVel = pybullet.getBaseVelocity(pybullet_robot)
        baseVel = R.T @ np.array(baseVel)
        baseAngVel = R.T @ np.array(baseAngVel)
        return baseVel, baseAngVel

    def add_box(self, box_pos):
        self.box_pos.append(box_pos)

    @abc.abstractmethod
    def get_state_from_pybullet(self, pybullet_robot, q, v):
        return NotImplementedError()

    @abc.abstractmethod
    def apply_control_input_to_pybullet(self, pybullet_robot, u):
        return NotImplementedError()

    @abc.abstractmethod
    def init_pybullet_robot(self, pybullet_robot, q):
        return NotImplementedError()

    def print_joint_info(self):
        pybullet.connect(pybullet.DIRECT)
        robot = pybullet.loadURDF(self.path_to_urdf, 
                                  useFixedBase=False, 
                                  useMaximalCoordinates=False)
        nJoints = pybullet.getNumJoints(robot)
        for j in range(nJoints):
            info = pybullet.getJointInfo(robot, j)
            print(info)
        pybullet.disconnect()

    def add_print_item(self, item):
        self.print_items.append(item)

    def run_simulation(self, mpc, q0, v0, feedback_delay=False, 
                       terrain=False, verbose=False, 
                       record=False, record_name='mpc_sim.mp4'):
        pybullet.connect(pybullet.GUI)
        pybullet.setGravity(0, 0, -9.81)
        pybullet.setTimeStep(self.time_step)
        if terrain:
            plane_urdf = os.path.join(os.path.dirname(__file__), "rsc/terrain.urdf")
            plane = pybullet.loadURDF(fileName=plane_urdf)
        else:
            import pybullet_data
            pybullet.setAdditionalSearchPath(pybullet_data.getDataPath())
            plane = pybullet.loadURDF("plane.urdf")
        if self.box_pos:
            box_urdf = os.path.join(os.path.dirname(__file__), "rsc/box.urdf")
            for pos in self.box_pos:
                box = pybullet.loadURDF(fileName=box_urdf, basePosition=pos, useFixedBase=True)
        robot = pybullet.loadURDF(self.path_to_urdf,  
                                  useFixedBase=False, 
                                  useMaximalCoordinates=False)

        self.init_pybullet_robot(robot, q0)
        t = self.start_time
        q = q0
        v = v0
        u = mpc.get_initial_control_input().copy()
        sim_time = self.end_time - self.start_time
        sim_steps = math.floor(sim_time/self.time_step)

        if self.calib_camera:
            pybullet.resetDebugVisualizerCamera(self.camera_distance,
                                                self.camera_yaw,
                                                self.camera_pitch,
                                                self.camera_target_pos)
        pybullet.configureDebugVisualizer(pybullet.COV_ENABLE_GUI, 0)

        if record:
            pybullet.startStateLogging(pybullet.STATE_LOGGING_VIDEO_MP4, 
                                       record_name)

        for i in range(sim_steps):
            self.get_state_from_pybullet(robot, q, v)
            if verbose:
                print('t = {:.6g}:'.format(t))
            if feedback_delay:
                u = mpc.get_initial_control_input().copy()
            mpc.update_solution(t, self.time_step, q, v)
            if verbose:
                print('KKT error = {:.6g}'.format(mpc.KKT_error(t, q, v)))
                print('')
                if self.print_items:
                    for e in self.print_items:
                        print(e)
            if not feedback_delay:
                u = mpc.get_initial_control_input().copy()
            self.apply_control_input_to_pybullet(robot, u)
            pybullet.stepSimulation()
            time.sleep(self.time_step)
            t = t + self.time_step
        pybullet.disconnect()