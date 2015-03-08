#include "WPILib.h"
#include "Robot.h"

class Robot: public IterativeRobot
{
private:
#if BUILD_VERSION == COMPETITION
	CANTalon *liftMotor;
#else
	CANJaguar *liftMotor;
#endif

	Encoder *liftEncoder;
	DigitalInput *liftLimitSwitchMin;
	DigitalInput *liftLimitSwitchMax;
	Joystick *joystick;

	bool initialZeroing; //initial zeroing but not yet zeroed
	double liftEncoderDistPerPulse = 1.0/LIFT_ENCODER_RESOLUTION;

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
		liftMotor->Set(LIFT_MOTOR_DIR*val);
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
		joystick = new Joystick(CHAN_JS);
		liftEncoder = new Encoder(CHAN_LIFT_ENCODER_LEFT_A, CHAN_LIFT_ENCODER_LEFT_B, false, Encoder::EncodingType::k4X);
		liftEncoder->SetDistancePerPulse(liftEncoderDistPerPulse);

#if BUILD_VERSION == COMPETITION
		liftMotor = new CANTalon(CHAN_LIFT_MOTOR);
#else
		liftMotor = new CANJaguar(CHAN_LIFT_MOTOR);
#endif
		liftLimitSwitchMin = new DigitalInput(CHAN_LIFT_LOW_LS);
		liftLimitSwitchMax = new DigitalInput(CHAN_LIFT_HIGH_LS);
		SetLiftMotor(-MOTOR_SPEED_GO); //move towards the bottom
		initialZeroing = true;
	}


	void TeleopPeriodic()
	{
		char myString [STAT_STR_LEN];

		if (initialZeroing)  // moving to the inner limit
		{
			sprintf(myString, "initZero ip\n"); //in progress
			SmartDashboard::PutString("DB/String 0", myString);
			if (GetLiftLimitSwitchMin())
			{
				SetLiftMotor(MOTOR_SPEED_STOP);
				liftEncoder->Reset();
				initialZeroing = false;
				sprintf(myString, "initZero comp\n"); //complete
				SmartDashboard::PutString("DB/String 0", myString);
			}
		}
		else  //manual control
		{
			//motor control
			if (joystick->GetRawButton(BUT_JS_UP) && !GetLiftLimitSwitchMax())  // move to the top
				SetLiftMotor(MOTOR_SPEED_GO);
			else if(joystick->GetRawButton(BUT_JS_DOWN) && !GetLiftLimitSwitchMin())  // move to the bottom
				SetLiftMotor(-MOTOR_SPEED_GO);
			else
				SetLiftMotor(MOTOR_SPEED_STOP); //stop

			//counter control
			if (joystick->GetRawButton(BUT_JS_RES_EN))  // reset the encoder
				liftEncoder->Reset();

		}

		//status
		sprintf(myString, "jsUp %d\n", joystick->GetRawButton(BUT_JS_UP));
		SmartDashboard::PutString("DB/String 1", myString);
		sprintf(myString, "jsDown %d\n", joystick->GetRawButton(BUT_JS_DOWN));
		SmartDashboard::PutString("DB/String 2", myString);
		sprintf(myString, "jsResEnc %d\n", joystick->GetRawButton(BUT_JS_RES_EN));
		SmartDashboard::PutString("DB/String 3", myString);
		sprintf(myString, "curr: %f\n", liftMotor->GetOutputCurrent());
		SmartDashboard::PutString("DB/String 4", myString);
		sprintf(myString, "enc dist: %d\n", liftEncoder->GetDistance()); //encoder distance
		SmartDashboard::PutString("DB/String 5", myString);
	}

	void TestPeriodic()
	{
		//not used
	}
};

START_ROBOT_CLASS(Robot);
