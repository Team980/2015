//Adding a line to get Git to sync

#include "WPILib.h"
#include "Robot.h"
#include <math.h>

class Robot: public IterativeRobot
{
private:
	typedef enum{lowering,
		raising,
		holding} LiftState;

#if BUILD_VERSION == COMPETITION
	CANTalon *liftMotor;
	CANTalon *liftMotor2;
#else
	CANJaguar *liftMotor;
	CANJaguar *liftMotor2;
#endif
	Encoder *liftEncoder;
	PIDController *controlLift;
	PIDController *controlLift2;
	DigitalInput *liftLimitSwitchMin; //at the bottom of the lift
	DigitalInput *liftLimitSwitchMax; //at the top of the lift
	Joystick *joystick; //used to enter and exit the holding position state

	LiftState liftState;
	bool running;
	float  motorSpeed;
	bool enterHoldCommand; //command from the joystick to enter into holding position state
	bool exitHoldCommand; //command from the joystick to exit the holding position state
	bool liftEncZeroed; //zero is at the bottom of the lift
	bool liftEncFullRanged; //full range is at the top of the lift
	float maxLiftEncDist; //encoder distance at the top
	float pidPosSetPoint; //set point for the PID controller

	bool GetLiftLimitSwitchMin()
	{
		//invert so that TRUE when limit switch is closed and FALSE when limit switch is open
		return !(liftLimitSwitchMin->Get());
	}

	bool GetLiftLimitSwitchMax()
	{
		//invert so that TRUE when limit switch is closed and FALSE when limit switch is open
		return !(liftLimitSwitchMax->Get());
	}

	void SetLiftMotor(float val)
	{
		liftMotor->Set(val);
#if BUILD_VERSION == COMPETITION
		liftMotor2->Set(val);
#endif
	}

	float DistToSetpoint()
	{
		if(!(controlLift->IsEnabled()))
				return UNINIT_VAL;
		else
			return (liftEncoder->GetDistance() - pidPosSetPoint);
	}

	bool AtSetpoint()
	{
		if(!(controlLift->IsEnabled()))
			return false;
		else
			return (abs(DistToSetpoint()) < PID_POS_TOL*LIFT_ENCODER_DIST_PER_PULSE);
	}

	void AutonomousInit()
	{
		//not used
	}

	void AutonomousPeriodic()
	{
		//not used
	}

	void RobotInit()
	{
		//not used
	}

	void TeleopInit()
	{
		liftState = lowering;
		running = true;
		motorSpeed = -MOTOR_SPEED_DOWN;  //start by lowering the lift
		enterHoldCommand = false;
		exitHoldCommand = true;
		liftEncZeroed = false;
		liftEncFullRanged = false;
		maxLiftEncDist = UNINIT_VAL;
		pidPosSetPoint = UNINIT_VAL;
	}

