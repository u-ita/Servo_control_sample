const int VOL_PIN = A0;

void setup(){
    Serial.begin( 9600 );
}

void loop(){
    int value;
    int value2;

    value = analogRead( A0 );
    value2 = analogRead( A1 );

    //Serial.print( "A: " );
    //Serial.print( value );
    Serial.print( "  B: " );
    Serial.println( value2 );


    delay( 10 );
}
