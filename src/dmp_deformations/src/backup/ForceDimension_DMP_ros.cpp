#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include <chrono>

 #include <ros/ros.h>
 #include "std_msgs/String.h"

// Panda
#include "PandaController.h"
#include <franka/robot_state.h>

// Force Dimension
#include "dhdc.h"
#include "drdc.h"

// Socket Libraries for FT sensor readings
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <eigen3/Eigen/Geometry>
#include <thread> 


using namespace std;
using namespace std::chrono;

array<double,3> workspace_center = {0.42, 0.1, 0.25};
array<double, 3> force_dimension = {0.0, 0.0, 0.0};

std::ofstream outputfile;

void signalHandler(int sig)
{
    std::cout << "Interrupt " << sig << " recieved in ForceDimension.cpp\n";
    //cout << "Closing Force Dimension" << endl;
    //dhdClose();
    dhdEnableForce (DHD_OFF);
    cout << "Closing Panda" << endl;
    PandaController::stopControl();
    exit(sig);
}

bool init_forcedimension() {
    cout <<"Setting up Force Dimension\n";
    if (dhdOpen () < 0) {
        cout << "Unable to open device" << endl;
        dhdSleep (2.0);
        return false;
    }
    cout << "Force Dimension Setup Complete" << endl;
}


void fd_neutral_position(double *x_curr, double *y_curr, double *z_curr){
    
    double x_d = 0.0;
    double y_d = 0.0;
    double z_d = 0.0;

    double v_x = 0.0;
    double v_y = 0.0;
    double v_z = 0.0;

    double stiffness = 200;
    double damping = 2*sqrt(stiffness);

    // Get current position and velocity
    dhdGetPosition (x_curr, y_curr, z_curr);
    dhdGetLinearVelocity(&v_x,&v_y,&v_z);
    
    // Command to that position with P controller
    dhdSetForceAndTorque(-stiffness*(*x_curr-x_d)-damping*v_x,-stiffness*(*y_curr-y_d)-damping*v_y,
    -stiffness*(*z_curr-z_d)-damping*v_z,0.0,0.0,0.0);

}


