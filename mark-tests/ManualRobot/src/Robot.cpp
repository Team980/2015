#include "WPILib.h"
#include "Constants.h"
#include "LiftSystem.h"
#include "DriveSystem.cpp"

//char myString[64]; //for debugging

//local function
bool myOnTarget(PIDController *controller, PIDSource *source)
{
	float error = controller->GetSetpoint() - source->PIDGet();
	float tolerance = error * POS_ERR_TOL; //any percentage value
	return (fabs(tolerance) < POS_TOL_COMP); // a tuned value
}

float velocityProfileX(float x)
{
	if(x > -0.5 && x < 0.5)
		return 0.5*x;
	else
		return (1.5*(x-0.5) + 0.25);
}

float velocityProfileY(float y)
{
	//for now use the same profile as x
	return velocityProfileX(y);
}

class Robot: public IterativeRobot
{
private:
	//memory management
	bool enteredTelopInit; //drive system is created in teleop init so only delete the drive system in the destructor if entered teleop init

	//drive system
#if BUILD_VER == COMPETITION
	Talon * leftDrive;
	Talon * rightDrive;
#else
	Victor * leftDrive;
	Victor * rightDrive;
#endif
	Encoder *leftEncoder;
	Encoder *rightEncoder;
	DriveSystem *driveSystem;

	//driver controls
	Joystick *driveJoystick;
	Joystick *steeringWheel;
	float driveX, driveY; //values from driver controls: x from steeringWheel, y from driveJoystick

	//lift system
#if BUILD_VER == COMPETITION
	CANTalon *forkMotor;
	CANTalon *liftMotorBack;
	CANTalon *liftMotorFront;
	CANTalon *leftIntakeMotor;
	CANTalon *rightIntakeMotor;
#else
	CANJaguar *forkMotor;
	CANJaguar *liftMotorBack;
	CANJaguar *liftMotorFront;
	CANJaguar *leftIntakeMotor;
	CANJaguar *rightIntakeMotor;
#endif
	DigitalInput *forkLimitInner;
	DigitalInput *forkLimitOuter;
	DigitalInput *liftLimitLow; //at the bottom of the lift
	DigitalInput *liftLimitHigh; //at the top of the lift
	Joystick *liftSysJoystick;
	LiftSystem *liftSystem;

	// Linear PID controllers
	PIDController *controlPosRight;
	PIDController *controlPosLeft;
	enum {start, drive, hold} autonomousDriveState;

	void RobotInit()
	{
		//not used
	}

	void AutonomousInit()
	{
		controlPosLeft->Enable();
		controlPosLeft->SetSetpoint(AUTONMOUS_MOVE_DIST);
		controlPosRight->Enable();
		controlPosRight->SetSetpoint(-AUTONMOUS_MOVE_DIST);
		autonomousDriveState = drive;
	}

	void AutonomousPeriodic()
	{
		char myString[64];

		if (autonomousDriveState == drive)
		{
    		sprintf(myString, "Driving\n");
    		SmartDashboard::PutString("DB/String 0", myString);
    		sprintf(myString, "L Setpoint: %f\n", controlPosLeft->GetSetpoint());
    		SmartDashboard::PutString("DB/String 6", myString);
       		sprintf(myString, "L PID: %f\n", leftEncoder->PIDGet());
    		SmartDashboard::PutString("DB/String 7", myString);
    		sprintf(myString, "R Setpoint: %f\n", controlPosRight->GetSetpoint());
    		SmartDashboard::PutString("DB/String 8", myString);
       		sprintf(myString, "R PID: %f\n", rightEncoder->PIDGet());
    		SmartDashboard::PutString("DB/String 9", myString);

			if ((myOnTarget(controlPosLeft, leftEncoder)) && (myOnTarget(controlPosRight, rightEncoder)))
			{
				controlPosLeft->Disable();  // disable the drive PID controllers
				controlPosRight->Disable();
				autonomousDriveState = hold;
			}
		}
	}

	void TeleopInit()
	{
		enteredTelopInit = true;
		driveSystem = new DriveSystem(leftEncoder, rightEncoder, leftDrive, rightDrive);

		//Initialize PID
		driveSystem->SetPIDDrive(PID_CONFIG);
		//Set Wheel Diameter
		driveSystem->SetWheelDiameter(WHEEL_DIAMETER);
		// start recording the distance traveled
		driveSystem->StartRecordingDistance();
	}

	void TeleopPeriodic()
	{
		//get driver controls values
		driveX = -steeringWheel->GetX();
		driveY = driveJoystick->GetY();

		//Filter deadband
		if (driveX > DRIVE_DB_LOW && driveX < DRIVE_DB_HIGH)
			driveX = ZERO_FL;
		if (driveY > DRIVE_DB_LOW && driveY < DRIVE_DB_HIGH)
			driveY = ZERO_FL;

		//map to velocity profile
		driveX = velocityProfileX(driveX);
		driveY = velocityProfileY(driveY);

		//Give drive instructions
		driveSystem->SetDriveInstruction(driveY * MAX_RPS, driveX * MAX_RPS);
		driveSystem->Update();

		//Give lift instructions
		liftSystem->Update();
	}

