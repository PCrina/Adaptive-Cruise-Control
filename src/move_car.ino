// connect motor controller pins to Arduino digital pins
int input = 0, motorSpeed = 0, chDirSpeed = 90;
char dir = 'f', ant;

// motor R
int enR = 10;
int in1 = 9;
int in2 = 8;

// motor L
int enL = 5;
int in3 = 7;
int in4 = 6;

void setup()
{
  // set all the motor control pins to outputs
  pinMode(enR, OUTPUT);
  pinMode(enL, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  Serial.begin(115200);
  delay(50);
  Serial.println("start");
}

void turnOnF()
{
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}

void turnOnB()
{
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void turnOff()
{
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void decelerate()
{
    for (int i = motorSpeed; i >= input; i--) {
        analogWrite(enR, i);
        analogWrite(enL, i);
        delay(10);
    }
}

void accelerate()
{
    for (int i = motorSpeed; i <= input; i++) {
        analogWrite(enR, i);
        analogWrite(enL, i);
        delay(10);
    }
}

void goLeft()
{
	for (int i = 0; i < chDirSpeed; i++) {
		analogWrite(enR, motorSpeed + i);
		analogWrite(enL, motorSpeed - i * 2);
		delay(10);
	}
  	for (int i = 0; i < chDirSpeed; i++) {
    	analogWrite(enL, motorSpeed + i);
    	analogWrite(enR, motorSpeed - i * 2);
    	delay(10);
  	}
}

void goRight()
{
	for (int i = 0; i < chDirSpeed; i++) {
		analogWrite(enL, motorSpeed + i);
		analogWrite(enR, motorSpeed - i * 2);
		delay(10);
	}
  	for (int i = 0; i < chDirSpeed; i++) {
    	analogWrite(enR, motorSpeed + i);
    	analogWrite(enL, motorSpeed - i * 2);
    }
}

void constantSpeed()
{
    analogWrite(enR, motorSpeed);
    analogWrite(enL, motorSpeed);
}

void move()
{
  // this function will run the motors in both directions at a given speed
  if (input == 0) {
	 // turn off motors
	 turnOff();
  } else {

		switch(dir) {
			case 'f':
				    // turn on motors forward
        		turnOnF();
        		break;
        	case 'b':
        		// turn on motors backward
        		turnOnB();
        		break;
        	case 'a':
        		// go left
        		goLeft();
            	dir = ant;
        		break;
        	case 'd':
        		// go right
        		goRight();
            	dir = ant;
        		break;
        	default:
        		Serial.println("Invalid direction");
        }

        if (input < motorSpeed) {
			       // decelerate from motorSpeed to input
            decelerate();
        } else if (input > motorSpeed) {
            // accelerate from motorSpeed to input
            accelerate();
        } else {
            // maintain constant speed
            constantSpeed();
        }

        motorSpeed = input;
    }
}

char rx_byte = 0;
String rx_str = "";
boolean not_number = false;

void loop()
{
  // receive commands
  if (Serial.available() > 0) {

    rx_byte = Serial.read();

    // check if number
    if ((rx_byte >= '0') && (rx_byte <= '9')) {
      rx_str += rx_byte;
    } else if (rx_byte == '\n') {

        // received number(speed)
        if(!not_number) {
          input = rx_str.toInt();
        }

        not_number = false;
        rx_str = "";

    } else {
        // received direction(f or b)
        not_number = true;
        ant = dir;
        dir = rx_byte;
    }
  }

  // move car
  move();
}