void replay_demo(){
    std::ifstream dmpfile("learneddmp.csv");

    double starting_x;
    double starting_y;
    double starting_f;

    double attractor_x;
    double attractor_y;
    double attractor_f;
    
    double x;
    double y;
    double z = 0.03;
    double z_high = 0.15;

    double f;

    string temp;
    if(dmpfile.good())
    {
        double k=50;
        double b=sqrt(2*k);

        double ddx, ddy, ddf;

        double dx = 0.0;
        double dy = 0.0;
        double df = 0.0; 

        double f_x, f_y, f_f;

        double fd_x, fd_y, fd_z;

        std::array<double, 6> pose;
        std::array<double, 6> ft;
        std::array<double, 3> selection;

        // Read first mode command
        string throwaway;
        getline(dmpfile,throwaway,',');
        getline(dmpfile,throwaway,',');
        getline(dmpfile,temp);

        // Read Selection Vector
        getline(dmpfile,temp,',');
        selection[0] = atof(temp.c_str());
        getline(dmpfile,temp,',');
        selection[1] = atof(temp.c_str());
        getline(dmpfile,temp);
        selection[2] = atof(temp.c_str());
        PandaController::writeSelectionVector(selection);

        // Read Starting Points
        getline(dmpfile,temp,',');
        starting_x = atof(temp.c_str());
        getline(dmpfile,temp,',');
        starting_y = atof(temp.c_str());
        getline(dmpfile,temp);
        starting_f = atof(temp.c_str());

        // Read Attractor Points
        getline(dmpfile,temp,',');
        attractor_x = atof(temp.c_str());
        getline(dmpfile,temp,',');
        attractor_y = atof(temp.c_str());
        getline(dmpfile,temp);
        attractor_f = atof(temp.c_str());

        // Reset
        x = starting_x;
        y = starting_y;
        f = starting_f;

        dx = 0.0;
        dy = 0.0;
        df = 0.0; 

        cout << "STARTING: " << starting_x << "," << starting_y << "," << starting_f << endl;

        std::array<double, 7> command;
        // Time to arrive in milliseconds
        std::array<double, 7> commandedPath[1]; 
        command[0] = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1) + 2000;
        command[1] = starting_x;
        command[2] = starting_y;
        command[3] = starting_f;
        command[4] = 0;
        command[5] = 0;
        command[6] = 0;
        commandedPath[0] = command;

        // GO ABOVE STARTING POINT
        PandaController::writeCommandedPath(commandedPath,1);

        dhdSleep(2.1);

        for(int i=0; i<1000;i++)
        {
            fd_neutral_position(&fd_x,&fd_y,&fd_z);
            dhdSleep(0.001);
        }

        cout << "REPLAYING MOTION" << endl;

        while(getline(dmpfile,temp,',')){
            if(temp=="mode")
            {
                getline(dmpfile,temp,',');
                f_y = atof(temp.c_str());
                getline(dmpfile,temp);
                f_f = atof(temp.c_str());

                // Read Selection Vector
                getline(dmpfile,temp,',');
                selection[0] = atof(temp.c_str());
                getline(dmpfile,temp,',');
                selection[1] = atof(temp.c_str());
                getline(dmpfile,temp);
                selection[2] = atof(temp.c_str());

                // Read Starting Points
                getline(dmpfile,temp,',');
                starting_x = atof(temp.c_str());
                getline(dmpfile,temp,',');
                starting_y = atof(temp.c_str());
                getline(dmpfile,temp);
                starting_f = atof(temp.c_str());

                // Read Attractor Points
                getline(dmpfile,temp,',');
                attractor_x = atof(temp.c_str());
                getline(dmpfile,temp,',');
                attractor_y = atof(temp.c_str());
                getline(dmpfile,temp);
                attractor_f = atof(temp.c_str());

                // Reset
                x = starting_x;
                y = starting_y;
                f = starting_f;

                dx = 0.0;
                dy = 0.0;
                df = 0.0; 

                PandaController::writeSelectionVector(selection);

                if(selection[0]==0 || selection [1]==0 || selection[2]==0){
                    // Do force onloading with the first sample
                    cout << "FORCE ONLOADING STARTED" << endl;

                    ft = {starting_x, starting_y, starting_f, 0.0, 0.0, 0.0};
                    pose = {starting_x, starting_y, starting_f, 0.0, 0.0, 0.0};
                    PandaController::writeCommandedPosition(pose);
                    PandaController::writeCommandedFT(ft);

                    bool proper_contact = false;
                    while(!proper_contact)
                    {
                        fd_neutral_position(&fd_x,&fd_y,&fd_z);
                        std::array<double, 6> FTData = PandaController::readFTForces();
                        cout << "FZ: " << FTData[2] << " " << starting_f << endl;
                        if(FTData[2]<0.98*starting_f && FTData[2]>1.02*starting_f)
                        {
                            proper_contact = true;
                        }
                        dhdSleep(0.001);

                    }
                    cout << "FORCE ONLOADING COMPLETE" << endl;

                }



            }
            
            else{
                f_x = atof(temp.c_str());
                getline(dmpfile,temp,',');
                f_y = atof(temp.c_str());
                getline(dmpfile,temp);
                f_f = atof(temp.c_str());
                fd_neutral_position(&fd_x,&fd_y,&fd_z);

                // Force mode gain, Position mode gain
                std::array<double, 2> sel_gains = {6000, 75};

                // Calculate New X
                ddx = k*(attractor_x-x)-b*dx+f_x-sel_gains[(int) selection[0]]*fd_x;
                dx = dx + ddx*0.001;
                x = x+dx*0.001;

                // Calculate New Y
                ddy = k*(attractor_y-y)-b*dy+f_y-sel_gains[(int) selection[1]]*fd_y;
                dy = dy + ddy*0.001;
                y = y+dy*0.001;

                // Calculate New F
                ddf = k*(attractor_f-f)-b*df+f_f+sel_gains[(int) selection[2]]*fd_z;
                df = df + ddf*0.001;
                f = f+df*0.001;

                //cout << "X:" << fd_x << " Y:" << fd_y << " F:" << fd_z << endl;

                ft = {x, y, f, 0.0, 0.0, 0.0};
                pose = {x, y, f, 0.0, 0.0, 0.0};
                PandaController::writeCommandedPosition(pose);
                PandaController::writeCommandedFT(ft);
                dhdSleep(0.001);
            }
        }

        // Offload FD Forces
        dhdSetForceAndTorque(0.0,0.0,0.0,0.0,0.0,0.0);

    }


}

