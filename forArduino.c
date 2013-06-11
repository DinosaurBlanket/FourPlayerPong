const int Apotpin = A6;
const int Bpotpin = A4;
const int Cpotpin = A2;
const int Dpotpin = A0;

String Apot;
String Bpot;
String Cpot;
String Dpot;

String output;


void setup()
{
	Serial.begin(9600);
}

void loop()
{
	Apot = String( (analogRead(Apotpin)>>1) + 100 );
	Bpot = String( (analogRead(Bpotpin)>>1) + 100 );
	Cpot = String( (analogRead(Cpotpin)>>1) + 100 );
	Dpot = String( (analogRead(Dpotpin)>>1) + 100 );
	
	output = Apot + Bpot + Cpot + Dpot;
	
	if (Serial.available() > 0)
	{
		//Serial.read();
		Serial.println(output);
	}
	
	delay(10);
}