	void TestPeriodic()
	{
		//not used
	}

public:
	Robot()
	{
		//memory management
		enteredTelopInit = false;

		//drive system
#if BUILD_VER == COMPETITION
		leftDrive = new Talon(CHAN_LEFT_DRIVE);
		rightDrive = new Talon(CHAN_RIGHT_DRIVE);
#else
		leftDrive = new Victor(CHAN_LEFT_DRIVE);
		rightDrive = new Victor(CHAN_RIGHT_DRIVE);
#endif
		leftEncoder = new Encoder(CHAN_ENCODER_LEFT_A, CHAN_ENCODER_LEFT_B, false, Encoder::EncodingType::k4X);
		leftEncoder->SetDistancePerPulse(ENCODER_DIST_PER_PULSE);
        leftEncoder->SetPIDSourceParameter(leftEncoder->kDistance);
		rightEncoder = new Encoder(CHAN_ENCODER_RIGHT_A, CHAN_ENCODER_RIGHT_B, false, Encoder::EncodingType::k4X);
		rightEncoder->SetDistancePerPulse(ENCODER_DIST_PER_PULSE);
        rightEncoder->SetPIDSourceParameter(rightEncoder->kDistance);

		//driver controls
		driveJoystick = new Joystick(CHAN_DRIVE_JS);
		steeringWheel = new Joystick(CHAN_STEERING_WHEEL);

		//lift system
#if BUILD_VER == COMPETITION
		forkMotor = new CANTalon(CHAN_FORK_MOTOR);
		liftMotorBack = new CANTalon(CHAN_LIFT_MOTOR_BACK);
		liftMotorFront = new CANTalon(CHAN_LIFT_MOTOR_FRONT);
		leftIntakeMotor = new CANTalon(CHAN_L_INTAKE_MOTOR);
		rightIntakeMotor = new CANTalon(CHAN_R_INTAKE_MOTOR);
#else
		forkMotor = new CANJaguar(CHAN_FORK_MOTOR);
		liftMotorBack = new CANJaguar(CHAN_LIFT_MOTOR_BACK);
		liftMotorFront = new CANJaguar(CHAN_LIFT_MOTOR_FRONT);
		leftIntakeMotor = new CANJaguar(CHAN_L_INTAKE_MOTOR);
		rightIntakeMotor = new CANJaguar(CHAN_R_INTAKE_MOTOR);
#endif
		forkLimitInner = new DigitalInput(CHAN_FORK_MIN_LS);
		forkLimitOuter = new DigitalInput(CHAN_FORK_MAX_LS);
		liftLimitLow = new DigitalInput(CHAN_LIFT_LOW_LS);
		liftLimitHigh = new DigitalInput(CHAN_LIFT_HIGH_LS);
		liftSysJoystick = new Joystick(CHAN_LIFT_SYS_JS);
		liftSystem = new LiftSystem(forkMotor, liftMotorBack, liftMotorFront, leftIntakeMotor, rightIntakeMotor,
				forkLimitInner, forkLimitOuter, liftLimitLow, liftLimitHigh,
				liftSysJoystick);

		// Linear PID controllers
        controlPosLeft = new PIDController(POS_PROPORTIONAL_TERM, POS_INTEGRAL_TERM, POS_DIFFERENTIAL_TERM, leftEncoder, leftDrive);
        controlPosLeft->SetContinuous(true);
        controlPosLeft->SetOutputRange(AUTONOMOUS_MAX_REVERSE_SPEED, AUTONOMOUS_MAX_FORWARD_SPEED);
        controlPosRight = new PIDController(POS_PROPORTIONAL_TERM, POS_INTEGRAL_TERM, POS_DIFFERENTIAL_TERM, rightEncoder, rightDrive);
        controlPosRight->SetContinuous(true);
        controlPosRight->SetOutputRange(AUTONOMOUS_MAX_REVERSE_SPEED, AUTONOMOUS_MAX_FORWARD_SPEED);
        autonomousDriveState = start;
	}

	~Robot()
	{
		//drive system
		delete leftDrive;
		delete rightDrive;
		delete leftEncoder;
		delete rightEncoder;
		if(enteredTelopInit)
			delete driveSystem;

		//driver controls
		delete driveJoystick;
		delete steeringWheel;

		//lift system
		delete forkMotor;
		delete liftMotorBack;
		delete liftMotorFront;
		delete leftIntakeMotor;
		delete rightIntakeMotor;
		delete forkLimitInner;
		delete forkLimitOuter;
		delete liftLimitLow;
		delete liftLimitHigh;
		delete liftSysJoystick;
		delete liftSystem;

		// Linear PID controllers
		delete controlPosLeft;
		delete controlPosRight;
	}
};

START_ROBOT_CLASS(Robot);