void poll_forcedimension(bool buttonPressed, bool resetCenter, double velcenterx, double velcentery,double velcenterz) {

    // Scaling Values
    array<double,3> scaling_factors = {-4.0, -4.0, 4.0};

    array<double, 6> panda_pos = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    
    double vx=0.0;
    double vy=0.0;
    double vz=0.0;
    double scaling = 0.001;

    if (buttonPressed){
        array<double, 3> force_dimension_temp = {0.0, 0.0, 0.0};
        dhdGetPosition (&force_dimension_temp[0], &force_dimension_temp[1], &force_dimension_temp[2]);
        workspace_center[0]=workspace_center[0]-scaling*(force_dimension_temp[0]-velcenterx);
        workspace_center[1]=workspace_center[1]-scaling*(force_dimension_temp[1]-velcentery);
        workspace_center[2]=workspace_center[2]+scaling*(force_dimension_temp[2]-velcenterz);

    }

    else{
        dhdGetPosition (&force_dimension[0], &force_dimension[1], &force_dimension[2]);
        if(resetCenter){
            cout << "Reset Center" << endl;
            workspace_center[0]=workspace_center[0]-scaling_factors[0]*(force_dimension[0]-velcenterx);
            workspace_center[1]=workspace_center[1]-scaling_factors[1]*(force_dimension[1]-velcentery);
            workspace_center[2]=workspace_center[2]-scaling_factors[2]*(force_dimension[2]-velcenterz);
        }
    }

    panda_pos[0] = scaling_factors[0] * force_dimension[0] + workspace_center[0];
    panda_pos[1] = scaling_factors[1] * force_dimension[1] + workspace_center[1];
    panda_pos[2] = scaling_factors[2] * force_dimension[2] + workspace_center[2];

    //cout << "FD: " << panda_pos[0]  << "," << panda_pos[1] << "," << panda_pos[2] << endl;
    //cout << "WC: " << workspace_center[0]  << "," << workspace_center[1] << "," << workspace_center[2] << endl;
    PandaController::writeCommandedPosition(panda_pos);

}

void feedback_forcedimension(double *x, double *y, double *z, double *fx, double *fy, double *fz, bool buttonPressed){
    franka::RobotState state = PandaController::readRobotState();
    double scale = -1.0;
    //cout << "Forces: " << state.O_F_ext_hat_K[0]  << "," << state.O_F_ext_hat_K[1] << "," << state.O_F_ext_hat_K[2] << endl;
    double vx;
    double vy;
    double vz;
    double b = 30;
    dhdGetLinearVelocity(&vx,&vy,&vz);
    if (!buttonPressed){
        dhdSetForceAndTorque(-state.O_F_ext_hat_K[0] * scale-vx*b,-state.O_F_ext_hat_K[1] * scale-vy*b,state.O_F_ext_hat_K[2] * scale-vz*b,0.0,0.0,0.0);
    }
    *x = state.O_T_EE[12];
    *y = state.O_T_EE[13];
    *z = state.O_T_EE[14];
    *fx = -state.O_F_ext_hat_K[0];
    *fy = -state.O_F_ext_hat_K[1];
    *fz = -state.O_F_ext_hat_K[2];
}

void log_demonstration(double x, double y, double z, double fx, double fy, double fz){
    outputfile << x << "," << y << "," << z << "," << -fx << "," << -fy << "," << -fz << "\n";
}