	void TeleopPeriodic()
	{
		char myString [STAT_STR_LEN];

		if (running)
		{
			enterHoldCommand = joystick->GetRawButton(BUT_JS_ENT_POS_HOLD);
			exitHoldCommand = joystick->GetRawButton(BUT_JS_EXIT_POS_HOLD);

			switch (liftState)
			{
				case raising:
					if (GetLiftLimitSwitchMax())
					{
						SetLiftMotor(MOTOR_SPEED_STOP);
						if(!liftEncFullRanged)
						{
							maxLiftEncDist = liftEncoder->GetDistance();
							liftEncFullRanged = true;
						}
						motorSpeed = -MOTOR_SPEED_DOWN;
						liftState = lowering;
						SetLiftMotor(motorSpeed);
					}

					if (enterHoldCommand && liftEncZeroed && liftEncFullRanged)
					{
						liftState = holding;
					}

					break;

				case lowering:
					if (GetLiftLimitSwitchMin())
					{
						SetLiftMotor(MOTOR_SPEED_STOP);
						if(!liftEncZeroed)
						{
							liftEncoder->Reset();
							liftEncZeroed = true;
						}
						motorSpeed=MOTOR_SPEED_UP;
						liftState = raising;
						SetLiftMotor(motorSpeed);
					}

					if (enterHoldCommand && liftEncZeroed && liftEncFullRanged)
					{
						liftState = holding;
					}
					break;

				case holding:
					if(!(controlLift->IsEnabled()))
					{
						pidPosSetPoint = SP_RANGE_FRACTION*maxLiftEncDist; //go to the midpoint of the range
						controlLift->SetSetpoint(pidPosSetPoint);
#if BUILD_VERSION == COMPETITION
						controlLift2->SetSetpoint(pidPosSetPoint);
#endif
						controlLift->Enable();
#if BUILD_VERSION == COMPETITION
						controlLift2->Enable();
#endif
					}

					if(exitHoldCommand)
					{
						controlLift->Disable();
#if BUILD_VERSION == COMPETITION
						controlLift2->Disable();
#endif
						motorSpeed = -MOTOR_SPEED_DOWN;
						liftState = lowering;
						SetLiftMotor(motorSpeed);
					}
				break;
			}
		}

		//status
		sprintf(myString, "running: %d\n", running);
		SmartDashboard::PutString("DB/String 0", myString);
		sprintf(myString, "State: %d\n", liftState);
		SmartDashboard::PutString("DB/String 1", myString);
		sprintf(myString, "motorSpeed: %f\n", motorSpeed);
		SmartDashboard::PutString("DB/String 2", myString);
		sprintf(myString, "lift encoder zeroed: %d\n", liftEncZeroed);
		SmartDashboard::PutString("DB/String 3", myString);
		sprintf(myString, "max enc set: %d\n", liftEncFullRanged);
		SmartDashboard::PutString("DB/String 4", myString);
		sprintf(myString, "maxLiftEncDist: %f\n", maxLiftEncDist);
		SmartDashboard::PutString("DB/String 5", myString);
		sprintf(myString, "enc dist: %f\n", liftEncoder->GetDistance());
		SmartDashboard::PutString("DB/String 6", myString);
		sprintf(myString, "pid: %d\n", controlLift->IsEnabled());
		SmartDashboard::PutString("DB/String 7", myString);
		sprintf(myString, "dist to sp : %f\n", DistToSetpoint());
		SmartDashboard::PutString("DB/String 8", myString);
		sprintf(myString, "at sp : %d\n", AtSetpoint());
		SmartDashboard::PutString("DB/String 9", myString);
	}

	void TestPeriodic()
	{
		//not used
	}

public:
	Robot()
	{
#if BUILD_VERSION == COMPETITION
		liftMotor = new CANTalon(CHAN_LIFT_MOTOR);
		liftMotor2 = new CANTalon(CHAN_LIFT_MOTOR2);
#else
		liftMotor = new CANJaguar(CHAN_LIFT_MOTOR);
		liftMotor2 = new CANJaguar(CHAN_LIFT_MOTOR2);
#endif
		liftEncoder = new Encoder(CHAN_LIFT_ENCODER_A, CHAN_LIFT_ENCODER_B, false, Encoder::EncodingType::k4X);
		liftEncoder->SetDistancePerPulse(LIFT_ENCODER_DIST_PER_PULSE);
		liftEncoder->SetPIDSourceParameter(liftEncoder->kDistance);
#if BUILD_VERSION == COMPETITION
		liftEncoder->SetReverseDirection(true);
#endif
		controlLift = new PIDController(PID_P, PID_I, PID_D, liftEncoder, liftMotor);
		controlLift->SetContinuous(true); //treat input to controller as continuous; true by default
		controlLift->SetOutputRange(PID_OUT_MIN, PID_OUT_MAX);
		controlLift->Disable(); //do not enable until in holding position mode
		controlLift2 = new PIDController(PID_P, PID_I, PID_D, liftEncoder, liftMotor2);
		controlLift2->SetContinuous(true); //treat input to controller as continuous; true by default
		controlLift2->SetOutputRange(PID_OUT_MIN, PID_OUT_MAX);
		controlLift2->Disable(); //do not enable until in holding position mode
		liftLimitSwitchMin = new DigitalInput(CHAN_LIFT_LOW_LS);
		liftLimitSwitchMax = new DigitalInput(CHAN_LIFT_HIGH_LS);
		joystick = new Joystick(CHAN_JS);
	}

	~Robot()
	{
		delete liftEncoder;
		delete liftMotor;
		delete liftMotor2;
		delete controlLift;
		delete controlLift2;
		delete liftLimitSwitchMin;
		delete liftLimitSwitchMax;
		delete joystick;
	}
};

START_ROBOT_CLASS(Robot);