void feedback_ft_forcedimension(bool buttonPressed, double velcenterx, double velcentery, double velcenterz,
double *x, double *y, double *z, double *fx, double *fy, double *fz){

    double vx;
    double vy;
    double vz;
    double b = 50; // injected damping
    double scale = -0.4; //force scaling
    double vel_mode_stiffness = 5000;

    double fd_x;
    double fd_y;
    double fd_z;

    dhdGetLinearVelocity(&vx,&vy,&vz);
    dhdGetPosition (&fd_x, &fd_y, &fd_z);
    std::array<double, 6> FTData = PandaController::readFTForces();
    
    Eigen::VectorXd v = Eigen::Map<Eigen::VectorXd>(FTData.data(),3);

    Eigen::Matrix3d m;
    m = Eigen::AngleAxisd(-M_PI/6, Eigen::Vector3d::UnitZ());
    v = m * v;

    FTData[0] = v[0];
    FTData[1] = v[1];
    FTData[2] = v[2];


    if(!buttonPressed){ 
        dhdSetForceAndTorque(FTData[0] * scale -vx*b,-FTData[1]* scale -vy*b,FTData[2]* scale -vz*b,0.0,0.0,0.0);
    }

    else{
        dhdSetForceAndTorque(-vel_mode_stiffness*(fd_x-velcenterx)*abs(fd_x-velcenterx),-vel_mode_stiffness*(fd_y-velcentery)*abs(fd_y-velcentery),
        -vel_mode_stiffness*(fd_z-velcenterz)*abs(fd_z-velcenterz),0.0,0.0,0.0);
    }
    
    franka::RobotState state = PandaController::readRobotState();
    *x = state.O_T_EE[12];
    *y = state.O_T_EE[13];
    *z = state.O_T_EE[14];
    *fx = -FTData[0];
    *fy = -FTData[1];
    *fz = -FTData[2];

    // TODO: Fix this to be the forces from the actual force torque sensor

    //cout << "FT: " << -FTData[0] * scale -vx*b  << "," << -FTData[1]* scale << "," << FTData[2]* scale -vz*b << endl;

}

int main(int argc, char **argv) {
    //Setup the signal handler for exiting
    signal(SIGINT, signalHandler);

    ros::init(argc, argv, "ForceDimensionDMP");
    ros::NodeHandle n("~");  
    ros::Publisher file_pub = 
        n.advertise<std_msgs::String>("/dmp/filepub", 5);
    ros::Publisher reset_pub = 
        n.advertise<std_msgs::String>("/dmp/reset", 5);

    //setup_ft();

    if (!init_forcedimension()) {
        cout << endl << "Failed to init force dimension" << endl;
        return -1;
    }


    // Turn on gravity compensation
    dhdEnableForce (DHD_ON);

    // Second button press to start control & forces
    bool buttonReleased=false;
    while(!buttonReleased)
        if(dhdGetButton(0)==0) // button released
        {
            buttonReleased = true;
        }

    cout << "Press Button again to enable Forces and Panda Control" << endl;

    bool buttonPressed=false;
    while(!buttonPressed)
        if(dhdGetButton(0)==1) // button pressed
        {
            buttonPressed = true;
        }
    
    buttonReleased=false;
    while(!buttonReleased)
        if(dhdGetButton(0)==0) // button released
        {
            buttonReleased = true;
        }


    // Start Panda controller and poll force dimension to make sure it has reasonable starting values
    pid_t pid = PandaController::initPandaController(PandaController::ControlMode::HybridControl);
    if (pid < 0) {
       cout << "Failed to start panda process" << endl;
    }
    poll_forcedimension(false,false,0.0,0.0,0.0);

    // Initialize Hybrid Controller
    std::array<double, 3> selectionVector = {1.0, 1.0, 1.0};
    std::array<double, 6> FT_command = {0.0, 0.0, -10.0, 0.0, 0.0, 0.0};
    PandaController::writeSelectionVector(selectionVector);
    PandaController::writeCommandedFT(FT_command);


    // State flow based on button presses
    bool start_recording = false;

    // Initialize data for storage
    double x = 0;
    double y = 0;
    double z = 0;
    double fx = 0;
    double fy = 0;
    double fz = 0;

    double velcenterx = 0;
    double velcentery = 0;
    double velcenterz = 0;
    
    buttonPressed = false;
    bool velocity_mode = false;
    auto start = high_resolution_clock::now(); 
    bool gripping = false;
    bool recording = false;
    
    bool replayMode = false;

    int file_iter = 0;

    bool resetCenter = true;

    while (PandaController::isRunning() && ros::ok() && !replayMode) {
        poll_forcedimension(velocity_mode,resetCenter, velcenterx,velcentery,velcenterz);

        if (resetCenter){ //only allow one correction
            resetCenter=false;
        }
        //feedback_forcedimension(&x,&y,&z,&fx,&fy,&fz,buttonPressed);
        feedback_ft_forcedimension(velocity_mode, velcenterx, velcentery, velcenterz,&x,&y,&z,&fx,&fy,&fz);


        // If button pressed and released in less than 0.3 seconds,
        // it is a gripping action

        // If held greater than 0.3 seconds, it is a velocity control action

        if(!buttonPressed && dhdGetButton(0)==1) // button initially pressed
        {
            buttonPressed = true;
            start = high_resolution_clock::now(); 
        }

        
        if(buttonPressed) // button held
        {
            auto end = high_resolution_clock::now(); 
            auto duration = duration_cast<microseconds>(end - start); 

            if(duration.count()>=300000)
            {
                if (velocity_mode==false)
                {
                    velocity_mode=true;
                    // When starting a potential velocity mode action
                    // Need to see where the current center value is
                    dhdGetPosition (&velcenterx, &velcentery, &velcenterz);
                    cout << "Velocity Mode" << endl;
                }
               
            }
            
        }

        if(buttonPressed && dhdGetButton(0)==0) // button released
        {
            auto end = high_resolution_clock::now(); 
            auto duration = duration_cast<microseconds>(end - start); 

            if(duration.count()<300000)
            {
                cout << "GRIPPER ACTION" << endl;
                if(!gripping){
                    PandaController:: graspObject();
                    gripping=true;
                }
                else{
                    PandaController::releaseObject();
                    gripping=false;
                }
            }

            cout << "Button Unpressed" << endl;
            buttonPressed=false;
        
            if (velocity_mode==true){
                // Deal with discontinuity
                double tempx;
                double tempy;
                double tempz;
                resetCenter=true;

                velocity_mode=false;
            }
        }

        if (dhdKbHit()) {
            
            char keypress = dhdKbGet();
            if (keypress == 'r'){
                if(recording==false){
                    string filename = {"panda_demo_"+to_string(file_iter)+".csv"};
                    remove( filename.c_str() );
                    outputfile.open (filename.c_str());
                    cout << "Starting Recording: " << filename.c_str() <<  endl;
                    file_iter++;
                    recording=true;
                }
                else{
                    outputfile.close();
                    cout << "Ending Recording" << endl;
                    recording=false;
                    // Let ROS know a file has been published to update the DMP
                    file_pub.publish("panda_demo_"+to_string(file_iter-1)+".csv");
                }
            
            }

            if (keypress == 'x'){
                reset_pub.publish('Reset');
                cout << "RESET DMP" << endl;
                }

            if (keypress == 'p'){
                cout << "Initiating Replay " << endl;
                replayMode = true;
                } 
        }

        if (recording){
            log_demonstration(x,y,z,fx,fy,fz);
        }
        
    ros::spinOnce();

    }

    if(replayMode)
    {
        // Replay loop if the correct key has been pressed!
        cout << "REPLAY MODE: Switching to DRD" << endl;
           // Fork process for the force torque sensor
        // pid_t pid_child = fork();
        // if (pid_child == 0) {
        //     fd_neutral_position();
        //     exit(0);
        // }
        thread(fd_neutral_position);


        while(PandaController::isRunning() && ros::ok() && replayMode){
            if (dhdKbHit()) {
                
                char keypress = dhdKbGet();
                if (keypress == 'r'){
                    replay_demo();
                }

                if (keypress == 'q'){
                    replayMode=false;
                }

            }
        }
        
    }


    dhdEnableForce (DHD_OFF);
    cout << "Closing Panda" << endl;
    PandaController::stopControl();
    dhdClose();
    
    return 0;
}
